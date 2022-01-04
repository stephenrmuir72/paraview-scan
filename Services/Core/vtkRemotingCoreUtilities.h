/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemotingCoreUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRemotingCoreUtilities
 * @brief
 *
 */

#ifndef vtkRemotingCoreUtilities_h
#define vtkRemotingCoreUtilities_h

#include "vtkObject.h"
#include "vtkServicesCoreModule.h" // for exports

#include <thread> // for std::thread

class VTKSERVICESCORE_EXPORT vtkRemotingCoreUtilities : public vtkObject
{
public:
  static vtkRemotingCoreUtilities* New();
  vtkTypeMacro(vtkRemotingCoreUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkRemotingCoreUtilities(const vtkRemotingCoreUtilities&) = delete;
  void operator=(const vtkRemotingCoreUtilities&) = delete;

  /**
   * Use this in non-threadsafe methods to ensure they are called on the owner
   * thread alone.
   */
  static void EnsureThread(const std::thread::id& owner);

protected:
  vtkRemotingCoreUtilities();
  ~vtkRemotingCoreUtilities() override;
};

#endif
