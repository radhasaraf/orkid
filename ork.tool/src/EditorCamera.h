////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef _ORK_ENT_EDITORCAMERA_H_
#define _ORK_ENT_EDITORCAMERA_H_

#include <pkg/ent/entity.h>
#include <pkg/ent/component.h>
#include <pkg/ent/componenttable.h>
#include <ork/math/TransformNode.h>
#include <ork/lev2/gfx/renderer.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/lev2/gfx/camera/cameraman.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 { class XgmModel; class GfxMaterial3DSolid; } }
///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace ent {

///////////////////////////////////////////////////////////////////////////////

class EditorCamArchetype : public Archetype
{
	RttiDeclareConcrete( EditorCamArchetype, Archetype );

	/*virtual*/ void DoStartEntity(SceneInst* psi, const CMatrix4 &world, Entity *pent ) const {}
	/*virtual*/ void DoCompose(ork::ent::ArchComposer& composer);

public:

	EditorCamArchetype();

};

///////////////////////////////////////////////////////////////////////////////

class EditorCamControllerData : public ent::ComponentData
{
	RttiDeclareConcrete( EditorCamControllerData, ent::ComponentData );

	lev2::CCamera_persp*					mPerspCam;

public:

	virtual ent::ComponentInst* CreateComponent(ent::Entity* pent) const;

	EditorCamControllerData();
	const lev2::CCamera* GetCamera() const { return mPerspCam; }
	ork::Object* CameraAccessor() { return mPerspCam; }

};

///////////////////////////////////////////////////////////////////////////////

class EditorCamControllerInst : public ent::ComponentInst
{
	RttiDeclareAbstract( EditorCamControllerInst, ent::ComponentInst );

	const EditorCamControllerData&			mCD;
	
	virtual void DoUpdate(ent::SceneInst* sinst);

public:
	const EditorCamControllerData&	GetCD() const { return mCD; }

	EditorCamControllerInst( const EditorCamControllerData& cd, ork::ent::Entity* pent );
	bool DoLink(SceneInst *psi);
	bool DoStart(SceneInst *psi, const CMatrix4 &world);

};

///////////////////////////////////////////////////////////////////////////////

} }

#endif
