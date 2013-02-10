////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef ORK_ENT_RACINGLINEDATA_H
#define ORK_ENT_RACINGLINEDATA_H

#include <pkg/ent/entity.h>

namespace ork { namespace ent {
class PhysicsDebugger;
} }

namespace ork { namespace ent { namespace bullet {

class Track;

typedef enum
{
	RACINGLINEFLAG_NOSPAWN = (1<<0),
	RACINGLINEFLAG_BOOSTING = (1<<1),
	RACINGLINEFLAG_DRIFTING = (1<<2),
	RACINGLINEFLAG_MISSILE = (1<<3),
	RACINGLINEFLAG_MINE = (1<<4),
	RACINGLINEFLAG_SHOCKWAVE = (1<<5),
	RACINGLINEFLAG_SPECIAL = (1<<6),
	RACINGLINEFLAG_NONE = 0
} RacingLineFlags;

class RacingLineSample : public ork::Object
{
	RttiDeclareConcrete(RacingLineSample, ork::Object)
public:
	RacingLineSample() : mThrust(0.0f), mBrake(0.0f), mSteering(0.0f), mFlags(RACINGLINEFLAG_NONE) {}

	const ork::CMatrix4 &GetPosition() const { return mPosition; }
	void SetPosition(const ork::CMatrix4 &position) { mPosition = position; }

	float GetTime() const { return mTime; }
	void SetTime(float time) { mTime = time; }

	float GetThrust() const { return mThrust; }
	void SetThrust(float thrust) { mThrust = thrust; }

	float GetBrake() const { return mBrake; }
	void SetBrake(float brake) { mBrake = brake; }

	float GetSteering() const { return mSteering; }
	void SetSteering(float steering) { mSteering = steering; }

	bool isNoSpawn() const {return mFlags & RACINGLINEFLAG_NOSPAWN;}
	bool hasBoost() const {return mFlags & RACINGLINEFLAG_BOOSTING;}
	bool hasMissile() const {return mFlags & RACINGLINEFLAG_MISSILE;}
	bool hasMine() const {return mFlags & RACINGLINEFLAG_MINE;}
	bool hasShockwave() const {return mFlags & RACINGLINEFLAG_SHOCKWAVE;}
	bool hasSpecial() const {return mFlags & RACINGLINEFLAG_SPECIAL;}
	int getFlags() const {return mFlags;}
	void setFlags(int flags) {mFlags = (RacingLineFlags)flags;}
protected:
	ork::CMatrix4 mPosition;
	float mThrust;
	float mBrake;
	float mSteering;
	float mTime;
	RacingLineFlags mFlags;
};

class RacingLine : public ork::Object
{
	RttiDeclareConcrete(RacingLine, ork::Object)
public:
	RacingLine() : mTime(-1.0f) {}

	float GetTime() const { return mTime; }
	void SetTime(float time) { mTime = time; }

	const orklut<float, RacingLineSample *> &GetRacingLineSamples() const { return mRacingLineSamples; }
	orklut<float, RacingLineSample *> &GetRacingLineSamples() { return mRacingLineSamples; }
	~RacingLine();
protected:
	float mTime;

	// Racing Line Samples by Sector Progress
	orklut<float, RacingLineSample *> mRacingLineSamples;
};

class RacingLineData : public ork::ent::ComponentData
{
	RttiDeclareConcrete(RacingLineData, ork::ent::ComponentData)
public:
	const orklut<int, RacingLine *> &GetRacingLines() const { return mRacingLines; }
	orklut<int, RacingLine *> &GetRacingLines() { return mRacingLines; }

	const RacingLine *GetRacingLine(int index) const;
private:

	virtual ork::ent::ComponentInst *CreateComponent(ork::ent::Entity *pent) const;
	/*virtual*/ bool PostDeserialize(reflect::IDeserializer &);

	~RacingLineData();

	// Racing Lines by Racing Line Index
	orklut<int, RacingLine *> mRacingLines;
};

class RacingLineInst : public ork::ent::ComponentInst
{
	RttiDeclareAbstract(RacingLineInst, ork::ent::ComponentInst)
public:
	RacingLineInst(const RacingLineData &data, ork::ent::Entity *pent);

	const RacingLineData &GetData() const { return mData; }

	void Sample(int racing_line_index, float progress, RacingLineSample &sample,
		ork::CVector3 &racingLineDir) const;
private:

	/*virtual*/ void DoUpdate(ork::ent::SceneInst *sinst);
	/*virtual*/ bool DoLink(ork::ent::SceneInst *sinst);

	const RacingLineData &mData;

	const ork::ent::bullet::Track *mTrack;

	ork::ent::PhysicsDebugger *mPhysicsDebugger;
};

} } }

#endif // ORK_ENT_RACINGLINEDATA_H
