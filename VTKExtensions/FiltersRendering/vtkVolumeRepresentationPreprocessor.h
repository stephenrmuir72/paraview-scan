/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRepresentationPreprocessor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeRepresentationPreprocessor
 * @brief   prepare data object for volume rendering
 *
 * vtkVolumeRepresentationPreprocessor prepares data objects for volume rendering.  If
 * the data object is a data set, then the set is passed through a vtkDataSetTriangleFilter
 * before being output as a vtkUnstructuredGrid.  If the data object is a multiblock
 * dataset with at least one unstructured grid leaf node, then the unstructured grid
 * is extracted using vtkCompositeDataToUnstructuredGridFilter.  The TetrahedraOnly
 * property may be set and it will be passed to the vtkDataSetTriangleFilter.
 *
 * @sa
 * vtkExtractBlock vtkTriangleFilter
 */

#ifndef vtkVolumeRepresentationPreprocessor_h
#define vtkVolumeRepresentationPreprocessor_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h"                          // for vtkSmartPointer

#include <set>

class vtkCompositeDataSet;
class vtkExtractBlock;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkVolumeRepresentationPreprocessor
  : public vtkDataObjectAlgorithm
{
public:
  static vtkVolumeRepresentationPreprocessor* New();
  vtkTypeMacro(vtkVolumeRepresentationPreprocessor, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When On, the internal triangle filter will cull all 1D and 2D cells from the output.
   * The default is Off.
   */
  vtkSetMacro(TetrahedraOnly, int);
  vtkGetMacro(TetrahedraOnly, int);
  //@}

  //@{
  /**
   * Select the block indices to extract for volume rendering.
   * Default is 0.
   */
  bool AddExtractedBlockIndex(unsigned int index);
  bool RemoveExtractedBlockIndex(unsigned int index);
  bool RemoveAllExtractedBlockIndices();
  //@}

protected:
  vtkVolumeRepresentationPreprocessor();
  ~vtkVolumeRepresentationPreprocessor() override;

  void Tetrahedralize(vtkDataObject*, vtkDataObject*);
  void ExtractDataSet(vtkDataObject*, vtkDataObject*);

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int TetrahedraOnly;
  std::set<unsigned int> ExtractedBlockIndices;

private:
  vtkVolumeRepresentationPreprocessor(const vtkVolumeRepresentationPreprocessor&) = delete;
  void operator=(const vtkVolumeRepresentationPreprocessor&) = delete;
};

#endif
