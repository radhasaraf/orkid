////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include "ITypedArray.h"

#include <ork/config/config.h>

namespace ork { namespace reflect {

template <typename T> class AccessorTypedArray : public ITypedArray<T> {
public:
  AccessorTypedArray(
      void (Object::*getter)(T&, size_t) const,
      void (Object::*setter)(const T&, size_t),
      size_t (Object::*counter)() const,
      void (Object::*resizer)(size_t) = 0);

private:
  void get(T&, object_constptr_t, size_t) const override;
  void set(const T&, object_ptr_t, size_t) const override;
  size_t count(object_constptr_t) const override;
  void resize(object_ptr_t obj, size_t size) const override;

  void (Object::*_getter)(T&, size_t) const;
  void (Object::*_setter)(const T&, size_t);
  size_t (Object::*_counter)() const;
  void (Object::*_resizer)(size_t);
};

}} // namespace ork::reflect
