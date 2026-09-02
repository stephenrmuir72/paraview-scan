## New omega_h mesh reader

ParaView can now open [omega_h](https://github.com/SCOREC/omega_h) meshes (`.osh` files), including meshes
partitioned across multiple ranks. Each writer part is loaded as a
partition of a Partitioned Dataset, and point and cell tags are exposed
as data arrays.

The reader supports omega_h format versions 9 through 11 and is enabled
by building the `OmegaHReader` plugin. Build with
`-DPARAVIEW_PLUGIN_ENABLE_OmegaHReader=ON` and add
`-DPARAVIEW_PLUGIN_AUTOLOAD_OmegaHReader=ON` to read `.osh` files from
**File > Open**.
