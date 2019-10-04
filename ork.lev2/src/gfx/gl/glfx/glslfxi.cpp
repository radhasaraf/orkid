////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "../gl.h"
#include "glslfxi.h"
#include <ork/file/file.h>
#include <ork/kernel/prop.h>
#include <ork/kernel/string/string.h>

namespace ork::lev2 {

///////////////////////////////////////////////////////////////////////////////
// FX Interface
///////////////////////////////////////////////////////////////////////////////

void GfxTargetGL::FxInit() {
  static bool binit = true;

  if (true == binit) {
    binit = false;
    FxShader::RegisterLoaders("shaders/glfx/", "glfx");
  }
}

}

namespace ork::lev2::glslfx {

Container* GenPlat2SolidFx(const AssetPath& pth);
Container* GenPlat2UiFx(const AssetPath& pth);

///////////////////////////////////////////////////////////////////////////////

void Interface::DoBeginFrame() { mLastPass = 0; }

///////////////////////////////////////////////////////////////////////////////

bool Pass::hasUniformInstance(UniformInstance* puni) const {
  Uniform* pun                                               = puni->mpUniform;
  std::map<std::string, UniformInstance*>::const_iterator it = _uniformInstances.find(pun->mName);
  return it != _uniformInstances.end();
}

///////////////////////////////////////////////////////////////////////////////

const UniformInstance* Pass::uniformInstance(Uniform* puni) const {
  std::map<std::string, UniformInstance*>::const_iterator it = _uniformInstances.find(puni->mName);
  return (it != _uniformInstances.end()) ? it->second : nullptr;
}

///////////////////////////////////////////////////////////////////////////////

Interface::Interface(GfxTargetGL& glctx)
    : mTarget(glctx) {}

///////////////////////////////////////////////////////////////////////////////

bool Interface::LoadFxShader(const AssetPath& pth, FxShader* pfxshader) {
  // printf( "GLSLFXI LoadShader<%s>\n", pth.c_str() );
  GL_ERRORCHECK();
  bool bok = false;
  pfxshader->SetInternalHandle(0);

  Container* pcontainer = LoadFxFromFile(pth);
  OrkAssert(pcontainer != nullptr);
  pfxshader->SetInternalHandle((void*)pcontainer);
  bok = pcontainer->IsValid();

  pcontainer->mFxShader = pfxshader;

  if (bok) {
    BindContainerToAbstract(pcontainer, pfxshader);
  }
  GL_ERRORCHECK();

  return bok;
}

///////////////////////////////////////////////////////////////////////////////

void Interface::BindContainerToAbstract(Container* pcont, FxShader* fxh) {
  for (const auto& ittek : pcont->_techniqueMap) {
    Technique* ptek            = ittek.second;
    FxShaderTechnique* ork_tek = new FxShaderTechnique((void*)ptek);
    ork_tek->mTechniqueName    = ittek.first;
    // pabstek->mPasses = ittek->first;
    ork_tek->mbValidated = true;
    fxh->addTechnique(ork_tek);
  }
  for (const auto& itp : pcont->_uniforms) {
    Uniform* puni                = itp.second;
    FxShaderParam* ork_parm      = new FxShaderParam;
    ork_parm->_name     = itp.first;
    ork_parm->mParameterSemantic = puni->mSemantic;
    ork_parm->mInternalHandle    = (void*)puni;
    fxh->addParameter(ork_parm);
  }
}

///////////////////////////////////////////////////////////////////////////////

void Container::addConfig(Config* pcfg) { mConfigs[pcfg->mName] = pcfg; }
void Container::addUniformBlock(UniformBlock* pif) { _uniformBlocks[pif->_name] = pif; }
void Container::addUniformSet(UniformSet* pif) { _uniformSets[pif->_name] = pif; }
void Container::addVertexInterface(StreamInterface* pif) { _vertexInterfaces[pif->mName] = pif; }
void Container::addTessCtrlInterface(StreamInterface* pif) { _tessCtrlInterfaces[pif->mName] = pif; }
void Container::addTessEvalInterface(StreamInterface* pif) { _tessEvalInterfaces[pif->mName] = pif; }
void Container::addGeometryInterface(StreamInterface* pif) { _geometryInterfaces[pif->mName] = pif; }
void Container::addFragmentInterface(StreamInterface* pif) { _fragmentInterfaces[pif->mName] = pif; }
void Container::addStateBlock(StateBlock* psb) { _stateBlocks[psb->mName] = psb; }
void Container::addLibBlock(LibBlock* plb) { _libBlocks[plb->mName] = plb; }
void Container::addTechnique(Technique* ptek) { _techniqueMap[ptek->mName] = ptek; }
void Container::addVertexShader(ShaderVtx* psha) { _vertexShaders[psha->mName] = psha; }
void Container::addTessCtrlShader(ShaderTsC* psha) { _tessCtrlShaders[psha->mName] = psha; }
void Container::addTessEvalShader(ShaderTsE* psha) { _tessEvalShaders[psha->mName] = psha; }
void Container::addGeometryShader(ShaderGeo* psha) { _geometryShaders[psha->mName] = psha; }
void Container::addFragmentShader(ShaderFrg* psha) { _fragmentShaders[psha->mName] = psha; }

///////////////////////////////////////////////////////////////////////////////

StateBlock* Container::GetStateBlock(const std::string& name) const {
  const auto& it = _stateBlocks.find(name);
  return (it == _stateBlocks.end()) ? nullptr : it->second;
}
ShaderVtx* Container::vertexShader(const std::string& name) const {
  const auto& it = _vertexShaders.find(name);
  return (it == _vertexShaders.end()) ? nullptr : it->second;
}
ShaderTsC* Container::tessCtrlShader(const std::string& name) const {
  const auto& it = _tessCtrlShaders.find(name);
  return (it == _tessCtrlShaders.end()) ? nullptr : it->second;
}
ShaderTsE* Container::tessEvalShader(const std::string& name) const {
  const auto& it = _tessEvalShaders.find(name);
  return (it == _tessEvalShaders.end()) ? nullptr : it->second;
}
ShaderGeo* Container::geometryShader(const std::string& name) const {
  const auto& it = _geometryShaders.find(name);
  return (it == _geometryShaders.end()) ? nullptr : it->second;
}
ShaderFrg* Container::fragmentShader(const std::string& name) const {
  const auto& it = _fragmentShaders.find(name);
  return (it == _fragmentShaders.end()) ? nullptr : it->second;
}

UniformBlock* Container::uniformBlock(const std::string& name) const {
  const auto& it = _uniformBlocks.find(name);
  return (it == _uniformBlocks.end()) ? nullptr : it->second;
}
UniformSet* Container::uniformSet(const std::string& name) const {
  const auto& it = _uniformSets.find(name);
  return (it == _uniformSets.end()) ? nullptr : it->second;
}
StreamInterface* Container::vertexInterface(const std::string& name) const {
  const auto& it = _vertexInterfaces.find(name);
  return (it == _vertexInterfaces.end()) ? nullptr : it->second;
}
StreamInterface* Container::tessCtrlInterface(const std::string& name) const {
  const auto& it = _tessCtrlInterfaces.find(name);
  return (it == _tessCtrlInterfaces.end()) ? nullptr : it->second;
}
StreamInterface* Container::tessEvalInterface(const std::string& name) const {
  const auto& it = _tessEvalInterfaces.find(name);
  return (it == _tessEvalInterfaces.end()) ? nullptr : it->second;
}
StreamInterface* Container::geometryInterface(const std::string& name) const {
  const auto& it = _geometryInterfaces.find(name);
  return (it == _geometryInterfaces.end()) ? nullptr : it->second;
}
StreamInterface* Container::fragmentInterface(const std::string& name) const {
  const auto& it = _fragmentInterfaces.find(name);
  return (it == _fragmentInterfaces.end()) ? nullptr : it->second;
}
Uniform* Container::GetUniform(const std::string& name) const {
  const auto& it = _uniforms.find(name);
  return (it == _uniforms.end()) ? nullptr : it->second;
}

#if defined(ENABLE_NVMESH_SHADERS)
void Container::addNvTaskShader(ShaderNvTask* psha) { _nvTaskShaders[psha->mName] = psha; }
void Container::addNvMeshShader(ShaderNvMesh* psha) { _nvMeshShaders[psha->mName] = psha; }
ShaderNvTask* Container::nvTaskShader(const std::string& name) const {
  const auto& it = _nvTaskShaders.find(name);
  return (it == _nvTaskShaders.end()) ? nullptr : it->second;
}
ShaderNvMesh* Container::nvMeshShader(const std::string& name) const {
  const auto& it = _nvMeshShaders.find(name);
  return (it == _nvMeshShaders.end()) ? nullptr : it->second;
}
StreamInterface* Container::nvTaskInterface(const std::string& name) const {
  const auto& it = _nvTaskInterfaces.find(name);
  return (it == _nvTaskInterfaces.end()) ? nullptr : it->second;
}
StreamInterface* Container::nvMeshInterface(const std::string& name) const {
  const auto& it = _nvMeshInterfaces.find(name);
  return (it == _nvMeshInterfaces.end()) ? nullptr : it->second;
}
#endif

///////////////////////////////////////////////////////////////////////////////

Uniform* Container::MergeUniform(const std::string& name) {
  Uniform* pret  = nullptr;
  const auto& it = _uniforms.find(name);
  if (it == _uniforms.end()) {
    pret            = new Uniform(name);
    _uniforms[name] = pret;
  } else {
    pret = it->second;
  }
  // printf( "MergedUniform<%s><%p>\n", name.c_str(), pret );
  return pret;
}

///////////////////////////////////////////////////////////////////////////////

Container::Container(const std::string& nam)
    : mEffectName(nam)
    , mActiveTechnique(nullptr)
    , mActivePass(nullptr)
    , mActiveNumPasses(0)
    , mShaderCompileFailed(false) {
  StateBlock* pdefsb = new StateBlock;
  pdefsb->mName      = "default";
  this->addStateBlock(pdefsb);
}

///////////////////////////////////////////////////////////////////////////////

void Container::Destroy(void) {}

///////////////////////////////////////////////////////////////////////////////

bool Container::IsValid(void) { return true; }

///////////////////////////////////////////////////////////////////////////////

int Interface::BeginBlock(FxShader* hfx, const RenderContextInstData& data) {
  mTarget.SetRenderContextInstData(&data);
  Container* container = static_cast<Container*>(hfx->GetInternalHandle());
  mpActiveEffect       = container;
  mpActiveFxShader     = hfx;
  if (nullptr == container->mActiveTechnique)
    return 0;
  return container->mActiveTechnique->mPasses.size();
}

///////////////////////////////////////////////////////////////////////////////

void Interface::EndBlock(FxShader* hfx) { mpActiveFxShader = 0; }

///////////////////////////////////////////////////////////////////////////////

void Interface::CommitParams(void) {
  if (mpActiveEffect && mpActiveEffect->mActivePass && mpActiveEffect->mActivePass->_stateBlock) {
    const auto& items = mpActiveEffect->mActivePass->_stateBlock->mApplicators;

    for (const auto& item : items) {
      item(&mTarget);
    }
    // const SRasterState& rstate =
    // mpActiveEffect->mActivePass->_stateBlock->mState;
    // mTarget.RSI()->BindRasterState(rstate);
  }
  // if( (mpActiveEffect->mActivePass != mLastPass) ||
  // (mTarget.GetCurMaterial()!=mpLastFxMaterial) )
  {
    // orkprintf( "CgFxInterface::CommitParams() activepass<%p>\n",
    // mpActiveEffect->mActivePass ); cgSetPassState( mpActiveEffect->mActivePass
    // ); mpLastFxMaterial = mTarget.GetCurMaterial(); mLastPass =
    // mpActiveEffect->mActivePass;
  }
}

///////////////////////////////////////////////////////////////////////////////

const FxShaderTechnique* Interface::GetTechnique(FxShader* hfx, const std::string& name) {
  // orkprintf( "Get cgtek<%s> hfx<%x>\n", name.c_str(), hfx );
  OrkAssert(hfx != 0);
  Container* container = static_cast<Container*>(hfx->GetInternalHandle());
  OrkAssert(container != 0);
  /////////////////////////////////////////////////////////////

  const auto& tekmap            = hfx->GetTechniques();
  const auto& it                = tekmap.find(name);
  const FxShaderTechnique* htek = (it != tekmap.end()) ? it->second : 0;

  return htek;
}

///////////////////////////////////////////////////////////////////////////////

bool Interface::BindTechnique(FxShader* hfx, const FxShaderTechnique* htek) {
  if (nullptr == hfx)
    return false;
  if (nullptr == htek)
    return false;

  Container* container        = static_cast<Container*>(hfx->GetInternalHandle());
  const Technique* ptekcont   = static_cast<const Technique*>(htek->GetPlatformHandle());
  container->mActiveTechnique = ptekcont;
  container->mActivePass      = 0;

  // orkprintf( "binding cgtek<%s:%x>\n", ptekcont->mName.c_str(), ptekcont );

  return (ptekcont->mPasses.size() > 0);
}

///////////////////////////////////////////////////////////////////////////////

bool Shader::Compile() {
  GL_NF_ERRORCHECK();
  mShaderObjectId = glCreateShader(mShaderType);

  std::string shadertext = "";

  shadertext += mShaderText;

  shadertext += "void main() { ";
  shadertext += mName;
  shadertext += "(); }\n";
  const char* c_str = shadertext.c_str();

  // printf( "Shader<%s>\n/////////////\n%s\n///////////////////\n",
  // mName.c_str(), c_str );

  GL_NF_ERRORCHECK();
  glShaderSource(mShaderObjectId, 1, &c_str, NULL);
  GL_NF_ERRORCHECK();
  glCompileShader(mShaderObjectId);
  GL_NF_ERRORCHECK();

  GLint compiledOk = 0;
  glGetShaderiv(mShaderObjectId, GL_COMPILE_STATUS, &compiledOk);
  if (GL_FALSE == compiledOk) {
    char infoLog[1 << 16];
    glGetShaderInfoLog(mShaderObjectId, sizeof(infoLog), NULL, infoLog);
    printf("//////////////////////////////////\n");
    printf("%s\n", c_str);
    printf("//////////////////////////////////\n");
    printf("Effect<%s>\n", mpContainer->mEffectName.c_str());
    printf("ShaderType<0x%x>\n", mShaderType);
    printf("Shader<%s> InfoLog<%s>\n", mName.c_str(), infoLog);
    printf("//////////////////////////////////\n");

    if (mpContainer->mFxShader->GetAllowCompileFailure() == false)
      OrkAssert(false);

    mbError = true;
    return false;
  }
  mbCompiled = true;
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Shader::IsCompiled() const { return mbCompiled; }

///////////////////////////////////////////////////////////////////////////////

bool Interface::BindPass(FxShader* hfx, int ipass) {
  Container* container = static_cast<Container*>(hfx->GetInternalHandle());
  if (container->mShaderCompileFailed)
    return false;

  assert(container->mActiveTechnique != nullptr);

  container->mActivePass = container->mActiveTechnique->mPasses[ipass];
  Pass* ppass            = const_cast<Pass*>(container->mActivePass);

  GL_ERRORCHECK();
  if (0 == container->mActivePass->_programObjectId) {

    auto& pipeVTG = container->mActivePass->_primpipe.Get<PrimPipelineVTG>();

    Shader* pvtxshader = pipeVTG._vertexShader;
    Shader* ptecshader = pipeVTG._tessCtrlShader;
    Shader* pteeshader = pipeVTG._tessEvalShader;
    Shader* pgeoshader = pipeVTG._geometryShader;
    Shader* pfrgshader = pipeVTG._fragmentShader;

    OrkAssert(pvtxshader != nullptr);
    OrkAssert(pfrgshader != nullptr);

    auto l_compile = [&](Shader* psh) {
      bool compile_ok = true;
      if (psh && psh->IsCompiled() == false)
        compile_ok = psh->Compile();

      if (false == compile_ok) {
        container->mShaderCompileFailed = true;
        hfx->SetFailedCompile(true);
      }
    };

    l_compile(pvtxshader);
    l_compile(ptecshader);
    l_compile(pteeshader);
    l_compile(pgeoshader);
    l_compile(pfrgshader);

    if (container->mShaderCompileFailed)
      return false;

    if (pvtxshader->IsCompiled() && pfrgshader->IsCompiled()) {
      GL_ERRORCHECK();
      GLuint prgo             = glCreateProgram();
      ppass->_programObjectId = prgo;

      //////////////
      // attach shaders
      //////////////

      glAttachShader(prgo, pvtxshader->mShaderObjectId);
      GL_ERRORCHECK();

      if (ptecshader && ptecshader->IsCompiled()) {
        glAttachShader(prgo, ptecshader->mShaderObjectId);
        GL_ERRORCHECK();
      }

      if (pteeshader && pteeshader->IsCompiled()) {
        glAttachShader(prgo, pteeshader->mShaderObjectId);
        GL_ERRORCHECK();
      }

      if (pgeoshader && pgeoshader->IsCompiled()) {
        glAttachShader(prgo, pgeoshader->mShaderObjectId);
        GL_ERRORCHECK();
      }

      glAttachShader(prgo, pfrgshader->mShaderObjectId);
      GL_ERRORCHECK();

      //////////////
      // link
      //////////////

      StreamInterface* vtx_iface = pvtxshader->mpInterface;
      StreamInterface* frg_iface = pfrgshader->mpInterface;

      /*printf( "//////////////////////////////////\n");

      printf( "Linking pass<%p> {\n", ppass );

      if( pvtxshader )
              printf( "	vtxshader<%s:%p> compiled<%d>\n",
      pvtxshader->mName.c_str(), pvtxshader, pvtxshader->IsCompiled() ); if(
      ptecshader ) printf( "	tecshader<%s:%p> compiled<%d>\n",
      ptecshader->mName.c_str(), ptecshader, ptecshader->IsCompiled() ); if(
      pteeshader ) printf( "	teeshader<%s:%p> compiled<%d>\n",
      pteeshader->mName.c_str(), pteeshader, pteeshader->IsCompiled() ); if(
      pgeoshader ) printf( "	geoshader<%s:%p> compiled<%d>\n",
      pgeoshader->mName.c_str(), pgeoshader, pgeoshader->IsCompiled() ); if(
      pfrgshader ) printf( "	frgshader<%s:%p> compiled<%d>\n",
      pfrgshader->mName.c_str(), pfrgshader, pfrgshader->IsCompiled() );
*/

      if (nullptr == vtx_iface) {
        printf("	vtxshader<%s> has no interface!\n", pvtxshader->mName.c_str());
        OrkAssert(false);
      }
      if (nullptr == frg_iface) {
        printf("	frgshader<%s> has no interface!\n", pfrgshader->mName.c_str());
        OrkAssert(false);
      }

      // printf( "	binding vertex attributes count<%d>\n",
      // int(vtx_iface->mAttributes.size()) );

      //////////////////////////
      // Bind Vertex Attributes
      //////////////////////////

      for (const auto& itp : vtx_iface->mAttributes) {
        Attribute* pattr = itp.second;
        int iloc         = pattr->mLocation;
        // printf( "	vtxattr<%s> loc<%d> dir<%s> sem<%s>\n",
        // pattr->mName.c_str(), iloc, pattr->mDirection.c_str(),
        // pattr->mSemantic.c_str() );
        glBindAttribLocation(prgo, iloc, pattr->mName.c_str());
        GL_ERRORCHECK();
        ppass->_vtxAttributeById[iloc]                    = pattr;
        ppass->_vtxAttributesBySemantic[pattr->mSemantic] = pattr;
      }

      //////////////////////////
      // ensure vtx_iface exports what frg_iface imports
      //////////////////////////

      /*if( nullptr == pgeoshader )
      {
              for( const auto& itp : frg_iface->mAttributes )
              {	const Attribute* pfrgattr = itp.second;
                      if( pfrgattr->mDirection=="in" )
                      {
                              int iloc = pfrgattr->mLocation;
                              const std::string& name = pfrgattr->mName;
                              //printf( "frgattr<%s> loc<%d> dir<%s>\n",
      pfrgattr->mName.c_str(), iloc, pfrgattr->mDirection.c_str() ); const auto&
      itf=vtx_iface->mAttributes.find(name); const Attribute* pvtxattr =
      (itf!=vtx_iface->mAttributes.end()) ? itf->second : nullptr; OrkAssert(
      pfrgattr != nullptr ); OrkAssert( pvtxattr != nullptr ); OrkAssert(
      pvtxattr->mTypeName == pfrgattr->mTypeName );
                      }
              }
      }*/

      //////////////////////////

      GL_ERRORCHECK();
      glLinkProgram(prgo);
      GL_ERRORCHECK();
      GLint linkstat = 0;
      glGetProgramiv(prgo, GL_LINK_STATUS, &linkstat);
      if (linkstat != GL_TRUE) {
        char infoLog[1 << 16];
        glGetProgramInfoLog(prgo, sizeof(infoLog), NULL, infoLog);
        printf("\n\n//////////////////////////////////\n");
        printf("program InfoLog<%s>\n", infoLog);
        printf("//////////////////////////////////\n\n");
        OrkAssert(false);
      }
      OrkAssert(linkstat == GL_TRUE);

      // printf( "} // linking complete..\n" );

      // printf( "//////////////////////////////////\n");

      //////////////////////////
      // query attr
      //////////////////////////

      GLint numattr = 0;
      glGetProgramiv(prgo, GL_ACTIVE_ATTRIBUTES, &numattr);
      GL_ERRORCHECK();

      for (int i = 0; i < numattr; i++) {
        GLchar nambuf[256];
        GLsizei namlen = 0;
        GLint atrsiz   = 0;
        GLenum atrtyp  = GL_ZERO;
        GL_ERRORCHECK();
        glGetActiveAttrib(prgo, i, sizeof(nambuf), &namlen, &atrsiz, &atrtyp, nambuf);
        OrkAssert(namlen < sizeof(nambuf));
        GL_ERRORCHECK();
        const auto& it = vtx_iface->mAttributes.find(nambuf);
        OrkAssert(it != vtx_iface->mAttributes.end());
        Attribute* pattr = it->second;
        // printf( "qattr<%d> loc<%d> name<%s>\n", i, pattr->mLocation, nambuf
        // );
        pattr->meType = atrtyp;
        // pattr->mLocation = i;
      }

      //////////////////////////
      // query unis
      //////////////////////////

      GLint numunis = 0;
      GL_ERRORCHECK();
      glGetProgramiv(prgo, GL_ACTIVE_UNIFORMS, &numunis);
      GL_ERRORCHECK();

      ppass->_samplerCount = 0;

      for (int i = 0; i < numunis; i++) {
        GLsizei namlen = 0;
        GLint unisiz   = 0;
        GLenum unityp  = GL_ZERO;
        std::string str_name;

        {
          GLchar nambuf[256];
          glGetActiveUniform(prgo, i, sizeof(nambuf), &namlen, &unisiz, &unityp, nambuf);
          OrkAssert(namlen < sizeof(nambuf));
          // printf( "find uni<%s>\n", nambuf );
          GL_ERRORCHECK();

          str_name = nambuf;
          auto its = str_name.find('[');
          if (its != str_name.npos) {
            str_name = str_name.substr(0, its);
            // printf( "nnam<%s>\n", str_name.c_str() );
          }
        }
        const auto& it = container->_uniforms.find(str_name);
        OrkAssert(it != container->_uniforms.end());
        Uniform* puni = it->second;

        puni->meType = unityp;

        UniformInstance* pinst = new UniformInstance;
        pinst->mpUniform       = puni;

        GLint uniloc     = glGetUniformLocation(prgo, str_name.c_str());
        pinst->mLocation = uniloc;

        if (puni->mTypeName == "sampler2D") {
          pinst->mSubItemIndex = ppass->_samplerCount;
          ppass->_samplerCount++;
          pinst->mPrivData.Set<GLenum>(GL_TEXTURE_2D);
        } else if (puni->mTypeName == "sampler3D") {
          pinst->mSubItemIndex = ppass->_samplerCount;
          ppass->_samplerCount++;
          pinst->mPrivData.Set<GLenum>(GL_TEXTURE_3D);
        } else if (puni->mTypeName == "sampler2DShadow") {
          pinst->mSubItemIndex = ppass->_samplerCount;
          ppass->_samplerCount++;
          pinst->mPrivData.Set<GLenum>(GL_TEXTURE_2D);
        }

        const char* fshnam = pfrgshader->mName.c_str();

        // printf("fshnam<%s> uninam<%s> loc<%d>\n", fshnam, str_name.c_str(),
        // (int) uniloc );

        const_cast<Pass*>(container->mActivePass)->_uniformInstances[puni->mName] = pinst;
      }
    }
  }

  GL_ERRORCHECK();
  glUseProgram(container->mActivePass->_programObjectId);
  GL_ERRORCHECK();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void Interface::EndPass(FxShader* hfx) {
  Container* container = static_cast<Container*>(hfx->GetInternalHandle());
  GL_ERRORCHECK();
  glUseProgram(0);
  GL_ERRORCHECK();
  // cgResetPassState( container->mActivePass );
}

///////////////////////////////////////////////////////////////////////////////

const FxShaderParam* Interface::GetParameterH(FxShader* hfx, const std::string& name) {
  OrkAssert(0 != hfx);
  const auto& parammap        = hfx->paramByName();
  const auto& it              = parammap.find(name);
  const FxShaderParam* hparam = (it != parammap.end()) ? it->second : 0;
  return hparam;
}

///////////////////////////////////////////////////////////////////////////////

const FxShaderParamBlock* Interface::GetParameterBlockH(FxShader* hfx, const std::string& name) {
  OrkAssert(0 != hfx);
  const auto& parammap        = hfx->paramBlockByName();
  const auto& it              = parammap.find(name);
  const FxShaderParamBlock* hparam = (it != parammap.end()) ? it->second : 0;
  return hparam;
}

///////////////////////////////////////////////////////////////////////////////

void Interface::BindParamBool(FxShader* hfx, const FxShaderParam* hpar, const bool bv) {}

///////////////////////////////////////////////////////////////////////////////

void Interface::BindParamInt(FxShader* hfx, const FxShaderParam* hpar, const int iv) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_INT);

      GL_ERRORCHECK();
      glUniform1i(iloc, iv);
      GL_ERRORCHECK();
    } else {
      assert(false);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Interface::BindParamVect2(FxShader* hfx, const FxShaderParam* hpar, const fvec2& Vec) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT_VEC2);

      GL_ERRORCHECK();
      glUniform2fv(iloc, 1, Vec.GetArray());
      GL_ERRORCHECK();
    }
  }
}

void Interface::BindParamVect3(FxShader* hfx, const FxShaderParam* hpar, const fvec3& Vec) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT_VEC3);

      GL_ERRORCHECK();
      glUniform3fv(iloc, 1, Vec.GetArray());
      GL_ERRORCHECK();
    }
  }
}

void Interface::BindParamVect4(FxShader* hfx, const FxShaderParam* hpar, const fvec4& Vec) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT_VEC4);

      GL_ERRORCHECK();
      glUniform4fv(iloc, 1, Vec.GetArray());
      GL_ERRORCHECK();
    }
  }
}

void Interface::BindParamVect4Array(FxShader* hfx, const FxShaderParam* hpar, const fvec4* Vec, const int icount) {
  /*Container* container = static_cast<Container*>(
  hfx->GetInternalHandle() ); Uniform* puni = static_cast<Uniform*>(
  hpar->GetPlatformHandle() ); int iloc = puni->mLocation; if( iloc>=0 )
  {
          const char* psem = puni->mSemantic.c_str();
          const char* pnam = puni->mName.c_str();
          GLenum etyp = puni->meType;
          OrkAssert( etyp == 	GL_FLOAT_VEC4 );

          glUniform4fv( iloc, icount, (float*) Vec );
          GL_ERRORCHECK();
  }*/
}

void Interface::BindParamFloat(FxShader* hfx, const FxShaderParam* hpar, float fA) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT);

      GL_ERRORCHECK();
      glUniform1f(iloc, fA);
      GL_ERRORCHECK();
    }
  }
}
void Interface::BindParamFloatArray(FxShader* hfx, const FxShaderParam* hpar, const float* pfa, const int icount) {
  /*Container* container = static_cast<Container*>(
  hfx->GetInternalHandle() ); Uniform* puni = static_cast<Uniform*>(
  hpar->GetPlatformHandle() ); int iloc = puni->mLocation; if( iloc>=0 )
  {
          const char* psem = puni->mSemantic.c_str();
          const char* pnam = puni->mName.c_str();
          GLenum etyp = puni->meType;
          OrkAssert( etyp == GL_FLOAT );

          glUniform1fv( iloc, icount, pfa );
          GL_ERRORCHECK();
  }*/
}

void Interface::BindParamFloat2(FxShader* hfx, const FxShaderParam* hpar, float fA, float fB) {
  /*Container* container = static_cast<Container*>(
  hfx->GetInternalHandle() ); Uniform* puni = static_cast<Uniform*>(
  hpar->GetPlatformHandle() ); int iloc = puni->mLocation; if( iloc>=0 )
  {
          const char* psem = puni->mSemantic.c_str();
          const char* pnam = puni->mName.c_str();
          GLenum etyp = puni->meType;
          OrkAssert( etyp == 	GL_FLOAT_VEC2 );

          fvec2 v2( fA, fB );

          glUniform2fv( iloc, 1, v2.GetArray() );
          GL_ERRORCHECK();
  }*/
}

void Interface::BindParamFloat3(FxShader* hfx, const FxShaderParam* hpar, float fA, float fB, float fC) {
  /*Container* container = static_cast<Container*>(
  hfx->GetInternalHandle() ); Uniform* puni = static_cast<Uniform*>(
  hpar->GetPlatformHandle() ); int iloc = puni->mLocation; if( iloc>=0 )
  {
          const char* psem = puni->mSemantic.c_str();
          const char* pnam = puni->mName.c_str();
          GLenum etyp = puni->meType;
          OrkAssert( etyp == 	GL_FLOAT_VEC3 );

          fvec3 v3( fA, fB, fC );

          glUniform3fv( iloc, 1, v3.GetArray() );
          GL_ERRORCHECK();
  }*/
}

void Interface::BindParamFloat4(FxShader* hfx, const FxShaderParam* hpar, float fA, float fB, float fC, float fD) {
  /*Container* container = static_cast<Container*>(
  hfx->GetInternalHandle() ); Uniform* puni = static_cast<Uniform*>(
  hpar->GetPlatformHandle() ); int iloc = puni->mLocation; if( iloc>=0 )
  {
          const char* psem = puni->mSemantic.c_str();
          const char* pnam = puni->mName.c_str();
          GLenum etyp = puni->meType;
          OrkAssert( etyp == 	GL_FLOAT_VEC4 );

          fvec4 v4( fA, fB, fC, fD );

          glUniform4fv( iloc, 1, v4.GetArray() );
          GL_ERRORCHECK();
  }*/
}

void Interface::BindParamU32(FxShader* hfx, const FxShaderParam* hpar, U32 uval) {
  /*
          CgFxContainer* container = static_cast<CgFxContainer*>(
     hfx->GetInternalHandle() ); CGeffect cgeffect = container->mCgEffect;
          CGparameter cgparam =
     reinterpret_cast<CGparameter>(hpar->GetPlatformHandle()); GL_ERRORCHECK();
  */
}

void Interface::BindParamMatrix(FxShader* hfx, const FxShaderParam* hpar, const fmtx4& Mat) {
  Container* container = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni        = static_cast<Uniform*>(hpar->GetPlatformHandle());
  assert(container->mActivePass != nullptr);
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT_MAT4);

      GL_ERRORCHECK();
      glUniformMatrix4fv(iloc, 1, GL_FALSE, Mat.GetArray());
      GL_ERRORCHECK();
    }
  }
}

void Interface::BindParamMatrix(FxShader* hfx, const FxShaderParam* hpar, const fmtx3& Mat) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT_MAT3);

      GL_ERRORCHECK();
      glUniformMatrix3fv(iloc, 1, GL_FALSE, Mat.GetArray());
      GL_ERRORCHECK();
    }
  }
}

void Interface::BindParamMatrixArray(FxShader* hfx, const FxShaderParam* hpar, const fmtx4* Mat, int iCount) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  if (pinst) {
    int iloc = pinst->mLocation;
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      OrkAssert(etyp == GL_FLOAT_MAT4);

      // printf( "pnam<%s>\n", pnam );
      GL_ERRORCHECK();
      glUniformMatrix4fv(iloc, iCount, GL_FALSE, (const float*)Mat);
      GL_ERRORCHECK();
    }
  }

  /*Container* container = static_cast<Container*>(
  hfx->GetInternalHandle() ); Uniform* puni = static_cast<Uniform*>(
  hpar->GetPlatformHandle() ); int iloc = puni->mLocation; if( iloc>=0 )
  {
          const char* psem = puni->mSemantic.c_str();
          const char* pnam = puni->mName.c_str();
          GLenum etyp = puni->meType;
          OrkAssert( etyp == GL_FLOAT_MAT4 );

          glUniformMatrix4fv( iloc, iCount, GL_FALSE, (float*) Mat );
          GL_ERRORCHECK();
  }*/
}

///////////////////////////////////////////////////////////////////////////////

void Interface::BindParamCTex(FxShader* hfx, const FxShaderParam* hpar, const Texture* pTex) {
  Container* container         = static_cast<Container*>(hfx->GetInternalHandle());
  Uniform* puni                = static_cast<Uniform*>(hpar->GetPlatformHandle());
  const UniformInstance* pinst = container->mActivePass->uniformInstance(puni);
  // printf( "Bind1 Tex<%p> par<%s> pinst<%p>\n",
  // pTex,hpar->mParameterName.c_str(), pinst );
  if (pinst) {
    int iloc = pinst->mLocation;

    const char* teknam = container->mActiveTechnique->mName.c_str();

    // printf( "Bind2 Tex<%p> par<%s> iloc<%d> teknam<%s>\n",
    // pTex,hpar->mParameterName.c_str(), iloc, teknam );
    if (iloc >= 0) {
      const char* psem = puni->mSemantic.c_str();
      const char* pnam = puni->mName.c_str();
      GLenum etyp      = puni->meType;
      // OrkAssert( etyp == GL_FLOAT_MAT4 );

      if (pTex != 0) {
        const GLTextureObject* pTEXOBJ = (GLTextureObject*)pTex->GetTexIH();
        GLuint texID                   = pTEXOBJ ? pTEXOBJ->mObject : 0;
        int itexunit                   = pinst->mSubItemIndex;

        GLenum textgt = pinst->mPrivData.Get<GLenum>();

        // printf( "Bind3 ISDEPTH<%d> tex<%p> texobj<%d> itexunit<%d>
        // textgt<%d>\n", int(pTex->_isDepthTexture), pTex, texID, itexunit,
        // int(textgt) );

        GL_ERRORCHECK();
        glActiveTexture(GL_TEXTURE0 + itexunit);
        GL_ERRORCHECK();
        glBindTexture(textgt, texID);
        GL_ERRORCHECK();
        // glEnable( GL_TEXTURE_2D );
        // GL_ERRORCHECK();
        glUniform1i(iloc, itexunit);
        GL_ERRORCHECK();
      }
    }
  }
  /*
          if( 0 == hpar ) return;
          CgFxContainer* container = static_cast<CgFxContainer*>(
     hfx->GetInternalHandle() ); CGeffect cgeffect = container->mCgEffect;
          CGparameter cgparam =
     reinterpret_cast<CGparameter>(hpar->GetPlatformHandle()); if( (pTex!=0) &&
     (cgparam!=0) )
          {
                  const GLTextureObject* pTEXOBJ = (GLTextureObject*)
     pTex->GetTexIH();
                  //orkprintf( "BINDTEX param<%p:%s> pTEX<%p> pTEXOBJ<%p>
     obj<%d>\n", hpar, hpar->mParameterName.c_str(), pTex, pTEXOBJ,
     pTEXOBJ->mObject ); cgGLSetTextureParameter( cgparam, pTEXOBJ ?
     pTEXOBJ->mObject : 0 );
          }
          else
          {
                  cgGLSetTextureParameter( cgparam, 0 );
          }
          GL_ERRORCHECK();
  */
}

UniformBlockItem UniformBlockBinding::findUniform(std::string named) const {
  UniformBlockItem rval;
  assert(_pass!=nullptr);



  return rval;
}

UniformBlockBinding Pass::findUniformBlock(std::string blockname){
  UniformBlockBinding rval;
  rval._blockIndex = glGetUniformBlockIndex( _programObjectId, blockname.c_str() );
  rval._pass = this;
  rval._name = blockname;

  GLint blocksize = 0;
  glGetActiveUniformBlockiv(_programObjectId,rval._blockIndex,GL_UNIFORM_BLOCK_DATA_SIZE,&blocksize);

  GLint numunis = 0;
  glGetActiveUniformBlockiv(_programObjectId,rval._blockIndex,GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,&numunis);

  auto uniindices = new GLuint[numunis];
  glGetActiveUniformBlockiv(_programObjectId,rval._blockIndex,GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,(GLint*)uniindices);

  auto uniblkidcs = new GLint[numunis];
  glGetActiveUniformBlockiv(_programObjectId,rval._blockIndex,GL_UNIFORM_BLOCK_INDEX,uniblkidcs);

  auto unioffsets = new GLint[numunis];
  glGetActiveUniformsiv( _programObjectId, numunis, uniindices, GL_UNIFORM_OFFSET, unioffsets );

  auto unitypes = new GLint[numunis];
  glGetActiveUniformsiv( _programObjectId, numunis, uniindices, GL_UNIFORM_TYPE, unioffsets );

  auto unisizes = new GLint[numunis];
  glGetActiveUniformsiv( _programObjectId, numunis, uniindices, GL_UNIFORM_SIZE, unioffsets );

  auto uniarystrides = new GLint[numunis];
  glGetActiveUniformsiv( _programObjectId, numunis, uniindices, GL_UNIFORM_ARRAY_STRIDE, unioffsets );

  auto unimtxstrides = new GLint[numunis];
  glGetActiveUniformsiv( _programObjectId, numunis, uniindices, GL_UNIFORM_MATRIX_STRIDE, unioffsets );

  delete[] unimtxstrides;
  delete[] uniarystrides;
  delete[] unisizes;
  delete[] unitypes;
  delete[] unioffsets;
  delete[] uniblkidcs;
  delete[] uniindices;

  return rval;
}

} // namespace ork::lev2::glslfx
