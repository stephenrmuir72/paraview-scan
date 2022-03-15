/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialLibrary.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMaterialLibrary.h"

#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVLogger.h"
#include "vtkPVVersion.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayMaterialLibrary.h"
#endif

#include <cassert>
#include <vtksys/SystemTools.hxx>

#if defined(_WIN32) && !defined(__CYGWIN__)
const char ENV_PATH_SEP = ';';
#else
const char ENV_PATH_SEP = ':';
#endif

class vtkPVMaterialLibrary::vtkInternals
{
public:
  std::vector<std::string> SearchPaths;
  std::vector<std::string> Files;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  std::vector<std::string> MaterialNames;

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkNew<vtkOSPRayMaterialLibrary> MaterialLibrary;
#endif

  vtkInternals()
  {
    // Update search paths.

    // 1. start with environment variable.
    if (const char* env = vtksys::SystemTools::GetEnv("PV_MATERIALS_PATH"))
    {
      std::vector<std::string> paths;
      vtksys::SystemTools::Split(env, paths, ENV_PATH_SEP);
      this->SearchPaths.insert(this->SearchPaths.end(), paths.begin(), paths.end());
    }

    // Next, we look in some default locations relative to the executable.
    std::string vtklibs = vtkGetLibraryPathForSymbol(GetVTKVersion);
    if (vtklibs.empty())
    {
      // this can happen for static builds (see paraview/paraview#19775),
      // in which case we just we the executable path to locate the materials.
      auto pvapp = vtkPVCoreApplication::GetInstance();
      vtklibs = pvapp ? std::string(pvapp->GetApplicationDirPath()) : std::string();
    }

    // where materials might be in relation to that
    std::vector<std::string> prefixes = {
#if defined(_WIN32) || defined(__APPLE__)
      "materials"
#else
      "share/paraview-" PARAVIEW_VERSION "/materials"
#endif
    };
    // search
    vtkNew<vtkResourceFileLocator> locator;
    auto resource_dir = locator->Locate(vtklibs, prefixes, "ospray_mats.json");
    if (!resource_dir.empty())
    {
      // append results to search path
      this->SearchPaths.push_back(resource_dir);
    }
  }

  void LoadFile(const std::string& file)
  {
    this->Files.push_back(file);
    if (vtksys::SystemTools::FileIsFullPath(file))
    {
      this->LoadFileAbsolutePath(file);
    }
    else
    {
      this->LoadFileRelativePath(file);
    }
  }

private:
  void LoadFileAbsolutePath(const std::string& fname)
  {
    (void)fname;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    this->MaterialLibrary->ReadFile(fname.c_str());
#endif
  }

  void LoadFileRelativePath(const std::string& fname)
  {
    (void)fname;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    int rank = this->Controller ? this->Controller->GetLocalProcessId() : 0;
    int numRanks = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;

    std::string realFilename;
    if (rank == 0)
    {
      for (auto& dir : this->SearchPaths)
      {
        std::string fullpath = dir + "/" + fname;
        if (vtksys::SystemTools::FileExists(fullpath, /*isFile=*/true))
        {
          realFilename = fullpath;
          break;
        }
      }
    }

    if (numRanks > 1)
    {
      int size = static_cast<int>(realFilename.size());
      this->Controller->Broadcast(&size, 1, 0);
      if (size > 0)
      {
        char buffer[size + 1];
        strcpy(buffer, realFilename.c_str());
        this->Controller->Broadcast(buffer, size + 1, 0);
        realFilename = buffer;
      }
    }

    if (!realFilename.empty())
    {
      this->LoadFileAbsolutePath(realFilename);
    }
#endif
  }
};

vtkStandardNewMacro(vtkPVMaterialLibrary);
//-----------------------------------------------------------------------------
vtkPVMaterialLibrary::vtkPVMaterialLibrary()
  : Internals(new vtkPVMaterialLibrary::vtkInternals())
{
}

//-----------------------------------------------------------------------------
vtkPVMaterialLibrary::~vtkPVMaterialLibrary() = default;

//-----------------------------------------------------------------------------
void vtkPVMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkPVMaterialLibrary::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  internals.Controller = controller;
}

//-----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVMaterialLibrary::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//-----------------------------------------------------------------------------
void vtkPVMaterialLibrary::SetFiles(const std::vector<std::string>& files)
{
  vtkLogScopeF(INFO, "SetFiles(...)");
  auto& internals = (*this->Internals);
  std::set<std::string> oldFiles(internals.Files.begin(), internals.Files.end());
  std::set<std::string> newFiles(files.begin(), files.end());
  // note: removal of files is not really supported.

  std::set<std::string> to_add;
  std::set_difference(newFiles.begin(), newFiles.end(), oldFiles.begin(), oldFiles.end(),
    std::inserter(to_add, to_add.end()));
  for (auto& fname : to_add)
  {
    internals.LoadFile(fname);
  }
}

//-----------------------------------------------------------------------------
const std::vector<std::string>& vtkPVMaterialLibrary::GetFiles() const
{
  const auto& internals = (*this->Internals);
  return internals.Files;
}

//-----------------------------------------------------------------------------
vtkObject* vtkPVMaterialLibrary::GetMaterialLibrary() const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  const auto& internals = (*this->Internals);
  return internals.MaterialLibrary;
#else
  return nullptr;
#endif
}

//-----------------------------------------------------------------------------
bool vtkPVMaterialLibrary::ReadFile(const char* fileName)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  const auto& internals = (*this->Internals);
  return internals.MaterialLibrary->ReadFile(fileName);
#else
  (void)fileName;
  return false;
#endif
}

//-----------------------------------------------------------------------------
void vtkPVMaterialLibrary::WriteFile(const std::string& filename)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  const auto& internals = (*this->Internals);
  internals.MaterialLibrary->WriteFile(filename);
#else
  (void)filename;
#endif
}

//-----------------------------------------------------------------------------
const std::vector<std::string>& vtkPVMaterialLibrary::GetMaterialNames() const
{
  auto& internals = (*this->Internals);
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  auto setOfNames = internals.MaterialLibrary->GetMaterialNames();
  internals.MaterialNames.clear();
  internals.MaterialNames.reserve(setOfNames.size());
  std::copy(setOfNames.begin(), setOfNames.end(), std::back_inserter(internals.MaterialNames));
#endif
  return internals.MaterialNames;
}
