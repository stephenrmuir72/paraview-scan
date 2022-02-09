/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGUIOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGUIOptions.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include <vtk_cli11.h>

class vtkPVGUIOptions::vtkInternals
{
public:
  std::vector<std::string> ServerConfigurationsFiles;
  std::string ServerResourceName;
  std::string ServerURL;
  std::string DataDirectory;
  std::string TestDirectory;
  std::string BaselineDirectory;
};

#define CHECK_READ_ONLY()                                                                          \
  vtkLogIfF(ERROR, this->GetReadOnly(), "Options is read-only!");                                  \
  if (this->GetReadOnly())                                                                         \
  {                                                                                                \
    return;                                                                                        \
  }

vtkStandardNewMacro(vtkPVGUIOptions);
//----------------------------------------------------------------------------
vtkPVGUIOptions::vtkPVGUIOptions()
  : Internals(new vtkPVGUIOptions::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVGUIOptions::~vtkPVGUIOptions() = default;

//----------------------------------------------------------------------------
const std::vector<std::string>& vtkPVGUIOptions::GetServerConfigurationsFiles() const
{
  const auto& internals = (*this->Internals);
  return internals.ServerConfigurationsFiles;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::SetServerConfigurationsFiles(const std::vector<std::string>& files)
{
  CHECK_READ_ONLY();
  auto& internals = (*this->Internals);
  internals.ServerConfigurationsFiles = files;
}

//----------------------------------------------------------------------------
const std::string& vtkPVGUIOptions::GetServerResourceName() const
{
  const auto& internals = (*this->Internals);
  return internals.ServerResourceName;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::SetServerResourceName(const std::string& name)
{
  CHECK_READ_ONLY();
  auto& internals = (*this->Internals);
  internals.ServerResourceName = name;
}

//----------------------------------------------------------------------------
const std::string& vtkPVGUIOptions::GetServerURL() const
{
  const auto& internals = (*this->Internals);
  return internals.ServerURL;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::SetServerURL(const std::string& name)
{
  CHECK_READ_ONLY();
  auto& internals = (*this->Internals);
  internals.ServerURL = name;
}

//----------------------------------------------------------------------------
const std::string& vtkPVGUIOptions::GetBaselineDirectory() const
{
  const auto& internals = (*this->Internals);
  return internals.BaselineDirectory;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::SetBaselineDirectory(const std::string& name)
{
  CHECK_READ_ONLY();
  auto& internals = (*this->Internals);
  internals.BaselineDirectory = name;
}

//----------------------------------------------------------------------------
const std::string& vtkPVGUIOptions::GetTestDirectory() const
{
  const auto& internals = (*this->Internals);
  return internals.TestDirectory;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::SetTestDirectory(const std::string& name)
{
  CHECK_READ_ONLY();
  auto& internals = (*this->Internals);
  internals.TestDirectory = name;
}

//----------------------------------------------------------------------------
const std::string& vtkPVGUIOptions::GetDataDirectory() const
{
  const auto& internals = (*this->Internals);
  return internals.DataDirectory;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::SetDataDirectory(const std::string& name)
{
  CHECK_READ_ONLY();
  auto& internals = (*this->Internals);
  internals.DataDirectory = name;
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::Populate(CLI::App& app)
{
  CHECK_READ_ONLY();

  this->Superclass::Populate(app);

  auto& internals = (*this->Internals);
  auto* groupConnection = app.add_option_group(
    "Connection options", "Options affecting connections between client/server processes");

  groupConnection->add_option("--servers-file", internals.ServerConfigurationsFiles,
    "Additional servers configuration file(s) (.pvsc) to use.");

  auto url = groupConnection->add_option("--url,--server-url", internals.ServerURL,
    "Server connection URL. On startup, the client will connect to the server using the URL "
    "specified.");

  groupConnection
    ->add_option("-s,--server", internals.ServerResourceName,
      "Server resource name. On startup, the client will connect to the server using the "
      "resource specified.")
    ->excludes(url);

  auto* groupTesting = app.add_option_group("Testing", "Testing specific options");

  groupTesting
    ->add_option("--baseline-directory", internals.BaselineDirectory,
      "Baseline directory where test recorder will store baseline images.")
    ->envname("PARAVIEW_TEST_BASELINE_DIR");

  groupTesting
    ->add_option("--test-directory", internals.TestDirectory,
      "Temporary directory used to output test results and other temporary files.")
    ->envname("PARAVIEW_TEST_DIR");

  groupTesting
    ->add_option(
      "--data-directory", internals.DataDirectory, "Directory containing data files for tests.")
    ->envname("PARAVIEW_DATA_ROOT");
}

//----------------------------------------------------------------------------
void vtkPVGUIOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
