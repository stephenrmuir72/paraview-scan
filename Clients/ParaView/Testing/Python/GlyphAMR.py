#### Verify that the Glyph filter gives a vtkPartitionedDataSetCollection for AMR input
from paraview.simple import *

amr = AMRGaussianPulseSource()

glyph = Glyph(Input=amr, GlyphType='Arrow')
glyph.UpdatePipeline()
info = glyph.GetDataInformation()
assert info.GetDataSetTypeAsString() == 'vtkPartitionedDataSetCollection', \
    'Glyph gives %s' % info.GetDataSetTypeAsString()
assert info.GetNumberOfCells() > 0, 'Glyph gives no cells'
