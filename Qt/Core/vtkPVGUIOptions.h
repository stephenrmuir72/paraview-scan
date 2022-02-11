/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGUIOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVGUIOptions
 * @brief options subclass for Qt GUI specific options.
 *
 */

#ifndef vtkPVGUIOptions_h
#define vtkPVGUIOptions_h

#include "pqCoreModule.h" // for exports
#include "vtkPVApplicationOptions.h"

#include <vector> // for std::vector

class PQCORE_EXPORT vtkPVGUIOptions : public vtkPVApplicationOptions
{
public:
  static vtkPVGUIOptions* New();
  vtkTypeMacro(vtkPVGUIOptions, vtkPVApplicationOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Provides a list of server configurations files to
   * use instead of the default user-specific server configurations file.
   */
  const std::vector<std::string>& GetServerConfigurationsFiles() const;
  void SetServerConfigurationsFiles(const std::vector<std::string>& files);
  ///@}

  ///@{
  /**
   * The server connection url to use to connect to the server process
   * on startup, if any.
   *
   * @sa GetServerResourceName
   */
  const std::string& GetServerURL() const;
  void SetServerURL(const std::string& url);
  ///@}

  ///@{
  /**
   * The server connection resource name to
   * use to connect to the server process(es) on startup, if any.
   */
  const std::string& GetServerResourceName() const;
  void SetServerResourceName(const std::string& name);
  ///@}

  ///@{
  /**
   * The baseline directory to use for image regression tests.
   */
  const std::string& GetBaselineDirectory() const;
  void SetBaselineDirectory(const std::string& name);
  ///@}

  ///@{
  /**
   * The temporary directory to use for generating temporary files or test
   * results.
   */
  const std::string& GetTestDirectory() const;
  void SetTestDirectory(const std::string& name);
  ///@}

  ///@{
  /**
   * The data directory to use for generating temporary files or test
   * results.
   */
  const std::string& GetDataDirectory() const;
  void SetDataDirectory(const std::string& name);
  ///@}

  void Populate(CLI::App& app) override;

protected:
  vtkPVGUIOptions();
  ~vtkPVGUIOptions() override;

private:
  vtkPVGUIOptions(const vtkPVGUIOptions&) = delete;
  void operator=(const vtkPVGUIOptions&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
