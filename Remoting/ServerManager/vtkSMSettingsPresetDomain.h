// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMSettingsPresetDomain
 * @brief   list of settings presets
 *
 * vtkSMSettingsPresetDomain is used on vtkSMStringVectorProperty when the values
 * on the property correspond to settings presets in the input.
 */

#ifndef vtkSMSettingsPresetDomain_h
#define vtkSMSettingsPresetDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class vtkSMSettingsObserver;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSettingsPresetDomain : public vtkSMStringListDomain
{
public:
  static vtkSMSettingsPresetDomain* New();
  vtkTypeMacro(vtkSMSettingsPresetDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to get list of presets from vtkSMSettings singleton.
   */
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMSettingsPresetDomain();
  ~vtkSMSettingsPresetDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMSettingsPresetDomain(const vtkSMSettingsPresetDomain&) = delete;
  void operator=(const vtkSMSettingsPresetDomain&) = delete;

  friend class vtkSMSettingsObserver;
  vtkSMSettingsObserver* Observer;
};

#endif
