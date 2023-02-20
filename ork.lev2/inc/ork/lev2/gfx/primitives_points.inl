////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once
#include <ork/lev2/gfx/meshutil/rigid_primitive.inl>
#include <ork/lev2/gfx/scenegraph/scenegraph.h>
#include <ork/lev2/gfx/fxstate_instance.h>

namespace ork::lev2::primitives {

//////////////////////////////////////////////////////////////////////////////

template <typename VertexType>
struct PointsPrimitive {

  using vtx_t = VertexType;
  using vtx_buf_t = DynamicVertexBuffer<vtx_t>;

  //////////////////////////////////////////////////////////////////////////////

  inline PointsPrimitive(int numpoints){
    _numpoints = numpoints;
    _vertexBuffer = std::make_shared<vtx_buf_t>(numpoints,0,PrimitiveType::POINTS);
  }

  //////////////////////////////////////////////////////////////////////////////

  inline vtx_t* lock(Context* context) {
    return (vtx_t*) context->GBI()->LockVB(*_vertexBuffer,0,_numpoints);
  }

  inline void unlock(Context* context) {
    context->GBI()->UnLockVB(*_vertexBuffer);
  }

  //////////////////////////////////////////////////////////////////////////////

  inline void renderEML(Context* context) {
    auto gbi = context->GBI();
    gbi->DrawPrimitiveEML(*_vertexBuffer, PrimitiveType::POINTS,0,_numpoints);
  }

  //////////////////////////////////////////////////////////////////////////////
  inline scenegraph::drawable_node_ptr_t createNode(
      std::string named, //
      scenegraph::layer_ptr_t layer,
      fxpipeline_ptr_t pipeline) {

    OrkAssert(pipeline);

    _pipeline = pipeline;

    auto drw = std::make_shared<CallbackDrawable>(nullptr);
    drw->SetRenderCallback([=](lev2::RenderContextInstData& RCID) { //
      auto context = RCID.context();
      _pipeline->wrappedDrawCall(RCID, //
                                 [this, context]() { //
                                  this->renderEML(context); //
                                });
    });
    return layer->createDrawableNode(named, drw);
  }
  //////////////////////////////////////////////////////////////////////////////

  int _numpoints = 0;
  fxpipeline_ptr_t _pipeline;
  std::shared_ptr<vtx_buf_t> _vertexBuffer;
};

///////////////////////////////////////////////////////////////////////////////

using points_v12c4_ptr_t = std::shared_ptr<PointsPrimitive<SVtxV12C4>>;
using points_v12t8_ptr_t = std::shared_ptr<PointsPrimitive<SVtxV12T8>>;

} // namespace ork::lev2::primitives