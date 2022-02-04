/*=========================================================================

  Program:   ParaView
  Module:    vtkInteractionProvider.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkInteractionProvider
 * @brief provider to add support for interaction with views.
 */

#ifndef vtkInteractionProvider_h
#define vtkInteractionProvider_h

#include "vtkPacket.h" // for vtkPacket
#include "vtkProvider.h"
#include "vtkRemotingServerManagerViewsModule.h" // for exports

class vtkObjectStore;
class vtkMultiProcessController;
class vtkCamera;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkInteractionProvider : public vtkProvider
{
public:
  static vtkInteractionProvider* New();
  vtkTypeMacro(vtkInteractionProvider, vtkProvider);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the object store to use.
   */
  void SetObjectStore(vtkObjectStore* store);
  vtkSmartPointer<vtkObjectStore> GetObjectStore() const;
  ///@}

  ///@{
  static vtkPacket Render(vtkTypeUInt32 gid, vtkCamera* camera);
  ///@}
protected:
  vtkInteractionProvider();
  ~vtkInteractionProvider() override;

  void InitializeInternal(vtkService* service) override;

private:
  vtkInteractionProvider(const vtkInteractionProvider&) = delete;
  void operator=(const vtkInteractionProvider&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
