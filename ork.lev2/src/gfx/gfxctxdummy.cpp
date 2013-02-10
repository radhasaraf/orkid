////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/file/file.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxctxdummy.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/gfx/shadman.h>

/////////////////////////////////////////////////////////////////////////
bool LoadIL(const ork::AssetPath& pth, ork::lev2::Texture *ptex);
/////////////////////////////////////////////////////////////////////////

INSTANTIATE_TRANSPARENT_RTTI(ork::lev2::GfxTargetDummy, "GfxTargetDummy")
namespace ork { namespace lev2 {

/////////////////////////////////////////////////////////////////////////

void DummyGfxTargetInit()
{
	GfxEnv::SetTargetClass(GfxTargetDummy::GetClassStatic());
}

/////////////////////////////////////////////////////////////////////////

bool DummyFxInterface::LoadFxShader( const AssetPath& pth, FxShader *pfxshader  )
{
	AssetPath assetname = pth;
	assetname.SetExtension( "fxml" );
	FxShader* shader = new FxShader;
	pfxshader->SetInternalHandle( 0 );
	printf( "DUMMYFX::LOADED<%s>\n", pth.c_str() );
	//bool bOK = LoadFxShader( shader );
	//OrkAssert(bOK);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

CMatrix4 DuMatrixStackInterface::Ortho( float left, float right, float top, float bottom, float fnear, float ffar )
{
	CMatrix4 mat;
	mat.Ortho( left, right, top, bottom, fnear, ffar );
	return  mat;
}

///////////////////////////////////////////////////////////////////////////////

DuFrameBufferInterface::DuFrameBufferInterface(GfxTarget& target )
	: FrameBufferInterface( target )
{
}

DuFrameBufferInterface::~DuFrameBufferInterface()
{
}

///////////////////////////////////////////////////////////////////////////////

GfxTargetDummy::~GfxTargetDummy()
{
}

///////////////////////////////////////////////////////////////////////////////

GfxTargetDummy::GfxTargetDummy()
	: GfxTarget()
	, mFbI( *this )
	, mMtxI( *this )
{
	DummyGfxTargetInit();
	static bool binit = true;

	if( true == binit )
	{
		binit = false;
		FxShader::RegisterLoaders( "shaders/dummy/", "fxml" );
	}
}

void GfxTargetDummy::InitializeContext( GfxWindow *pWin, CTXBASE* pctxbase )
{
}

void GfxTargetDummy::InitializeContext( GfxBuffer *pBuf )
{
}

void GfxTargetDummy::SetSize( int ix, int iy, int iw, int ih )
{
	miX=ix;
	miY=iy;
	miW=iw;
	miH=ih;
}

///////////////////////////////////////////////////////////////////////////////

void* DuGeometryBufferInterface::LockIB( IndexBufferBase& IdxBuf, int ibase, int icount )
{	if( 0 == IdxBuf.GetHandle() )
	{
		IdxBuf.SetHandle( (void*) std::malloc( IdxBuf.GetNumIndices() * IdxBuf.GetIndexSize()) );
	}
	char* pch = (char*) IdxBuf.GetHandle();
	return (void*) (pch+ibase);
}
void DuGeometryBufferInterface::UnLockIB( IndexBufferBase& IdxBuf)
{
}

const void* DuGeometryBufferInterface::LockIB( const IndexBufferBase& IdxBuf, int ibase, int icount )
{	if( 0 == IdxBuf.GetHandle() )
	{
		IdxBuf.SetHandle( (void*) std::malloc( IdxBuf.GetNumIndices() * IdxBuf.GetIndexSize()) );
	}
	const char* pch = (const char*) IdxBuf.GetHandle();
	return (const void*) (pch+ibase);
}
void DuGeometryBufferInterface::UnLockIB( const IndexBufferBase& IdxBuf)
{
}

void DuGeometryBufferInterface::ReleaseIB( IndexBufferBase& IdxBuf )
{
	std::free( IdxBuf.GetHandle() );
	IdxBuf.SetHandle(0);
}

void* DuGeometryBufferInterface::LockVB( VertexBufferBase& VBuf, int ibase, int icount )
{	OrkAssert( false == VBuf.IsLocked() );
	int iVBlen = VBuf.GetVtxSize()*VBuf.GetMax();
	if( 0 == VBuf.GetHandle() )
	{	void* pdata = std::malloc( iVBlen );
		//orkprintf( "DuGeometryBufferInterface::LockVB() malloc_vblen<%d>\n", iVBlen );
		VBuf.SetHandle( pdata );
	}
	VBuf.Lock();
	VBuf.Reset();
	return VBuf.GetHandle();
}

const void* DuGeometryBufferInterface::LockVB( const VertexBufferBase& VBuf, int ibase, int icount )
{	OrkAssert( false == VBuf.IsLocked() );
	int iVBlen = VBuf.GetVtxSize()*VBuf.GetMax();
	VBuf.Lock();
	const void* pdata = VBuf.GetHandle();
	OrkAssert(pdata!=0);
	return pdata;
}

void DuGeometryBufferInterface::UnLockVB( VertexBufferBase& VBuf )
{
	OrkAssert( VBuf.IsLocked() );
	VBuf.Unlock();
}
void DuGeometryBufferInterface::UnLockVB( const VertexBufferBase& VBuf )
{	OrkAssert( VBuf.IsLocked() );
	VBuf.Unlock();
}
void DuGeometryBufferInterface::ReleaseVB( VertexBufferBase& VBuf )
{
	std::free( (void *)  VBuf.GetHandle());
}

bool GfxTargetDummy::SetDisplayMode(DisplayMode *mode)
{
	return false;
}

void DuGeometryBufferInterface::DrawIndexedPrimitive( const VertexBufferBase& VBuf,const IndexBufferBase& IdxBuf , EPrimitiveType eType, int ivbase, int ivcount)
{
}

void DuGeometryBufferInterface::DrawPrimitive( const VertexBufferBase& VBuf, EPrimitiveType eType, int ivbase, int ivcount)
{
}

void DuGeometryBufferInterface::DrawIndexedPrimitiveEML( const VertexBufferBase& VBuf, const IndexBufferBase& IdxBuf , EPrimitiveType eType, int ivbase, int ivcount)
{
}
void DuGeometryBufferInterface::DrawPrimitiveEML( const VertexBufferBase& VBuf, EPrimitiveType eType, int ivbase, int ivcount)
{
}

bool DuTextureInterface::LoadTexture( const AssetPath& fname, Texture *ptex )
{
	///////////////////////////////////////////////
	AssetPath Filename = fname;
	bool bHasExt = Filename.HasExtension();
	if( false == bHasExt )
	{
		Filename.SetExtension( "dds" );
	}
	///////////////////////////////////////////////
	CFile TextureFile( Filename, ork::EFM_READ );
	if( false == TextureFile.IsOpen() )
	{
		return false;
	}
	return true;
}

} }
