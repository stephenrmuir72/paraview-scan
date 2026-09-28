# Custom menus and toolbars

This example shows how plugins can add menus and toolbars to ParaView to expose
analysis tools, i.e. sources, filters and Python scripts, at locations of the
developer's choosing. Neither plugin requires compiling: load them using
`Tools > Manage Plugins > Load New` or make them load automatically, e.g. using
the `PV_PLUGIN_PATH` environment variable.

Both plugins add items to the same **Analysis Tools** menu and toolbar.

* `AnalysisTools.xml` is an XML-only plugin. It defines **Contour With
  Normals**, a pipeline of standard filters exposed as a single filter, and
  uses a `CustomMenus` element to add a source, standard filters, the new filter
  and inline Python scripts to the menu and toolbar, including a sub-menu.

* `AnalysisToolsPython.py` is a Python plugin. It uses the `smmenu` decorators
  from `paraview.util.vtkAlgorithm` to add **Normalized Height**, a filter
  implemented in Python, and Python functions to the menu and toolbar.

## `CustomMenus` XML

```xml
<ServerManagerConfiguration>
  <CustomMenus>
    <Menu name="MyTools" label="My Tools" show_in_toolbar="1">
      <Proxy group="filters" name="Cut" />
      <Script label="Say Hello" tooltip="Prints a greeting.">
        print("Hello!")
      </Script>
      <Menu label="More">
        ...
      </Menu>
    </Menu>
  </CustomMenus>
</ServerManagerConfiguration>
```

Each top-level `Menu` adds a menu to the menu bar (unless
`show_in_menubar="0"`) and, when `show_in_toolbar="1"`, a toolbar with the same
items. Menus and toolbars with the same name or label are shared, so several
plugins can add items to them. Items are:

* `Proxy`: a filter (`group="filters"`), applied to the selected pipeline
  objects, or a source (`group="sources"`), with optional `label` and `icon`.
* `Script`: Python code, inline or using a `filename` attribute, with optional
  `icon` and `tooltip`. Use a `<![CDATA[ ... ]]>` section for code containing
  `<` or `&`.
* `Separator`.
* `Menu`: a sub-menu, shown as a drop-down button in toolbars.

`icon` can be a Qt resource path, e.g. `:/pqWidgets/Icons/pqCalculator.svg`, or
a file path.

## `smmenu` decorators

```python
from paraview.util.vtkAlgorithm import smmenu, smproxy, ...

@smmenu.proxy(menu="My Tools", toolbar=True)
@smproxy.filter(label="My Filter")
...
class MyFilter(VTKPythonAlgorithmBase):
    ...

@smmenu.action(menu="My Tools/More", label="Say Hello", toolbar=True)
def say_hello():
    """Prints a greeting."""
    print("Hello!")
```

`smmenu.action` adds a module-level function, called when the item is
triggered. `smmenu.proxy` adds a filter or source defined in the plugin. Use `/`
in `menu` for sub-menus.
