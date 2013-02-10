////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/orktool_pch.h>
#include <ork/math/plane.h>
#include <orktool/filter/gfx/meshutil/meshutil.h>
#include <ork/math/collision_test.h>
#include <ork/math/sphere.h>

#include <ork/reflect/DirectObjectMapPropertyType.hpp>
#include <ork/kernel/orklut.hpp>

///////////////////////////////////////////////////////////////////////////////

INSTANTIATE_TRANSPARENT_RTTI(ork::MeshUtil::Light, "MeshUtilLight");
INSTANTIATE_TRANSPARENT_RTTI(ork::MeshUtil::LightContainer, "MeshUtilLightContainer");
INSTANTIATE_TRANSPARENT_RTTI(ork::MeshUtil::AmbientLight, "MeshUtilAmbientLight");
INSTANTIATE_TRANSPARENT_RTTI(ork::MeshUtil::DirLight, "MeshUtilDirLight");
INSTANTIATE_TRANSPARENT_RTTI(ork::MeshUtil::PointLight, "MeshUtilPointLight");

template class ork::orklut<ork::PoolString,ork::MeshUtil::Light*>;

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace MeshUtil {
///////////////////////////////////////////////////////////////////////////////

void LightContainer::Describe()
{
	ork::reflect::RegisterMapProperty( "Lights", & LightContainer::mLights );

}

///////////////////////////////////////////////////////////////////////////////

void Light::Describe()
{
	ork::reflect::RegisterProperty( "Color", & Light::mColor );
	ork::reflect::RegisterProperty( "WorldMatrix", & Light::mWorldMatrix );
	ork::reflect::RegisterProperty( "AffectsSpecular", & Light::mbSpecular );
	ork::reflect::RegisterProperty( "ShadowSamples", & Light::mShadowSamples );
	ork::reflect::RegisterProperty( "ShadowBlur", & Light::mShadowBlur );
	ork::reflect::RegisterProperty( "ShadowBias", & Light::mShadowBias );
	ork::reflect::RegisterProperty( "ShadowCaster", & Light::mbIsShadowCaster );	

	ork::reflect::AnnotatePropertyForEditor<Light>( "ShadowBias", "editor.range.min", "0.0" );
	ork::reflect::AnnotatePropertyForEditor<Light>( "ShadowBias", "editor.range.max", "10.0" );
	ork::reflect::AnnotatePropertyForEditor<Light>( "ShadowSamples", "editor.range.min", "1" );
	ork::reflect::AnnotatePropertyForEditor<Light>( "ShadowSamples", "editor.range.max", "256.0" );
	ork::reflect::AnnotatePropertyForEditor<Light>( "ShadowBlur", "editor.range.min", "0" );
	ork::reflect::AnnotatePropertyForEditor<Light>( "ShadowBlur", "editor.range.max", "1.0" );

}

///////////////////////////////////////////////////////////////////////////////

void DirLight::Describe()
{
}

///////////////////////////////////////////////////////////////////////////////

void AmbientLight::Describe()
{
}

///////////////////////////////////////////////////////////////////////////////

void PointLight::Describe()
{
	ork::reflect::RegisterProperty( "Falloff", & PointLight::mFalloff );
	ork::reflect::RegisterProperty( "Radius", & PointLight::mRadius );
}

///////////////////////////////////////////////////////////////////////////////

bool PointLight::AffectsSphere( const CVector3& center, float radius ) const
{
	float dist = ( mWorldMatrix.GetTranslation()-center).Mag();
	float combinedradii = (radius+mRadius);

//	orkprintf( "PointLight::AffectsSphere point<%f %f %f> center<%f %f %f>\n",
//				mWorldPosition.GetX(), mWorldPosition.GetY(), mWorldPosition.GetZ(),
//				center.GetX(), center.GetY(), center.GetZ() );

	//float crsq = combinedradii; //(combinedradii*combinedradii);
	return (dist<combinedradii);
}

///////////////////////////////////////////////////////////////////////////////

bool PointLight::AffectsAABox( const AABox& aab ) const
{
	return CollisionTester::SphereAABoxTest( Sphere( mWorldMatrix.GetTranslation(),mRadius), aab );
}

///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////
