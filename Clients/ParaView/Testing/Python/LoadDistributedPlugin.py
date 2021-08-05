#### import the simple module from the paraview
from paraview.simple import *

#### Load SurfaceLIC plugin
LoadDistributedPlugin('SurfaceLIC', remote=True, ns=globals())

# create a new 'Wavelet'
wavelet1 = Wavelet(registrationName='Wavelet1')

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.AssignView(0, renderView1)

# create a new 'Random Attributes'
randomAttributes1 = RandomAttributes(registrationName='RandomAttributes1', Input=wavelet1)

# Properties modified on randomAttributes1
randomAttributes1.GeneratePointVectors = 1

# show data in view
randomAttributes1Display = Show(randomAttributes1, renderView1, 'UniformGridRepresentation')

# change representation type
randomAttributes1Display.SetRepresentationType('Surface LIC')

from os.path import join
from paraview.vtk.vtkTestingRendering import vtkTesting
import sys
testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "LoadDistributedPlugin.png"))

testimage = join(testing.GetTempDirectory(), "LoadDistributedPlugin.png")
SaveScreenshot(testimage, layout=layout1, ImageResolution=[1280,800])

result = testing.RegressionTest(testimage, 10)
if result == testing.DO_INTERACTOR:
    sys.exit(0)
elif result == testing.NOT_RUN:
    sys.exit(125)
elif result == testing.FAILED:
    raise RuntimeError("test failed!")
