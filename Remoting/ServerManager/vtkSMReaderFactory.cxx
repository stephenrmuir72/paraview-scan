/*=========================================================================
  Program:   ParaView
  Module:    vtkSMReaderFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMReaderFactory.h"

#include "vtkClientSession.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>

const std::string vtkSMReaderFactory::SUPPORTED_TYPES_DESCRIPTION = "Supported Types";
const std::string vtkSMReaderFactory::ALL_FILES_DESCRIPTION = "All Files";

static void string_replace(std::string& string, char c, std::string str)
{
  size_t cc = string.find(c);
  while (cc < std::string::npos)
  {
    string = string.replace(cc, 1, str);
    cc = string.find(c, cc + str.size());
  }
}

static std::set<std::string> vtkBuildExtensions(const std::string& filename)
{
  std::set<std::string> extensions;

  // basically we are filling up extensions with all possible extension
  // combintations eg. myfilename.tar.gz.vtk.000 results in
  // 000, vtk.000, gz.vtk.000, tar.gz.vtk.000,
  // vtk, gz.vtk, tar.gz.vtk
  // gz, tar.gz
  // tar, tar.gz
  // gz
  // in that order.
  std::string extension = vtksys::SystemTools::GetFilenameExtension(filename);
  if (!extension.empty())
  {
    extension.erase(extension.begin()); // remove the first "."
  }
  std::vector<std::string> parts;
  vtksys::SystemTools::Split(extension, parts, '.');
  int num_parts = static_cast<int>(parts.size());
  for (int cc = num_parts - 1; cc >= 0; cc--)
  {
    for (int kk = cc; kk >= 0; kk--)
    {
      std::string cur_string;
      for (int ii = kk; ii <= cc; ii++)
      {
        if (parts[ii].empty())
        {
          continue; // skip empty parts.
        }
        if (ii != kk)
        {
          cur_string += ".";
        }
        cur_string += parts[ii];
      }
      extensions.insert(std::move(cur_string));
    }
  }

  return extensions;
}

class vtkSMReaderFactory::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkWeakPointer<vtkClientSession> Session;

  struct vtkReaderHint
  {
    std::vector<std::string> Extensions;
    std::vector<std::string> FilenamePatterns;
    std::vector<vtksys::RegularExpression> FilenameRegExs;
    std::string Description;
    bool IsDirectory = false;

    std::vector<std::string> GetFilenamePatterns() const
    {
      std::vector<std::string> result;
      for (auto const& ext : this->Extensions)
      {
        result.push_back("*." + ext);
      }

      for (auto const& pattern : this->FilenamePatterns)
      {
        result.push_back(pattern);
      }
      return result;
    }
  };

  struct vtkVectorOfReaderHints : public std::vector<vtkReaderHint>
  {
    bool IsMatch(
      const std::string& fname, const std::set<std::string>& extensions, bool isDir) const
    {
      if (std::none_of(this->begin(), this->end(),
            [isDir](const vtkReaderHint& hint) -> bool { return hint.IsDirectory == isDir; }))
      {
        // no supported pattern accepts a directory.
        return false;
      }

      // do a extensions test.
      const auto extMatch =
        std::any_of(this->begin(), this->end(), [&extensions](const vtkReaderHint& hint) {
          return std::any_of(hint.Extensions.begin(), hint.Extensions.end(),
            [&](const std::string& ext) { return (extensions.find(ext) != extensions.end()); });
        });

      if (extMatch)
      {
        return true;
      }

      // do regex match.
      const auto regExMatch =
        std::any_of(this->begin(), this->end(), [&fname](const vtkReaderHint& hint) {
          return std::any_of(hint.FilenameRegExs.begin(), hint.FilenameRegExs.end(),
            [&](const vtksys::RegularExpression& regEx) {
              vtksys::RegularExpressionMatch match;
              return regEx.find(fname.c_str(), match);
            });
        });

      if (extMatch)
      {
        return true;
      }

      return false;
    }
  };

  using KeyT = std::pair<std::string, std::string>;
  std::map<KeyT, vtkVectorOfReaderHints> Readers;

  bool RegisterPrototype(
    const std::string& xmlgroup, const std::string& xmlname, vtkPVXMLElement* xmlHints)
  {
    vtkInternals::vtkVectorOfReaderHints infos;
    for (unsigned int cc = 0, max = (xmlHints ? xmlHints->GetNumberOfNestedElements() : 0);
         cc < max; ++cc)
    {
      auto* rfHint = xmlHints->GetNestedElement(cc);
      if (strcmp(rfHint->GetName(), "ReaderFactory") != 0)
      {
        continue;
      }

      vtkInternals::vtkReaderHint hint;

      // Description
      hint.Description = rfHint->GetAttributeOrEmpty("file_description");

      // Extensions
      if (const char* exts = rfHint->GetAttributeOrEmpty("extensions"))
      {
        vtksys::SystemTools::Split(exts, hint.Extensions, ' ');
      }

      // Patterns
      if (const char* filename_patterns = rfHint->GetAttribute("filename_patterns"))
      {
        vtksys::SystemTools::Split(filename_patterns, hint.FilenamePatterns, ' ');
        // convert the wild-card based patterns to regular expressions.
        for (auto item : hint.FilenamePatterns)
        {
          ::string_replace(item, '.', "\\.");
          ::string_replace(item, '?', ".");
          ::string_replace(item, '*', ".*");
          hint.FilenameRegExs.emplace_back(vtksys::RegularExpression(item.c_str()));
        }
      }

      // Directory
      int is_directory = 0;
      if (rfHint->GetScalarAttribute("is_directory", &is_directory))
      {
        hint.IsDirectory = (is_directory == 1);
      }
      else
      {
        hint.IsDirectory = false;
      }

      // Add the completed hint
      infos.emplace_back(std::move(hint));
    }

    if (infos.empty())
    {
      return false;
    }

    this->Readers[std::make_pair(xmlgroup, xmlname)] = std::move(infos);
    return true;
  }

  std::vector<std::pair<std::string, std::string>> GetMatches(
    const std::string& fname, bool isDir) const
  {
    std::vector<std::pair<std::string, std::string>> result;
    const auto extensions = vtkBuildExtensions(fname);
    for (const auto& pair : this->Readers)
    {
      if (pair.second.IsMatch(fname, extensions, isDir))
      {
        result.push_back(pair.first);
      }
    }
    return result;
  }
};

//----------------------------------------------------------------------------
// Use VTK's object factory to construct new instances. This allows derived
// applications to derive from vtkSMReaderFactory and implement changes to its
// functionality.
vtkObjectFactoryNewMacro(vtkSMReaderFactory);
//----------------------------------------------------------------------------
vtkSMReaderFactory::vtkSMReaderFactory()
  : Internals(new vtkSMReaderFactory::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMReaderFactory::~vtkSMReaderFactory() = default;

//----------------------------------------------------------------------------
void vtkSMReaderFactory::SetSession(vtkClientSession* session)
{
  auto& internals = (*this->Internals);
  if (internals.Session != session)
  {
    internals.Session = session;
    this->Reset();
  }
}

//----------------------------------------------------------------------------
vtkClientSession* vtkSMReaderFactory::GetSession() const
{
  const auto& internals = (*this->Internals);
  return internals.Session;
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMReaderFactory::GetSessionProxyManager() const
{
  auto* session = this->GetSession();
  return session ? session->GetProxyManager() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::Reset()
{
  auto& internals = (*this->Internals);
  internals.Readers.clear();
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototype(const std::string& xmlgroup, const std::string& xmlname)
{
  auto* pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    vtkLogF(ERROR, "RegisterPrototype called without a proxy manager!");
    return;
  }

  auto* prototype = pxm->GetPrototypeProxy(xmlgroup.c_str(), xmlname.c_str());
  if (!prototype)
  {
    vtkLogF(ERROR, "No proxy found (%s, %s)", xmlgroup.c_str(), xmlname.c_str());
    return;
  }

  auto& internals = (*this->Internals);
  auto* xmlHints = prototype->GetHints();
  if (!xmlHints || !internals.RegisterPrototype(xmlgroup, xmlname, xmlHints))
  {
    vtkLogF(WARNING, "Prototype (%s, %s) missing ReaderFactory hints.", xmlgroup.c_str(),
      xmlname.c_str());
  }
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototypesFromGroup(const std::string& xmlgroup)
{
  auto& internals = (*this->Internals);

  auto* pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    return;
  }

  auto* pdm = pxm->GetProxyDefinitionManager();
  for (const auto& el : pdm->GetDefinitions(xmlgroup))
  {
    if (auto* prototype = pxm->GetPrototypeProxy(el.GetGroup().c_str(), el.GetName().c_str()))
    {
      internals.RegisterPrototype(el.GetGroup(), el.GetName(), prototype->GetHints());
    }
  }
}

//----------------------------------------------------------------------------
std::vector<FileTypeDetailed> vtkSMReaderFactory::GetSupportedFileTypes() const
{
  const auto& internals = (*this->Internals);

  FileTypeDetailed supportedFiles;
  supportedFiles.Description = vtkSMReaderFactory::SUPPORTED_TYPES_DESCRIPTION;

  std::vector<FileTypeDetailed> result;
  for (auto& pair : internals.Readers)
  {
    const auto& type = pair.first;
    for (const auto& readerHint : pair.second)
    {
      FileTypeDetailed item;
      std::tie(item.Group, item.Name) = type;
      item.Description = readerHint.Description;
      item.FilenamePatterns = readerHint.GetFilenamePatterns();
      if (!item.FilenamePatterns.empty())
      {
        std::copy(item.FilenamePatterns.begin(), item.FilenamePatterns.end(),
          std::back_inserter(supportedFiles.FilenamePatterns));
        result.push_back(std::move(item));
      }
    }
  }

#if 0 // FIXME: ASYNC
  // Add custom patterns to supported files
  auto* settings = vtkSMSettings::GetInstance();
  char const* settingName = ".settings.RepresentedArrayListSettings.ReaderDetails";
  unsigned int const numberOfEntries = settings->GetSettingNumberOfElements(settingName) / 3;
  for (unsigned int entryIndex = 0; entryIndex < numberOfEntries; ++entryIndex)
  {
    std::string const patternsString =
      settings->GetSettingAsString(settingName, entryIndex * 3, "");
    vtksys::SystemTools::Split(patternsString, supportedFiles.FilenamePatterns, ' ');
  }
#endif

  // sort patterns by description
  std::sort(
    result.begin(), result.end(), [](FileTypeDetailed const& lhs, FileTypeDetailed const& rhs) {
      return vtksys::SystemTools::Strucmp(lhs.Description.c_str(), rhs.Description.c_str()) < 0;
    });

  FileTypeDetailed allFiles;
  allFiles.Description = vtkSMReaderFactory::ALL_FILES_DESCRIPTION;
  allFiles.FilenamePatterns = { "*" };
  result.insert(result.begin(), { supportedFiles, allFiles });

  return result;
}

//----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string>> vtkSMReaderFactory::FindReadersForFile(
  const std::string& filename) const
{
  const auto& internals = (*this->Internals);

  // find all reader types where the extensions / patterns match.
  return internals.GetMatches(filename, /*isDir=*/false);
}

//----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string>> vtkSMReaderFactory::FindReadersForDirectory(
  const std::string& filename) const
{
  const auto& internals = (*this->Internals);

  // find all reader types where the extensions / patterns match.
  return internals.GetMatches(filename, /*isDir=*/true);
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
