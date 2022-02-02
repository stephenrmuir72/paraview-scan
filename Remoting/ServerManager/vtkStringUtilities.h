/*=========================================================================

  Program:   ParaView
  Module:    vtkStringUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkStringUtilities
 * @brief
 *
 */

#ifndef vtkStringUtilities_h
#define vtkStringUtilities_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports
#include <algorithm>                        // for std::transform
#include <vtksys/SystemTools.hxx>           // for SystemTools

class VTKREMOTINGSERVERMANAGER_EXPORT vtkStringUtilities : public vtkObject
{
public:
  static vtkStringUtilities* New();
  vtkTypeMacro(vtkStringUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static bool EndsWith(const std::string& text, const std::string& suffix);
  static std::string GetPrettyLabel(const std::string& txt);

  static bool Parse(const std::string& value, double& result);
  static bool Parse(const std::string& value, int& result);
  static bool Parse(const std::string& value, vtkIdType& result);

  template <typename T>
  static bool Parse(const std::string& value, std::vector<T>& result, char delimiter = ' ')
  {
    const auto strings = vtksys::SystemTools::SplitString(value, delimiter);
    std::transform(
      strings.begin(), strings.end(), std::back_inserter(result), [](const std::string& value) {
        T tmp;
        return vtkStringUtilities::Parse(value, tmp) ? tmp : T{};
      });
    return true;
  }

protected:
  vtkStringUtilities();
  ~vtkStringUtilities();

private:
  vtkStringUtilities(const vtkStringUtilities&) = delete;
  void operator=(const vtkStringUtilities&) = delete;
};

#endif
