////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef _UTIL_ASSETFILTER_H
#define _UTIL_ASSETFILTER_H

#include <ork/object/Object.h>
#include <ork/kernel/core/kerneltypes.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace tool {

///////////////////////////////////////////////////////////////////////////////

class FilterOption
{
	std::string		mOptionName;
	std::string		mDefaultValue;
	std::string		mValue;

public:

	FilterOption( const char* name, const char* defval ) : mOptionName(name), mDefaultValue(defval) {}
	FilterOption() : mOptionName(""), mDefaultValue(""), mValue("") {}

	bool RequiresValue() const { return mDefaultValue.length()>0; }
	const std::string& GetValue() const;
	const std::string& GetDefault() const { return mDefaultValue; }
	void SetValue( const char* pval );
	void SetDefault( const char* pval );
};

///////////////////////////////////////////////////////////////////////////////

struct FilterOptMap
{
	orkmap<std::string, FilterOption> moptions_map;

	void SetDefault( const char* name, const char* defval );

	FilterOption* GetOption( const std::string& key );
	const FilterOption* GetOption( const std::string& key ) const;
	
	void SetOptions( const tokenlist& options );
	bool HasOption( const std::string& key ) const;
	void DumpOptions() const;
	void DumpDefaults() const;

	FilterOptMap();
};

///////////////////////////////////////////////////////////////////////////////

class CAssetFilterContext //: public IInterface::Context
{
	public: //
	std::string infile, outfile, mifname;
	CAssetFilterContext( std::string ifname, std::string inf, std::string outf )
		: infile(inf)
		, outfile(outf)
		, mifname( ifname )
	{
	
	}

	virtual void Clear( void ) {}
};

///////////////////////////////////////////////////////////////////////////////
// newer better filters (adds interface support for various functions like configurable logging)


class CAssetFilterBase : public ork::Object
{
	RttiDeclareAbstract(CAssetFilterBase,ork::Object);

public: //

	CAssetFilterBase();

	bool IsConverted( void ) const { return false; }

	virtual bool ConvertAsset( const tokenlist& toklist ) = 0;

	private:
};


///////////////////////////////////////////////////////////////////////////////

//typedef bool(*AssetConvCB)( std::string inname, std::string outname );

struct SFilterInfo
{
	//ork::PoolString inext;
	//ork::PoolString outext;
	ork::PoolString filtername;
	ork::PoolString classname;
	ork::PoolString pathmethod;	// leave rebase
	ork::PoolString pathloc;	// rebase location
};

class CAssetFilter
{
	public: //

	static orkvector< CAssetFilter* >					svFilters;
	static orkmap< ork::PoolString, SFilterInfo* >		smFilterMap;

	static bool ConvertFile( const char* Filter, const tokenlist& toklist );
	static bool ConvertTree( const char* Filter, const std::string& InTree, const std::string& OutDir );
	static bool ListFilters( void );
	static void RegisterFilter( const char* filtername, const char* classname, const char* pathmethod="leave", const char* pathloc="." );

	virtual bool ConvertAsset( tokenlist toklist ) = 0;

};

#define RegFilt( nam,cls ) AssetFilterMap.insert(pair<string,AssetConvCB>((string)nam,(AssetConvCB)cls::ConvertAsset));

///////////////////////////////////////////////////////////////////////////////

} }

#endif

