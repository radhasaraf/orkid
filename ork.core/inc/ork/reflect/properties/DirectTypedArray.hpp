////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////
#include "DirectTypedArray.h"
#include "ITypedArray.hpp"
///////////////////////////////////////////////////////////////////////////
namespace ork::reflect {
///////////////////////////////////////////////////////////////////////////
template <typename ArrayType>
DirectTypedArray<ArrayType>::DirectTypedArray(ArrayType(Object::*prop))
    : _member(prop)
    , _size(array_length) {
}
///////////////////////////////////////////////////////////////////////////
template <typename ArrayType> //
void DirectTypedArray<ArrayType>::get(
    element_type& value, //
    object_constptr_t obj,
    size_t index) const {
  auto nonconst = std::const_pointer_cast<Object>(obj);
  auto& array   = (nonconst.get()->*_member);
  value         = array[index];
}
///////////////////////////////////////////////////////////////////////////
template <typename ArrayType> //
void DirectTypedArray<ArrayType>::set(
    const element_type& value, //
    object_ptr_t obj,
    size_t index) const {
  auto& array  = (obj.get()->*_member);
  array[index] = value;
}
///////////////////////////////////////////////////////////////////////////
template <typename ArrayType> //
size_t DirectTypedArray<ArrayType>::count(object_constptr_t) const {
  return _size;
}
///////////////////////////////////////////////////////////////////////////
template <typename ArrayType> //
void DirectTypedArray<ArrayType>::resize(
    object_ptr_t obj, //
    size_t newsize) const {
  OrkAssert(newsize == array_length);
}
///////////////////////////////////////////////////////////////////////////
} // namespace ork::reflect
