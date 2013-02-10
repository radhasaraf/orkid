////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>
#include <ork/reflect/serialize/BinaryDeserializer.h>
#include <ork/reflect/Command.h>
#include <ork/reflect/IProperty.h>
#include <ork/reflect/IObjectProperty.h>
#include <ork/rtti/Class.h>
#include <ork/rtti/Category.h>
#include <ork/rtti/downcast.h>

namespace ork { namespace reflect { namespace serialize {

BinaryDeserializer::BinaryDeserializer(stream::IInputStream &stream)
    : mStream(stream)
    , mCurrentCommand(NULL)
{
}

BinaryDeserializer::~BinaryDeserializer()
{
	for(int i = 0; i < mStringPool.Size(); i++)
		delete mStringPool.FromIndex(i).c_str();
}

template<>
bool BinaryDeserializer::Read<ConstString>(ConstString &text)
{
	int len_or_backref;

	Read(len_or_backref);

	if(len_or_backref < 0)
	{
		int len = -(len_or_backref + 1);
		
		char *data = new char[len + 1];
		mStream.Read(reinterpret_cast<unsigned char *>(data), size_t(len));
		data[len] = '\0';

		text = data;

		mStringPool.Literal(data);
		return true;
	}
	else
	{
		text = mStringPool.FromIndex(len_or_backref).c_str();
		return true;
	}
}

template<typename T>
bool BinaryDeserializer::Read(T &value)
{
	return mStream.Read(reinterpret_cast<unsigned char *>(&value), sizeof(T)) == sizeof(T);
}

template bool BinaryDeserializer::Read<char>(char &);
template bool BinaryDeserializer::Read<short>(short &);
template bool BinaryDeserializer::Read<int>(int &);
template bool BinaryDeserializer::Read<long>(long &);
template bool BinaryDeserializer::Read<float>(float &);
template bool BinaryDeserializer::Read<double>(double &);
template bool BinaryDeserializer::Read<bool>(bool &);

bool BinaryDeserializer::Deserialize(char &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(short &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(int &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(long &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(float &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(double &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(bool &value)
{
	return Read(value);
}

bool BinaryDeserializer::Deserialize(const IProperty *prop)
{
	return prop->Deserialize(*this);
}

bool BinaryDeserializer::Deserialize(const IObjectProperty *prop, Object *object)
{
	return prop->Deserialize(*this, object);
}

bool BinaryDeserializer::ReferenceObject(rtti::ICastable *object)
{
	OrkAssert(FindObject(object) == -1);
	mDeserializedObjects.push_back(object);
	return true;
}

int BinaryDeserializer::FindObject(rtti::ICastable *object)
{
	for(orkvector<rtti::ICastable *>::size_type index = 0; index < mDeserializedObjects.size(); index++)
	{
		if(mDeserializedObjects[index] == object) return int(index);
	}

	return -1;
}

char BinaryDeserializer::Peek()
{
	char c;
	if(mStream.Peek(reinterpret_cast<unsigned char *>(&c), 1) == 1)
	{
		return c;
	}
	else
	{
		return 0;
	}
}

bool BinaryDeserializer::Match(char c)
{
	if(Peek() == c)
	{
		Read(c);
		return true;
	}
	else
	{
		return false;
	}
}

bool BinaryDeserializer::Deserialize(rtti::ICastable *&object)
{
	if(Match('N'))
	{
		object = NULL;
		return true;
	}
	if(Match('B'))
	{
		int id;

		Read(id);

		object = mDeserializedObjects[id];

		return true;
	}
	else if(Match('R'))
	{
		bool result = true;

		ConstString name;

		if(false == Read(name))
			result = false;

		const rtti::Category *category = rtti::downcast<rtti::Category *>(
				rtti::Class::FindClass(name));

		if(false == category->DeserializeReference(*this, object))
			result = false;

		if(false == Match('r'))
			result = false;
		
		return true;
	}
	else
	{
		orkprintf("BinaryDeserializer:: expected R, B, or N\n");
		return false;
	}
}

bool BinaryDeserializer::Deserialize(MutableString &text)
{
	ConstString data;
	Read(data);
	text = data;
	return true;
}

bool BinaryDeserializer::Deserialize(ResizableString &text)
{
	ConstString data;
	Read(data);
	text = data;
	return true;
}

bool BinaryDeserializer::DeserializeData(unsigned char *data, size_t size)
{
	return mStream.Read(data, size) == int(size);
}

bool BinaryDeserializer::BeginCommand(Command &command)
{
    bool result = false;

	if(Match('O'))
	{
		ConstString name;
		Read(name);
		command.Setup(Command::EOBJECT, name);
		result = true;
	}
	else if(Match('P'))
	{
		ConstString name;
		Read(name);
		command.Setup(Command::EPROPERTY, name);
		result = true;
	}
	else if(Match('A'))
	{
		ConstString name;
		Read(name);
		command.Setup(Command::EATTRIBUTE, name);
		result = true;
	}
	else if(Match('I'))
	{
		command.Setup(Command::EITEM, "");
		result = true;
	}

	if(result)
	{
		command.PreviousCommand() = mCurrentCommand;
		mCurrentCommand = &command;
	}

    return result;
}

bool BinaryDeserializer::EndCommand(const Command &command)
{
	bool result = true;

    if(mCurrentCommand == &command)
    {
        mCurrentCommand = mCurrentCommand->PreviousCommand();
    }
    else
    {
        orkprintf("Mismatched Serializer commands! expected: %s got: %s\n", 
                mCurrentCommand ? mCurrentCommand->Name().c_str() : "<no command>",
                command.Name().c_str());
        return false;
    }

    switch(command.Type())
    {
    case Command::EPROPERTY:
        result = Match('p');
        break;
    case Command::EATTRIBUTE:
		result = Match('a');
        break;
    case Command::EOBJECT:
        result = Match('o');
        break;
    case Command::EITEM:
        result = Match('i');
        break;
    }

	if(result == false)
	{
		orkprintf("failed to match end for tag '%s'\n", command.Name().c_str());
	}
    
    return result;
}

} } }
