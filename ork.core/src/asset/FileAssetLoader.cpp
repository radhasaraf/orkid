////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>

#include <ork/application/application.h>
#include <ork/asset/Asset.h>
#include <ork/asset/FileAssetLoader.h>
#include <ork/kernel/string/PieceString.h>
#include <ork/kernel/string/ArrayString.h>
#include <ork/kernel/string/PoolString.h>
#include <ork/file/file.h>
#include <ork/kernel/Array.h>
#include <ork/kernel/string/string.h>
#include <ork/file/path.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace asset {
///////////////////////////////////////////////////////////////////////////////

std::set<file::Path> FileAssetLoader::EnumerateExisting()
{
	std::set<file::Path> rval;
	//GetLoaders
	for( auto& item : mLocations )
	{
		auto wild = file_ext_t("*")+item.mExt;
		auto dir = item.mPathBase;


		auto files = CFileEnv::filespec_search( wild.c_str(), dir );
		int inumfiles = (int) files.size();

		orkprintf( "FileAssetLoader<%p> searching<%s> for<%s> inumfiles<%d>\n",
					this,
					dir.c_str(),
					wild.c_str(),
					inumfiles );

		file::Path::NameType searchdir( dir.ToAbsolute().c_str() );
		searchdir.replace_in_place("\\","/");
		for( int ifile=0; ifile<inumfiles; ifile++ )
		{
			auto the_file = files[ifile];
			auto the_stripped = CFileEnv::filespec_strip_base( the_file, "./" );
			file::Path::NameType ObjPtrStr = CFileEnv::filespec_no_extension( the_stripped );
			file::Path::NameType ObjPtrStrA;
			ObjPtrStrA.replace(ObjPtrStr.c_str(), searchdir.c_str(), "" );
			//OrkSTXFindAndReplace( ObjPtrStrA, searchdir, file::Path::NameType("") );
			file::Path::NameType ObjPtrStr2 = file::Path::NameType(dir.c_str()) + ObjPtrStrA;
			file::Path OutPath( ObjPtrStr2.c_str() );
			//orkprintf( "FOUND ASSET<%s>\n", the_file.c_str() );

			rval.insert(OutPath);
		}
	}
	return rval;
}

void FileAssetLoader::AddLocation( file_pathbase_t b, file_ext_t e)
{
	file::Path p(b.c_str());

	FileSet fset;
	fset.mExt = e;
	fset.mLoc = p.HasUrlBase() ? p.GetUrlBase() : "";
	fset.mPathBase = b;
	mLocations.push_back(fset);

	printf( "FileAssetLoader added set ext<%s> loc<%s> base<%s>\n",
			fset.mExt.c_str(),
			fset.mLoc.c_str(),
			fset.mPathBase.c_str() );
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetLoader::FindAsset(const PieceString &name, MutableString result, int first_extension)
{
	//////////////////////////////////////////
	// do we already have an extension
	//////////////////////////////////////////

	file::Path pathobjnoq(name);
	file::Path pathobj(name);
	AssetPath::NameType pathsp, qrysp;
	pathobj.SplitQuery( pathsp, qrysp );
	pathobjnoq.Set( pathsp.c_str() );

	file::Path::NameType preext;
	preext.format( ".%s", pathobjnoq.GetExtension().c_str() );
	bool has_extension = pathobjnoq.GetExtension().length()!=0;
	bool has_valid_extension = false;
	if( has_extension )
	{
		for( auto l : mLocations )
		{
			if( 0 == strcmp(l.mExt.c_str(),preext.c_str()) )
			{
				has_valid_extension = true;
			}
		}
	}

    orkprintf( "FindAsset<%s> has_valid_extension<%d>\n",
     ork::Application::AddPooledString(name).c_str(),
     int(has_valid_extension)
     );

	//////////////////////////////////////////
	// check Munged Paths first (Munged path is a path run thru 1 or more path converters)
	//////////////////////////////////////////

	file::Path::SmallNameType url = pathobjnoq.GetUrlBase();

	const SFileDevContext& ctx = ork::CFileEnv::UrlBaseToContext(url);

	//////////////////////
	// munge the path
	//////////////////////

	const orkvector<SFileDevContext::path_converter_type>& converters = ctx.GetPathConverters();

	int inumc = int( converters.size() );

	ork::fixedvector<ork::file::Path,8> MungedPaths;

	for( int i=0; i<inumc; i++ )
	{
		file::Path MungedPath = pathobjnoq;
		bool bret = converters[i]( MungedPath );
		if( bret )
		{
			MungedPaths.push_back(MungedPath);
    orkprintf( "MungedPaths<%s>\n", MungedPath.c_str() );
		}
	}
	//////////////////////////////////////
	// original path has lower priority
	MungedPaths.push_back( pathobjnoq );
	//////////////////////////////////////
    orkprintf( "MungedPaths<%s>\n", pathobjnoq.c_str() );

	//////////////////////
	// path is munged
	//////////////////////

	size_t inummunged = MungedPaths.size();

	for( size_t i=0; i<inummunged; i++ )
	{
		ork::file::Path MungedPath = MungedPaths[i];

		if( has_valid_extension ) // path already have an extension ?
		{
			if(CFileEnv::DoesFileExist(MungedPath))
			{
				result = MungedPath.c_str();
				return true;
			}
		}
		else // no extension test the registered extensions
		{
			for( auto l : mLocations )
			{
				MungedPath.SetExtension( l.mExt.c_str() );

				printf( "munged_ext<%s>\n", MungedPath.c_str() );

				if(CFileEnv::DoesFileExist(MungedPath))
				{
					//pathobj.SetExtension( extension.c_str() );

					result = MungedPath.c_str();
					return true;
				}
			}
		}
	}

	//////////////////////////////////////////
	// if we got here then munged paths do not exist
	// try the original path
	//////////////////////////////////////////

	PieceString thename = name;

	if( has_valid_extension )
	{
					//printf( "TESTPTH3<%s>\n", pathobjnoq.c_str() );
		if(CFileEnv::DoesFileExist(pathobjnoq))
		{
			ork::PieceString ps(pathobjnoq.c_str());
			result = ps;
			//intf( "PTH3<%s>\n", pathobjnoq.c_str() );
			return true;
		}
	}
	else
	{
		for( auto l : mLocations )
		{
			pathobjnoq.SetExtension( l.mExt.c_str() );

			//		printf( "TESTPTH4<%s>\n", pathobjnoq.c_str() );
			if(CFileEnv::DoesFileExist(pathobjnoq))
			{
				result = pathobjnoq.c_str();
				//printf( "PTH4<%s>\n", pathobjnoq.c_str() );
				return true;
			}
		}
	}

	//printf( "NOTFOUND\n" );
	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetLoader::CheckAsset(const PieceString &name)
{
	ArrayString<0> null_result;

	return FindAsset(name, null_result);
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetLoader::LoadAsset(Asset *asset)
{
	float ftime1 = ork::CSystem::GetRef().GetLoResRelTime();
#if defined(_XBOX) && defined(PROFILE)
	PIXBeginNamedEvent(0, "FileAssetLoader::LoadAsset(%s)", asset->GetName());
#endif
	ArrayString<256> asset_name;
				
	///////////////////////////////////////////////////////////////////////////////
	if(false == FindAsset(asset->GetName(), asset_name))
	{
		orkprintf("Error Loading File Asset %s\n", asset->GetName().c_str());
#if defined(ORKCONFIG_ASSET_UNLOAD)
		return false;
#else
		OrkAssertI(false, "Can't file asset second-time around");
#endif
	}

	bool out = LoadFileAsset(asset, asset_name);
#if defined(_XBOX) && defined(PROFILE)
	PIXEndNamedEvent();
#endif
	float ftime2 = ork::CSystem::GetRef().GetLoResRelTime();

	static float ftotaltime = 0.0f;
	static int iltotaltime = 0;

	ftotaltime += (ftime2-ftime1);

	int itotaltime = int(ftotaltime);

	//if( itotaltime > iltotaltime )
	{
		std::string outstr = ork::CreateFormattedString(
		"FILEAsset AccumTime<%f>\n", ftotaltime );
		////OutputDebugString( outstr.c_str() );
		iltotaltime = itotaltime;
	}
	return out;
}

///////////////////////////////////////////////////////////////////////////////
} }
///////////////////////////////////////////////////////////////////////////////
