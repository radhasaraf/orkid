////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/file/file.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/shadman.h>

namespace ork { namespace lev2 {

extern bool gearlyhack;

FxShader::FxShader() {
}

///////////////////////////////////////////////////////////////////////////////

FxShaderPass::FxShaderPass(){
}

///////////////////////////////////////////////////////////////////////////////

FxShaderParam::FxShaderParam()
    : mBindable(true)
    , mChildParam(0) {
}

///////////////////////////////////////////////////////////////////////////////

fxparam_constptr_t FxUniformBlock::param(const std::string& name) const {
  auto it = _parametersByName.find(name);
  return (it != _parametersByName.end()) ? it->second : nullptr;
}

FxUniformBufferMapping::FxUniformBufferMapping() {
}
FxUniformBufferMapping::~FxUniformBufferMapping() {
  assert(_mappedaddr == nullptr);
}

///////////////////////////////////////////////////////////////////////////////

const char* FxShader::assetTypeNameStatic() {
  return "fxshader";
}

const techniquebynamemap_t& FxShader::techniques() const {
  return _techniques;
}

const parambynamemap_t& FxShader::namedParams() const {
  return _parameterByName;
}

const fxcompute_byname_map_t& FxShader::namedComputeShaders() const {
  return _computeShaderByName;
}

void FxShader::addTechnique(const FxShaderTechnique* tek) {
  _techniques[tek->_techniqueName] = tek;
}

void FxShader::addParameter(const FxShaderParam* param) {
  _parameterByName[param->_name] = param;
}
void FxShader::addComputeShader(const FxComputeShader* csh) {
  _computeShaderByName[csh->_name] = csh;
}
FxComputeShader* FxShader::findComputeShader(const std::string& named) {
  auto it = _computeShaderByName.find(named);
  return const_cast<FxComputeShader*>((it != _computeShaderByName.end()) ? it->second : nullptr);
}

void FxShader::addStorageBlock(const FxShaderStorageBlock* block) {
  _storageBlockByName[block->_name] = block;
}
FxShaderStorageBlock* FxShader::storageBlockByName(const std::string& named) {
  auto it = _storageBlockByName.find(named);
  return const_cast<FxShaderStorageBlock*>((it != _storageBlockByName.end()) ? it->second : nullptr);
}
FxShaderStorageBufferMapping::FxShaderStorageBufferMapping() {
}
FxShaderStorageBufferMapping::~FxShaderStorageBufferMapping() {
  assert(_mappedaddr == nullptr);
}
void FxShaderStorageBufferMapping::unmap() {
  _ci->unmapStorageBuffer(this);
}

///////////////////////////////////////////////////////////////////////////////

void FxShader::RegisterLoaders(const file::Path& base, const std::string& ext) {
  static auto gShaderFileContext1 = std::make_shared<FileDevContext>();
  auto lev2ctx                    = FileEnv::contextForUriProto("lev2://");
  auto shaderpath                 = lev2ctx->getFilesystemBaseAbs() / base;
  auto shaderfilectx              = FileEnv::createContextForUriBase("orkshader://", shaderpath);
  shaderfilectx->SetFilesystemBaseEnable(true);
  // OrkAssert(false);
  if (0)
    printf(
        "FxShader::RegisterLoaders ext<%s> l2<%s> base<%s> shaderpath<%s>\n", //
        ext.c_str(),
        lev2ctx->getFilesystemBaseAbs().c_str(),
        base.c_str(),
        shaderpath.c_str());
  gearlyhack = false;
}

///////////////////////////////////////////////////////////////////////////////

void FxShader::OnReset() {
  auto target = lev2::contextForCurrentThread();

  for (const auto& it : _parameterByName ) {
    const FxShaderParam* param = it.second;
    const std::string& type    = param->mParameterType;
    if (param->mParameterType == "sampler" || param->mParameterType == "texture") {
      target->FXI()->BindParamCTex(param, 0);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

FxShaderParam* FxShader::FindParamByName(const std::string& named) {
  orkmap<std::string, const FxShaderParam*>::iterator it = _parameterByName.find(named);
  return const_cast<FxShaderParam*>((it != _parameterByName.end()) ? it->second : 0);
}

///////////////////////////////////////////////////////////////////////////////

FxShaderTechnique* FxShader::FindTechniqueByName(const std::string& named) {
  orkmap<std::string, const FxShaderTechnique*>::iterator it = _techniques.find(named);
  return const_cast<FxShaderTechnique*>((it != _techniques.end()) ? it->second : 0);
}

void FxShader::SetName(const char* name) {
  mName = name;
}

const char* FxShader::GetName() {
  return mName.c_str();
}

///////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_COMPUTE_SHADER)
FxComputeShader* FxShader::findComputeShader(const std::string& named) {
  FxComputeShader* rval = nullptr;
  assert(false);
  return rval;
}
#endif

///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2
