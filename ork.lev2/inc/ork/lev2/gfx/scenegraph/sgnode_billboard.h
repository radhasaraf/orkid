#pragma once
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/lev2_asset.h>
///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

struct BillboardDrawableData final : public DrawableData {

  DeclareConcreteX(BillboardDrawableData, DrawableData);

public:
  drawable_ptr_t createDrawable() const final;
  BillboardDrawableData();
  ~BillboardDrawableData();

  std::string _colortexpath;
  float _alpha = 1.0f;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
///////////////////////////////////////////////////////////////////////////////