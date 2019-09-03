////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////


#include <ork/pch.h>
#include <ork/util/hotkey.h>
#include <ork/util/Context.hpp>
#include <ork/kernel/string/PoolString.h>
#include <ork/kernel/thread.h>
#include <ork/application/application.h>
#include <ork/file/path.h>
#include <ork/file/fileenv.h>
#include <ork/stream/FileInputStream.h>
#include <ork/stream/FileOutputStream.h>
#include <ork/stream/StringInputStream.h>
#include <ork/reflect/serialize/XMLDeserializer.h>
#include <ork/reflect/serialize/XMLSerializer.h>
#include <ork/reflect/DirectObjectMapPropertyType.hpp>
#include <ork/reflect/RegisterProperty.h>

#include <fcntl.h>
#if defined(LINUX)
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#endif


INSTANTIATE_TRANSPARENT_RTTI( ork::HotKey, "HotKey" );
INSTANTIATE_TRANSPARENT_RTTI( ork::HotKeyConfiguration, "HotKeyConfiguration" );
INSTANTIATE_TRANSPARENT_RTTI( ork::HotKeyManager, "HotKeyManager" );

namespace ork {

#if defined( LINUX )

static bool ix_kb_state[256];

void* ix_kb_thread( void* pctx )
{
	SetCurrentThreadName( "IxKeyboardThread" );

	for(int i=0; i<256; i++)
		ix_kb_state[i] = false;

	//int fd = open("/dev/input/event10", O_RDONLY );
	int fd = open("/dev/input/by-path/platform-i8042-serio-0-event-kbd", O_RDONLY );

	if( fd<0 )
		while(1) usleep(1<<20);

	struct input_event ev[64];

	std::queue<U8> byte_queue;
	while(1)
	{
		int rd = read(fd,ev,sizeof(input_event)*64);
		if( rd>0 )
		{
			U8* psrc = (U8*) ev;
			for( int i=0; i<rd; i++ )
			{
				byte_queue.push(psrc[i]);
			}
		}
		while(byte_queue.size()>=sizeof(input_event))
		{
			input_event oev;
			u8* pdest = (U8*) & oev;
			for( int i=0; i<sizeof(input_event); i++ )
			{
				pdest[i]=byte_queue.front();
				byte_queue.pop();
			}
			if( oev.type == 1 )
			{
				bool bkdown = (oev.value == 1);
				bool bkup = (oev.value == 0);
				bool bksta = (0!=oev.value);
				printf( "ev typ<%d> cod<%d> val<%d>\n", oev.type, oev.code, oev.value );

				switch(oev.code)
				{

					case 17: // w
						ix_kb_state['W'] = bksta;
						break;
					case 30: // a
						ix_kb_state['A'] = bksta;
						break;
					case 31: // s
						ix_kb_state['S'] = bksta;
						break;
					case 32: // d
						ix_kb_state['D'] = bksta;
						break;
					case 24: // o
						ix_kb_state['O'] = bksta;
						break;
					case 25: // p
						ix_kb_state['P'] = bksta;
						break;
					case 33: // f
						ix_kb_state['F'] = bksta;
						break;
					//case 105: // L
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_LEFT] = bksta;
					//	break;
					//case 106: // R
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_RIGHT] = bksta;
					//	break;
					//case 103: // U
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_UP] = bksta;
					//	break;
					//case 108: // D
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_DOWN] = bksta;
					//	break;
					//case 42: // lshift
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_LSHIFT] = bksta;
					//	break;
					//case 29: // lctrl
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_LCTRL] = bksta;
					//	break;
					//case 56: // lalt
					//	ix_kb_state[ork::lev2::ETRIG_RAW_KEY_LALT] = bksta;
					//	break;
//					case 28: // return
//						ix_kb_state[ork::lev2::ETRIG_RAW_KEY_ENTER] = bksta;
//						break;
					// 0 11
					// 9 10
					// [ 26
					default:
						break;
				}
			}
		}
//		printf( "rd<%d>\n", rd );
	}
}

#endif



bool OldSchool::IsKeyDepressed(int ch)
{
#if 1
	return false;
#elif defined(ORK_OSX)
	KeyMap theKeys;
	GetKeys ( theKeys );
	int iword = ch/32;
	int ibit = ch%32;
	u32 imask = 1<<ibit;
	u32* uword = reinterpret_cast<u32*>(&theKeys[0]);
	bool rv = false; //(*uword&imask);
	// f == 0x00000008
	switch( ch )
	{
		case 'W':
			rv = (uword[0]&0x00002000);
			//printf( "O: %d\n", int(rv) );
			break;
		case 'A':
			rv = (uword[0]&0x00000001);
			//printf( "O: %d\n", int(rv) );
			break;
		case 'S':
			rv = (uword[0]&0x00000002);
			//printf( "O: %d\n", int(rv) );
			break;
		case 'D':
			rv = (uword[0]&0x00000004);
			//printf( "O: %d\n", int(rv) );
			break;
		case 'O':
			rv = (uword[0]&0x80000000);
			//printf( "O: %d\n", int(rv) );
			break;
		case 'P':
			rv = (uword[1]&8);
			//printf( "P: %d\n", int(rv) );
			break;
		case 'F':
			rv = (uword[0]&8);
			//printf( "F: %d\n", int(rv) );
			break;
		case ork::lev2::ETRIG_RAW_KEY_LEFT:
			rv = (uword[3]&0x08000000);
			//printf( "F: %d\n", int(rv) );
			break;
		case ork::lev2::ETRIG_RAW_KEY_RIGHT:
			rv = (uword[3]&0x10000000);
			//printf( "F: %d\n", int(rv) );
			break;
		case ork::lev2::ETRIG_RAW_KEY_UP:
			rv = (uword[3]&0x40000000);
			//printf( "F: %d\n", int(rv) );
			break;
		case ork::lev2::ETRIG_RAW_KEY_DOWN:
			rv = (uword[3]&0x20000000);
			//printf( "F: %d\n", int(rv) );
			break;

		default:
			// curs up : uword[3] 40000000
			// curs lf : uword[3] 08000000
			// curs rt : uword[3] 10000000
			// curs dn : uword[3] 20000000



			//printf( "K: %08x\n", int(uword[3]) );
			break;
	}

	//if( false==rv )
		//printf( "Isdepressed<%d> rv<%d> KEYS<%08x:%08x:%08x:%08x>\n", ch, int(rv), theKeys[0], theKeys[1], theKeys[2], theKeys[3] );
	return rv;
#elif defined( IX )
	static pthread_t kb_thread = 0;
	if( 0 == kb_thread )
	{
		int istat = pthread_create(&kb_thread, NULL, ix_kb_thread, (void*) 0);
	}
	return ix_kb_state[ch];
#elif defined( WIN32 ) && ! defined( _XBOX )
	switch(ch)
	{
		case ork::lev2::ETRIG_RAW_KEY_LALT:
			ch = 0x10e;
			break;
		case ork::lev2::ETRIG_RAW_KEY_RALT:
			ch = 0x10f;
			break;
		case ork::lev2::ETRIG_RAW_KEY_LCTRL:
			ch = 0x10c;
			break;
		case ork::lev2::ETRIG_RAW_KEY_RCTRL:
			ch = 0x10d;
			break;
		case ork::lev2::ETRIG_RAW_KEY_LSHIFT:
			ch = 0x105;
			break;
		case ork::lev2::ETRIG_RAW_KEY_RSHIFT:
			ch = 0x10A;
			break;
		case ork::lev2::ETRIG_RAW_KEY_LEFT:
			ch = VK_LEFT;
			break;
		case ork::lev2::ETRIG_RAW_KEY_RIGHT:
			ch = VK_RIGHT;
			break;
		case ork::lev2::ETRIG_RAW_KEY_UP:
			ch = VK_UP;
			break;
		case ork::lev2::ETRIG_RAW_KEY_DOWN:
			ch = VK_DOWN;
			break;
		default:
			if( (ch>='A') && (ch<='Z') )
			{
				//ch += (int('a')-int('A'));
			}
			break;
	}

	return (GetAsyncKeyState(ch) & 0x8000) == 0x8000;
#else
	return false;
#endif
}

} // namespace ork {


namespace ork {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void HotKey::Describe()
{
	ork::reflect::RegisterProperty("KeyCode", &HotKey::miKeyCode);
	ork::reflect::RegisterProperty("Alt", &HotKey::mbAlt);
	ork::reflect::RegisterProperty("Ctrl", &HotKey::mbCtrl);
	ork::reflect::RegisterProperty("Shift", &HotKey::mbShift);
	ork::reflect::RegisterProperty("LMB", &HotKey::mbLeftMB);
	ork::reflect::RegisterProperty("MMB", &HotKey::mbMiddleMB);
	ork::reflect::RegisterProperty("RMB", &HotKey::mbRightMB);
}

///////////////////////////////////////////////////////////////////////////////

HotKey::HotKey()
	: miKeyCode(-1)
	, mbAlt( false )
	, mbCtrl( false )
	, mbShift( false )
	, mbLeftMB( false )
	, mbMiddleMB( false )
	, mbRightMB( false )
{
}

///////////////////////////////////////////////////////////////////////////////

HotKey::HotKey( const char* keycode )
	: miKeyCode(-1)
	, mbAlt( false )
	, mbCtrl( false )
	, mbShift( false )
	, mbLeftMB( false )
	, mbMiddleMB( false )
	, mbRightMB( false )
{

	mbAlt = strstr(keycode,"alt")!=0;
	mbCtrl = strstr(keycode,"ctrl")!=0;
	mbShift = strstr(keycode,"shift")!=0;
	mbLeftMB = strstr(keycode,"lmb")!=0;
	mbMiddleMB = strstr(keycode,"mmb")!=0;
	mbRightMB = strstr(keycode,"rmb")!=0;

	const char* plast=strrchr( keycode, '-' );

	const char* kcstr = (plast==0) ? keycode : plast+1;

	size_t ilen = strlen(kcstr);

	switch( ilen )
	{
		case 1:
		{
			char ch = kcstr[0];

			#if 0 //defined(IX)
			if( ch>='f' )
			{
				miKeyCode = 3;
			}
			else if( ch>='a' && ch<='z' )
			{
				miKeyCode = int(ch-'a')+'A';
			}

			#else
			if( ch>='a' && ch<='z' )
			{
				miKeyCode = int(ch-'a')+'A';
			}
			else if( ch>='A' && ch<='Z' )
			{
				miKeyCode = int(ch-'A')+'A';
			}
			else if( ch>='0' && ch<='9' )
			{
				miKeyCode = 26+int(ch-'0');
			}
			else switch( ch )
			{
				case '[':
				case ']':
				case '\\':
				case '-':
				case '=':
				case ';':
				case '\'':
				case '/':
				case ',':
				case '.':
				case '`':
					miKeyCode = int(ch);
					break;
			}
			#endif
			break;
		}
		case 2: // f keys
		case 3:
		{	//

			break;
		}
		case 4: // numeric keypad
		{
			const char* pnum=strstr( keycode, "num" );
			if( pnum )
			{
				int idigit = keycode[3]-'0';
				#if defined(_WIN32)
				miKeyCode = VK_NUMPAD0+idigit;
				#endif
			}
			else if( strstr( keycode, "none" )!=0 )
			{
				miKeyCode=-2;
			}

			break;
		}
	}


}

FixedString<32> HotKey::GetAcceleratorCode() const
{
	FixedString<32> rval;
	FixedString<32> rv_c;
	FixedString<32> rv_a;
	FixedString<32> rv_s;
	FixedString<32> rv_ch;

	if( mbCtrl )
	{
		rv_c.set( "Ctrl" );
	}
	if( mbShift )
	{
		if( mbCtrl ) rv_s.set( "+Shift" ); else rv_s.set( "Shift" );
	}
	if( mbAlt )
	{
		if( mbCtrl||mbShift ) rv_a.set( "+Alt" ); else rv_a.set( "Alt" );
	}
	{
		if( mbCtrl||mbShift||mbAlt ) rv_ch.format( "+%c",char(miKeyCode) ); else rv_ch.format( "%c",char(miKeyCode) );
	}
	rval.format( "%s%s%s%s", rv_c.c_str(), rv_s.c_str(), rv_a.c_str(),rv_ch.c_str() );

	return rval;

}

///////////////////////////////////////////////////////////////////////////////

boost::Crc64 HotKey::GetHash() const
{
	boost::Crc64 rval;
	boost::crc64_init(rval);
	boost::crc64_compute( rval, & miKeyCode, sizeof( miKeyCode ) );
	boost::crc64_compute( rval, & mbAlt, sizeof( mbAlt ) );
	boost::crc64_compute( rval, & mbCtrl, sizeof( mbCtrl ) );
	boost::crc64_compute( rval, & mbShift, sizeof( mbShift ) );
	boost::crc64_compute( rval, & mbLeftMB, sizeof( mbLeftMB ) );
	boost::crc64_compute( rval, & mbMiddleMB, sizeof( mbMiddleMB ) );
	boost::crc64_compute( rval, & mbRightMB, sizeof( mbRightMB ) );
	boost::crc64_fin(rval);
	return rval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void HotKeyConfiguration::Describe()
{
	ork::reflect::RegisterMapProperty("HotKeys", &HotKeyConfiguration::mHotKeys);
	ork::reflect::AnnotatePropertyForEditor< HotKeyConfiguration >("HotKeys", "editor.factorylistbase", "HotKey" );

}

///////////////////////////////////////////////////////////////////////////////

HotKeyConfiguration::HotKeyConfiguration()
{
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyConfiguration::Default()
{
	AddHotKey( "copy", "ctrl-c" );
	AddHotKey( "paste", "ctrl-v" );
	AddHotKey( "open", "ctrl-o" );
	AddHotKey( "save", "ctrl-s" );
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyConfiguration::AddHotKey( const char* actionname, const HotKey& hkey )
{
	ork::PoolString psname = ork::AddPooledString( actionname );
	if( IsHotKeyPresent( hkey ) )
	{
	}
	else
	{
		mHotKeys.AddSorted( psname, new HotKey( hkey ) );
		mHotKeysUsed.insert(hkey.GetHash());
	}
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyConfiguration::RemoveHotKey(  const char* actionname )
{
	ork::PoolString psname = ork::AddPooledString( actionname );
	//if( IsHotKeyPresent( hkey ) )
	{
	//	mHotKeysUsed.erase(hkey.GetHash());
	}
}

bool HotKeyConfiguration::PostDeserialize(reflect::IDeserializer &) // virtual
{
	for( orklut<PoolString,ork::Object*>::const_iterator it=mHotKeys.begin(); it!=mHotKeys.end(); it++ )
	{
		ork::Object* pobj = it->second;
		HotKey* pkey = rtti::autocast( pobj );
		OrkAssert( mHotKeysUsed.find( pkey->GetHash() ) == mHotKeysUsed.end() );
		mHotKeysUsed.insert( pkey->GetHash() );
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyConfiguration::RemoveHotKey( const HotKey& hkey )
{
}

///////////////////////////////////////////////////////////////////////////////

bool HotKeyConfiguration::IsHotKeyPresent( const HotKey& hkey ) const
{
	boost::Crc64 hash = hkey.GetHash();
	return (mHotKeysUsed.find(hash) != mHotKeysUsed.end());
}

HotKey* HotKeyConfiguration::GetHotKey( PoolString ps ) const
{	orklut<PoolString,ork::Object*>::const_iterator it = mHotKeys.find( ps );
	if( it != mHotKeys.end() )
	{	HotKey* pkey = rtti::autocast( it->second );
		return pkey;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

HotKeyManager HotKeyManager::gHotKeyManager;

void HotKeyManager::Describe()
{
	ork::reflect::RegisterMapProperty("Configurations", &HotKeyManager::mHotKeyConfigurations);
	ork::reflect::AnnotatePropertyForEditor< HotKeyManager >("Configurations", "editor.factorylistbase", "HotKeyConfiguration" );
}

///////////////////////////////////////////////////////////////////////////////

HotKeyManager::HotKeyManager()
{
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyManager::AddHotKeyConfiguration( const char* configname, const HotKeyConfiguration& hkc )
{	ork::PoolString psname = ork::AddPooledString( configname );
	orklut<PoolString,ork::Object*>::const_iterator it=mHotKeyConfigurations.find(psname);
	if( it==mHotKeyConfigurations.end() )
	{	HotKeyConfiguration* dupe = new HotKeyConfiguration(hkc);
		mHotKeyConfigurations.AddSorted( psname, dupe );
		mCurrent = dupe;
	}
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyManager::RemoveHotKeyConfiguration( const char* actionname )
{
}

///////////////////////////////////////////////////////////////////////////////

HotKeyConfiguration* HotKeyManager::GetConfiguration( const char* actionname )
{
	return mCurrent;
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyManager::SetCurrentConfiguration( const char* configname )
{	ork::PoolString psname = ork::AddPooledString( configname );
	orklut<PoolString,ork::Object*>::const_iterator it=mHotKeyConfigurations.find(psname);
	if( it!=mHotKeyConfigurations.end() )
	{	mCurrent = rtti::autocast( it->second );
	}
}

///////////////////////////////////////////////////////////////////////////////

const HotKey& HotKeyManager::GetHotKey( const char* actionname )
{	ork::PoolString psname = ork::AddPooledString( actionname );
	if( GetRef().mCurrent )
	{	HotKey* hkey = GetRef().mCurrent->GetHotKey( psname );
		if( hkey )
		{
			return *hkey;
		}
	}
	static const HotKey gnull;
	return gnull;
}

///////////////////////////////////////////////////////////////////////////////

static const char* HotKeyFileName = "hotkeys.ork";

void HotKeyManager::Save()
{	ork::stream::FileOutputStream istream(HotKeyFileName);
	ork::reflect::serialize::XMLSerializer ser(istream);
	GetClass()->Description().SerializeProperties(ser, this);
}

///////////////////////////////////////////////////////////////////////////////

void HotKeyManager::Load()
{	mHotKeyConfigurations.clear();
	file::Path pth(HotKeyFileName);
	if( ork::FileEnv::GetRef().DoesFileExist( pth ) )
	{	ork::stream::FileInputStream istream(pth.c_str());
		ork::reflect::serialize::XMLDeserializer deser(istream);
		GetClass()->Description().DeserializeProperties(deser, this);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool HotKeyManager::IsDepressed( const HotKey& hkey )
{	if( GetRef().mCurrent )
	{	int ikc = hkey.miKeyCode;
		if( ikc>=0 )
		{
#if defined(IX)
			return OldSchool::IsKeyDepressed(ikc);
#elif defined(_XBOX)
			return false;
#elif defined(_WIN32)

			bool bks = (GetAsyncKeyState(ikc) & 0x8000) == 0x8000;
			bool bkalt = hkey.mbAlt?(GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000:true;
			bool bkshf = hkey.mbShift?(GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000:true;
			bool bkctl = hkey.mbCtrl?(GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000:true;
			bool bmodok		= bkalt&&bkshf&&bkctl;
			return (bmodok&&bks);
#endif
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool HotKeyManager::IsDepressed( PoolString pact )
{	if( GetRef().mCurrent )
	{	HotKey* hkey = GetRef().mCurrent->GetHotKey( pact );
		if( hkey )
		{
			return IsDepressed( *hkey );
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool HotKeyManager::IsDepressed( const char* pact )
{
	//if( 0 == strcmp( pact, "camera_bwd" ) )
	//{
	//	int i = 0;
	//}
	//
	return IsDepressed( ork::AddPooledString( pact ) );
}

///////////////////////////////////////////////////////////////////////////////

FixedString<32> HotKeyManager::GetAcceleratorCode(const char* action)
{
	FixedString<32> rval;

	ork::PoolString psname = ork::AddPooledString( action );
	if( GetRef().mCurrent )
	{	HotKey* hkey = GetRef().mCurrent->GetHotKey( psname );
		if( hkey )
		{
			rval = hkey->GetAcceleratorCode();
		}
	}
	return rval;
}

///////////////////////////////////////////////////////////////////////////////

bool HotKeyManager::PreDeserialize(reflect::IDeserializer &) // virtual
{
	mHotKeyConfigurations.clear();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool HotKeyManager::PostDeserialize(reflect::IDeserializer &) // virtual
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////

}
