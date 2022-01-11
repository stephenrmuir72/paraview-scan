## Label components of non-symmetric tensors

Vectors of length 6 have long been interpreted as symmetric 3x3 matrices in
ParaView, and have had labels XX, YY, ZZ, XY, YZ, XZ attached to the
components.

This has now been updated to also interpret length 9 vectors as 3x3
non-symmetric matrices, with the same ordering for the first 6 components,
and YX, ZY, ZX for the last 3.
