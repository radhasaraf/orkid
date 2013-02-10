////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

#include <ork/reflect/IObjectArrayPropertyType.h>

#include <ork/config/config.h>

namespace ork { namespace reflect {

template<typename VectorType>
class  DirectObjectVectorPropertyType 
	: public IObjectArrayPropertyType<typename VectorType::value_type>
{
public:
	typedef typename VectorType::value_type ValueType;
	
	DirectObjectVectorPropertyType(VectorType Object::*);
private:
    /*virtual*/ void Get(ValueType &, const Object *, size_t) const;
    /*virtual*/ void Set(const ValueType &, Object *, size_t) const;
	/*virtual*/ size_t Count(const Object *) const;
	/*virtual*/ bool Resize(Object *, size_t) const;

	VectorType Object::*mProperty;
};

} }
