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

#include "vtkCommand.h" // for vtkCommand
#include "vtkObject.h"
#include "vtkObjectWrapper.h"               // for vtkObjectWrapper
#include "vtkRemotingServerManagerModule.h" // for exports
#include "vtkSmartPointer.h"                // for vtkSmartPointer
#include <memory>                           // for std::unique_ptr

class VTKREMOTINGSERVERMANAGER_EXPORT vtkObjectStore : public vtkObject
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

  template <typename T,
    typename SFINAE = typename std::enable_if<std::is_base_of<vtkObject, T>::value>::type>
  vtkSmartPointer<T> FindObject(vtkTypeUInt32 gid) const
  {
    vtkSmartPointer<T> result;
    auto smartPtr = this->FindObject(gid);
    if (smartPtr)
    {
      result = T::SafeDownCast(smartPtr);
    }
    return result;
  }

  template <typename T,
    typename SFINAE = typename std::enable_if<std::is_base_of<vtkObject, T>::value>::type>
  vtkSmartPointer<T> FindVTKObject(vtkTypeUInt32 gid) const
  {
    auto wrapper = this->FindObject<vtkObjectWrapper>(gid);
    return (wrapper ? T::SafeDownCast(wrapper->GetVTKObject()) : nullptr);
  }

  /**
   * Get the id for an object.
   *
   * This method is thread safe.
   */
  vtkTypeUInt32 GetID(const vtkObject* object) const;

  enum
  {
    RegisterObjectEvent = vtkCommand::UserEvent + 91,
    UnregisterObjectEvent,
  };

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
