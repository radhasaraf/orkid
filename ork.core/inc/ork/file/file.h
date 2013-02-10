////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

///////////////////////////////////////////////////////////////////////////////

#include <ork/file/efileenum.h>

// These are not needed for this header, but remains for compatibility for other include file.h and
// expecting to get these class declarations.
#include <ork/file/fileenv.h>
#include <ork/file/filedev.h>
#include <ctype.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

struct QueueBuffer
{
	static const size_t kreadblocksize;
	static const size_t kmaxbuffersize;

	U8*	mData;
	size_t minumqueued;
	size_t mireadidx;
	size_t miwriteidx;

	size_t miReadFromQueue;

	QueueBuffer();
	~QueueBuffer();

	void Read( void* pwhere, size_t isize );
	void Queue( const void* pwhere, size_t isize );
	size_t GetWriteFree() const { return (kmaxbuffersize-GetNumBytesQueued()); }
	size_t GetNumBytesQueued() const; // { return minumqueued; }
	void Flush();
};

///////////////////////////////////////////////////////////////////////////////

class CFile
{
	//int					miFilePos; //current position the OS has us at
	size_t					miPhysicalPos;
	size_t					miUserPos; //current position user wants

public:

	QueueBuffer				mFifo;
	CFileDev*				mpDevice;
	file::Path				msFileName;
	EFileMode				meFileMode;
	size_t					miFileLen;
	FileH					mHandle;
    bool                    mbEnableBuffering;

	CFile( CFileDev* pdev = NULL );
	CFile( const char* sFileName, EFileMode eMode, CFileDev* pdev = NULL );
	CFile( const file::Path & sFileName, EFileMode eMode, CFileDev* pdev = NULL );
	~CFile();

	EFileErrCode OpenFile( const file::Path& sFileName, EFileMode eMode );
	EFileErrCode Open();
	EFileErrCode Close();
	EFileErrCode Load( void **filebuffer, size_t& size );

	EFileErrCode Read( void *pTo, size_t iSize );
	EFileErrCode Write( const void *pFrom, size_t iSize );
	EFileErrCode SeekFromStart( size_t iOffset );
	EFileErrCode SeekFromCurrent( size_t iOffset );
	EFileErrCode GetLength( size_t& riOffset );

	// Serializer functions
	template<class T> inline CFile & operator<<( const T & d );
	template<class T> inline CFile & operator>>( T & d );

	const file::Path& GetFileName( void ) { return msFileName; }

	bool IsOpen( void ) const;

	bool Reading( void ) { return bool(meFileMode & EFM_READ); }
	bool Writing( void ) { return bool(meFileMode & EFM_WRITE); }
	bool Ascii( void ) { return bool(meFileMode & EFM_ASCII); }
	bool Appending( void ) { return bool(meFileMode & EFM_APPEND); }
	bool Binary( void ) { return !Ascii(); }
	bool Fast( void ) { return bool(meFileMode & EFM_FAST); }

	size_t GetUserPos() const { return miUserPos; }
	void SetUserPos( size_t ip ) { miUserPos=ip; }

	size_t GetPhysicalPos() const { return miPhysicalPos; }
	void SetPhysicalPos( size_t ip ) { miPhysicalPos=ip; }

	size_t NumPhysicalRemaining() const { return miFileLen-miPhysicalPos; }
    
    bool IsBufferingEnabled() const { return mbEnableBuffering; }

};

///////////////////////////////////////////////////////////////////////////////

inline EFileErrCode CFile::Read( void *pTo, size_t iSize )
{
	return mpDevice->Read( *this, pTo, iSize );
}

///////////////////////////////////////////////////////////////////////////////

inline EFileErrCode CFile::Write( const void *pFrom, size_t iSize )
{
	return mpDevice->Write( *this, pFrom, iSize );
}

///////////////////////////////////////////////////////////////////////////////

inline EFileErrCode CFile::SeekFromStart( size_t iOffset )
{
	return mpDevice->SeekFromStart( *this, iOffset );
}

///////////////////////////////////////////////////////////////////////////////

inline EFileErrCode CFile::SeekFromCurrent( size_t iOffset )
{
	return mpDevice->SeekFromCurrent( *this, iOffset );
}

///////////////////////////////////////////////////////////////////////////////

inline EFileErrCode CFile::GetLength( size_t &riOffset )
{
	return mpDevice->GetLength( *this, riOffset );
}

///////////////////////////////////////////////////////////////////////////////

inline EFileErrCode CFile::Close( void )
{
	return mpDevice->CloseFile( *this );
}

///////////////////////////////////////////////////////////////////////////////

template<class T> CFile &
CFile::operator<<( const T & d )
{
	OrkAssert( !Reading() );

	if ( Ascii() )
	{
		//CStringStream strStream;
		//strStream << d;
		//CFileEnv::Write( *this, (void *)strStream.str().c_str(), strStream.str().length() * sizeof( char ) );
	}
	else
		mpDevice->Write( *this, (void *)&d, sizeof( T ) );

	return *this;
}

///////////////////////////////////////////////////////////////////////////////

template<class T> CFile &CFile::operator>>( T & d )
{
	OrkAssert( Reading() );
	mpDevice->Read( *this, (void *)&d, sizeof( T ) );
	return *this;
}

///////////////////////////////////////////////////////////////////////////////

#define FileReadObj( fil, item ) fil.Read( (void*) & item, sizeof(item) )
#define FileWriteObj( fil, item ) fil.Write( (void*) & item, sizeof(item) )
#define FileWriteStr( fil, item ) fil.Write( (void*) item, (strlen(item)+1) )

///////////////////////////////////////////////////////////////////////////////
// functors for transform

struct dos2unixpathsep
{	char operator() (char c)
	{
		if( c == '\\' ) c = '/';
		return c;
	}
};

///////////////////////////////////////////////////////////////////////////////

struct unix2dospathsep
{	char operator() (char c)
	{
		if( c == '/' ) c = '\\';
		return c;
	}
};

///////////////////////////////////////////////////////////////////////////////

struct pathtolower
{	char operator() (char c)
	{
#if defined(WII)
		c = char(std::tolower(c));
#else
		c = char(tolower(c));
#endif
		return c;
	}
};

///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
#define NativePathSep '\\'
#elif defined(_LINUX) || defined(_OSX)
#define NativePathSep '/'
#endif

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////
