////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/file/chunkfile.h>
#include <ork/file/chunkfile.hpp>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/gfx/gfxmaterial_basic.h>
#include <ork/lev2/gfx/gfxmaterial_fx.h>
#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/kernel/string/StringBlock.h>
#include <ork/rtti/downcast.h>
#include <ork/kernel/prop.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/application/application.h>
#include <ork/kernel/string/string.h>

#if defined(WII)
#include <ork/mem/wii_mem.h>
#endif

#if ! defined( USE_XGM_FILES )
#include <miniork_tool/filter/gfx/collada/collada.h>
#endif

///////////////////////////////////////////////////////////////////////////////

namespace ork {
namespace lev2 {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if defined( USE_XGM_FILES )

///////////////////////////////////////////////////////////////////////////////
struct ModelLoadAllocator
{	//////////////////////////////
	// per chunk allocation policy
	//////////////////////////////
	void* alloc( const char* pchkname, int ilen )
	{
		void* pmem = 0;
#if defined(WII)
		//static void* pmodeldatabuffer = wii::MEM2StackedAlloc(kmaxbuf);
		if( 0 == strcmp( pchkname, "header" ) ) pmem = new char[ilen];
		else if( 0 == strcmp( pchkname, "modeldata" ) ) pmem = wii::LockSharedMem2Buf( ilen );
#else
		if( 0 == strcmp( pchkname, "header" ) ) pmem = new char[ilen];
		else if( 0 == strcmp( pchkname, "modeldata" ) ) pmem = new char[ilen];
#endif
		return pmem;
	}
	//////////////////////////////
	// per chunk deallocation policy
	//////////////////////////////
	void done( const char* pchkname, void* pdata )
	{
		char* pch = (char*) pdata;

		if( 0 == strcmp( pchkname, "header" ) )
		{

			delete[] pch;
		}			
		else if( 0 == strcmp( pchkname, "modeldata" ) )
		{
#if defined(WII)
			wii::UnLockSharedMem2Buf();
#else
			delete[] pch;

#endif
		}
	}
};

////////////////////////////////////////////////////////////
// account for bug in old XGM files (version 0)
// since EVTXSTREAMFMT_V12N12T16C4 was missing,
// old xgm files had bad enum strings
// => when loading old xgm files
//     and seeing EVTXSTREAMFMT_V12N12B12T8C4 or higher , subtract one
// we will add a xgm version string to the format
//  and attempt to make the PropTypeString for enums more robust 
////////////////////////////////////////////////////////////

EVtxStreamFormat GetVersion0VertexStreamFormat( const char* fmtstr )
{
	static bool gbINIT = true;
	static orkmap<std::string,EVtxStreamFormat> formatmap;
	if( gbINIT )
	{
		formatmap["EVTXSTREAMFMT_V16"]=EVTXSTREAMFMT_V16;				// 0		
		formatmap["EVTXSTREAMFMT_V4T4"]=EVTXSTREAMFMT_V4T4;				// 1
		formatmap["EVTXSTREAMFMT_V4C4"]=EVTXSTREAMFMT_V4C4;				// 2
		formatmap["EVTXSTREAMFMT_V4T4C4"]=EVTXSTREAMFMT_V4T4C4;			// 3
		formatmap["EVTXSTREAMFMT_V12C4T16"]=EVTXSTREAMFMT_V12C4T16;		// 4
		
		formatmap["EVTXSTREAMFMT_V12N6I1T4"]=EVTXSTREAMFMT_V12N6I1T4;		// 5
		formatmap["EVTXSTREAMFMT_V12N6C2T4"]=EVTXSTREAMFMT_V12N6C2T4;
		
		formatmap["EVTXSTREAMFMT_V16T16C16"]=EVTXSTREAMFMT_V16T16C16;		// 7
		formatmap["EVTXSTREAMFMT_V12I4N12T8"]=EVTXSTREAMFMT_V12I4N12T8;	
		formatmap["EVTXSTREAMFMT_V12C4N6I2T8"]=EVTXSTREAMFMT_V12C4N6I2T8;	
		formatmap["EVTXSTREAMFMT_V6I2C4N3T2"]=EVTXSTREAMFMT_V6I2C4N3T2;   
		formatmap["EVTXSTREAMFMT_V12I4N6W4T4"]=EVTXSTREAMFMT_V12I4N6W4T4;		// 11

		formatmap["EVTXSTREAMFMT_V12N12T8I4W4"]=EVTXSTREAMFMT_V12N12T8I4W4;		// 12
		formatmap["EVTXSTREAMFMT_V12N12B12T8"]=EVTXSTREAMFMT_V12N12B12T8;	
		formatmap["EVTXSTREAMFMT_V12N12T16C4"]=EVTXSTREAMFMT_V12N12B12T8;		// uhoh ! << this was missing
		formatmap["EVTXSTREAMFMT_V12N12B12T8C4"]=EVTXSTREAMFMT_V12N12T16C4;		// 15
		formatmap["EVTXSTREAMFMT_V12N12B12T16"]=EVTXSTREAMFMT_V12N12B12T8C4;
		formatmap["EVTXSTREAMFMT_V12N12B12T8I4W4"]=EVTXSTREAMFMT_V12N12B12T16;	// 17

		formatmap["EVTXSTREAMFMT_MODELERRIGID"]=EVTXSTREAMFMT_V12N12B12T8I4W4;	// 18

		formatmap["EVTXSTREAMFMT_XP_VCNT"]=EVTXSTREAMFMT_MODELERRIGID;			// 19
		formatmap["EVTXSTREAMFMT_XP_VCNTI"]=EVTXSTREAMFMT_END;
		formatmap["EVTXSTREAMFMT_END"]=EVTXSTREAMFMT_END;
	}
	orkmap<std::string,EVtxStreamFormat>::const_iterator it=formatmap.find(fmtstr);
	EVtxStreamFormat eret = EVTXSTREAMFMT_END;
	if( it!=formatmap.end() ) eret=it->second;
	return eret;
}
////////////////////////////////////////////////////////////


bool XgmModel::LoadUnManaged( XgmModel * mdl, const AssetPath& Filename )	
{
	GfxTarget* pTARG = GfxEnv::GetRef().GetLoaderTarget();

	bool rval = true;
	
	int XGMVERSIONCODE = 0;
	static const int kVERSIONTAG = 0x01234567;
	/////////////////////////////////////////////////////////////
	AssetPath fnameext(Filename);
	fnameext.SetExtension( "xgm" );
	//AssetPath ActualPath = fnameext.ToAbsolute();
	//orkprintf("XgmModel: %s\n", ActualPath.c_str());

	/////////////////////////////////////////////////////////////
	OrkHeapCheck();
	chunkfile::Reader<ModelLoadAllocator> chunkreader( fnameext, "xgm" );
	OrkHeapCheck();
	/////////////////////////////////////////////////////////////
	if( chunkreader.IsOk() )
	{	chunkfile::InputStream* HeaderStream = chunkreader.GetStream("header");
		chunkfile::InputStream* ModelDataStream = chunkreader.GetStream("modeldata");
		/////////////////////////////////////////////////////////
		mdl->msModelName = AddPooledString( Filename.c_str() );
		/////////////////////////////////////////////////////////
		mdl->mbSkinned = false;
		/////////////////////////////////////////////////////////
		PropTypeString ptstring;
		/////////////////////////////////////////////////////////
		int inumjoints = 0;
		HeaderStream->GetItem( inumjoints );
		/////////////////////////////////////////////////////////
		// test for version tag
		/////////////////////////////////////////////////////////
		if( inumjoints == kVERSIONTAG )
		{
			HeaderStream->GetItem( XGMVERSIONCODE );
			HeaderStream->GetItem( inumjoints );
		}
		/////////////////////////////////////////////////////////
		if( inumjoints )
		{	mdl->mSkeleton.SetNumJoints( inumjoints );
			for( int ib=0; ib<inumjoints; ib++ )
			{	int iskelindex = 0, iparentindex=0, ijointname=0, ijointmatrix=0, iinvrestmatrix=0;
				HeaderStream->GetItem( iskelindex ); OrkAssert(ib==iskelindex);
				HeaderStream->GetItem( iparentindex  );
				HeaderStream->GetItem( ijointname  );
				HeaderStream->GetItem( ijointmatrix  );
				HeaderStream->GetItem( iinvrestmatrix  );
				const char* pjntname = chunkreader.GetString(ijointname);

				fxstring<256> jnamp(pjntname);
				//jnamp.replace_in_place("f_idle_","");
				//printf( "FIXUPJOINTNAME<%s:%s>\n", pjntname,jnamp.c_str());
				mdl->mSkeleton.AddJoint( iskelindex , iparentindex, AddPooledString(jnamp.c_str()) );
				ptstring.set(chunkreader.GetString(ijointmatrix));
				mdl->mSkeleton.RefJointMatrix( iskelindex ) = CPropType<CMatrix4>::FromString(ptstring);
				ptstring.set(chunkreader.GetString(iinvrestmatrix));
				mdl->mSkeleton.RefInverseBindMatrix( iskelindex ) = CPropType<CMatrix4>::FromString(ptstring);
			}
		}
		///////////////////////////////////
		// write out flattened bones
		///////////////////////////////////
		int inumbones = 0; 
		HeaderStream->GetItem( inumbones );
		for( int ib=0; ib<inumbones; ib++ )
		{	int iib = 0;
			lev2::XgmBone Bone;
			HeaderStream->GetItem( iib ); OrkAssert(iib==ib);
			HeaderStream->GetItem( Bone.miParent );
			HeaderStream->GetItem( Bone.miChild );
			mdl->mSkeleton.AddFlatBone( Bone );
		}
		if( inumbones )
		{
			mdl->mSkeleton.miRootNode = (inumbones>0) ? mdl->mSkeleton.GetFlattenedBone(0).miParent : -1;
		}
		///////////////////////////////////
		HeaderStream->GetItem( mdl->mBoundingCenter );
		HeaderStream->GetItem( mdl->mAABoundXYZ );
		HeaderStream->GetItem( mdl->mAABoundWHD );
		HeaderStream->GetItem( mdl->mBoundingRadius );
		//END HACK
		///////////////////////////////////
		int inummeshes = 0, inummats = 0;
		HeaderStream->GetItem( mdl->miBonesPerCluster  );
		HeaderStream->GetItem( inummeshes  );
		HeaderStream->GetItem( inummats  );
		///////////////////////////////////
		mdl->mMeshes.reserve( inummeshes );
		///////////////////////////////////
		for( int imat=0; imat<inummats; imat++ )
		{
			int iimat = 0, imatname = 0 , imatclass = 0;
			HeaderStream->GetItem( iimat ); OrkAssert( iimat==imat );
			HeaderStream->GetItem( imatname  );
			HeaderStream->GetItem( imatclass  );
			const char* pmatname = chunkreader.GetString(imatname);
			const char* pmatclassname = chunkreader.GetString(imatclass);
			ork::object::ObjectClass* pmatclass = rtti::autocast( rtti::Class::FindClass(pmatclassname) );

			lev2::GfxMaterial *pmat = 0;

			static const int kdefaulttranssortpass = 100;

			printf( "MODEL USEMATCLASS<%s>\n", pmatclassname );

			/////////////////////////////////////////////////////////////
			// wii (basic) material
			/////////////////////////////////////////////////////////////
			if( pmatclass == GfxMaterialWiiBasic::GetClassStatic() )
			{		
				int ibastek = -1;
				int iblendmode = -1;

				HeaderStream->GetItem( ibastek );

				const char* bastek = chunkreader.GetString(ibastek);

				printf( "MODEL USETEK<%s>\n", bastek );
				//assert(false);
				GfxMaterialWiiBasic *pbasmat = new GfxMaterialWiiBasic( bastek );
				pbasmat->Init( pTARG );
				pmat = pbasmat;
				pmat->SetName( AddPooledString(pmatname) );
				HeaderStream->GetItem( iblendmode );
				const char* blendmodestring = chunkreader.GetString(iblendmode);
				lev2::EBlending eblend = CPropType<lev2::EBlending>::FromString( blendmodestring );
				lev2::RenderQueueSortingData& rqdata = pbasmat->GetRenderQueueSortingData();
				
				if( (eblend!=lev2::EBLENDING_OFF) )
				{	rqdata.miSortingPass = kdefaulttranssortpass;
					pbasmat->mRasterState.SetAlphaTest( EALPHATEST_GREATER, 0.0f );
				}
				pbasmat->mRasterState.SetBlending( eblend );
			}
			/////////////////////////////////////////////////////////////
			// minimal solid material
			/////////////////////////////////////////////////////////////
			else if( pmatclass == lev2::GfxMaterial3DSolid::GetClassStatic() )
			{
				lev2::GfxMaterial3DSolid* pmatsld = new lev2::GfxMaterial3DSolid;
				int imode;
				CVector4 color;
//				float fr, fg, fb, fa;
				HeaderStream->GetItem( imode );
				HeaderStream->GetItem( color );

				//printf( "READCOLOR %f %f %f %f\n", color.GetX(), color.GetY(), color.GetZ(), color.GetW() );

				pmatsld->Init( pTARG );
				pmatsld->SetColorMode( lev2::GfxMaterial3DSolid::EMODE_INTERNAL_COLOR );
				pmatsld->SetColor( color );
				pmat = pmatsld;
				pmat->SetName( AddPooledString(pmatname) );
			}
			/////////////////////////////////////////////////////////////
			// data driven FX material
			/////////////////////////////////////////////////////////////
			else if( pmatclass == lev2::GfxMaterialFx::GetClassStatic() )
			{	lev2::GfxMaterialFx* pmatfx = rtti::autocast( pmatclass->CreateObject() );
				lev2::RenderQueueSortingData& rqdata = pmatfx->GetRenderQueueSortingData();
				rqdata.miSortingPass = 0; //kdefaulttranssortpass;
				pmat = pmatfx;
				pmat->SetName( AddPooledString(pmatname) );
				int iparamcount = -1;
				HeaderStream->GetItem( iparamcount );
				for( int ic=0; ic<iparamcount; ic++ )
				{	int ipt = -1;
					int ipn = -1;
					int ipv = -1;
					HeaderStream->GetItem( ipt );
					HeaderStream->GetItem( ipn );
					HeaderStream->GetItem( ipv );
					const char*  paramname = chunkreader.GetString(ipn);
					const char*  paramval = chunkreader.GetString(ipv);
					//orkprintf( "READXGM paramtype<%d> paramname<%s> paramval<%s>\n", ipt, paramname, paramval );
					EPropType ept = EPropType(ipt);
					GfxMaterialFxParamBase* param = 0;
					switch( ept )
					{	case EPROPTYPE_VEC2REAL:
						{	GfxMaterialFxParamArtist<CVector2> *paramf = new GfxMaterialFxParamArtist<CVector2>;
							paramf->mValue = CPropType<CVector2>::FromString( paramval );
							param = paramf;
							break;
						}
						case EPROPTYPE_VEC3FLOAT:
						{	GfxMaterialFxParamArtist<CVector3> *paramf = new GfxMaterialFxParamArtist<CVector3>;
							paramf->mValue = CPropType<CVector3>::FromString( paramval );
							param = paramf;
							break;
						}
						case EPROPTYPE_VEC4REAL:
						{	GfxMaterialFxParamArtist<CVector4> *paramf = new GfxMaterialFxParamArtist<CVector4>;
							paramf->mValue = CPropType<CVector4>::FromString( paramval );
							param = paramf;
							break;
						}
						case EPROPTYPE_MAT44REAL:
						{	GfxMaterialFxParamArtist<CMatrix4> *paramf = new GfxMaterialFxParamArtist<CMatrix4>;
							paramf->mValue = CPropType<CMatrix4>::FromString( paramval );
							param = paramf;
							break;
						}
						case EPROPTYPE_REAL:
						{	GfxMaterialFxParamArtist<float> *paramf = new GfxMaterialFxParamArtist<float>;
							paramf->mValue = CPropType<float>::FromString( paramval );
							param = paramf;
							orkprintf( "ModelIO::LoadFloatParam mdl<> param<%s> val<%s>\n",paramname, paramval );
							break;
						}
						case EPROPTYPE_S32:
						{	
							////////////////////////////////////////////////////////
							// read artist supplied renderqueue sorting key
							////////////////////////////////////////////////////////
							int ival = CPropType<int>::FromString( paramval );
							if( strcmp(paramname,"ork_rqsort") == 0 )
							{
								rqdata.miSortingOffset = ival;
							}
							else if( strcmp(paramname,"ork_rqsort_pass") == 0 )
							{
								rqdata.miSortingPass = ival;
							}
							else
							{
								GfxMaterialFxParamArtist<int> *paramf = new GfxMaterialFxParamArtist<int>;
								paramf->mValue = ival;
								param = paramf;
							}
							break;
						}
						case EPROPTYPE_SAMPLER:
						{	GfxMaterialFxParamArtist<lev2::Texture*> *paramf = new GfxMaterialFxParamArtist<lev2::Texture*>;
							
							AssetPath texname ( paramval );
							const char* ptexnam = texname.c_str();
							printf( "texname<%s>\n", ptexnam );
							Texture* ptex(NULL);
							if(0 != strcmp( texname.c_str(), "None" ) )
							{
								ork::lev2::TextureAsset* ptexa = asset::AssetManager<TextureAsset>::Create(texname.c_str());
								ptex = ptexa ? ptexa->GetTexture() : 0;
#if defined(_DEBUG)				
								if( ptex )
								{
									ptex->SetProperty( "filename", texname.c_str() );
								}
#endif
							}
							//orkprintf( "ModelIO::LoadTexture mdl<%s> tex<%s> ptex<%p>\n", "", texname.c_str(), ptex );
							paramf->mValue = ptex;
							param = paramf;
							break;
						}
						case EPROPTYPE_STRING:
						{	GfxMaterialFxParamArtist<std::string>* paramstr = new GfxMaterialFxParamArtist<std::string>;
							paramstr->mValue = paramval;
							param = paramstr;
							param->SetBindable(false);
							break;
						}
						default:
							OrkAssert(false);
							break;
					}
					if( param )
					{
						param->GetRecord().mParameterName = paramname;
						pmatfx->AddParameter( param );
					}							
				}
				//pmat->Init( pTARG );
			}
			else // material class not supported in XGM 
			{
				OrkAssert(false);
			}
			
			mdl->AddMaterial( pmat );

			for( int idest=int(ETEXDEST_AMBIENT); idest!=int(ETEXDEST_END); idest++ )
			{	int itexdest = -1;
				HeaderStream->GetItem( itexdest );
				const char* texdest = chunkreader.GetString(itexdest);
				ETextureDest TexDest = CPropType<ETextureDest>::FromString( texdest ); 
				TextureContext & TexCtx = pmat->GetTexture( TexDest );
				int itexname;
				HeaderStream->GetItem( itexname );
				AssetPath texname ( chunkreader.GetString(itexname) );
				Texture* ptex(NULL);
				if(0 != strcmp( texname.c_str(), "None" ) )
				{
					//orkprintf( "Loadtexture<%s>\n", texname.c_str());
					texname.SetUrlBase( Filename.GetUrlBase().c_str() );
					texname.SetFolder( Filename.GetFolder(ork::file::Path::EPATHTYPE_NATIVE).c_str() );
				
					ptex = asset::AssetManager<TextureAsset>::Create(texname.c_str())->GetTexture();
				}
				pmat->SetTexture( TexDest, ptex );
				HeaderStream->GetItem( TexCtx.mfRepeatU );
				HeaderStream->GetItem( TexCtx.mfRepeatV );
			}
		}
		for( int imesh=0; imesh<inummeshes; imesh++ )
		{
			XgmMesh* Mesh = new XgmMesh;

			int itestmeshindex = -1;
			int itestmeshname = -1;
			int imeshnummats = -1;
			int imeshnumsubmeshes = -1;

			HeaderStream->GetItem( itestmeshindex );
			OrkAssert( itestmeshindex==imesh );

			HeaderStream->GetItem( itestmeshname );
			const char* MeshName = chunkreader.GetString(itestmeshname);
			PoolString MeshNamePS = AddPooledString(MeshName);
			Mesh->SetMeshName( MeshNamePS );
			mdl->mMeshes.AddSorted( MeshNamePS, Mesh );

			HeaderStream->GetItem( imeshnumsubmeshes );

			Mesh->ReserveSubMeshes( imeshnumsubmeshes );

			for( int ics=0; ics<imeshnumsubmeshes; ics++ )
			{
				int itestclussetindex = -1, imatname = -1;
				HeaderStream->GetItem( itestclussetindex );
				OrkAssert( ics==itestclussetindex );

				XgmSubMesh* submesh = new XgmSubMesh;
				Mesh->AddSubMesh( submesh );
				XgmSubMesh & CS = * submesh;

				HeaderStream->GetItem( CS.miNumClusters );

				int ilightmapname;
				int ivtxlitflg;

				HeaderStream->GetItem( imatname );
				HeaderStream->GetItem( ilightmapname );
				HeaderStream->GetItem( ivtxlitflg );

				const char* matname = chunkreader.GetString(imatname);
				submesh->mLightMapPath = file::Path(chunkreader.GetString(ilightmapname));

				//////////////////////////////
				// vertex lit or lightmapped ?
				//////////////////////////////
				if( ivtxlitflg )
				{
					submesh->mbVertexLit = true;
				}
				//////////////////////////////
				else if( submesh->mLightMapPath.length() )
				{
					if( CFileEnv::DoesFileExist( submesh->mLightMapPath ) )
					{
						ork::lev2::TextureAsset* plmtexa = asset::AssetManager<TextureAsset>::Create(submesh->mLightMapPath.c_str());
						submesh->mLightMap = (plmtexa==0) ? 0 : plmtexa->GetTexture();	
					}
				}
				//////////////////////////////

				for( int imat=0; imat<mdl->miNumMaterials; imat++ )
				{
					GfxMaterial *pmat = mdl->GetMaterial(imat);
					if( strcmp( pmat->GetName().c_str(), matname )  == 0 )
					{
						CS.mpMaterial = pmat;
					}
				}

				CS.mpClusters = new XgmCluster[ CS.miNumClusters ];
				for( int ic=0; ic<CS.miNumClusters; ic++ )
				{
					int iclusindex = -1;
					int inumbb = -1;
					int ivbformat = -1;
					int ivboffset = -1;
					int ivbnum = -1;
					int ivbsize = -1;
					CVector3 boxmin, boxmax;

					////////////////////////////////////////////////////////////////////////
					HeaderStream->GetItem( iclusindex );
					OrkAssert( ic==iclusindex );
					XgmCluster & Clus = CS.RefCluster( ic );
					HeaderStream->GetItem( Clus.miNumPrimGroups );
					HeaderStream->GetItem( inumbb );
					HeaderStream->GetItem( ivbformat );
					HeaderStream->GetItem( ivboffset );
					HeaderStream->GetItem( ivbnum );
					HeaderStream->GetItem( ivbsize );
					HeaderStream->GetItem( boxmin );
					HeaderStream->GetItem( boxmax );
					////////////////////////////////////////////////////////////////////////
					Clus.mBoundingBox.SetMinMax( boxmin, boxmax );
					Clus.mBoundingSphere = Sphere( boxmin, boxmax );
					////////////////////////////////////////////////////////////////////////
					const char* vbfmt = chunkreader.GetString(ivbformat);
					EVtxStreamFormat efmt = CPropType<EVtxStreamFormat>::FromString( vbfmt );	
					//printf( "XGMLOAD vbfmt<%s> efmt<%d>\n", vbfmt, int(efmt) );
					////////////////////////////////////////////////////////////////////////
					// fix a bug in old files
					if( 0 == XGMVERSIONCODE )
					{
						efmt = GetVersion0VertexStreamFormat(vbfmt);
						//printf( "XGMLOAD(V0FIX) new efmt<%d>\n", efmt );
					}
					////////////////////////////////////////////////////////////////////////
					//lev2::GfxEnv::GetRef().GetGlobalLock().Lock();
					VertexBufferBase *pvb = VertexBufferBase::CreateVertexBuffer( efmt, ivbnum, true );
					void *pverts = (void*) (ModelDataStream->GetDataAt(ivboffset));
					int ivblen = ivbnum * ivbsize;

					printf( "ReadVB NumVerts<%d> VtxSize<%d>\n", ivbnum, pvb->GetVtxSize() );
					void *poutverts = pTARG->GBI()->LockVB( *pvb, 0, ivbnum ); //ivblen );
					{
						memcpy( poutverts, pverts, ivblen );
						pvb->SetNumVertices( ivbnum );
						auto pv = (const SVtxV12N12T8I4W4*) pverts;
						if(efmt==EVTXSTREAMFMT_V12N12T8I4W4)
						{
							for( int iv=0; iv<ivbnum; iv++ )
							{
								auto& v = pv[iv];
								auto& p = v.mPosition;

								printf( " iv<%d> pos<%f %f %f> bi<%08x> bw<%08x>\n", iv, p.GetX(), p.GetY(), p.GetZ(), v.mBoneIndices, v.mBoneWeights );

							}
						}



					}
					pTARG->GBI()->UnLockVB( *pvb );
					//lev2::GfxEnv::GetRef().GetGlobalLock().UnLock();
					Clus.mpVertexBuffer = pvb;
					////////////////////////////////////////////////////////////////////////
					Clus.mpPrimGroups = new XgmPrimGroup[ Clus.miNumPrimGroups ];
					for( int ipg=0; ipg<Clus.miNumPrimGroups; ipg++ )
					{
						int ipgindex = -1;
						int ipgprimtype = -1;
						HeaderStream->GetItem( ipgindex );
						OrkAssert( ipgindex==ipg );

						XgmPrimGroup & PG = Clus.RefPrimGroup(ipg);
						HeaderStream->GetItem( ipgprimtype );
						const char* primtype = chunkreader.GetString(ipgprimtype);
						PG.mePrimType = CPropType<EPrimitiveType>::FromString( primtype );
						HeaderStream->GetItem( PG.miNumIndices );

						int idxdataoffset = -1;
						HeaderStream->GetItem( idxdataoffset );

						U16 *pidx = (U16*) ModelDataStream->GetDataAt(idxdataoffset);

						StaticIndexBuffer<U16> *pidxbuf = new StaticIndexBuffer<U16>(PG.miNumIndices);

						//lev2::GfxEnv::GetRef().GetGlobalLock().Lock();
						void *poutidx = (void*) pTARG->GBI()->LockIB( *pidxbuf );
						{
							// TODO: Make 16-bit indices a policy
							if(PG.miNumIndices > 0xFFFF)
								orkerrorlog("WARNING: <%s> Wii cannot have num indices larger than 65535: MeshName=%s, MatName=%s\n", Filename.c_str(), MeshName, matname);

							memcpy( poutidx, pidx, PG.miNumIndices*sizeof(U16) );
						}
						pTARG->GBI()->UnLockIB( *pidxbuf );
						//lev2::GfxEnv::GetRef().GetGlobalLock().UnLock();

						PG.mpIndices = pidxbuf;
					}
					////////////////////////////////////////////////////////////////////////
					Clus.mJoints.resize( inumbb );
					Clus.mJointSkelIndices.resize(inumbb);
					for( int ib=0; ib<inumbb; ib++ )
					{
						int ibindingindex = -1;
						int ibindingname = -1;

						HeaderStream->GetItem( ibindingindex );
						HeaderStream->GetItem( ibindingname );

						const char* jointname = chunkreader.GetString(ibindingname);
						fxstring<256> jnamp(jointname);
						//jnamp.replace_in_place("f_idle_","");
						//printf( "FIXUPJOINTNAME<%s:%s>\n", jointname,jnamp.c_str());

						PoolString JointNameIndex = FindPooledString( jnamp.c_str() );
						orklut<PoolString,int>::const_iterator itfind = mdl->mSkeleton.mmJointNameMap.find( JointNameIndex );
					
						OrkAssert( itfind != mdl->mSkeleton.mmJointNameMap.end() );
						int iskelindex = (*itfind).second;
						Clus.mJoints[ ib ] = AddPooledString(jnamp.c_str());
						Clus.mJointSkelIndices[ib] = iskelindex;
					}

					mdl->mbSkinned |= (inumbb>0);

					printf( "mdl<%p> mbSkinned<%d>\n", mdl, int(mdl->mbSkinned));
					////////////////////////////////////////////////////////////////////////
				}
			}
		}
	} // if( chunkreader.IsOk() )

	if( rval )
	{
		mdl->InitDisplayLists( pTARG );
	}
	//rval->mSkeleton.dump();
	//mdl->dump();
	OrkHeapCheck();
	return rval;	
}

#else
XgmModel* XgmModel::Load(const std::string& Filename)	
{
	if(ork::tool::CColladaModel* colladaModel = ork::tool::CColladaModel::Load(Filename))
	{
		colladaModel->mXgmModel.mpColladaModel = colladaModel;
		colladaModel->mXgmModel.msModelName = Filename;
		return &colladaModel->mXgmModel;
	}
	
	return NULL;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

bool SaveXGM( const AssetPath& Filename, const lev2::XgmModel *mdl )	
{
	printf( "Writing Xgm<%p> to path<%s>\n", mdl, Filename.c_str() );
	EndianContext* pendianctx = 0;
	
	bool bwii = (0!=strstr(Filename.c_str(),"wii"));
	bool bxb360 = (0!=strstr(Filename.c_str(),"xb360"));

	lev2::GfxTargetDummy DummyTarget;

	if( bwii||bxb360 )
	{
		pendianctx = new EndianContext;
		pendianctx->mendian = ork::EENDIAN_BIG;
	}

	///////////////////////////////////
	chunkfile::Writer chunkwriter( "xgm" );
	///////////////////////////////////
	chunkfile::OutputStream* HeaderStream = chunkwriter.AddStream("header");
	chunkfile::OutputStream* ModelDataStream = chunkwriter.AddStream("modeldata");

	///////////////////////////////////
	// write out new VERSION code
	int32_t iVERSIONTAG = 0x01234567;
	int32_t iVERSION = 1;
	HeaderStream->AddItem( iVERSIONTAG );
	HeaderStream->AddItem( iVERSION );	
	printf( "WriteXgm<%s> VERSION<%d>\n", Filename.c_str(), iVERSION );
	///////////////////////////////////
	// write out Joints 

	const lev2::XgmSkeleton& skel = mdl->RefSkel();

	int32_t inumjoints = skel.GetNumJoints();

	

	HeaderStream->AddItem( inumjoints );

	int32_t istring;
	printf( "WriteXgm<%s> numjoints<%d>\n", Filename.c_str(), inumjoints );

	for( int32_t ib=0; ib<inumjoints; ib++ )
	{
		const PoolString & JointName = skel.GetJointName(ib);
		int32_t JointParentIndex = skel.GetJointParent(ib);
		const CMatrix4 & InvRestMatrix = skel.RefInverseBindMatrix(ib);
		const CMatrix4 & JointMatrix = skel.RefJointMatrix(ib);
		
		HeaderStream->AddItem( ib  );
		HeaderStream->AddItem( JointParentIndex  );
		istring = chunkwriter.GetStringIndex(JointName.c_str());
		HeaderStream->AddItem( istring  );

		PropTypeString tstr;
		CPropType<CMatrix4>::ToString( JointMatrix, tstr );
		istring = chunkwriter.GetStringIndex(tstr.c_str());
		HeaderStream->AddItem( istring  );

		CPropType<CMatrix4>::ToString( InvRestMatrix, tstr );
		istring = chunkwriter.GetStringIndex(tstr.c_str());
		HeaderStream->AddItem( istring  );
	}

	///////////////////////////////////
	// write out flattened bones

	int32_t inumbones = skel.GetNumBones();

	HeaderStream->AddItem( inumbones );

	printf( "WriteXgm<%s> numbones<%d>\n", Filename.c_str(), inumbones );
	for( int32_t ib=0; ib<inumbones; ib++ )
	{
		const lev2::XgmBone & Bone = skel.GetFlattenedBone( ib );

		HeaderStream->AddItem( ib );
		HeaderStream->AddItem( Bone.miParent );
		HeaderStream->AddItem( Bone.miChild );
	}

	///////////////////////////////////

	int32_t inummeshes = mdl->GetNumMeshes();
	int32_t inummats   = mdl->GetNumMaterials();

	printf( "WriteXgm<%s> nummeshes<%d>\n", Filename.c_str(), inummeshes );
	printf( "WriteXgm<%s> nummtls<%d>\n", Filename.c_str(), inummats );

	const CVector3& bc = mdl->GetBoundingCenter();
	float br = mdl->GetBoundingRadius();
	const CVector3& bbxyz = mdl->GetBoundingAA_XYZ();
	const CVector3&	bbwhd = mdl->GetBoundingAA_WHD();

	HeaderStream->AddItem( bc.GetX()  );
	HeaderStream->AddItem( bc.GetY()  );
	HeaderStream->AddItem( bc.GetZ()  );
	HeaderStream->AddItem( bbxyz.GetX() );
	HeaderStream->AddItem( bbxyz.GetY() );
	HeaderStream->AddItem( bbxyz.GetZ() );
	HeaderStream->AddItem( bbwhd.GetX() );
	HeaderStream->AddItem( bbwhd.GetY() );
	HeaderStream->AddItem( bbwhd.GetZ() );
	HeaderStream->AddItem( br );

	HeaderStream->AddItem( mdl->GetBonesPerCluster()  );
	HeaderStream->AddItem( inummeshes  );
	HeaderStream->AddItem( inummats  );

	std::set<std::string> ParameterIgnoreSet;
	ParameterIgnoreSet.insert( "binMembership" );
	ParameterIgnoreSet.insert( "colorSource" );
	ParameterIgnoreSet.insert( "texCoordSource" );
	ParameterIgnoreSet.insert( "uniformParameters" );
	ParameterIgnoreSet.insert( "varyingParameters" );

	for( int32_t imat=0; imat<inummats; imat++ )
	{
		const lev2::GfxMaterial *pmat = mdl->GetMaterial(imat);
		HeaderStream->AddItem( imat  );
		istring = chunkwriter.GetStringIndex(pmat->GetName().c_str());
		HeaderStream->AddItem( istring  );

		rtti::Class* pclass = pmat->GetClass();
		const PoolString& classname = pclass->Name();
		const char* pclassname = classname.c_str();

		printf( "WriteXgm<%s> material<%d> class<%s> name<%s>\n", Filename.c_str(), imat, pclassname, pmat->GetName().c_str() );
		istring = chunkwriter.GetStringIndex(classname.c_str());
		HeaderStream->AddItem( istring  );

		//orkprintf( "Material Name<%s> Class<%s>\n", pmat->GetName().c_str(), classname.c_str() );
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		// basic materials (fixed, simple materials
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		if( pmat->GetClass()->IsSubclassOf( lev2::GfxMaterialWiiBasic::GetClassStatic() ) )
		{
			const lev2::GfxMaterialWiiBasic *pbasmat = rtti::safe_downcast<const lev2::GfxMaterialWiiBasic*>( pmat );
			istring = chunkwriter.GetStringIndex(pbasmat->GetBasicTechName().c_str());
			HeaderStream->AddItem( istring  );


			lev2::EBlending eblend = pbasmat->mRasterState.GetBlending();

			PropTypeString BlendModeString;
			CPropType<lev2::EBlending>::ToString( eblend,BlendModeString );

			istring = chunkwriter.GetStringIndex(BlendModeString.c_str());
			HeaderStream->AddItem( istring  );
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		// solid material 
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		else if( pmat->GetClass()->IsSubclassOf( lev2::GfxMaterial3DSolid::GetClassStatic() ) )
		{
			const lev2::GfxMaterial3DSolid *pbasmat = rtti::safe_downcast<const lev2::GfxMaterial3DSolid*>( pmat );

			int32_t imode = int(pbasmat->GetColorMode());	
			const CVector4& clr = pbasmat->GetColor();
			
			HeaderStream->AddItem( imode  );

			HeaderStream->AddItem( clr );

		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		// fx materials (data driven materials)
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		else if( pmat->GetClass()->IsSubclassOf( lev2::GfxMaterialFx::GetClassStatic() ) )
		{
			const lev2::GfxMaterialFx *pfxmat = rtti::safe_downcast<const lev2::GfxMaterialFx*>( pmat );
			const lev2::GfxMaterialFxEffectInstance& fxinst = pfxmat->GetEffectInstance();
			const orklut<std::string,lev2::GfxMaterialFxParamBase*>	& parms = fxinst.mParameterInstances;

			int32_t iparamcount = 0;

			for( orklut<std::string,lev2::GfxMaterialFxParamBase*>::const_iterator itf=parms.begin(); itf!=parms.end(); itf++ )
			{
				const std::string& paramname = itf->first;
				const lev2::GfxMaterialFxParamBase* pbase = itf->second;

				bool bignore = ParameterIgnoreSet.find( paramname ) != ParameterIgnoreSet.end();

				if( false == bignore )
				{
					iparamcount++;
				}
			}
			HeaderStream->AddItem( iparamcount );

			for( orklut<std::string,lev2::GfxMaterialFxParamBase*>::const_iterator itf=parms.begin(); itf!=parms.end(); itf++ )
			{
				const std::string& paramname = itf->first;
				const lev2::GfxMaterialFxParamBase* pbase = itf->second;

				bool bignore = ParameterIgnoreSet.find( paramname ) != ParameterIgnoreSet.end();

				if( false == bignore )
				{
					EPropType etype = pbase->GetRecord().meParameterType;

					std::string valstr = pbase->GetValueString();

					printf( "SaveXGM paramtype<%d> paramname<%s> valstr<%s>\n", int(etype), paramname.c_str(), valstr.c_str() );

					const lev2::GfxMaterialFxParamArtist<lev2::Texture*>* ptexparam = rtti::autocast(pbase);

					if( ptexparam )
					{
						const char* ptexnam = pbase->GetInitString().c_str();
						std::string tmpstr(ptexnam);
						printf( "texname<%s>\n", ptexnam );
						#ifdef WIN32
						std::string::size_type loc = tmpstr.find("data\\src\\");
						#else
						std::string::size_type loc = tmpstr.find("data/src/");
						#endif
						if(loc == std::string::npos)
						{
							orkerrorlog("ERROR: Output texture path is outside of 'data\\src\\'! (%s)\n", tmpstr.c_str());
							return false;
						}
						tmpstr = std::string("data://") + tmpstr.substr(loc + 9);
						for(std::string::size_type i = 0; i < tmpstr.length(); i++)
							if(tmpstr[i] == '\\')
								tmpstr[i] = '/';
						file::Path AsPath(tmpstr.c_str());
						AsPath.SetExtension( "" );
						valstr = AsPath.c_str();

						printf( "SaveXGM paramtype<%d> paramname<%s> valstr<%s>\n", int(etype), paramname.c_str(), valstr.c_str() );

					}
					
					HeaderStream->AddItem( int32_t(etype) );
					istring = chunkwriter.GetStringIndex(paramname.c_str());
					HeaderStream->AddItem( istring );
					istring = chunkwriter.GetStringIndex(valstr.c_str());
					HeaderStream->AddItem( istring );
				}


			}
				
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		// material class not supported for XGM
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		else
		{
			OrkAssert(false);
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////

		for( int32_t idest=int(lev2::ETEXDEST_AMBIENT); idest!=int(lev2::ETEXDEST_END); idest++ )
		{
			lev2::ETextureDest edest = lev2::ETextureDest(idest);

			const lev2::TextureContext & TexCtx = pmat->GetTexture( edest );

			PropTypeString tstr;
			CPropType<lev2::ETextureDest>::ToString( edest, tstr ); 
			std::string TexDest = tstr.c_str();
			
			std::string TexName = "None";
			if( TexCtx.mpTexture )
			{
				const std::string & tname = TexCtx.mpTexture->GetProperty( "abspath" ) ;
				ork::AssetPath pth( tname.c_str() );
				pth.SetExtension("");
				pth.SetUrlBase("");
				pth.SetFolder("");
				TexName = pth.c_str();
			}

			if( TexName != "None" )
			{
				//orkprintf( " WRITING slot<%s> texname<%s>\n", tstr.c_str(), TexName.c_str() );
			}

			istring = chunkwriter.GetStringIndex(TexDest.c_str());
			HeaderStream->AddItem( istring );
			istring = chunkwriter.GetStringIndex(TexName.c_str());
			HeaderStream->AddItem( istring );
			HeaderStream->AddItem( TexCtx.mfRepeatU );
			HeaderStream->AddItem( TexCtx.mfRepeatV );
		}

	}

	for( int32_t imesh=0; imesh<inummeshes; imesh++ )
	{
		const lev2::XgmMesh & Mesh = * mdl->GetMesh(imesh);

		int32_t inumsubmeshes = Mesh.GetNumSubMeshes();

		HeaderStream->AddItem( imesh  );
		istring = chunkwriter.GetStringIndex(Mesh.GetMeshName().c_str());
		HeaderStream->AddItem( istring  );
		HeaderStream->AddItem( inumsubmeshes  );

		printf( "WriteXgm<%s> mesh<%d:%s> numsubmeshes<%d>\n", Filename.c_str(), imesh, Mesh.GetMeshName().c_str(), inumsubmeshes );
		for( int32_t ics=0; ics<inumsubmeshes; ics++ )
		{
			const lev2::XgmSubMesh & CS = *Mesh.GetSubMesh( ics );
			const lev2::GfxMaterial *pmat = CS.GetMaterial();

			int32_t inumclus = CS.GetNumClusters();

			int32_t inumenabledclus = 0;

			for( int ic=0; ic<inumclus; ic++ )
			{
				const lev2::XgmCluster & Clus = CS.RefCluster( ic );
				const lev2::VertexBufferBase * VB = Clus.mpVertexBuffer;

				if(!VB)
					return false;

				if( VB->GetNumVertices() > 0 )
				{
					inumenabledclus++;
				}
				else
				{
					orkprintf( "WARNING: material<%s> cluster<%d> has a zero length vertex buffer, skipping\n",
								pmat->GetName().c_str(), ic );
				}
			}

			HeaderStream->AddItem( ics  );
			HeaderStream->AddItem( inumenabledclus  );

			printf( "WriteXgm<%s>  submesh<%d> numenaclus<%d>\n", Filename.c_str(), ics, inumenabledclus );
			////////////////////////////////////////////////////////////
			istring = chunkwriter.GetStringIndex(pmat ? pmat->GetName().c_str() : "None");
			HeaderStream->AddItem( istring  );
			////////////////////////////////////////////////////////////
			const file::Path& LightMapPath = CS.mLightMapPath;
			istring = chunkwriter.GetStringIndex(LightMapPath.c_str());
			HeaderStream->AddItem( istring  );
			////////////////////////////////////////////////////////////
			int32_t ivtxlitflg = 0;
			if( CS.mbVertexLit ) ivtxlitflg=1;
			HeaderStream->AddItem( ivtxlitflg  );
			////////////////////////////////////////////////////////////
			for( int32_t ic=0; ic<inumclus; ic++ )
			{
				const lev2::XgmCluster & Clus = CS.RefCluster( ic );
				const lev2::VertexBufferBase * VB = Clus.mpVertexBuffer;
				lev2::VertexBufferBase * VBNC = const_cast<lev2::VertexBufferBase*>( VB );
				const Sphere& clus_sphere = Clus.mBoundingSphere;
				const AABox& clus_box = Clus.mBoundingBox;

				if( VB->GetNumVertices() == 0 ) continue;
				
				int32_t inumpg = Clus.GetNumPrimGroups();
				int32_t inumjb = (int) Clus.GetNumJointBindings();
				

				PropTypeString tstr;
				CPropType<lev2::EVtxStreamFormat>::ToString( VB->GetStreamFormat(), tstr );
				std::string VertexFmt = tstr.c_str();

				int32_t ivbufoffset = ModelDataStream->GetSize();
				const u8* VBdata = (const u8*) DummyTarget.GBI()->LockVB( *VB );
				OrkAssert( VBdata!=0 );
				{

					int VBlen = VB->GetNumVertices()*VB->GetVtxSize();

					printf( "WriteVB NumVerts<%d> VtxSize<%d>\n", VB->GetNumVertices(), VB->GetVtxSize() );

					HeaderStream->AddItem( ic  );
					HeaderStream->AddItem( inumpg  );
					HeaderStream->AddItem( inumjb  );

					istring = chunkwriter.GetStringIndex(VertexFmt.c_str());
					HeaderStream->AddItem( istring  );
					HeaderStream->AddItem( ivbufoffset  );
					HeaderStream->AddItem( VB->GetNumVertices() );
					HeaderStream->AddItem( VB->GetVtxSize() );

					HeaderStream->AddItem( clus_box.Min()  );
					HeaderStream->AddItem( clus_box.Max()  );

					//VBNC->EndianSwap();

					ModelDataStream->Write( VBdata, VBlen );
				}
				DummyTarget.GBI()->UnLockVB( *VB );

				for( int32_t ipg=0; ipg<inumpg; ipg++ )
				{	
					const lev2::XgmPrimGroup & PG = Clus.RefPrimGroup( ipg );

					CPropType<lev2::EPrimitiveType>::ToString( PG.GetPrimType(), tstr );	
					std::string PrimType = tstr.c_str();

					int32_t inumidx = PG.GetNumIndices();

					printf( "WritePG<%d> NumIndices<%d>\n", ipg, inumidx );

					HeaderStream->AddItem( ipg  );
					istring = chunkwriter.GetStringIndex(PrimType.c_str());
					HeaderStream->AddItem( istring  );
					HeaderStream->AddItem( inumidx  );
					HeaderStream->AddItem( ModelDataStream->GetSize() );

					//////////////////////////////////////////////////
					U16 *pidx = (U16*) DummyTarget.GBI()->LockIB( *PG.GetIndexBuffer() ); //->GetDataPointer();
					OrkAssert(pidx!=0);
					for( int32_t ii=0; ii<inumidx; ii++ )
					{
						int32_t iv = int32_t(pidx[ii]);
						if( iv >= VB->GetNumVertices() )
						{
							orkprintf( "index id<%d> val<%d> is > vertex count<%d>\n", ii, iv, VB->GetNumVertices() );
						}
						OrkAssert(iv<VB->GetNumVertices());

						//swapbytes_dynamic<U16>( pidx[ii] ); 
					}
					DummyTarget.GBI()->UnLockIB( *PG.GetIndexBuffer() ); 
					//////////////////////////////////////////////////

					ModelDataStream->Write( (const unsigned char *) pidx, inumidx*sizeof(U16) );
				}

				for( int32_t ij=0; ij<inumjb; ij++ )
				{
					const PoolString & bound = Clus.GetJointBinding( ij );
					HeaderStream->AddItem( ij  );
					istring = chunkwriter.GetStringIndex(bound.c_str());
					HeaderStream->AddItem( istring  );
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////

	//file::Path outpath = Filename;
//	outpath.SetExtension( "xgm" );
	chunkwriter.WriteToFile( Filename );

	////////////////////////////////////////////////////////////////////////////////////

	if( pendianctx )
	{
		delete pendianctx;
	}

	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

}}

template class ork::chunkfile::Reader<ork::lev2::ModelLoadAllocator>;

