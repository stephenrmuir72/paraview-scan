## Surface representations have a new coincident topology polygon offset property

The **Surface** representation now has a **Coincident Topology Polygon
Offset** property, in its **Miscellaneous** group. Use it to manually push a
surface's depth back by a given factor and units when another representation
sharing the same input, such as a separately added **Feature Edges**
representation, is coincident with it and causing z-fighting. The property
defaults to `0, 0` and combines additively with the offset ParaView applies
automatically to keep a surface behind an active selection overlay.
