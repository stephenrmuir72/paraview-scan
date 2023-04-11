#include "CatalystAdaptor.h"
#include <iostream>
#include <catalyst.hpp>
#include "conduit_cpp_to_c.hpp"
//#include <string>

namespace CatalystAdaptor
{
void Initialize(conduit_cpp::Node& node)
{
  conduit_node* c_node = nullptr;
  c_node = conduit_cpp::c_node(&node);
  catalyst_status err = catalyst_initialize(c_node);
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to initialize Catalyst: " << err << std::endl;
  }
}

void Execute(conduit_cpp::Node& node)
{
  // catalyst_status err = catalyst_execute(conduit::c_node(&node));
  // if (err != catalyst_status_ok)
  // {
  //   std::cerr << "Failed to execute Catalyst: " << err << std::endl;
  // }
}

void Finalize()
{
  // conduit::Node node;
  // catalyst_status err = catalyst_finalize(conduit::c_node(&node));
  // if (err != catalyst_status_ok)
  // {
  //   std::cerr << "Failed to finalize Catalyst: " << err << std::endl;
  // }
}
}
