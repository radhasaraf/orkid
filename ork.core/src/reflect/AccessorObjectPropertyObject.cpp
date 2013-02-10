////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>

#include <ork/object/Object.h>
#include <ork/reflect/AccessorObjectPropertyObject.h>
#include <ork/reflect/Command.h>
#include <ork/reflect/ISerializer.h>
#include <ork/reflect/IDeserializer.h>

namespace ork { namespace reflect {

AccessorObjectPropertyObject::AccessorObjectPropertyObject(Object *(Object::*property)())
    : mObjectAccessor(property)
{}

bool AccessorObjectPropertyObject::Serialize(ISerializer &serializer, const Object *object) const
{
    const Object *object_property = (const_cast<Object *>(object)->*mObjectAccessor)();
    object_property->Serialize(serializer);
    return true;
}

bool AccessorObjectPropertyObject::Deserialize(IDeserializer &serializer, Object *object) const
{
    Object *object_property = (object->*mObjectAccessor)();
	Command command;
	serializer.BeginCommand(command);

	OrkAssertI(command.Type() == Command::EOBJECT,
		"AccessorObjectPropertyObject::Deserialize::Expected an Object command!\n");

	if(command.Type() == Command::EOBJECT)
	{
		object_property->Deserialize(serializer);
	}

	serializer.EndCommand(command);

    return true;
}

Object *AccessorObjectPropertyObject::Access(Object *object) const
{
	return (object->*mObjectAccessor)();
}

const Object *AccessorObjectPropertyObject::Access(const Object *object) const
{
	return (const_cast<Object *>(object)->*mObjectAccessor)();
}


} }
