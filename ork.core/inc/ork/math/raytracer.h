///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyrigh 1996-2009, Michael T. Mayers
// See License at OrkidRoot/license.html or http://www.tweakoz.com/orkid/license.html
//
// based on code from Jacco Bikker
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ORK_MATH_RAYTRACER_H
#define _ORK_MATH_RAYTRACER_H

#include <ork/math/line.h>
#include <ork/math/box.h>
#include <ork/kernel/Array.h>
#include <ork/math/cvector3.h>
#include <ork/kernel/mutex.h>
#include <ork/kernel/any.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

class FixedGrid;

#define HIT		 1		// Ray hit primitive
#define MISS	 0		// Ray missed primitive
#define INPRIM	-1		// Ray started inside primitive
#define TRACEDEPTH		4
#define IMPORTANCE

///////////////////////////////////////////////////////////////////////////////

struct BakeShadowFragment;
class Engine;

///////////////////////////////////////////////////////////////////////////////

struct RayPixel
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

///////////////////////////////////////////////////////////////////////////////

struct RayBundle
{
	fixedvector<Ray3,16>	mRays;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct Jitterer
{
	int miNumSamples;
	int miKos;

	CVector3	sample[1024];

	Jitterer( int ikos, float fradius, const CVector3& dX, const CVector3& dY );
	const CVector3& GetSample( int idx ) const { return sample[idx]; }

};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct BakeShader
{
	BakeShader( Engine& eng ) : mEngine(eng) {}
	Engine&	mEngine;
	anyp	mPlatformShader;
	virtual void Compute( int ix, int iy ) const = 0;
};


class Material
{
public:
	Material();
	void SetColor( CVector3& clr ) { mColor = clr; }
	CVector3 GetColor() { return mColor; }
	void SetDiffuse( float a_Diff ) { m_Diff = a_Diff; }
	void SetSpecular( float a_Spec ) { m_Spec = a_Spec; }
	void SetReflection( float a_Refl ) { m_Refl = a_Refl; }
	void SetRefraction( float a_Refr ) { m_Refr = a_Refr; }
	void SetParameters( float a_Refl, float a_Refr, const CVector3& a_Col, float a_Diff, float a_Spec );
	float GetSpecular() { return m_Spec; }
	float GetDiffuse() { return m_Diff; }
	float GetReflection() { return m_Refl; }
	float GetRefraction() { return m_Refr; }
	void SetRefrIndex( float a_Refr ) { m_RIndex = a_Refr; }
	float GetRefrIndex() { return m_RIndex; }
	void SetDiffuseRefl( float a_DRefl ) { m_DRefl = a_DRefl; }
	float GetDiffuseRefl() { return m_DRefl; }
private:
	CVector3 mColor;
	float m_Refl, m_Refr;
	float m_Diff, m_Spec;
	float m_DRefl;
	float m_RIndex;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Primitive;
struct RgmSubMesh;

class RgmGeoSet
{
public:
	int NumPrimitives() const { return mPrimitives.size(); }
	const Primitive* GetPrimitive( int idx ) const { return mPrimitives[idx]; }
	void AddPrimitive( const Primitive* prim ) { mPrimitives.push_back(prim); }
	const orkvector<const Primitive*>& GetPrims() const { return mPrimitives; }
	AABox& GetAABox() { return mAABox; }
private:
	orkvector<const Primitive*> mPrimitives;
	AABox	mAABox;
};

struct RgmLight
{
	CVector3	mPos;
	CVector3	mColor;
	CVector3	mDir;
	bool		mCastsShadows;
	float		mDispersion;
	float		mFalloff;
	float		mRadius;

	RgmLight( const CVector3& p=CVector3(0.0f,0.0f,0.0f), const CVector3& c=CVector3(0.0f,0.0f,0.0f) )
		: mPos(p)
		, mColor(c)
		, mDispersion(1.0f)
		, mDir(0.0f,1.0f,0.0f)
		, mFalloff(0.0f)
		, mRadius(0.0f)
		, mCastsShadows(false)
	{
	}

};

struct RgmShaderBuilder
{
	virtual BakeShader* CreateShader(const RgmSubMesh& sub) const = 0;
	virtual Material* CreateMaterial(const RgmSubMesh& sub) const = 0;
};
struct RgmLightBuilder
{
	virtual void CreateLight(const RgmLight& sub) const = 0;
};

struct RgmVertex
{
	CVector3 pos;
	CVector3 nrm;
	CVector2 uv;
};
struct RgmTri
{
	RgmVertex*	mpv0;
	RgmVertex*	mpv1;
	RgmVertex*	mpv2;
	CPlane		mFacePlane;
	CPlane		mEdgePlane0;
	CPlane		mEdgePlane1;
	CPlane		mEdgePlane2;
	float		mArea;
	void Compute();
};
struct RgmSubMesh
{
	s32								minumverts;
	RgmVertex*						mpVertices;
	s32								minumtris;
	RgmTri*							mtriangles;
	const BakeShader*				mpShader;
	const Material*					mpMaterial;
	std::string						mname;
	orkmap<std::string,std::string>	mAnnos;
	RgmGeoSet*						mGeoSet;
};
struct RgmModel
{
	s32				minumsubs;
	RgmSubMesh*		msubmeshes;
	orkmap<std::string,RgmSubMesh*>	mSubMeshMap;
	orkmap<std::string,std::string>	mAnnos;
	AABox			mAABox;
};
struct RgmLightContainer
{
	orkmap<std::string,RgmLight>	mLights;
	void LoadLitFile( const char* pfilename );
};
RgmModel* LoadRgmFile( const char* pfilename, RgmShaderBuilder& shbuilder );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Primitive
{
public:
	Primitive() : mLastRayID(-1) , mMaterial(0), mBakeShader(0) {}
	~Primitive();
	BakeShader* GetBakeShader() const { return mBakeShader; }
	void SetBakeShader( BakeShader* mtl ) { mBakeShader = mtl; }
	Material* GetMaterial() const { return mMaterial; }
	void SetMaterial( Material* mtl ) { mMaterial = mtl; }
	CVector3 GetColor( const CVector3& ) const;
	int GetLastRayID() const { return mLastRayID; }
	//////////////////////////////////////////////////
	static bool PlaneBoxOverlap( const CVector3& Normal, const CVector3& Vert, const CVector3& MaxBox );
	static bool IntersectTriBox( const CVector3& BoxCentre, const CVector3& BoxHalfsize, const CVector3& V0, const CVector3& V1, const CVector3& V2 );
	//////////////////////////////////////////////////
	virtual AABox GetAABox() const = 0;
	virtual int Intersect( const Ray3& a_Ray, CVector3& isect, float& a_Dist ) const = 0;
	virtual CVector3 GetNormal( const CVector3& pos ) const = 0;
	virtual bool IntersectBox( const AABox& a_Box ) const = 0;
	virtual void Rasterize( Engine* peng ) const = 0;
	//////////////////////////////////////////////////

protected:
	Material* mMaterial;
	BakeShader*	mBakeShader;
	int mLastRayID;
};

///////////////////////////////////////////////////////////////////////////////

class RaytSphere : public Primitive
{
public:
	CVector3 mCenter;						
	float mSqRadius, mRadius, mRRadius;		
	CVector3 mVe, mVn, mVc;					
	RaytSphere( CVector3& center, float radius );
	// sphere primitive methods
	const CVector3& GetCentre() const { return mCenter; }
	float GetSqRadius() const { return mSqRadius; }
	float GetRadius() const { return mRadius; }
	bool IntersectSphereBox( const CVector3& a_Centre, const AABox& a_Box ) const;
	virtual AABox GetAABox() const;
	virtual int Intersect( const Ray3& a_Ray, CVector3& isect, float& a_Dist ) const;
	virtual CVector3 GetNormal( const CVector3& pos ) const;
	virtual bool IntersectBox( const AABox& a_Box ) const;
	virtual void Rasterize( Engine* peng ) const;
};

class RaytTriangle : public Primitive
{
public:
	const RgmTri* mRgmPoly;					
	CVector3 mN;							
	const RgmVertex* mVertex[3];			
	float mU, mV;							
	float nu, nv, nd;						
	int k;									
	float bnu, bnv;							
	float cnu, cnv;							

	RaytTriangle( const RgmVertex* v1, const RgmVertex* v2, const RgmVertex* v3 );
	// triangle primitive methods
	const CVector3& GetNormal() const { return mN; }
	const RgmVertex* GetVertex( int idx ) const { return mVertex[idx]; }
	void SetVertex( int idx, RgmVertex* vtx ) { mVertex[idx] = vtx; }
	virtual AABox GetAABox() const;
	virtual int Intersect( const Ray3& a_Ray, CVector3& isect, float& a_Dist ) const;
	virtual CVector3 GetNormal( const CVector3& pos ) const;
	virtual bool IntersectBox( const AABox& a_Box ) const;
	virtual void Rasterize( Engine* peng ) const;
};

///////////////////////////////////////////////////////////////////////////////

class ObjectList
{
public:
	ObjectList() : m_Primitive( 0 ), m_Next( 0 ), mMutex("ObjectList") {}
	~ObjectList() { delete m_Next; }
	void SetPrimitive( const Primitive* a_Prim ) { m_Primitive = a_Prim; }
	const Primitive* GetPrimitive() const { return m_Primitive; }
	void SetNext( ObjectList* a_Next ) { m_Next = a_Next; }
	const ObjectList* GetNext() const { return m_Next; }
	void Lock() { mMutex.Lock(); }
	void UnLock() { mMutex.UnLock(); }
private:
	const Primitive*			m_Primitive;
	ObjectList*					m_Next;
	ork::recursive_mutex		mMutex;
};

///////////////////////////////////////////////////////////////////////////////

class Scene
{
public:
	////////////////////////////////////////////////////////
	Scene();
	~Scene();
	bool InitScene( const AABox& scene_box );
	void ExitScene();
	////////////////////////////////////////////////////////
	const AABox& GetExtends() const { return mExtends; }
	FixedGrid* GetFixedGrid() const { return mpFixedGrid; }
	void AddGeoSet( const std::string& name, RgmGeoSet* pset );
	void RemoveGeoSet( const std::string& name );
	const RgmGeoSet* FindGeoSet( const std::string& name ) const;
	void ClearGeoSets() { mGeoSets.clear(); }
	const orkmap<std::string,const RgmGeoSet*>& GetGeoSets() const { return mGeoSets; }
	const orkvector<RgmLight>& GetLights() const { return mLights; }
	orkvector<RgmLight>& GetLights() { return mLights; }
	////////////////////////////////////////////////////////
private:
	////////////////////////////////////////////////////////
	orkmap<std::string,const RgmGeoSet*>	mGeoSets;
	orkvector<RgmLight>						mLights;
	AABox mExtends;
	FixedGrid*	mpFixedGrid;
	
};

///////////////////////////////////////////////////////////////////////////////

struct BakeShadowFragment
{
	CVector3	mPos;
	CVector3	mNrm;

	inline BakeShadowFragment operator-( const BakeShadowFragment &oth ) const
	{
		BakeShadowFragment rval;
		rval.mPos = mPos-oth.mPos;
		rval.mNrm = (mNrm-oth.mNrm);
		return rval;
	}
	inline BakeShadowFragment operator*( float scalar ) const
	{
		BakeShadowFragment rval;
		rval.mPos = mPos*scalar;
		rval.mNrm = mNrm*scalar;
		//rval.mNrm.Normalize();
		return rval;
	}
	inline void operator+=( const BakeShadowFragment & b )
	{
		mPos+=b.mPos;
		mNrm+=b.mNrm;
		//mnrm.Normalize();
	}
};

///////////////////////////////////////////////////////////////////////////////

struct SpanFragment
{
    SpanFragment(int X, int Y, BakeShadowFragment const& data) : x(X), y(Y), mData(data) {  }

    bool operator<(SpanFragment const& rhs) const
	{
		return (y<rhs.y) || ( (y==rhs.y) && (x<rhs.x) );
	}

    int x, y;
    BakeShadowFragment mData;
};

///////////////////////////////////////////////////////////////////////////////

struct SpanCtx
{
	bool swap_xy;
	bool flip_y;  

	SpanCtx()
		: swap_xy(false)
		, flip_y(false)
	{
	}
};

///////////////////////////////////////////////////////////////////////////////

class Engine
{
	public:
	
	Engine();
	~Engine();
	void SetTarget( RayPixel* dest, int w, int h );
	Scene* GetScene() { return mScene; }
	bool FindNearest( const FixedGrid* fg, const Ray3& a_Ray, const float tmin, const float tmax, float& dist, const Primitive*& prim );
	const Primitive* Raytrace( const Ray3& ray, const int depth, const float rindex, CVector3& acc, float& dist );
	void InitRender( CVector3& eye, CVector3& tgt );
	const Primitive* RenderRay( CVector3 screenpos, CVector3& acc );
	bool Render( const AABox& aab, const std::string& OutputName );
	bool Bake( const AABox& bbox, const std::string& OutputName );
	int GetHeight() const { return miH; }
	int GetWidth() const { return miW; }
	void Resize(int iw, int ih) { miW=iw; miH=ih; }
	RayPixel& RefPixel( int idx ) { return mDest[idx]; }
	const BakeShadowFragment& RefFragment( int idx ) { return mFragments[idx]; }
	const CVector3& CornerTL() const { return mCornerTL; }
	const CVector3& CornerTR() const { return mCornerTR; }
	const CVector3& CornerBL() const { return mCornerBL; }
	const CVector3& CornerBR() const { return mCornerBR; }

	void DrawSpan(	const BakeShader& shader,
					int x1, int y1, const BakeShadowFragment& d1, 
                    int x2, int y2, const BakeShadowFragment& d2, 
                    SpanCtx& ctx, orkset<SpanFragment>* output );

	void DrawSpanE(	const BakeShader& shader,
					int x1, int y1, BakeShadowFragment d1, 
                    int x2, int y2, BakeShadowFragment d2, 
                    SpanCtx& ctx, orkset<SpanFragment>* output );

	void RasterizeTriangle(	const BakeShader& shader,
							int x1, int y1, const BakeShadowFragment& d1, 
							int x2, int y2, const BakeShadowFragment& d2, 
							int x3, int y3, const BakeShadowFragment& d3 );
  

	private:

	Scene*					mScene;
	RayPixel*				mDest;
	BakeShadowFragment*		mFragments;
	int						miW, miH;
	CVector3				mEye;
	CVector3				mCornerTL, mCornerTR, mCornerBL, mCornerBR;
	CVector3				mDX, mDY;
	int*					mMod;
};

///////////////////////////////////////////////////////////////////////////////

template< typename SceneType > struct RayShader
{
	static inline void Sample( float fxc, float fyc, float fZ0, float fZ1, const SceneType& scene, Ray3HitTest& reciever )
	{	float fx0 = fxc;
		float fy0 = fyc;
		float fz0 = fZ0;
		float fx1 = fx0+0.1f;
		float fy1 = fy0+0.1f;
		float fz1 = fZ1;
		CVector3 v0( fx0, fy0, fz0 );
		CVector3 v1( fx1, fy1, fz1 );
		Ray3 myray( v0, (v1-v0).Normal() );
		scene.RayTest(myray,reciever);
	}
};

///////////////////////////////////////////////////////////////////////////////

}

#endif
