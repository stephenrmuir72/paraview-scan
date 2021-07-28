/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPedestalDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPedestalDomain.h"

#include "vtkBoundingBox.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSourceProxy.h"

#include <set>
#include <string>

vtkStandardNewMacro(vtkSMPedestalDomain);

vtkSMPedestalDomain::vtkSMPedestalDomain() = default;

vtkSMPedestalDomain::~vtkSMPedestalDomain() = default;

int vtkSMPedestalDomain::SetDefaultValues(vtkSMProperty* property, bool use_unchecked_values)
{
  // Iterate over all loaded data sources and get bounds
  vtkBoundingBox bbox;

  std::set<std::string> proxyGroups;
  proxyGroups.insert("sources");
  proxyGroups.insert("filters");

  for (auto& group : proxyGroups)
  {
    vtkNew<vtkSMProxyIterator> iter;
    iter->SetModeToOneGroup();
    auto session = property->GetParent()->GetSession();
    iter->SetSession(session);
    for (iter->Begin(group.c_str()); !iter->IsAtEnd(); iter->Next())
    {
      auto sourceProxy = vtkSMSourceProxy::SafeDownCast(iter->GetProxy());
      if (sourceProxy)
      {
        // Iterate over representations to see if the source is visible in any of them
        vtkNew<vtkSMProxyIterator> repIter;
        repIter->SetModeToOneGroup();
        repIter->SetSession(session);
        for (repIter->Begin("representations"); !repIter->IsAtEnd(); repIter->Next())
        {
          auto repProxy = repIter->GetProxy();
          auto inputProxy = vtkSMPropertyHelper(repProxy, "Input").GetAsProxy();
          bool visible = vtkSMPropertyHelper(repProxy, "Visibility").GetAsInt() == 1;
          if (visible && inputProxy == sourceProxy)
          {
            auto dataInfo = sourceProxy->GetDataInformation();
            double sourceBounds[6];
            dataInfo->GetBounds(sourceBounds);
            bbox.AddBounds(sourceBounds);
            break;
          }
        }
      }
    }
  }

  double bounds[6];
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
  }
  else
  {
    // Just use XML defaults
    return 1;
  }

  vtkSMPropertyHelper helper(property);
  helper.SetUseUnchecked(use_unchecked_values);

  double dx = bounds[1] - bounds[0];
  double dy = bounds[3] - bounds[2];
  double dz = bounds[5] - bounds[4];

  // Set the pedestal to one quarter the bounds height in y
  double height = 0.25 * dy;
  double radius = std::max(dx, dz);

  std::string propertyName(property->GetXMLName());
  if (propertyName == "Height")
  {
    helper.Set(0, height);
  }
  else if (propertyName == "Radius")
  {
    // Set the radius to the maximum of the x and z bounds
    helper.Set(0, radius);
  }
  else if (propertyName == "Center")
  {
    // Center pedestal on bounds in x and z
    double x = 0.5 * (bounds[1] + bounds[0]);
    double z = 0.5 * (bounds[5] + bounds[4]);

    // Place pedestal just below data bounds
    double y = bounds[2] - 0.5 * height;
    helper.Set(0, x);
    helper.Set(1, y);
    helper.Set(2, z);
  }
  else if (propertyName == "XLength")
  {
    helper.Set(0, 2 * dx);
  }
  else if (propertyName == "YLength")
  {
    helper.Set(0, height);
  }
  else if (propertyName == "ZLength")
  {
    helper.Set(0, 2 * dz);
  }

  return 1;
}

void vtkSMPedestalDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
