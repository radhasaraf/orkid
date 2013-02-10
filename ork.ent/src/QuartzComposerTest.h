////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#if defined(ORK_OSX) && ! defined(_ORK_ENT_QUARTZCOMPOSERTEST_H_)
#define _ORK_ENT_QUARTZCOMPOSERTEST_H_

///////////////////////////////////////////////////////////////////////////////

#include <ork/orkstl.h>
#include <ork/rtti/RTTI.h>
#include <ork/object/Object.h>
#include <ork/object/ObjectClass.h>
#include <ork/lev2/gfx/renderable.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/kernel/tempstring.h>
#include <ork/kernel/mutex.h>
#include <ork/kernel/any.h>
#include <pkg/ent/componentfamily.h>
#include <pkg/ent/component.h>
#include <pkg/ent/drawable.h>
#include <ork/gfx/camera.h>
#include <ork/kernel/objc.h>

//#define GLEW_STATIC
//#include <gl/glew.h>
#include <OpenGL/gl3.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////////

class QuartzComposerData : public ComponentData
{
	RttiDeclareConcrete(QuartzComposerData, ComponentData);

public:

	QuartzComposerData();
	//~QuartzComposerData();
	
	virtual ComponentInst *CreateComponent(Entity *pent) const;
	
	const ork::file::Path& GetCompositionPath() const { return mCompositionPath; }
	float GetStartTime() const { return mfStartTime; }
	float GetEndTime() const { return mfEndTime; }
	float GetFPS() const { return miFPS; }

private:

	ork::file::Path mCompositionPath;
	int				miFPS;
	float			mfStartTime;
	float			mfEndTime;
};

///////////////////////////////////////////////////////////////////////////////
struct QuartzComposerDrawable;

class QuartzComposerInst : public ComponentInst
{
	RttiDeclareAbstract(QuartzComposerInst, ComponentInst);

public:

	QuartzComposerInst(const QuartzComposerData &data, Entity *pent);
	~QuartzComposerInst();

	void DoUpdate( ork::ent::SceneInst* psi );		// virtual 
	bool DoLink(ork::ent::SceneInst *psi);			// virtual
	bool DoNotify(const ork::event::Event *event);	// virtual
	
	const QuartzComposerData& mCD;
	
	void UpdateFBO();
	
	Objc::Object				mQCRenderer;
	GLuint						mFBO;
	GLuint						mDBO;
	GLuint						mTEX;
	int							miW;
	int							miH;
	int							miFrame;
	float						mfUpdateTime;
	QuartzComposerDrawable*		mpQuartzComposerDrawable;
};

///////////////////////////////////////////////////////////////////////////////

class QuartzComposerArchetype : public Archetype
{
	RttiDeclareConcrete( QuartzComposerArchetype, Archetype );

	/*virtual*/ void DoStartEntity(SceneInst* psi, const CMatrix4 &world, Entity *pent ) const {}
	/*virtual*/ void DoCompose(ork::ent::ArchComposer& composer);

public:

	QuartzComposerArchetype();

};

///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////

#endif
