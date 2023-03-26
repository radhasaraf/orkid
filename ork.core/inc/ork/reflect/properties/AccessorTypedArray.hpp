////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include "AccessorTypedArray.h"
#include "ITypedArray.hpp"

namespace ork { namespace reflect {

template <typename T>
AccessorTypedArray<T>::AccessorTypedArray(
    void (Object::*getter)(T&, size_t index) const,
    void (Object::*setter)(const T&, size_t index),
    size_t (Object::*counter)() const,
    void (Object::*resizer)(size_t))
    : _getter(getter)
    , _setter(setter)
    , _counter(counter)
    , _resizer(resizer) {
  OrkAssert(_resizer);
}

template <typename T>
void AccessorTypedArray<T>::get(
    T& value, //
    object_constptr_t instance,
    size_t index) const {
  (instance.get()->*_getter)(value, index);
}

template <typename T>
void AccessorTypedArray<T>::set(
    const T& value, //
    object_ptr_t instance,
    size_t index) const {
  (instance.get()->*_setter)(value, index);
}

template <typename T> //
size_t AccessorTypedArray<T>::count(object_constptr_t instance) const {
  return (instance.get()->*_counter)();
}

template <typename T>
void AccessorTypedArray<T>::resize(
    object_ptr_t instance, //
    size_t size) const {
  (instance.get()->*_resizer)(size);
}

}} // namespace ork::reflect
