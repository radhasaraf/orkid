///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2010, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

#include <orktool/orktool_pch.h>
#include <ork/math/plane.h>
#include <orktool/filter/gfx/meshutil/meshutil.h>

///////////////////////////////////////////////////////////////////////////////
#include <FCollada.h>
#include <FCDocument/FCDocument.h>
#include <FCDocument/FCDLibrary.h>
#include <FCDocument/FCDExtra.h>
#include <FCDocument/FCDGeometry.h>
#include <FCDocument/FCDMaterial.h>
#include <FCDocument/FCDEffect.h>
#include <FCDocument/FCDEffectStandard.h>
#include <FCDocument/FCDEffectProfile.h>
#include <FCDocument/FCDGeometrySource.h>
#include <FCDocument/FCDGeometryMesh.h>
#include <FCDocument/FCDGeometryInstance.h>
#include <FCDocument/FCDGeometryPolygons.h>
#include <FCDocument/FCDGeometryPolygonsInput.h>
//#include <FCDocument/FCDGeometryPolygonsTools.h> // For Triagulate
#include <FCDocument/FCDEntityInstance.h>
#include <FCDocument/FCDSceneNode.h>
#include <FCDocument/FCDAsset.h>
#include <orktool/filter/gfx/collada/collada.h>

#if defined(USE_FCOLLADA)
#include <orktool/filter/gfx/collada/daeutil.h>
#include <boost/thread.hpp>
bool ParseColladaMaterialBindings( FCDocument& daedoc, orkmap<std::string,std::string>& MatSemMap );

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace MeshUtil {
///////////////////////////////////////////////////////////////////////////////

struct DaeNodeQ
{
	const std::string		mName;
	const DaeExtraNode*		mNode;
	FCDENode*				mParent;
	DaeNodeQ( const std::string& name, const DaeExtraNode* node, FCDENode* parent ) : mName(name), mNode(node), mParent(parent) {}
};

static void CreateDaeMesh(
							FCDocument& daedoc,
							FCDSceneNode* Child,
							const std::string& grpname,
							FCDMaterial* DaeMat,
							const submesh& pgroup,
							const DaeExtraNode* extranodes )
{
	const fm::string GrpNameFmStr(grpname.c_str());

	fm::string str_geo_id = GrpNameFmStr + "_GeoDaeId";
	fm::string str_geo_nm = GrpNameFmStr+ "_GeoDaeName";

	fm::string str_possrc_id = GrpNameFmStr+ "_PosSrcId";
	fm::string str_possrc_nm = GrpNameFmStr+ "_PosSrcName";
	
	fm::string str_clrsrc0_id = GrpNameFmStr+ "_ClrSrc0Id";
	fm::string str_clrsrc0_nm = GrpNameFmStr+ "_ClrSrc0Name";
	fm::string str_clrsrc1_id = GrpNameFmStr+ "_ClrSrc1Id";
	fm::string str_clrsrc1_nm = GrpNameFmStr+ "_ClrSrc1Name";

	fm::string str_nrmsrc_id = GrpNameFmStr+ "_NrmSrcId";
	fm::string str_nrmsrc_nm = GrpNameFmStr+ "_NrmSrcName";

	fm::string str_binsrc_id = GrpNameFmStr+ "_BinSrcId";
	fm::string str_binsrc_nm = GrpNameFmStr+ "_BinSrcName";

	fm::string str_uv0src_id = GrpNameFmStr+ "_Uv0SrcId";
	fm::string str_uv0src_nm = GrpNameFmStr+ "_Uv0SrcName";
	fm::string str_uv1src_id = GrpNameFmStr+ "_Uv1SrcId";
	fm::string str_uv1src_nm = GrpNameFmStr+ "_Uv1SrcName";

	FCDGeometryLibrary *GeoLib = daedoc.GetGeometryLibrary();
	FCDGeometry* DaeGeo = GeoLib->AddEntity();
	DaeGeo->SetDaeId( str_geo_id );
	DaeGeo->SetName( str_geo_nm );
	FCDGeometryMesh* DaeMesh = DaeGeo->CreateMesh();
	///////////////////////////////////////////////////
	FCDGeometrySource* DaePosSrc = DaeMesh->AddSource( FUDaeGeometryInput::POSITION );
	DaePosSrc->SetDaeId( str_possrc_id );
	DaePosSrc->SetName( str_possrc_nm );
	DaeMesh->AddVertexSource(DaePosSrc);
	///////////////////////////////////////////////////
	FCDGeometrySource* DaeClrSrc0 = DaeMesh->AddSource( FUDaeGeometryInput::COLOR );
	DaeClrSrc0->SetDaeId( str_clrsrc0_id );
	DaeClrSrc0->SetName( str_clrsrc0_nm );
	DaeMesh->AddVertexSource(DaeClrSrc0);
	///////////////////////////////////////////////////
	FCDGeometrySource* DaeClrSrc1 = DaeMesh->AddSource( FUDaeGeometryInput::COLOR );
	DaeClrSrc1->SetDaeId( str_clrsrc1_id );
	DaeClrSrc1->SetName( str_clrsrc1_nm );
	DaeMesh->AddVertexSource(DaeClrSrc1);
	///////////////////////////////////////////////////
	FCDGeometrySource* DaeNrmSrc = DaeMesh->AddSource( FUDaeGeometryInput::NORMAL );
	DaeNrmSrc->SetDaeId( str_nrmsrc_id );
	DaeNrmSrc->SetName( str_nrmsrc_nm );
	DaeMesh->AddVertexSource(DaeNrmSrc);
	///////////////////////////////////////////////////
	FCDGeometrySource* DaeBinSrc = DaeMesh->AddSource( FUDaeGeometryInput::TEXBINORMAL );
	DaeBinSrc->SetDaeId( str_binsrc_id );
	DaeBinSrc->SetName( str_binsrc_nm );
	DaeMesh->AddVertexSource(DaeBinSrc);
	///////////////////////////////////////////////////
	FCDGeometrySource* DaeUv0Src = DaeMesh->AddSource( FUDaeGeometryInput::TEXCOORD );
	DaeUv0Src->SetDaeId( str_uv0src_id );
	DaeUv0Src->SetName( str_uv0src_nm );
	DaeMesh->AddVertexSource(DaeUv0Src);
	///////////////////////////////////////////////////
	FCDGeometrySource* DaeUv1Src = DaeMesh->AddSource( FUDaeGeometryInput::TEXCOORD );
	DaeUv1Src->SetDaeId( str_uv1src_id );
	DaeUv1Src->SetName( str_uv1src_nm );
	DaeMesh->AddVertexSource(DaeUv1Src);

	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	FCDGeometryPolygons* DaePolys = DaeMesh->AddPolygons();
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////

	///////////////////////////////////////////////////
	// find extra nodes
	///////////////////////////////////////////////////

	if( extranodes )
	{
		FCDExtra* DaeExtra = DaePolys->GetExtra();
		FCDEType* DaeType = DaeExtra->GetDefaultType();
		FCDETechnique* DaeTech = DaeType->AddTechnique("MiniOrk");
		FCDENode* parameterTree = DaeTech->AddChildNode();
		parameterTree->SetName("Root");

		orkstack<DaeNodeQ> NodeStack;
		NodeStack.push(DaeNodeQ("",extranodes,parameterTree));

		while( NodeStack.empty() == false )
		{
			DaeNodeQ top = NodeStack.top();
			NodeStack.pop();
			const DaeExtraNode* node = top.mNode;
			FCDENode* parent = top.mParent;

			if( node->mChildren.size()>0  )
			{
				for( const auto& it : node->mChildren )
				{
					FCDENode* newparent = parent->AddChildNode( it.first.c_str() );
					
					if( it.second->mChildren.size() )
					{

						NodeStack.push(DaeNodeQ(it.first,it.second,newparent));
					}
					else
					{
						//FCDENode* firstParameter = newparent->AddChildNode();
						newparent->SetContent(FS(it.second->mValue.c_str()));
					}
				}
			}
			else if( node->mValue.length()>0 )
			{
			}
			
					//firstParameter->AddAttribute("Guts", 0);
		}
	}

	///////////////////////////////////////////////////

	const orkvector<poly>& polys = pgroup.RefPolys();
	int inump = polys.size();
	orkprintf( "NumPolys<%d>\n", inump );

	///////////////////////////////////////////////////

	DaePolys->SetMaterialSemantic( DaeMat->GetDaeId() );
	int inumv = pgroup.RefVertexPool().GetNumVertices();
	//int inumtri = GetNumBaseTriangles();
	//int inumqua = GetNumBaseQuads();
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	//const vertex& vtxbase =  NewVtxPool.GetVertex(0);
	FloatList daeout_pos;daeout_pos.reserve(inumv*3);
	FloatList daeout_cl0;daeout_cl0.reserve(inumv*4);
	FloatList daeout_cl1;daeout_cl1.reserve(inumv*4);
	FloatList daeout_nrm;daeout_nrm.reserve(inumv*3);
	FloatList daeout_bin;daeout_bin.reserve(inumv*3);
	FloatList daeout_uv0;daeout_uv0.reserve(inumv*2);
	FloatList daeout_uv1;daeout_uv1.reserve(inumv*2);
	///////////////////////////////////////////////////
	for( int iv0=0; iv0<inumv; iv0++ )
	{	
		const vertex& invtx =  pgroup.RefVertexPool().GetVertex(iv0);
		///////////////////////////////////////////////
		daeout_pos.push_back( invtx.mPos.GetX() );
		daeout_pos.push_back( invtx.mPos.GetY() );
		daeout_pos.push_back( invtx.mPos.GetZ() );
		///////////////////////////////////////////////
		daeout_cl0.push_back( invtx.mCol[0].GetX() );
		daeout_cl0.push_back( invtx.mCol[0].GetY() );
		daeout_cl0.push_back( invtx.mCol[0].GetZ() );
		daeout_cl0.push_back( invtx.mCol[0].GetW() );
		///////////////////////////////////////////////
		daeout_cl1.push_back( invtx.mCol[1].GetX() );
		daeout_cl1.push_back( invtx.mCol[1].GetY() );
		daeout_cl1.push_back( invtx.mCol[1].GetZ() );
		daeout_cl1.push_back( invtx.mCol[1].GetW() );
		///////////////////////////////////////////////
		daeout_nrm.push_back( invtx.mNrm.GetX() );
		daeout_nrm.push_back( invtx.mNrm.GetY() );
		daeout_nrm.push_back( invtx.mNrm.GetZ() );
		///////////////////////////////////////////////
		daeout_bin.push_back( invtx.mUV[0].mMapBiNormal.GetX() );
		daeout_bin.push_back( invtx.mUV[0].mMapBiNormal.GetY() );
		daeout_bin.push_back( invtx.mUV[0].mMapBiNormal.GetZ() );
		///////////////////////////////////////////////
		daeout_uv0.push_back( invtx.mUV[0].mMapTexCoord.GetX() );
		daeout_uv0.push_back( invtx.mUV[0].mMapTexCoord.GetY() );
		///////////////////////////////////////////////
		daeout_uv1.push_back( invtx.mUV[1].mMapTexCoord.GetX() );
		daeout_uv1.push_back( invtx.mUV[1].mMapTexCoord.GetY() );
		///////////////////////////////////////////////
	}
	///////////////////////////////////////////////////
	DaePosSrc->SetData( daeout_pos, 3, 0, 0 );
	DaeClrSrc0->SetData( daeout_cl0, 4, 0, 0 );
	DaeClrSrc1->SetData( daeout_cl1, 4, 0, 0 );
	DaeNrmSrc->SetData( daeout_nrm, 3, 0, 0 );
	DaeBinSrc->SetData( daeout_bin, 3, 0, 0 );
	DaeUv0Src->SetData( daeout_uv0, 2, 0, 0 );
	DaeUv1Src->SetData( daeout_uv1, 2, 0, 0 );
	///////////////////////////////////////////////////
	FCDGeometryPolygonsInput* idcp_pos = DaePolys->FindInput( DaePosSrc );
	FCDGeometryPolygonsInput* idcp_clr0 = DaePolys->FindInput( DaeClrSrc0 );
	FCDGeometryPolygonsInput* idcp_clr1 = DaePolys->FindInput( DaeClrSrc1 );
	FCDGeometryPolygonsInput* idcp_nrm = DaePolys->FindInput( DaeNrmSrc );
	FCDGeometryPolygonsInput* idcp_bin = DaePolys->FindInput( DaeBinSrc );
	FCDGeometryPolygonsInput* idcp_uv0 = DaePolys->FindInput( DaeUv0Src );
	FCDGeometryPolygonsInput* idcp_uv1 = DaePolys->FindInput( DaeUv1Src );
	///////////////////////////////////////////////////

	orkprintf( "NumPolys<%d>\n", inump );

	for( orkvector<poly>::const_iterator itp=polys.begin(); itp!=polys.end(); itp++  )
	{
		const poly& ply = *itp;

		int inumsides = ply.GetNumSides();

		switch( inumsides )
		{
			case 3:
			case 4:
			{
				int idxbase = idcp_pos->GetIndexCount(); 
				DaePolys->AddFace(inumsides);
				for( int iv=0; iv<inumsides; iv++ )
				{
					int iraw_idx = ply.GetVertexID(iv);
					idcp_pos->GetIndices()[ idxbase+iv ] = iraw_idx;
					idcp_clr0->GetIndices()[ idxbase+iv ] = iraw_idx;
					idcp_clr1->GetIndices()[ idxbase+iv ] = iraw_idx;
					idcp_nrm->GetIndices()[ idxbase+iv ] = iraw_idx;
					idcp_uv0->GetIndices()[ idxbase+iv ] = iraw_idx;
					idcp_uv1->GetIndices()[ idxbase+iv ] = iraw_idx;
				}
				break;
			}
		}

	}

	FCDGeometryInstance* entinst = static_cast<FCDGeometryInstance*>( Child->AddInstance( DaeGeo ) );
	entinst->AddMaterialInstance( DaeMat, DaePolys );
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
static void CloneParam( FCDEffectProfileFX* pfx, const FCDEffectParameter* param )
{
	const T *porig = static_cast<const T*>( param );
	T* NewParam = (T*) pfx->AddEffectParameter(param->GetType());
	NewParam->SetSemantic( porig->GetSemantic() );
	NewParam->SetReference( porig->GetReference() );
	NewParam->SetValue( porig->GetValue() );
	if( porig->IsGenerator() ) NewParam->SetGenerator(  );
	if( porig->IsModifier() ) NewParam->SetModifier(  );
	if( porig->IsAnimator() ) NewParam->SetAnimator(  );
	if( porig->IsReferencer() ) NewParam->SetReferencer(  );
	if( porig->IsConstant() ) NewParam->SetConstant(  );
}

///////////////////////////////////////////////////////////////////////////////

FCDMaterial* AddDaeShader( const toolmesh& tmesh, FCDocument& daedoc, std::string baseName, const DaeWriteOpts& writeopts )
{
	///////////////////////////////////////////////////
	FCDEffectLibrary* EfxLib = daedoc.GetEffectLibrary();
	FCDMaterial* DaeMat = 0;

	switch( writeopts.meMaterialSetup )
	{
		case DaeWriteOpts::EMS_NOMATERIALS:
		{	FCDEffect* DaeEfx = EfxLib->AddEntity();
			FCDEffectProfile* DaeEfxProf = DaeEfx->AddProfile( FUDaeProfileType::COMMON );
			FCDEffectStandard* EfxStd = (FCDEffectStandard*) DaeEfxProf;
			///////////////////////////////////////////////////
			EfxStd->SetLightingType( FCDEffectStandard::LAMBERT );
			DaeEfx->SetDaeId( CreateFormattedString( "%s-effect", baseName.c_str() ).c_str() );
			//DaeEfx->SetName( CreateFormattedString( "%s", baseName.c_str() ).c_str() );
			FMVector4 white( 1.0f,1.0f,1.0f,1.0f);
			EfxStd->SetAmbientColor( white );
			///////////////////////////////////////////////////
			FCDMaterialLibrary *MatLib = daedoc.GetMaterialLibrary();
			DaeMat = MatLib->AddEntity();
			DaeMat->SetDaeId( CreateFormattedString( "%s", baseName.c_str() ).c_str() );
			//DaeMat->SetName( CreateFormattedString( "%s", baseName.c_str() ).c_str() );
			DaeMat->SetEffect( DaeEfx );
			break;
		}
		case DaeWriteOpts::EMS_DEFTEXMATERIALS:
		{	FCDEffect* DaeEfx = EfxLib->AddEntity();
			FCDEffectProfile* DaeEfxProf = DaeEfx->AddProfile( FUDaeProfileType::COMMON );
			FCDEffectStandard* EfxStd = (FCDEffectStandard*) DaeEfxProf;
			///////////////////////////////////////////////////
			EfxStd->SetLightingType( FCDEffectStandard::LAMBERT );
			DaeEfx->SetDaeId( CreateFormattedString( "%s-effect", baseName.c_str() ).c_str() );
			//DaeEfx->SetName( CreateFormattedString( "%s", baseName.c_str() ).c_str() );
			FMVector4 white( 1.0f,1.0f,1.0f,1.0f);
			EfxStd->SetAmbientColor( white );
			///////////////////////////////////////////////////
			FCDMaterialLibrary *MatLib = daedoc.GetMaterialLibrary();
			DaeMat = MatLib->AddEntity();
			DaeMat->SetDaeId( CreateFormattedString( "%s", baseName.c_str() ).c_str() );
			//DaeMat->SetName( CreateFormattedString( "%s", baseName.c_str() ).c_str() );
			DaeMat->SetEffect( DaeEfx );
			std::string imgpth = CreateFormattedString( "./%s_%s.tga",
				writeopts.mTextureBase.c_str(),
				baseName.c_str() );

			file::Path imgpthout( imgpth.c_str() );
			imgpthout = imgpthout.ToAbsolute( file::Path::EPATHTYPE_POSIX );

			FixedString<1024> tmp = imgpthout.c_str(); 
			tmp.replace_in_place("data/temp/uvatlasdbg", "" );
			tmp.replace_in_place(".//", "./" );

			imgpth = std::string("file://") + std::string(tmp.c_str());

			FCDImageLibrary* ImgLib = daedoc.GetImageLibrary();
			FCDImage* pimg = ImgLib->AddEntity();
			pimg->SetFilename( fm::string(imgpth.c_str()) );

			FCDTexture* ptex = EfxStd->AddTexture(FUDaeTextureChannel::DIFFUSE);
			ptex->SetImage(pimg);
			FCDEffectParameterSampler* psampler = ptex->GetSampler();
			FCDEffectParameterInt* pset = ptex->GetSet();
			FUSStringBuilder builder("TEX");
			builder.append(int32(0));
			pset->SetSemantic(builder.ToCharPtr());

			orkprintf( "SetIMAGE %s sampler<%p>\n", imgpth.c_str(), psampler );
			break;
		}
	}
	return DaeMat;
}

///////////////////////////////////////////////////////////////////////////////

FCDMaterial* PreserveMaterial(const toolmesh& tmesh, FCDocument& daedoc, const std::string& mtlname )
{
	FCDEffectLibrary* EfxLib = daedoc.GetEffectLibrary();
	FCDMaterial* DaeMat = 0;

	const orkmap<std::string,ork::tool::SColladaMaterial*>& materials = tmesh.RefMaterialsByName();
	orkmap<std::string,ork::tool::SColladaMaterial*>::const_iterator it = materials.find(mtlname);

	if( it != materials.end() )
	{
		ork::tool::SColladaMaterial* pmaterial = it->second;

		//orkprintf( "omat1\n" );
		lev2::GfxMaterialFx* pfx = rtti::autocast( pmaterial->mpOrkMaterial );
		FCDEffectProfileFX* pcfx = pmaterial->mFxProfile;

		if( 0 == pfx )
		{
			orkprintf( "ERROR: material<%s> not an FX Shader!\n", mtlname.c_str() );
		}
		if( pfx )
		{
			std::string omatname = mtlname;
			orkmap<std::string, std::string>::const_iterator itml = tmesh.RefShadingGroupToMaterialMap().find(mtlname);					
			if( itml != tmesh.RefShadingGroupToMaterialMap().end() )
			{
				omatname = itml->second;
			}

			FCDEffect* DaeEfx = EfxLib->AddEntity();
			FCDEffectProfileFX* pfx2 = static_cast<FCDEffectProfileFX*>(DaeEfx->AddProfile( FUDaeProfileType::CG  ));
			DaeEfx->SetDaeId( CreateFormattedString( "%s-fx", omatname.c_str() ).c_str() );
			FCDMaterialLibrary *MatLib = daedoc.GetMaterialLibrary();
			DaeMat = MatLib->AddEntity();
			DaeMat->SetDaeId( CreateFormattedString( "%s", omatname.c_str() ).c_str() );
			DaeMat->SetEffect( DaeEfx );
		
			size_t numtek = pcfx->GetTechniqueCount();
			for( size_t t=0; t<numtek; t++ )
			{
				FCDEffectTechnique* ptemptek = pcfx->GetTechnique(t);
				FCDEffectTechnique* pnewtek = pcfx->AddTechnique();

				size_t npasses = ptemptek->GetPassCount();
				for( size_t tpas=0; tpas<npasses; tpas++ )
				{
					FCDEffectPass* ppass = ptemptek->GetPass(tpas);
					FCDEffectPass* pnewpass = pnewtek->AddPass();


				}
			}

			const lev2::GfxMaterialFxEffectInstance& fxi = pfx->GetEffectInstance();
			const orklut<std::string,lev2::GfxMaterialFxParamBase*>& params = fxi.mParameterInstances;
			int inumparams = pcfx->GetEffectParameterCount();

			for( int ip=0; ip<inumparams; ip++ )
			{
				const FCDEffectParameter* Param = pcfx->GetEffectParameter(ip);
				FCDEffectParameter::Type ParamType = Param->GetType();
				const fm::string & ParamSemantic = Param->GetSemantic();
				const fm::string & ParamReference = Param->GetReference();
				std::string parameter_name( ParamReference.c_str() );

				///////////////////////////////////////////////////////////////////////
				// Remove Leading material name in the reference name
				std::string stripped_parameter_name = parameter_name;
				std::string::size_type itst = stripped_parameter_name.find( omatname );
				if( itst != std::string::npos )
				{
					int iparlen = parameter_name.length();
					int imatlen = omatname.length();
					stripped_parameter_name = stripped_parameter_name.substr( itst+imatlen+1, (iparlen-(1+imatlen)) );
				}
				///////////////////////////////////////////////////////////////////////
				lev2::GfxMaterialFxParamBase* l2parambase = 0;
				orklut<std::string,lev2::GfxMaterialFxParamBase*>::const_iterator itp = params.find(stripped_parameter_name);
				if( itp!=params.end() )
				{
					l2parambase = itp->second;
				}
				///////////////////////////////////////////////////////////////////////
				// Skip parameters with the word "engine" in them
				if(parameter_name.find("engine") != std::string::npos)
					continue;
				///////////////////////////////////////////////////////////////////////
				int inumanno = Param->GetAnnotationCount();
				for(int ia = 0; ia < inumanno; ++ia)
				{	const FCDEffectParameterAnnotation* Anno = Param->GetAnnotation(ia);
					const fm::string& AnnoName = Anno->name;
					const fm::string& AnnoVal = Anno->value;
				}

				switch( ParamType )
				{
					case FCDEffectParameter::FLOAT:		{	CloneParam<FCDEffectParameterFloat>(pfx2,Param);break;	}
					case FCDEffectParameter::FLOAT2:	{	CloneParam<FCDEffectParameterFloat2>(pfx2,Param);break;	}
					case FCDEffectParameter::FLOAT3:	{	CloneParam<FCDEffectParameterFloat3>(pfx2,Param);break;	}
					case FCDEffectParameter::VECTOR:	{	CloneParam<FCDEffectParameterVector>(pfx2,Param);break;	}
					case FCDEffectParameter::BOOLEAN:	{	/*CloneParam<FCDEffectParameterBool(pfx2,Param);*/break;	}
					case FCDEffectParameter::INTEGER:	{	CloneParam<FCDEffectParameterInt>(pfx2,Param);break;	}
					case FCDEffectParameter::MATRIX:	{	CloneParam<FCDEffectParameterMatrix>(pfx2,Param);break;	}
					case FCDEffectParameter::STRING:	{	CloneParam<FCDEffectParameterString>(pfx2,Param);break;	}
					case FCDEffectParameter::SAMPLER:
					{	lev2::GfxMaterialFxParamArtist<ork::lev2::Texture*>* l2texparam = rtti::autocast(l2parambase); 
						if( l2texparam )
						{	const std::string& init_string = l2texparam->GetInitString();
							const orklut<std::string,std::string>& l2annos = l2texparam->RefAnnotations();
							///////////////////////////////////////////////
							std::string CgFxParamName = ParamReference.c_str();
							std::string::size_type icolon = CgFxParamName.find( ":" );
							if( icolon != std::string::npos )
							{
								int ilen = CgFxParamName.length();
								CgFxParamName = CgFxParamName.substr( icolon+1, (ilen-icolon)-1 );
							}
							///////////////////////////////////////////////
							FCDEffectParameterSampler *ParamSampler = (FCDEffectParameterSampler*) Param;
							FCDEffectParameterSampler::SamplerType SamplerType = ParamSampler->GetSamplerType();
							FCDEffectParameterSurface * Surface = ParamSampler->GetSurface();
							FCDImageLibrary* ImgLib = daedoc.GetImageLibrary();
							FCDImage* pimg = ImgLib->AddEntity();
							pimg->SetFilename( fm::string(init_string.c_str()) );
							FCDEffectParameterSampler* NewSampler = (FCDEffectParameterSampler*) pfx2->AddEffectParameter(FCDEffectParameter::SAMPLER);
							FCDEffectParameterSurface* NewSurface = (FCDEffectParameterSurface*) pfx2->AddEffectParameter(FCDEffectParameter::SURFACE);
							NewSurface->AddImage( pimg );
							NewSampler->SetReference( ParamReference );
							NewSampler->SetSurface( NewSurface );
							orklut<std::string,std::string>::const_iterator itst = l2annos.find("sampler_type");
							if( itst!=l2annos.end() )
							{	if(itst->second=="1d")   NewSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER1D);
								if(itst->second=="2d")   NewSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER2D);
								if(itst->second=="3d")   NewSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER3D);
								if(itst->second=="cube") NewSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLERCUBE);
							}
							itst = l2annos.find("image_ent_name");
							if( itst!=l2annos.end() )
							{	pimg->SetName(itst->second.c_str());
								pimg->SetDaeId(itst->second.c_str());
								FCDEffectParameterSurfaceInitFrom* surfinit = new FCDEffectParameterSurfaceInitFrom;
								//fm::string slicestr = fm::string(itst->second.c_str());
								surfinit->mip.push_back(fm::string("0"));
								surfinit->slice.push_back(fm::string("0"));
								NewSurface->SetInitMethod(surfinit);
							}
							itst = l2annos.find("surface_ref_name");
							if( itst!=l2annos.end() )
							{	NewSurface->SetReference(itst->second.c_str());
							}
							//NewParam->SetValue( porig->GetValue() );
							//if( porig->IsGenerator() ) NewParam->SetGenerator(  );
							//if( porig->IsModifier() ) NewParam->SetModifier(  );
							//if( porig->IsAnimator() ) NewParam->SetAnimator(  );
							//if( porig->IsReferencer() ) NewParam->SetReferencer(  );
							//if( porig->IsConstant() ) NewParam->SetConstant(  );
							FCDImage *pimage = Surface->GetImage();
							const fstring & ImageName = pimage->GetFilename();
							const char * ImageFileName = ImageName.c_str();
							ork::lev2::GfxMaterialFxParamArtist<lev2::Texture*> *paramf = new ork::lev2::GfxMaterialFxParamArtist<lev2::Texture*>;
							//param=paramf;
							//param->GetRecord().meParameterType = EPROPTYPE_SAMPLER;
							paramf->SetInitString( std::string(ImageFileName) );
						}
						break;
					}
				}
			}
			//orkprintf( "omat5\n" );
		}
	}
	return DaeMat;
}

///////////////////////////////////////////////////////////////////////////////

void WriteDaeMeshPreservingMaterials( const toolmesh& tmesh, FCDocument& daedoc, FCDSceneNode* Child )
{
	std::map<std::string,FCDMaterial*>	DaeMaterials;
	const orkmap<std::string,ork::tool::SColladaMaterial*>& origmaterialsbyname = tmesh.RefMaterialsByName();
	///////////////////////////////////////////////////
	for( orkmap<std::string,ork::tool::SColladaMaterial*>::const_iterator
			it=origmaterialsbyname.begin();
			it!=origmaterialsbyname.end();
			it++ )
	{
		const std::string& matname = it->first;
		const tool::SColladaMaterial* colmat = it->second;
		FCDMaterial* DaeMat = PreserveMaterial( tmesh, daedoc, matname );
		DaeMaterials[matname] = DaeMat;
		orkprintf( "DaeMaterial<%s><%p>\n", matname.c_str(), DaeMat );
	}
	///////////////////////////////////////////////////
	const orklut<std::string,submesh*>& SubMeshLut2 = tmesh.RefSubMeshLut();
	for( orklut<std::string,submesh*>::const_iterator 
			it = SubMeshLut2.begin();
			it != SubMeshLut2.end();
			it++ )
	{	const submesh& pgroup = *it->second;
		const std::string& polygroupname = it->first;
		std::string lmapgrp = pgroup.GetAnnotation("LightMapGroup");
		std::string vtxlit = pgroup.GetAnnotation("vtxlit");
		std::string material = pgroup.GetAnnotation("Material");
		std::map<std::string,FCDMaterial*>::const_iterator itf = DaeMaterials.find(material);
		if( itf!=DaeMaterials.end() )
		{	FCDMaterial* DaeMat = itf->second;
			if( DaeMat )
			{	DaeExtraNode* RootNode = new DaeExtraNode;
				if( lmapgrp.length() )
				{	DaeExtraNode* LmapNode = new DaeExtraNode;
					LmapNode->mValue = lmapgrp;
					DaeExtraNode* LmapGroupNode = new DaeExtraNode;
					LmapGroupNode->mChildren["LightMap"] = LmapNode;
					RootNode->mChildren["LightMapper"] = LmapGroupNode;
				}
				if( vtxlit.length() )
				{	DaeExtraNode* LmapNode = new DaeExtraNode;
					LmapNode->mValue = "true";
					DaeExtraNode* LmapGroupNode = new DaeExtraNode;
					LmapGroupNode->mChildren["VtxLit"] = LmapNode;
					RootNode->mChildren["VtxLit"] = LmapGroupNode;
				}
				CreateDaeMesh(daedoc,Child,polygroupname,DaeMat,pgroup,RootNode);
			}
			else
			{
				orkprintf( "No FXMaterial mtl<%s> grp<%s>, cannot export submesh\n", material.c_str(), polygroupname.c_str() );
			}
		}
	}
	///////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void toolmesh::WriteToDaeFile( const file::Path& BasePath, const DaeWriteOpts& writeopts ) const
{
	FCollada::Initialize();
	ork::file::Path DaePath = BasePath;
	DaePath.SetExtension( "dae" );
	FCDocument daedoc;
	daedoc.SetFileUrl( DaePath.ToAbsolute(file::Path::EPATHTYPE_POSIX ).c_str() );
	///////////////////////////////////////////////////
	FCDSceneNode *daescenenode = daedoc.AddVisualScene();
	daescenenode->SetName( "ToolMeshSceneNode" );
	//daedoc.SetLengthUnitWanted( 0.01f );
	daedoc.GetAsset()->SetUnitConversionFactor( 0.01f );
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	FCDSceneNode* Child = daescenenode->AddChildNode();
	Child->SetDaeId( "ChildNodeDaeID" );
	Child->SetName( "ChildNode" );
	FCDTMatrix* pxf = (FCDTMatrix*) Child->AddTransform( FCDTransform::MATRIX );
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	int inumgrps = mPolyGroupLut.size();
	int igidx = 0;
	if( writeopts.meMaterialSetup == DaeWriteOpts::EMS_PRESERVEMATERIALS )
	{
		WriteDaeMeshPreservingMaterials( *this, daedoc, Child );
	}
	else
	{	for( orklut<std::string,submesh*>::const_iterator 
					it = mPolyGroupLut.begin();
					it != mPolyGroupLut.end();
					it++ )
		{

			const submesh& pgroup = *it->second;
			const std::string& grpname = it->first;
			const vertexpool& vpool = pgroup.RefVertexPool();
			FCDMaterial* DaeMat = AddDaeShader( *this, daedoc, grpname, writeopts );
			if( DaeMat )
			{
				orkprintf( "Creating DaeMesh grp<%d> of<%d> name<%s>\n", igidx, inumgrps, grpname.c_str() );
				CreateDaeMesh(daedoc,Child,grpname,DaeMat,pgroup,0);
				igidx++;
			}
		}
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	FCollada::SaveDocument( & daedoc, DaePath.ToAbsolute().c_str() );
	///////////////////////////////////////////////////
	FCollada::Release();

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct uni_lower
{
	unsigned char operator() (unsigned char c) { return (unsigned char)tolower(c); }
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct DaeSrcRec
{
	DaeDataSource		mSource;
	FCDGeometrySource*	mGeoSource;

};

struct DaeReadQueueItem
{
	MeshUtil::submesh*					mpDestSubMesh;
	const FCDGeometryPolygons*			mpMatGroup;
	DaeDataSource						mPosSource;
	DaeDataSource						mNrmSource;
	orkvector<DaeSrcRec>				mVtxColorSources;
	orkvector<DaeSrcRec>				mUvSources;
	orkvector<DaeSrcRec>				mBinSources;
	const AnnoMap*						mpAnnoMap;
	const file::Path*					mBasePath;
	std::string							MeshDaeID;
	std::string							ShadingGroupName;

	DaeReadQueueItem() 
		: mpDestSubMesh(0), mpMatGroup(0)
		, mpAnnoMap(0), mBasePath(0)
	{}

	void ReadPolys(int ithreadidx) const;
};

///////////////////////////////////////////////////////////////////////////////

struct DaeReadQueue
{
	int												miNumFinished;
	int												miNumQueued;
	LockedResource< orkvector<DaeReadQueueItem> >	mJobSet;
	ork::mutex										mSourceMutex;

	DaeReadQueue() : miNumFinished(0), miNumQueued(0), mSourceMutex("srcmutex") {}
};

///////////////////////////////////////////////////////////////////////////////

struct DaeReadThreadData
{
	DaeReadQueue*	mQ;
	int				miThreadIndex;
};
///////////////////////////////////////////////////////////////////////////////

void* DaeReadJobThread(void*pval)
{
	DaeReadThreadData* thread_data = (DaeReadThreadData*) pval;
	DaeReadQueue* q = thread_data->mQ;

	bool bdone = false;
	while( ! bdone )
	{
		orkvector<DaeReadQueueItem>& qq = q->mJobSet.LockForWrite();
		if( qq.size() )
		{
			orkvector<DaeReadQueueItem>::iterator it = (qq.end()-1);
			DaeReadQueueItem qitem = *it;
			qq.erase(it);
			q->mJobSet.UnLock();
			///////////////////////////////
			qitem.ReadPolys(thread_data->miThreadIndex);
			///////////////////////////////
			q->mSourceMutex.Lock();
			{
				q->miNumFinished++;
				bdone = (q->miNumFinished==q->miNumQueued);
				//qitem.mpSourceToolMesh->RemoveSubMesh(qitem.mSourceSubName);
			}
			q->mSourceMutex.UnLock();
			///////////////////////////////
			ork::msleep(1000);
		}
		else
		{
			q->mJobSet.UnLock();
			bdone=true;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
void DaeReadQueueItem::ReadPolys(int ithreadidx) const
{
	float ftimeA = float(CSystem::GetRef().GetLoResTime());
	size_t imatnumfaces = mpMatGroup->GetFaceCount();
	int inumvtxcolors = mVtxColorSources.size();
	int inumtex = mUvSources.size();
	int inumbin = mBinSources.size();

	for( size_t iface=0; iface<imatnumfaces; iface++ )
	{
		size_t iface_numfverts = mpMatGroup->GetFaceVertexCount(iface);
		size_t iface_fvertbase = mpMatGroup->GetFaceVertexOffset(iface);

		int ivertexcache[kmaxsidesperpoly];

		for( size_t iface_v=0; iface_v<iface_numfverts; iface_v++ )
		{
			MeshUtil::vertex muvtx;
			/////////////////////////////////
			// position
			/////////////////////////////////
			uint32 iposidx = mPosSource.GetSourceIndex( iface_fvertbase, iface_v );
			uint32 inrmidx = mNrmSource.GetSourceIndex( iface_fvertbase, iface_v );
			/////////////////////////////////
			float fX = mPosSource.GetData( iposidx+0 );
			float fY = mPosSource.GetData( iposidx+1 );
			float fZ = mPosSource.GetData( iposidx+2 );
			/////////////////////////////////
			float fNX = mNrmSource.GetData( inrmidx+0 );
			float fNY = mNrmSource.GetData( inrmidx+1 );
			float fNZ = mNrmSource.GetData( inrmidx+2 );
			/////////////////////////////////
			if(ColladaExportPolicy::GetContext()->mReadComponentsPolicy.mReadComponents & ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_POSITION)
				muvtx.mPos.SetXYZ( fX, fY, fZ );
			if(ColladaExportPolicy::GetContext()->mReadComponentsPolicy.mReadComponents & ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_NORMAL)
				muvtx.mNrm.SetXYZ( fNX, fNY, fNZ );
			/////////////////////////////////
			for( int icolor=0; icolor<inumvtxcolors; icolor++ )
			{
				bool breadcolor = false;
				ColladaReadComponentsPolicy::ReadComponents rc = 
					ColladaExportPolicy::GetContext()->mReadComponentsPolicy.mReadComponents;
				switch(icolor)
				{	case 0: breadcolor=(rc&ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_COLOR0); break;
					case 1: breadcolor=(rc&ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_COLOR1); break;
				}
				if(breadcolor)	
				{
					const DaeDataSource& daesrc = mVtxColorSources[icolor].mSource;
					uint32 iclridx = daesrc.GetSourceIndex( iface_fvertbase, iface_v );
					float fCR = daesrc.GetData( iclridx+0 );
					float fCG = daesrc.GetData( iclridx+1 );
					float fCB = daesrc.GetData( iclridx+2 );
					float fCA = daesrc.GetData( iclridx+3 );
					muvtx.mCol[icolor] = CVector4( fCR, fCG, fCB, fCA );
				}
			}
			/////////////////////////////////
			for( int iuv=0; iuv<inumtex; iuv++ )
			{
				bool breaduv = false;
				ColladaReadComponentsPolicy::ReadComponents rc = 
					ColladaExportPolicy::GetContext()->mReadComponentsPolicy.mReadComponents;
				switch(iuv)
				{	case 0: breaduv=(rc&ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_UV0); break;
					case 1: breaduv=(rc&ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_UV1); break;
				}
				if(breaduv)	
				{
					const DaeDataSource& daesrc = mUvSources[iuv].mSource;
					uint32 iuvidx = daesrc.GetSourceIndex( iface_fvertbase, iface_v );
					float fU0 = daesrc.GetData( iuvidx+0 );
					float fV0 = daesrc.GetData( iuvidx+1 );
					muvtx.mUV[iuv].mMapTexCoord = CVector2( fU0, fV0 );
				}
			}
			/////////////////////////////////
			for( int ibin=0; ibin<inumbin; ibin++ )
			{
				bool breadbin = false;
				ColladaReadComponentsPolicy::ReadComponents rc = 
					ColladaExportPolicy::GetContext()->mReadComponentsPolicy.mReadComponents;
				switch(ibin)
				{	case 0: breadbin=(rc&ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_BIN0); break;
					case 1: breadbin=(rc&ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_BIN1); break;
				}
				if(breadbin)	
				{
					const DaeDataSource& daesrc = mBinSources[ibin].mSource;
					uint32 iuvidx = daesrc.GetSourceIndex( iface_fvertbase, iface_v );
					float fbx = daesrc.GetData( iuvidx+0 );
					float fby = daesrc.GetData( iuvidx+1 );
					float fbz = daesrc.GetData( iuvidx+2 );
					muvtx.mUV[ibin].mMapBiNormal = CVector3( fbx, fby, fbz ).Normal();
				}
			}
			//const std::string& PolyGroupName =	ShadingGroupName; //	readopts.mbMergeMeshShGrpName
			/////////////////////////////////
			/////////////////////////////////
			int iv = mpDestSubMesh->MergeVertex( muvtx );
			//orkprintf("iface=%d, iposidx/3=%d, iface_v=%d, iv=%d\n", iface, iposidx/3, iface_v, iv);
			ivertexcache[iface_v] = iv;

			if(iface_v==iface_numfverts-1) {
				if (iface_numfverts <= MeshUtil::kmaxsidesperpoly) {
					MeshUtil::poly ply( ivertexcache, iface_numfverts);
					ply.SetAnnoMap(mpAnnoMap);
					mpDestSubMesh->MergePoly( ply );
				} else {
					orkprintf( "warning<%s>: unhandled number of sides for face<%d> numsides<%d> pos<%f,%f,%f>\n", mBasePath->c_str(), iface, iface_numfverts, fX, fY, fZ );
				}	
			}
		}
	}
	float ftimeB = float(CSystem::GetRef().GetLoResTime());
	float ftime = (ftimeB-ftimeA);
	orkprintf( "<<PROFILE>> <<ReadFromDaeFile::DaeReadThread>> Thread<%d> ShGrp<%s> Mesh<%s> NumFaces<%d> Seconds<%f>\n", ithreadidx, ShadingGroupName.c_str(), MeshDaeID.c_str(), imatnumfaces, ftime );
}
///////////////////////////////////////////////////////////////////////////////

void toolmesh::ReadFromDaeFile( const file::Path& BasePath, DaeReadOpts& readopts )
{
	float ftimeA = float(CSystem::GetRef().GetLoResTime());

	FCollada::Initialize();

	ork::file::Path DaePath = BasePath;
	DaePath.SetExtension( "dae" );
	FCDocument daedoc;

	bool bOK = FCollada::LoadDocumentFromFile( & daedoc, DaePath.ToAbsolute().c_str() );

	OrkAssert( bOK );

	///////////////////////////////////////////////////
	FCDVisualSceneNodeLibrary *daesceneliv = daedoc.GetVisualSceneLibrary();

	float funitfactor = daedoc.GetAsset()->GetUnitConversionFactor();

	FCDEffectLibrary* EfxLib = daedoc.GetEffectLibrary();
	FCDMaterialLibrary *MatLib = daedoc.GetMaterialLibrary();
	FCDGeometryLibrary *GeoLib = daedoc.GetGeometryLibrary();

	///////////////////////////////////////////////////
	// figure out which layers
	///////////////////////////////////////////////////

	std::set<const FCDLayer*> LayerSet;

	FCDLayerList& layers = daedoc.GetLayers();

	int inumlayers = int(layers.size());

	for( int il=0; il<inumlayers; il++ )
	{
		const FCDLayer* layer = layers[il];
		std::string layername( layer->name.c_str() );
		std::transform( layername.begin(), layername.end(), layername.begin(), lower() );
	
		bool binsert = readopts.mReadLayers.empty();

		binsert |= (	readopts.mReadLayers.find( layername.c_str() )
					!=	readopts.mReadLayers.end() );

		binsert &= (	readopts.mExcludeLayers.find( layername.c_str() ) 
					==	readopts.mExcludeLayers.end() );

		if( binsert )
		{
			LayerSet.insert( layer );
		}
	}

	///////////////////////////////////////////////////
	///////////////////////////////////////////////////

	bOK = ParseColladaMaterialBindings( daedoc, mShadingGroupToMaterialMap );

	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	orkset<FCDGeometryMesh*>	MeshSet;
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////

	///////////////////////////////////////////////////
	
	bool blse = LayerSet.empty();
	bool brle = readopts.mReadLayers.empty();

	bool buseall	=	readopts.mbEmptyLayers 
					?	(blse||brle)	// lightmap code wants this
					:	(blse&&brle) ;	// sector code wants this

	///////////////////////////////////////////////////

	if( buseall ) // search for all geometry
	{
		int inumgeoent = GeoLib->GetEntityCount();
	 	   
		for( int ie=0; ie<inumgeoent; ie++ )
		{
			FCDGeometry* DaeGeo = GeoLib->GetEntity(ie);
			if( DaeGeo->IsMesh() )
			{
				FCDGeometryMesh* mesh = DaeGeo->GetMesh();
				MeshSet.insert( mesh );
			}
		}
	}
	///////////////////////////////////////////////////
  	else // search for geometry in specified layers
  	///////////////////////////////////////////////////
  	{
		for( std::set<const FCDLayer*>::const_iterator it=LayerSet.begin(); it!=LayerSet.end(); it++ )
		{
			const FCDLayer* layer = (*it);

			int inumobjects = int(layer->objects.size());

			for( int io=0; io<inumobjects; io++ )
			{
				const fm::string& objname = layer->objects[io];

				FCDEntity *pent = daedoc.FindEntity( objname );

				if( pent->GetType() == FCDEntity::SCENE_NODE )
				{
					orkstack<FCDSceneNode*> NodeStack;

					FCDSceneNode *parnode = static_cast<FCDSceneNode*>(pent);

					NodeStack.push(parnode);

					while(NodeStack.empty()==false)
					{
						FCDSceneNode *node = NodeStack.top();
						NodeStack.pop();

						size_t inumchild = node->GetChildrenCount();

						for( size_t ichild=0; ichild<inumchild; ichild++ )
						{
							FCDSceneNode* Child = node->GetChild(ichild);
							NodeStack.push(Child);
						}

						fm::string NodeName = node->GetName();

						size_t inuminst = node->GetInstanceCount();

						for( size_t i=0; i<inuminst; i++ )
						{
							FCDEntityInstance * pinst = node->GetInstance( i );

							FCDEntity* pent = pinst->GetEntity();

							if(pent->HasType(FCDGeometry::GetClassType()))
							{
								FCDGeometry* GeoObj = static_cast<FCDGeometry*>(pent);
								if( GeoObj->IsMesh() )
								{
									FCDGeometryMesh* mesh = GeoObj->GetMesh();
									MeshSet.insert( mesh );
								}

							}
							if(pinst->HasType(FCDGeometryInstance::GetClassType()))
							{
								FCDGeometryInstance *pgeoinst = static_cast<FCDGeometryInstance*>(pinst);
								//FCDGeometry* DaeGeo = pgeoinst->Get

								fm::string instname = pgeoinst->GetEntity()->GetName();

								//IgnoreMeshSet.insert( instname );
							}
						}
					}
				}
			}
		}
	}

	float ftimeB = float(CSystem::GetRef().GetLoResTime());
	float ftime = (ftimeB-ftimeA);
	orkprintf( "<<PROFILE>> <<ReadFromDaeFile::Stage1 %f seconds>>\n", ftime );

	///////////////////////////////////////////////////

	DaeReadQueue Q;
	ftimeA = float(CSystem::GetRef().GetLoResTime());
	for( orkset<FCDGeometryMesh*>::const_iterator it=MeshSet.begin(); it!=MeshSet.end(); it++ )
	{

		FCDGeometryMesh* mesh = (*it);
		const std::string MeshDaeID = mesh->GetDaeId().c_str();

		if( ColladaExportPolicy::GetContext()->mTriangulation.GetPolicy() == ColladaTriangulationPolicy::ECTP_TRIANGULATE )
		{
			FCDGeometryPolygonsTools::Triangulate(mesh);
		}

		//////////////////////////////////////////////////
		// enumerate sources

		std::multimap<FUDaeGeometryInput::Semantic,FCDGeometrySource*> SourceMap;

		size_t inumsources = mesh->GetSourceCount();
		for( size_t isrc=0; isrc<inumsources; isrc++ )
		{
			FCDGeometrySource *Source = mesh->GetSource( isrc );
			FUDaeGeometryInput::Semantic src_type = Source->GetType();
			const fstring& src_name = Source->GetName();
			SourceMap.insert( std::make_pair(src_type,Source) );
		}

		//////////////////////////////////////////////////

		int inumpos = SourceMap.count(FUDaeGeometryInput::POSITION);
		int inumnrm = SourceMap.count(FUDaeGeometryInput::NORMAL);
		int inumclr = SourceMap.count(FUDaeGeometryInput::COLOR);
		int inumtex = SourceMap.count(FUDaeGeometryInput::TEXCOORD);
		int inumbin = SourceMap.count(FUDaeGeometryInput::TEXBINORMAL);
		int inumtan = SourceMap.count(FUDaeGeometryInput::TEXTANGENT);

		OrkAssert( inumpos == 1 );
		OrkAssert( inumnrm == 1 );

		FCDGeometrySource *PositionSource = SourceMap.find( FUDaeGeometryInput::POSITION )->second;
		FCDGeometrySource *NormalSource = SourceMap.find( FUDaeGeometryInput::NORMAL )->second;
		//FCDGeometrySource *LightMapColorSource = mesh->FindSourceByName( "LightMapColorSet" );

		//////////////////////////////////////////////

		orkvector<DaeSrcRec> UvSources;

		for(	std::multimap<FUDaeGeometryInput::Semantic,FCDGeometrySource*>::const_iterator 
				itm=SourceMap.find(FUDaeGeometryInput::TEXCOORD);
				itm!=SourceMap.upper_bound(FUDaeGeometryInput::TEXCOORD);
				itm++ )
		{
			FCDGeometrySource* uvsrc = itm->second;
			DaeSrcRec dsr;
			dsr.mGeoSource = uvsrc;
			UvSources.push_back(dsr);
		}
		
		//////////////////////////////////////////////

		orkvector<DaeSrcRec> BinSources;

		for(	std::multimap<FUDaeGeometryInput::Semantic,FCDGeometrySource*>::const_iterator 
				itm=SourceMap.begin(); //(FUDaeGeometryInput::TEXBINORMAL);
				itm!=SourceMap.end(); //upper_bound(FUDaeGeometryInput::TEXBINORMAL);
				itm++ )
		{
			FUDaeGeometryInput::Semantic sem = itm->second->GetType();
			if( FUDaeGeometryInput::TEXBINORMAL == sem )
			{
				FCDGeometrySource* binsrc = itm->second;
				DaeSrcRec dsr;
				dsr.mGeoSource = binsrc;
				BinSources.push_back(dsr);
			}
		}

		//////////////////////////////////////////////

		orkvector<DaeSrcRec> VtxColorSources;

		for(	std::multimap<FUDaeGeometryInput::Semantic,FCDGeometrySource*>::const_iterator 
				itm=SourceMap.find(FUDaeGeometryInput::COLOR);
				itm!=SourceMap.upper_bound(FUDaeGeometryInput::COLOR);
				itm++ )
		{
			FCDGeometrySource* clrsrc = itm->second;
			DaeSrcRec dsr;
			dsr.mGeoSource = clrsrc;
			VtxColorSources.push_back(dsr);
		}

		int inumvtxcolors = VtxColorSources.size();

		//////////////////////////////////////////////

		size_t inummaterialgroups = mesh->GetPolygonsCount();
		for( size_t imatgroup=0; imatgroup<inummaterialgroups; imatgroup++ )
		{
			FCDGeometryPolygons * MatGroup = mesh->GetPolygons(imatgroup);

			DaeDataSource PosSource( PositionSource, MatGroup );
			DaeDataSource NrmSource( NormalSource, MatGroup );

			for( int i=0; i<inumtex; i++ )
			{
				UvSources[i].mSource.Bind( UvSources[i].mGeoSource, MatGroup );
			}
			inumbin = BinSources.size();
			for( int i=0; i<inumbin; i++ )
			{
				BinSources[i].mSource.Bind( BinSources[i].mGeoSource, MatGroup );
			}
			for( int i=0; i<inumvtxcolors; i++ )
			{
				VtxColorSources[i].mSource.Bind( VtxColorSources[i].mGeoSource, MatGroup );
			}

			FCDMaterialLibrary *MatLib = daedoc.GetMaterialLibrary();
			std::string ShadingGroupName = MatGroup->GetMaterialSemantic ().c_str();
			std::string MeshShGrp = MeshDaeID+std::string("_")+ShadingGroupName;
			std::string MaterialName = mShadingGroupToMaterialMap[ ShadingGroupName ];
			
			AnnoMap* anno_map = new AnnoMap;
			anno_map->SetAnnotation("shadinggroup",ShadingGroupName);
			anno_map->SetAnnotation("material",MaterialName);
			anno_map->SetAnnotation("daemesh",MeshDaeID);

			ork::tool::SColladaMaterial* colladamaterial = new ork::tool::SColladaMaterial;
			colladamaterial->ParseMaterial( &daedoc, ShadingGroupName, MaterialName );
			//mMaterialMap[ ShadingGroupName ] = colladamaterial;
			//ColMatGroup->Parse( colladamaterial );
			mMaterialsByShadingGroup[ShadingGroupName] = colladamaterial;
			mMaterialsByName[MaterialName] = colladamaterial;
			
			
			FCDGeometryPolygons::PrimitiveType eprimtype = MatGroup->GetPrimitiveType();
			orkvector<unsigned int> TriangleIndices;
			size_t imatnumfaces = MatGroup->GetFaceCount();

			MeshUtil::submesh& out_submesh = MergeSubMesh( ShadingGroupName.c_str() );

			if( readopts.mbMergeMeshShGrpName )
			{
				out_submesh.mAnnotations["shadinggroup"] = ShadingGroupName;
			}

			if(FCDMaterial *material = MatLib->FindDaeId(MaterialName.c_str()))
			{	if(const FCDExtra *extra = material->GetExtra())
				{	if(const FCDETechnique* Technique = extra->GetDefaultType()->FindTechnique("MAYA"))
					{	if(const FCDENode* Node = extra->GetDefaultType()->FindRootNode("dynamic_attributes"))
						{	for(size_t i = 0; i < Node->GetChildNodeCount(); ++i)
							{	const FCDENode* attributeNode = Node->GetChildNode(i);
								std::string attributeName = attributeNode->GetName();
								const FCDEAttribute* shortNameAttribute = attributeNode->FindAttribute("short_name");
								std::string shortName = (shortNameAttribute != NULL) ? shortNameAttribute->GetValue().c_str() : attributeName;
								if(attributeNode->GetChildNodeCount() == 0)
								{	std::string value = attributeNode->GetContent();
									out_submesh.mAnnotations[shortName] = value;
								}
							}
						}
					}
				}
			}

			///////////////////////////////////////////////////////
			DaeReadQueueItem qITEM;

			qITEM.mpDestSubMesh = & out_submesh;
			qITEM.mpMatGroup = MatGroup;
			qITEM.mPosSource = PosSource;
			qITEM.mNrmSource = NrmSource;
			qITEM.mVtxColorSources = VtxColorSources;
			qITEM.mUvSources = UvSources;
			qITEM.mBinSources = BinSources;
			qITEM.mpAnnoMap = anno_map;
			qITEM.mBasePath = & BasePath;
			qITEM.MeshDaeID = MeshDaeID;
			qITEM.ShadingGroupName = ShadingGroupName;

			orkvector<DaeReadQueueItem>& QV = Q.mJobSet.LockForWrite();
			QV.push_back(qITEM);
			Q.miNumQueued = QV.size();
			Q.mJobSet.UnLock();

			///////////////////////////////////////////////////////
		}
	}
	/////////////////////////////////////////////////////////
	// start threads
	/////////////////////////////////////////////////////////
	int inumcores = 1; //readopts.miNumThreads;
	orkvector<boost::thread*>	ThreadVect;
	for( int ic=0; ic<inumcores; ic++ )
	{
		DaeReadThreadData* thread_data = new DaeReadThreadData;
		thread_data->mQ = & Q;
		thread_data->miThreadIndex = ic;
		boost::thread* job_thread = new boost::thread(DaeReadJobThread,thread_data);
		ThreadVect.push_back(job_thread);
	}
	/////////////////////////////////////////////////////////
	// wait for threads
	/////////////////////////////////////////////////////////
	for( auto it=ThreadVect.begin(); it!=ThreadVect.end(); it++ )
	{
		boost::thread* job = (*it);
        printf( "joining thread<%p>\n", job );
		job->join();
        delete job;
	}
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	ftimeB = float(CSystem::GetRef().GetLoResTime());
	ftime = (ftimeB-ftimeA);
	orkprintf( "<<PROFILE>> <<ReadFromDaeFile::Stage2 %f seconds>>\n", ftime );
	FCollada::Release();
}

///////////////////////////////////////////////////////////////////////////////

bool DAEToDAE(const tokenlist& options)
{
	bool rval = true;

	ork::tool::FilterOptMap	OptionsMap;
	OptionsMap.SetDefault("-in", "coldae_in.dae");
	OptionsMap.SetDefault("-out", "coldae_out.sec");
	OptionsMap.SetOptions(options);

	std::string ttv_in = OptionsMap.GetOption("-in")->GetValue();
	std::string ttv_out = OptionsMap.GetOption("-out")->GetValue();

	file::Path inPath(ttv_in.c_str());
	file::Path outPath(ttv_out.c_str());

	ork::file::Path::SmallNameType ext = inPath.GetExtension();

	if (ext != "dae")
		return false;

	ColladaExportPolicy policy;
	policy.mNormalPolicy.meNormalPolicy = ColladaNormalPolicy::ENP_ALLOW_SPLIT_NORMALS;
	//policy.mReadComponentsPolicy.mReadComponents = ColladaReadComponentsPolicy::EPOLICY_READCOMPONENTS_POSITION;
	policy.mTriangulation.SetPolicy( ColladaTriangulationPolicy::ECTP_DONT_TRIANGULATE );

	//Read sectors from the DAE file and grab sector info as well as last resort collision data
	DaeReadOpts daeReadOptions;
	//sectorReadOptions.mReadLayers.insert("sectors");
	MeshUtil::toolmesh daeMesh;
	daeMesh.ReadFromDaeFile(inPath, daeReadOptions);

	DaeWriteOpts out_opts;
	out_opts.meMaterialSetup = DaeWriteOpts::EMS_PRESERVEMATERIALS;
	daeMesh.WriteToDaeFile( outPath, out_opts );

	return rval;
}

///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////
#endif
