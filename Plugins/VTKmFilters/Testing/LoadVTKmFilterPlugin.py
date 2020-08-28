from paraview.simple import *

LoadPlugin("VTKmFilters" , ns=globals())

Wavelet()
UpdatePipeline()
VTKmContour()
UpdatePipeline()
