/*=========================================================================

  Program:   ParaView
  Module:    vtkObjectWrapper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkObjectWrapper
 * @brief a wrapper for vtkObject
 *
 * vtkObjectWrapper is used to create/update vtkObject-subclass instances on a
 * remote service using the vtkRemoteObjectProvider.
 */

#ifndef vtkObjectWrapper_h
#define vtkObjectWrapper_h

#include "vtkJsonFwd.h"
#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" // for exports

#include <memory>        // for std::unique_ptr
#include <vtk_pugixml.h> // for pugi

class vtkRemoteObjectProvider;

class VTKREMOTINGCORE_EXPORT vtkObjectWrapper : public vtkObject
{
public:
  static vtkObjectWrapper* New();
  vtkTypeMacro(vtkObjectWrapper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setup the object wrapper by processing property definitions from the XML
   * file.
   */
  bool ReadXMLAttributes(const pugi::xml_node& node);

  /**
   * Initializes the wrapper.
   */
  bool Initialize(vtkTypeUInt32 gid, const vtkJson& state, vtkRemoteObjectProvider* provider);

  /**
   * Returns the global id assigned to this wrapper.
   */
  vtkTypeUInt32 GetGlobalID() const;

  /**
   * Update the wrapped object using the state.
   */
  void UpdateState(const vtkJson& state, vtkRemoteObjectProvider* provider);

  /**
   * Returns state for all information properties.
   */
  vtkJson UpdateInformation() const;

  /**
   * Returns the VTK object this wrapper is wrapping.
   *
   * This is created in `ReadXMLAttributes` and hence will not be valid before
   * that method has been called, which happens during initialization stage.
   */
  vtkObject* GetVTKObject() const;

protected:
  vtkObjectWrapper();
  ~vtkObjectWrapper();

private:
  vtkObjectWrapper(const vtkObjectWrapper&) = delete;
  void operator=(const vtkObjectWrapper&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
