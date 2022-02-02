/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutputPort.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMOutputPort
 * @brief   reference for an output port of a vtkAlgorithm.
 *
 * This object manages one output port of an algorithm. It is used
 * internally by vtkSMSourceProxy to manage all of it's outputs.
 * @attention
 * Previously, vtkSMOutputPort used to be a vtkSMProxy subclass since it was
 * indeed a Proxy for the output port. However that has now changed. This merely
 * sits as a datastructure to manage ports specific things like
 * data-information. However for backwards compatibility, to keep the impact
 * minimal, we leave this as a sub-class of a Proxy with GlobalID=0 and
 * Session=nullptr.
 */

#ifndef vtkSMOutputPort_h
#define vtkSMOutputPort_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer
#include "vtkWeakPointer.h"  // needed for vtkWeakPointer

#include <map> // needed for std::map

class vtkCollection;
class vtkPVDataAssemblyInformation;
class vtkPVDataInformation;
class vtkPVTemporalDataInformation;
class vtkSMCompoundSourceProxy;
class vtkSMSourceProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMOutputPort : public vtkSMProxy
{
public:
  static vtkSMOutputPort* New();
  vtkTypeMacro(vtkSMOutputPort, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns data information. If data information is marked
   * invalid, calls GatherDataInformation.
   * If data information is gathered then this fires the
   * vtkCommand::UpdateInformationEvent event.
   */
  vtkPVDataInformation* GetDataInformation();

  /**
   * Get rank-specific data information.
   */
  vtkPVDataInformation* GetRankDataInformation(int rank);

  //@{
  /**
   * For composite datasets, `GetDataInformation` returns summary data information for
   * all blocks combined. However, applications may require information about
   * specific subset of blocks. In that case, one can use this API. Internally,
   * the data information is cached per selector / assembly-name pair. That way,
   * if the data information is not re-gathered unless changed.
   *
   * @arg \c selector the selector expression
   * @arg \c assemblyName name of the assembly to use to apply the selector
   *         to to determine the subset. If none specified, hierarchy is assumed.
   */
  vtkPVDataInformation* GetSubsetDataInformation(
    const char* selector, const char* assemblyName = nullptr);
  //@}

  /**
   * A `GetSubsetDataInformation` overload that uses composite index. It is only
   * supported for multiblock datasets.
   */
  vtkPVDataInformation* GetSubsetDataInformation(unsigned int compositeIndex);

  /**
   * Returns data information collected over all timesteps provided by the
   * pipeline. If the data information is not valid, this results iterating over
   * the pipeline and hence can be slow. Use with caution.
   */
  vtkPVTemporalDataInformation* GetTemporalDataInformation();

  /**
   * Mark data information as invalid.
   */
  void InvalidateDataInformation();

  //@{
  /**
   * Returns the index of the port the output is obtained from.
   */
  vtkGetMacro(PortIndex, int);
  //@}

  /**
   * Provides access to the source proxy to which the output port belongs.
   */
  vtkSMSourceProxy* GetSourceProxy();

protected:
  vtkSMOutputPort();
  ~vtkSMOutputPort() override;

  void SetSourceProxy(vtkSMSourceProxy* src);

  // When set to non-nullptr, GetSourceProxy() returns this rather than the real
  // source-proxy set using SetSourceProxy(). This provides a mechanism for
  // vtkSMCompoundSourceProxy to take ownership of ports that don't really
  // belong to it.
  void SetCompoundSourceProxy(vtkSMCompoundSourceProxy* src);

  /**
   * An internal update pipeline method that subclasses may override.
   */
  void UpdatePipelineInternal(double time, bool doTime);

  // The index of the port the output is obtained from.
  vtkSetMacro(PortIndex, int);
  int PortIndex;

  vtkWeakPointer<vtkSMSourceProxy> SourceProxy;
  // vtkWeakPointer<vtkSMCompoundSourceProxy> CompoundSourceProxy;

  vtkPVDataInformation* DataInformation;
  vtkPVTemporalDataInformation* TemporalDataInformation;
  std::map<std::string, std::map<int, vtkSmartPointer<vtkPVDataInformation>>>
    SubsetDataInformations;
  std::map<int, vtkSmartPointer<vtkPVDataInformation>> RankDataInformations;

private:
  vtkSMOutputPort(const vtkSMOutputPort&) = delete;
  void operator=(const vtkSMOutputPort&) = delete;

  friend class vtkSMSourceProxy;
  friend class vtkSMCompoundSourceProxy;
};

#endif
