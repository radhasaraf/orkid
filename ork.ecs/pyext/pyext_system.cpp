////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "pyext.h"

///////////////////////////////////////////////////////////////////////////////

namespace ork::ecs {
void pyinit_system(py::module& module_ecs) {
  auto type_codec = python::TypeCodec::instance();
  /////////////////////////////////////////////////////////////////////////////////
  py::class_<SystemData,systemdata_ptr_t>(module_ecs, "SystemData")
      .def(
          "__repr__",
          [](const systemdata_ptr_t& sysdata) -> std::string {
            fxstring<256> fxs;
            fxs.format("ecs::SystemData(%p)", sysdata.get());
            return fxs.c_str();
          });
  /////////////////////////////////////////////////////////////////////////////////
} // void pyinit_system(py::module& module_ecs) {
/////////////////////////////////////////////////////////////////////////////////
} // namespace ork::ecs {