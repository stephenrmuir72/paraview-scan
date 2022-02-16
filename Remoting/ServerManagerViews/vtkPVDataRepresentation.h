/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVDataRepresentation
 * @brief base class for all representations in ParaView
 *
 * vtkPVDataRepresentation is the base class for all representations in
 * ParaView. Representations have a unique role in ParaView pipelines. They
 * serve as a bridge between VTK's demand driven data processing pipeline which
 * pull-based and the ParaView's rendering pipeline which is push-based.
 */

#ifndef vtkPVDataRepresentation_h
#define vtkPVDataRepresentation_h

#include "vtkAlgorithm.h"
#include "vtkRemotingServerManagerViewsModule.h" // needed for exports

#include <memory> // for std::unique_ptr

class vtkPVView;
class vtkMultiProcessController;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkPVDataRepresentation : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkPVDataRepresentation, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Each representation is assigned a unique id.
   */
  void SetGlobalID(vtkTypeUInt32 gid);
  vtkTypeUInt32 GetGlobalID() const;
  ///@}

  ///@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkMultiProcessController* GetController() const;
  ///@}

  ///@{
  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  virtual void SetVisibility(bool val);
  bool GetVisibility() const;
  ///@}

  ///@{
  /**
   * Set the update time. This is the time that will be requested from the
   * upstream data processing pipeline during next update.
   */
  void SetUpdateTime(double time);
  double GetUpdateTime() const;
  ///@}

  //@{
  /**
   * This is solely intended to simplify debugging and use for any other purpose
   * is vehemently discouraged.
   */
  void SetLogName(const std::string& name);
  const std::string& GetLogName() const;
  //@}

  /**
   * Provides access to the view.
   */
  vtkPVView* GetView() const;

  /**
   * Returns the timestamp when `RequestData` was executed on the
   * representation.
   */
  vtkMTimeType GetPipelineDataTime() const;

  /**
   * Returns number of rendering ports i.e. data streams that this
   * representation uses. This is typically 1. Subclasses can change that by
   * calling `SetNumberOfRenderingPorts` in the constructor.
   */
  int GetNumberOfRenderingPorts() const;

  /**
   * @sa vtkAlgorithm::ProcessRequest
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ///@{
  virtual void InitializeForDataProcessing();
  virtual void InitializeForRendering();
  ///@}

protected:
  vtkPVDataRepresentation();
  ~vtkPVDataRepresentation() override;

  /**
   * Set the number of rendering ports. This must be called, if at all, in the
   * constructor of the subclass to avoid odd side-effects.
   */
  void SetNumberOfRenderingPorts(int ports);

  ///@{
  friend class vtkPVView;
  virtual bool AddToView(vtkPVView* view);
  virtual bool RemoveFromView(vtkPVView* view);
  bool ProcessViewRequest(vtkInformation* request);
  ///@}

  ///@{
  /**
   * vtkAlgorithm passes. While subclasses can override these, there shouldn't really be any need
   * for that. The default implementation should suffice for most cases.
   */
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  ///@}

  /**
   * Subclasses can override this method to provide "geometries" for rendering.
   * Default implementation simply pass the input.
   */
  virtual bool RequestDataToRender(
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  /**
   * Subclasses can override this method to pass "geometries" provided for
   * rendering to rendering components e.g. mappers etc.
   */
  virtual bool RequestPrepareForRender(vtkInformation* request, vtkInformationVector* inputVector);

  /**
   * Subclasses can override this method to update rendering components just
   * before a render.
   */
  virtual bool RequestRender(vtkInformation* request);

private:
  vtkPVDataRepresentation(const vtkPVDataRepresentation&) = delete;
  void operator=(const vtkPVDataRepresentation&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
