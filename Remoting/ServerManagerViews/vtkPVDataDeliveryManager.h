/*=========================================================================

  Program: ParaView
  Module:  vtkPVDataDeliveryManager

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVDataDeliveryManager
 * @brief manager for data-delivery.
 *
 * ParaView's multi-configuration / multi-process modes pose a challenge for
 * views. At runtime, the current configuration will determine which processes
 * have what parts of data and which processes are expected to "render" that data.
 * While views and their representations may add certain qualifiers to this
 * statement, generally speaking, all views have to support taking the data from
 * the data-processing nodes and delivering it to the rendering nodes. This is
 * where vtkPVDataDeliveryManager comes in play. It helps views (viz. vtkPVView
 * subclasses) move the data.
 */

#ifndef vtkPVDataDeliveryManager_h
#define vtkPVDataDeliveryManager_h

#include "vtkNJson.h" // for vtkNJson
#include "vtkObject.h"
#include "vtkRemotingServerManagerViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"                     // needed for iVar.
#include "vtkTuple.h"                            // needed for vtkTuple.
#include "vtkWeakPointer.h"                      // needed for iVar.

#include <map> // for std::map

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtentTranslator;
class vtkInformation;
class vtkPVDataRepresentation;
class vtkPVView;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkPVDataDeliveryManager : public vtkObject
{
public:
  static vtkPVDataDeliveryManager* New();
  vtkTypeMacro(vtkPVDataDeliveryManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the render-view. The view is not reference counted.
   */
  void SetView(vtkPVView*);
  vtkPVView* GetView() const;
  //@}

  ///@{
  /**
   * View uses these methods to register a representation with the storage. This
   * makes it possible for representations to communicate with the storage
   * directly using a self pointer, while enables views on different processes
   * to communicate information about representations using global ids.
   */
  void RegisterRepresentation(vtkPVDataRepresentation* repr);
  void UnRegisterRepresentation(vtkPVDataRepresentation*);
  ///@}

  ///@{
  /**
   * Representations (indirectly via vtkPVRenderView::SetPiece()) call this
   * method to register the geometry type they are rendering. Every
   * representation that requires delivering of any geometry must register with
   * the vtkPVDataDeliveryManager and never manage the delivery on its own.
   */
  void SetPiece(vtkPVDataRepresentation* repr, int port, vtkDataObject* data, bool useLOD);
  vtkSmartPointer<vtkDataObject> GetPiece(vtkPVDataRepresentation* repr, int port, bool useLOD);
  bool HasPiece(vtkPVDataRepresentation* repr, int port, bool useLOD);
  ///@}

  ///@{
  /**
   * Set/Get meta-data container for the specific piece. Views can use it to
   * store arbitrary metadata for each piece.
   */
  vtkInformation* GetInformation(vtkPVDataRepresentation* repr, int port, bool useLOD);
  ///@}

  /**
   * Returns the size for all visible geometries.
   */
  vtkTypeUInt64 GetVisibleDataSize(bool useLOD) const;

  ///@{
  /**
   * Get/Set data for rendering. Internal methods used by vtkPVViewProvider.
   */
  std::map<std::string, vtkSmartPointer<vtkObject>> GetData(vtkMTimeType timestamp) const;
  void SetData(const std::map<std::string, vtkSmartPointer<vtkObject>>& data);
  ///@}

protected:
  vtkPVDataDeliveryManager();
  ~vtkPVDataDeliveryManager() override;

private:
  vtkPVDataDeliveryManager(const vtkPVDataDeliveryManager&) = delete;
  void operator=(const vtkPVDataDeliveryManager&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
