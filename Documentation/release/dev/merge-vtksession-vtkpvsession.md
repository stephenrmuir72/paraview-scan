## Merge vtkSession and vtkPVSession classes

`vtkSession` and `vtkPVSession` classes are now merged to resolve naming conflicts in install tree
caused by the introduction of a new `vtkSession` class in VTK.

Additionally, the `vtkSessionIterator` is deprecated and renamed to `vtkPVSessionIterator` to maintain consistency.
