#### Verify that the OmegaHReader reads omega_h meshes and exposes the
#### expected partitions, vertices, cells and tag arrays.
from paraview.simple import *
from paraview import servermanager as sm
from paraview import smtesting
import os

smtesting.ProcessCommandLineArguments()

DataDir = os.path.join(smtesting.DataDir, "Plugins", "OmegaHReader", "Testing", "Data")

# load plugin
LoadDistributedPlugin('OmegaHReader', ns=globals())

def read_mesh(name):
    return Omega_hMeshReader(registrationName=name + '.osh',
        FileName=os.path.join(DataDir, name + ".osh"))

# plate_6elem.osh: format version 9, one part of 8 vertices and 6 triangles.
plate = read_mesh("plate_6elem")
plate_out = sm.Fetch(plate)
assert plate_out.GetClassName() == "vtkPartitionedDataSet"
assert plate_out.GetNumberOfPartitions() == 1
plate_part = plate_out.GetPartition(0)
assert plate_part.GetNumberOfPoints() == 8
assert plate_part.GetNumberOfCells() == 6
assert set(plate_part.GetCellType(i) for i in range(plate_part.GetNumberOfCells())) == {5} # VTK_TRIANGLE
plate_point_data = plate_part.GetPointData()
plate_cell_data = plate_part.GetCellData()
assert set(plate_point_data.GetArrayName(i) for i in range(plate_point_data.GetNumberOfArrays())) == {"global", "class_id", "class_dim"}
assert set(plate_cell_data.GetArrayName(i) for i in range(plate_cell_data.GetNumberOfArrays())) == {"global", "class_id", "class_dim"}
# the element ids are global and unique
assert [plate_cell_data.GetArray("global").GetValue(i) for i in range(plate_part.GetNumberOfCells())] == [0, 1, 2, 3, 4, 5]

# box_3d_2p.osh: format version 9, two parts of 18 vertices and 24 tetrahedra
# each, with a boundary tag and ghost information.
box = read_mesh("box_3d_2p")
box_out = sm.Fetch(box)
assert box_out.GetNumberOfPartitions() == 2
for p in range(2):
    box_part = box_out.GetPartition(p)
    assert box_part.GetNumberOfPoints() == 18
    assert box_part.GetNumberOfCells() == 24
    assert set(box_part.GetCellType(i) for i in range(box_part.GetNumberOfCells())) == {10} # VTK_TETRA
    boundary = box_part.GetPointData().GetArray("field1_boundary")
    assert boundary is not None
    # boundary vertices are marked with value 127, interior ones with 100;
    # 13 of the 18 vertices of each part lie on the mesh boundary
    assert set(boundary.GetValue(i) for i in range(boundary.GetNumberOfTuples())) == {100.0, 127.0}
    assert sum(1 for i in range(boundary.GetNumberOfTuples()) if boundary.GetValue(i) == 127) == 13
    assert box_part.GetPointData().GetArray("vtkGhostType") is not None
    assert box_part.GetCellData().GetArray("vtkGhostType") is not None

# ltx_graded_gauss.osh: format version 11, one part of 4966 vertices and
# 9104 triangles.
ltx = read_mesh("ltx_graded_gauss")
ltx_out = sm.Fetch(ltx)
assert ltx_out.GetNumberOfPartitions() == 1
ltx_part = ltx_out.GetPartition(0)
assert ltx_part.GetNumberOfPoints() == 4966
assert ltx_part.GetNumberOfCells() == 9104
assert set(ltx_part.GetCellType(i) for i in range(ltx_part.GetNumberOfCells())) == {5} # VTK_TRIANGLE
