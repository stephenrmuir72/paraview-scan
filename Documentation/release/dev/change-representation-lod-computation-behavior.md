## Change geometry representation LOD computation behavior

When using geometry representation, if the LOD needs to be computed (depending on the LOD settings), it is now performed as soon as the pipeline is updated. This prevents a render view freeze when interacting with it or when resizing the window.
