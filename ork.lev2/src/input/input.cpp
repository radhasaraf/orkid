////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/lev2/input/input.h>

#if defined(WII)
#include <ork/lev2/input/CInputDeviceWii.h>
#elif defined( _PS2 )
#include <ork/lev2/input/inputps2.h>
#elif defined( _XBOX )
#include <ork/lev2/input/CInputDeviceXInput.h>
#elif defined( _WIN32 )
#include <ork/lev2/input/CInputDeviceDX.h>
#include <ork/lev2/input/CInputDeviceXInput.h>
#include <ork/lev2/input/CInputDeviceKeyboard.h>
#elif defined( IX ) || defined( _CYGWIN )
#include <ork/lev2/input/CInputDeviceIX.h>
#elif defined( ORK_OSX )
#include <ork/lev2/input/CInputDeviceOSX.h>
#include <ork/lev2/input/CInputDeviceKeyboard.h>
#endif

namespace ork { namespace lev2 {

///////////////////////////////////////////////////////////////////////////////

RawInputKey InputMap::MapInput( const MappedInputKey& inkey ) const
{
	orkmap<MappedInputKey,RawInputKey>::const_iterator it = mInputMap.find( inkey );
	return (it==mInputMap.end()) ? RawInputKey(inkey.mKey) : it->second;
}

///////////////////////////////////////////////////////////////////////////////

void InputMap::Set(ork::lev2::MappedInputKey inch, ork::lev2::RawInputKey outch)
{
	mInputMap[ inch ] = outch;
}

///////////////////////////////////////////////////////////////////////////////

InputMap InputState::gInputMap;

///////////////////////////////////////////////////////////////////////////////

InputState::InputState()
{
	for( int ival=0; ival<KMAX_TRIGGERS; ival++ )
	{
		LastPressureValues[ival] = 0;
		PressureValues[ival] = 0;
		PressureThresh[ival] = 16;
		TriggerDown[ival] = false;
		TriggerUp[ival] = false;
		TriggerState[ival] = false;
	}
}

///////////////////////////////////////////////////////////////////////////////

bool InputState::IsDown( MappedInputKey mapped, const InputMap& InputMap ) const
{
	//RawInputKey raw = InputMap.MapInput( mapped );
	int index = int(mapped.mKey);
	return TriggerState[index];
}

///////////////////////////////////////////////////////////////////////////////

bool InputState::IsUpEdge( MappedInputKey mapped, const InputMap& InputMap ) const
{
	RawInputKey raw = InputMap.MapInput( mapped );
	int index = int(mapped.mKey);
	return TriggerUp[index];
}

///////////////////////////////////////////////////////////////////////////////

bool InputState::IsDownEdge( MappedInputKey mapped, const InputMap& InputMap ) const
{
	RawInputKey raw = InputMap.MapInput( mapped );
	int index = int(mapped.mKey);
	OrkAssert( index < KMAX_TRIGGERS );
	return TriggerDown[index];
}

///////////////////////////////////////////////////////////////////////////////

F32 InputState::GetPressure( MappedInputKey mapped, const InputMap& InputMap ) const
{
	RawInputKey raw = InputMap.MapInput( mapped );
	const F32 frecip = 1.0f/127.0f;
	int index = int(raw.mKey);
	OrkAssert( index < KMAX_TRIGGERS );
	S8 uval = PressureValues[ index ];
	F32 fval = frecip * (F32) uval;
	return F32(fval);
}

///////////////////////////////////////////////////////////////////////////////

void InputState::SetPressure( RawInputKey ch, S8 uVal )
{

	int index = int(ch.mKey);
	OrkAssert( index < KMAX_TRIGGERS );
	S8 Thresh = PressureThresh[ index ];
	bool newstate = (uVal>Thresh);
	PressureValues[ index ] = uVal;
	TriggerState[index] = newstate;
}

S8 InputState::GetPressureRaw(  int ch ) const
{
	return(PressureValues[ ch ]);
}

void InputState::SetPressureRaw( int ch, S8 uVal )
{
	PressureValues[ ch ] = uVal;
}

void InputState::BeginCycle()
{
}

void InputState::Clear(int index)
{
	OrkAssert( index < KMAX_TRIGGERS );

	if(index >=0)
		TriggerState[index] = 0;
	else {
		for( int index=0; index<KMAX_TRIGGERS; index++ )
		{
			TriggerState[index] = 0;
		}
	}
}

void InputState::EndCycle()
{
	for( int index=0; index<KMAX_TRIGGERS; index++ )
	{
		bool newstate = TriggerState[index];

		S8 Thresh = PressureThresh[ index ];
		S8 OldVal = LastPressureValues[ index ];

		bool oldstate = (OldVal>Thresh);
		if( (false==oldstate) && (true==newstate) ) // key on
		{
			TriggerDown[ index ] = true;
			TriggerUp[ index ] = false;
			//orkprintf( "KEYON<%d> %x\n", index,(void *) this );
		}
		else if( (true==oldstate) && (false==newstate) ) // key off
		{
			TriggerDown[ index ] = false;
			TriggerUp[ index ] = true;
			//orkprintf( "KEYOFF<%d> %x\n", index,(void *) this );
		}
		else
		{
			TriggerDown[ index ] = false;
			TriggerUp[ index ] = false;
		}

		LastPressureValues[index] = PressureValues[index];
	}
}

CInputManager::CInputManager() : NoRttiSingleton<CInputManager>()
{
	CreateInputDevices();
}

void CInputManager::Poll( void )
{
	size_t inumdevices = GetRef().mvpInputDevices.size();
	for (size_t i = 0; i <inumdevices ;i++)
	{
		CInputDevice* pdevice = GetRef().mvpInputDevices[i];
		pdevice->Input_Poll();
	}
}

void CInputManager::ClearAll()
{
	for (size_t i = 0; i < GetRef().mvpInputDevices.size();i++)
		GetRef().mvpInputDevices[i]->RefInputState().Clear(-1);
}

void CInputManager::SetRumble(bool mode)
{
	for (size_t i = 0; i < GetRef().mvpInputDevices.size();i++)
		GetRef().mvpInputDevices[i]->SetMasterRumbleEnabled(mode);
}


const CInputDevice* CInputManager::GetInputDevice(unsigned int id) const
{
	int inumdevs = GetRef().mvpInputDevices.size();
	if( 0 == inumdevs ) return nullptr;
	return mvpInputDevices[id%inumdevs];
}

CInputDevice* CInputManager::GetInputDevice(unsigned int id)
{
	int inumdevs = GetRef().mvpInputDevices.size();
	if( 0 == inumdevs ) return nullptr;
	return mvpInputDevices[id%inumdevs];
}

void CInputManager::CreateInputDevices()
{
#if defined(IX)
		CInputDeviceIX *pref = new CInputDeviceIX();
		//pref->SetUserIndex(0);
		CInputManager::GetRef().AddDevice(pref);
		CInputManager::GetRef().mvpKeyboardInputDevice = pref;
#elif defined(_WIN32) || defined(_XBOX)
	{
		CInputDeviceXInput *pref = new CInputDeviceXInput();
		pref->SetUserIndex(0);
		CInputManager::GetRef().AddDevice(pref);
		pref = new CInputDeviceXInput();
		pref->SetUserIndex(1);
		CInputManager::GetRef().AddDevice(pref);
		pref = new CInputDeviceXInput();
		pref->SetUserIndex(2);
		CInputManager::GetRef().AddDevice(pref);
		pref = new CInputDeviceXInput();
		pref->SetUserIndex(3);
		CInputManager::GetRef().AddDevice(pref);
	}
#endif

#if (defined(_WIN32) && !defined(_XBOX))
	CInputDX::GetRef();
#endif

#if ! defined(_XBOX)
	//CInputDevice *pref = new CInputDeviceKeyboard();
	//CInputManager::GetRef().AddDevice(pref);
	//CInputManager::GetRef().mvpKeyboardInputDevice = pref;
#endif
}

void CInputDevice::Activate()
{
	if(mConnectionStatus == CONN_STATUS_CONNECTED || mConnectionStatus == CONN_STATUS_INSERTED)
		mConnectionStatus = CONN_STATUS_ACTIVE;
}

void CInputDevice::Deactivate()
{
	if(mConnectionStatus == CONN_STATUS_ACTIVE)
		mConnectionStatus = CONN_STATUS_CONNECTED;
}

bool CInputDevice::IsDisconnected() const
{
	return !IsConnected();
}

bool CInputDevice::IsConnected() const
{
	return mConnectionStatus == CONN_STATUS_ACTIVE || mConnectionStatus == CONN_STATUS_CONNECTED
		|| mConnectionStatus == CONN_STATUS_INSERTED;
}

bool CInputDevice::IsActive() const
{
	return mConnectionStatus == CONN_STATUS_ACTIVE;
}

void CInputDevice::Connect()
{
	if(IsDisconnected())
		mConnectionStatus = CONN_STATUS_INSERTED;
	else if(mConnectionStatus == CONN_STATUS_INSERTED)
		mConnectionStatus = CONN_STATUS_CONNECTED;
}




void CInputDevice::Disconnect()
{
	if(IsConnected())
		mConnectionStatus = CONN_STATUS_REMOVED;
	else if(mConnectionStatus == CONN_STATUS_REMOVED)
		mConnectionStatus = CONN_STATUS_DISCONNECTED;
}

void CInputDevice::SetInputMap( EMappedTriggerNames inch, ERawTriggerNames outch )
{
	InputState::RefGlobalInputMap().Set( inch, outch );
}

//e TWEAK you need to do this RUNBLE STUB functions any device can do what they want here
void CInputDevice::RumbleClear() 
{
}
void CInputDevice::RumbleTrigger(int amounmt) 
{
}




} } // ork::lev2
