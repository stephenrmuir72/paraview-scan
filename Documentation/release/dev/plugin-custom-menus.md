## Plugins can add custom menus and toolbars

Plugins, including XML-only and Python plugins that do not require compiling, can now add menus and toolbars to ParaView to expose analysis tools at locations of their choosing. Items can create sources and filters, including filters defined by the plugin, or run Python scripts. Several plugins can add items to the same menu and toolbar.

In XML, use a `CustomMenus` element:

```xml
<ServerManagerConfiguration>
  <CustomMenus>
    <Menu name="MyTools" label="My Tools" show_in_toolbar="1">
      <Proxy group="filters" name="Cut" />
      <Separator />
      <Script label="Say Hello" tooltip="Prints a greeting.">
        print("Hello!")
      </Script>
      <Menu label="More">
        <Proxy group="sources" name="SphereSource" />
      </Menu>
    </Menu>
  </CustomMenus>
</ServerManagerConfiguration>
```

In Python plugins, use the new `smmenu` decorators from `paraview.util.vtkAlgorithm`: `smmenu.action` adds a function to a menu and `smmenu.proxy` adds a filter or source defined by the plugin.

```python
@smmenu.action(menu="My Tools/More", label="Say Hello", toolbar=True)
def say_hello():
    """Prints a greeting."""
    print("Hello!")
```

See `Examples/Plugins/CustomMenus` for complete examples.
