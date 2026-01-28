# Add selection with Python

## Selection with Id Type Array

It is now possible to select by point/cell data (on a `vtkIdTypeArray`) with Python with the functions `SelectPointsData(ArrayName, IdValue, Source, Modifier)` and `SelectCellsData(ArrayName, IdValue, Source, Modifier)`

## Block selection

It is now possible to select points/cells from a block of a composite dataset with Python with the function `SelectBlocks(BlockSelector, Source FieldType, Modifier)`
