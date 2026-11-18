#### Verify that the Clip and Slice filters give a vtkPartitionedDataSetCollection for AMR input
from paraview.simple import *

amr = AMRGaussianPulseSource()

for filter in (Clip(Input=amr), Slice(Input=amr)):
    filter.UpdatePipeline()
    info = filter.GetDataInformation()
    assert info.GetDataSetTypeAsString() == 'vtkPartitionedDataSetCollection', \
        '%s gives %s' % (filter.GetXMLName(), info.GetDataSetTypeAsString())
    assert info.GetNumberOfCells() > 0, '%s gives no cells' % filter.GetXMLName()
