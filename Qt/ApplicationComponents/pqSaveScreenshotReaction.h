/*=========================================================================

   Program: ParaView
   Module:    pqSaveScreenshotReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqSaveScreenshotReaction_h
#define pqSaveScreenshotReaction_h

#include "pqReaction.h"

#include <vtkImageData.h>
#include <vtkLegacy.h> // for legacy macros
#include <vtkSmartPointer.h>

class vtkSMSaveScreenshotProxy;

/**
* @ingroup Reactions
* @class pqSaveScreenshotReaction
* @brief Reaction to save a screenshot.
*
* pqSaveScreenshotReaction can be connected to a QAction to prompt user and save
* screenshots or captures from views using the `("misc", "SaveScreenshot")` proxy.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveScreenshotReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqSaveScreenshotReaction(QAction* parent, bool clipboardMode = false);

  /**
  * Saves the screenshot. This will prompt the user to choose a filename. Based
  * on the filename chosen, a dialog will be presented to the user to customize
  * screenshot settings like resolution, etc. and then the screenshot will be
  * saved.
  *
  * @returns true if a screenshot was saved, else false.
  */
  static bool saveScreenshot();

  /**
   * Same as `saveScreenshot` except that the user is not presented any prompts
   * to choose filename or other options.
   *
   * @returns true if a screenshot was saved, else false.
   */
  static bool saveScreenshot(
    const QString& filename, const QSize& size, int quality, bool all_views = false);

  /**
   * Saves a screenshot to the clipboard. If `all_views` is true, then
   * the screenshot for the active layout, including all the view in that layout
   * is saved. If `all_views` is false then only the active view is saved.
   * The size of the image will be same as the size of the active layout (or
   * view) respectively.
   *
   * @returns true if a screenshot was saved, else false.
   */
  static bool saveScreenshotToClipboard(bool all_views = false);

  /**
  * Save a screenshot to the clipboard at a given `size`.
  *
  * @returns true if a screenshot was saved, else false.
  */
  static bool saveScreenshotToClipboard(const QSize& size, bool all_views = false);

  //@{
  /**
   * Saving of the screenshot split into multiple stages. Use `prepare` to
   * create an initialize an options proxy. The proxy provides parameters that
   * can be configured to customize how the screenshot is saved.
   *
   * Use `prompt` to present to the user an ability to customize the options.
   *
   * Use `saveScreenshot` to generate the screenshot using the options proxy.
   */
  static vtkSmartPointer<vtkSMSaveScreenshotProxy> prepare(
    const QString& filename, bool all_views = false);
  static bool prompt(vtkSMSaveScreenshotProxy* options, const QString& title = QString());
  static bool saveScreenshot(const QString& filename, vtkSMSaveScreenshotProxy* options);
  //@}

  //@{
  /**
  * Take a screenshot of the active view or all the views at the given size
  * and return it as a vtkImageData.
  */
  static vtkSmartPointer<vtkImageData> takeScreenshot(const QSize& size, bool all_views = false);
  static vtkSmartPointer<vtkImageData> takeScreenshot(vtkSMSaveScreenshotProxy* options);
  //@}

  /**
  * Prompt user for filename using the various format proxies listed for the
  * given proxy.
  */
  static QString promptFileName(
    vtkSMSaveScreenshotProxy* saveProxy, const QString& defaultExtension);
  static QString promptFileName(const QString& defaultExtension);

  /**
   * @deprecated in ParaView 5.9, use copyScreenshotToClipboard().
   */
  VTK_LEGACY(static bool copyScreenshotToClipboard(const QSize& size, bool all_views = false));
  VTK_LEGACY(static void saveScreenshot(bool));

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqSaveScreenshotReaction)
  bool ClipboardMode;
};

#endif
