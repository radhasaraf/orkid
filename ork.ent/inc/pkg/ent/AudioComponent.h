////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef ORK_ENT_AudioComponent_H
#define ORK_ENT_AudioComponent_H

///////////////////////////////////////////////////////////////////////////////

#include <pkg/ent/entity.h>
#include <ork/math/cvector3.h>
#include <ork/math/TransformNode.h>
#include <ork/lev2/aud/audiodevice.h>
#include <pkg/ent/dataflow.h>

namespace ork { class CCameraData; }

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class AudioEffectComponentData;
class AudioEffectComponentInst;

///////////////////////////////////////////////////////////////////////////////

class AudioEffectPlayDataBase : public ork::Object
{
	RttiDeclareAbstract(AudioEffectPlayDataBase, ork::Object);

public:

	void									SetAECD( const AudioEffectComponentData* aecd ) { mData=aecd; }
	const AudioEffectComponentData*			GetData() const { return mData; }	
	int										GetMutexPriority() const { return miMutexPriority; }
	const ork::PoolString&					GetMutexGroup() const { return mMutexGroup; }
	const ork::PoolString&					GetSubMixGroup() const { return mSubMixGroup; }
	float									GetMaxEmitterDist() const { return mMaxEmitterDist; }
	bool									IsEmitter() const { return mbEmitter; }
	const ork::MultiCurve1D&				GetAttenuationCurve() const { return mAttenuationCurve; }

protected:

	AudioEffectPlayDataBase();

private:

	ork::Object* AttenCurveAccessor() { return & mAttenuationCurve; }

	const AudioEffectComponentData*		mData;	
	ork::PoolString						mMutexGroup;
	ork::PoolString						mSubMixGroup;
	int									miMutexPriority;
	float								mMaxEmitterDist;
	ork::MultiCurve1D					mAttenuationCurve;
	bool								mbEmitter;
};

///////////////////////////////////////////////////////////////////////////////

class AudioMultiEffectPlayInstItemBase;

class AudioMultiEffectPlayDataItemBase : public ork::Object
{
	RttiDeclareAbstract(AudioMultiEffectPlayDataItemBase, ork::Object);

public:

	AudioMultiEffectPlayDataItemBase();

	const ork::PoolString&			GetBankName() const { return mBankName; } 
	const ork::PoolString&			GetProgramName() const { return mProgramName; }
	int								GetNote() const { return miNote; }
	int								GetVelocity() const { return miVelocity; }
	float							GetPan() const { return mPan; }

	virtual AudioMultiEffectPlayInstItemBase* CreateInst() const = 0;

private:

	ork::PoolString					mBankName;
	ork::PoolString					mProgramName;
	int								miNote;
	int								miVelocity;
	float							mPan;

};

///////////////////////////////////////////////////////////////////////////////

class AudioMultiEffectPlayDataItemFixed : public AudioMultiEffectPlayDataItemBase
{
	RttiDeclareConcrete(AudioMultiEffectPlayDataItemFixed, AudioMultiEffectPlayDataItemBase);
public:
	AudioMultiEffectPlayDataItemFixed();
private:
	AudioMultiEffectPlayInstItemBase* CreateInst() const ; // virtual
};

///////////////////////////////////////////////////////////////////////////////

class AudioMultiEffectPlayDataItemModular : public AudioMultiEffectPlayDataItemBase
{
	RttiDeclareConcrete(AudioMultiEffectPlayDataItemModular, AudioMultiEffectPlayDataItemBase );
public:
	AudioMultiEffectPlayDataItemModular();
	const ork::lev2::AudioGraph&	GetTemplate() const { return mControlGraph; }
	ork::Object* TemplateAccessor() { return & mControlGraph; }
	int GetNumVoices() const { return miNumVoices; }
private:
	bool DoNotify(const ork::event::Event *event);
	ork::lev2::AudioGraph			mControlGraph;
	int								miNumVoices;
	AudioMultiEffectPlayInstItemBase* CreateInst() const ; // virtual
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class AudioMultiEffectPlayInstItemBase
{
public:
	AudioMultiEffectPlayInstItemBase( const AudioMultiEffectPlayDataItemBase& data )
		: mItemBaseData( data )
	{
	}
	virtual ~AudioMultiEffectPlayInstItemBase() {}
	ork::lev2::AudioInstrumentPlayback* Play(AudioEffectComponentInst* aeci, ork::lev2::AudioIntrumentPlayParam& param); 
	void Stop(AudioEffectComponentInst* aeci, ork::lev2::AudioInstrumentPlayback*pb); 
	const AudioMultiEffectPlayDataItemBase& GetData() const { return mItemBaseData; }
private:
	virtual ork::lev2::AudioInstrumentPlayback* DoPlay(AudioEffectComponentInst* aeci, ork::lev2::AudioIntrumentPlayParam& param) = 0;
	virtual void DoStop(AudioEffectComponentInst* aeci, ork::lev2::AudioInstrumentPlayback*pb) = 0;
	const AudioMultiEffectPlayDataItemBase& mItemBaseData;
};

class AudioMultiEffectPlayInstItemFixed : public AudioMultiEffectPlayInstItemBase
{
public:
	AudioMultiEffectPlayInstItemFixed(const AudioMultiEffectPlayDataItemFixed& itemdata);
private:
	const AudioMultiEffectPlayDataItemFixed&	mItemData;
	ork::lev2::AudioInstrumentPlayback* DoPlay(AudioEffectComponentInst* aeci, ork::lev2::AudioIntrumentPlayParam& param); // virtual
	void DoStop(AudioEffectComponentInst* aeci, ork::lev2::AudioInstrumentPlayback*pb); // virtual
	void Stop(AudioEffectComponentInst* aeci, ork::lev2::AudioInstrumentPlayback*pb); 
};

class AudioMultiEffectPlayInstItemModular : public AudioMultiEffectPlayInstItemBase
{
public:
	AudioMultiEffectPlayInstItemModular( const AudioMultiEffectPlayDataItemModular& itemdata );
private:
	const AudioMultiEffectPlayDataItemModular&	mItemData;
	ork::lev2::AudioGraphPool					mGraphPool;
	ork::lev2::AudioInstrumentPlayback* DoPlay(AudioEffectComponentInst* aeci, ork::lev2::AudioIntrumentPlayParam& param); // virtual
	void DoStop(AudioEffectComponentInst* aeci, ork::lev2::AudioInstrumentPlayback*pb); // virtual
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum EMultiSoundSelectMode
{
	EMS_RANDOM = 0,
	EMS_CYCLE,
};

///////////////////////////////////////////////////////////////////////////////

class AudioMultiEffectPlayData : public AudioEffectPlayDataBase
{
	RttiDeclareConcrete(AudioMultiEffectPlayData, AudioEffectPlayDataBase);

public:

	AudioMultiEffectPlayData();
	~AudioMultiEffectPlayData();

	const ork::orklut< ork::PoolString, AudioMultiEffectPlayDataItemBase* >&	GetSubSoundMap() const { return mSubSoundMap; }
	EMultiSoundSelectMode														GetSelectMode() const { return mSelectMode; }
	bool																		GetEnable3D() const { return mEnable3D; }

private:

	ork::orklut< ork::PoolString, AudioMultiEffectPlayDataItemBase* >	mSubSoundMap;
	EMultiSoundSelectMode												mSelectMode;
	bool																mEnable3D;
};

///////////////////////////////////////////////////////////////////////////////

class AudioEffectComponentInst;

class AudioMultiEffectPlayInst : public ork::Object
{
	RttiDeclareAbstract(AudioMultiEffectPlayInst, ork::Object);

public:

	const ork::lev2::AudioIntrumentPlayParam& GetParams() const { return mParam; }
	ork::lev2::AudioIntrumentPlayParam& GetParams() { return mParam; }

	AudioMultiEffectPlayInst( const AudioEffectComponentInst& aeci, const AudioMultiEffectPlayData& pdata );
	~AudioMultiEffectPlayInst();

	const AudioMultiEffectPlayData&	GetData() const { return mData; }

	ork::lev2::AudioInstrumentPlayback* Play( AudioEffectComponentInst* aeci, int inote, int ivel, const ork::TransformNode* pnode );
	virtual ork::lev2::AudioInstrumentPlayback* DoPlay( AudioEffectComponentInst* aeci, ork::lev2::AudioIntrumentPlayParam& param );
	void Stop(AudioEffectComponentInst* aeci,ork::lev2::AudioInstrumentPlayback*pb);
	virtual void DoStop(AudioEffectComponentInst* aeci,ork::lev2::AudioInstrumentPlayback*pb);
	
	const ork::orklut< ork::PoolString, AudioMultiEffectPlayInstItemBase* >&	GetSubSoundMap() const { return mSubSoundMap; }

private:

	ork::orklut< ork::PoolString, AudioMultiEffectPlayInstItemBase* >	mSubSoundMap;
	const AudioMultiEffectPlayData&										mData;
	const AudioEffectComponentInst&										mAECI;
	ork::lev2::AudioIntrumentPlayParam									mParam;
	int																	mLastPlayIndex;
	int																	mRange;
	int																	mSoundMapSize;

};

///////////////////////////////////////////////////////////////////////////////

class AudioEffectComponentData : public ork::ent::ComponentData
{
	RttiDeclareConcrete(AudioEffectComponentData, ork::ent::ComponentData);

public:
	///////////////////////////////////////////////////////
	AudioEffectComponentData();
	~AudioEffectComponentData();
	virtual ork::ent::ComponentInst *CreateComponent(ork::ent::Entity *pent) const;
	///////////////////////////////////////////////////////

	AudioEffectPlayDataBase* GetPlayData(ork::PoolString ps) const;

	const ork::orklut<ork::PoolString,ork::lev2::AudioBank*>& GetBankMap() const { return mBankMap; }
	const ork::orklut<ork::PoolString,AudioEffectPlayDataBase*>& GetSoundMap() const { return mSoundMap; }
	const ork::PoolString& GetMutexGroups() const { return mMutexGroups; }

private:
	ork::orklut<ork::PoolString,ork::lev2::AudioBank*>		mBankMap;
	ork::orklut<ork::PoolString,AudioEffectPlayDataBase*>	mSoundMap;
	ork::PoolString											mMutexGroups;

	void DoRegisterWithScene( ork::ent::SceneComposer& sc );
};

///////////////////////////////////////////////////////////////////////////////

class AudioManagerComponentInst;

struct EmitterCtx
{
	AudioMultiEffectPlayInst*											mEmitter;
	bool																mbEmitterToggle;
	ork::lev2::AudioInstrumentPlayback*									mEmitterPB;
	int																	miEmitterSN;
	float																mMaxDist;
	const ork::MultiCurve1D*											mAttenuationCurve;

	EmitterCtx();
};

class AudioEffectComponentInst : public ork::ent::ComponentInst
{
	RttiDeclareAbstract(AudioEffectComponentInst, ork::ent::ComponentInst);

public:

	AudioEffectComponentInst( const AudioEffectComponentData& aecd, ork::ent::Entity* pent );
	~AudioEffectComponentInst();

	const AudioEffectComponentData& GetData() const { return mData; }
	void UpdateEmitter( const ork::CCameraData* camdat1, const ork::CCameraData* camdat2 );

	ork::lev2::AudioInstrumentPlayback* PlaySound( ork::PoolString soundname, const ork::TransformNode* pnode = nullptr );
	void StopSound( ork::PoolString soundname );

	ork::ent::DataflowRecieverComponentInst* GetDflowReciver() const { return mDflowRecv; }

private:
	AudioMultiEffectPlayInst* GetPlayInst(ork::PoolString ps) const;

	const AudioEffectComponentData&										mData;
	AudioManagerComponentInst*											mAmci;
	ork::orklut<ork::PoolString,AudioMultiEffectPlayInst*>				mSoundMap;
	orkvector<ork::lev2::AudioInstrumentPlayback*>						mPlaybacks;
	ork::ent::DataflowRecieverComponentInst*							mDflowRecv;
	const ork::TransformNode*											mXform;
	orkvector<EmitterCtx>												mEmitters;

	void DoUpdate(ork::ent::SceneInst *inst);
	ork::lev2::AudioInstrumentPlayback* PlaySoundEx( ork::PoolString soundname, int inote, int ivel, const ork::TransformNode* pnode );
	
	bool DoStart(ork::ent::SceneInst *psi, const ork::CMatrix4 &world); // virtual
	bool DoLink( ork::ent::SceneInst *psi );
	void DoStop(ork::ent::SceneInst *psi); // virtual

	bool DoNotify(const ork::event::Event *pev); // virtual

	void AddSound( const ork::PoolString& ps, AudioMultiEffectPlayInst* pib  );

	void AddPlayback( ork::lev2::AudioInstrumentPlayback* pb );
	void RemovePlayback( ork::lev2::AudioInstrumentPlayback* pb );
	void StopAllSounds();
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class AudioStreamComponentData : public ork::ent::ComponentData
{
	RttiDeclareConcrete(AudioStreamComponentData, ork::ent::ComponentData);

public:
	typedef ork::orklut<ork::PoolString, ork::lev2::AudioStream *> StreamMap;

	AudioStreamComponentData();

	virtual ork::ent::ComponentInst *CreateComponent(ork::ent::Entity *pent) const;

	const StreamMap& GetStreamMap() const { return mStreamMap; }
	StreamMap& GetStreamMap() { return mStreamMap; }

private:
	StreamMap mStreamMap;
};

///////////////////////////////////////////////////////////////////////////////

enum EAudioStreamXfadeType
{
	EXFTYP_6CH_LINEAR_BC = 0,
};

///////////////////////////////////////////////////////////////////////////////

struct AudioStreamLerpableParam
{
	bool							mbQuickGate;
	float							mfCurrent;
	float							mfPrevious;
	float							mfTarget;
	float							mfTransitionTime;

	AudioStreamLerpableParam();

	void TransitionTo( float flevel, float fseconds );
	void Update( float fdeltat );
};

struct AudioStreamInstItem
{	
	ork::lev2::AudioStreamPlayback*	mPlayback;
	ork::lev2::AudioStream*			mStream;
	EAudioStreamXfadeType			meCrossFadeType;

	AudioStreamLerpableParam		mParamCrossfade;
	AudioStreamLerpableParam		mParamVolume;

	AudioStreamInstItem();

	void CrossfadeTransitionTo( float flevel, float fseconds );
	void VolumeTransitionTo( float flevel, float fseconds );

	void Update( float fdeltatime );
};

///////////////////////////////////////////////////////////////////////////////

class AudioStreamComponentInst : public ork::ent::ComponentInst
{
	RttiDeclareAbstract(AudioStreamComponentInst, ork::ent::ComponentInst);

public:

	AudioStreamComponentInst( const AudioStreamComponentData& data, ork::ent::Entity* pent );

	const AudioStreamComponentData& GetData() const { return mData; }
	void Stop( ork::PoolString streamname );
private:
	void DoUpdate(ork::ent::SceneInst *inst);
	const AudioStreamComponentData& mData;


	void SetDebugLevel(bool on );
	void Play( ork::PoolString streamname );
	void SetCrossfade( ork::PoolString streamname, float fxfade, float fxtransitiontime );
	void SetVolume( ork::PoolString streamname, float fade, float transitiontime );

	ork::orklut<ork::PoolString, AudioStreamInstItem> mItems;

	bool DoStart(ork::ent::SceneInst *psi, const ork::CMatrix4 &world); // virtual
	void DoStop(ork::ent::SceneInst *psi); // virtual
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class AudioManagerComponentData : public ork::ent::SceneComponentData
{
	RttiDeclareConcrete(AudioManagerComponentData, ork::ent::SceneComponentData);

public:
	///////////////////////////////////////////////////////
	AudioManagerComponentData();
	ork::ent::SceneComponentInst* CreateComponentInst( ork::ent::SceneInst *pinst ) const; // virtual 
	///////////////////////////////////////////////////////
	const ork::lev2::AudioReverbProperties&	GetReverbProperties() const { return mReverbProperties; }

	float GetDistMin() const { return mfDistMin; }
	float GetDistMax() const { return mfDistMax; }
	float GetDistScale() const { return mfDistScale; }
	float GetDistAttenPower() const { return mfDistAttenPower; }

private:

	ork::Object*					ReverbAccessor() { return & mReverbProperties; }

	ork::lev2::AudioReverbProperties	mReverbProperties;

	float							mfDistScale;
	float							mfDistMin;
	float							mfDistMax;
	float							mfDistAttenPower;
};

///////////////////////////////////////////////////////////////////////////////

class AudioManagerComponentInst : public ork::ent::SceneComponentInst
{
	RttiDeclareAbstract(AudioManagerComponentInst, ork::ent::SceneComponentInst);

public:

	AudioManagerComponentInst( const AudioManagerComponentData& ascd, ork::ent::SceneInst *pinst );
	~AudioManagerComponentInst();

	void AddEmitter( AudioEffectComponentInst* mEmitter ) { mEmitters.push_back(mEmitter); }

	const AudioManagerComponentData& GetAmcd() const { return mAmcd; }

private:
	void DoUpdate(ork::ent::SceneInst *inst);
	void DoStop(ork::ent::SceneInst *psi);

	orkvector<AudioEffectComponentInst*>	mEmitters;
	const AudioManagerComponentData& mAmcd;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
}} // namespace ork::ent
///////////////////////////////////////////////////////////////////////////////
#endif
