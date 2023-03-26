////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include "DirectTypedVector.h"
#include "ITypedArray.hpp"

namespace ork { namespace reflect {

template <typename VectorType> //
DirectTypedVector<VectorType>::DirectTypedVector(VectorType Object::*prop)
    : _member(prop) {
  OrkAssert(_member != nullptr);
}

template <typename VectorType> //
void DirectTypedVector<VectorType>::get(
    value_type& value, //
    object_constptr_t instance,
    size_t index) const {
  value = (instance.get()->*_member)[index];
}

template <typename VectorType> //
void DirectTypedVector<VectorType>::set(
    const value_type& value, //
    object_ptr_t instance,
    size_t index) const {
  auto& vect  = (instance.get()->*_member);
  vect[index] = value;
}

template <typename VectorType> //
size_t DirectTypedVector<VectorType>::count(object_constptr_t instance) const {
  auto& vect = (instance.get()->*_member);
  return size_t(vect.size());
}

template <typename VectorType> //
void DirectTypedVector<VectorType>::resize(
    object_ptr_t instance, //
    size_t size) const {
  auto& vect = (instance.get()->*_member);
  vect.resize(size);
}

}} // namespace ork::reflect
