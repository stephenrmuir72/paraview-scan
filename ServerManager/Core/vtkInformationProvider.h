/*=========================================================================

  Program:   ParaView
  Module:    vtkInformationProvider.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkInformationProvider
 * @brief provider that helps gathering information
 */

#ifndef vtkInformationProvider_h
#define vtkInformationProvider_h

#include "vtkPacket.h" // for vtkPacket
#include "vtkProvider.h"
#include "vtkServerManagerCoreModule.h"

class vtkObjectStore;
class vtkPVInformation;

class VTKSERVERMANAGERCORE_EXPORT vtkInformationProvider : public vtkProvider
{
public:
  static vtkInformationProvider* New();
  vtkTypeMacro(vtkInformationProvider, vtkProvider);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the object store to use.
   */
  void SetObjectStore(vtkObjectStore* store);
  vtkObjectStore* GetObjectStore() const;
  ///@}

  static vtkPacket GatherInformation(vtkPVInformation* information, vtkTypeUInt32 targetGID);
  static bool ParseResponse(const vtkPacket& packet, vtkPVInformation* information);

protected:
  vtkInformationProvider();
  ~vtkInformationProvider() override;

  void InitializeInternal(vtkService* service) override;

private:
  vtkInformationProvider(const vtkInformationProvider&) = delete;
  void operator=(const vtkInformationProvider&) = delete;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
