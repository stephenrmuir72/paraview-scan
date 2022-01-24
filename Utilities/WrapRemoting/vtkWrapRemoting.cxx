/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapRemoting.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLogger.h"
#include "vtkParseData.h"
#include "vtkParseExtras.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkParseType.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <algorithm> // for std::find
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h> // for Sleep()
#endif

namespace
{
struct vtkGlobals
{

  /**
   * String cache is used to create strings that will be released when exiting
   * the program
   */
  StringCache* StringsCache;

  int TempVariableCounter;

  vtkGlobals()
  {
    TempVariableCounter = 0;
    this->StringsCache = (StringCache*)(std::malloc(sizeof(StringCache)));
    vtkParse_InitStringCache(this->StringsCache);
  }

  ~vtkGlobals()
  {
    vtkParse_FreeStringCache(this->StringsCache);
    std::free(this->StringsCache);
  }
};

}
const char* GENERATED_INTERNALLY_STR = "GENERATED_INTERNALLY";
vtkGlobals* Globals = nullptr;

/**
 * Notes:
 * - For std::array and std::vector we  store the element type by combining two flags bitwise.
 * - Property types are constrained to only a few ones:
 *   see https://gitlab.kitware.com/vtk/remoting/-/issues/39
 */
enum RemotingParserTypes
{
  REMOTING_PARSER_INVALID_TYPE = 0,
  REMOTING_PARSER_VOID_TYPE,
  REMOTING_PARSER_BOOL_TYPE,
  REMOTING_PARSER_INT32_TYPE,
  REMOTING_PARSER_INT64_TYPE,
  REMOTING_PARSER_DOUBLE_TYPE,
  REMOTING_PARSER_STD_STRING_TYPE,
  REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION,
  REMOTING_PARSER_VTK_OBJECT,
  REMOTING_PARSER_STD_ARRAY_TYPE = 0x100,
  REMOTING_PARSER_STD_VECTOR_TYPE = 0x200,
};

//------------------------------------------------------------------------------
unsigned int vtkWrapRemoting_GetElementType(unsigned int parserType)
{
  const unsigned int ELEMENT_MASK = 0x0FF;
  return parserType & ELEMENT_MASK;
}

//------------------------------------------------------------------------------
unsigned int vtkWrapRemoting_GetContainerType(unsigned int parserType)
{
  const unsigned int CONTAINER_MASK = 0xF00;
  return parserType & CONTAINER_MASK;
}

const char* vtkWrapRemoting_GetTypeName(unsigned int parserType)
{
  parserType = vtkWrapRemoting_GetElementType(parserType);
  const char* returnValue = "UNKNOWN_TYPE";
  if (parserType == REMOTING_PARSER_INVALID_TYPE)
  {
    returnValue = "INVALID_TYPE";
  }
  else if (parserType == REMOTING_PARSER_VOID_TYPE)
  {
    returnValue = "void";
  }
  else if (parserType == REMOTING_PARSER_BOOL_TYPE)
  {
    returnValue = "bool";
  }
  else if (parserType == REMOTING_PARSER_INT32_TYPE)
  {
    returnValue = "int32_t";
  }
  else if (parserType == REMOTING_PARSER_INT64_TYPE)
  {
    returnValue = "int64_t";
  }
  else if (parserType == REMOTING_PARSER_DOUBLE_TYPE)
  {
    returnValue = "double";
  }
  else if (parserType == REMOTING_PARSER_STD_STRING_TYPE)
  {
    returnValue = "std::string";
  }
  else if (parserType == REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION)
  {
    returnValue = "vtkDataArraySelection";
  }

  return returnValue;
}

//------------------------------------------------------------------------------
int vtkWrapRemoting_IsStdArray(ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN && val->Class &&
    strncmp(val->Class, "std::array<", 11) == 0);
}

int vtkWrapRemoting_IsStdString(ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_STRING);
}

//------------------------------------------------------------------------------
/**
 * Create a copy of \p source using the string cache
 */
char* vtkWrapRemoting_CreateStringCopy(const char* source)
{
  unsigned long n = strlen(source);
  char* result = vtkParse_NewString(Globals->StringsCache, n);
  strncpy(result, source, n);
  result[n] = '\0';
  return result;
}
//------------------------------------------------------------------------------

const char* vtkWrapRemoting_CreateUniqueTemporaryVariableName()
{
  std::string varName = "temp";
  varName.append(std::to_string(Globals->TempVariableCounter));
  Globals->TempVariableCounter++;
  return vtkWrapRemoting_CreateStringCopy(varName.c_str());
}

//------------------------------------------------------------------------------
/**
 * Open file with error handling
 */
FILE* vtkWrapRemoting_OpenFile(const char* filename, const char* mode)
{
  FILE* fp = std::fopen(filename, mode);
#ifdef _WIN32
  if (!fp)
  {
    /* repeatedly try to open output file in case of access/sharing error */
    /* (for example, antivirus software might be scanning the output file) */
    int tries;
    for (tries = 0; !fp && tries < 5 && errno == EACCES; tries++)
    {
      Sleep(1000);
      fp = std::fopen(filename, mode);
    }
  }
#endif

  if (!fp)
  {
    int e = errno;
    const char* etext = std::strerror(e);
    etext = (etext ? etext : "Unknown error");
    vtkLogF(ERROR, "Error %d opening output file %s: %s", e, filename, etext);
    exit(1);
  }

  return fp;
}
//------------------------------------------------------------------------------
/**
 * Query the size of the Interger type held by \p val
 */
int vtkWrapRemoting_GetIntegerSize(ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);

  if (t != VTK_PARSE_UNSIGNED_CHAR)
  {
    t = (t & ~VTK_PARSE_UNSIGNED);
  }
  switch (t)
  {
    case VTK_PARSE_SHORT:
      return sizeof(short);
    case VTK_PARSE_INT:
      return sizeof(int);
    case VTK_PARSE_LONG:
      return sizeof(long);
    case VTK_PARSE_LONG_LONG:
      return sizeof(long long);
    case VTK_PARSE___INT64:
      return sizeof(int64_t);
    case VTK_PARSE_UNSIGNED_CHAR:
      return sizeof(unsigned char);
    case VTK_PARSE_SIGNED_CHAR:
      return sizeof(char);
    case VTK_PARSE_SSIZE_T:
      return sizeof(size_t);
  }

  return 0;
}

//------------------------------------------------------------------------------
/**
 * Get the type of a of the first argument of a templated type as string.
 * Example std::vector<double> -> double
 * It works type we have to convert \p val to be easier to pack with the parser
 */
const char* vtkWrapRemoting_GetFirstTemplateType(const char* className)
{

  // find value between < , . or between <>
  std::string type = className;
  std::string::size_type first = type.find('<');
  std::string::size_type last = type.find(',');
  if (last == std::string::npos)
  {
    last = type.find('>');
  }
  first++;
  std::string templateType = type.substr(first, last - first);
  // remove spaces to make pointer parsing easier
  templateType.erase(
    std::remove(templateType.begin(), templateType.end(), ' '), templateType.end());

  return vtkWrapRemoting_CreateStringCopy(templateType.c_str());
}

//------------------------------------------------------------------------------
/**
 * Get the type we have to convert \p val to be easier to pack with the parser
 */
unsigned int vtkWrapRemoting_CorrespondingParserType(ValueInfo* val) // NOLINT(misc-no-recursion)
{
  unsigned int parserType = REMOTING_PARSER_INVALID_TYPE;
  if (vtkWrap_IsVoid(val))
  {
    parserType = REMOTING_PARSER_VOID_TYPE;
  }
  else if (vtkWrapRemoting_IsStdString(val) || vtkWrap_IsCharPointer(val) ||
    vtkWrap_IsTypeOf(nullptr, val->Class, "std::string"))
  {
    parserType = REMOTING_PARSER_STD_STRING_TYPE;
  }
  else if (vtkWrap_IsArray(val) && val->Count > 0)
  {
    parserType = REMOTING_PARSER_STD_ARRAY_TYPE;
    auto* elementValue = (ValueInfo*)(std::malloc(sizeof(ValueInfo)));
    vtkParse_CopyValue(elementValue, val);
    elementValue->Type &= VTK_PARSE_BASE_TYPE;
    // NOLINTNEXTLINE(misc-no-recursion)
    parserType = parserType | vtkWrapRemoting_CorrespondingParserType(elementValue);
    vtkParse_FreeValue(elementValue);
  }
  else if (vtkWrap_IsBool(val))
  {
    parserType = REMOTING_PARSER_BOOL_TYPE;
  }
  else if (vtkWrap_IsScalar(val) && vtkWrap_IsRealNumber(val))
  {
    parserType = REMOTING_PARSER_DOUBLE_TYPE;
  }
  // else if (vtkWrap_IsScalar(val) && vtkWrap_IsInteger(val)) IsInterget returns false for
  // VTK_PARSE_CHAR
  else if (vtkWrap_IsScalar(val) && vtkWrap_IsNumeric(val) && !vtkWrap_IsRealNumber(val))
  {
    int size = vtkWrapRemoting_GetIntegerSize(val);
    if (size <= 4)
    {
      parserType = REMOTING_PARSER_INT32_TYPE;
    }
    else
    {
      parserType = REMOTING_PARSER_INT64_TYPE;
    }
  }
  else if (vtkWrap_IsStdVector(val))
  {
    parserType = REMOTING_PARSER_STD_VECTOR_TYPE;
    // NOLINTNEXTLINE(misc-no-recursion)
    parserType = parserType | vtkWrapRemoting_CorrespondingParserType(val->Template->Parameters[0]);
  }
  else if (vtkWrap_IsTypeOf(nullptr, val->Class, "vtkDataArraySelection"))
  {
    parserType = REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION;
  }
  else if (vtkWrap_IsTypeOf(nullptr, val->Class, "vtkAlgorithmOutput"))
  {
    parserType = REMOTING_PARSER_VTK_OBJECT;
  }
  else if (vtkWrap_IsTypeOf(nullptr, val->Class,
             "vtkPVDataRepresentation*")) // FIXME we should handle pointers separately
  {
    parserType = REMOTING_PARSER_VTK_OBJECT;
  }
  return parserType;
}

//------------------------------------------------------------------------------
/**
 *  Get a VTK type that corresponds to a ParserType.  This is mainly
 *  intented to allow comparison of scalar types between the two formats.
 */
unsigned int vtkWrapRemoting_CorrespondingVTKType(unsigned int parserType)
{
  unsigned int vtktype = VTK_PARSE_UNKNOWN;
  if (parserType == REMOTING_PARSER_VOID_TYPE)
  {
    vtktype = VTK_PARSE_VOID;
  }
  else if (parserType == REMOTING_PARSER_BOOL_TYPE)
  {
    vtktype = VTK_PARSE_BOOL;
  }
  else if (parserType == REMOTING_PARSER_INT32_TYPE)
  {
    vtktype = VTK_PARSE_INT;
  }
  else if (parserType == REMOTING_PARSER_INT64_TYPE)
  {
    vtktype = VTK_PARSE___INT64;
  }
  else if (parserType == REMOTING_PARSER_DOUBLE_TYPE)
  {
    vtktype = VTK_PARSE_DOUBLE;
  }
  else if (parserType == REMOTING_PARSER_STD_STRING_TYPE)
  {
    vtktype = VTK_PARSE_STRING;
  }
  else if (parserType == REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION)
  {
    vtktype = VTK_PARSE_VTK_OBJECT;
  }
  else if (parserType == REMOTING_PARSER_INVALID_TYPE ||
    vtkWrapRemoting_GetContainerType(parserType) == REMOTING_PARSER_STD_ARRAY_TYPE ||
    vtkWrapRemoting_GetContainerType(parserType) == REMOTING_PARSER_STD_VECTOR_TYPE)
  {
    vtktype = VTK_PARSE_UNKNOWN;
  }
  return vtktype;
}

//------------------------------------------------------------------------------
/**
 * @brief Convert from Parser-safe types to the type expected by VTK.
 *
 * @param fp  the file we are curretly writting to
 * @param parserType type of the Parser-safe variable used in the generated API
 * @param value type of variable expected by the VTK API
 * @param parserVariableName name of the variable to convert from
 * @param vtkVariableName name of the variable to convert to
 *
 * @sa vtkWrapRemoting_ConvertToParserType
 */
void vtkWrapRemoting_ConvertToVTKType(FILE* fp, unsigned int parserType, ValueInfo* value,
  const char* parserVariableName, const char* vtkVariableName)
{
  if (parserType == REMOTING_PARSER_STD_STRING_TYPE && vtkWrap_IsCharPointer(value))
  {
    const char* tmp = vtkWrapRemoting_CreateUniqueTemporaryVariableName();
    std::fprintf(
      fp, "    std::string %s = vtk::variant_cast<std::string>(%s);\n", tmp, parserVariableName);
    std::fprintf(fp, "    const char* %s = %s.c_str();\n", vtkVariableName, tmp);
  }
  else if (parserType == REMOTING_PARSER_STD_STRING_TYPE && vtkWrap_IsString(value))
  {
    const char* tmp = vtkWrapRemoting_CreateUniqueTemporaryVariableName();
    std::fprintf(
      fp, "    std::string %s = vtk::variant_cast<std::string>(%s);\n", tmp, parserVariableName);
    std::fprintf(fp, "    auto %s = std::move(%s);\n", vtkVariableName, tmp);
  }
  else if (vtkWrapRemoting_GetContainerType(parserType) == REMOTING_PARSER_STD_ARRAY_TYPE)
  {
    const unsigned int vtkElementType = value->Type & VTK_PARSE_BASE_TYPE;
    const char* vtkElementTypeName = vtkWrap_GetTypeName(value);
    const char* parserElementTypeName = vtkWrapRemoting_GetTypeName(parserType);

    // FIXME parserVariableName contains already index in the name and we need to remove it
    // or update every other case above
    std::string tmp = parserVariableName;
    tmp.resize(tmp.length() - 3);
    const char* parserVariableName2 = tmp.c_str();

    if (vtkWrap_IsNumeric(value))
    {
      const int count = value->Count;
      std::fprintf(fp,
        "    %s %s[%d];\n"
        "    for(int i = 0;  i < %d; i++)\n"
        "    {\n"
        "      %s[i] = vtk::variant_cast<%s>(%s[i]);\n"
        "    }\n",
        vtkElementTypeName, vtkVariableName, count, count, vtkVariableName, parserElementTypeName,
        parserVariableName2);
    }
    else
    {
      const char* parserElementTypeName = vtkWrapRemoting_GetTypeName(parserType);
      vtkLogF(ERROR, "Unknown conversion from %s to %s", vtkElementTypeName, parserElementTypeName);
    }
  }
  else if (vtkWrapRemoting_GetContainerType(parserType) == REMOTING_PARSER_STD_VECTOR_TYPE)
  {
    const char* vtkElementTypeName = vtkWrap_GetTypeName(value);
    const char* parserElementTypeName = nullptr;
    if (vtkWrapRemoting_GetElementType(parserType) == REMOTING_PARSER_VTK_OBJECT)
    {
      parserElementTypeName = vtkWrapRemoting_GetFirstTemplateType(value->Class);
    }
    else
    {
      parserElementTypeName = vtkWrapRemoting_GetTypeName(parserType);
    }

    // FIXME parserVariableName contains already index in the name and we need to remove it
    // or update every other case above
    std::string tmp = parserVariableName;
    tmp.resize(tmp.length() - 3);
    const char* parserVariableName2 = tmp.c_str();
    std::fprintf(fp,
      "    std::vector<%s> %s;\n"
      "    for(const auto& item : %s)\n"
      "    {\n"
      "      %s.push_back(vtk::variant_cast<%s>(item));\n"
      "    }\n",
      parserElementTypeName, vtkVariableName, parserVariableName2, vtkVariableName,
      parserElementTypeName);
  }
  else if (parserType == REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION)
  {
    const char* vtkElementTypeName = vtkWrap_GetTypeName(value);
    std::fprintf(fp, "    vtkDataArraySelection* %s = %s.GetPointer();\n", vtkVariableName,
      parserVariableName);
  }
  else if (parserType == REMOTING_PARSER_VTK_OBJECT)
  {
    const char* vtkElementTypeName = vtkWrap_GetTypeName(value);
    std::fprintf(fp, "    %s* %s = vtk::variant_cast<%s*>(%s);\n", vtkElementTypeName,
      vtkVariableName, vtkElementTypeName, parserVariableName);
  }
  else if (parserType == REMOTING_PARSER_INT32_TYPE || parserType == REMOTING_PARSER_INT64_TYPE ||
    parserType == REMOTING_PARSER_DOUBLE_TYPE || parserType == REMOTING_PARSER_BOOL_TYPE)
  {
    // cast to resticted type
    const char* parserElementTypeName = vtkWrapRemoting_GetTypeName(parserType);
    const char* vtkElementTypeName = vtkWrap_GetTypeName(value);
    const char* tmp = vtkWrapRemoting_CreateUniqueTemporaryVariableName();
    std::fprintf(fp, "    auto %s = vtk::variant_cast<%s>(%s);\n", tmp, parserElementTypeName,
      parserVariableName);
    // cast expected VTK type
    std::fprintf(
      fp, "    auto %s = static_cast<%s>(%s);\n", vtkVariableName, vtkElementTypeName, tmp);
    const char* elementTypeName = vtkWrapRemoting_GetTypeName(parserType);
  }
  else
  {
    std::fprintf(fp, "  UNSUPPORTED REMOTING_PARSER TYPE\n");
  }
}

//------------------------------------------------------------------------------
/**
 * Generate code that converts a variable of type \p value and name \p
 * vtkVariableName to a REMOTING_PARSER-safe variable named \p parserVariableName.
 * This allows to restrict the supported types on only a smal set and avoid
 * packing raw pointers.
 */
void vtkWrapRemoting_ConvertToParserType(
  FILE* fp, ValueInfo* value, const char* vtkVariableName, const char* parserVariableName)
{
  unsigned int parserType = vtkWrapRemoting_CorrespondingParserType(value);

  if (parserType == REMOTING_PARSER_INT32_TYPE)
  {
    std::fprintf(
      fp, "    auto %s = static_cast<int32_t>(%s);\n", parserVariableName, vtkVariableName);
  }
  else if (parserType == REMOTING_PARSER_INT64_TYPE)
  {
    std::fprintf(
      fp, "    auto %s = static_cast<int64_t>(%s);\n", parserVariableName, vtkVariableName);
  }
  else if (parserType == REMOTING_PARSER_DOUBLE_TYPE)
  {
    std::fprintf(
      fp, "    auto %s = static_cast<double>(%s);\n", parserVariableName, vtkVariableName);
  }
  else if (parserType == REMOTING_PARSER_BOOL_TYPE || parserType == REMOTING_PARSER_VTK_OBJECT ||
    (parserType == REMOTING_PARSER_STD_STRING_TYPE && vtkWrapRemoting_IsStdString(value)))
  {
    // no conversion
    std::fprintf(fp, "    auto %s = %s;\n", parserVariableName, vtkVariableName);
  }
  else if (vtkWrapRemoting_GetContainerType(parserType) == REMOTING_PARSER_STD_ARRAY_TYPE)
  {
    const char* elementTypeName = vtkWrapRemoting_GetTypeName(parserType);
    std::fprintf(fp,
      "    %s %s[%d];\n"
      "    for (int i = 0; i < %d; i++)\n"
      "    {\n"
      "      %s[i] = static_cast<%s>(%s[i]);\n"
      "    }\n",
      elementTypeName, parserVariableName, value->Count, value->Count, parserVariableName,
      elementTypeName, vtkVariableName);
  }
  else if (vtkWrapRemoting_GetContainerType(parserType) == REMOTING_PARSER_STD_VECTOR_TYPE)
  {
    const char* elementTypeName = vtkWrapRemoting_GetTypeName(parserType);
    std::fprintf(fp,
      "    std::vector<%s> %s;\n"
      "    for (const auto& item : %s)\n"
      "    {\n"
      "      %s.emplace_back(item);\n"
      "    }\n",
      elementTypeName, parserVariableName, vtkVariableName, parserVariableName);
  }
  else if (parserType == REMOTING_PARSER_STD_STRING_TYPE && vtkWrap_IsCharPointer(value))
  {
    std::fprintf(fp, "    std::string %s;\n", parserVariableName);
    std::fprintf(fp, "    if(%s)\n", vtkVariableName);
    std::fprintf(fp, "    {\n");
    std::fprintf(fp, "      %s.assign(%s);\n", parserVariableName, vtkVariableName);
    std::fprintf(fp, "    }\n");
  }
  else if (parserType == REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION)
  {
    std::fprintf(fp, "    vtkSmartPointer<vtkDataArraySelection> %s(%s);\n", parserVariableName,
      vtkVariableName);
  }
  else
  {
    std::string type;
    if (vtkWrap_IsConst(value))
    {
      type.append("const ");
    }
    type.append(vtkWrap_GetTypeName(value));
    if (vtkWrap_IsPointer(value))
    {
      type.push_back('*');
    }
    vtkLogF(ERROR, "Conversion code for %s is missing !", type.c_str());
  }
}
//------------------------------------------------------------------------------
/**
 * @brief Create a ValueInfo object based on a given name.
 *
 * Normally the parser will populate ValueInfo's fields. However, if we create
 * out own functions we need to fill the minimum information by ourlselves.
 * This function "parses" \p className and produces the corresponding
 * ValueInfo. The type are extremly limited and cover only what is supported by
 * RemotingParserType enum.
 */
ValueInfo* vtkWrapRemoting_CreateTypeFromName(const char* className) // NOLINT(misc-no-recursion)
{
  auto* parameter = (ValueInfo*)(std::malloc(sizeof(ValueInfo)));
  vtkParse_InitValue(parameter);
  parameter->Class = vtkWrapRemoting_CreateStringCopy(className);
  parameter->Type = VTK_PARSE_UNKNOWN;
  if (!strcmp(className, "void"))
  {
    parameter->Type = VTK_PARSE_VOID;
  }
  else if (!strcmp(className, "bool"))
  {
    parameter->Type = VTK_PARSE_BOOL;
  }
  else if (!strcmp(className, "int32_t"))
  {
    parameter->Type = VTK_PARSE_INT;
  }
  else if (!strcmp(className, "int64_t"))
  {
    parameter->Type = VTK_PARSE___INT64;
  }
  else if (!strcmp(className, "double"))
  {
    parameter->Type = VTK_PARSE_DOUBLE;
  }
  else if (!strcmp(className, "std::string"))
  {
    parameter->Type = VTK_PARSE_STRING;
  }
  else if (!strcmp(className, "vtkDataArraySelection") ||
    !strcmp(className, "vtkAlgorithmOutputValue") || !strcmp(className, "vtkPVDataRepresentation*"))
  {
    parameter->Type = VTK_PARSE_VTK_OBJECT;
  }
  else if (vtkWrap_IsStdVector(parameter))
  {
    // we need some minimal parsing to find the template type and set the template_parameter

    const char* templateElementType = vtkWrapRemoting_GetFirstTemplateType(parameter->Class);

    parameter->Template = (TemplateInfo*)(std::malloc(sizeof(TemplateInfo)));
    vtkParse_InitTemplate(parameter->Template);

    // NOLINTNEXTLINE(misc-no-recursion)
    auto* template_parameter = vtkWrapRemoting_CreateTypeFromName(templateElementType);
    parameter->Template->Parameters = (ValueInfo**)(std::malloc(sizeof(ValueInfo*)));
    parameter->Template->Parameters[parameter->Template->NumberOfParameters++] = template_parameter;
  }
  else
  {
    // vtkLogF(WARNING, "Creating variable of Class %s with unknown type !", className);
  }
  return parameter;
}

//------------------------------------------------------------------------------
/**
 * Check \p val to see if it is wrappable
 */
int vtkWrapRemoting_IsValueWrappable(ValueInfo*& val)
{
  if (vtkWrap_IsVoid(val))
  {
    return 1;
  }
  if (vtkWrap_IsScalar(val) && vtkWrap_IsNumeric(val))
  {
    return 1;
  }
  if (vtkWrap_IsArray(val) && val->Count > 0 && vtkWrap_IsNumeric(val))
  {
    return 1;
  }
  // FIXME
  // arrays with size specified at runtime are not supported for now
  if (vtkWrap_IsArray(val) && val->CountHint != nullptr && vtkWrap_IsNumeric(val))
  {
    return 0;
  }
  if (val->Class && !strcmp(val->Class, "vtkStdString"))
  {
    return 0;
  }
  if (vtkWrapRemoting_IsStdString(val) || vtkWrap_IsCharPointer(val))
  {
    return 1;
  }
  if (vtkWrap_IsTypeOf(nullptr, val->Class, "vtkAlgorithmOutput"))
  {
    return 1;
  }
  if (vtkWrap_IsTypeOf(
        nullptr, val->Class, "vtkPVDataRepresentation*")) // FIXME we should remove pointers
  {
    return 1;
  }
  if (vtkWrap_IsTypeOf(nullptr, val->Class, "vtkDataArraySelection"))
  {
    return 0;
  }
  if (vtkWrap_IsStdVector(val))
  {
    if (!val->Template)
    {
      // FIXME: parser didn't gave us enough information. Let try ourselves
      ValueInfo* enrichedValue = vtkWrapRemoting_CreateTypeFromName(val->Class);
      std::free(val);
      val = enrichedValue;
      int result = vtkWrapRemoting_IsValueWrappable(enrichedValue);
      return result;
    }
    else
    {
      return vtkWrapRemoting_IsValueWrappable(val->Template->Parameters[0]);
    }
  }

  return 0;
}
//------------------------------------------------------------------------------
/**
 * A simple check based on function name and type of arguments Specifically,
 * function should have the following signatures: void Set##(ValueType) or
 * ValueType Get##() i.e. Setters can have one or more one arguments.
 * getter have no arguments and a single return value
 */
/** @{*/
int vtkWrapRemoting_IsGetter(FunctionInfo* func)
{
  if (func->Name && strlen(func->Name) > 3 && (!strncmp(func->Name, "Get", 3)))
  {
    if (func->NumberOfParameters == 0)
    {
      return 1;
    }
    return 0;
  }
  return 0;
}
//------------------------------------------------------------------------------
int vtkWrapRemoting_IsSetter(FunctionInfo* func)
{
  if (func->Name && strlen(func->Name) > 3 && (!strncmp(func->Name, "Set", 3)))
  {
    if (func->NumberOfParameters > 0)
    {
      return 1;
    }
    return 0;
  }
  return 0;
}
//------------------------------------------------------------------------------
int vtkWrapRemoting_IsGetterOrSetter(FunctionInfo* func)
{
  return vtkWrapRemoting_IsGetter(func) || vtkWrapRemoting_IsSetter(func);
}
/** @}*/

//------------------------------------------------------------------------------
/**
 * Assign the value of \p variableBaseName to an object of type \p className
 * using \p theFunc. This function can also handle internally generated
 * functions created by vtkWrapRemoting_GenerateSpecialMethods
 */
void vtkWrapRemoting_AssignToInternalObject(
  FILE* fp, const char* variableBaseName, FunctionInfo* theFunc)
{
  // Internally generated functions  need special care
  if (theFunc->Macro && !strcmp(theFunc->Macro, GENERATED_INTERNALLY_STR))
  {
    ValueInfo* val = theFunc->Parameters[0];
    std::string variableNameSTD = variableBaseName;
    variableNameSTD.append("0");
    const char* variableName = variableNameSTD.c_str();
    if (vtkWrap_IsTypeOf(nullptr, val->Class, "vtkDataArraySelection"))
    {
      char* getter_name = vtkWrapRemoting_CreateStringCopy(theFunc->Name);
      getter_name[0] = 'G';
      std::fprintf(fp, "  target->%s()->DeepCopy(%s);\n", getter_name, variableName);
    }
    else if (!strcmp(theFunc->Name, "SetFileNames"))
    {
      std::fprintf(fp,
        "    target->ClearFileNames();\n"
        "    for (const auto& name : %s)\n"
        "    {\n"
        "      target->AddFileName(name.c_str());\n"
        "    }\n",
        variableName);
    }
    else
    {
      // assign by calling the  setter
      std::fprintf(fp, "    target->%s(%s%d);\n", theFunc->Name, variableName, 0);
    }
  }
  else if (theFunc->NumberOfParameters > 1)
  {
    std::fprintf(fp, "    target->%s(%s%d", theFunc->Name, variableBaseName, 0);
    for (int i = 1; i < theFunc->NumberOfParameters; i++)
    {
      std::fprintf(fp, ", %s%d", variableBaseName, i);
    }
    std::fprintf(fp, ");\n");
  }
  else
  {
    // assign by calling the  setter
    std::fprintf(fp, "    target->%s(%s%d);\n", theFunc->Name, variableBaseName, 0);
  }
}

//------------------------------------------------------------------------------
/**
 * Get value from the internal object using the return value of \p theFunc and
 * store it in \p variableOutName
 */
void vtkWrapRemoting_AssignFromInternalObject(
  FILE* fp, FunctionInfo* theFunc, const char* variableOutName)
{
  unsigned int parserType = vtkWrapRemoting_CorrespondingParserType(theFunc->ReturnValue);
  // this is a placeholder for all functions generated by vtkWrapRemoting_GenerateSpecialMethods
  // due to their nature they may require custom code to get the value
  if (theFunc->Macro && !strcmp(theFunc->Macro, GENERATED_INTERNALLY_STR))
  {
    if (parserType == REMOTING_PARSER_VTK_DATA_ARRAY_SELECTION)
    {
      // nothing special
      std::fprintf(fp, "  auto vtkValue = target->%s();\n", theFunc->Name);
      vtkWrapRemoting_ConvertToParserType(fp, theFunc->ReturnValue, "vtkValue", variableOutName);
    }
    else if (!strcmp(theFunc->Name, "GetFileNames"))
    {
      std::fprintf(fp,
        "    std::vector<std::string> %s;\n"
        "    %s.reserve(target->GetNumberOfFileNames());\n"
        "    for (int i = 0 ; i < target->GetNumberOfFileNames(); i++)\n"
        "    {\n"
        "      %s.emplace_back(target->GetFileName(i));\n"
        "    }\n",
        variableOutName, variableOutName, variableOutName);
    }
    else if (!strcmp(theFunc->Name, "GetFileName"))
    {
      std::fprintf(fp, "const auto* vtkValue = target->GetFileName(0);\n");
      vtkWrapRemoting_ConvertToParserType(fp, theFunc->ReturnValue, "vtkValue", variableOutName);
    }
  }
  else
  {
    std::fprintf(fp, "    "); // indentation

    // Create a variable to hold the result
    // add auto*/const auto* as needed to avoid clang-tidy warnings
    if (vtkWrap_IsPointer(theFunc->ReturnValue) || vtkWrap_IsArray(theFunc->ReturnValue))
    {
      if (vtkWrap_IsConst(theFunc->ReturnValue))
      {
        std::fprintf(fp, "const ");
      }
      std::fprintf(fp, "auto* vtkValue = ");
    }
    else
    {
      std::fprintf(fp, "auto vtkValue = ");
    }

    std::fprintf(fp, "target->%s();\n", theFunc->Name);

    // create the return variable
    vtkWrapRemoting_ConvertToParserType(fp, theFunc->ReturnValue, "vtkValue", variableOutName);
  }
}

//------------------------------------------------------------------------------
int vtkWrapRemoting_GetArgumentCount(ValueInfo* valueInfo)
{
  if (vtkWrap_IsScalar(valueInfo) || vtkWrap_IsVTKObject(valueInfo) ||
    vtkWrap_IsCharPointer(valueInfo)) // char* is returned as a single std::string
  {
    return 1;
  }
  else if (vtkWrap_IsArray(valueInfo) && valueInfo->Count > 0)
  {
    return valueInfo->Count;
  }
  return 0;
}

//------------------------------------------------------------------------------
/**
 * Generate Code for a method of the wrapper class.
 */
void vtkWrapRemoting_GenerateOneMethod(FILE* fp, ClassInfo* classInfo, FunctionInfo* theFunc)
{
  const char* classname = classInfo->Name;
  if (vtkWrapRemoting_IsSetter(theFunc))
  {
    int count = theFunc->NumberOfParameters;
    if (count == 1 && vtkWrap_IsStdVector(theFunc->Parameters[0]))
    {
      std::fprintf(fp, "  if (method == \"%s\" && args.size() > 0 )\n", theFunc->Name);
    }
    else
    {
      std::fprintf(fp, "  if (method == \"%s\" && args.size() == %d)\n", theFunc->Name, count);
    }

    std::fprintf(fp, "  {\n");
    std::fprintf(fp, "    vtkRLogF(\"%s(%%p)::%s\", object);\n", classname, theFunc->Name);

    for (int i = 0; i < theFunc->NumberOfParameters; i++)
    {

      unsigned int parserType = vtkWrapRemoting_CorrespondingParserType(theFunc->Parameters[i]);
      // create a variable that matches the VTK API that will hold the value.
      std::string varName = "args[" + std::to_string(i) + "]";
      std::string vtkVarName = "value" + std::to_string(i);
      vtkWrapRemoting_ConvertToVTKType(
        fp, parserType, theFunc->Parameters[i], varName.c_str(), vtkVarName.c_str());
      // assign to real object calling the actual setter
    }
    vtkWrapRemoting_AssignToInternalObject(fp, "value", theFunc);

    std::fprintf(fp, "    return true;\n");
    std::fprintf(fp, "  }\n");
  }
  else if (vtkWrapRemoting_IsGetter(theFunc))
  {
    std::fprintf(fp, "  if (method == \"%s\")\n", theFunc->Name);

    std::fprintf(fp, "  {\n");
    std::fprintf(fp, "    vtkRLogF(\"%s(%%p)::%s\", object);\n", classname, theFunc->Name);
    vtkWrapRemoting_AssignFromInternalObject(fp, theFunc, "convertedValue");

    int count = vtkWrapRemoting_GetArgumentCount(theFunc->ReturnValue);
    if (vtkWrap_IsStdVector(theFunc->ReturnValue))
    {
      std::fprintf(fp,
        "    for(const auto& item : convertedValue)\n"
        "    {\n"
        "      args.emplace_back(item);\n"
        "    }\n");
    }
    else if (count == 0)
    {
      std::fprintf(fp, "    INVALID SIZE\n");
    }
    else if (count == 1)
    {
      std::fprintf(fp, "    args.emplace_back(convertedValue);\n");
    }
    else
    {
      std::fprintf(fp,
        "    for(int i = 0 ; i <  %d; i++)\n"
        "    {\n"
        "      args.emplace_back(convertedValue[i]);\n"
        "    }\n",
        count);
    }

    std::fprintf(fp, "    return true;\n");
    std::fprintf(fp, "  }\n");
  }
  else
  {
    std::fprintf(fp, "// Don't know how to convert %s\n", theFunc->Signature);
  }
}
//------------------------------------------------------------------------------
/**
 * Remove methods from \p wrappedFunctions which although have wrappable arguments alternative,
 * there are alternative signatures that are prefered. For example, if both `SetCenter(double*)` and
 * `SetCenter(double x0, double x1, double x2)` exist only the second will be kept since it is
 * easier to pack its value in a vtkVariant with no ambiquities.
 */
void vtkWrapRemoting_RemoveMethodsBasedOnPrecedence(std::vector<FunctionInfo*>& wrappedFunctions)
{
  FunctionInfo* sig1;
  FunctionInfo* sig2;
  int vote1 = 0;
  int vote2 = 0;

  for (size_t i = 0; i < wrappedFunctions.size(); i++)
  {
    sig1 = wrappedFunctions[i];
    if (!sig1->Name)
    {
      continue;
    }
    for (size_t j = i + 1; j < wrappedFunctions.size(); j++)
    {
      sig2 = wrappedFunctions[j];
      if (!sig2->Name)
      {
        continue;
      }
      if (!strcmp(sig1->Name, sig2->Name))
      {
        // This is very specifc to SetVector properties.
        // FIXME parser does not Populate `Macro` field of a Function
        // if(vtkWrap_IsSetVectorMethod(sig1) && vtkWrap_IsSetVectorMethod(sig2))
        vote1 = 0;
        vote2 = 0;
        bool sig1HasNoPointerParameter = (sig1->NumberOfParameters > 1 &&
          std::all_of(sig1->Parameters, sig1->Parameters + sig1->NumberOfParameters,
            [](ValueInfo* v) { return !vtkWrap_IsPointer(v); }));
        bool sig2HasSinglePointerParameter =
          (sig2->NumberOfParameters == 1 && vtkWrap_IsArray(sig2->Parameters[0]));
        bool sig2HasNoPointerParameter = (sig2->NumberOfParameters > 1 &&
          std::all_of(sig2->Parameters, sig2->Parameters + sig2->NumberOfParameters,
            [](ValueInfo* v) { return !vtkWrap_IsPointer(v); }));
        bool sig1HasSinglePointerParameter =
          (sig1->NumberOfParameters == 1 && vtkWrap_IsArray(sig1->Parameters[0]));
        if (sig2HasSinglePointerParameter && sig1HasNoPointerParameter)
        {
          vote1++;
        }
        else if (sig1HasSinglePointerParameter && sig2HasNoPointerParameter)
        {
          vote2++;
        }

        if (vote1 > vote2)
        {
          sig2->Name = nullptr;
        }
        else if (vote1 < vote2)
        {
          sig1->Name = nullptr;
          break;
        }
      }
    }
  }

  wrappedFunctions.erase(std::remove_if(wrappedFunctions.begin(), wrappedFunctions.end(),
                           [](FunctionInfo* f) { return f->Name == nullptr; }),
    wrappedFunctions.end());
}
//------------------------------------------------------------------------------
/**
 * Check a method to see if we want to wrap it.
 */
int vtkWrapRemoting_MethodCheck(ClassInfo* data, FunctionInfo* currentFunction)
{
  // The wrapping code takes care of these
  if (vtkWrap_IsDestructor(data, currentFunction) || vtkWrap_IsConstructor(data, currentFunction))
  {
    return 0;
  }
  // we do not currently support either of them
  if (currentFunction->IsOperator || currentFunction->Template)
  {
    return 0;
  }

  // some functions will not get wrapped no matter what
  if (currentFunction->IsExcluded || currentFunction->IsDeleted ||
    currentFunction->Access != VTK_ACCESS_PUBLIC)
  {
    return 0;
  }

  if (currentFunction->Name == nullptr)
  {
    return 0;
  }

  // wrapper takes care of lifetime so no need for new/delete, or any inheritance information
  const static std::vector<std::string> functionsToSkip = { "Delete", "GetClassName",
    "GetNumberOfGenerationsFromBase", "GetNumberOfGenerationsFromBaseType", "IsA", "IsTypeOf",
    "New", "NewInstance", "Register", "SafeDownCast", "UnRegister" };
  if (std::find(functionsToSkip.begin(), functionsToSkip.end(), currentFunction->Name) !=
    functionsToSkip.end())
  {
    return 0;
  }
  // we care only for Get/Setters for now
  if (!vtkWrapRemoting_IsGetterOrSetter(currentFunction))
  {
    return 0;
  }

  // check to see if we can handle the argument
  if (currentFunction->NumberOfParameters > 0)
  {
    for (int i = 0; i < currentFunction->NumberOfParameters; i++)
    {
      if (!vtkWrapRemoting_IsValueWrappable(currentFunction->Parameters[i]))
      {
        return 0;
      }
    }
  }

  // check the return value
  if (!vtkWrapRemoting_IsValueWrappable(currentFunction->ReturnValue))
  {
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
/** Create a new function with signature
 *
 *\code{.cpp}
 *   returnType functionName( parameterType )
 *\endcode
 *
 */
FunctionInfo* vtkWrapRemoting_GenerateOneSpecialMethod(
  const char* functionName, const char* returnType, const char* parameterType)
{
  auto* newFunction = (FunctionInfo*)(std::malloc(sizeof(FunctionInfo)));
  vtkParse_InitFunction(newFunction);
  newFunction->Name = vtkWrapRemoting_CreateStringCopy(functionName);
  std::string signature;
  signature.append(" ");
  signature.append("(");
  signature.append(returnType);
  signature.append(functionName);
  signature.append("(");
  if (parameterType)
  {
    signature.append(parameterType);
  }
  signature.append(")");
  newFunction->Signature = vtkWrapRemoting_CreateStringCopy(signature.c_str());

  newFunction->NumberOfParameters = parameterType == nullptr ? 0 : 1;
  if (newFunction->NumberOfParameters)
  {
    auto* parameter = vtkWrapRemoting_CreateTypeFromName(parameterType);
    newFunction->Parameters = (ValueInfo**)(std::malloc(sizeof(ValueInfo*)));
    newFunction->Parameters[newFunction->NumberOfParameters - 1] = parameter;
  }
  auto* returnValue = vtkWrapRemoting_CreateTypeFromName(returnType);
  newFunction->ReturnValue = returnValue;
  newFunction->Macro = vtkWrapRemoting_CreateStringCopy(GENERATED_INTERNALLY_STR);
  return newFunction;
}
//------------------------------------------------------------------------------
/**
 * This is a placeholder for generating methods that do not exist in the VTK
 * API but needed for Wrapping.
 *
 * For example SetFileName of a reader requires a GetFilename in order
 * to be wrapped.  Methods generated by this function have the `Macro` field
 * set to "GENERATED_INTERNALLY"
 */
void vtkWrapRemoting_GenerateSpecialMethods(
  ClassInfo* data, std::vector<FunctionInfo*>& wrappableFunctions)
{
  // Add setters for vtkDataArraySelection getters
  size_t count = wrappableFunctions.size();
  FunctionInfo* theFunc;
  for (size_t i = 0; i < count; i++)
  {
    theFunc = wrappableFunctions[i];
    if (vtkWrapRemoting_IsGetter(theFunc) &&
      vtkWrap_IsTypeOf(nullptr, theFunc->ReturnValue->Class, "vtkDataArraySelection"))
    {
      char* new_name = vtkWrapRemoting_CreateStringCopy(theFunc->Name);
      new_name[0] = 'S';
      auto* newFunction =
        vtkWrapRemoting_GenerateOneSpecialMethod(new_name, "void", "vtkDataArraySelection");
      vtkParse_AddFunctionToClass(data, newFunction);
      wrappableFunctions.push_back(newFunction);
    }
  }

  // Check if AddFileName() exists in order to add Set/GetFilenames
  // If not, check if SetFileName exists and GetFilename is missing.
  // In the latter case add a GetFilename function
  bool addFilenameExists = false;
  bool getFilenameMissing = false;
  for (int i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];
    if (theFunc->Name && !strcmp(theFunc->Name, "AddFileName"))
    {
      addFilenameExists = true;
    }
    if (vtkWrapRemoting_IsSetter(theFunc) && !strcmp(theFunc->Name, "SetFileName"))
    {
      bool getter_found = false;
      for (size_t j = 0; j < data->NumberOfFunctions; j++)
      {
        FunctionInfo* theFunc2 = data->Functions[j];
        if (vtkWrapRemoting_IsGetter(theFunc2) && !strcmp(theFunc2->Name, "GetFileName"))
        {
          getter_found = true;
          break;
        }
      }
      if (!getter_found)
      {
        getFilenameMissing = true;
      }
    }
  }

  if (addFilenameExists)
  {
    // invalide Set/GetFileName, we will create our own
    for (size_t i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];
      if (vtkWrapRemoting_IsSetter(theFunc) && !strcmp(theFunc->Name, "SetFileName"))
      {
        theFunc->Name = nullptr;
      }
      if (vtkWrapRemoting_IsGetter(theFunc) && !strcmp(theFunc->Name, "GetFileName"))
      {
        theFunc->Name = nullptr;
      }
    }
    auto* getter =
      vtkWrapRemoting_GenerateOneSpecialMethod("GetFileNames", "std::vector<std::string>", nullptr);
    vtkParse_AddFunctionToClass(data, getter);
    wrappableFunctions.push_back(getter);

    auto* setter =
      vtkWrapRemoting_GenerateOneSpecialMethod("SetFileNames", "void", "std::vector<std::string>");
    vtkParse_AddFunctionToClass(data, setter);
    wrappableFunctions.push_back(setter);
  }
  // else if (getFilenameMissing)
  //{
  //  vtkLogF(ERROR,"getFilenameMissing");
  //  auto* newFunction =
  //    vtkWrapRemoting_GenerateOneSpecialMethod("GetFileName", "std::string", nullptr);
  //  vtkParse_AddFunctionToClass(data, newFunction);
  //  wrappableFunctions.push_back(newFunction);
  //}
}

/**
 * Get All the functions we can wrap
 */
std::vector<FunctionInfo*> vtkWrapRemoting_GetWrappableFunctions(ClassInfo* classInfo)
{
  std::vector<FunctionInfo*> wrappableFunctions;
  FunctionInfo* theFunc = nullptr;
  // Collect functions we want to wrap. That is all the properties
  for (int i = 0; i < classInfo->NumberOfFunctions; i++)
  {
    theFunc = classInfo->Functions[i];

    // check for wrappability
    if (vtkWrapRemoting_MethodCheck(classInfo, theFunc))
    {
      wrappableFunctions.push_back(theFunc);
    }
  }

  vtkWrapRemoting_RemoveMethodsBasedOnPrecedence(wrappableFunctions);
  vtkWrapRemoting_GenerateSpecialMethods(classInfo, wrappableFunctions);

  return wrappableFunctions;
}
//------------------------------------------------------------------------------
/**
 * Generate the implementation and header file of the wrapped class
 */
void vtkWrapRemoting_GenerateMethods(FILE* fp, ClassInfo* classInfo)
{
  const char* className = classInfo->Name;
  FunctionInfo* theFunc = nullptr;

  std::vector<FunctionInfo*> wrappableFunctions = vtkWrapRemoting_GetWrappableFunctions(classInfo);

  // Write Setters
  std::fprintf(fp,
    "static bool %sSetter(vtkObject* object, const std::string& method, const "
    "std::vector<vtkVariant>& args)\n"
    "{\n"
    "  auto* target = %s::SafeDownCast(object);\n"
    "  if (!target)\n"
    "  {\n"
    "    return false;\n"
    "  }\n",
    className, className);

  // write out the wrapper for each function in the array
  for (FunctionInfo* theFunc : wrappableFunctions)
  {
    if (!vtkWrapRemoting_IsSetter(theFunc))
    {
      continue;
    }

    std::fprintf(fp, "\n");

    vtkWrapRemoting_GenerateOneMethod(fp, classInfo, theFunc);
  }

  std::fprintf(fp, "\n\n");
  // if method does not match with any in this class go to base class.
  std::fprintf(
    fp, "  return vtkReflection::Set(object, method, args, \"%s\");\n", classInfo->SuperClasses[0]);

  std::fprintf(fp, "}\n");
  // end of Setters

  std::fprintf(fp, "\n\n");

  // Write Getters
  std::fprintf(fp,
    "static bool %sGetter(vtkObject* object, const std::string& method, std::vector<vtkVariant>& "
    "args)\n"
    "{\n"
    "  auto* target = %s::SafeDownCast(object);\n"
    "  if (!target)\n"
    "  {\n"
    "    return false;\n"
    "  }\n",
    className, className);

  // write out the wrapper for each function in the array
  for (FunctionInfo* theFunc : wrappableFunctions)
  {
    if (!vtkWrapRemoting_IsGetter(theFunc))
    {
      continue;
    }
    std::fprintf(fp, "\n");

    vtkWrapRemoting_GenerateOneMethod(fp, classInfo, theFunc);
  }

  // if method does not match with any in this class go to base class.
  std::fprintf(
    fp, "  return vtkReflection::Get(object, method, args, \"%s\");\n", classInfo->SuperClasses[0]);

  // end of Getters
  std::fprintf(fp, "}\n");

  std::fprintf(fp, "\n\n");
  // Registration
  std::fprintf(fp,
    "void %sRXX()\n"
    "{\n",
    className);
  if (!classInfo->IsAbstract)
  {
    std::fprintf(fp, "  vtkReflection::RegisterNew<%s>(\"%s\");\n", className, className);
  }

  std::fprintf(fp,
    "  vtkReflection::RegisterSetter(\"%s\", %sSetter);\n"
    "  vtkReflection::RegisterGetter(\"%s\", %sGetter);\n"
    "}\n",
    className, className, className, className);
}
//------------------------------------------------------------------------------
int vtkWrapRemoting_WrapOneClass(FILE* fp, ClassInfo* classInfo)
{
  /* output all the methods are wrappable */
  vtkWrapRemoting_GenerateMethods(fp, classInfo);

  return 0;
}

void vtkWrapRemoting_DummyFile(FILE* fp, const char* className)
{
  std::fprintf(fp, "// This automatically generated file contains only a stub,\n");
  std::fprintf(fp, "// because the class %s cannot be wrapped.\n", className);
  std::fprintf(fp, "\n");
  std::fprintf(fp, "void %sRXX()\n", className);
  std::fprintf(fp, "{\n");
  std::fprintf(fp, "}\n");
}
//------------------------------------------------------------------------------
/**
 * Add arguments used to generate the file to ease debugging
 */
void vtkWrapRemoting_DecorateHeader(FILE* f, int argc, char** argv)
{
  std::fprintf(f, "// This file was auto-generated using :\n");
  std::fprintf(f, "/*\n");
  for (int i = 0; i < argc; i++)
  {
    std::fprintf(f, "  %s \n", argv[i]);
  }
  std::fprintf(f, "*/\n");
}

int main(int argc, char* argv[])
{
  ClassInfo* data = nullptr;
  HierarchyInfo* hinfo = nullptr;
  FileInfo* file_info = nullptr;
  OptionInfo* options = nullptr;
  FILE* fp = nullptr;
  NamespaceInfo* contents = nullptr;
  bool skip = false;

  int numberOfWrappedClasses = 0;
  int numberOfWrappedNamespaces = 0;
  // get command-line args and parse the header file
  file_info = vtkParse_Main(argc, argv);

  Globals = new vtkGlobals();

  // get the command-line options
  options = vtkParse_GetCommandLineOptions();

  // get the hierarchy info for accurate typing
  if (options->HierarchyFileNames)
  {
    hinfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  // get the output file
  fp = vtkWrapRemoting_OpenFile(options->OutputFileName, "w");

  std::string headerFilename(options->OutputFileName);
  size_t index = headerFilename.find_last_of('.');
  headerFilename.resize(index + 1); // drop rest of characters
  headerFilename.push_back('h');

  // get the filename from path without the extension
  std::string fileNameAsSTDString(file_info->FileName);
  std::string basename = fileNameAsSTDString.substr(fileNameAsSTDString.find_last_of("/\\") + 1);
  if (basename.empty())
  {
    vtkLogF(ERROR, "Could not extract basename of file %s", file_info->FileName);
    exit(1);
  }
  std::string className = basename.substr(0, basename.find_last_of('.'));

  // get the global namespace
  contents = file_info->Contents;

  data = file_info->MainClass;
  if (!data || data->IsExcluded)
  {
    skip = true;
  }

  // use the hierarchy file to expand typedefs
  if ((data) && (hinfo))
  {
    vtkWrap_ApplyUsingDeclarations(data, file_info, hinfo);
    vtkWrap_ExpandTypedefs(data, file_info, hinfo);
    if (!vtkWrap_IsTypeOf(hinfo, className.c_str(), "vtkObject"))
    {
      skip = true;
    }
  }
  // Templated Classes are not supported yet
  if ((data) && data->Template)
  {
    skip = true;
  }

  if (!skip)
  {
    std::fprintf(fp, "// Wrapper for %s\n", className.c_str());
    vtkWrapRemoting_DecorateHeader(fp, argc, argv);

    std::fprintf(fp, "#define VTK_WRAPPING_CXX\n");
    std::fprintf(fp, "#include \"vtkReflection.h\"\n");
    std::fprintf(fp, "#include \"%s.h\"\n\n", className.c_str());

    int error = vtkWrapRemoting_WrapOneClass(fp, data);
    if (error)
    {
      vtkLogF(ERROR, "Could not wrap %s", data->Name);
    }
  }
  else
  {
    vtkWrapRemoting_DummyFile(fp, className.c_str());
  }

  std::fclose(fp);
  vtkParse_Free(file_info);
  if (hinfo)
  {
    vtkParseHierarchy_Free(hinfo);
  }
  // vtkParse_FreeOptions(options);

  vtkParse_FinalCleanup();
  delete Globals;
  return 0;
}
