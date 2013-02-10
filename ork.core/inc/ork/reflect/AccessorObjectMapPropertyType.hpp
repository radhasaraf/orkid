////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

#include <ork/reflect/AccessorObjectMapPropertyType.h>
#include <ork/reflect/IObjectMapPropertyType.hpp>
#include <ork/reflect/IDeserializer.h>
#include <ork/reflect/ISerializer.h>
#include <ork/reflect/BidirectionalSerializer.h>

namespace ork { namespace reflect {

template<typename KeyType, typename ValueType>
AccessorObjectMapPropertyType<KeyType, ValueType>::AccessorObjectMapPropertyType(
		bool (Object::*getter)(const KeyType &, int, ValueType &) const,
		void (Object::*setter)(const KeyType &, int, const ValueType &),
		void (Object::*eraser)(const KeyType &, int),
		void (Object::*serializer)(
			typename AccessorObjectMapPropertyType<KeyType, ValueType>::ItemSerializeFunction,
			BidirectionalSerializer &) const)
	: mGetter(getter)
	, mSetter(setter)
	, mEraser(eraser)
	, mSerializer(serializer)
{
}


template<typename KeyType, typename ValueType>
bool AccessorObjectMapPropertyType<KeyType, ValueType>::ReadItem(
	const Object *object, const KeyType &key, int multi_index, ValueType &value) const
{
	return (object->*mGetter)(key, multi_index, value);
}

template<typename KeyType, typename ValueType>
bool AccessorObjectMapPropertyType<KeyType, ValueType>::WriteItem(
	Object *object, const KeyType &key, int multi_index, const ValueType *value) const
{
	if(value)
	{
		(object->*mSetter)(key, multi_index, *value);
	}
	else
	{
		(object->*mEraser)(key, multi_index);
	}

	return true;
}

template<typename KeyType, typename ValueType>
bool AccessorObjectMapPropertyType<KeyType, ValueType>::MapSerialization(
	typename AccessorObjectMapPropertyType<KeyType, ValueType>::ItemSerializeFunction serialize_function,
	BidirectionalSerializer &bidi,
	const Object *object) const
{
	(object->*mSerializer)(serialize_function, bidi);

	return bidi.Succeeded();
}

} }

