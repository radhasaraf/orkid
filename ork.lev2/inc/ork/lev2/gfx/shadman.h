////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/kernel/core/singleton.h>
#include <ork/kernel/prop.h>
#include <ork/lev2/gfx/gfxenv.h>

namespace ork::lev2 {

  using fxuniformset_byname_map_t      = std::unordered_map<std::string, fxuniformset_constptr_t>;
  using fxsamplerset_byname_map_t      = std::unordered_map<std::string, fxsamplerset_constptr_t>;
  using fxuniformblock_byname_map_t      = std::unordered_map<std::string, fxuniformblock_constptr_t>;
  using parambynamemap_t     = std::map<std::string, fxparam_constptr_t>;
  using techniquebynamemap_t  = std::map<std::string, fxtechnique_constptr_t>;
  using fxcompute_byname_map_t    = std::map<std::string, const FxComputeShader*>;
  using fxstorageblock_byname_map_t = std::unordered_map<std::string, const FxShaderStorageBlock*>;

///////////////////////////////////////////////////////////////////////////////

struct FxShaderPass {

  FxShaderPass();

  std::string _name;
  svarp_t _impl;

};

///////////////////////////////////////////////////////////////////////////////

struct FxShaderTechnique {

  bool _validated = false;
  fxshader_ptr_t _shader = nullptr;
  std::string _techniqueName;
  orkvector<FxShaderPass*> _passes;
  svarp_t _impl;

};

///////////////////////////////////////////////////////////////////////////////

struct FxShaderParamInBlockInfo {
  FxUniformBlock* _parent = nullptr;
};

struct FxShaderParam {

  FxShaderParam();

  std::string _name;
  std::string _semantic;
  std::string mParameterType;
  bool mBindable;
  FxShaderParamInBlockInfo* _blockinfo = nullptr;
  FxShaderParam* mChildParam;
  orklut<std::string, std::string> _annotations;
  svarp_t _impl;
};
struct FxUniformSet {
  std::map<std::string, fxparam_constptr_t> _parametersByName;
};
struct FxSamplerSet {
  std::map<std::string, fxparam_constptr_t> _parametersByName;
};

struct FxUniformBlock {
  std::string _name;
  fxparam_constptr_t param(const std::string& name) const;
  std::map<std::string, fxparam_constptr_t> _parametersByName;
  svarp_t _impl;
  FxInterface* _fxi = nullptr;
};
struct FxUniformBuffer {
  size_t _length = 0;
  svarp_t _impl;
};
struct FxUniformBufferMapping {
  FxUniformBufferMapping();
  ~FxUniformBufferMapping();
  FxUniformBuffer* _buffer = nullptr;
  FxInterface* _fxi            = nullptr;
  size_t _offset               = 0;
  size_t _cursor               = 0;
  size_t _length               = 0;
  svarp_t _impl;
  ///////////////////////////////////////////////////
  template <typename T> T& ref(size_t offset) {
    size_t end = offset + sizeof(T);
    OrkAssert(end <= _length);
    auto tstar = (T*)(((char*)_mappedaddr) + offset);
    return *tstar;
  }
  ///////////////////////////////////////////////////
  template <typename T, typename ... A> T& make(A&&... args) {
    size_t end = _cursor + sizeof(T);
    OrkAssert(end <= _length);
    auto tstar = (T*)(((char*)_mappedaddr) + _cursor);
    new (tstar) T(std::forward<A>(args)...);
    _cursor += sizeof(T);
    return *tstar;
  }
  ///////////////////////////////////////////////////
  void seek(size_t offset) {
    _cursor = offset;
  }
  ///////////////////////////////////////////////////
  void* _mappedaddr = nullptr;
};

///////////////////////////////////////////////////////////////////////////////

struct FxShaderStorageBlock {
  std::string _name;
  svarp_t _impl;
};
struct FxShaderStorageBuffer {
  size_t _length = 0;
  svarp_t _impl;
};
struct FxShaderStorageBufferMapping {
  FxShaderStorageBufferMapping();
  ~FxShaderStorageBufferMapping();
  void unmap();
  FxShaderStorageBuffer* _buffer = nullptr;
  ComputeInterface* _ci          = nullptr;
  size_t _offset                 = 0;
  size_t _cursor                 = 0;
  size_t _length                 = 0;
  svarp_t _impl;

  template <typename T> T& ref(size_t offset) {
    size_t end = offset + sizeof(T);
    assert(end <= _length);
    auto tstar = (T*)(((char*)_mappedaddr) + offset);
    return *tstar;
  }

  static int alignTo(int num, int alignment) {
    return ((num + alignment - 1) / alignment) * alignment;
  }

  ///////////////////////////////////////////////////
  void align(int quanta) {
    _cursor = alignTo(_cursor,quanta);
  }
  ///////////////////////////////////////////////////
  template <typename T, typename ... A> T& make(A&&... args) {

    switch(sizeof(T)){ // std430 layout rules
      case 4: { // float
        _cursor = alignTo(_cursor,4); break;
      }
      case 8: { // double, int, vec2
        _cursor = alignTo(_cursor,8); break;
      }
      case 12: { // vec3
        _cursor = alignTo(_cursor,16); break;
      }
      case 16: { // vec4
        _cursor = alignTo(_cursor,16); break;
      }
      case 32: {
        _cursor = alignTo(_cursor,16); break;
      }
      case 64: { // mat4
        _cursor = alignTo(_cursor,16); break;
      }
      default: OrkAssert(false); break;
    }

    size_t end = _cursor + sizeof(T);
    OrkAssert(end <= _length);
    auto tstar = (T*)(((char*)_mappedaddr) + _cursor);
    new (tstar) T(std::forward<A>(args)...);
    _cursor += sizeof(T);
    return *tstar;
  }
  ///////////////////////////////////////////////////
  void seek(size_t offset) {
    _cursor = offset;
  }

  void* _mappedaddr = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
// TODO : implement descriptor sets at public API level
//  so we can hoist static descriptor binding code out of rendering loop 
///////////////////////////////////////////////////////////////////////////////

struct FxShaderDescriptorSetItem {
  svarp_t _impl;
};
struct FxShaderDescriptorSet {
  std::unordered_map<std::string,fxdescriptorsetitem_ptr_t> _items_by_name;
  std::unordered_map<fxparam_constptr_t,fxdescriptorsetitem_ptr_t> _items_by_param;
  std::unordered_map<int,fxdescriptorsetitem_ptr_t> _items_by_binding;
  svarp_t _impl;
};
struct FxShaderDescriptorSetBindPoint {
  svarp_t _impl;
};

///////////////////////////////////////////////////////////////////////////////

struct FxComputeShader {
  svar64_t _impl;
  std::string _name;
};

///////////////////////////////////////////////////////////////////////////////

struct FxShader {

  void OnReset();

  static void SetLoaderTarget(Context* targ);

  FxShader();

  static void RegisterLoaders(const file::Path& base, const std::string& ext);

  static const char* assetTypeNameStatic();

  void addTechnique(fxtechnique_constptr_t tek);
  void addParameter(fxparam_constptr_t param);
  void addComputeShader(const FxComputeShader* csh);

  const techniquebynamemap_t& techniques() const;
  const parambynamemap_t& namedParams() const;
  const fxcompute_byname_map_t& namedComputeShaders() const;

  FxShaderParam* FindParamByName(const std::string& named);
  FxUniformBlock* FindParamBlockByName(const std::string& named);
  FxShaderTechnique* FindTechniqueByName(const std::string& named);
  FxComputeShader* findComputeShader(const std::string& named);

  void SetAllowCompileFailure(bool bv) {
    mAllowCompileFailure = bv;
  }
  bool GetAllowCompileFailure() const {
    return mAllowCompileFailure;
  }
  void SetFailedCompile(bool bv) {
    mFailedCompile = bv;
  }
  bool GetFailedCompile() const {
    return mFailedCompile;
  }

  void SetName(const char*);
  const char* GetName();

  ////////////////////////////////////////////////////
  // SSBO support
  ////////////////////////////////////////////////////

  const fxstorageblock_byname_map_t& namedStorageBlocks() const {
    return _storageBlockByName;
  }
  void addStorageBlock(const FxShaderStorageBlock* block);
  FxShaderStorageBlock* storageBlockByName(const std::string& named);

  ////////////////////////////////////////////////////

  svar16_t _internalHandle;
  techniquebynamemap_t _techniques;
  parambynamemap_t _parameterByName;

  fxuniformset_byname_map_t _uniformSets;
  fxsamplerset_byname_map_t _samplerSets;
  fxuniformblock_byname_map_t _uniformBlocks;
  fxstorageblock_byname_map_t _storageBlockByName;
  fxcompute_byname_map_t _computeShaderByName;
  ork::varmap::VarMap _varmap;
  
  bool mAllowCompileFailure = false;
  bool mFailedCompile       = false;
  std::string mName;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::lev2
