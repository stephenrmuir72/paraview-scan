## Minimize Empty Space in Preview mode

When using the preview mode in Paraview (View->Preview) the widget is locked
into sizes that are integral multiples of the target resolution. This has as a
result a lot of empty space in the preview window.

This MR creates previews that fill at least one of the dimensions of the widget
while maintaining the target (up to integer rounding error) aspect ratio.

Screenshots generated from these previews utilize internal buffers and thus are
unaffected from integer rounding errors.
