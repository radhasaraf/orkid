////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/reflect/serialize/JsonSerializer.h>
#include <ork/reflect/enum_serializer.inl>
#include <ork/reflect/Command.h>

#include <ork/reflect/properties/ObjectProperty.h>
#include <ork/stream/IOutputStream.h>
#include <ork/rtti/Category.h>
#include <ork/rtti/downcast.h>
#include <ork/kernel/string/string.h>
#include <ork/kernel/string/PoolString.h>
#include <ork/file/path.h>
#include <ork/object/Object.h>
#include <boost/uuid/uuid_io.hpp>
#include <cstring>
#include <boost/filesystem.hpp>
#include <ork/math/cvector4.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#pragma GCC diagnostic pop

namespace ork::reflect::serdes {
struct JsonSerObjectNode {
  JsonSerObjectNode() {
    _jsonvalue.SetObject();
  }
  rapidjson::Value _jsonvalue;
};
////////////////////////////////////////////////////////////////////////////////
JsonSerializer::JsonSerializer() {
  _allocator = std::make_shared<allocator_t>();
  _document = std::make_shared<rapidjson::Document>( rapidjson::kObjectType, _allocator.get() );
}
////////////////////////////////////////////////////////////////////////////////
JsonSerializer::~JsonSerializer() {
  _document = nullptr;
  _allocator = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
node_ptr_t JsonSerializer::_createNode(std::string named, NodeType type) {
  node_ptr_t n   = std::make_shared<Node>(); //)named, rapidjson::kObjectType);
  auto impl      = n->_impl.makeShared<JsonSerObjectNode>();
  n->_name       = named;
  n->_type       = type;
  n->_serializer = this;
  if (type == NodeType::ARRAY) {
    impl->_jsonvalue.SetArray();
  }
  return n;
}
////////////////////////////////////////////////////////////////////////////////
node_ptr_t JsonSerializer::pushNode(std::string named, NodeType type) {
  node_ptr_t n = _createNode(named, type);
  _nodestack.push(n);
  return n;
}
////////////////////////////////////////////////////////////////////////////////
void JsonSerializer::popNode() {
  auto n = topNode();
  _nodestack.pop();

  rapidjson::Value key(n->_name.c_str(), *_allocator);
  auto impl = n->_impl.getShared<JsonSerObjectNode>();

  switch (n->_type) {
    case NodeType::ARRAY:
    case NodeType::OBJECT:
    case NodeType::PROPERTIES:
    case NodeType::MAP: {
      if (_nodestack.empty()) {
        _document->AddMember(
            key, //
            impl->_jsonvalue,
            *_allocator);
      } else {
        auto parent  = topNode();
        auto topimpl = parent->_impl.getShared<JsonSerObjectNode>();
        topimpl->_jsonvalue.AddMember(
            key, //
            impl->_jsonvalue,
            *_allocator);
      }
      break;
    }
    case NodeType::ARRAY_ELEMENT_OBJECT: {
      auto parent  = topNode();
      auto topimpl = parent->_impl.getShared<JsonSerObjectNode>();
      topimpl->_jsonvalue.PushBack(
          impl->_jsonvalue, //
          *_allocator);
      break;
    }
    case NodeType::MAP_ELEMENT_OBJECT: {
      auto parent  = topNode();
      auto topimpl = parent->_impl.getShared<JsonSerObjectNode>();
      topimpl->_jsonvalue.AddMember(
          key, //
          impl->_jsonvalue,
          *_allocator);
      break;
    }
    case NodeType::MAP_ELEMENT_LEAF:
    case NodeType::ARRAY_ELEMENT_LEAF:
    case NodeType::LEAF:
      break;
    default:
      OrkAssert(false);
      break;
  }
}
////////////////////////////////////////////////////////////////////////////////
std::string JsonSerializer::output() {

  popNode(); // pop objects

  rapidjson::StringBuffer strbuf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strbuf);
  writer.SetIndent(' ', 1);
  _document->Accept(writer);

  return strbuf.GetString();
}
////////////////////////////////////////////////////////////////////////////////
node_ptr_t JsonSerializer::serializeContainerElement(node_ptr_t elemnode) {

  OrkAssert(elemnode->_ser_instance);

  if (auto as_obj = elemnode->_value.tryAs<object_ptr_t>()) {
    elemnode->_ser_instance = as_obj.value();
    switch (elemnode->_type) {
      case NodeType::ARRAY_ELEMENT_LEAF:
        elemnode->_type = NodeType::ARRAY_ELEMENT_OBJECT;
        break;
      case NodeType::MAP_ELEMENT_LEAF:
        elemnode->_type = NodeType::MAP_ELEMENT_OBJECT;
        break;
      default:
        OrkAssert(false);
        break;
    }
    auto objnode = serializeObject(elemnode);
      if(objnode){
          OrkAssert(objnode);
          objnode->_parent = elemnode;
      }
    // popNode(); // pop objnode
  } else {
    elemnode->_name = elemnode->_key;
    /////////////////////////////////
    // put leafnodes under mapnode
    /////////////////////////////////
    auto mapnode    = elemnode->_parent;
    elemnode->_impl = mapnode->_impl;
    /////////////////////////////////
    serializeLeaf(elemnode);
  }

  return elemnode;
}
////////////////////////////////////////////////////////////////////////////////
void JsonSerializer::serializeLeaf(node_ptr_t leafnode) {
  auto parimplnode = leafnode->_impl.getShared<JsonSerObjectNode>();
  rapidjson::Value nameval(leafnode->_name.c_str(), *_allocator);

  using addfn_t = std::function<void(rapidjson::Value & value)>;

  addfn_t addfn = [&](rapidjson::Value& value) {
    parimplnode->_jsonvalue.AddMember(
        nameval, //
        value,
        *_allocator);
  };
  if (leafnode->_type == NodeType::ARRAY_ELEMENT_LEAF) {
    addfn = [&](rapidjson::Value& value) { //
      parimplnode->_jsonvalue.PushBack(value, *_allocator);
    };
  }

  if (auto as_bool = leafnode->_value.tryAs<bool>()) {
    rapidjson::Value boolval;
    boolval.SetBool(as_bool.value());
    addfn(boolval);
  } else if (auto as_int = leafnode->_value.tryAs<int>()) {
    rapidjson::Value intval;
    intval.SetInt(as_int.value());
    addfn(intval);
  } else if (auto as_uint = leafnode->_value.tryAs<unsigned int>()) {
    rapidjson::Value uintval(uint32_t(as_uint.value()));
    addfn(uintval);
  } else if (auto as_ulong = leafnode->_value.tryAs<unsigned long>()) {
    rapidjson::Value ulongval(uint64_t(as_ulong.value()));
    addfn(ulongval);
  } else if (auto as_float = leafnode->_value.tryAs<float>()) {
    rapidjson::Value floatval;
    floatval.SetFloat(as_float.value());
    addfn(floatval);
  } else if (auto as_double = leafnode->_value.tryAs<double>()) {
    rapidjson::Value doubleval;
    doubleval.SetDouble(as_double.value());
    addfn(doubleval);
  } else if (auto as_fvec2 = leafnode->_value.tryAs<fvec2>()) {
    rapidjson::Value fv2val;  
    fv2val.SetArray();
    fv2val.PushBack(as_fvec2.value().x, *_allocator);
    fv2val.PushBack(as_fvec2.value().y, *_allocator);
    addfn(fv2val);
  } else if (auto as_fvec3 = leafnode->_value.tryAs<fvec3>()) {
    rapidjson::Value fv3val;  
    fv3val.SetArray();
    fv3val.PushBack(as_fvec3.value().x, *_allocator);
    fv3val.PushBack(as_fvec3.value().y, *_allocator);
    fv3val.PushBack(as_fvec3.value().z, *_allocator);
    addfn(fv3val);
  } else if (auto as_fvec4 = leafnode->_value.tryAs<fvec4>()) {
    rapidjson::Value fv4val;  
    fv4val.SetArray();
    fv4val.PushBack(as_fvec4.value().x, *_allocator);
    fv4val.PushBack(as_fvec4.value().y, *_allocator);
    fv4val.PushBack(as_fvec4.value().z, *_allocator);
    fv4val.PushBack(as_fvec4.value().w, *_allocator);
    addfn(fv4val);
  } else if (auto as_enum = leafnode->_value.tryAs<enumvalue_ptr_t>()) {
    rapidjson::Value strval(as_enum.value()->_name.c_str(), *_allocator);
    addfn(strval);
  } else if (auto as_str = leafnode->_value.tryAs<std::string>()) {
    rapidjson::Value strval(as_str.value().c_str(), *_allocator);
    addfn(strval);
  } else if (auto as_str = leafnode->_value.tryAs<PoolString>()) {
    rapidjson::Value strval(as_str.value().c_str(), *_allocator);
    addfn(strval);
  } else if (auto as_str = leafnode->_value.tryAs<file::Path>()) {
    rapidjson::Value strval(as_str.value().c_str(), *_allocator);
    addfn(strval);
  } else if (auto as_nil = leafnode->_value.tryAs<void*>()) {
    //////////////////////////////////////////////////////////////////
    // if we get here we have an object property, but set to nullptr
    //  otherwise we would have went into serializeObject
    //////////////////////////////////////////////////////////////////
    // auto parimplnode = leafnode->_impl.getShared<JsonSerObjectNode>();
    OrkAssert(as_nil.value() == nullptr);
    rapidjson::Value nilval("nil", *_allocator);
    addfn(nilval);
  } else {
    printf( "leafnode->_value<%s>\n", leafnode->_value.typeName() );
    // did you mean to use directObjectXXXProperty
    //    instead of directXXXProperty ?
    // todo: get it to fail at compile time
    OrkAssert(false);
  }
}
////////////////////////////////////////////////////////////////////////////////
node_ptr_t JsonSerializer::serializeObject(node_ptr_t parnode) {

  node_ptr_t onode;
  auto instance = parnode->_ser_instance;
  if (instance) {
    auto parimplnode  = parnode->_impl.getShared<JsonSerObjectNode>();
    const auto& uuid  = instance->_uuid;
    std::string uuids = boost::uuids::to_string(uuid);
    auto it           = _reftracker.find(uuids);
    rapidjson::Value uuidval(uuids.c_str(), *_allocator);
    ////////////////////////////////////
    // firstreference
    ////////////////////////////////////
    if (it == _reftracker.end()) {
      _reftracker.insert(uuids);

      auto objclazz = instance->GetClass();

      onode                = pushNode("object", NodeType::OBJECT);
      onode->_parent       = parnode;
      onode->_ser_instance = instance;
      auto oimplnode       = onode->_impl.getShared<JsonSerObjectNode>();
      auto classname       = objclazz->Name();
      OrkAssert(classname.c_str() != nullptr); // did you 'touch' the class ?
      rapidjson::Value classval(classname.c_str(), *_allocator);
      oimplnode->_jsonvalue.AddMember(
          "class", //
          classval,
          *_allocator);

      oimplnode->_jsonvalue.AddMember(
          "uuid", //
          uuidval,
          *_allocator);

      auto propsnode           = pushNode("properties", NodeType::PROPERTIES);
      propsnode->_ser_instance = instance;
      propsnode->_parent       = onode;

      bool done = false;

      while (objclazz) {
        auto& desc = objclazz->Description();
        for (auto prop_item : desc.properties()) {
          auto propname        = prop_item.first;
          auto property        = prop_item.second;
          propsnode->_property = property;
          propsnode->_name     = propname.c_str();
          property->serialize(propsnode);
        }
        objclazz = dynamic_cast<object::ObjectClass*>(objclazz->Parent());
      }

      propsnode->_name = "properties";

      popNode(); // pop "properties"
      popNode(); // pop "object"
    }
    ////////////////////////////////////
    // backreference
    ////////////////////////////////////
    else {
      onode                = pushNode("object-ref", NodeType::OBJECT);
      onode->_parent       = parnode;
      onode->_ser_instance = instance;
      auto oimplnode       = onode->_impl.getShared<JsonSerObjectNode>();
      oimplnode->_jsonvalue.AddMember(
          "uuid-ref", //
          uuidval,
          *_allocator);
      popNode(); // pop "object"
    }

  } else {
    auto parimplnode = parnode->_impl.getShared<JsonSerObjectNode>();
    parimplnode->_jsonvalue.AddMember(
        "object", //
        "nil",
        *_allocator);
  }
  return onode;
}
////////////////////////////////////////////////////////////////////////////////
} // namespace ork::reflect::serdes
