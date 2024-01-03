// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkPython.h" // must be first
#endif

#include "vtkInSituPipelinePython.h"

#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"

#if VTK_MODULE_ENABLE_VTK_vtkvtkm
#include <vtkm/cont/Initialize.h>
#endif

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkCPPythonScriptV2Helper.h"
#else
// dummy implementation.
class vtkCPPythonScriptV2Helper : public vtkObject
{
public:
  static vtkCPPythonScriptV2Helper* New();

private:
  vtkCPPythonScriptV2Helper() = default;
  ~vtkCPPythonScriptV2Helper() = default;
  vtkCPPythonScriptV2Helper(const vtkCPPythonScriptV2Helper&) = delete;
  void operator=(const vtkCPPythonScriptV2Helper&) = delete;
};
vtkStandardNewMacro(vtkCPPythonScriptV2Helper);
#endif

vtkStandardNewMacro(vtkInSituPipelinePython);
//----------------------------------------------------------------------------
vtkInSituPipelinePython::vtkInSituPipelinePython()
  : FileName(nullptr)
{
}

//----------------------------------------------------------------------------
vtkInSituPipelinePython::~vtkInSituPipelinePython()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::SetArguments(const std::vector<std::string>& args)
{
  if (args != this->Arguments)
  {
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
    if (this->Helper->IsImported())
    {
      vtkErrorMacro(
        "'SetArguments' called after script has been imported. It will have no effect.");
      return;
    }
#endif
    this->Arguments = args;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::SetParameters(const std::vector<std::string>& params)
{
  if (params != this->Parameters)
  {
    this->Parameters = params;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Initialize()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  // call some Python API.
  auto status = Py_IsInitialized();
  (void)status;
#if VTK_MODULE_ENABLE_VTK_vtkvtkm
  {
    char** argv = new char*[this->Arguments.size()];
    int argc = 0;
    for (std::size_t i = 0; i < this->Arguments.size(); ++i, ++argc)
    {
      argv[i] = vtksys::SystemTools::DuplicateString(this->Arguments[i].c_str());
    }
    vtkm::cont::Initialize(argc, argv);
    for (std::size_t i = 0; i < this->Arguments.size(); ++i)
    {
      delete[] argv[i];
    }
    delete[] argv;
  }
#endif
  return this->Helper->PrepareFromScript(this->FileName) && this->Helper->Import(this->Arguments) &&
    this->Helper->CatalystInitialize();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Execute(int timestep, double time)
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  return this->Helper->CatalystExecute(timestep, time, this->Parameters);
#else
  (void)time;
  (void)timestep;
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Results()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  return this->Helper->CatalystResults();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Finalize()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  return this->Helper->CatalystFinalize();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
}
