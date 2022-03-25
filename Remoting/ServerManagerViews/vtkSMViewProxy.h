/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMViewProxy
 * @brief
 *
 */

#ifndef vtkSMViewProxy_h
#define vtkSMViewProxy_h

#include "vtkRemotingServerManagerViewsModule.h" // for exports
#include "vtkSMProxy.h"

class vtkBoundingBox;
class vtkCamera;
class vtkPacket;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSMSourceProxy;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  rxcpp::observable<bool> Update();

  void StillRender();

  /**
   * These are the most recent visible prop bounds. These are gathered from the
   * rendering service in the most recent `Update` call after the updated
   * geometries have been delivered (and processed) by the rendering service.
   * Until that happens, these values may not be correct.
   */
  const vtkBoundingBox& GetVisiblePropBounds() const;

  /**
   * Convenience method to simply reset the camera using the current bounds
   * returned by `GetVisiblePropBounds`. If the bounds are not valid, then this
   * method has no effect.
   */
  void ResetCameraUsingVisiblePropBounds();

  /**
   * Provides access to the render window.
   */
  vtkRenderWindow* GetRenderWindow() const;

  /**
   * Provides access to the render window interactor.
   */
  vtkRenderWindowInteractor* GetRenderWindowInteractor() const;

  /**
   * Return the camera.
   */
  vtkCamera* GetCamera() const;

  /**
   * Called by vtkProcessInitializer when a rendering result is available.
   */
  void PostRenderingResult(const vtkPacket& packet);

  ///@{
  /**
   * Use this to indicate that the process should use
   * vtkGenericOpenGLRenderWindow rather than vtkRenderWindow when creating an
   * new render window.
   */
  static void SetUseGenericOpenGLRenderWindow(bool val);
  static bool GetUseGenericOpenGLRenderWindow();
  ///@}

  /**
   * Returns true if the view can display the data produced by the producer's
   * port. Internally calls GetRepresentationType() and returns true only if the
   * type is valid a representation proxy of that type can be created.
   */
  virtual bool CanDisplayData(vtkSMSourceProxy* producer, int outputPort);

  /**
   * Create a default representation for the given source proxy.
   * Returns a new proxy.
   * In version 4.1 and earlier, subclasses overrode this method. Since 4.2, the
   * preferred way is to simply override GetRepresentationType(). That
   * ensures that CreateDefaultRepresentation() and CanDisplayData() both
   * work as expected.
   */
  virtual vtkSMProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

  /**
   * Returns the xml name of the representation proxy to create to show the data
   * produced in this view, if any. Default implementation checks if the
   * producer has any "Hints" that define the representation to create in this
   * view and if so, returns that.
   * Or if this->DefaultRepresentationName is set and its Input property
   * can accept the data produced, returns this->DefaultRepresentationName.
   * Subclasses should override this method.
   */
  virtual const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort);

  /**
   * Finds the representation proxy showing the data produced by the provided
   * producer, if any. Note the representation may not necessarily be visible.
   */
  vtkSMProxy* FindRepresentation(vtkSMSourceProxy* producer, int outputPort);

  ///@{
  /**
   * Method used to hide other representations if the view has a
   * `<ShowOneRepresentationAtATime/>` hint.
   * This only affects other representations that have data inputs, not non-data
   * representations.
   *
   * @returns true if any representations were hidden by this call, otherwise
   *         returns false.
   */
  virtual bool HideOtherRepresentationsIfNeeded(vtkSMProxy* repr);
  static bool HideOtherRepresentationsIfNeeded(vtkSMViewProxy* self, vtkSMProxy* repr)
  {
    return self ? self->HideOtherRepresentationsIfNeeded(repr) : false;
  }
  ///@}

  ///@{
  /**
   * Certain views maintain properties (or other state) that should be updated
   * when visibility of representations is changed e.g. SpreadSheetView needs to
   * update the value of the "FieldAssociation" when a new data representation
   * is being shown in the view. Subclasses can override this method to perform
   * such updates to View properties. This is called explicitly by the
   * `vtkSMParaViewPipelineControllerWithRendering` after changing
   * representation visibility. Changes to representation visibility outside of
   * `vtkSMParaViewPipelineControllerWithRendering` will require calling this
   * method explicitly.
   *
   * Default implementation does not do anything.
   */
  virtual void RepresentationVisibilityChanged(vtkSMProxy* repr, bool new_visibility);
  static void RepresentationVisibilityChanged(
    vtkSMViewProxy* self, vtkSMProxy* repr, bool new_visibility)
  {
    if (self)
    {
      self->RepresentationVisibilityChanged(repr, new_visibility);
    }
  }
  ///@}

  /**
   * Helper method to locate a view to which the representation has been added.
   */
  static vtkSMViewProxy* FindView(vtkSMProxy* repr, const char* reggroup = "views");

protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy() override;

  /**
   * Read attributes from an XML element.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  void ProcessPiggybackedInformation(const vtkNJson& json) override;

  /**
   * Initialize the interactor.
   */
  virtual void InitializeInteractor(vtkRenderWindowInteractor* iren);

private:
  vtkSMViewProxy(const vtkSMViewProxy&) = delete;
  void operator=(const vtkSMViewProxy&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  static bool UseGenericOpenGLRenderWindow;
};

#endif
