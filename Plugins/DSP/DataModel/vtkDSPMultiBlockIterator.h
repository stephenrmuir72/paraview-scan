// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class    vtkDSPMultiBlockIterator
 * @brief    Iterator for a data object tree of vtkTables.
 *
 * Iterates over the leaves of a vtkDataObjectTree, e.g. a
 * vtkMultiBlockDataSet of vtkTables, or a vtkPartitionedDataSetCollection
 * whose partitioned data sets each hold a single vtkTable partition.
 * Simple wrapper over a vtkCompositeDataIterator.
 *
 * @warning
 * Traverses the whole structure on creation.
 *
 * @sa
 * vtkDSPIterator vtkDSPTableIterator
 */

#ifndef vtkDSPMultiBlockIterator_h
#define vtkDSPMultiBlockIterator_h

#include "vtkDSPIterator.h"

#include <memory> // for std::unique_ptr

class vtkDataObjectTree;
class vtkTable;

class vtkDSPMultiBlockIterator : public vtkDSPIterator
{
public:
  vtkTypeMacro(vtkDSPMultiBlockIterator, vtkDSPIterator);
  static vtkDSPMultiBlockIterator* New(vtkDataObjectTree* tree);

  /**
   * Move the iterator to the first item.
   */
  void GoToFirstItem() override;

  /**
   * Move the iterator to the next item.
   */
  void GoToNextItem() override;

  /**
   * Return true if the iterator reached the end.
   */
  bool IsDoneWithTraversal() override;

  /**
   * Get the current item as a table.
   */
  vtkTable* GetCurrentTable() override;

  /**
   * Get the number of iterations needed to traverse the current item.
   * This is equivalent to the number of non-empty leaf blocks.
   */
  vtkIdType GetNumberOfIterations() override;

protected:
  vtkDSPMultiBlockIterator();
  ~vtkDSPMultiBlockIterator() override = default;

private:
  vtkDSPMultiBlockIterator(const vtkDSPMultiBlockIterator&) = delete;
  void operator=(const vtkDSPMultiBlockIterator&) = delete;

  static vtkDSPMultiBlockIterator* New();

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif // vtkDSPMultiBlockIterator_h
