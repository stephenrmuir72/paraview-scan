// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOmegaHReader
 * @brief   reader for omega_h mesh files (.osh)
 *
 * omega_h stores a mesh as a directory ending in ".osh": ASCII sidecar files
 * "nparts" and "version" plus one binary part file "<rank>.osh" per writer
 * rank (see omega_h's src/Omega_h_file.cpp; format versions 9-11 are
 * supported here).
 *
 * vtkOmegaHReader parses the binary format itself; it does not depend on the
 * omega_h library. On P processes, part p, p+P, p+2P, ... is read by rank
 * p mod P (round robin) and every part becomes one partition of a
 * vtkPartitionedDataSet. Point and cell tags are exposed as point/cell data
 * arrays; when a mesh has multiple parts, entities owned by another part are
 * marked in the vtkGhostType array.
 *
 * A single part can also be read from a vtkResourceStream; see SetStream().
 */

#ifndef vtkOmegaHReader_h
#define vtkOmegaHReader_h

#include "vtkOmegaHReaderModule.h" // for export macro
#include "vtkPartitionedDataSetAlgorithm.h"

#include "vtkResourceStream.h" // for Stream ivar
#include "vtkSmartPointer.h"   // for ivars

class vtkMultiProcessController;

class VTKOMEGAHREADER_EXPORT vtkOmegaHReader : public vtkPartitionedDataSetAlgorithm
{
public:
  static vtkOmegaHReader* New();
  vtkTypeMacro(vtkOmegaHReader, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the mesh directory (e.g. "/path/to/mesh.osh").
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the stream to read a single part from. When both FileName and
   * Stream are set, the stream is used.
   *
   * The stream must be seekable and contain the bytes of a single omega_h
   * part file in the latest format version (11). Every rank reads the same
   * stream and produces a single-partition dataset.
   */
  void SetStream(vtkResourceStream* stream);
  vtkResourceStream* GetStream();
  ///@}

  ///@{
  /**
   * The controller used to read the mesh collectively. Defaults to the
   * global controller (vtkMultiProcessController::GetGlobalController());
   * a vtkDummyController is used when no global controller exists.
   */
  vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController*);
  ///@}

protected:
  vtkOmegaHReader();
  ~vtkOmegaHReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName = nullptr;
  vtkSmartPointer<vtkResourceStream> Stream;
  vtkSmartPointer<vtkMultiProcessController> Controller;

private:
  vtkOmegaHReader(const vtkOmegaHReader&) = delete;
  void operator=(const vtkOmegaHReader&) = delete;
};

#endif
