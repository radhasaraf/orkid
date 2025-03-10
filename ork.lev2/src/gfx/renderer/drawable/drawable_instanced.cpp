////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/renderable.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/fx_pipeline.h>

#include <ork/reflect/properties/registerX.inl>

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////
InstancedDrawable::InstancedDrawable() 
  : Drawable() {
  _instancedata = std::make_shared<InstancedDrawableInstanceData>();
  _drawcount = 0;
}
///////////////////////////////////////////////////////////////////////////////
void InstancedDrawable::resize(size_t count) {
  OrkAssert(count <= k_max_instances);
  _instancedata->resize(count);
  _count = count;
}
///////////////////////////////////////////////////////////////////////////////
drawablebufitem_ptr_t InstancedDrawable::enqueueOnLayer( const DrawQueueXfData& xfdata, //
                                                          DrawableBufLayer& buffer) const {
  auto instances_copy = std::make_shared<InstancedDrawableInstanceData>();
  *instances_copy = *_instancedata;
  drawablebufitem_ptr_t dbufitem = Drawable::enqueueOnLayer(xfdata, buffer);
  dbufitem->_usermap["rtthread_instance_data"_crcu].set<instanceddrawinstancedata_ptr_t>(instances_copy);
  //printf( "_instancedata.count<%zu>\n", _instancedata->_count );
  return dbufitem;
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2 {
