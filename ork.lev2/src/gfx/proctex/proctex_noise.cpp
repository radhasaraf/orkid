///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2010, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/proctex/proctex.h>
#include <ork/lev2/gfx/gfxmaterial_test.h>

#include <ork/reflect/RegisterProperty.h>
#include <ork/reflect/DirectObjectMapPropertyType.h>
#include <ork/reflect/DirectObjectPropertyType.hpp>

#include <ork/reflect/enum_serializer.h>
#include <ork/math/polar.h>
#include <ork/math/plane.hpp>

///////////////////////////////////////////////////////////////////////////////

INSTANTIATE_TRANSPARENT_RTTI(ork::proctex::Cells,"proctex::Cells");
INSTANTIATE_TRANSPARENT_RTTI(ork::proctex::Octaves,"proctex::Octaves");

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace proctex {
///////////////////////////////////////////////////////////////////////////////
void Octaves::Describe()
{	RegisterObjInpPlug( Octaves, Input );
	RegisterFloatXfPlug( Octaves, BaseOffsetX, -100.0f, 100.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, BaseOffsetY, -100.0f, 100.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, ScalOffsetX, -4.0f, 4.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, ScalOffsetY, -4.0f, 4.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, BaseFreq, -1.0f, 1.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, BaseAmp, -1.0f, 1.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, ScalFreq, -4.0f, 4.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Octaves, ScalAmp, -4.0f, 4.0f, ged::OutPlugChoiceDelegate );

	ork::reflect::RegisterProperty( "NumOctaves", & Octaves::miNumOctaves );
	ork::reflect::AnnotatePropertyForEditor< Octaves >( "NumOctaves", "editor.range.min", "1" );
	ork::reflect::AnnotatePropertyForEditor< Octaves >( "NumOctaves", "editor.range.max", "10" );

	static const char* EdGrpStr =
		        "grp://Basic Input NumOctaves "
                "grp://Plugs BaseOffsetX ScalOffsetX BaseOffsetY ScalOffsetY BaseFreq ScalFreq BaseAmp ScalAmp ";
	reflect::AnnotateClassForEditor<Octaves>( "editor.prop.groups", EdGrpStr );
}
///////////////////////////////////////////////////////////////////////////////
Octaves::Octaves()
	: ConstructInpPlug(Input, dataflow::EPR_UNIFORM,gNoCon)
	, ConstructInpPlug( BaseOffsetX, dataflow::EPR_UNIFORM, mfBaseOffsetX )
	, ConstructInpPlug( BaseOffsetY, dataflow::EPR_UNIFORM, mfBaseOffsetY )
	, ConstructInpPlug( ScalOffsetX, dataflow::EPR_UNIFORM, mfScalOffsetX )
	, ConstructInpPlug( ScalOffsetY, dataflow::EPR_UNIFORM, mfScalOffsetY )
	, ConstructInpPlug( BaseFreq, dataflow::EPR_UNIFORM, mfBaseFreq )
	, ConstructInpPlug( BaseAmp, dataflow::EPR_UNIFORM, mfBaseAmp )
	, ConstructInpPlug( ScalFreq, dataflow::EPR_UNIFORM, mfScalFreq )
	, ConstructInpPlug( ScalAmp, dataflow::EPR_UNIFORM, mfScalAmp )
	, mfBaseFreq(1.0f)
	, mfScalFreq(2.0f)
	, mfBaseAmp(1.0f)
	, mfScalAmp(0.5f)
	, mfBaseOffsetX(0.0f)
	, mfBaseOffsetY(0.0f)
	, mfScalOffsetX(1.0f)
	, mfScalOffsetY(1.0f)
	, miNumOctaves(1)
	, mOctMaterial( ork::lev2::GfxEnv::GetRef().GetLoaderTarget(), "orkshader://proctex", "octaves" )
{
}
///////////////////////////////////////////////////////////////////////////////
dataflow::inplugbase* Octaves::GetInput(int idx)
{	dataflow::inplugbase* rval = 0;
	switch( idx )
	{	case 0:	rval = & mPlugInpInput;			break;
		case 1:	rval = & mPlugInpBaseOffsetX;	break;
		case 2:	rval = & mPlugInpBaseOffsetY;	break;
		case 3:	rval = & mPlugInpScalOffsetX;	break;
		case 4:	rval = & mPlugInpScalOffsetY;	break;
		case 5:	rval = & mPlugInpBaseFreq;		break;
		case 6:	rval = & mPlugInpBaseAmp;		break;
		case 7:	rval = & mPlugInpScalFreq;		break;
		case 8:	rval = & mPlugInpScalAmp;		break;
	}
	return rval;
}
///////////////////////////////////////////////////////////////////////////////
void Octaves::compute( ProcTex& ptex )
{	Buffer& buffer = GetWriteBuffer(ptex);
	//printf( "Octaves wrbuf<%p> wrtex<%p>\n", & buffer, buffer.OutputTexture() );
	const ImgOutPlug* conplug = rtti::autocast(mPlugInpInput.GetExternalOutput());
	if(conplug)
	{
		ork::lev2::GfxTarget* pTARG = buffer.GetContext();
		mOctMaterial.SetColorMode( lev2::GfxMaterial3DSolid::EMODE_USER );
		mOctMaterial.mRasterState.SetAlphaTest( ork::lev2::EALPHATEST_OFF );
		mOctMaterial.mRasterState.SetCullTest( ork::lev2::ECULLTEST_OFF );
		mOctMaterial.mRasterState.SetBlending( ork::lev2::EBLENDING_ADDITIVE );
		mOctMaterial.mRasterState.SetDepthTest( ork::lev2::EDEPTHTEST_ALWAYS );
		mOctMaterial.mRasterState.SetZWriteMask( false );

		auto inptex = conplug->GetValue().GetBuffer(ptex).OutputTexture();
		inptex->TexSamplingMode().PresetTrilinearWrap();
		pTARG->TXI()->ApplySamplingMode(inptex);

		mOctMaterial.SetTexture( inptex );
		mOctMaterial.SetUser0( CVector4(0.0f,0.0f,0.0f,float(Buffer::kw)) );
		////////////////////////////////////
		float ffrq = mPlugInpBaseFreq.GetValue();
		float famp = mPlugInpBaseAmp.GetValue();
		float offx = mPlugInpBaseOffsetX.GetValue();
		float offy = mPlugInpBaseOffsetY.GetValue();
		buffer.GetContext()->FBI()->SetAutoClear(true);
		buffer.PtexBegin();
		pTARG->BindMaterial( & mOctMaterial );
		for( int i=0; i<miNumOctaves; i++ )
		{
			CMatrix4 mtxS;
			CMatrix4 mtxT;
			mtxT.SetTranslation(offx,offy,0.0f);
			mtxS.Scale( ffrq, ffrq, famp );
			mOctMaterial.SetAuxMatrix( mtxS*mtxT );
			{
				//printf( "DrawUnitTexQuad oct<%d>\n", i );
				UnitTexQuad( pTARG );
			}
			ffrq *= mPlugInpScalFreq.GetValue();
			famp *= mPlugInpScalAmp.GetValue();
			offx *= mPlugInpScalOffsetX.GetValue();
			offy *= mPlugInpScalOffsetY.GetValue();
		}
		buffer.PtexEnd();
		////////////////////////////////////
		pTARG->BindMaterial( 0 );
	}
	MarkClean();
}
///////////////////////////////////////////////////////////////////////////////
void Cells::Describe()
{
	ork::reflect::RegisterProperty( "SeedA", & Cells::miSeedA );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "SeedA", "editor.range.min", "0" );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "SeedA", "editor.range.max", "1000" );

	ork::reflect::RegisterProperty( "SeedB", & Cells::miSeedB );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "SeedB", "editor.range.min", "0" );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "SeedB", "editor.range.max", "1000" );

	ork::reflect::RegisterProperty( "DimU", & Cells::miDimU );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "DimU", "editor.range.min", "1" );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "DimU", "editor.range.max", "7" );

	ork::reflect::RegisterProperty( "DimV", & Cells::miDimV );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "DimV", "editor.range.min", "1" );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "DimV", "editor.range.max", "7" );

	ork::reflect::RegisterProperty( "Divs", & Cells::miDiv );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "Divs", "editor.range.min", "1" );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "Divs", "editor.range.max", "16" );

	ork::reflect::RegisterProperty( "Smoothing", & Cells::miSmoothing );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "Smoothing", "editor.range.min", "0" );
	ork::reflect::AnnotatePropertyForEditor< Cells >( "Smoothing", "editor.range.max", "8" );

	RegisterFloatXfPlug( Cells, Dispersion, 0.001f, 1.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Cells, SeedLerp, 0.0f, 1.0f, ged::OutPlugChoiceDelegate );
	RegisterFloatXfPlug( Cells, SmoothingRadius, 0.0f, 1.0f, ged::OutPlugChoiceDelegate );

	ork::reflect::RegisterProperty( "AntiAlias", & Cells::mbAA );

	static const char* EdGrpStr =
		        "grp://Basic AntiAlias SeedA SeedB DimU DimV Divs Smoothing "
                "grp://Plugs Dispersion SeedLerp SmoothingRadius ";

	reflect::AnnotateClassForEditor<Cells>( "editor.prop.groups", EdGrpStr );

}
///////////////////////////////////////////////////////////////////////////////
Cells::Cells()
	: mVertexBuffer(65536, 0, ork::lev2::EPRIM_MULTI)
	, miSeedA(0)
	, miSeedB(0)
	, miDimU(2)
	, miDimV(2)
	, miDiv(1)
	, mVBHash()
	, mPlugInpDispersion( this, dataflow::EPR_UNIFORM,mfDispersion, "di" )
	, mPlugInpSeedLerp( this, dataflow::EPR_UNIFORM,mfSeedLerp, "sl" )
	, mPlugInpSmoothingRadius( this, dataflow::EPR_UNIFORM,mfSmoothingRadius, "sr" )
	, miSmoothing(0)
	, mbAA(false)
{
}
///////////////////////////////////////////////////////////////////////////////
dataflow::inplugbase* Cells::GetInput(int idx)
{	dataflow::inplugbase* rval = 0;
	switch( idx )
	{	case 0:	rval = & mPlugInpDispersion; break;
		case 1:	rval = & mPlugInpSeedLerp; break;
		case 2:	rval = & mPlugInpSmoothingRadius; break;
		default:
			OrkAssert(false);
			break;
	}
	return rval;
}
///////////////////////////////////////////////////////////////////////////////
void Cells::ComputeVB( lev2::GfxBuffer& buffer )
{	CVector3 wrapu( float(miDimU), 0.0f, 0.0f );
	CVector3 wrapv( 0.0f, float(miDimV), 0.0f );
	////////////////////////////////////////////////
	int ivecsize = miDimU*miDimV;
	mSitesA.resize(ivecsize);
	mSitesB.resize(ivecsize);
	mPolys.resize(ivecsize);
	for( int ix=0; ix<miDimU; ix++ )
	{	for( int iy=0; iy<miDimV; iy++ )
		{	mPolys[site_index(ix,iy)].SetDefault();
			mPolys[site_index(ix,iy)].AddVertex( CVector3(-100.0f,-100.0f,0.0f) );
			mPolys[site_index(ix,iy)].AddVertex( CVector3(+100.0f,-100.0f,0.0f) );
			mPolys[site_index(ix,iy)].AddVertex( CVector3(+100.0f,+100.0f,0.0f) );
			mPolys[site_index(ix,iy)].AddVertex( CVector3(-100.0f,+100.0f,0.0f) );

		}
	}
	srand(miSeedA);
	for( int ix=0; ix<miDimU; ix++ )
	{	for( int iy=0; iy<miDimV; iy++ )
		{	float ifx = float(rand()%miDiv)/float(miDiv);
			float fx = float(ix)+ifx*mPlugInpDispersion.GetValue();
			float ify = float(rand()%miDiv)/float(miDiv);	
			float fy = float(iy)+ify*mPlugInpDispersion.GetValue();
			mSitesA[site_index(ix,iy)] = CVector3(fx,fy,0.0f);
		}
	}
	srand(miSeedB);
	for( int ix=0; ix<miDimU; ix++ )
	{	for( int iy=0; iy<miDimV; iy++ )
		{	float ifx = float(rand()%miDiv)/float(miDiv);
			float fx = float(ix)+ifx*mPlugInpDispersion.GetValue();
			float ify = float(rand()%miDiv)/float(miDiv);	
			float fy = float(iy)+ify*mPlugInpDispersion.GetValue();
			mSitesB[site_index(ix,iy)] = CVector3(fx,fy,0.0f);
		}
	}
	float flerp = mPlugInpSeedLerp.GetValue();
	////////////////////////////////////////////////
	ork::lev2::GfxTarget* pTARG = buffer.GetContext();
	mVertexBuffer.Reset();
	mVW.Lock( pTARG, & mVertexBuffer, 3*8*(1+miSmoothing)*miDimU*miDimV );
	////////////////////////////////////////////////
	for( int ix=0; ix<miDimU; ix++ )
	{	for( int iy=0; iy<miDimV; iy++ )
		{	const CVector3& Site0A = mSitesA[site_index(ix,iy)];
			const CVector3& Site0B = mSitesB[site_index(ix,iy)];
			static const int kmaxpc = 64;
			CellPoly polychain[kmaxpc];
			int polychi = 0;
			polychain[polychi] = mPolys[site_index(ix,iy)];
			for( int iox=-2; iox<3; iox++ )
			{	int ixb = ix + iox;
				bool bwrapL = (ixb<0);
				bool bwrapR = (ixb>=miDimU);
				ixb = bwrapL ? ixb+miDimU : ixb%miDimU;
				for( int ioy=-2; ioy<3; ioy++ )
				{	if(iox!=0||ioy!=0)
					{	int iyb = iy + ioy;
						bool bwrapT = (iyb<0);
						bool bwrapB = (iyb>=miDimV);
						iyb = bwrapT ? iyb+miDimV : iyb%miDimV;
						CVector3 Site1A = mSitesA[site_index(ixb,iyb)];
						CVector3 Site1B = mSitesB[site_index(ixb,iyb)];
						if( bwrapL ) { Site1A -= wrapu; Site1B -= wrapu; }
						if( bwrapR ) { Site1A += wrapu; Site1B += wrapu; }
						if( bwrapB ) { Site1A += wrapv; Site1B += wrapv; }
						if( bwrapT ) { Site1A -= wrapv; Site1B -= wrapv; }

						CVector3 CenterA = (Site0A+Site1A)*0.5f;
						CVector3 DirA = (Site0A-Site1A).Normal();

						CVector3 CenterB = (Site0B+Site1B)*0.5f;
						CVector3 DirB = (Site0B-Site1B).Normal();

						CVector3 Center; Center.Lerp(CenterA,CenterB,flerp );
						CVector3 Dir; Dir.Lerp(DirA,DirB,flerp );
						Dir.Normalize();

						CPlane plane( Dir, Center );
						plane.ClipPoly( polychain[polychi], polychain[polychi+1] );
						polychi++;
					}
				}
			}
			OrkAssert(polychi<kmaxpc);
			const CellPoly& outpoly = polychain[polychi]; 
			CVector3 vctr;
			vctr.Lerp( Site0A,Site0B, flerp );
			
			////////////////////////////////////////////////
			if( miSmoothing )
			{
				float smoothrad = mPlugInpSmoothingRadius.GetValue();

				for( int iv=0; iv<outpoly.GetNumVertices(); iv++ )
				{	int ivp = (iv-1); if(ivp<0) ivp+= outpoly.GetNumVertices();
					int ivn = (iv+1)%outpoly.GetNumVertices();
					CVector3 p0; p0.Lerp( vctr, outpoly.GetVertex(iv).Pos(), smoothrad );
					CVector3 pp; pp.Lerp( vctr, outpoly.GetVertex(ivp).Pos(), smoothrad );
					CVector3 pn; pn.Lerp( vctr, outpoly.GetVertex(ivn).Pos(), smoothrad );
					
					for( int is=0; is<miSmoothing; is++ )
					{
						float fu0 = (float(is)/float(miSmoothing));
						float fu1 = (float(is+1)/float(miSmoothing));

						CVector3 p_0 = (pp+p0)*0.5f;
						CVector3 n_0 = (p0+pn)*0.5f;

						CVector3 p_p_0; p_p_0.Lerp( p_0, p0, fu0 );
						CVector3 p_p_1; p_p_1.Lerp( p_0, p0, fu1 );

						CVector3 p_n_0; p_n_0.Lerp( p0, n_0, fu0 );
						CVector3 p_n_1; p_n_1.Lerp( p0, n_0, fu1 );

						CVector3 p_s_0; p_s_0.Lerp( p_p_0, p_n_0, fu0 );
						CVector3 p_s_1; p_s_1.Lerp( p_p_1, p_n_1, fu1 );

						mVW.AddVertex( ork::lev2::SVtxV12C4T16(vctr,CVector2(),0xffffffff) );
						mVW.AddVertex( ork::lev2::SVtxV12C4T16(p_s_0,CVector2(),0) );
						mVW.AddVertex( ork::lev2::SVtxV12C4T16(p_s_1,CVector2(),0) );
					}
				}
			}
			else
			for( int iv=0; iv<outpoly.GetNumVertices(); iv++ )
			{	
				float fi = float(iv)/float(outpoly.GetNumVertices()-1);
				CVector4 clr(fi,fi,fi,fi);
				int ivi = (iv+1)%outpoly.GetNumVertices();
				CVector3 p0 = outpoly.GetVertex(iv).Pos();
				CVector3 p1 = outpoly.GetVertex(ivi).Pos();
				mVW.AddVertex( ork::lev2::SVtxV12C4T16(vctr,CVector2(),0xffffffff) );
				mVW.AddVertex( ork::lev2::SVtxV12C4T16(p0,CVector2(),0) );
				mVW.AddVertex( ork::lev2::SVtxV12C4T16(p1,CVector2(),0) );
			}
		}
	}
	mVW.UnLock( pTARG );
}
///////////////////////////////////////////////////////////////////////////////
void Cells::compute( ProcTex& ptex )
{	Buffer& buffer = GetWriteBuffer(ptex);
	////////////////////////////////////////////////////////////////
	dataflow::node_hash testhash;
	testhash.Hash( miDimU );
	testhash.Hash( miDimV );
	testhash.Hash( miDiv );
	testhash.Hash( miSeedA );
	testhash.Hash( miSeedB );
	testhash.Hash( miSmoothing );
	testhash.Hash( mPlugInpDispersion.GetValue() );
	testhash.Hash( mPlugInpSeedLerp.GetValue() );
	testhash.Hash( mPlugInpSmoothingRadius.GetValue() );
	if( testhash != mVBHash )
	{	ComputeVB( buffer );
		mVBHash = testhash;
	}
	////////////////////////////////////////////////////////////////
	struct AA16RenderCells : public AA16Render
	{
		virtual void DoRender( float left, float right, float top, float bot, Buffer& buf  )
		{	buf.GetContext()->PushModColor( ork::CVector4::Red() );
			CMatrix4 mtxortho = buf.GetContext()->MTXI()->Ortho( left, right, top, bot, 0.0f, 1.0f );
			buf.GetContext()->PushMaterial( & stdmat );
			buf.GetContext()->MTXI()->PushPMatrix( mtxortho );
			for( int iw=0; iw<9; iw++ )
			{	int ix = (iw%3)-1;
				int iy = (iw/3)-1;
				CVector3 wpu = (ix>0) ? wrapu : (ix<0) ? -wrapu : CVector3::Zero(); 
				CVector3 wpv = (iy>0) ? wrapv : (iy<0) ? -wrapv : CVector3::Zero(); 
				CMatrix4 mtx;
				mtx.SetTranslation( wpu+wpv );
				buf.GetContext()->MTXI()->PushMMatrix( mtx );
				buf.GetContext()->GBI()->DrawPrimitive( mVW, ork::lev2::EPRIM_TRIANGLES );
				buf.GetContext()->MTXI()->PopMMatrix();
			}
			buf.GetContext()->MTXI()->PopPMatrix();
			buf.GetContext()->PopMaterial();
			buf.GetContext()->PopModColor();
		}
		lev2::GfxMaterial3DSolid stdmat;
		ork::lev2::VtxWriter<ork::lev2::SVtxV12C4T16>& mVW;
		CVector3 wrapu;
		CVector3 wrapv;
		AA16RenderCells( ProcTex& ptx, Buffer& bo,
						 ork::lev2::VtxWriter<ork::lev2::SVtxV12C4T16>& vw,
						 int idimU, int idimV )
			: AA16Render( ptx, bo )
			, stdmat( bo.GetContext() )
			, mVW(vw)
			, wrapu( float(idimU), 0.0f, 0.0f )
			, wrapv( 0.0f, float(idimV), 0.0f )
		{
			stdmat.SetColorMode( lev2::GfxMaterial3DSolid::EMODE_VERTEX_COLOR );
			stdmat.mRasterState.SetAlphaTest( ork::lev2::EALPHATEST_OFF );
			stdmat.mRasterState.SetCullTest( ork::lev2::ECULLTEST_OFF );
			stdmat.mRasterState.SetBlending( ork::lev2::EBLENDING_OFF );
			stdmat.mRasterState.SetDepthTest( ork::lev2::EDEPTHTEST_ALWAYS );
			stdmat.SetUser0( CVector4(0.0f,0.0f,0.0f,float(Buffer::kw)) );

			mOrthoBoxXYWH = CVector4( 0.0f, 0.0f, float(idimU), float(idimV) );
		}
	};

	////////////////////////////////////////////////////////////////

	AA16RenderCells renderer( ptex, buffer, mVW, miDimU, miDimV );
	renderer.Render( mbAA );

	//MarkClean();
}

}}

///////////////////////////////////////////////////////////////////////////////

template bool ork::CPlane::ClipPoly<ork::proctex::CellPoly>(const ork::proctex::CellPoly& in, ork::proctex::CellPoly& out );
template bool ork::CPlane::ClipPoly<ork::proctex::CellPoly>(const ork::proctex::CellPoly& in, ork::proctex::CellPoly& fr, ork::proctex::CellPoly& bk );
