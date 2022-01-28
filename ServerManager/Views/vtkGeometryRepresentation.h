/*=========================================================================

  Program:   ParaView
  Module:    vtkGeometryRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGeometryRepresentation
 * @brief   representation for showing any datasets as
 * external shell of polygons.
 *
 * vtkGeometryRepresentation is a representation for showing polygon geometry.
 * It handles non-polygonal datasets by extracting external surfaces. One can
 * use this representation to show surface/wireframe/points/surface-with-edges.
 * @par Thanks:
 * The addition of a transformation matrix was supported by CEA/DIF
 * Commissariat a l'Energie Atomique, Centre DAM Ile-De-France, Arpajon, France.
 */

#ifndef vtkGeometryRepresentation_h
#define vtkGeometryRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkServerManagerViewsModule.h" // needed for exports

class VTKSERVERMANAGERVIEWS_EXPORT vtkGeometryRepresentation : public vtkPVDataRepresentation
{

public:
  static vtkGeometryRepresentation* New();
  vtkTypeMacro(vtkGeometryRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool val) override;

  //@{
  /**
   * This is temporary approach to control the number of ghost-levels to request
   * by default for data pipelines. Currently, change in ghost level
   * request causes the pipeline to re-execute which can be expensive. In an
   * ideal world, additional ghost levels can be automatically provided by
   * data-exchange between ranks. Until we do that, this is only mechanism
   * available to override the number of ghost levels requested by default.
   *
   * The default is 0 for structured pipelines, and 1 for unstructured
   * pipelines. When not running in parallel, however, these ghost-level
   * requests don't make sense and hence are generally ignored.
   *
   * Note, this is expected to change in the future, so use this with
   * caution.
   */
  static void SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(int);
  static int GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines();
  static void SetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines(int);
  static int GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines();
  //@}

  /**
   * This returns number of ghost level to request based on characteristics of
   * the pipelines.
   */
  static int GetNumberOfGhostLevelsToRequest(vtkInformation* outInfo);

protected:
  vtkGeometryRepresentation();
  ~vtkGeometryRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Overridden to request correct ghost-level to avoid internal surfaces.
   */
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ///@{
  void InitializeForDataProcessing() override;
  void InitializeForRendering() override;
  bool AddToView(vtkPVView* view) override;
  bool RemoveFromView(vtkPVView* view) override;
  bool RequestDataToRender(
    vtkInformationVector** inputVector, vtkInformationVector* outputVector) override;
  bool RequestPrepareForRender(vtkInformation* request, vtkInformationVector* inputVector) override;
  bool RequestRender(vtkInformation* request) override;
  ///@}

private:
  vtkGeometryRepresentation(const vtkGeometryRepresentation&) = delete;
  void operator=(const vtkGeometryRepresentation&) = delete;

  class vtkDataProcessingInternals;
  class vtkRenderingInternals;

  std::unique_ptr<vtkDataProcessingInternals> DInternals;
  std::unique_ptr<vtkRenderingInternals> RInternals;

  static int DefaultMinimumGhostLevelsToRequestForStructuredPipelines;
  static int DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines;
};

#endif
