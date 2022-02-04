/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplicationOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVApplicationOptions
 * @brief
 *
 * vtkPVApplicationOptions can be treated as thread-safe since the `Set*` APIs
 * are only expected to called before the application is started (or during
 * command line argument processing).
 */

#ifndef vtkPVApplicationOptions_h
#define vtkPVApplicationOptions_h

#include "vtkLogger.h" // for vtkLogger
#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports

#include <vtk_cli11_forward.h> // for namespace CLI
namespace CLI
{
class App;
}

#include <string> // for std::string
#include <vector> // for std::vector

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVApplicationOptions : public vtkObject
{
public:
  static vtkPVApplicationOptions* New();
  vtkTypeMacro(vtkPVApplicationOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to set verbosity to use for logging to stderr. This is generally set up
   * via command line arguments, but this API just exposes the same option. Any
   * value set after initialization will have no effect.
   */
  void SetLogStdErrVerbosity(vtkLogger::Verbosity verbosity);
  vtkLogger::Verbosity GetLogStdErrVerbosity() const;
  ///@}

  ///@{
  /**
   * Returns a vector of pairs for log files requested.
   */
  const std::vector<std::pair<std::string, vtkLogger::Verbosity>>& GetLogFiles() const;
  void SetLogFiles(const std::vector<std::pair<std::string, vtkLogger::Verbosity>>& files);
  ///@}

  ///@{
  /**
   * When set to true, the application is expected to ignore any user overrides
   * for settings and application options and use default configuration.
   */
  bool GetDisableRegistry() const;
  void SetDisableRegistry(bool value);
  ///@}

  ///@{
  /**
   * LocalHostname. This can be IP address or hostname. By default this is setup
   * using OS-specific calls to determine the hostname. If the default logic is
   * not yielding a reasonable value, one can use this to override it.
   */
  const std::string& GetLocalHostname() const;
  void SetLocalHostname(const std::string& value);
  ///@}

  ///@{
  /**
   * XDisplay test on server processes during initialization sometimes happens
   * too early and may result in remote rendering prematurely disabled. When
   * this flag is set, ParaView will skip such X-display tests. Note, if the
   * display is truly inaccessible when ParaView tries to connect to the server,
   * we will indeed get runtimes errors, including segfaults.
   */
  bool GetDisableXDisplayTests() const;
  void SetDisableXDisplayTests(bool value);
  ///@}

  ///@{
  /**
   * When set to true, ParaView will create headless only render windows on the
   * current process.
   */
  bool GetForceOffscreenRendering() const;
  void SetForceOffscreenRendering(bool value);
  ///@}

  ///@{
  /**
   * When set to true, ParaView will create on-screen render windows.
   */
  bool GetForceOnscreenRendering() const;
  void SetForceOnscreenRendering(bool value);
  ///@}

  ///@{
  /**
   * Get/Set whether stereo rendering should be enabled.
   */
  bool GetUseStereoRendering() const;
  void SetUseStereoRendering(bool value);
  ///@}

  ///@{
  /**
   * Get stereo type requested. Returned values are `VTK_STEREO_*` defined in
   * vtkRenderWindow.h.
   */
  int GetStereoType() const;
  const char* GetStereoTypeAsString() const;
  void SetStereoType(int value);
  ///@}

  ///@{
  /**
   * Eye separation to use when using stereo rendering.
   */
  double GetEyeSeparation() const;
  void SetEyeSeparator(double value);
  ///@}

  ///@{
  /**
   * Get/Set connection id.
   * The connection identifier used to validate client-server
   * connections.
   */
  int GetConnectID() const;
  void SetConnectID(int value);
  ///@}

  /**
   * Populate CLI::App with command line arguments to update options.
   */
  virtual void Populate(CLI::App& app);

  /**
   * Returns true if the options are no longer modifiable.
   */
  bool GetReadOnly() const;

  /**
   * To use a help text formatter that we decided is more suitable for
   * dense command line arguments like ParaView's, use this function.
   */
  static void SetupDefaults(CLI::App& app);

protected:
  vtkPVApplicationOptions();
  ~vtkPVApplicationOptions();

private:
  vtkPVApplicationOptions(const vtkPVApplicationOptions&) = delete;
  void operator=(const vtkPVApplicationOptions&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  friend class vtkPVCoreApplication;
  void SetReadOnly(bool value);
};

#endif
