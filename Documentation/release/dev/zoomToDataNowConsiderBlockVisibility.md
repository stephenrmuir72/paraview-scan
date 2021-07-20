# Zoom to data actions now consider the visibility of the source and blocks

The Zoom To Data actions (Zoom To Data and Zoom Closest To Data) now consider the visibility of the source and its blocks :

* If the source or every block of it is invisible, then the buttons are disabled
* If only some blocks of the source are visible, then the Zoom To Data zoom only on the bounding box of the **visible** blocks.
