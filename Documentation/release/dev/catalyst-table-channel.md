## Catalyst: table channels

Catalyst 2 channels now accept `type: "table"`. The channel's `data` node
follows the Conduit table blueprint: each child of `values` is a column given
as a data array or an mcarray, and all columns have the same length. The
channel produces a `vtkTable`, or a multiblock of tables when `data` holds
several named tables. Use it for monitors, probes, residual histories and
other tabular output that has no mesh.
