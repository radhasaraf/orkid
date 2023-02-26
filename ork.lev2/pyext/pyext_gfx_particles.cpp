////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "pyext.h"
#include <ork/lev2/gfx/particle/modular_particles2.h>
#include <ork/lev2/gfx/particle/modular_emitters.h>
#include <ork/lev2/gfx/particle/modular_forces.h>
#include <ork/lev2/gfx/particle/modular_renderers.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork::lev2 {

namespace dflow = dataflow;
namespace ptc = particle;

void pyinit_gfx_particles(py::module& module_lev2) {
  auto ptc_module  = module_lev2.def_submodule("particles", "lev2 dataflow particles");
  auto type_codec = python::TypeCodec::instance();

  /////////////////////////////////////////////////////////////////////////////
  auto moduledata_type = //
      py::class_<ptc::ModuleData, dflow::DgModuleData, ptc::moduledata_ptr_t>(ptc_module, "Module")
      .def("__repr__", [](ptc::moduledata_ptr_t m) -> std::string {
          return FormatString("ptc::ModuleData(%p)", (void*)m.get());
      });
  type_codec->registerStdCodec<ptc::moduledata_ptr_t>(moduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto ptcmoduledata_type = //
      py::class_<ptc::ParticleModuleData, ptc::ModuleData, ptc::ptcmoduledata_ptr_t>(ptc_module, "ParticleModule");
  type_codec->registerStdCodec<ptc::ptcmoduledata_ptr_t>(ptcmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto globmoduledata_type = //
      py::class_<ptc::GlobalModuleData, ptc::ModuleData, ptc::globalmodule_ptr_t>(ptc_module, "Globals")
      .def_static("createShared", []() -> ptc::globalmodule_ptr_t { return ptc::GlobalModuleData::createShared(); });
  type_codec->registerStdCodec<ptc::globalmodule_ptr_t>(globmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto poolmoduledata_type = //
      py::class_<ptc::ParticlePoolData, ptc::ModuleData, ptc::poolmodule_ptr_t>(ptc_module, "Pool")
      .def_static("createShared", []() -> ptc::poolmodule_ptr_t { return ptc::ParticlePoolData::createShared(); })
      .def_property("pool_size", 
        [](ptc::poolmodule_ptr_t  m) -> int { //
          return m->_poolSize;
        },
        [](ptc::poolmodule_ptr_t  m, int count) { //
          return m->_poolSize = count;
        }
        );
  type_codec->registerStdCodec<ptc::poolmodule_ptr_t>(poolmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto nzlmoduledata_type = //
      py::class_<ptc::NozzleEmitterData, ptc::ModuleData, ptc::nozzleemittermodule_ptr_t>(ptc_module, "NozzleEmitter")
      .def_static("createShared", []() -> ptc::nozzleemittermodule_ptr_t { return ptc::NozzleEmitterData::createShared(); });
  type_codec->registerStdCodec<ptc::nozzleemittermodule_ptr_t>(nzlmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto ringmoduledata_type = //
      py::class_<ptc::RingEmitterData, ptc::ModuleData, ptc::ringemittermodule_ptr_t>(ptc_module, "RingEmitter")
      .def_static("createShared", []() -> ptc::ringemittermodule_ptr_t { return ptc::RingEmitterData::createShared(); });
  type_codec->registerStdCodec<ptc::ringemittermodule_ptr_t>(ringmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto grvmoduledata_type = //
      py::class_<ptc::GravityModuleData, ptc::ModuleData, ptc::gravitymodule_ptr_t>(ptc_module, "Gravity")
      .def_static("createShared", []() -> ptc::gravitymodule_ptr_t { return ptc::GravityModuleData::createShared(); });
  type_codec->registerStdCodec<ptc::gravitymodule_ptr_t>(grvmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto turbmoduledata_type = //
      py::class_<ptc::TurbulenceModuleData, ptc::ModuleData, ptc::turbulencemodule_ptr_t>(ptc_module, "Turbulence")
      .def_static("createShared", []() -> ptc::turbulencemodule_ptr_t { return ptc::TurbulenceModuleData::createShared(); });
  type_codec->registerStdCodec<ptc::turbulencemodule_ptr_t>(turbmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto vortmoduledata_type = //
      py::class_<ptc::VortexModuleData, ptc::ModuleData, ptc::vortexmodule_ptr_t>(ptc_module, "Vortex")
      .def_static("createShared", []() -> ptc::vortexmodule_ptr_t { return ptc::VortexModuleData::createShared(); });
  type_codec->registerStdCodec<ptc::vortexmodule_ptr_t>(vortmoduledata_type);
  /////////////////////////////////////////////////////////////////////////////
  auto spritemoduledata_type = //
      py::class_<ptc::SpriteRendererData, ptc::ModuleData, ptc::spritemodule_ptr_t>(ptc_module, "SpriteRenderer")
      .def_static("createShared", []() -> ptc::spritemodule_ptr_t { return ptc::SpriteRendererData::createShared(); });
  type_codec->registerStdCodec<ptc::spritemodule_ptr_t>(spritemoduledata_type);
}

} //namespace ork::lev2 {
