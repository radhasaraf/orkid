////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

#include <ork/reflect/IObjectPropertyType.h>

#include <ork/config/config.h>

namespace ork { namespace reflect {

template<typename T>
class  AccessorObjectPropertyType : public IObjectPropertyType<T>
{
public:
    AccessorObjectPropertyType(void (Object::*getter)(T &) const, void (Object::*setter)(const T &));

    /*virtual*/ void Get(T &value, const Object *obj) const;
    /*virtual*/ void Set(const T &value, Object *obj) const;
private:
    void (Object::*mGetter)(T &) const;
    void (Object::*mSetter)(const T &);
};

} }
