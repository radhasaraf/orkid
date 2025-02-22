////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "../gl.h"
#include "glslfxi.h"
#include <ork/file/file.h>
#include <ork/kernel/prop.h>
#include <ork/kernel/string/string.h>
#include <ork/util/logger.h>

namespace ork::lev2::glslfx {

static logchannel_ptr_t logchan_pass = logger()->createChannel("GLSLFXPASS", fvec3(0.8, 0.8, 0.3));

///////////////////////////////////////////////////////////////////////////////

bool Pass::hasUniformInstance(UniformInstance* puni) const {
  auto pun = puni->mpUniform;
  auto it      = _uniformInstances.find(pun->_name);
  return it != _uniformInstances.end();
}

///////////////////////////////////////////////////////////////////////////////

const UniformInstance* Pass::uniformInstance(uniform_ptr_t puni) const {
  auto it = _uniformInstances.find(puni->_name);
  return (it != _uniformInstances.end()) ? it->second : nullptr;
}

///////////////////////////////////////////////////////////////////////////////

UniformBlockBinding* Pass::uniformBlockBinding(UniformBlock* block) {

  auto it = _uboBindingMap.find(block);
  if (it != _uboBindingMap.end()) {
    return it->second;
  }

  auto rval = new UniformBlockBinding;

  //logchan_pass->log("PASS<%s> uniformBlockBinding", _name.c_str() );

  rval->_blockIndex = glGetUniformBlockIndex(_programObjectId, block->_name.c_str());
  rval->_pass       = this;
  rval->_block      = block;
  rval->_bindingPoint = 0;

  //logchan_pass->log("block<%s> _blockIndex<%d>", block->_name.c_str(), rval->_blockIndex );

  if (rval->_blockIndex == GL_INVALID_INDEX) {
    logchan_pass->log("block<%s> blockindex<0x%08x>", block->_name.c_str(), rval->_blockIndex);
    logchan_pass->log("perhaps the UBO is not referenced...");
    return rval;
  }


  glUniformBlockBinding(_programObjectId, rval->_blockIndex, rval->_bindingPoint);

  glGetActiveUniformBlockiv(_programObjectId, rval->_blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &rval->_blockSize);
  //logchan_pass->log("block<%s> blocksize<%d>", block->_name.c_str(), rval->_blockSize);

  GLint numunis = 0;
  glGetActiveUniformBlockiv(_programObjectId, rval->_blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numunis);
  //logchan_pass->log("block<%s> numunis<%d>", block->_name.c_str(), numunis);

  auto uniindices = new GLuint[numunis];
  glGetActiveUniformBlockiv(_programObjectId, rval->_blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)uniindices);

  // auto uniblkidcs = new GLint[numunis];
  // glGetActiveUniformBlockiv(_programObjectId,rval._blockIndex,GL_UNIFORM_BLOCK_INDEX,uniblkidcs);

  auto unioffsets = new GLint[numunis];
  glGetActiveUniformsiv(_programObjectId, numunis, uniindices, GL_UNIFORM_OFFSET, unioffsets);

  auto unitypes = new GLint[numunis];
  glGetActiveUniformsiv(_programObjectId, numunis, uniindices, GL_UNIFORM_TYPE, unitypes);

  auto unisizes = new GLint[numunis];
  glGetActiveUniformsiv(_programObjectId, numunis, uniindices, GL_UNIFORM_SIZE, unisizes);

  auto uniarystrides = new GLint[numunis];
  glGetActiveUniformsiv(_programObjectId, numunis, uniindices, GL_UNIFORM_ARRAY_STRIDE, uniarystrides);

  auto unimtxstrides = new GLint[numunis];
  glGetActiveUniformsiv(_programObjectId, numunis, uniindices, GL_UNIFORM_MATRIX_STRIDE, unimtxstrides);

  //////////////////////////////////////////////

  for (int i = 0; i < numunis; i++) {
    UniformBlockBinding::Item item;
    item._actuniindex = uniindices[i];
    // item._blockindex = uniblkidcs[i];
    item._offset       = unioffsets[i];
    item._type         = unitypes[i];
    item._size         = unisizes[i];
    item._arraystride  = uniarystrides[i];
    item._matrixstride = unimtxstrides[i];
    rval->_ubbitems.push_back(item);
    //logchan_pass->log("block<%s> uni<%d> actidx<%d>", block->_name.c_str(), i, uniindices[i]);
    //logchan_pass->log("block<%s> uni<%d> blkidx<%d>", block->_name.c_str(), i, uniblkidcs[i] );
    //logchan_pass->log("block<%s> uni<%d> offset<%d>", block->_name.c_str(), i, unioffsets[i]);
    //logchan_pass->log("block<%s> uni<%d> type<%d>", block->_name.c_str(), i, unitypes[i]);
    //logchan_pass->log("block<%s> uni<%d> size<%d>", block->_name.c_str(), i, unisizes[i]);
    //logchan_pass->log("block<%s> uni<%d> arystride<%d>", block->_name.c_str(), i, uniarystrides[i]);
    //logchan_pass->log("block<%s> uni<%d> mtxstride<%d>", block->_name.c_str(), i, unimtxstrides[i]);
  }

  //////////////////////////////////////////////

  if(block->_name=="ub_vtx_boneblock"){
    //OrkAssert(rval->_blockIndex==0);
    //OrkAssert(false);
  }

  //////////////////////////////////////////////

  delete[] unimtxstrides;
  delete[] uniarystrides;
  delete[] unisizes;
  delete[] unitypes;
  delete[] unioffsets;
  // delete[] uniblkidcs;
  delete[] uniindices;

  _uboBindingMap[block] = rval;

  return rval;
}

void Pass::bindUniformBlockBuffer(UniformBlock* block, UniformBuffer* buffer) {

  auto binding = uniformBlockBinding(block);
  assert(binding != nullptr);
  assert(binding->_blockIndex != GL_INVALID_INDEX);
  GLuint ubo_bindingindex = binding->_blockIndex;
  GLuint ubo_bindingpoint = binding->_bindingPoint;

  if (_ubobindings.size() < (ubo_bindingindex + 1)) {
    _ubobindings.resize(ubo_bindingindex + 1);
    //logchan_pass->log("RESIZEUBOB<%d>", ubo_bindingindex + 1);
  }

  if(_ubobindings[ubo_bindingpoint] != buffer) {
    GLintptr ubo_offset = 0;
    GLintptr ubo_size   = buffer->_length;
    GL_ERRORCHECK();
    // glBindBufferRange(GL_UNIFORM_BUFFER,   // target
    //                ubo_bindingindex,    // index
    //              buffer->_glbufid, // buffer objid
    //            ubo_offset,          // offset
    //          ubo_size);           // length
    glBindBufferBase(
        GL_UNIFORM_BUFFER, // target
        ubo_bindingpoint,  // index
        buffer->_glbufid); // buffer objid
    //logchan_pass->log("glBindBufferRange bidx<%d> bufid<%d>", int(ubo_bindingindex), int(buffer->_glbufid));
    GL_ERRORCHECK();
    _ubobindings[ubo_bindingpoint] = buffer;

  }
}

////////////////////////////////////////////////////////////////////////////////

void Pass::postProc(rootcontainer_ptr_t container) {
  Timer pptimer;
  pptimer.Start();
  auto flatunimap = container->flatUniMap();
  //////////////////////////
  // query unis
  //////////////////////////

  GLint numunis = 0;
  GL_ERRORCHECK();
  glGetProgramiv(_programObjectId, GL_ACTIVE_UNIFORMS, &numunis);
  GL_ERRORCHECK();

  _samplerCount = 0;

  for (int i = 0; i < numunis; i++) {
    GLsizei namlen = 0;
    GLint unisiz   = 0;
    GLenum unityp  = GL_ZERO;
    std::string str_name;

    {
      GLchar nambuf[256];
      glGetActiveUniform(_programObjectId, i, sizeof(nambuf), &namlen, &unisiz, &unityp, nambuf);
      OrkAssert(namlen < sizeof(nambuf));
      // printf("find uni<%s>", nambuf);
      GL_ERRORCHECK();

      str_name = nambuf;
      auto its = str_name.find('[');
      if (its != str_name.npos) {
        str_name = str_name.substr(0, its);
        // printf( "nnam<%s>", str_name.c_str() );
      }
    }
    auto it = container->_uniforms.find(str_name);
    if (it != container->_uniforms.end()) {

      uniform_ptr_t puni = it->second;

      puni->_type = unityp;
      OrkAssert(unityp != GL_ZERO);

      UniformInstance* pinst = new UniformInstance;
      pinst->mpUniform       = puni;

      GLint uniloc     = glGetUniformLocation(_programObjectId, str_name.c_str());
      pinst->mLocation = uniloc;

      if (puni->_typeName == "sampler2D") {
        pinst->mSubItemIndex = this->_samplerCount++;
        pinst->mPrivData.set<GLenum>(GL_TEXTURE_2D);
      } else if (puni->_typeName == "usampler2D") {
        pinst->mSubItemIndex = this->_samplerCount++;
        pinst->mPrivData.set<GLenum>(GL_TEXTURE_2D);
      } else if (puni->_typeName == "sampler3D") {
        pinst->mSubItemIndex = this->_samplerCount++;
        pinst->mPrivData.set<GLenum>(GL_TEXTURE_3D);
      } else if (puni->_typeName == "usampler3D") {
        pinst->mSubItemIndex = this->_samplerCount++;
        pinst->mPrivData.set<GLenum>(GL_TEXTURE_3D);
      } else if (puni->_typeName == "sampler2DShadow") {
        pinst->mSubItemIndex = this->_samplerCount++;
        pinst->mPrivData.set<GLenum>(GL_TEXTURE_2D);
      }

      this->_uniformInstances[puni->_name] = pinst;
    } else {
      it = flatunimap.find(str_name);
      //printf("uni<%s> not found!", str_name.c_str());
      OrkAssert(it != flatunimap.end());
      // prob a UBO uni
    }
  }
  double postproc_time = pptimer.SecsSinceStart();
   //printf( "postproctime<%f>\n", postproc_time );
}

} // namespace ork::lev2::glslfx
