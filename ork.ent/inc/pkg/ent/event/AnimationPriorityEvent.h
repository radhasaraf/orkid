////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef ORK_ENT_EVENT_ANIMATIONPRIORITY_H
#define ORK_ENT_EVENT_ANIMATIONPRIORITY_H

#include <ork/rtti/RTTI.h>
#include <ork/object/Object.h>

#include <ork/event/Event.h>

//#include <ork/application/application.h>

namespace ork { namespace ent { namespace event {

class AnimationPriority : public ork::event::Event
{
	RttiDeclareAbstract(AnimationPriority,ork::event::Event);
	
public:


	AnimationPriority(ork::PieceString name = "", int priority = 0);

	inline void SetName(ork::PoolString name) { mName = name; }
	inline ork::PoolString GetName() const { return mName; }

	inline void SetPriority(int priority) { mPriority = priority; }
	inline int GetPriority() const { return mPriority; }

private:

	ork::PoolString mName;
	int mPriority;

	virtual Object *Clone() const
	{
		return new AnimationPriority(mName,mPriority);
	}

};

} } } // ork::ent::event

#endif // ORK_ENT_EVENT_ANIMATIONPRIORITY_H
