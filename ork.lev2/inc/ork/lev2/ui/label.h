////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/lev2/ui/widget.h>

namespace ork::ui {

////////////////////////////////////////////////////////////////////
// Simple (Colored) Label Widget
//  mostly used for testing, but if you need a colored box...
////////////////////////////////////////////////////////////////////

struct Label final : public Widget {
public:
  Label(
      const std::string& name, //
      fvec4 color,
      std::string label);
  fvec4 _color;
  fvec4 _textcolor;
  std::string _label;
  std::string _font = "i14";

private:
  void DoDraw(ui::drawevent_constptr_t drwev) override;
};

using label_ptr_t = std::shared_ptr<Label>;

} // namespace ork::ui
