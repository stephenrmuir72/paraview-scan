## plugin-path-no-autoload

* Cmake option PARAVIEW_PLUGIN_CONF_EXTRA_PATHS may be used to define
  plugin directories containing plugins that are not intended to be
  autoloaded. The found plugins may be activated manually in the
  PluginManager. The option is analogous to
  PARAVIEW_PLUGIN_LOADER_PATHS, which autoloads found plugins.
  Example usage:
```
  cmake -DPARAVIEW_PLUGIN_CONF_EXTRA_PATHS="/usr/share/paraview/plugins"
```
