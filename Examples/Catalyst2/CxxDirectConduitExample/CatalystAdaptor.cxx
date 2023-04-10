#include "CatalystAdaptor.h"
#include <iostream>
#include <catalyst.hpp>
#include "conduit_cpp_to_c.hpp"
//#include <string>

namespace CatalystAdaptor
{
void Initialize(conduit::Node& node)
{
  catalyst_status err = catalyst_initialize(conduit::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to initialize Catalyst: " << err << std::endl;
  }
}

void Execute(conduit::Node& node)
{
  catalyst_status err = catalyst_execute(conduit::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to execute Catalyst: " << err << std::endl;
  }
}

void Finalize()
{
  conduit::Node node;
  catalyst_status err = catalyst_finalize(conduit::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to finalize Catalyst: " << err << std::endl;
  }
}
}
