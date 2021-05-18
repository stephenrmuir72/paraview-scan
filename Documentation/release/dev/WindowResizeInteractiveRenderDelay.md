# View Property WindowResizeInteractiveRenderDelay

A new double property, 'WindowResizeInteractiveRenderDelay', on the view has been added
that lets you set the time in seconds before doing a full-resolution render after a WindowResize
event. This allows the view to render interactively when resizing a window, and can improve the
performance when using large data.
