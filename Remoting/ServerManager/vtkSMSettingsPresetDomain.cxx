// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSettingsPresetDomain.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSettings.h"
#include "vtkSMSourceProxy.h"

#include <iterator>

class vtkSMSettingsObserver : public vtkCommand
{
public:
  static vtkSMSettingsObserver* New()
  {
    vtkSMSettingsObserver* ret = new vtkSMSettingsObserver;
    return ret;
  }

  vtkTypeMacro(vtkSMSettingsObserver, vtkCommand);

  vtkSMSettingsObserver()
  {
    this->Owner = nullptr;
    this->Watchee = vtkSMSettings::GetInstance();
  }

  ~vtkSMSettingsObserver() override = default;

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventid),
    void* vtkNotUsed(calldata)) override
  {
    this->Owner->Update(nullptr);
    this->Owner->DomainModified();
  }

  void StartWatching()
  {
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    if (settings)
    {
      this->Watchee = settings;
      if (this->Watchee)
      {
        this->Watchee->AddObserver(vtkCommand::UpdateDataEvent, this);
      }
    }
  }

  void StopWatching()
  {
    if (this->Watchee)
    {
      this->Watchee->RemoveObserver(this);
    }
  }

  vtkWeakPointer<vtkSMSettingsPresetDomain> Owner;
  vtkWeakPointer<vtkSMSettings> Watchee;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSettingsPresetDomain);

//------------------------------------------------------------------------------
vtkSMSettingsPresetDomain::vtkSMSettingsPresetDomain()
{
  this->Observer = vtkSMSettingsObserver::New();
  this->Observer->Owner = this;
}

//------------------------------------------------------------------------------
vtkSMSettingsPresetDomain::~vtkSMSettingsPresetDomain()
{
  this->Observer->StopWatching();
  this->Observer->Delete();
}

//------------------------------------------------------------------------------
void vtkSMSettingsPresetDomain::Update(vtkSMProperty*)
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  std::vector<std::string> values{ "Custom defaults" };
  const auto& presets = settings->GetPresets();
  std::copy(presets.begin(), presets.end(), std::back_inserter(values));
  this->SetStrings(values);
}

//------------------------------------------------------------------------------
int vtkSMSettingsPresetDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
  {
    return 0;
  }
  this->Update(nullptr);

  // register to update self whenever presets list change
  this->Observer->StartWatching();
  return 1;
}

//------------------------------------------------------------------------------
void vtkSMSettingsPresetDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
