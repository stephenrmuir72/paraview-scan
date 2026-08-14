// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPMergeConnected
 *
 *
 *  This filter merges connected voroni tessellation regions based on the
 *  global region ID.
 */

#ifndef vtkPMergeConnected_h
#define vtkPMergeConnected_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

class vtkDataObjectTree;
class vtkMultiProcessController;
class vtkUnstructuredGrid;
class vtkIdList;
class vtkFloatArray;
class vtkIdTypeArray;

// vtkPMergeConnected accepts either a vtkMultiBlockDataSet or a
// vtkPartitionedDataSetCollection on its input, and produces an output of
// the same composite data type. Internally, the algorithm operates on the
// input/output vtkDataObjectTree (the common base of both supported types):
// each top-level child is a single leaf vtkUnstructuredGrid, wrapped in a
// vtkPartitionedDataSet for vtkPartitionedDataSetCollection.
class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPMergeConnected : public vtkDataObjectAlgorithm
{
public:
  static vtkPMergeConnected* New();
  vtkTypeMacro(vtkPMergeConnected, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  struct FaceWithKey
  {
    int num_pts;
    vtkIdType *key, *orig;
  };
  struct cmp_ids;

protected:
  vtkPMergeConnected();
  ~vtkPMergeConnected() override;

  /**
   * Use a composite-data-aware executive so that this filter correctly
   * handles composite (multi-block or partitioned) inputs and outputs.
   */
  vtkExecutive* CreateDefaultExecutive() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPMergeConnected(const vtkPMergeConnected&) = delete;
  void operator=(const vtkPMergeConnected&) = delete;

  // parallelism
  int NumProcesses;
  int MyId;
  vtkMultiProcessController* Controller;
  void SetController(vtkMultiProcessController* c);

  // filter
  void LocalToGlobalRegionId(vtkMultiProcessController* contr, vtkDataObjectTree* data);
  void MergeCellsOnRegionId(vtkUnstructuredGrid* ugrid, int target, vtkIdList* facestream);
  float MergeCellDataOnRegionId(
    vtkFloatArray* data_array, vtkIdTypeArray* rid_array, vtkIdType target);

  void delete_key(FaceWithKey* key);
  FaceWithKey* IdsToKey(vtkIdList* ids);
};

#endif
