// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGmshMetaFileReader
 * @brief   Reader for partitioned Gmsh solutions described by a .mshi meta-file.
 *
 * The meta-file is an XML file with a GmshMetaFile root element that gives:
 * - the number of partitioned .msh files (number_of_partitioned_msh_files);
 * - the geometry file pattern (GeometryInfo path);
 * - the field file patterns (FieldInfo/PathList path), each one an array;
 * - the time steps (TimeSteps), with indices and values;
 * - the default adaptation level and tolerance (AdaptView).
 * Patterns can contain [partID], [zeroPadPartID] (six digits) and [step];
 * relative paths are relative to the meta-file.
 *
 * Each partition is read with the Gmsh API. When arrays are selected, the field
 * files are merged onto the mesh and the high-order fields are adapted
 * (refined) into linear cells that carry the values as point data; "_p_N"
 * suffixes are removed from the view names. Otherwise only the mesh is read:
 * its 3D cells, or its 2D cells when there are no 3D ones.
 *
 * The output is a vtkPartitionedDataSetCollection with one partitioned dataset
 * that holds one partition per partitioned file; the partitions are distributed
 * across ranks.
 */

#ifndef vtkGmshMetaFileReader_h
#define vtkGmshMetaFileReader_h

#include "vtkGmshIOModule.h" // for export macro
#include "vtkNew.h"          // for vtkNew
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <memory> // for std::unique_ptr

class vtkDataArraySelection;
class vtkUnstructuredGrid;
struct vtkGmshMetaFileReaderInternals;

class VTKGMSHIO_EXPORT vtkGmshMetaFileReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkGmshMetaFileReader* New();
  vtkTypeMacro(vtkGmshMetaFileReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the .mshi meta-file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * The arrays to read, one per field file pattern of the meta-file. When none
   * is enabled, only the mesh is read.
   */
  vtkDataArraySelection* GetPointDataArraySelection();
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  ///@}

  ///@{
  /**
   * The adaptation (refinement) level and tolerance of the high-order fields.
   * A negative tolerance refines uniformly up to the level. A non-negative
   * tolerance is only used with a single array, since the arrays must share the
   * refined cells. Defaults are 0 and -0.0001.
   */
  vtkSetClampMacro(AdaptationLevel, int, 0, VTK_INT_MAX);
  vtkGetMacro(AdaptationLevel, int);
  vtkSetMacro(AdaptationTolerance, double);
  vtkGetMacro(AdaptationTolerance, double);
  ///@}

  ///@{
  /**
   * The adaptation level and tolerance that the meta-file suggests (AdaptView),
   * available after RequestInformation. Defaults are 0 and -0.0001.
   */
  vtkGetMacro(AdaptationLevelInfo, int);
  vtkGetMacro(AdaptationToleranceInfo, double);
  ///@}

protected:
  vtkGmshMetaFileReader();
  ~vtkGmshMetaFileReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkGmshMetaFileReader(const vtkGmshMetaFileReader&) = delete;
  void operator=(const vtkGmshMetaFileReader&) = delete;

  bool ParseMetaFile();
  int GetRequestedTimeStep(vtkInformation* outInfo) const;
  bool ReadPiece(int piece, int timeStep, vtkUnstructuredGrid* grid);
  void ReadAdaptedFields(vtkUnstructuredGrid* grid);

  char* FileName = nullptr;
  int AdaptationLevel = 0;
  double AdaptationTolerance = -0.0001;
  int AdaptationLevelInfo = 0;
  double AdaptationToleranceInfo = -0.0001;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  std::unique_ptr<vtkGmshMetaFileReaderInternals> Internals;
};

#endif // vtkGmshMetaFileReader_h
