/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVInformation
 * @brief Superclass for information objects.
 *
 * Information objects are one of the mechanisms provided by the ParaView
 * ServerManager to collect information (or meta-data) from services. This
 * abstract class defines the API. Subclasses add support to collect different
 * kinds of information.
 */

#ifndef vtkPVInformation_h
#define vtkPVInformation_h

#include "vtkJson.h" // for vtkJson
#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

#include <memory> // for std::unique_ptr

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVInformation : public vtkObject
{
public:
  vtkTypeMacro(vtkPVInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Save/load state of the information object. This is not the meta-data
   * gather, but the options that dictate how the meta-data is gathered,
   * if any.
   */
  virtual vtkJson SaveState() const;
  virtual bool LoadState(const vtkJson& state);
  ///@}

  /**
   * Gather information from the target.
   */
  virtual bool GatherInformation(vtkObject* target) = 0;

  /**
   * Used to merge information between ranks in a distributed configuration.
   */
  virtual void AddInformation(vtkPVInformation* other) = 0;

  ///@{
  /**
   * Save/load information / meta-data gathered.
   */
  virtual bool LoadInformation(const vtkJson& json) = 0;
  virtual vtkJson SaveInformation() const = 0;
  ///@}

  ///@{
  /**
   * Returns true if this information should only be collected from the
   * root node in a distributed configuration.
   */
  bool GetRootOnly() const;
  ///@}

  /**
   * Returns whether this information can/should be collected on the RPC
   * thread instead of the main thread of the service. By default it is false.
   * Derived classes should override this function to change the default behavior.
   */
  virtual bool CanRunOnRPC() const;

protected:
  vtkPVInformation();
  ~vtkPVInformation() override;

  void SetRootOnly(bool);

private:
  vtkPVInformation(const vtkPVInformation&) = delete;
  void operator=(const vtkPVInformation&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
