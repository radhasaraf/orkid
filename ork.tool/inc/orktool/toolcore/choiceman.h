///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2010, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

#ifndef _ORK_TOOLCORE_CHOICEMAN_H
#define _ORK_TOOLCORE_CHOICEMAN_H

#include <ork/kernel/slashnode.h>
#include <ork/kernel/any.h>

#include <ork/file/path.h>

///////////////////////////////////////////////////////////////////////////////

class QMenu;

namespace ork {
namespace tool {

///////////////////////////////////////////////////////////////////////////////

class ChoiceFunctor
{
public:
	virtual std::string ComputeValue( const std::string & ValueStr ) const;
};
	
///////////////////////////////////////////////////////////////////////////////

class CAttrChoiceValue
{
    public: //

    CAttrChoiceValue()
		: mName( "defaultchoice" )
		, mValue( "" )
		, mpSlashNode( 0 )
		, mShortName( "" )
		, mpFunctor( 0 )
    {
    }

	CAttrChoiceValue( const std::string &nam, const std::string &val, const std::string & shname = ""  )
		: mName( nam )
		, mValue( val )
		, mShortName( shname )
		, mpFunctor( 0 )
	{
    }

	CAttrChoiceValue set( const std::string & nname, const std::string &nval, const std::string & shname = "" )
    {
        mName = nname;
        mValue = nval;
        mShortName = shname;
        return (*this);
    }

	std::string GetValue( void ) const
	{
		return mValue;
	}
	std::string EvaluateValue( void ) const
	{
		return (mpFunctor==0) ? mValue : mpFunctor->ComputeValue(mValue);
	}
	void SetValue( const std::string &val ) { mValue = val; }
    SlashNode* GetSlashNode( void ) const { return mpSlashNode; }
    void SetSlashNode( SlashNode *pnode ) { mpSlashNode = pnode; }

    const std::string GetName( void ) const { return mName; }
    void SetName( const std::string &name ) { mName = name; }
    const std::string GetShortName( void ) const { return mShortName; }
    void SetShortName( const std::string &name ) { mShortName = name; }

	void AddKeyword( const std::string & Keyword )
	{
		mKeywords.insert( Keyword );
	}

	bool HasKeyword( const std::string & Keyword ) const { return mKeywords.find(Keyword)!=mKeywords.end(); }

	void CopyKeywords( const CAttrChoiceValue &From ) { mKeywords=From.mKeywords; }

	void SetFunctor( const ChoiceFunctor* ftor ) { mpFunctor=ftor; }
	const ChoiceFunctor* GetFunctor( void ) const { return mpFunctor; }

	void SetCustomData( const any64& data ) { mCustomData=data; }
	const any64& GetCustomData() const { return mCustomData; }

private:

	std::string				mValue;
	std::string				mName;
    std::string				mShortName;
	SlashNode*				mpSlashNode;
	orkset<std::string>		mKeywords;
	const ChoiceFunctor*	mpFunctor;
	any64					mCustomData;
};

///////////////////////////////////////////////////////////////////////////////

typedef std::pair<std::string,std::string> ChoiceListFilter;

struct ChoiceListFilters
{
	orkmultimap<std::string,std::string> mFilterMap;


	void AddFilter( const std::string &key, const std::string &val ) { mFilterMap.insert( ChoiceListFilter( key,val ) ); }
	bool KeyMatch( const std::string &key, const std::string &val ) const;

};

class CChoiceList
{
private:

    orkvector< CAttrChoiceValue* >						mChoicesVect;
	orkmap< std::string, CAttrChoiceValue * >			mValueMap;
    orkmap< std::string, CAttrChoiceValue * >			mNameMap;
    orkmap< std::string, CAttrChoiceValue * >			mShortNameMap;
    SlashTree*											mHierarchy;

    void UpdateHierarchy( void );

protected:
	
    void remove( const CAttrChoiceValue & val );

	void clear( void );

public: //

    CChoiceList();
    virtual ~CChoiceList();

	void FindAssetChoices(const file::Path& sdir, const std::string& wildcard);

    const CAttrChoiceValue* FindFromLongName( const std::string &longname  ) const ;
    const CAttrChoiceValue* FindFromShortName( const std::string &shortname ) const ;
	const CAttrChoiceValue* FindFromValue( const std::string & uval ) const;

	const SlashTree* GetHierarchy( void ) const { return mHierarchy; }

	virtual void EnumerateChoices( bool bforcenocache=false ) = 0;
	void add( const CAttrChoiceValue & val );

	QMenu* CreateMenu( const ChoiceListFilters *Filter = 0 ) const;

	bool DoesSlashNodePassFilter( const SlashNode *pnode, const ChoiceListFilters *Filter ) const;

	void dump( void );
};

///////////////////////////////////////////////////////////////////////////////

class CChoiceManager
{
	orkmap<std::string,CChoiceList*>	mChoiceListMap;

public:

	void AddChoiceList( const std::string & ListName, CChoiceList *plist );
	const CChoiceList *GetChoiceList( const std::string & ListName ) const;
	CChoiceList *GetChoiceList( const std::string & ListName );

	CChoiceManager();
	~CChoiceManager();

};


///////////////////////////////////////////////////////////////////////////////

} } // namespace ork::tool

///////////////////////////////////////////////////////////////////////////////

#endif // _ORK_TOOLCORE_CHOICEMAN_H
