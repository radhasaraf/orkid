////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/rtti/Category.h>

namespace ork { namespace rtti {
class RTTIData;
}} // namespace ork::rtti
namespace ork { namespace reflect {
class ISerializer;
class IDeserializer;
}} // namespace ork::reflect

namespace ork { namespace object {

class ObjectCategory : public rtti::Category {
public:
  ObjectCategory(const rtti::RTTIData&);

private:
  using dispatch_fn_t = std::function<Object*(Class* clazz)>;
  bool serializeObject(reflect::ISerializer&, rtti::castable_rawconstptr_t) const override;
  bool deserializeObject(reflect::IDeserializer&, rtti::castable_rawptr_t&) const override;
  bool serializeObject(reflect::ISerializer&, rtti::castable_constptr_t) const override;
  bool deserializeObject(reflect::IDeserializer&, rtti::castable_ptr_t&) const override;
};

}} // namespace ork::object
