/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectStore.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkObjectStore
 * @brief a thread-safe store for vtkObject instances.
 *
 * vtkObjectStore is intended to be a thread-safe store vtkObject instances.
 */

#ifndef vtkObjectStore_h
#define vtkObjectStore_h

#include "vtkObject.h"
#include "vtkServerManagerCoreModule.h" // for exports
#include <memory>                       // for std::unique_ptr

class VTKSERVERMANAGERCORE_EXPORT vtkObjectStore : public vtkObject
{
public:
  static vtkObjectStore* New();
  vtkTypeMacro(vtkObjectStore, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Register an object.
   *
   * This method is thread safe.
   */
  void RegisterObject(vtkTypeUInt32 gid, vtkSmartPointer<vtkObject> const& object);

  /**
   * Unregister an object given its id.
   *
   * This method is thread safe.
   */
  void UnregisterObject(vtkTypeUInt32 gid);

  /**
   * Find an object, given the id.
   *
   * This method is thread safe.
   */
  vtkSmartPointer<vtkObject> FindObject(vtkTypeUInt32 gid) const;

  /**
   * Get the id for an object.
   *
   * This method is thread safe.
   */
  vtkTypeUInt32 GetID(const vtkObject* object) const;

protected:
  vtkObjectStore();
  ~vtkObjectStore() override;

private:
  vtkObjectStore(const vtkObjectStore&) = delete;
  void operator=(const vtkObjectStore&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
