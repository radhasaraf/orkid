////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

#include <ork/reflect/IObjectProperty.h>

#include <ork/config/config.h>

namespace ork { namespace reflect {

class  IObjectMapProperty : public IObjectProperty
{
	DECLARE_TRANSPARENT_CASTABLE(IObjectMapProperty, IObjectProperty)

public:

	virtual int GetSize(const Object* obj) const = 0;

	static const int kDeserializeInsertItem = -1;

    virtual bool DeserializeItem(IDeserializer *value, IDeserializer &key, int, Object *) const = 0;
    virtual bool SerializeItem(ISerializer &value, IDeserializer &key, int, const Object *) const = 0;

	virtual bool IsMultiMap(const Object* obj) const = 0;

private:
    /*virtual*/ bool Deserialize(IDeserializer &serializer, Object *obj) const = 0;
    /*virtual*/ bool Serialize(ISerializer &serializer, const Object *obj) const = 0;
protected:

	IObjectMapProperty() {}
};

} }
