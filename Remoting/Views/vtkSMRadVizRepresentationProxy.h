// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMRadVizRepresentationProxy
 *
 * vtkSMRadVizRepresentationProxy is the proxy for vtkPVRadVizRepresentation.
 * It keeps the VerticesRotation, LensCenter and LensRadius properties in
 * sync with the client-side representation when the user interactively
 * rotates the vertices or drags/resizes the lens in the view, so that the
 * property panel, state files and Python tracing reflect the current
 * interactive state. The properties are updated once per drag gesture, with
 * its final result, rather than continuously while it is in progress: see
 * OnEndInteractionEvent(). Every intervening change is only signaled by
 * vtkCommand::InteractionEvent on this proxy, so that clients (see
 * pqRadVizChartView) can call UpdatePropertiesFromRepresentation() mid-drag
 * at a pace of their choosing.
 */

#ifndef vtkSMRadVizRepresentationProxy_h
#define vtkSMRadVizRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMChartRepresentationProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMRadVizRepresentationProxy : public vtkSMChartRepresentationProxy
{
public:
  static vtkSMRadVizRepresentationProxy* New();
  vtkTypeMacro(vtkSMRadVizRepresentationProxy, vtkSMChartRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copies the client-side vtkPVRadVizRepresentation's current
   * VerticesRotation, LensCenter and LensRadius ivars into the
   * corresponding properties. Called by OnEndInteractionEvent(), once per
   * drag gesture with its final result; clients may also call it mid-drag,
   * e.g. in response to vtkCommand::InteractionEvent.
   */
  void UpdatePropertiesFromRepresentation();

protected:
  vtkSMRadVizRepresentationProxy();
  ~vtkSMRadVizRepresentationProxy() override;

  void CreateVTKObjects() override;

  /**
   * Called when the client-side vtkPVRadVizRepresentation fires
   * vtkCommand::InteractionEvent, i.e. on every change during a drag of the
   * vertices or lens. Re-fires the same event on this proxy, without
   * updating the properties.
   */
  void OnInteractionEvent();

  /**
   * Called when the client-side vtkPVRadVizRepresentation fires
   * vtkCommand::StartInteractionEvent, i.e. when the user presses the
   * mouse to start interactively dragging the vertices or lens. Re-fires
   * the same event on this proxy: see OnEndInteractionEvent() for why.
   */
  void OnStartInteractionEvent();

  /**
   * Called when the client-side vtkPVRadVizRepresentation fires
   * vtkCommand::EndInteractionEvent, i.e. when the user releases the mouse
   * after interactively dragging the vertices or lens. Calls
   * UpdatePropertiesFromRepresentation() to push the drag's final result
   * into the properties, then re-fires the same event on this proxy, so
   * that Qt-level code (see pqRadVizChartView) can, in turn, react once
   * per drag gesture instead of on every intervening change.
   */
  void OnEndInteractionEvent();

private:
  vtkSMRadVizRepresentationProxy(const vtkSMRadVizRepresentationProxy&) = delete;
  void operator=(const vtkSMRadVizRepresentationProxy&) = delete;
};

#endif
