////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>

#include <ork/asset/Asset.h>
#include <ork/asset/AssetSet.h>
#include <ork/asset/AssetSetLevel.h>
#include <ork/asset/AssetSetEntry.h>
#include <ork/asset/AssetLoader.h>
#include <ork/reflect/RegisterProperty.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace asset {
///////////////////////////////////////////////////////////////////////////////

//class Asset;

template<typename Operator>
static void Apply(AssetSetLevel *top_level, Operator op, int depth = -1);

std::pair<AssetSetEntry *, bool> FindAssetEntryInternal(AssetSetLevel *top_level, PoolString name);

///////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename ReturnType>
class SimpleExecutor
{
	ReturnType (ClassType::*mMemberFunction)(AssetSetLevel *level);
	AssetSetLevel *mLevel;
public:
	SimpleExecutor(ReturnType (ClassType::*function)(AssetSetLevel *level), AssetSetLevel *level)
		: mMemberFunction(function)
		, mLevel(level)
	{}

	ReturnType operator()(ClassType *object)
	{
		return (object->*mMemberFunction)(mLevel);
	}
};

///////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename ReturnType>
SimpleExecutor<ClassType, ReturnType> BuildExecutor(ReturnType (ClassType::*function)(AssetSetLevel *), AssetSetLevel *level)
{
	return SimpleExecutor<ClassType, ReturnType>(function, level);
}

///////////////////////////////////////////////////////////////////////////////

AssetSet::AssetSet()
	: mTopLevel(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////

void AssetSet::Register(PoolString name, Asset *asset, AssetLoader *loader)
{
	if(NULL == mTopLevel)
		PushLevel(ork::rtti::safe_downcast<AssetClass *>(asset->GetClass()));

	std::pair<AssetSetEntry *, bool> result = FindAssetEntryInternal(mTopLevel, name);
	AssetSetEntry *entry = result.first;
	
	if(entry == NULL)
	{
		OrkAssert(asset != NULL);
		entry = new AssetSetEntry(asset, loader, mTopLevel);
	}

	if(false == result.second)
		mTopLevel->GetSet().push_back(entry);
}

///////////////////////////////////////////////////////////////////////////////

Asset *AssetSet::FindAsset(PoolString name)
{
	AssetSetEntry *entry = FindAssetEntry(name);

	if(entry)
		return entry->GetAsset();

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

AssetSetEntry *AssetSet::FindAssetEntry(PoolString name)
{
	return FindAssetEntryInternal(mTopLevel, name).first;
}

///////////////////////////////////////////////////////////////////////////////

AssetLoader *AssetSet::FindLoader(PoolString name)
{
	AssetSetEntry *entry = FindAssetEntry(name);

	if(entry)
		return entry->GetLoader();

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

struct AssetEntryCompare : public std::binary_function<PoolString, const AssetSetEntry *, bool>
{
public:
	bool operator()(PoolString name, const AssetSetEntry *entry) const
	{
		return name == entry->GetAsset()->GetName();
	}
};

///////////////////////////////////////////////////////////////////////////////

std::pair<AssetSetEntry *, bool> FindAssetEntryInternal(AssetSetLevel *top_level, PoolString name)
{
	for(AssetSetLevel *level = top_level; level != NULL; level = level->Parent())
	{
		AssetSetLevel::SetType::iterator it 
			= std::find_if(
				level->GetSet().begin(), 
				level->GetSet().end(), 
				std::bind1st(AssetEntryCompare(), name));

		if(it != level->GetSet().end())
		{
			return std::make_pair(*it, level == top_level);
		}
	}

	return std::make_pair(static_cast<AssetSetEntry *>(NULL), false);
}

///////////////////////////////////////////////////////////////////////////////

bool AssetSet::Load(int depth)
{
	int load_count = 0;
	ork::ConstString name("");

	for(AssetSetLevel *level = mTopLevel; depth != 0 && level != NULL; level = level->Parent(), depth--)
	{
		for(orkvector<AssetSetEntry *>::size_type i = 0; i < level->GetSet().size(); ++i)
		{
			AssetSetEntry *entry = level->GetSet()[i];
			if(false == entry->IsLoaded())
			{	
				if(entry->Load(mTopLevel))
				{
					load_count++;
					name = entry->GetAsset()->GetClass()->Name();
				}
			}
		}
	}

	return load_count != 0;
}

///////////////////////////////////////////////////////////////////////////////

#if defined(ORKCONFIG_ASSET_UNLOAD)
bool AssetSet::UnLoad(int depth)
{
	int unload_count = 0;
	ork::ConstString name("");

	for(AssetSetLevel *level = mTopLevel; depth != 0 && level != NULL; level = level->Parent(), depth--)
	{
		for(orkvector<AssetSetEntry *>::size_type i = 0; i < level->GetSet().size(); ++i)
		{
			AssetSetEntry *entry = level->GetSet()[i];
			if(entry->UnLoad(mTopLevel))
			{
				unload_count--;
				name = entry->GetAsset()->GetClass()->Name();
			}
		}
	}

	return unload_count != 0;
}
#endif

///////////////////////////////////////////////////////////////////////////////

void AssetSet::PushLevel(AssetClass *type)
{
	Apply(mTopLevel, BuildExecutor(&AssetSetEntry::OnPush, mTopLevel));
	
	mTopLevel = new AssetSetLevel(mTopLevel);
}

///////////////////////////////////////////////////////////////////////////////

void AssetSet::PopLevel()
{
	Apply(mTopLevel, BuildExecutor(&AssetSetEntry::OnPop, mTopLevel));

	AssetSetLevel *top_level = mTopLevel;

	mTopLevel = mTopLevel->Parent();

	top_level->~AssetSetLevel();
}

///////////////////////////////////////////////////////////////////////////////

AssetSetLevel *AssetSet::GetTopLevel() const
{
	return mTopLevel;
}

///////////////////////////////////////////////////////////////////////////////

template<typename Operator>
void Apply(AssetSetLevel *top_level, Operator op, int depth)
{
	for(AssetSetLevel *level = top_level; depth != 0 && level != NULL; level = level->Parent(), depth--)
	{
		std::for_each(level->GetSet().begin(), level->GetSet().end(), op);
	}
}

///////////////////////////////////////////////////////////////////////////////
} }
///////////////////////////////////////////////////////////////////////////////
