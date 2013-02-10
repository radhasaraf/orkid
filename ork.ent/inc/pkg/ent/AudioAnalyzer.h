////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

// Some code in here from Apple's Core Audio Play Thru sample.
//   But according to the sample code's license,
//   if it is modified, and/or not in it's entirety, then I do not need the original license text....
//

#if defined(ORK_OSX) && ! defined(_PKG_ENT_AUDIOAnalysis_H)
#define _PKG_ENT_AUDIOAnalysis_H

#include <CoreServices/CoreServices.h>
#include <CoreMIDI/CoreMIDI.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <cstdlib>

namespace ork { namespace ent {

class AudioAnalysisComponentData;
class AudioAnalysisComponentInst;

///////////////////////////////////////////////////////////////////////////////

struct AudBufItem
{
	static const int kmaxframes = 4096;
	float mData[kmaxframes];
	int miNumFrames;
	
	AudBufItem() : miNumFrames(0) {}
};

struct AudBufSet
{
	static const int kmaxitems = 4;
	AudBufItem mItems[kmaxitems];
	int miNumItems;
	
	AudBufSet() : miNumItems(0) {}
};

struct AudMultiBuffer
{
	static const int kmaxmbuf = 8;
	
	int32_t miWriteIndex;
	int32_t miReadIndex;
	
	AudBufSet mMultiBuf[kmaxmbuf];

	AudBufSet& GetWriteBuffer();
	void GetReadBuffer(AudBufSet& output);
	
	AudMultiBuffer() : miWriteIndex(1), miReadIndex(0) {}
};

///////////////////////////////////////////////////////////////////////////////

class AudioDevice
{
public:
	AudioDevice() : mID(kAudioDeviceUnknown) { }
	AudioDevice(AudioDeviceID devid, bool isInput) { Init(devid, isInput); }

	void	Init(AudioDeviceID devid, bool isInput);
	
	bool	Valid() { return mID != kAudioDeviceUnknown; }
	
	void	SetBufferSize(UInt32 size);
	
	int		CountChannels();
	char *	GetName(char *buf, UInt32 maxlen);

public:
	AudioDeviceID					mID;
	bool							mIsInput;
	UInt32							mSafetyOffset;
	UInt32							mBufferSizeFrames;
	AudioStreamBasicDescription		mFormat;	
};

///////////////////////////////////////////////////////////////////////////////

class AudioDeviceList
{
public:
	struct Device
	{
		char			mName[64];
		AudioDeviceID	mID;
	};

	typedef std::vector<Device> DeviceList;

	AudioDeviceList(bool inputs);
	~AudioDeviceList();

	DeviceList &GetList() { return mDevices; }

protected:
	void		BuildList();
	void		EraseList();

	bool				mInputs;
	DeviceList			mDevices;
	
};

class CAPlayThrough;

class CAPlayThroughHost
{
public:
	CAPlayThroughHost(AudioDeviceID input,AudioAnalysisComponentInst*pAACI);
	~CAPlayThroughHost();
	
	void		CreatePlayThrough(AudioDeviceID input);
	void		DeletePlayThrough();
	bool		PlayThroughExists();
	
	OSStatus	Start();
	OSStatus	Stop();
	Boolean		IsRunning();

private:
	CAPlayThrough* GetPlayThrough() { return mPlayThrough; }

	void AddDeviceListeners(AudioDeviceID input);
	void RemoveDeviceListeners(AudioDeviceID input);
	
	void ResetPlayThrough();

	static OSStatus StreamListener(	AudioStreamID           inStream,
								UInt32                  inChannel,
								AudioDevicePropertyID   inPropertyID,
								void*                   inClientData);
										
private:
	CAPlayThrough *mPlayThrough;
	AudioAnalysisComponentInst*	mpAACI;
};


///////////////////////////////////////////////////////////////////////////////

class AudioAnalysisManagerComponentData : public ork::ent::SceneComponentData
{
	RttiDeclareConcrete(AudioAnalysisManagerComponentData, ork::ent::SceneComponentData);
public:
	///////////////////////////////////////////////////////
	AudioAnalysisManagerComponentData();
	ork::ent::SceneComponentInst* CreateComponentInst(ork::ent::SceneInst *pinst) const; // virtual 
	///////////////////////////////////////////////////////
	AudioDeviceList* GetAudioDeviceList() const { return mAudioDeviceList; }

private:

	std::map<AudioDeviceID,std::string>	mDeviceNames;
	std::string							mSelectedDevice;
	AudioDeviceList*					mAudioDeviceList;

};

///////////////////////////////////////////////////////////////////////////

class AudioAnalysisManagerComponentInst : public ork::ent::SceneComponentInst
{
	RttiDeclareAbstract(AudioAnalysisManagerComponentInst, ork::ent::ComponentInst);
public:
	AudioAnalysisManagerComponentInst( const AudioAnalysisManagerComponentData &data, ork::ent::SceneInst *pinst );

	AudioAnalysisComponentInst* GetAudioAnalysisComponentInst( int icidx ) const;
		
	const AudioAnalysisManagerComponentData& GetAAMCD() const { return mAAMCD; }
	
	void AddAACI( AudioAnalysisComponentInst* cci );

private:

	orkvector<AudioAnalysisComponentInst*>		mAACIs;
	const AudioAnalysisManagerComponentData&	mAAMCD;
};

///////////////////////////////////////////////////////////////////////////////

class AudioAnalysisComponentData : public ork::ent::ComponentData
{
	RttiDeclareConcrete(AudioAnalysisComponentData, ork::ent::ComponentData);

public:
	///////////////////////////////////////////////////////
	AudioAnalysisComponentData();
	virtual ork::ent::ComponentInst *CreateComponent(ork::ent::Entity *pent) const;
	///////////////////////////////////////////////////////
	void DoRegisterWithScene( ork::ent::SceneComposer& sc );

	int GetAudioInputDeviceID() const { return mAudioInputDeviceID; }

private:
	
	int				mAudioInputDeviceID;
	
};

///////////////////////////////////////////////////////////////////////////////

class AudioAnalysisComponentInst : public ork::ent::ComponentInst
{
	RttiDeclareAbstract(AudioAnalysisComponentInst, ork::ent::ComponentInst);

public:

	AudioAnalysisComponentInst( const AudioAnalysisComponentData &data, ork::ent::Entity *pent );
	~AudioAnalysisComponentInst();
	
	const AudioAnalysisComponentData& GetAnalysisData() const { return mAnalysisData; }

	void ControlMessage( int icontroller, float fV );
	float GetController( int icontroller ) const;
	void NoteOn( int iNOTE, float fV );
	void GetNote( int& outnote, float& outvel ) const;
	
	void StartMidi();
	void StopMidi();
	void SendMidiPacket(const MIDIPacketList* pktlist);
	static void MidiReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon);

	AudMultiBuffer& GetMultiBuffer() { return mMultiBuffer; }
	const AudMultiBuffer& GetMultiBuffer() const { return mMultiBuffer; }
	
private:
	
	const AudioAnalysisComponentData& mAnalysisData;
	bool DoLink(ork::ent::SceneInst *psi);
	void DoUnLink(SceneInst *psi);
	bool DoStart(ork::ent::SceneInst *inst, const ork::CMatrix4 &world);
	void DoUpdate(SceneInst *inst);

	orkmap<int,float> mControlValues;
	float	mfVelocity;
	int		miNote;
	float	mfTimeAccum;
	float	mfLastTime;
	
	MIDIPortRef		mInPort;
	MIDIPortRef		mOutPort;
	MIDIEndpointRef mInSource;
	MIDIClientRef	mMidiClient;
	int				mMidiInputDevice;
	AudioAnalysisManagerComponentInst*	mpAAMCI;
	CAPlayThroughHost*	mCoreAudioHost;
	AudMultiBuffer	mMultiBuffer;
};

///////////////////////////////////////////////////////////////////////////////

class AudioAnalysisArchetype : public Archetype
{
	RttiDeclareConcrete(AudioAnalysisArchetype, Archetype);
public:
	AudioAnalysisArchetype();
private:
	void DoCompose(ArchComposer& composer); // virtual
	void DoStartEntity(SceneInst*, const CMatrix4& mtx, Entity* pent ) const {}
};

///////////////////////////////////////////////////////////////////////////////
}} // namespace { ork { namespace ent {
#endif
