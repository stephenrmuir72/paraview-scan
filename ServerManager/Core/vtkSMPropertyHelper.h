/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (c) 2007, Sandia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sandia Corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @class   vtkSMPropertyHelper
 * @brief   helper class to get/set property values.
 *
 * vtkSMPropertyHelper is a helper class to get/set property values in a type
 * independent fashion.
 * eg.
 * \code
 *    vtkSMPropertyHelper(proxy, "Visibility").Set(0);
 *    vtkSMPropertyHelper(proxy, "Input").Set(inputProxy, 0);
 *
 *    double center[3] = {...};
 *    vtkSMPropertyHelper(proxy, "Center").Set(center, 3);
 * \endcode
 * @par Caveat:
 * This class is not wrapped, hence not available in any of the wrapped
 * languagues such as python.
 */

#ifndef vtkSMPropertyHelper_h
#define vtkSMPropertyHelper_h

#include "vtkSMProperty.h"              // for vtkSMProperty::PropertyTypes
#include "vtkServerManagerCoreModule.h" //needed for exports
#include "vtkVariant.h"                 // for vtkVariant

#include <vector> // for std::vector

class vtkSMDoubleVectorProperty;
class vtkSMIdTypeVectorProperty;
class vtkSMInputProperty;
class vtkSMIntVectorProperty;
class vtkSMOutputPort;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyProperty;
class vtkSMStringVectorProperty;

class VTKSERVERMANAGERCORE_EXPORT vtkSMPropertyHelper
{
public:
  //@{
  /**
   * If quiet is true, then no errors or warning are raised if the property is
   * missing or of incorrect type.
   */
  vtkSMPropertyHelper(vtkSMProxy* proxy, const char* name, bool quiet = false);
  vtkSMPropertyHelper(vtkSMProperty* property, bool quiet = false);
  ~vtkSMPropertyHelper();
  //@}

  /**
   * Set the number of elements in the property. For vtkSMProxyProperty, this is
   * equivalent to SetNumberOfProxies().
   */
  void SetNumberOfElements(size_t elems);

  /**
   * Get the number of elements in the property.
   * For vtkSMProxyProperty, this is equivalent to GetNumberOfProxies().
   */
  size_t GetNumberOfElements() const;

  /**
   * Equivalent to SetNumberOfElements(0).
   */
  void RemoveAllValues() { this->SetNumberOfElements(0); }

  /**
   * Get value as a variant.
   */
  vtkVariant GetAsVariant(size_t index) const;

  /**
   * Templated method to call GetIntArray(), GetDoubleArray(), GetIdTypeArray().
   * Note, we  only provide implementations for T==double, int, vtkIdType.
   */
  template <class T>
  std::vector<T> GetArray() const;

  /**
   * Templated method to call GetAsInt(), GetAsDouble(), GetAsIdType()
   * Note, we  only provide implementations for T==double, int, vtkIdType.
   */
  template <class T>
  T GetAs(size_t index = 0) const;

  //@{
  /**
   * Set/Get methods with \c int API. Calling these method on
   * vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
   */
  void Set(int value) { this->Set(static_cast<size_t>(0), value); }
  void Set(size_t index, int value);
  void Set(const int* values, size_t count);
  void Append(const int* values, size_t count);
  int GetAsInt(size_t index = 0) const;
  size_t Get(int* values, size_t count = 1) const;
  std::vector<int> GetIntArray() const;
  //@}

  //@{
  /**
   * Set/Get methods with \c double API. Calling these method on
   * vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
   */
  void Set(double value) { this->Set(0, value); }
  void Set(size_t index, double value);
  void Set(const double* values, size_t count);
  void Append(const double* values, size_t count);
  double GetAsDouble(size_t index = 0) const;
  size_t Get(double* values, size_t count = 1) const;
  std::vector<double> GetDoubleArray() const;
  //@}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
  //@{
  /**
   * Set/Get methods with \c vtkIdType API. Calling these method on
   * vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
   */
  void Set(vtkIdType value) { this->Set(0, value); }
  void Set(size_t index, vtkIdType value);
  void Set(const vtkIdType* values, size_t count);
  void Append(const vtkIdType* values, size_t count);
  size_t Get(vtkIdType* values, size_t count = 1) const;
#endif
  vtkIdType GetAsIdType(size_t index = 0) const;
  std::vector<vtkIdType> GetIdTypeArray() const;
  //@}

  //@{
  /**
   * Set/Get methods for vtkSMStringVectorProperty. Calling these methods on any
   * other type of property will raise errors.
   * These overloads can be used for vtkSMIntVectorProperty with an enumeration
   * domain as well, in which case the string-to-int (and vice-versa)
   * translations are done internally.
   */
  void Set(const std::string& value) { this->Set(0, value); }
  void Set(size_t index, const std::string& value);
  std::string GetAsString(size_t index = 0) const;
  //@}

  //@{
  /**
   * Set/Get methods for vtkSMProxyProperty or vtkSMInputProperty.
   * Calling these methods on any other type of property will raise errors.
   * The option \c outputport(s) argument is used only for vtkSMInputProperty.
   */
  void Set(vtkSMProxy* value, int outputport = 0) { this->Set(0, value, outputport); }
  void Set(size_t index, vtkSMProxy* value, int outputport = 0);
  void Set(vtkSMProxy** value, size_t count, int* outputports = nullptr);
  void Add(vtkSMProxy* value, int outputport = 0);
  void Remove(vtkSMProxy* value);
  vtkSMProxy* GetAsProxy(size_t index = 0) const;
  int GetOutputPort(size_t index = 0) const;
  vtkSMOutputPort* GetAsOutputPort(size_t index = 0) const;
  //@}

  //@{
  /**
   * This API is useful for setting values on vtkSMStringVectorProperty that is
   * used for status where the first value is the name of the array (for
   * example) and the second value is its status.
   */
  void SetStatus(const char* key, int value);
  int GetStatus(const char* key, int default_value = 0) const;
  //@}

  //@{
  /**
   * This API is useful for setting values on vtkSMStringVectorProperty that is
   * used for status where the first value is the name of the array (for
   * example), the second value is its status and the third value is number of status.
   */
  void SetStatus(const char* key, double* values, int num_values);
  bool GetStatus(const char* key, double* values, int num_values) const;
  //@}

  //@{
  /**
   * This API is useful for setting values on vtkSMIntVectorProperty that is
   * used for status where the first value is the id of the element (for
   * example), the second value is its status and the third value is number of status.
   */
  void SetStatus(int key, int* values, int num_values);
  bool GetStatus(int key, int* values, int num_values) const;
  //@}

  //@{
  /**
   * This API is useful for setting values on vtkSMStringVectorProperty that is
   * used for status where the first value is the name of the array (for
   * example) and the second value is its status (as a string)
   */
  void SetStatus(const char* key, const char* value);
  const char* GetStatus(const char* key, const char* default_value) const;
  //@}

  /**
   * Removes the status value specified for the given key, if any. Applicable only to
   * `vtkSMStringVectorProperty` objects, and this function assumes that the presence of a key in
   * the property indicates that it is enabled.
   */
  void RemoveStatus(const char* key);

  //@{
  /**
   * This API is useful for setting values on vtkSMIntVectorProperty that is
   * used for status where the first value is the id of the element (for
   * example) and the second value is its status.
   */
  void SetStatus(int key, int value);
  int GetStatus(int key, int default_value = 0) const;
  //@}

  //@{
  /**
   * For vtkSMStringVectorProperty that is used to setting input array to
   * process on algorithms, this provides a convenient API to get/set the
   * values.
   */
  void SetInputArrayToProcess(int fieldAssociation, const char* arrayName);
  int GetInputArrayAssociation() const;
  const char* GetInputArrayNameToProcess() const;
  //@}

  /**
   * Get/Set whether to use unchecked properties.
   */
  void SetUseUnchecked(bool val) { this->UseUnchecked = val; }
  bool GetUseUnchecked() const { return this->UseUnchecked; }

  /**
   * Copy property values from another vtkSMPropertyHelper. This only
   * works for compatible properties and currently only supported for numeric
   * vtkSMVectorProperty subclasses.
   */
  bool Copy(const vtkSMPropertyHelper& source);

  /**
   * Set the proxy to modified if necessary before calling Set()
   * Return reference so method chaining can be used.
   */
  vtkSMPropertyHelper& Modified();

protected:
  void setUseUnchecked(bool useUnchecked) { this->UseUnchecked = useUnchecked; }

private:
  vtkSMPropertyHelper(const vtkSMPropertyHelper&) = delete;
  void operator=(const vtkSMPropertyHelper&) = delete;
  void Initialize(vtkSMProperty* property);

  template <typename T>
  T GetProperty(size_t index) const;
  template <typename T>
  std::vector<T> GetPropertyArray() const;
  template <typename T>
  size_t GetPropertyArray(T* values, size_t count = 1) const;
  template <typename T>
  void SetProperty(size_t index, T value);
  template <typename T>
  void SetPropertyArray(const T* values, size_t count);
  void SetPropertyArrayIdType(const vtkIdType* values, size_t count);
  template <typename T>
  void AppendPropertyArray(const T* values, size_t count);
  template <typename T>
  bool CopyInternal(const vtkSMPropertyHelper& source);

  enum PType
  {
    INT,
    DOUBLE,
    IDTYPE,
    STRING,
    PROXY,
    INPUT,
    NONE
  };

  bool Quiet;
  bool UseUnchecked;
  vtkSMProxy* Proxy;
  PType Type;

  union
  {
    vtkSMProperty* Property;
    vtkSMIntVectorProperty* IntVectorProperty;
    vtkSMDoubleVectorProperty* DoubleVectorProperty;
    vtkSMIdTypeVectorProperty* IdTypeVectorProperty;
    vtkSMStringVectorProperty* StringVectorProperty;
    vtkSMProxyProperty* ProxyProperty;
    vtkSMInputProperty* InputProperty;
  };
};

template <>
inline std::vector<int> vtkSMPropertyHelper::GetArray() const
{
  return this->GetIntArray();
}

template <>
inline std::vector<double> vtkSMPropertyHelper::GetArray() const
{
  return this->GetDoubleArray();
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
template <>
inline std::vector<vtkIdType> vtkSMPropertyHelper::GetArray() const
{
  return this->GetIdTypeArray();
}
#endif

template <>
inline int vtkSMPropertyHelper::GetAs(size_t index) const
{
  return this->GetAsInt(index);
}

template <>
inline double vtkSMPropertyHelper::GetAs(size_t index) const
{
  return this->GetAsDouble(index);
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
template <>
inline vtkIdType vtkSMPropertyHelper::GetAs(size_t index) const
{
  return this->GetAsIdType(index);
}
#endif

#endif

// VTK-HeaderTest-Exclude: vtkSMPropertyHelper.h
