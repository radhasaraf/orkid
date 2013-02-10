///////////////////////////////////////////////////////////////////////////////
// Orkid2
// Copyright 1996-2010, Michael T. Mayers
// See License at OrkidRoot/license.html or http://www.tweakoz.com/orkid2/license.html
///////////////////////////////////////////////////////////////////////////////

#ifndef _STL_EXT_H
#define _STL_EXT_H

#include <ctype.h>
#include <ork/orktypes.h>
#include <ork/orkstl.h>

//#define OrkSTXSafeDelete(p) { if(p && ((U32) p != 0xcdcdcdcd) && ((U32) p != 0xbaadf00d)) { delete (p); (p)=NULL; } }

/*template< typename TYPE > void free_vect( orkvector< TYPE > *vec )
{
	int numinvect = vec->size();

	for( int i=0; i<numinvect; i++ )
	{
		TYPE delobj = (*vec)[i];
		OrkSTXSafeDelete(delobj);
	}
	vec->clear();
}*/

///////////////////////////////////////////////////////////////////////////////

struct lower
{
#if defined(WII)
	unsigned char operator() (unsigned char c) { return (unsigned char)std::tolower(int(c)); }
#else
	unsigned char operator() (unsigned char c) { return (unsigned char)tolower(int(c)); }
#endif
};

///////////////////////////////////////////////////////////////////////////////

struct OrkSTXPredicateAlways
{
	template< typename Type > bool operator()( Type T ) { return true; }
};

struct OrkSTXPredicateNever
{
	template< typename Type > bool operator()( Type T ) { return false; }
};

///////////////////////////////////////////////////////////////////////////////
// use this for sorting containers of pointers

template<typename T> class OrkSTXPointerPredGreater
{
	public: //

	int operator()( const T* p1, const T * p2) const
	{
		if(!p1)
		return false;
		if(!p2)
		return true;
		return *p1 > *p2;
	}
};

template<typename T> class OrkSTXPointerPredLess
{
	public: //

	int operator()( const T* p1, const T * p2) const
	{
		if(!p1)
		return false;
		if(!p2)
		return true;
		return *p1 , *p2;
	}
};

///////////////////////////////////////////////////////////////////////////////

template< typename TYPE > bool OrkSTXIsItemInVector( orkvector< TYPE > &vec, const TYPE & obj )
{
	bool rval = false;

	for( typename orkvector< TYPE >::iterator vIter=vec.begin(); ((vIter!=vec.end())&&(rval==false)); vIter++ )
	{	
		if( *vIter == obj )
		{
			rval = true;
		}
	}

	return rval;
}

///////////////////////////////////////////////////////////////////////////////

template< typename maptype >
typename maptype::mapped_type & OrkSTXRefValFromKey( const maptype &rMap, const typename maptype::key_type & key )
{
	static typename maptype::mapped_type Default;
	typename maptype::const_iterator it = rMap.find( key );
	return ( it != rMap.end() ) ? (*it).second : Default;
}

///////////////////////////////////////////////////////////////////////////////

template< typename maptype >
typename maptype::mapped_type OrkSTXFindValFromKey( const maptype &rMap, const typename maptype::key_type & key, const typename maptype::mapped_type & def=0 )
{
	typename maptype::mapped_type val = def;
	typename maptype::const_iterator it = rMap.find( key );
	if( it != rMap.end() )
		val = (*it).second;
	return val;
}

///////////////////////////////////////////////////////////////////////////////

template< typename maptype >
int OrkSTXFindKeysFromVal( const maptype &rMap, const typename maptype::mapped_type & val, orkvector<typename maptype::key_type> &rKeyVect )
{	int inumfound = 0;
	typename maptype::const_iterator it=rMap.begin();
	for( ; it!=rMap.end(); ++it )
	{	const typename maptype::value_type &mypr = *it;
		if( val == mypr.second )
		{	rKeyVect.push_back( mypr.first );
			inumfound++;
		}
	}
	return inumfound;
}

///////////////////////////////////////////////////////////////////////////////

template< typename maptype >
bool OrkSTXIsInMap( const maptype &rMap, const typename maptype::key_type &key )
{
	return ( rMap.find( key ) != rMap.end() );
}

///////////////////////////////////////////////////////////////////////////////

template< typename maptype >
bool OrkSTXMapInsert( maptype &rMap, const typename maptype::key_type & key, const typename maptype::mapped_type & val )
{	bool bRVAL = false;
	if( rMap.find( key ) == rMap.end() )
	{	rMap.insert( std::make_pair( key, val ) );
		bRVAL = true;
	}
	return bRVAL;
}

template< typename luttype >
bool OrkSTXLutInsert( luttype &rMap, const typename luttype::key_type & key, const typename luttype::mapped_type & val )
{	bool bRVAL = false;
	if( rMap.find( key ) == rMap.end() )
	{	rMap.AddSorted( key, val );
		bRVAL = true;
	}
	return bRVAL;
}

///////////////////////////////////////////////////////////////////////////////

template< typename multimaptype >
void OrkSTXMultiMapInsert( multimaptype &rMap, const typename multimaptype::key_type & key, const typename multimaptype::mapped_type & val )
{	rMap.insert( std::make_pair( key, val ) );
}

///////////////////////////////////////////////////////////////////////////////

template< typename maptype >
bool OrkSTXRemoveFromMap( maptype &rMap, const typename maptype::key_type & key )
{
	bool bRVAL = false;
	typename maptype::iterator it = rMap.find( key );
	if( it != rMap.end() )
	{
		rMap.erase( it );
		bRVAL = true;
	}
	return bRVAL;
}

template< typename multimaptype >
bool OrkSTXRemoveSingleKeyFromMultiMap( multimaptype &rMap, const typename multimaptype::key_type & key )
{
	bool bRVAL = false;
	typename multimaptype::iterator it = rMap.find( key );
	if( it != rMap.end() )
	{
		rMap.erase( it );
		bRVAL = true;
	}
	return bRVAL;
}

template< typename multimaptype  >
bool OrkSTXRemoveSingleValueFromMultiMap( multimaptype &rMap, const typename multimaptype::mapped_type & val )
{
	bool bRVAL = false;

	typename multimaptype::iterator it = rMap.begin();

	if( it != rMap.end() )
	{
		typename multimaptype::value_type & pr = *it;

		if( pr.second == val )
		{
			rMap.erase( it );
			bRVAL = true;
		}
	}

	return bRVAL;
}

template< typename settype >
bool OrkSTXRemoveFromSet( settype &rSet, const typename settype::key_type & val )
{
	bool bRVAL = false;

	typename settype::iterator it = rSet.find( val );

	if( it != rSet.end() )
	{
		rSet.erase( it );
		bRVAL = true;
	}

	return bRVAL;
}

template< typename multimaptype >
bool OrkSTXRemoveSingleKeyAndValueFromMultiMap( multimaptype &rMap, const typename multimaptype::key_type & key, const typename multimaptype::mapped_type & val )
{
	bool bRVAL = false;

	typename multimaptype::iterator it = rMap.find(key);

	while( it != rMap.end() )
	{
		const typename multimaptype::value_type & pr = *it;

		if(		(pr.second == val)
			&&	(pr.first == key) )
		{
			rMap.erase( it );

			return true;
		}
		else
		{
			it++;
		}
	}

	return bRVAL;
}

template< typename vectortype > bool OrkSTXRemoveSingleItemFromVect( vectortype &vec, const typename vectortype::value_type & val  )
{
	typename vectortype::iterator vIter = std::find( vec.begin(), vec.end(), val );

	bool brval = false;

	if( vec.end() != vIter )
	{
		vec.erase( vIter );
		brval = true;
	}

	return brval;
}

////////////////////////////////////////////////////////////////////////////////

template< typename KeyType, typename LessPredType >
bool OrkSTXPredSetInsert( std::set< KeyType, LessPredType > &rSet, const KeyType & key )
{	bool bRVAL = false;
	if( rSet.find( key ) == rSet.end() )
	{	rSet.insert( key );
		bRVAL = true;
	}
	return bRVAL;
}

////////////////////////////////////////////////////////////////////////////////

template< typename KeyType >
bool OrkSTXSetInsert( std::set< KeyType > &rSet, const KeyType & key )
{	bool bRVAL = false;
	if( rSet.find( key ) == rSet.end() )
	{	rSet.insert( key );
		bRVAL = true;
	}
	return bRVAL;
}

template< typename KeyType >
bool OrkSTXIsInSet( const std::set< KeyType > &rSet, const KeyType & key )
{
	return ( rSet.find( key ) != rSet.end() );
}

///////////////////////////////////////////////////////////////////////////////

template< typename T >
inline T OrkSTXClampToRange( T inp, T min, T max )
{	if( inp>max ) inp=max;	else if( inp<min ) inp=min;
	return inp;
}

///////////////////////////////////////////////////////////////////////////////

#define OrkSTXForEachInSet( Set, Type, it ) for( typename std::set<Type>::const_iterator it=Set.begin(); it!=Set.end(); ++it )

#endif
