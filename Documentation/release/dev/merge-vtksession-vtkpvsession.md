## Merge vtkSession and vtkPVSession classes

`vtkSession` and `vtkPVSession` classes are now merged to resolve naming conflicts in install tree
caused by the introduction of a new `vtkSession` class in VTK.

Additionally, the `vtkSessionIterator` class is renamed to `vtkPVSessionIterator` to maintain consistency.
