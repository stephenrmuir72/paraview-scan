#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "catalyst.hpp"
#include "conduit_cpp_to_c.hpp"

namespace CatalystAdaptor
{
  void Initialize(conduit_cpp::Node& node);
  void Execute(conduit_cpp::Node& node);
  void Finalize();
}

#endif
