## HTG Generate Fields

Generify HTG Visible Leaves Size to allow new fields to be computed. The filter has been renamed `HyperTreeGridGenerateFields`.\
`HyperTreeGridVisibleLeavesSize` is now deprecated.

`HyperTreeGridGenerateFields` iterates over all the cells of the HTG and uses the selected strategies to compute the cell data fields.\
Then, it does the same for field data, using the previously computed cell data if necessary.

To add a new field, simply define a new strategy that inherits from `vtkHyperTreeGridGenerateFieldStrategy` and add it to the filter.
