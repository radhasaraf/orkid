///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2010, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <ork/object/ObjectClass.h>
#include <ork/rtti/RTTI.h>
#include <ork/asset/AssetCategory.h>
#include <ork/asset/AssetSet.h>
#include <ork/file/path.h>

namespace ork { namespace asset {

class Asset;
class AssetLoader;
class FileAssetNamer;

class AssetClass 
	: public object::ObjectClass
{
	RttiDeclareExplicit(AssetClass, object::ObjectClass, rtti::NamePolicy, AssetCategory);

public:
	AssetClass(const rtti::RTTIData &);

	AssetLoader* FindLoader(PieceString);
	Asset* LoadUnManagedAsset(PieceString);
	Asset* DeclareAsset(PieceString);
	Asset* FindAsset(PieceString);

	void AddLoader(AssetLoader *loader);

	Asset* CreateUnmanagedAsset(PieceString);

	AssetSet& GetAssetSet();

	void SetAssetNamer(const std::string& namer);
	void AddTypeAlias(ConstString alias);
	
	bool AutoLoad(int depth = -1);

	std::set<file::Path> EnumerateExisting() const;

private:

	FileAssetNamer *GetAssetNamer() const;
	FileAssetNamer *mAssetNamer;
	std::set<AssetLoader*> mLoaders;
	AssetSet mAssetSet;

};

} }
