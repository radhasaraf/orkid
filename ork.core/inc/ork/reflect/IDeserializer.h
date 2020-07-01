////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include "serialize/serdes.h"
#include <stack>

namespace ork::reflect::serdes {

class ObjectProperty;
class Command;

struct IDeserializer {

  virtual node_ptr_t pushNode(std::string named, NodeType type) {
    return nullptr;
  }
  virtual void popNode() {
    return;
  }

  virtual void deserializeTop(object_ptr_t&) = 0;
  virtual node_ptr_t deserializeObject(node_ptr_t) {
    return node_ptr_t(nullptr);
  }
  virtual node_ptr_t deserializeElement(node_ptr_t elemnode) {
    return node_ptr_t(nullptr);
  }

  void trackObject(boost::uuids::uuid id, object_ptr_t instance);
  object_ptr_t findTrackedObject(boost::uuids::uuid id) const;
  virtual ~IDeserializer();

  ///////////////////////////////////////////

  using trackervect_t = std::unordered_map<std::string, object_ptr_t>;
  std::stack<node_ptr_t> _nodestack;
  trackervect_t _reftracker;
};

} // namespace ork::reflect::serdes
