////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/reflect/ISerializer.h>
#include <ork/reflect/Command.h>

#include <ork/reflect/properties/ObjectProperty.h>
#include <ork/stream/IOutputStream.h>
#include <ork/rtti/Category.h>
#include <ork/rtti/downcast.h>
#include <ork/object/Object.h>
#include <boost/uuid/uuid_io.hpp>
////////////////////////////////////////////////////////////////
namespace ork::reflect::serdes {
ISerializer::~ISerializer() {
}
////////////////////////////////////////////////////////////////////////////////
node_ptr_t ISerializer::serializeRoot(object_constptr_t instance) {
  _rootnode                = pushNode("root", NodeType::OBJECT);
  _rootnode->_ser_instance = instance;
  auto objnode             = serializeObject(_rootnode);
  return _rootnode;
}
////////////////////////////////////////////////////////////////////////////////
node_ptr_t ISerializer::topNode() {
  node_ptr_t n;
  if (not _nodestack.empty()) {
    n = _nodestack.top();
  }
  return n;
}
////////////////////////////////////////////////////////////////
} // namespace ork::reflect::serdes
