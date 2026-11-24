"""Catalyst V2 run in which only some ranks have data for a channel.

Run with pvbatch on several ranks. Rank 0 has data for the mesh channel 'grid'
and the amrmesh channel 'amr'; the other ranks have data for the two meshes of
the multimesh channel 'blocks'. Every rank passes every channel, with an empty
node where it has no data. With --missing, ranks without data leave the
channel out instead, which must fail on every rank. With --fides, only a
'fides_conduit' channel 'fides' is passed: rank 0 has one block of it and the
other ranks pass the same schema with no blocks.
"""

import argparse

import catalyst
import catalyst_conduit as conduit
import numpy as np
from vtkmodules.vtkParallelCore import vtkMultiProcessController

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--script", required=True)
parser.add_argument("--missing", action="store_true")
parser.add_argument("--fides", action="store_true")
args = parser.parse_args()

controller = vtkMultiProcessController.GetGlobalController()
rank = controller.GetLocalProcessId()

init = conduit.Node()
init["catalyst/scripts/script/filename"] = args.script
init["catalyst_load/implementation"] = "paraview"
catalyst.initialize(init)


def add_cube(mesh):
    mesh["coordsets/coords/type"] = "uniform"
    for axis, dim in (("i", "x"), ("j", "y"), ("k", "z")):
        mesh[f"coordsets/coords/dims/{axis}"] = 3
        mesh[f"coordsets/coords/origin/{dim}"] = 0.0
        mesh[f"coordsets/coords/spacing/d{dim}"] = 0.5
    mesh["topologies/mesh/type"] = "uniform"
    mesh["topologies/mesh/coordset"] = "coords"


def add_triangles(mesh, x0):
    x = np.array([x0, x0 + 1.0, x0 + 1.0, x0])
    y = np.array([0.0, 0.0, 1.0, 1.0])
    mesh["coordsets/coords/type"] = "explicit"
    mesh["coordsets/coords/values/x"] = x
    mesh["coordsets/coords/values/y"] = y
    mesh["coordsets/coords/values/z"] = np.zeros_like(x)
    mesh["topologies/mesh/type"] = "unstructured"
    mesh["topologies/mesh/coordset"] = "coords"
    mesh["topologies/mesh/elements/shape"] = "tri"
    mesh["topologies/mesh/elements/connectivity"] = np.array([0, 1, 2, 0, 2, 3], dtype=np.int64)


FIDES_SCHEMA = """{
  "unstructured_grid": {
    "data_sources": [ { "name": "source", "type": "conduit" } ],
    "coordinate_system": {
      "array": { "array_type": "basic", "data_source": "source", "variable": "points" }
    },
    "cell_set": {
      "cell_set_type": "single_type",
      "cell_type": "hexahedron",
      "data_source": "source",
      "variable": "connectivity"
    }
  }
}"""


def add_hexahedra(data):
    """Two hexahedra as one block, or no blocks on ranks other than 0."""
    points = np.array(
        [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0], [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1],
         [2, 0, 0], [2, 1, 0], [2, 0, 1], [2, 1, 1]], dtype=np.float64)
    connectivity = np.array(
        [0, 1, 2, 3, 4, 5, 6, 7, 1, 8, 9, 2, 5, 10, 11, 6], dtype=np.int64)
    for name, values, shape in (("points", points.reshape(-1), [12, 3]),
                                ("connectivity", connectivity, [16])):
        blocks = data[f"variables/{name}/blocks"]
        if rank == 0:
            blocks["b0/shape"] = np.array(shape, dtype=np.int64)
            blocks["b0/data"] = values
        else:
            blocks.set(conduit.Node())


def add_mesh_channels(channels):
    if rank == 0:
        channels["grid/type"] = "mesh"
        add_cube(channels["grid/data"])
        channels["amr/type"] = "amrmesh"
        domain = channels["amr/data/domain0"]
        domain["state/domain_id"] = 0
        domain["state/level"] = 0
        add_cube(domain)
    elif not args.missing:
        channels["grid/type"] = "mesh"
        channels["grid/data"].set(conduit.Node())
        channels["amr/type"] = "amrmesh"
        channels["amr/data"].set(conduit.Node())
    if rank != 0:
        channels["blocks/type"] = "multimesh"
        add_triangles(channels["blocks/data/a"], float(rank))
        add_triangles(channels["blocks/data/b"], float(rank) + 10.0)
        channels["blocks/assembly/group"] = "a"
    elif not args.missing:
        channels["blocks/type"] = "multimesh"
        channels["blocks/data/a"].set(conduit.Node())
        channels["blocks/data/b"].set(conduit.Node())
        channels["blocks/assembly/group"] = "a"


for step in range(3):
    node = conduit.Node()
    node["catalyst/state/timestep"] = step
    node["catalyst/state/time"] = float(step)
    channels = node["catalyst/channels"]
    if args.fides:
        channels["fides/type"] = "fides_conduit"
        channels["fides/schema"] = FIDES_SCHEMA
        add_hexahedra(channels["fides/data"])
    else:
        add_mesh_channels(channels)
    catalyst.execute(node)

catalyst.finalize(conduit.Node())
