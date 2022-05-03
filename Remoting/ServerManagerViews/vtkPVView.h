/*=========================================================================

  Program:   ParaView
  Module:    vtkPVView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVView
 * @brief   baseclass for all ParaView views.
 *
 * TODO:
 */

#ifndef vtkPVView_h
#define vtkPVView_h

#include "vtkObject.h"
#include "vtkPVDataRepresentation.h"             // until wrapping if fixed
#include "vtkPacket.h"                           // for vtkPacket
#include "vtkRemotingServerManagerViewsModule.h" // needed for exports
#include "vtkSmartPointer.h"                     // for vtkSmartPointer

#include <memory> // for std::unique_ptr
#include <vector> // for std::vector

class vtkDataObject;
class vtkInformation;
class vtkInformationDoubleKey;
class vtkInformationRequestKey;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkPVDataDeliveryManager;
class vtkPVDataRepresentation;
class vtkRenderWindow;
class vtkCamera;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkPVView : public vtkObject
{
public:
  vtkTypeMacro(vtkPVView, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Each view is assigned a unique id.
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
   * Set the position on this view in the multi-view configuration.
   */
  virtual void SetPosition(int xpos, int ypos);
  const int* GetPosition() const;
  ///@}

  //@{
  /**
   * Set the size of this view in the multiview configuration.
   */
  virtual void SetSize(int width, int height);
  const int* GetSize() const;
  ///@}

  ///@{
  /**
   * Set the screen PPI.
   */
  void SetPPI(int ppi);
  int GetPPI() const;
  ///@}

  ///@{
  /**
   * Get/Set the time this view is showing.
   */
  virtual void SetViewTime(double value);
  double GetViewTime() const;
  ///@}

  ///@{
  /**
   * This is solely intended to simplify debugging and use for any other purpose
   * is vehemently discouraged.
   */
  void SetLogName(const std::string& name);
  const std::string& GetLogName() const;
  ///@}

  /**
   * If this view needs a render window (not all views may use one),
   * this method can be used to get the render window associated with this view
   * on the current process. Note that this window may be shared with other
   * views depending on the process on which this is called and the
   * configuration ParaView is running under.
   */
  virtual vtkRenderWindow* GetRenderWindow() const { return nullptr; }

  /**
   * Returns active camera, if available.
   */
  virtual vtkCamera* GetActiveCamera() const { return nullptr; }

  ///@{
  /**
   * Assign a role to the view. This should be assigned immediately after
   * creation and should not change during the lifetime of the view.
   */
  virtual void InitializeForDataProcessing();
  virtual void InitializeForRendering();
  ///@}

  /**
   * Triggers a high-resolution render.
   */
  void StillRender(int pass = 0);

  /**
   * Triggers a interactive render. Based on the settings on the view, this may
   * result in a low-resolution rendering or a simplified geometry rendering.
   */
  void InteractiveRender(int pass = 0);

  ///@{
  void DoPreviewRender();
  void DoRender();
  ///@}

  enum RenderingModes
  {
    STILL_RENDER,
    INTERACTIVE_RENDER
  };

  /**
   * Returns vtkPVView::RenderingModes indicating the type used by the last
   * render.
   */
  int GetLastRenderingMode() const;

  /**
   * Updates the representations. This triggers various view passes based on the
   * role assigned to this view.
   */
  void Update();

  /**
   * This is called when new geometries are available on the rendering services.
   * The view will triggering rendering passes that provide the new data to the
   * representations so they can update their rendering pipelines.
   */
  void UpdateForRendering();

  static vtkInformationRequestKey* REQUEST_UPDATE_DATA();
  static vtkInformationRequestKey* REQUEST_UPDATE_DATA_LOD();
  static vtkInformationRequestKey* REQUEST_UPDATE_RENDERING_INFORMATION();
  static vtkInformationRequestKey* REQUEST_PREPARE_FOR_RENDER();
  static vtkInformationRequestKey* REQUEST_PREPARE_FOR_RENDER_LOD();
  static vtkInformationRequestKey* REQUEST_RENDER();
  static vtkInformationDoubleKey* UPDATE_TIME_STEP();

  ///@{
  void AddRepresentation(vtkPVDataRepresentation* representation);
  void RemoveRepresentation(vtkPVDataRepresentation* representation);
  void SetRepresentations(const std::vector<vtkPVDataRepresentation*>& representations);
  ///@}

  //@{
  /**
   * Use this to indicate that the process should use
   * vtkGenericOpenGLRenderWindow rather than vtkRenderWindow when creating an
   * new render window.
   */
  static void SetUseGenericOpenGLRenderWindow(bool val);
  static bool GetUseGenericOpenGLRenderWindow();
  //@}

  //@{
  /**
   * When saving screenshots with tiling, these methods get called.
   * Not to be confused with tile scale and viewport setup on tile display.
   *
   * @sa vtkViewLayout::UpdateLayoutForTileDisplay
   */
  void SetTileScale(int x, int y);
  void SetTileViewport(double x0, double y0, double x1, double y1);
  //@}

  /**
   * vtkViewLayout calls this method to update the total viewport available for
   * this view. Generally, views can assume viewport is [0, 0, 1, 1] i.e. the
   * view has control over the complete window. However, in tile display mode,
   * this may not be the case, and hence a reduced viewport may be passed.
   * Generally, subclasses don't need to do much more than scale viewport for
   * each renderer they create within the provided viewport.
   *
   * Default implementation iterates over all renderers in the render window and
   * scales each assuming reach render's viewport is [0, 0, 1, 1]. Subclasses
   * may want to override to update renderers for which that is not the case.
   */
  virtual void ScaleRendererViewports(const double viewport[4]);

  /**
   * Generates a channel name given the proxy's global id.
   */
  static std::string GetChannelName(vtkTypeUInt32 gid);

  /**
   * Returns true if the application is currently in tile display mode.
   */
  bool InTileDisplayMode();

  /**
   * Returns true if the application is currently in cave/immersive display
   * mode.
   */
  bool InCaveDisplayMode();

  /**
   * Representations can call this method to pass data for rendering to the
   * view. This can only be called on data-processing service.
   */
  static void SetPiece(
    vtkPVDataRepresentation* repr, int index, vtkDataObject* data, int lodLevel = 0);

  /**
   * Representations can call this method to obtain data for rendering. If
   * available, the data will be returned.
   */
  static vtkSmartPointer<vtkDataObject> GetPiece(
    vtkPVDataRepresentation* repr, int index, int lodLevel = 0);

  /**
   * Provides access to the view's data delivery manager.
   */
  vtkPVDataDeliveryManager* GetDataDeliveryManager() const;

  /**
   * Capture current rendering result and returns it.
   */
  vtkPacket GetRenderedResult() const;

  /**
   * Capture current rendering result and returns it. If metadata is not empty
   * it will be appended to the packet under the "metadata" key.
   */
  vtkPacket GetRenderedResult(vtkNJson& metadata) const;

  /**
   * This is used only on the rendering service to determine if the view
   * requires progressive passes. In which case, the
   * vtkRemoteObjectProviderViews ensures that `StillRender` or
   * `InteractiveRender` is called again.
   */
  virtual bool GetContinueRendering() const { return false; }

  /**
   * Returns the pass number for the most recent StillRender or
   * InteractiveRender call. This can be used in vtkCommand::RenderEvent handler
   * to determine which pass was rendered.
   */
  int GetLastRenderingPass() const;

protected:
  vtkPVView();
  ~vtkPVView() override;

  /**
   * Subclasses should use this method to create new render windows instead of
   * directly creating a new one.
   */
  vtkRenderWindow* NewRenderWindow();

  ///@{
  /**
   * Subclasses can override these methods to know when representations are
   * added or removed.
   */
  virtual void AddRepresentationInternal(vtkPVDataRepresentation* rep){};
  virtual void RemoveRepresentationInternal(vtkPVDataRepresentation* rep){};
  ///@}

  ///@{
  /**
   * Subclasses can use this method to trigger a pass on all representations.
   * @returns the count for representations that processed the call and returned
   * success.
   */
  int CallProcessViewRequest(vtkInformation* request);
  ///@}

  virtual void PrepareForRendering(){};

private:
  vtkPVView(const vtkPVView&) = delete;
  void operator=(const vtkPVView&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  static bool UseGenericOpenGLRenderWindow;
};

#endif
