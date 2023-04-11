#ifndef ConduitAdaptor_h
#define ConduitAdaptor_h

/*
  In this example we create pure Conduit nodes in our in situ adaptor
  and then use the Catalyst V2 API to pass the Conduit information
  into Catalyst. We initially only include Conduit's header file
  to ensure that we are calling Conduit methods directly and then
  going to Catalyst later on. The idea is that by creating a Conduit
  representation of our simulation data we could pass this Conduit
  information to multiple in situ tools.
*/
#include "catalyst.hpp"
#include "conduit_cpp_to_c.hpp"

#include "CatalystAdaptor.h"
//#include <iostream>
#include <string>

namespace ConduitAdaptor
{
void Initialize(int argc, char* argv[])
 {
  conduit_cpp::Node node;
  //node.ACB(); // ACB debugging -- not in real conduit API so this must be removed!
  for (int cc = 1; cc < argc; ++cc)
  {
    node["catalyst/scripts/script" + std::to_string(cc - 1)].set_string(argv[cc]);
  }
  node["catalyst_load/implementation"] = "paraview";
  node["catalyst_load/search_paths/paraview"] = PARAVIEW_IMPL_DIR;

  CatalystAdaptor::Initialize(node);
}

void Execute(int cycle, double time, Grid& grid, Attributes& attribs)
{
  conduit_cpp::Node exec_params;

  // add time/cycle information
  auto state = exec_params["catalyst/state"];
  state["timestep"].set(cycle);
  state["time"].set(time);
  state["multiblock"].set(1);

  // Add channels.
  // We only have 1 channel here. Let's name it 'grid'.
  auto channel = exec_params["catalyst/channels/grid"];

  // Since this example is using Conduit Mesh Blueprint to define the mesh,
  // we set the channel's type to "mesh".
  channel["type"].set("mesh");

  // now create the mesh.
  auto mesh = channel["data"];

  // start with coordsets (of course, the sequence is not important, just make
  // it easier to think in this order).
  mesh["coordsets/coords/type"].set("explicit");

  mesh["coordsets/coords/values/x"].set_external(
    grid.GetPointsArray(), grid.GetNumberOfPoints(), /*offset=*/0, /*stride=*/3 * sizeof(double));
  mesh["coordsets/coords/values/y"].set_external(grid.GetPointsArray(), grid.GetNumberOfPoints(),
    /*offset=*/sizeof(double), /*stride=*/3 * sizeof(double));
  mesh["coordsets/coords/values/z"].set_external(grid.GetPointsArray(), grid.GetNumberOfPoints(),
    /*offset=*/2 * sizeof(double), /*stride=*/3 * sizeof(double));

  // Next, add topology
  mesh["topologies/mesh/type"].set("unstructured");
  mesh["topologies/mesh/coordset"].set("coords");
  mesh["topologies/mesh/elements/shape"].set("hex");
  mesh["topologies/mesh/elements/connectivity"].set_external(
    grid.GetCellPoints(0), grid.GetNumberOfCells() * 8);

  // Finally, add fields.
  auto fields = mesh["fields"];
  fields["velocity/association"].set("vertex");
  fields["velocity/topology"].set("mesh");
  fields["velocity/volume_dependent"].set("false");

  // velocity is stored in non-interlaced form (unlike points).
  fields["velocity/values/x"].set_external(
    attribs.GetVelocityArray(), grid.GetNumberOfPoints(), /*offset=*/0);
  fields["velocity/values/y"].set_external(attribs.GetVelocityArray(), grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(double));
  fields["velocity/values/z"].set_external(attribs.GetVelocityArray(), grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(double) * 2);

  // pressure is cell-data.
  fields["pressure/association"].set("element");
  fields["pressure/topology"].set("mesh");
  fields["pressure/volume_dependent"].set("false");
  fields["pressure/values"].set_external(attribs.GetPressureArray(), grid.GetNumberOfCells());
  CatalystAdaptor::Execute(exec_params);
}

void Finalize()
{
  CatalystAdaptor::Finalize();
}
} // end ConduitAdaptor namespace

#endif
