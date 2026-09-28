## Image extractors can save all views in a layout

The **PNG** and **JPG** image extractors now have a **Save All Views** property. When checked, the extractor saves all views in the layout containing its view as a single image, like the **Save All Views** option when saving a screenshot. The **Separator Width** and **Separator Color** properties control the separator drawn between the views. Since extractors are used to generate extracts in situ, this makes it possible to generate combined multi-view images with Catalyst.

When saving a Catalyst state, the layouts used by such extractors are now saved along with all the views in them, even the ones without an extractor. In a Catalyst Python script, you can combine the views of a layout created with `CreateLayout` as follows:

```python
layout1 = CreateLayout(name='Layout #1')
layout1.SplitHorizontal(0, 0.5)
layout1.AssignView(1, renderView1)
layout1.AssignView(2, renderView2)
layout1.SetSize(1600, 800)

pNG1 = CreateExtractor('PNG', renderView1, registrationName='PNG1')
pNG1.Writer.SaveAllViews = 1
pNG1.Writer.ImageResolution = [1600, 800]
```

The **Phi-Theta** camera mode of these extractors moves the camera in all render views of the layout when **Save All Views** is checked.

Saving a screenshot with **Separator Width** set in `pvpython`, `pvbatch` or Catalyst now uses the requested separator width between views; previously, the separator width set on the layout was always used. Also, `SaveScreenshot(filename, view, SaveAllViews=1)` now saves all views in the layout containing `view`.
