////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/thread.h>

#include <ork/lev2/input/input.h>
#include <ork/lev2/input/CInputDeviceIX.h>

#define USE_TUIO

#if defined(USE_TUIO)
#include "TuioListener.h"
#include "TuioClient.h"
#include <functional>
#endif

#if defined( _IOS )
#elif defined( IX )
    #include <ork/lev2/qtui/qtui.h>
#elif defined(_LINUX)
    #include <GL/glx.h>
    #include <Qt/QX11Info>
    #define XK_LATIN1
    #define XK_MISCELLANY
    #include <X11/keysymdef.h>
#endif
namespace ork { namespace lev2
{

#if defined(USE_TUIO)

using namespace TUIO;

typedef std::function<void(TuioCursor*)> on_tuio_cur_t;

struct TuioInputReader : public TuioListener {

	on_tuio_cur_t mOnDown;
	on_tuio_cur_t mOnUpdate;
	on_tuio_cur_t mOnUp;

	TuioInputReader()
		: mOnDown(nullptr)
		, mOnUpdate(nullptr)
		, mOnUp(nullptr)
	{

	}

	void addTuioObject(TuioObject *tobj) {}
	void updateTuioObject(TuioObject *tobj) {}
	void removeTuioObject(TuioObject *tobj) {}
	void  refresh(TuioTime frameTime) {
		//std::cout << "refresh " << frameTime.getTotalMilliseconds() << std::endl;
	}



	void addTuioCursor(TuioCursor *tcur) {
		if( mOnDown )
			mOnDown(tcur);
	}

	void updateTuioCursor(TuioCursor *tcur) {
		if( mOnUpdate )
			mOnUpdate(tcur);
	}

	void removeTuioCursor(TuioCursor *tcur) {
		if( mOnUp )
			mOnUp(tcur);
	}


};

#endif

///////////////////////////////////////////////////////////////////////////////

static float lanay = 0.0f;
static float ranay = 0.0f;
int lpid = -1;
int rpid = -1;

CInputDeviceIX::CInputDeviceIX() 
	: CInputDevice()
{


	printf("CREATED IX INPUTDEVICE\n" );
	/*OrkSTXMapInsert( mInputMap, 'W', (int) ETRIG_RAW_JOY0_LDIG_UP );	
	OrkSTXMapInsert( mInputMap, 'A', (int) ETRIG_RAW_JOY0_LDIG_LEFT );	
	OrkSTXMapInsert( mInputMap, 'D', (int) ETRIG_RAW_KEY_RIGHT );	
	OrkSTXMapInsert( mInputMap, 'S', (int) ETRIG_RAW_KEY_DOWN );	
	OrkSTXMapInsert( mInputMap, ETRIG_RAW_KEY_LEFT, (int) ETRIG_RAW_KEY_LEFT );	
	OrkSTXMapInsert( mInputMap, ETRIG_RAW_KEY_UP, (int) ETRIG_RAW_KEY_UP );	
	OrkSTXMapInsert( mInputMap, ETRIG_RAW_KEY_RIGHT, (int) ETRIG_RAW_KEY_RIGHT );	
	OrkSTXMapInsert( mInputMap, ETRIG_RAW_KEY_DOWN, (int) ETRIG_RAW_KEY_DOWN );*/

#if defined(USE_TUIO)

	OrkSTXMapInsert( mInputMap, ETRIG_RAW_JOY0_LANA_YAXIS, (int) ETRIG_RAW_JOY0_LANA_YAXIS );	
	OrkSTXMapInsert( mInputMap, ETRIG_RAW_JOY0_RANA_YAXIS, (int) ETRIG_RAW_JOY0_RANA_YAXIS );	

	auto thr = new ork::Thread("CInputDeviceIX");

	thr->start( [=]()
	{
        TuioInputReader reader;

        reader.mOnDown = [=](TuioCursor *tcur)
		{	
			auto id = tcur->getCursorID();
			auto x = tcur->getX();
			auto y = tcur->getY();

			if( x<0.2f ) // left stick
			{
				lpid = id;
			}
			else if( x>0.8f ) // right stick
			{
				rpid = id;
			}

	        //std::cout << "add cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY() << std::endl;
		};
        reader.mOnUpdate = [=](TuioCursor *tcur)
		{	
			auto id = tcur->getCursorID();
			auto x = tcur->getX();
			auto y = tcur->getY();

			if( id==lpid ) // left stick
			{
				lanay=1.0f-y;
			}
			else if( id==rpid ) // right stick
			{
				ranay=1.0f-y;
			}

	        //std::cout << "set cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY()
	        //                        << " " << tcur->getMotionSpeed() << " " << tcur->getMotionAccel() << " " << std::endl;

		};
        reader.mOnUp = [=](TuioCursor *tcur)
		{	
			auto id = tcur->getCursorID();

			if( id==lpid ) // left stick
			{
				lanay=0.0f;
				lpid = -1;

			}
			else if( id==rpid ) // right stick
			{
				ranay=0.0f;
				rpid = -1;
			}

	        //std::cout << "del cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ")" << std::endl;
		};

        TuioClient client(3333);
        client.addTuioListener(&reader);
        client.connect(true);

    });

#endif

}


CInputDeviceIX::~CInputDeviceIX()
{
}

///////////////////////////////////////////////////////////////////////////////

void CInputDeviceIX::Input_Init(void)
{

    return ;
}

void CInputDeviceIX::Input_Poll()
{
	//printf( "POLL IXID\n");
	mConnectionStatus = CONN_STATUS_ACTIVE;

	InputState &inpstate = RefInputState();

	inpstate.BeginCycle();

	for( const auto& item : mInputMap )
	{
		char k = item.first;
		char v = item.second;
		int ist = int(CSystem::IsKeyDepressed(k))*127;
		//printf( "KEY<%d> ST<%d>\n", int(k), ist );
		inpstate.SetPressure( v, ist );
	}


	inpstate.SetPressure(ETRIG_RAW_JOY0_LANA_YAXIS,lanay*127.0f);
	inpstate.SetPressure(ETRIG_RAW_JOY0_RANA_YAXIS,ranay*127.0f);

	inpstate.EndCycle();

   return ;

}





void CInputDeviceIX::Input_Configure()
{

    return ;
}




} }

