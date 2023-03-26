////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>
#include <ork/reflect/properties/AccessorVariantMap.h>
#include <ork/reflect/BidirectionalSerializer.h>

namespace ork { namespace reflect {


#if 0 // don't allow instatiations yet, this class isn't finished!!!
AccessorVariantMap::AccessorVariantMap(
	bool (Object::*read)(IDeserializer &, int, ISerializer &),
	bool (Object::*write)(IDeserializer &, int, IDeserializer *) const,
	bool (Object::*map)(AccessorVariantMapContext &) const)
	: mReadElement(read)
	, mWriteElement(write)
	, mMapSerialization(map)
{}

bool AccessorVariantMap::Deserialize(IDeserializer &deserializer, Object *object) const
{
	BidirectionalSerializer bidi(deserializer);

	KeyType key;
	ValueType value;

	while(DoDeserialize(bidi, key, value))
	{
		WriteElement(object, key, -1, &value);
	}

	return bidi.Succeeded();

	return false;
}

bool AccessorVariantMap::Serialize(ISerializer &serializer, const Object *object) const
{
	BidirectionalSerializer bidi(serializer);
	AccessorVariantMapContext ctx(bidi);

	return (object->*mMapSerialization)(ctx);
}

bool AccessorVariantMap::DeserializeElement(IDeserializer *value, IDeserializer &key, int, Object *) const
{
	return false;
}

bool AccessorVariantMap::SerializeItem(ISerializer &value, IDeserializer &key, int, const Object *) const
{
	return false;
}

AccessorVariantMapContext::AccessorVariantMapContext(BidirectionalSerializer &bidi)
	: mBidi(bidi)
{}

BidirectionalSerializer &AccessorVariantMapContext::Bidi()
{
	return mBidi;
}

void AccessorVariantMapContext::BeginItem()
{
	mItemCommand.Setup(Command::ELEMENT);

	if(false == mBidi.Serializer()->beginCommand(mItemCommand))
		mBidi.Fail();

	mAttributeCommand.Setup(Command::EATTRIBUTE, "key");

	if(false == mBidi.Serializer()->beginCommand(mAttributeCommand))
		mBidi.Fail();
}

void AccessorVariantMapContext::BeginValue()
{
	if(false == mBidi.Serializer()->endCommand(mAttributeCommand))
		mBidi.Fail();
}

void AccessorVariantMapContext::EndItem()
{
	if(false == mBidi.Serializer()->endCommand(mItemCommand))
		mBidi.Fail();
}
#endif

} }
