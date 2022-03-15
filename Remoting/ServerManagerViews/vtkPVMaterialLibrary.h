/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialLibrary.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMaterialLibrary
 * @brief   manages visual material definitions
 *
 * vtkPVMaterialLibrary helps ParaView to load visual material definition
 * files from known, generally process relative, locations on the sever.
 *
 * This class does nothing without raytracing (module VTK_MODULE_ENABLE_VTK_RenderingRayTracing
 * disabled)
 */

#ifndef vtkPVMaterialLibrary_h
#define vtkPVMaterialLibrary_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerViewsModule.h" // needed for exports

#include <string> // for std::string
#include <vector> // for std::vector

class vtkMultiProcessController;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkPVMaterialLibrary : public vtkObject
{
public:
  static vtkPVMaterialLibrary* New();
  vtkTypeMacro(vtkPVMaterialLibrary, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the material library filenames.
   * All absolute paths are treated as absolute while relative paths are
   * searched relative to the search paths.
   */
  void SetFiles(const std::vector<std::string>& files);
  const std::vector<std::string>& GetFiles() const;
  ///@}

  ///@{
  /**
   * Get/set the multiprocess controller. This is used to avoid searching files
   * on all ranks in a distributed setup and instead only do those searches on
   * the root node.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkMultiProcessController* GetController() const;
  ///@}

  /**
   * Returns the underlying material library.
   * When compiled without OSPRAY, will return nullptr
   */
  vtkObject* GetMaterialLibrary() const;

  /**
   * Returns list of available materials.
   */
  const std::vector<std::string>& GetMaterialNames() const;

  ///@{
  /**
   * Defer to contained MaterialLibrary, if any, otherwise does nothing.
   */
  bool ReadFile(const char* FileName);
  void WriteFile(const std::string& filename);
  ///@}

protected:
  vtkPVMaterialLibrary();
  ~vtkPVMaterialLibrary() override;

private:
  vtkPVMaterialLibrary(const vtkPVMaterialLibrary&) = delete;
  void operator=(const vtkPVMaterialLibrary&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
