// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPMergeConnected.h"

#include <cstring>
#include <iostream>
#include <map>

#include <vtkCellData.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkDataObjectTree.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkDataObjectTypes.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkPolyhedron.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnstructuredGrid.h>

#include <vtkSmartPointer.h>
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
#define VTK_NEW(type, name) name = vtkSmartPointer<type>::New()

namespace
{
// vtkDataObjectTree only exposes child read access (GetChild(),
// GetNumberOfChildren()) publicly; the setters are protected, so callers go
// through a concrete subclass's own public API instead (which is how each
// subclass enforces its own structural invariants, e.g.
// vtkPartitionedDataSetCollection requiring every entry to be a non-null
// vtkPartitionedDataSet). The helpers below dispatch writes to the right
// subclass API for whichever of vtkMultiBlockDataSet or
// vtkPartitionedDataSetCollection is actually in use.
//
// The structure is flat: a top-level child is a single leaf
// vtkUnstructuredGrid, wrapped in a vtkPartitionedDataSet for
// vtkPartitionedDataSetCollection.

void SetNumberOfChildren(vtkDataObjectTree* tree, unsigned int n)
{
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(tree))
  {
    pdc->SetNumberOfPartitionedDataSets(n);
  }
  else if (auto mb = vtkMultiBlockDataSet::SafeDownCast(tree))
  {
    mb->SetNumberOfBlocks(n);
  }
}

// vtkPartitionedDataSetCollection always wraps each entry in its own
// vtkPartitionedDataSet, so a plain indexed child access is not enough to
// reach the leaf: use a leaf iterator scoped to that one entry to unwrap it,
// regardless of nesting.
vtkDataObject* GetLeaf(vtkDataObjectTree* tree, unsigned int index)
{
  vtkDataObject* child = tree->GetChild(index);
  auto subtree = vtkDataObjectTree::SafeDownCast(child);
  if (!subtree)
  {
    return child;
  }
  vtkSmartPointer<vtkDataObjectTreeIterator> iter;
  iter.TakeReference(subtree->NewTreeIterator());
  iter->VisitOnlyLeavesOn();
  iter->SkipEmptyNodesOn();
  iter->InitTraversal();
  return iter->IsDoneWithTraversal() ? nullptr : iter->GetCurrentDataObject();
}

void SetLeaf(vtkDataObjectTree* tree, unsigned int index, vtkDataObject* leaf)
{
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(tree))
  {
    pdc->SetNumberOfPartitions(index, leaf ? 1 : 0);
    if (leaf)
    {
      pdc->SetPartition(index, 0, leaf);
    }
  }
  else if (auto mb = vtkMultiBlockDataSet::SafeDownCast(tree))
  {
    mb->SetBlock(index, leaf);
  }
}
}

vtkStandardNewMacro(vtkPMergeConnected);

vtkPMergeConnected::vtkPMergeConnected()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPMergeConnected::~vtkPMergeConnected()
{
  this->SetController(nullptr);
}

void vtkPMergeConnected::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkPMergeConnected filter"
     << "\n";
}

void vtkPMergeConnected::SetController(vtkMultiProcessController* c)
{
  if ((c == nullptr) || (c->GetNumberOfProcesses() == 0))
  {
    this->NumProcesses = 1;
    this->MyId = 0;
  }

  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller != nullptr)
  {
    this->Controller->UnRegister(this);
    this->Controller = nullptr;
  }

  if (c == nullptr)
  {
    return;
  }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId = c->GetLocalProcessId();
}

vtkExecutive* vtkPMergeConnected::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

int vtkPMergeConnected::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo ? inInfo->Get(vtkDataObject::DATA_OBJECT()) : nullptr;
  if (!input)
  {
    return 0;
  }

  const char* outputClassName = vtkPartitionedDataSetCollection::SafeDownCast(input)
    ? "vtkPartitionedDataSetCollection"
    : "vtkMultiBlockDataSet";

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!output || !output->IsA(outputClassName))
  {
    vtkSmartPointer<vtkDataObject> newOutput;
    newOutput.TakeReference(vtkDataObjectTypes::NewDataObject(outputClassName));
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  }
  return 1;
}

int vtkPMergeConnected::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

int vtkPMergeConnected::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkDataObjectTree* input =
    vtkDataObjectTree::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObjectTree* output =
    vtkDataObjectTree::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int piece, numPieces;
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // Read file
  vtkMultiProcessController* contr = this->Controller;

  int sum = 0;
  int oops = ((piece != this->MyId) || (numPieces != this->NumProcesses));

  for (unsigned int i = piece; i < input->GetNumberOfChildren(); i += numPieces)
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(::GetLeaf(input, i));
    if (!ugrid)
    {
      vtkErrorMacro("Blocks in the input data are not vtkUnstructuredGrid");
      oops += 1;
      break;
    }

    vtkCellData* cd = ugrid->GetCellData();
    vtkPointData* pd = ugrid->GetPointData();
    vtkIdTypeArray* prid_array = vtkIdTypeArray::SafeDownCast(pd->GetArray("RegionId"));
    vtkIdTypeArray* crid_array = vtkIdTypeArray::SafeDownCast(cd->GetArray("RegionId"));
    vtkFloatArray* vol_array = vtkFloatArray::SafeDownCast(cd->GetArray("Volumes"));

    if (!prid_array || !crid_array || !vol_array)
    {
      vtkErrorMacro(
        "Input data does not have expected arrays.  vtkPMergeConnected expects input data"
        " to have 'RegionId' arrays on points and cells and a 'Volumes' array on the cells.");
      oops += 1;
      break;
    }
  }

  contr->Reduce(&oops, &sum, 1, vtkCommunicator::SUM_OP, 0);
  contr->Broadcast(&sum, 1, 0);

  if (sum > 0)
    return 1;

  if (!contr)
    return 1;

  ::SetNumberOfChildren(output, input->GetNumberOfChildren());

  int i, j, tb;
  tb = static_cast<int>(input->GetNumberOfChildren());
  for (i = piece; i < tb; i += numPieces)
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(::GetLeaf(input, i));

    VTK_CREATE(vtkUnstructuredGrid, ugrid_out);
    ugrid_out->SetPoints(ugrid->GetPoints());

    vtkCellData* cd = ugrid->GetCellData();
    vtkPointData* pd = ugrid->GetPointData();
    vtkCellData* ocd = ugrid_out->GetCellData();
    vtkPointData* opd = ugrid_out->GetPointData();

    vtkIdTypeArray* prid_array = vtkIdTypeArray::SafeDownCast(pd->GetArray("RegionId"));
    vtkIdTypeArray* crid_array = vtkIdTypeArray::SafeDownCast(cd->GetArray("RegionId"));
    vtkFloatArray* vol_array = vtkFloatArray::SafeDownCast(cd->GetArray("Volumes"));

    VTK_CREATE(vtkIdTypeArray, oprid_array);
    VTK_CREATE(vtkIdTypeArray, ocrid_array);
    VTK_CREATE(vtkFloatArray, ovol_array);

    // point data "RegionId" keep their old id for now
    oprid_array->DeepCopy(prid_array);
    oprid_array->SetName("RegionId");

    // Checking RegionId range
    double rid_range[2];
    crid_array->GetRange(rid_range, 0);

    // Initialize the size of rid arrays
    ocrid_array->SetName("RegionId");
    ovol_array->SetName("Volumes");

    ocrid_array->SetNumberOfComponents(1);
    ovol_array->SetNumberOfComponents(1);

    // Compute cell/point data
    for (j = rid_range[0]; j <= rid_range[1]; j++)
    {
      // Compute face stream of merged polyhedron cell
      VTK_CREATE(vtkIdList, mcell);
      MergeCellsOnRegionId(ugrid, j, mcell);
      ugrid_out->InsertNextCell(VTK_POLYHEDRON, mcell);

      // "RegionId" cells keep their old id
      ocrid_array->InsertNextValue(j);

      // Sum up individual volumes
      float vol = MergeCellDataOnRegionId(vol_array, crid_array, j);
      ovol_array->InsertNextValue(vol);
    }

    // Add new data arrays
    opd->AddArray(oprid_array);
    ocd->AddArray(ocrid_array);
    ocd->AddArray(ovol_array);

    ::SetLeaf(output, i, ugrid_out);
  }

  // Convert local region id to global id
  LocalToGlobalRegionId(contr, output);

  return 1;
}

// Convert local region id to global ones
void vtkPMergeConnected::LocalToGlobalRegionId(
  vtkMultiProcessController* contr, vtkDataObjectTree* data)
{
  int i, j;
  int rank, num_p, tb;
  num_p = contr->GetNumberOfProcesses();
  rank = contr->GetLocalProcessId();
  tb = static_cast<int>(data->GetNumberOfChildren());

  // Gather information about number of regions in each block
  int all_num_regions_local[tb];
  memset(all_num_regions_local, 0, sizeof(int) * tb);
  for (i = rank; i < tb; i += num_p)
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(::GetLeaf(data, i));
    vtkIdTypeArray* crid_array =
      vtkIdTypeArray::SafeDownCast(ugrid->GetCellData()->GetArray("RegionId"));

    double rid_range[2];
    crid_array->GetRange(rid_range, 0);
    all_num_regions_local[i] = rid_range[1] - rid_range[0] + 1;
  }

  int all_num_regions[tb];
  contr->AllReduce(all_num_regions_local, all_num_regions, tb, vtkCommunicator::SUM_OP);

  // Compute and adding offset and make local id global
  for (i = rank; i < tb; i += num_p)
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(::GetLeaf(data, i));
    vtkIdTypeArray* crid_array =
      vtkIdTypeArray::SafeDownCast(ugrid->GetCellData()->GetArray("RegionId"));
    vtkIdTypeArray* prid_array =
      vtkIdTypeArray::SafeDownCast(ugrid->GetPointData()->GetArray("RegionId"));

    int num_cells = crid_array->GetNumberOfTuples();
    int num_points = prid_array->GetNumberOfTuples();

    VTK_CREATE(vtkIdTypeArray, ocrid_array);
    VTK_CREATE(vtkIdTypeArray, oprid_array);

    ocrid_array->SetNumberOfComponents(1);
    oprid_array->SetNumberOfComponents(1);
    ocrid_array->SetNumberOfTuples(num_cells);
    oprid_array->SetNumberOfTuples(num_points);
    ocrid_array->SetName("RegionId");
    oprid_array->SetName("RegionId");

    int offset = 0;
    for (j = 0; j < i; j++)
      offset += all_num_regions[j];

    // vtkIdTypeArray doesn't update range when replace elements, hack around
    for (j = 0; j < num_cells; j++)
      ocrid_array->InsertValue(j, offset + crid_array->GetValue(j));
    for (j = 0; j < num_points; j++)
      oprid_array->InsertValue(j, offset + prid_array->GetValue(j));

    ugrid->GetCellData()->RemoveArray("RegionId");
    ugrid->GetCellData()->AddArray(ocrid_array);

    ugrid->GetPointData()->RemoveArray("Regionid");
    ugrid->GetPointData()->AddArray(oprid_array);
  }
}

// Comparison function to sort point id list
int compare_ids(const void* a, const void* b)
{
  vtkIdType A = *(static_cast<const vtkIdType*>(a));
  vtkIdType B = *(static_cast<const vtkIdType*>(b));
  return (static_cast<int>(A - B));
}

// Generate FaceWithKey struct from vtkIdList to use for indexing
vtkPMergeConnected::FaceWithKey* vtkPMergeConnected::IdsToKey(vtkIdList* ids)
{
  FaceWithKey* facekey = new FaceWithKey[1];

  int num_pts = ids->GetNumberOfIds();
  vtkIdType* key = new vtkIdType[num_pts];
  vtkIdType* orig = new vtkIdType[num_pts];

  int i;
  for (i = 0; i < num_pts; i++)
    orig[i] = ids->GetId(i);
  memcpy(key, orig, sizeof(vtkIdType) * num_pts);
  qsort(key, num_pts, sizeof(vtkIdType), compare_ids);

  facekey->num_pts = num_pts;
  facekey->key = key;
  facekey->orig = orig;

  return facekey;
}

// Compare function in face_map
struct vtkPMergeConnected::cmp_ids
{
  bool operator()(FaceWithKey const* a, FaceWithKey const* b) const
  {
    int i, ret = 0;

    if (a->num_pts == b->num_pts)
    {
      for (i = 0; i < a->num_pts; i++)
      {
        if (a->key[i] != b->key[i])
        {
          ret = a->key[i] - b->key[i];
          break;
        }
      }
    }
    else
      ret = a->num_pts - b->num_pts;

    return ret < 0;
  }
};

// Delete a FaceWithKey struct
void vtkPMergeConnected::delete_key(FaceWithKey* key)
{
  delete[] key->key;
  delete[] key->orig;
  delete[] key;
}

// Face stream of a polyhedron cell in the following format:
// numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...
void vtkPMergeConnected::MergeCellsOnRegionId(
  vtkUnstructuredGrid* ugrid, int target, vtkIdList* facestream)
{
  int i, j;

  vtkIdTypeArray* rids = vtkIdTypeArray::SafeDownCast(ugrid->GetCellData()->GetArray("RegionId"));

  // Initially set -1 for number of faces in facestream
  facestream->InsertNextId(-1);

  std::map<FaceWithKey*, int, cmp_ids> face_map;
  std::map<FaceWithKey*, int, cmp_ids>::iterator it;

  // Create face map and count how many times a face is shared.
  for (i = 0; i < rids->GetNumberOfTuples(); i++)
  {
    if (rids->GetTuple1(i) == target)
    {
      vtkPolyhedron* cell = vtkPolyhedron::SafeDownCast(ugrid->GetCell(i));
      for (j = 0; j < cell->GetNumberOfFaces(); j++)
      {
        vtkCell* face = cell->GetFace(j);
        vtkIdList* pts = face->GetPointIds();
        FaceWithKey* key = IdsToKey(pts);

        it = face_map.find(key);
        if (it == face_map.end())
          face_map[key] = 1;
        else
          it->second++;
      }
    }
  }

  // Keep those unshared faces and build up a new cell
  int face_count = 0;
  for (it = face_map.begin(); it != face_map.end(); ++it)
  {
    FaceWithKey* key = (*it).first;
    int count = (*it).second;

    if (count > 2)
      std::cerr << "error in building up face map" << std::endl;
    else if (count == 1)
    {
      facestream->InsertNextId(key->num_pts);
      for (i = 0; i < key->num_pts; i++)
        facestream->InsertNextId(key->orig[i]);
      face_count++;
    }
    delete_key(key);
  }
  facestream->SetId(0, face_count);
}

// For original cell data, sum them up if possible in merging the connected cells
float vtkPMergeConnected::MergeCellDataOnRegionId(
  vtkFloatArray* data_array, vtkIdTypeArray* rid_array, vtkIdType target)
{
  int i;
  float val = 0;

  for (i = 0; i < rid_array->GetNumberOfTuples(); i++)
    if (rid_array->GetTuple1(i) == target)
      val += data_array->GetTuple1(i);

  return val;
}
