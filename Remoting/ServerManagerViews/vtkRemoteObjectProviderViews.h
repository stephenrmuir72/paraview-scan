/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteObjectProviderViews.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRemoteObjectProviderViews
 * @brief
 *
 */

#ifndef vtkRemoteObjectProviderViews_h
#define vtkRemoteObjectProviderViews_h

#include "vtkPacket.h" // for vtkPacket
#include "vtkRemoteObjectProvider.h"
#include "vtkRemotingServerManagerViewsModule.h" // for exports

class vtkObjectStore;
class vtkMultiProcessController;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkRemoteObjectProviderViews
  : public vtkRemoteObjectProvider
{
public:
  static vtkRemoteObjectProviderViews* New();
  vtkTypeMacro(vtkRemoteObjectProviderViews, vtkRemoteObjectProvider);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static const char* GetImageChannelName() { return "pvview-image-stream"; }
  static const char* GetDataChannelName() { return "pvview-data-stream"; }

protected:
  vtkRemoteObjectProviderViews();
  ~vtkRemoteObjectProviderViews();

  void InitializeInternal(vtkService* service) override;

private:
  vtkRemoteObjectProviderViews(const vtkRemoteObjectProviderViews&) = delete;
  void operator=(const vtkRemoteObjectProviderViews&) = delete;

  void HandleObjectStoreEvent(vtkObject* caller, unsigned long eventId, void* callData);
  void PushViewData(vtkObject* caller, unsigned long eventId, void* callData);
  void PushViewImage(vtkObject* caller, unsigned long eventId, void* callData);

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
