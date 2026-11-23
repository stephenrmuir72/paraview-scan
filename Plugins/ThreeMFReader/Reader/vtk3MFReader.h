// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtk3MFReader
 * @brief   Read 3D Manufacturing Format (3MF) files.
 *
 * vtk3MFReader reads the triangle meshes of a 3MF package
 * (https://3mf.io/specification/). Each item of the model's build is
 * produced as a separate partition of the output partitioned dataset
 * collection, with the transforms of the build item and of any nested
 * components applied. Partitions are named after their object.
 *
 * Supported features:
 * - Core specification: meshes, components, build items, transforms, units,
 *   base materials and model metadata.
 * - Materials and Properties extension: color groups. Other property groups
 *   (textures, composite materials, multi-properties) are ignored.
 * - Production extension: objects stored in other model parts of the package
 *   and referenced through `p:path`.
 *
 * Output arrays:
 * - `ObjectId` (cell data): the resource id of the mesh object each triangle comes from.
 * - `Color` (cell data, RGBA unsigned char): the triangle color, from the triangle
 *   or object properties. Only produced when the file defines colors.
 *   Triangles with no color are white.
 * - `Unit` (field data): the model unit, e.g. "millimeter".
 * - One string field data array per model `metadata` entry, e.g. `Title`.
 *
 * Packages are read with a built-in ZIP reader using VTK's zlib; encrypted
 * and multi-disk archives are not supported.
 */

#ifndef vtk3MFReader_h
#define vtk3MFReader_h

#include "vtk3MFReaderModule.h" // for export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <string> // for std::string

class VTK3MFREADER_EXPORT vtk3MFReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtk3MFReader* New();
  vtkTypeMacro(vtk3MFReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the file name of the 3MF file to read.
   */
  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);
  ///@}

  /**
   * Return 1 if the file is a ZIP archive containing a 3MF model, 0 otherwise.
   */
  int CanReadFile(VTK_FILEPATH const char* fileName);

  ///@{
  /**
   * When on, scale point coordinates so that they are expressed in
   * millimeters, whatever the unit declared by the model. When off (default),
   * coordinates are kept in the model unit, which is reported by the `Unit`
   * field data array.
   */
  vtkSetMacro(ConvertToMillimeters, bool);
  vtkGetMacro(ConvertToMillimeters, bool);
  vtkBooleanMacro(ConvertToMillimeters, bool);
  ///@}

protected:
  vtk3MFReader();
  ~vtk3MFReader() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::string FileName;
  bool ConvertToMillimeters = false;

private:
  vtk3MFReader(const vtk3MFReader&) = delete;
  void operator=(const vtk3MFReader&) = delete;
};

#endif
