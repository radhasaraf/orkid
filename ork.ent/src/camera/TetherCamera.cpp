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
///////////////////////////////////////////////////////////////////////////////
#include <pkg/ent/scene.h>
#include <pkg/ent/entity.h>
#include <pkg/ent/entity.hpp>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <pkg/ent/event/MeshEvent.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/reflect/AccessorObjectPropertyType.hpp>
#include <ork/reflect/DirectObjectPropertyType.hpp>
#include <ork/reflect/DirectObjectMapPropertyType.hpp>
#include <ork/gfx/camera.h>
///////////////////////////////////////////////////////////////////////////////
#include "TetherCamera.h"
#include <ork/kernel/string/string.h>
///////////////////////////////////////////////////////////////////////////////
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::TetherCamArchetype, "TetherCamArchetype" );
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::TetherCamControllerData, "TetherCamControllerData");
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::TetherCamControllerInst, "TetherCamControllerInst");
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////////

void TetherCamArchetype::DoCompose(ork::ent::ArchComposer& composer)
{
	composer.Register<ork::ent::TetherCamControllerData>();
}

///////////////////////////////////////////////////////////////////////////////

void TetherCamArchetype::Describe()
{
}

///////////////////////////////////////////////////////////////////////////////

TetherCamArchetype::TetherCamArchetype()
{
}

///////////////////////////////////////////////////////////////////////////////

void TetherCamControllerData::Describe()
{
	ork::reflect::RegisterProperty( "Target", &TetherCamControllerData::mTarget );
	//ork::reflect::RegisterProperty( "Eye", &TetherCamControllerData::mEye );
	ork::reflect::RegisterProperty( "EyeUp", &TetherCamControllerData::mEyeUp );
	ork::reflect::RegisterProperty( "EyeOffset", &TetherCamControllerData::mEyeOffset );
	ork::reflect::RegisterProperty( "TargetOffset", &TetherCamControllerData::mTgtOffset );
	ork::reflect::RegisterProperty( "Aperature", &TetherCamControllerData::mfAperature );
	ork::reflect::RegisterProperty( "Near", &TetherCamControllerData::mfNear );
	ork::reflect::RegisterProperty( "Far", &TetherCamControllerData::mfFar );
	ork::reflect::RegisterProperty( "ApproachSpeed", &TetherCamControllerData::mApproachSpeed );

	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("ApproachSpeed", "editor.range.min", "0.1" );
	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("ApproachSpeed", "editor.range.max", "10.0" );

	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("Aperature", "editor.range.min", "1.0" );
	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("Aperature", "editor.range.max", "150.0" );

	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("Near", "editor.range.min", "0.1" );
	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("Near", "editor.range.max", "10000.0" );

	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("Far", "editor.range.min", "1.0" );
	reflect::AnnotatePropertyForEditor< TetherCamControllerData >("Far", "editor.range.max", "100000.0" );
}

///////////////////////////////////////////////////////////////////////////////

TetherCamControllerData::TetherCamControllerData()
	: mfAperature(45.0f)
	, mfNear(1.0f)
	, mfFar(500.0f)
	, mApproachSpeed(1.0f)
{
}

///////////////////////////////////////////////////////////////////////////////

ent::ComponentInst* TetherCamControllerData::createComponent(ent::Entity* pent) const
{
	return new TetherCamControllerInst( *this, pent );
}

///////////////////////////////////////////////////////////////////////////////

void TetherCamControllerInst::Describe()
{
}

///////////////////////////////////////////////////////////////////////////////

TetherCamControllerInst::TetherCamControllerInst(const TetherCamControllerData& occd, Entity* pent )
	: ComponentInst( & occd, pent )
	, mCD( occd )
	, mpTarget(0)
	//, mpEye(0)
{
	mCameraData.Persp( 0.1f, 1.0f, 45.0f );
	mCameraData.Lookat( fvec3(0.0f,0.0f,0.0f), fvec3(0.0f,0.0f,1.0f), fvec3(0.0f,1.0f,0.0f) );

	printf( "OCCI<%p> camdat<%p> l2cam<%p>\n", this, & mCameraData, mCameraData.getEditorCamera() );
}

///////////////////////////////////////////////////////////////////////////////

bool TetherCamControllerInst::DoLink(Simulation *psi)
{
	mpTarget = psi->FindEntity(mCD.GetTarget());
	//mpEye = psi->FindEntity(mCD.GetEye());
	return true;
}

bool TetherCamControllerInst::DoStart(Simulation *psi, const fmtx4 &world)
{
	if( GetEntity() )
	{
		const ent::EntData& ED = GetEntity()->GetEntData();
		PoolString name = ED.GetName();
		std::string Name = CreateFormattedString( "%s", name.c_str() );
 		psi->setCameraData( AddPooledString(Name.c_str()), & mCameraData );


	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////

void TetherCamControllerInst::DoUpdate( Simulation* psi )
{
	float fdt = psi->GetDeltaTime();

	fvec3 cam_UP = fvec3(0.0f,1.0f,0.0f);
	fvec3 cam_EYE = fvec3(0.0f,0.0f,0.0f);
	fvec3 eye_up = mCD.GetEyeUp();

	if( GetEntity() && mpTarget )
	{
		DagNode& dnodeEYE = GetEntity()->GetDagNode();
		TransformNode& t3dEYE = dnodeEYE.GetTransformNode();
		fmtx4 mtxEYE = t3dEYE.GetTransform().GetMatrix();
		auto eye_base = mtxEYE.GetTranslation();
		cam_EYE = eye_base+mCD.GetEyeOffset();
		cam_UP = mtxEYE.GetYNormal();

		if( eye_up.Mag() )
			cam_UP = eye_up.Normal();

		//////////

		DagNode& dnodeTGT = mpTarget->GetDagNode();
		TransformNode& t3dTGT = dnodeTGT.GetTransformNode();
		fmtx4 mtxTGT = t3dTGT.GetTransform().GetMatrix();
		fvec3 cam_TGT = mtxTGT.GetTranslation();

		float fnear = mCD.GetNear();
		float ffar = mCD.GetFar();
		float faper = mCD.GetAperature();

		//////////

		fvec3 DELT = (cam_TGT-cam_EYE);
		fvec3 N = DELT.Normal();
		float dist = DELT.Mag();

		//////////

		fvec3 DELT2 = (cam_TGT-eye_base);
		auto new_eye = eye_base+DELT2*mCD.GetApproachSpeed()*fdt;
		t3dEYE.GetTransform().SetPosition( new_eye );

		//////////

		mCameraData.Persp( fnear, ffar, faper );
		mCameraData.Lookat( cam_EYE, cam_TGT, cam_UP );

		//orkprintf( "ocam eye<%f %f %f>\n", cam_EYE.GetX(), cam_EYE.GetY(), cam_EYE.GetZ() );
		//orkprintf( "ocam tgt<%f %f %f>\n", cam_TGT.GetX(), cam_TGT.GetY(), cam_TGT.GetZ() );
		//orkprintf( "ocam dir<%f %f %f>\n", N.GetX(), N.GetY(), N.GetZ() );

		//psi->setCameraData( AddPooledLiteral("game1"), & mCameraData );
	}
}

///////////////////////////////////////////////////////////////////////////////
}}
