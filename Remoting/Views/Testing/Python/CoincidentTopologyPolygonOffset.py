# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
"""
Verifies that the "Coincident Topology Polygon Offset" representation
property (vtkGeometryRepresentation::SetUserRelativeCoincidentTopologyPolygonOffsetParameters)
reaches the surface mapper, survives repeated renders, and combines
additively with the offset vtkPVCompositeRepresentation applies
automatically to keep the surface behind the selection overlay.

This is a regression test for a bug where vtkPVCompositeRepresentation's
per-render selection-overlay update clobbered any user-set offset, since both
were routed through the same setter.
"""

from paraview.simple import *
from vtkmodules.vtkCommonCore import reference

cone = Cone()
rep = Show(cone)
Render()


def GetMapperOffsetParameters():
    geomRepr = rep.SMProxy.GetClientSideObject().GetActiveRepresentation()
    mapper = geomRepr.GetActor().GetMapper()
    factor = reference(0.0)
    units = reference(0.0)
    mapper.GetCoincidentTopologyPolygonOffsetParameters(factor, units)
    return (float(factor), float(units))


# No user offset, no selection -> mapper should see (0, 0).
assert GetMapperOffsetParameters() == (0.0, 0.0), "expected no offset by default"

# Setting the exposed property should reach the mapper directly.
rep.CoincidentTopologyPolygonOffset = [2.0, 3.0]
rep.UpdateVTKObjects()
Render()
assert GetMapperOffsetParameters() == (2.0, 3.0), "user-set offset did not reach the mapper"

# Repeated renders must not reset the user-set value -- this was the bug:
# vtkPVCompositeRepresentation re-applied its own selection-driven offset on
# every render, clobbering whatever the user had set.
for _ in range(5):
    Render()
assert GetMapperOffsetParameters() == (2.0, 3.0), "user-set offset was reset by a subsequent render"

# Selecting cells activates the automatic selection-overlay push-back
# (vtkPVCompositeRepresentation::UpdateCoincidentOffsetForSelectionOverlay),
# which must combine additively with the user-set offset rather than
# replacing it.
QuerySelect(QueryString="id >= 0", FieldType="CELL", Source=cone)
Render()
assert GetMapperOffsetParameters() == (3.0, 3.0), \
    "selection-driven offset did not combine additively with the user offset"

# Clearing the selection should drop the selection contribution but keep the
# user-set offset.
ClearSelection(proxy=cone)
Render()
assert GetMapperOffsetParameters() == (2.0, 3.0), "user offset was not restored after clearing the selection"

# Resetting the user-set offset back to zero should fully clear it, since
# there is no active selection contribution.
rep.CoincidentTopologyPolygonOffset = [0.0, 0.0]
rep.UpdateVTKObjects()
Render()
assert GetMapperOffsetParameters() == (0.0, 0.0), "resetting the user offset to 0, 0 did not clear it"
