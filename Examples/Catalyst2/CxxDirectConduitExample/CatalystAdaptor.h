#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "conduit.hpp"

namespace CatalystAdaptor
{
  void Initialize(conduit::Node& node);
  void Execute(conduit::Node& node);
  void Finalize();
}

#endif
