////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/asset/Asset.inl>
#include "gl.h"

///////////////////////////////////////////////////////////////////////////////

ImplementReflectionX(ork::lev2::ContextGL, "ContextGL");

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 {
///////////////////////////////////////////////////////////////////////////////

ork::MpMcBoundedQueue<load_token_t> ContextGL::_loadTokens;

GlPlatformObject* GlPlatformObject::_current = nullptr;

void GlPlatformObject::makeCurrent() {
    _current = this;
    if(_ctxbase){
      auto window = _ctxbase->_glfwWindow;
      printf( "_glfwWindow<%p> made current\n", (void*) window );
      glfwMakeContextCurrent(window);
    }
    else{
      OrkAssert(false);
    }
    //_ctxbase->makeCurrent();
}
void GlPlatformObject::swapBuffers() {

}

void touchClasses() {
  ContextGL::GetClassStatic();
}

namespace opengl{
  context_ptr_t createLoaderContext() {

    ///////////////////////////////////////////////////////////
    auto loader = std::make_shared<FxShaderLoader>();
    FxShader::RegisterLoaders("shaders/glfx/", "glfx");
    auto shadctx = FileEnv::contextForUriProto("orkshader://");
    auto democtx = FileEnv::contextForUriProto("demo://");
    loader->addLocation(shadctx, ".glfx"); // for glsl targets
    if (democtx) {
      loader->addLocation(democtx, ".glfx"); // for glsl targets
    }
    ///////////////////////////////////////////////////////////

    asset::registerLoader<FxShaderAsset>(loader);

    //_GVI       = std::make_shared<VulkanInstance>();
    auto clazz = dynamic_cast<object::ObjectClass*>(ContextGL::GetClassStatic());
    GfxEnv::setContextClass(clazz);
    auto target = std::make_shared<ContextGL>();
    target->initializeLoaderContext();
    GfxEnv::initializeWithContext(target);
    return target;
  }  
}

extern std::atomic<int> __FIND_IT;

GlPlatformObject::GlPlatformObject(): _bindop([=](){}) {}
GlPlatformObject::~GlPlatformObject() {}

void ContextGL::describeX(class_t* clazz) {
  clazz->annotateTyped<context_factory_t>("context_factory", []() { //
    return std::make_shared<ContextGL>();
  });
  __FIND_IT.store(0);
}

std::string indent(int count) {
  std::string rval = "";
  for (int i = 0; i < count; i++)
    rval += "  ";
  return rval;
}
static thread_local int _dbglevel = 0;
static thread_local std::stack<std::string> _groupstack;

void ContextGL::debugPopGroup(commandbuffer_ptr_t cb){
  OrkAssert(false);
}
void ContextGL::debugPushGroup(commandbuffer_ptr_t cb, const std::string str, const fvec4& color){
  OrkAssert(false);
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void ContextGL::debugPushGroup(const std::string str, const fvec4& color) {
    int level = _dbglevel++;
    auto mstr = indent(level) + str;
    //printf( "PSHGRP CTX<%p> lev<%d> name<%s>\n", this, level, mstr.c_str() );
    _groupstack.push(mstr);
    GL_ERRORCHECK();
    glPushGroupMarkerEXT(mstr.length(), mstr.c_str());
}
/////////////////////////////////////////////////////////////////////////
void ContextGL::debugPopGroup() {
    std::string top = _groupstack.top();
    //printf( "POPGRP CTX<%p> lev<%d> name<%s>\n", this,  _dbglevel, top.c_str() );
    // auto mstr = indent(_dbglevel--) + _prevgroup;
    _groupstack.pop();
    GL_ERRORCHECK();
    glPopGroupMarkerEXT();
    GL_ERRORCHECK();
    _dbglevel--;
}
/////////////////////////////////////////////////////////////////////////
void ContextGL::debugMarker(const std::string str, const fvec4& color) {
}
/////////////////////////////////////////////////////////////////////////
void ContextGL::debugLabel(GLenum target, GLuint object, std::string name) {
  glLabelObjectEXT(target, object, name.length(), name.c_str());
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
#else // LINUX
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void ContextGL::debugLabel(GLenum target, GLuint object, std::string name) {
  glObjectLabel(target, object, name.length(), name.c_str());
}

/////////////////////////////////////////////////////////////////////////

void ContextGL::debugPushGroup(const std::string str, const fvec4& color) {
  int level = _dbglevel++;
  auto mstr = indent(level) + str;
  //printf( "PSHGRP CTX<%p> lev<%d> name<%s>\n", (void*) this, level, mstr.c_str() );
  _groupstack.push(mstr);
  GL_ERRORCHECK();
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, mstr.length(), mstr.c_str());
  GL_ERRORCHECK();
  __FIND_IT.fetch_add(1);
}

/////////////////////////////////////////////////////////////////////////

void ContextGL::debugPopGroup() {
  std::string top = _groupstack.top();
  _groupstack.pop();
  //printf( "POPGRP CTX<%p> lev<%d> name<%s>\n", (void*) this, _dbglevel, top.c_str() );
  if(__FIND_IT.exchange(0)==1){
    //OrkAssert(false);
  }
  GL_ERRORCHECK();
  glPopDebugGroup();
  GL_ERRORCHECK();
  _dbglevel--;
}
/////////////////////////////////////////////////////////////////////////

void ContextGL::debugMarker(const std::string str, const fvec4& color) {
  auto mstr = indent(_dbglevel) + str;
  // printf( "Marker:: %s\n", mstr.c_str() );

  GL_ERRORCHECK();
  glDebugMessageInsert(
      GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, mstr.length(), mstr.c_str());
  GL_ERRORCHECK();
}
#endif

/////////////////////////////////////////////////////////////////////////

bool ContextGL::SetDisplayMode(DisplayMode* mode) {
  return false;
}

/////////////////////////////////////////////////////////////////////////

void recomputeHIDPI(GLFWwindow *glfw_window);

void ContextGL::_doResizeMainSurface(int iw, int ih) {
  
  miW                      = iw;
  miH                      = ih;
  mTargetDrawableSizeDirty = true;
  auto plato = _impl.getShared<GlPlatformObject>();
  auto ctx_glfw = plato->_ctxbase;
  auto win_glfw = ctx_glfw->_glfwWindow;
  recomputeHIDPI(win_glfw);
}

/////////////////////////////////////////////////////////////////////////

std::string GetGlErrorString(int iGLERR) {
  std::string RVAL = (std::string) "GL_UNKNOWN_ERROR";

  switch (iGLERR) {
    case GL_NO_ERROR:
      RVAL = "GL_NO_ERROR";
      break;
    case GL_INVALID_ENUM:
      RVAL = (std::string) "GL_INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      RVAL = (std::string) "GL_INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      RVAL = (std::string) "GL_INVALID_OPERATION";
      break;
      //	case GL_STACK_OVERFLOW:
      //	RVAL =  (std::string) "GL_STACK_OVERFLOW";
      //	break;
      //		case GL_STACK_UNDERFLOW:
      //			RVAL =  (std::string) "GL_STACK_UNDERFLOW";
      //			break;
    case GL_OUT_OF_MEMORY:
      RVAL = (std::string) "GL_OUT_OF_MEMORY";
      break;
    default:
      break;
  }
  return RVAL;
}

/////////////////////////////////////////////////////////////////////////

void check_debug_log();

int GetGlError(void) {
  int err = glGetError();

  if (err != GL_NO_ERROR) {
    std::string errstr = GetGlErrorString(err);
    orkprintf("GLERROR [%s]\n", errstr.c_str());
    check_debug_log();
  }

  return err;
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork::lev2
///////////////////////////////////////////////////////////////////////////////
