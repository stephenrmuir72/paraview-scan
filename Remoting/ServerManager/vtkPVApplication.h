/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVApplication
 * @brief
 *
 */

#ifndef vtkPVApplication_h
#define vtkPVApplication_h

#include "vtkPVCoreApplication.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

class vtkClientSession;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVApplication : public vtkPVCoreApplication
{
public:
  static vtkPVApplication* New();
  vtkTypeMacro(vtkPVApplication, vtkPVCoreApplication);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton once it has been created and initialized.
   */
  static vtkPVApplication* GetInstance();

  /**
   * Creates a new builtin session.
   *
   * Currently, only 1 builtin session can be created at a time. But that should
   * be fixable with a few code changes.
   */
  rxcpp::observable<vtkTypeUInt32> CreateBuiltinSession();

  /**
   * Creates a remote session.
   */
  rxcpp::observable<vtkTypeUInt32> CreateRemoteSession(const std::string& url);

  /**
   * Access a session given its id.
   */
  vtkClientSession* GetSession(vtkTypeUInt32 id) const;

  /**
   * Returns the session's id.
   */
  vtkTypeUInt32 GetSessionID(vtkClientSession* session) const;

  /**
   * Shutdown a session.
   */
  void CloseSession(vtkTypeUInt32 sessionId);

  /**
   * Get directory for user settings file. The last character is always the
   * file path separator appropriate for the system.
   */
  std::string GetUserSettingsDirectory() const;

  /**
   * Get file path for the user settings file.
   */
  std::string GetUserSettingsFilePath() const;

protected:
  vtkPVApplication();
  ~vtkPVApplication() override;

  /**
   * Loading settings json.
   */
  bool InitializeInternal() override;
  void FinalizeInternal() override;

private:
  vtkPVApplication(const vtkPVApplication&) = delete;
  void operator=(const vtkPVApplication&) = delete;

  ///@{
  /**
   * Load/Save vtkSMSettings to file.
   */
  void LoadSettings();
  void SaveSettings();
  ///@}

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
