////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/rtti/downcast.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/gfxmaterial_fx.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/kernel/orklut.hpp>
///////////////////////////////////////////////////////////////////////////////
#include <pkg/ent/scene.h>
#include <pkg/ent/entity.h>
#include <pkg/ent/entity.hpp>
#include <pkg/ent/drawable.h>
#include <pkg/ent/event/MeshEvent.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/reflect/AccessorObjectPropertyType.hpp>
#include <ork/reflect/DirectObjectPropertyType.hpp>
#include <ork/reflect/DirectObjectMapPropertyType.hpp>
#include <ork/gfx/camera.h>
///////////////////////////////////////////////////////////////////////////////
#include "SpinnyCamera.h"
#include <ork/kernel/string/string.h>
#include <pkg/ent/PerfController.h>

///////////////////////////////////////////////////////////////////////////////
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::SequenceCamArchetype, "SequenceCamArchetype" );
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::SequenceCamControllerData, "SequenceCamControllerData");
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::SequenceCamControllerInst, "SequenceCamControllerInst");

INSTANTIATE_TRANSPARENT_RTTI( ork::ent::SeqCamItemDataBase, "SeqCamItemDataBase" );
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::SpinnyCamControllerData, "SpinnyCamControllerData" );
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::CurvyCamControllerData, "CurvyCamControllerData" );

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////////

void SequenceCamArchetype::DoCompose(ork::ent::ArchComposer& composer) 
{
	composer.Register<ork::ent::SequenceCamControllerData>();
}

///////////////////////////////////////////////////////////////////////////////

void SequenceCamArchetype::Describe()
{
}

///////////////////////////////////////////////////////////////////////////////

SequenceCamArchetype::SequenceCamArchetype()
{
}

///////////////////////////////////////////////////////////////////////////////

void SequenceCamControllerData::Describe()
{
	ork::ent::RegisterFamily<SequenceCamControllerData>(ork::AddPooledLiteral("camera"));

	ork::reflect::RegisterMapProperty( "CamItems", & SequenceCamControllerData::mItemDatas );
	ork::reflect::AnnotatePropertyForEditor< SequenceCamControllerData >("CamItems", "editor.factorylistbase", "SeqCamItemDataBase" );
	ork::reflect::AnnotatePropertyForEditor< SequenceCamControllerData >("CamItems", "editor.map.policy.impexp", "true" );

	ork::reflect::RegisterProperty( "CurrentItem", & SequenceCamControllerData::mCurrentItem );
}

SequenceCamControllerData::SequenceCamControllerData()
{
}

///////////////////////////////////////////////////////////////////////////////

ent::ComponentInst* SequenceCamControllerData::CreateComponent(ent::Entity* pent) const
{
	return new SequenceCamControllerInst( *this, pent );
}

///////////////////////////////////////////////////////////////////////////////

void SequenceCamControllerInst::Describe()
{
}

///////////////////////////////////////////////////////////////////////////////

SequenceCamControllerInst::SequenceCamControllerInst(const SequenceCamControllerData& occd, Entity* pent )
	: ComponentInst( & occd, pent )
	, mCD( occd )
	, mpActiveItem(0)
{
	CameraDrawable* pcamd = new CameraDrawable(pent, & mCameraData ); // deleted when entity deleted
	pent->AddDrawable(AddPooledLiteral("Debug"),pcamd);
	pcamd->SetOwner(pent);
}

///////////////////////////////////////////////////////////////////////////////

bool SequenceCamControllerInst::DoLink(SceneInst *psi)
{
	//printf( "LINKING SpinnyCamControllerInst\n" );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SequenceCamControllerInst::DoStart(SceneInst *psi, const CMatrix4 &world)
{
	if( GetEntity() )
	{
		const ent::EntData& ED = GetEntity()->GetEntData();
		PoolString name = ED.GetName();
		std::string Name = CreateFormattedString( "%s", name.c_str() );
 		psi->SetCameraData( AddPooledString(Name.c_str()), & mCameraData );

		for( orklut<PoolString,ork::Object*>::const_iterator it=GetCD().GetItemDatas().begin(); it!=GetCD().GetItemDatas().end(); it++ )
		{
			if( it->second )
			{
				SeqCamItemDataBase* pdata = rtti::autocast(it->second);
				SeqCamItemInstBase* item = pdata->CreateInst( GetEntity() );
				
				mItemInsts.AddSorted( it->first, item );
				mpActiveItem = item;
			}
		}

	}

	//printf( "STARTING SpinnyCamControllerInst\n" );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

void SequenceCamControllerInst::DoUpdate( SceneInst* psi )
{
	const PoolString& ps = mCD.GetCurrentItem();
	orklut<PoolString,SeqCamItemInstBase*>::const_iterator it=mItemInsts.find(ps);
	
	if( it!=mItemInsts.end() )
	{
		mpActiveItem = it->second;
	}

	if( mpActiveItem )
	{
		//printf( "SequenceCamControllerInst<%p> ActiveItem<%s:%p>\n", this, ps.c_str(), mpActiveItem );
		mpActiveItem->DoUpdate(psi);
		mCameraData = mpActiveItem->GetCameraData();
	}
}

///////////////////////////////////////////////////////////////////////////////

bool SequenceCamControllerInst::DoNotify(const ork::event::Event *event)
{
	if( const ork::ent::PerfControlEvent* pce = rtti::autocast(event) )
	{
		const char* keyname = pce->mTarget.c_str();
		
		printf( "SequenceCamControllerInst<%p> PerfControlEvent<%p> key<%s>\n", this, pce, keyname );

		if( 0 == strcmp(keyname,"CurrentItem") )
		{
			mCD.GetCurrentItem()=AddPooledString(pce->mValue.c_str());
			printf( "Apply Value<%s> To SCCD<%p>\n", mCD.GetCurrentItem().c_str(), this );
		}
		return true;
	}
	else if( const ork::ent::PerfSnapShotEvent* psse = rtti::autocast(event) )
	{
		const PoolString& CurrentItem = mCD.GetCurrentItem();
		
		psse->PushNode( "CurrentItem" );
		{	
			ent::PerfSnapShotEvent::str_type NodeName = psse->GenNodeName();
			PerfProgramTarget* target = new PerfProgramTarget( NodeName.c_str(), CurrentItem.c_str() );
			psse->GetProgram()->AddTarget( NodeName.c_str(), target );
		}
		psse->PopNode();
				
		return true;
	}
	return false;
	
}

///////////////////////////////////////////////////////////////////////////////

void SeqCamItemDataBase::Describe()
{
}

SeqCamItemDataBase::SeqCamItemDataBase()
{
}

///////////////////////////////////////////////////////////////////////////////

SeqCamItemInstBase::SeqCamItemInstBase(const SeqCamItemDataBase& cd)
	: mCD(cd)
{
}

///////////////////////////////////////////////////////////////////////////////

void SpinnyCamControllerData::Describe()
{
	ork::reflect::RegisterProperty( "Elevation", &SpinnyCamControllerData::mfElevation );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Elevation", "editor.range.min", "-100.0f" );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Elevation", "editor.range.max", "100.0f" );

	ork::reflect::RegisterProperty( "Radius", &SpinnyCamControllerData::mfRadius );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Radius", "editor.range.min", "-1000.0f" );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Radius", "editor.range.max", "1000.0f" );

	ork::reflect::RegisterProperty( "SpinRate", &SpinnyCamControllerData::mfSpinRate );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "SpinRate", "editor.range.min", "-100.0f" );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "SpinRate", "editor.range.max", "100.0f" );

	ork::reflect::RegisterProperty( "Near", &SpinnyCamControllerData::mfNear );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Near", "editor.range.min", "0.1f" );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Near", "editor.range.max", "1000.0f" );

	ork::reflect::RegisterProperty( "Far", &SpinnyCamControllerData::mfFar );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Far", "editor.range.min", "1.0f" );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Far", "editor.range.max", "10000.0f" );

	ork::reflect::RegisterProperty( "Aper", &SpinnyCamControllerData::mfAper );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Aper", "editor.range.min", "15.0f" );
	ork::reflect::AnnotatePropertyForEditor< SpinnyCamControllerData >( "Aper", "editor.range.max", "160.0f" );
}

SpinnyCamControllerData::SpinnyCamControllerData()
	: mfSpinRate(1.0f)
	, mfElevation(0.0f)
	, mfRadius(1.0f)
	, mfNear(1.0f)
	, mfFar(100.0f)
	, mfAper(45.0f)
{
}

SeqCamItemInstBase* SpinnyCamControllerData::CreateInst(ork::ent::Entity* pent) const
{
	SpinnyCamControllerInst* pret = new SpinnyCamControllerInst( *this, pent );
	return pret;
}

///////////////////////////////////////////////////////////////////////////////

SpinnyCamControllerInst::SpinnyCamControllerInst(const SpinnyCamControllerData& cd, ork::ent::Entity* pent)
	: SeqCamItemInstBase(cd)
	, mSCCD(cd)
	, mfPhase(0.0f)
{
}

void SpinnyCamControllerInst::DoUpdate(ent::SceneInst* psi)
{
	mfPhase += mSCCD.GetSpinRate()*psi->GetDeltaTime();
		
	mCameraData.Persp( mSCCD.GetNear(), mSCCD.GetFar(), mSCCD.GetAper() );
	
	float famp = mSCCD.GetRadius();
	float fx = sinf(mfPhase)*famp;
	float fy = -cosf(mfPhase)*famp;

	CVector3 eye(fx,mSCCD.GetElevation(),fy);
	CVector3 tgt(0.0f,0.0f,0.0f);
	CVector3 up(0.0f,1.0f,0.0f);
	mCameraData.Lookat( eye,tgt,up );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CurvyCamControllerData::Describe()
{
	ork::reflect::RegisterProperty( "Elevation", &CurvyCamControllerData::mfElevation );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Elevation", "editor.range.min", "-100.0f" );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Elevation", "editor.range.max", "100.0f" );

	ork::reflect::RegisterProperty( "Radius", &CurvyCamControllerData::mfRadius );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Radius", "editor.range.min", "-1000.0f" );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Radius", "editor.range.max", "1000.0f" );

	ork::reflect::RegisterProperty( "Angle", &CurvyCamControllerData::mfAngle );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Angle", "editor.range.min", "-100.0f" );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Angle", "editor.range.max", "100.0f" );

	ork::reflect::RegisterProperty( "Near", &CurvyCamControllerData::mfNear );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Near", "editor.range.min", "0.1f" );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Near", "editor.range.max", "1000.0f" );

	ork::reflect::RegisterProperty( "Far", &CurvyCamControllerData::mfFar );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Far", "editor.range.min", "1.0f" );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Far", "editor.range.max", "10000.0f" );

	ork::reflect::RegisterProperty( "Aper", &CurvyCamControllerData::mfAper );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Aper", "editor.range.min", "15.0f" );
	ork::reflect::AnnotatePropertyForEditor< CurvyCamControllerData >( "Aper", "editor.range.max", "160.0f" );

	ork::reflect::RegisterProperty( "RadiusCurve", & CurvyCamControllerData::RadiusCurveAccessor );

}

CurvyCamControllerData::CurvyCamControllerData()
	: mfAngle(0.0f)
	, mfElevation(0.0f)
	, mfRadius(1.0f)
	, mfNear(1.0f)
	, mfFar(100.0f)
	, mfAper(45.0f)
{
}

SeqCamItemInstBase* CurvyCamControllerData::CreateInst(ork::ent::Entity* pent) const
{
	CurvyCamControllerInst* pret = new CurvyCamControllerInst( *this, pent );
	return pret;
}

///////////////////////////////////////////////////////////////////////////////

CurvyCamControllerInst::CurvyCamControllerInst(const CurvyCamControllerData& cd, ork::ent::Entity* pent)
	: SeqCamItemInstBase(cd)
	, mCCCD(cd)
	, mfPhase(0.0f)
{
}

void CurvyCamControllerInst::DoUpdate(ent::SceneInst* psi)
{
	mfPhase += mCCCD.GetAngle()*psi->GetDeltaTime();
		
	mCameraData.Persp( mCCCD.GetNear(), mCCCD.GetFar(), mCCCD.GetAper() );
	
	float famp = mCCCD.GetRadius();
	float fx = sinf(mfPhase)*famp;
	float fy = -cosf(mfPhase)*famp;

	CVector3 eye(fx,mCCCD.GetElevation(),fy);
	CVector3 tgt(0.0f,0.0f,0.0f);
	CVector3 up(0.0f,1.0f,0.0f);
	mCameraData.Lookat( eye,tgt,up );
}

}}
