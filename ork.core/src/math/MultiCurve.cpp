////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


///////////////////////////////////////////////////////////////////////////////
#include <ork/pch.h>
#include <ork/math/multicurve.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/reflect/DirectObjectMapPropertyType.hpp>
#include <ork/reflect/DirectObjectVectorPropertyType.hpp>
#include <ork/reflect/enum_serializer.h>

INSTANTIATE_TRANSPARENT_RTTI(ork::MultiCurve1D,"MultiCurve1D");

BEGIN_ENUM_SERIALIZER(ork, EMCSEGTYPE)
	DECLARE_ENUM(EMCST_LINEAR)
	DECLARE_ENUM(EMCST_BOX)
	DECLARE_ENUM(EMCST_LOG)
	DECLARE_ENUM(EMCST_EXP)
END_ENUM_SERIALIZER()

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

void MultiCurve1D::Describe()
{
	ork::reflect::RegisterArrayProperty("Segs", &ork::MultiCurve1D::mSegmentTypes);
	ork::reflect::RegisterMapProperty("Verts", &ork::MultiCurve1D::mVertices);

	ork::reflect::RegisterProperty("Curve", &ork::MultiCurve1D::ProxyAccessor);

	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >("Segs", "editor.visible", "false" );
	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >("Verts", "editor.visible", "false" );

	ork::reflect::RegisterProperty("min", &ork::MultiCurve1D::mMin);
	ork::reflect::RegisterProperty("max", &ork::MultiCurve1D::mMax);

	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >("Curve", "editor.class", "ged.factory.curve1d" );

	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >( "min", "editor.range.min", "-2000.0f" );
	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >( "min", "editor.range.max", "2000.0f" );

	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >( "max", "editor.range.min", "-2000.0f" );
	ork::reflect::AnnotatePropertyForEditor< MultiCurve1D >( "max", "editor.range.max", "2000.0f" );

	static const char* EdGrpStr = "sort://min max Curve";

	reflect::AnnotateClassForEditor<MultiCurve1D>( "editor.prop.groups", EdGrpStr );

}

///////////////////////////////////////////////////////////////////////////////

MultiCurve1D::MultiCurve1D()
	: mMin( 0.0f )
	, mMax( 1.0f )
	, mProxy(this)
{
	Init(1);
}

///////////////////////////////////////////////////////////////////////////////

void MultiCurve1D::Init( int inumsegs )
{
	OrkAssert( inumsegs>0 );

	mSegmentTypes.reserve(inumsegs);
	mVertices.reserve(inumsegs+1);

	mSegmentTypes.clear();
	mVertices.clear();

	for( int i=0; i<inumsegs; i++ )
	{
		mSegmentTypes.push_back(EMCST_LINEAR);
		float fu = float(i) / float(inumsegs);
		mVertices.AddSorted(fu,0.0f);
	}
	mVertices.AddSorted(1.0f,1.0f);

	OrkAssert( IsOk() == true );
}

///////////////////////////////////////////////////////////////////////////////

bool MultiCurve1D::IsOk() const
{
	int inumseg = GetNumSegments();
	int inumV = int(mVertices.size());

	bool bOK = true;

	bOK &= (inumseg==(inumV-1));
	bOK &= (mVertices.begin()->first == 0.0f );
	bOK &= ((mVertices.end()-1)->first == 1.0f );

	OrkAssert( bOK ); // temporary

	return bOK;
}

///////////////////////////////////////////////////////////////////////////////

int MultiCurve1D::GetNumSegments() const
{
	return int(mSegmentTypes.size());
}

///////////////////////////////////////////////////////////////////////////////

float MultiCurve1D::Sample(float fu) const
{
	if( fu<0.0f ) fu=0.0f;
	if( fu>1.0f ) fu=1.0f;
	if( isnan(fu) ) fu=0.0f;

	bool bdone = false;
	int isega = 0;
	int isegb = 0;
	int inumv = int(mVertices.size());
	if( 0==inumv ) return 0.0f;

	while( false == bdone )
	{
		isegb = (isega+1);
		if( isega>=inumv ) return 0.0f;
		if( isegb>=inumv ) return 0.0f;


		if(		(fu >= mVertices.GetItemAtIndex(isega).first)
			&&	(fu <= mVertices.GetItemAtIndex(isegb).first) )
		{
			bdone = true;
		}
		else
		{
			isega++;
		}
	}

	float rval = 0.0f;

	const std::pair<float,float>& VA = mVertices.GetItemAtIndex(isega);
	const std::pair<float,float>& VB = mVertices.GetItemAtIndex(isegb);

	float dU = VB.first-VA.first;
	float dV = VB.second-VA.second;
	float Slope = dV/dU;
	float Base = VA.first;

	float fsu = (fu-Base)/dU;

	switch( mSegmentTypes[isega] )
	{
		case EMCST_LINEAR:
		{	float fisu = 1.0f - fsu;
			rval = (VA.second*fisu)+(VB.second*fsu);
			break;
		}
		case EMCST_LOG:
		{	float fsup = fsu+(fsu-(fsu*fsu));
			float fisu = 1.0f - fsup;
			rval = (VA.second*fisu)+(VB.second*fsup);
			break;
		}
		case EMCST_EXP:
		{	float fsup = (fsu*fsu);
			float fisu = 1.0f - fsup;
			rval = (VA.second*fisu)+(VB.second*fsup);
			break;
		}
		case EMCST_BOX:
		{	rval = VA.second;
			break;
		}
		default:
			OrkAssert(false);
			break;
	}

	return mMin + rval*(mMax-mMin);
}

///////////////////////////////////////////////////////////////////////////////

void MultiCurve1D::SplitSegment(int iseg)
{
	int inumseg = GetNumSegments();
	OrkAssert( iseg < inumseg );
	OrkAssert( iseg >= 0 );
	OrkAssert( iseg < int(mVertices.size()+1) );

	int iva = iseg;
	int ivb = iseg+1;
	const std::pair<float,float>& VA = mVertices.GetItemAtIndex(iva);
	const std::pair<float,float>& VB = mVertices.GetItemAtIndex(ivb);

	mVertices.AddSorted( (VA.first+VB.first)*0.5f, (VA.second+VB.second)*0.5f );

	mSegmentTypes.insert(mSegmentTypes.begin()+ivb, EMCST_LINEAR );

	OrkAssert( IsOk() == true );

}

///////////////////////////////////////////////////////////////////////////////

void MultiCurve1D::MergeSegment(int ifirstseg)
{
	int inumseg = GetNumSegments();

	OrkAssert( ifirstseg < inumseg );
	OrkAssert( ifirstseg >= 0 );

	orklut<float,float>::iterator it = mVertices.begin()+ifirstseg;
	mVertices.RemoveItem(it);

	orkvector<EMCSEGTYPE>::iterator it2 = mSegmentTypes.begin()+ifirstseg;
	mSegmentTypes.erase(it2);

	OrkAssert( IsOk() == true );
}

///////////////////////////////////////////////////////////////////////////////

void MultiCurve1D::SetSegmentType( int iseg, EMCSEGTYPE etype )
{
	int inumseg = GetNumSegments();
	OrkAssert( iseg < inumseg );
	OrkAssert( iseg >= 0 );

	mSegmentTypes[iseg] = etype;

	OrkAssert( IsOk() == true );
}

///////////////////////////////////////////////////////////////////////////////

void MultiCurve1D::SetPoint( int ipoint, float fu, float fv )
{
	int inumV = int(mVertices.size());

	OrkAssert( ipoint <= inumV );
	OrkAssert( ipoint >= 0 );

	orklut<float,float>::iterator it = mVertices.begin()+ipoint;

	mVertices.RemoveItem(it);
	mVertices.AddSorted( fu, fv );

	OrkAssert( IsOk() == true );
}

///////////////////////////////////////////////////////////////////////////////

bool MultiCurve1D::PreDeserialize( ork::reflect::IDeserializer& deser )
{
	mVertices.clear();
	mSegmentTypes.clear();
	return true;
}

bool MultiCurve1D::PostDeserialize(reflect::IDeserializer &)
{
	return(true);
}



}
///////////////////////////////////////////////////////////////////////////////
