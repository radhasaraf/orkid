////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

#include <ork/kernel/string/ResizableString.h>
#include <ork/kernel/string/MutableString.h>

#include <ork/config/config.h>

namespace ork { namespace rtti { class ICastable; } }

namespace ork { namespace reflect {

//typedef ork::Object Serializable;
class IProperty;
class IObjectProperty;
class Command;

class  IDeserializer
{
public:
    virtual bool Deserialize(bool   &) = 0;
    virtual bool Deserialize(char   &) = 0;
    virtual bool Deserialize(short  &) = 0;
    virtual bool Deserialize(int    &) = 0;
    virtual bool Deserialize(long   &) = 0;
    virtual bool Deserialize(float  &) = 0;
    virtual bool Deserialize(double &) = 0;
	virtual bool Deserialize(rtti::ICastable *&) = 0;

	virtual bool Deserialize(const IProperty *) = 0;
	virtual bool Deserialize(const IObjectProperty *, Object *) = 0;
    
    virtual bool Deserialize(MutableString &) = 0;
    virtual bool Deserialize(ResizableString &) = 0;
    virtual bool DeserializeData(unsigned char *, size_t) = 0;

    virtual bool ReferenceObject(rtti::ICastable *) = 0;
    virtual bool BeginCommand(Command &) = 0;
    virtual bool EndCommand(const Command &) = 0;
	virtual void Hint(const PieceString &) {}

    virtual ~IDeserializer();
};

} }

