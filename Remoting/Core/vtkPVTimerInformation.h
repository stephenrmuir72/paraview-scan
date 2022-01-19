/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTimerInformation
 * @brief   Holds timer log for all processes.
 *
 * I am using this information object to gather timer logs from all processes.
 */

#ifndef vtkPVTimerInformation_h
#define vtkPVTimerInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class VTKREMOTINGCORE_EXPORT vtkPVTimerInformation : public vtkPVInformation
{
public:
  static vtkPVTimerInformation* New();
  vtkTypeMacro(vtkPVTimerInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the threshold to use to gather the timer log information. This must
   * be set before calling GatherInformation().
   */
  void SetLogThreshold(double threshold);
  double GetLogThreshold() const;
  ///@}

  ///@{
  /**
   * Access to the logs.
   */
  size_t GetNumberOfLogs() const;
  const std::string& GetLog(size_t proc) const;
  ///@}

  ///@{
  vtkJson SaveState() const override;
  bool LoadState(const vtkJson& state) override;
  bool GatherInformation(vtkObject* target) override;
  void AddInformation(vtkPVInformation* other) override;
  bool LoadInformation(const vtkJson& json) override;
  vtkJson SaveInformation() const override;
  ///@}

protected:
  vtkPVTimerInformation();
  ~vtkPVTimerInformation() override;

private:
  vtkPVTimerInformation(const vtkPVTimerInformation&) = delete;
  void operator=(const vtkPVTimerInformation&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
