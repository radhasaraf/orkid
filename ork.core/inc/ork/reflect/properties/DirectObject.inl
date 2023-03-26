////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/pch.h>

#include <ork/object/Object.h>
#include <ork/reflect/properties/DirectObject.h>
#include <ork/reflect/Command.h>
#include <ork/reflect/ISerializer.h>
#include <ork/reflect/IDeserializer.h>

namespace ork::reflect {


template <typename MemberType> //
inline DirectObject<MemberType>::DirectObject(sharedptrtype_t Object::*member)
    : _member(member) {
}

template <typename MemberType> //
inline void DirectObject<MemberType>::serialize(serdes::node_ptr_t propnode) const {
  auto serializer     = propnode->_serializer;
  auto parinstance    = propnode->_ser_instance;
  auto child_instance = (parinstance.get()->*_member);
  if (child_instance) {
    auto childnode           = serializer->pushNode(_name, serdes::NodeType::OBJECT);
    childnode->_ser_instance = child_instance;
    childnode->_parent       = propnode;
    serializer->serializeObject(childnode);
    serializer->popNode();
  } else {
    propnode->_value.template set<void*>(nullptr);
    serializer->serializeLeaf(propnode);
  }
}

template <typename MemberType> //
inline void DirectObject<MemberType>::deserialize(serdes::node_ptr_t dsernode) const {
  auto instance     = dsernode->_deser_instance;
  auto deserializer = dsernode->_deserializer;
  auto childnode    = deserializer->deserializeObject(dsernode);
  if (childnode) {
    auto subinstance           = childnode->_deser_instance;
    (instance.get()->*_member) = std::dynamic_pointer_cast<element_type>(subinstance);
  } else {
    (instance.get()->*_member) = nullptr;
  }
}

template <typename MemberType>                            //
inline typename DirectObject<MemberType>::sharedptrtype_t //
DirectObject<MemberType>::access(object_ptr_t instance) const {
  return (instance.get()->*_member);
}

template <typename MemberType>                                 //
inline typename DirectObject<MemberType>::sharedconstptrtype_t //
DirectObject<MemberType>::access(object_constptr_t instance) const {
  return (const_cast<Object*>(instance.get())->*_member);
}

template <typename MemberType> //
object_ptr_t DirectObject<MemberType>::getObject(object_constptr_t instance) const { // final
  auto value = (instance.get()->*_member);
  return value;
}

template <typename MemberType> //
void DirectObject<MemberType>::setObject(object_ptr_t instance, object_ptr_t obj) const { // final
  auto casted = dynamic_pointer_cast<element_type>(obj);
  (instance.get()->*_member) = casted;
}

template <typename MemberType> //
inline void DirectObject<MemberType>::get(
    sharedptrtype_t& value, //
    object_constptr_t instance) const {
  value = (instance.get()->*_member);
}
template <typename MemberType> //
inline void DirectObject<MemberType>::set(
    const sharedptrtype_t& value, //
    object_ptr_t instance) const {
  (instance.get()->*_member) = value;
}

} // namespace ork::reflect
