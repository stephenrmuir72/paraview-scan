// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOmegaHReader
 * @brief   reader for omega_h mesh files (.osh)
 *
 * omega_h stores a mesh as a directory ending in ".osh": ASCII sidecar files
 * "nparts" and "version" plus one binary part file "<rank>.osh" per writer
 * rank. Format versions 9-11 are supported.
 *
 * vtkOmegaHReader parses the binary format itself; it does not depend on the
 * omega_h library. On P processes, part p, p+P, p+2P, ... is read by rank
 * p mod P (round robin) and every part becomes one partition of a
 * vtkPartitionedDataSet. Point and cell tags are exposed as point/cell data
 * arrays; when a mesh has multiple parts, entities owned by another part are
 * marked in the vtkGhostType array.
 *
 * A single part can also be read from a vtkResourceStream; see SetStream().
 *
 * omega_h was originally developed by Sandia
 * (https://github.com/sandialabs/omega_h); this reader is based on the
 * SCOREC fork (https://github.com/SCOREC/omega_h), which is the actively
 * maintained version. The .osh format has no standalone specification; it
 * is defined by omega_h's file reader/writer implementation
 * (https://github.com/SCOREC/omega_h/blob/master/src/Omega_h_file.cpp),
 * which was used as the reference for this reader.
 */

#ifndef vtkOmegaHReader_h
#define vtkOmegaHReader_h

#include "vtkOmegaHReaderModule.h" // for export macro
#include "vtkPartitionedDataSetAlgorithm.h"

#include "vtkResourceStream.h" // for Stream ivar
#include "vtkSmartPointer.h"   // for ivars

#include <string> // for std::string

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
  void SetFileName(VTK_FILEPATH const std::string& fname);
  VTK_FILEPATH std::string GetFileName() const;
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
   * Return true if, after a quick check, it looks like the provided
   * directory or stream is an omega_h mesh (.osh) this reader can load, and
   * false if it is sure it cannot be read. This does not modify nor use
   * members. If the mesh uses a format version this reader does not support,
   * this still returns true and the reader will fail on reading; this
   * enables ParaView to distinguish between failures where another reader
   * should be tried and failures where it should not. The stream version may
   * move the stream cursor.
   */
  virtual vtkTypeBool CanReadFile(VTK_FILEPATH const char* name);
  static bool CanReadFile(vtkResourceStream* stream);
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

  std::string FileName;
  vtkSmartPointer<vtkResourceStream> Stream;
  vtkSmartPointer<vtkMultiProcessController> Controller;

private:
  vtkOmegaHReader(const vtkOmegaHReader&) = delete;
  void operator=(const vtkOmegaHReader&) = delete;
};

#endif
