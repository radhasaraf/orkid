////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

#include <ork/rtti/RTTI.h>
#include <ork/reflect/Serializable.h>
#include <ork/object/ObjectClass.h>
#include <ork/config/config.h>

namespace ork {

namespace event {
class Event;
}

namespace object {
class Signal;
}

namespace reflect { class ISerializer; class IDeserializer; class BidirectionalSerializer; }

//typedef rtti::RTTI<Object, rtti::ICastable, rtti::DefaultPolicy, object::ObjectClass> ObjectBase;
        
struct  Object : public rtti::ICastable
{
private:
    RttiDeclareAbstractWithCategory( Object, rtti::ICastable, object::ObjectClass );

    //RttiDeclareConcrete( Object, ObjectBase );
    
public:
	virtual ~Object() {}

	bool Serialize(reflect::ISerializer &) const;
	bool SerializeInPlace(reflect::ISerializer &serializer) const;
	bool Deserialize(reflect::IDeserializer &);
	bool DeserializeInPlace(reflect::IDeserializer &);

	object::Signal *FindSignal(ConstString name);

	virtual bool PreSerialize(reflect::ISerializer &) const;
	virtual bool PreDeserialize(reflect::IDeserializer &);
	virtual bool PostSerialize(reflect::ISerializer &) const;
	virtual bool PostDeserialize(reflect::IDeserializer &);

	virtual Object* Clone() const;

	bool Notify(const event::Event* pEV) { return DoNotify(pEV); }
	bool Query(event::Event* pEV) const { return DoQuery(pEV); }
	
private:

	virtual bool DoNotify(const event::Event* pEV) { return false; }
	virtual bool DoQuery(event::Event* pEV) const { return false; }

};

reflect::BidirectionalSerializer &operator ||(reflect::BidirectionalSerializer &, Object &);
reflect::BidirectionalSerializer &operator ||(reflect::BidirectionalSerializer &, const Object &);

template<typename T>
inline bool DeserializeUnknownObject(ork::reflect::IDeserializer &deser, T *&value)
{
	ork::rtti::ICastable *obj = NULL;
	bool result = ork::rtti::safe_downcast<ork::rtti::Category*>(ork::Object::GetClassStatic()->GetClass())->DeserializeReference(deser, obj);
	value = ork::rtti::safe_downcast<T *>(obj);
	return result;
}

Object *DeserializeObject(PieceString file);

}

#include <ork/event/EventListener.h>

