## Catalyst: table channels

Catalyst 2 channels now accept `type: "table"`. The channel's `data` node
follows the Conduit table blueprint: each child of `values` is a column given
as a data array or an mcarray, and all columns have the same length. The
channel produces a `vtkTable`, or a partitioned dataset collection of tables
when `data` holds several named tables. A rank with no rows, for example for a
global quantity that only rank 0 holds, passes an empty `data` node; with
several named tables, every rank provides the same tables, using zero-length
columns where it has no rows. Use it for monitors, probes, residual histories
and other tabular output that has no mesh.
