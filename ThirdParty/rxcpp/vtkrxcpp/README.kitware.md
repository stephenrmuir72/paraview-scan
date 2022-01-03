# RxCpp fork for vtk-remoting

This branch contains changes required to embed RxCpp into vtk-remoting. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Ignore whitespace problems.
  * Add CMake code to integrate with VTK's module system.
