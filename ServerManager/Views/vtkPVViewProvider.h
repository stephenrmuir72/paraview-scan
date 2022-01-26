/*=========================================================================

  Program:   ParaView
  Module:    vtkPVViewProvider.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVViewProvider
 * @brief
 *
 */

#ifndef vtkPVViewProvider_h
#define vtkPVViewProvider_h

#include "vtkPacket.h" // for vtkPacket
#include "vtkProvider.h"
#include "vtkServerManagerViewsModule.h" // for exports

class vtkObjectStore;
class vtkMultiProcessController;

class VTKSERVERMANAGERVIEWS_EXPORT vtkPVViewProvider : public vtkProvider
{
public:
  static vtkPVViewProvider* New();
  vtkTypeMacro(vtkPVViewProvider, vtkProvider);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the object store to use.
   */
  void SetObjectStore(vtkObjectStore* store);
  vtkSmartPointer<vtkObjectStore> GetObjectStore() const;
  ///@}

  static const char* GetImageChannelName() { return "pvview-image-stream"; }
  static const char* GetDataChannelName() { return "pvview-data-stream"; }

protected:
  vtkPVViewProvider();
  ~vtkPVViewProvider();

  void InitializeInternal(vtkService* service) override;

private:
  vtkPVViewProvider(const vtkPVViewProvider&) = delete;
  void operator=(const vtkPVViewProvider&) = delete;

  void HandleObjectStoreEvent(vtkObject* caller, unsigned long eventId, void* callData);
  void PushViewData(vtkObject* caller, unsigned long eventId, void* callData);
  void PushViewImage(vtkObject* caller, unsigned long eventId, void* callData);

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
