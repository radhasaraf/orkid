////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////



inline ork::fvec4 SphMap( const ork::fvec3& N, const ork::fvec3& EyeToPointDir, const rend_texture2D& tex )
{
	ork::fvec3 ref = EyeToPointDir-N*(N.dotWith(EyeToPointDir)*2.0f);
	float p = ::sqrtf( ref.x*ref.x+ref.y*ref.y+::powf(ref.z+1.0f,2.0f) );
	float reflectS = ref.x/(2.0f*p)+0.5f;
	float reflectT = ref.y/(2.0f*p)+0.5f;
	return tex.sample_point( reflectS, reflectT, true, true );
}

inline ork::fvec4 OctaveTex( int inumoctaves, float fu, float fv, float texscale, float texamp, float texscalemodifier, float texampmodifier, const rend_texture2D& tex )
{
	ork::fvec4 tex0;
	for( int i=0; i<inumoctaves; i++ )
	{
		tex0 += tex.sample_point( fabs(fu*texscale), fabs(fv*texscale), true, true )*texamp;
		texscale *= texscalemodifier;
		texamp *= texampmodifier;
	}
	return tex0;
}

///////////////////////////////////////////////////////////////////////////////
struct test_volume_shader : public rend_volume_shader
{
	ork::fvec4 ShadeVolume( const ork::fvec3& entrywpos, const ork::fvec3& exitwpos ) const; // virtual
};
///////////////////////////////////////////////////////////////////////////////
struct Shader1 : public rend_shader
{
	ork::math::Perlin2D						mPerlin2D;
	rend_texture2D mTexture1;
	//cl_program							mProgram;
	//cl_kernel							mKernel;
	//CLKernel							mCLKernel;

	eType GetType() const { return EShaderTypeSurface; } // virtual

	void Shade( const rend_prefragment& prefrag, rend_fragment* pdstfrag )  const;
	void ShadeBlock( AABuffer& aabuf, int ifragbase, int icount, int inumtri ) const; // virtual

	Shader1(/*const CLengine& eng*/);
};

///////////////////////////////////////////////////////////////////////////////

struct Shader2 : public rend_shader
{
	rend_texture2D mTexture1;
	rend_texture2D mSphMapTexture;

	Shader2();

	eType GetType() const { return EShaderTypeSurface; } // virtual

	void Shade( const rend_prefragment& prefrag, rend_fragment* pdstfrag ) const  // virtual
	{
		const rend_ivtx* srcvtxR = prefrag.srcvtxR;
		const rend_ivtx* srcvtxS = prefrag.srcvtxS;
		const rend_ivtx* srcvtxT = prefrag.srcvtxT;
		const ork::fvec3& wposR = srcvtxR->mWldSpacePos;
		const ork::fvec3& wposS = srcvtxS->mWldSpacePos;
		const ork::fvec3& wposT = srcvtxT->mWldSpacePos;
		const ork::fvec3& wnrmR = srcvtxR->mWldSpaceNrm;
		const ork::fvec3& wnrmS = srcvtxS->mWldSpaceNrm;
		const ork::fvec3& wnrmT = srcvtxT->mWldSpaceNrm;
		float r = prefrag.mfR;
		float s = prefrag.mfS;
		float t = prefrag.mfT;
		float z = prefrag.mfZ;
		float wnx = wnrmR.x*r+wnrmS.x*s+wnrmT.x*t;
		float wny = wnrmR.y*r+wnrmS.y*s+wnrmT.y*t;
		float wnz = wnrmR.z*r+wnrmS.z*s+wnrmT.z*t;
		float wx = wposR.x*r+wposS.x*s+wposT.x*t;
		float wy = wposR.y*r+wposS.y*s+wposT.y*t;
		float wz = wposR.z*r+wposS.z*s+wposT.z*t;
		/////////////////////////////////////////////////////////////////
		float area = prefrag.mpSrcPrimitive->mfArea;
		float areaintens = (area==0.0f) ? 0.0f : ::powf( 100.0f / area, 0.7f );
		/////////////////////////////////////////////////////////////////
		float wxm = ::fmod( ::abs(wx)/20.0f, 1.0f )<0.1f;
		float wym = ::fmod( ::abs(wy)/20.0f, 1.0f )<0.1f;
		float wzm = ::fmod( ::abs(wz)/20.0f, 1.0f )<0.1f;
		ork::fvec3 vN = (ork::fvec3(wx,wy,wz)-mRenderData->mEye).normalized();
		/////////////////////////////////////////////////////////////////
		ork::fvec4 tex0 = OctaveTex( 4, wy, wy, 0.01f, 0.407f, 2.0f, 0.6f, mTexture1 );
		ork::fvec4 tex1 = OctaveTex( 4, wx, wz, 0.01f, 0.507f, 2.0f, 0.5f, mTexture1 );
		ork::fvec4 texout = tex0*tex1;
		/////////////////////////////////////////////////////////////////
		float fgrid = (wxm+wym+wzm)!=0.0f;
		///////////////////////////////////////////////
		float fZblend = 64.0f*::pow( z, 5.0f );
		///////////////////////////////////////////////
		ork::fvec4 sphtexout = SphMap( ork::fvec3(wnx,wny,wnz), vN, mSphMapTexture );
		///////////////////////////////////////////////
		ork::fvec3 c0( fgrid, fgrid, fgrid );
		ork::fvec3 c( wnx, wny, wnz ); c=c*0.6f+c0*0.1f+texout*::powf(areaintens,0.1f)*0.7f+sphtexout*0.5f;
		pdstfrag->mRGBA.set(c.x, c.y, c.z, fZblend ); // cunc
		pdstfrag->mZ = z;
		pdstfrag->mpPrimitive = prefrag.mpSrcPrimitive;
		pdstfrag->mpShader = this;
		pdstfrag->mWldSpaceNrm =ork:: fvec3( wnx, wny, wnz );
		pdstfrag->mWorldPos = ork::fvec3( wx, wy, wz );
		///////////////////////////////////////////////
	}
	//void ShadeBlock( AABuffer& aabuf, const rend_prefragsubgroup* pfgsubgrp, int ifragbase, int icount ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct MyBakeShader : public ork::BakeShader
{
	MyBakeShader(ork::Engine& eng,const RenderData*prdata);
	void Compute( int ix, int iy ) const; // virtual
};

///////////////////////////////////////////////////////////////////////////////

struct ShaderBuilder : public ork::RgmShaderBuilder
{
	MyBakeShader*	mpbakeshader;
	ork::Material*		mpmaterial;

	ShaderBuilder(ork::Engine* tracer,const RenderData*prdata);
	ork::BakeShader* CreateShader(const ork::RgmSubMesh& sub) const; // virtual
	ork::Material* CreateMaterial(const ork::RgmSubMesh& sub) const; // virtual
};

///////////////////////////////////////////////////////////////////////////////
