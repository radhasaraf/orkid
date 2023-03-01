////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/reflect/properties/registerX.inl>
#include <ork/lev2/gfx/particle/modular_particles2.h>
#include <ork/lev2/gfx/particle/modular_emitters.h>
#include <ork/dataflow/module.inl>
#include <ork/dataflow/plug_data.inl>

using namespace ork::dataflow;

namespace ork::lev2::particle {

struct NozzleEmitterInst;

//////////////////////////////////////////////////////////////////////////

struct NozzleDirectedEmitter : public DirectedEmitter {

  NozzleDirectedEmitter(NozzleEmitterInst* emitterModule);
  void computePosDir(float fi, fvec3& pos, fvec3& dir) final;

  NozzleEmitterInst* _emitterModule;
};

struct NozzleEmitterInst : public ParticleModuleInst {

  NozzleEmitterInst(const NozzleEmitterData* ned);

  void onLink(GraphInst* inst) final;
  void compute(GraphInst* inst, ui::updatedata_ptr_t updata) final;
  void _emit(float fdt);
  void _reap(float fdt);

  float _timeAccumulator = 0.0f;
  fvec3 _lastPosition;
  fvec3 _lastDirection;
  fvec3 _curDirection;
  fvec3 _curOffset;

  NozzleDirectedEmitter _directedEmitter;
  EmitterCtx _emitter_context;


  floatxf_inp_pluginst_ptr_t _input_lifespan;
  floatxf_inp_pluginst_ptr_t _input_emissionrate;
  floatxf_inp_pluginst_ptr_t _input_emissionvelocity;
  floatxf_inp_pluginst_ptr_t _input_dispersionangle;

  fvec3xf_inp_pluginst_ptr_t _input_direction;
  fvec3xf_inp_pluginst_ptr_t _input_offset;
  fvec3xf_inp_pluginst_ptr_t _input_offset_velocity;


  float _updaterate = 30.0f;
  std::mt19937 _randgen;
  std::uniform_int_distribution<> _distribution;
  std::function<void(float min, float max)> _rangedf;
};

//////////////////////////////////////////////////////////////////////////

NozzleEmitterInst::NozzleEmitterInst(const NozzleEmitterData* ned)
    : ParticleModuleInst(ned)
    , _directedEmitter(this){

}

///////////////////////////////////////////////////////////////////////////////
void NozzleEmitterInst::onLink(GraphInst* inst) {
  _onLink(inst);
  _input_lifespan         = typedInputNamed<FloatXfPlugTraits>("LifeSpan");
  _input_emissionrate     = typedInputNamed<FloatXfPlugTraits>("EmissionRate");
  _input_emissionvelocity = typedInputNamed<FloatXfPlugTraits>("EmissionVelocity");
  _input_dispersionangle  = typedInputNamed<FloatXfPlugTraits>("DispersionAngle");
  _input_direction       = typedInputNamed<Vec3XfPlugTraits>("Direction");
  _input_offset          = typedInputNamed<Vec3XfPlugTraits>("Offset");
  _input_offset_velocity = typedInputNamed<Vec3XfPlugTraits>("OffsetVelocity");
}
///////////////////////////////////////////////////////////////////////////////
void NozzleEmitterInst::compute(GraphInst* inst, ui::updatedata_ptr_t updata) {

  if(_pool == nullptr)
    return;

  _timeAccumulator += updata->_dt;

  _lastPosition  = _curOffset;
  _lastDirection = _curDirection;

  _curDirection = _input_direction->value();
  _curOffset    = _input_offset->value();

  if (_curDirection.magnitudeSquared() == 0.0f) {
    _curDirection = fvec3::Green();
  }

  float fdelta = 1.0f / _updaterate;

  if (_timeAccumulator >= fdelta) { // limit to 30hz
    _timeAccumulator -= fdelta;
    _reap(fdelta);
    _emit(fdelta);
  }
}
///////////////////////////////////////////////////////////////////////////////
void NozzleEmitterInst::_emit(float fdt) {

  float femitvel                      = _input_emissionvelocity->value();
  float lifespan                      = std::clamp<float>(_input_lifespan->value(), 0.01f, 10.0f);
  float emissionrate                  = _input_emissionrate->value();
  _emitter_context.mPool              = _pool.get();
  _emitter_context.mfEmissionRate     = emissionrate;
  _emitter_context.mKey               = (void*)this;
  _emitter_context.mfLifespan         = lifespan;
  _emitter_context.mfDeltaTime        = fdt;
  _emitter_context.mfEmissionVelocity = femitvel;
  _emitter_context.mDispersion        = _input_dispersionangle->value();
  _directedEmitter.meDirection        = EmitterDirection::CONSTANT;
  fvec3 dir                           = _input_direction->value();
  _emitter_context.mPosition          = _input_offset->value();
  fvec3 offsetVel                     = _input_offset_velocity->value();
  _emitter_context.mOffsetVelocity    = offsetVel;
  _directedEmitter.Emit(_emitter_context);
}
///////////////////////////////////////////////////////////////////////////////
void NozzleEmitterInst::_reap(float fdt) {

  _emitter_context.mPool       = _pool.get();
  _emitter_context.mfDeltaTime = fdt;
  _emitter_context.mKey        = (void*)this;
  _directedEmitter.Reap(_emitter_context);
}

//////////////////////////////////////////////////////////////////////////

NozzleDirectedEmitter::NozzleDirectedEmitter(NozzleEmitterInst* emitterModule)
    : _emitterModule(emitterModule) {
}

//////////////////////////////////////////////////////////////////////////

void NozzleDirectedEmitter::computePosDir(float fi, fvec3& pos, fvec3& dir) {
  pos.lerp(_emitterModule->_lastPosition, _emitterModule->_curOffset, fi);
  dir.lerp(_emitterModule->_lastDirection, _emitterModule->_curDirection, fi);
}

//////////////////////////////////////////////////////////////////////////

void NozzleEmitterData::describeX(class_t* clazz) {
}

//////////////////////////////////////////////////////////////////////////

NozzleEmitterData::NozzleEmitterData() {
}

//////////////////////////////////////////////////////////////////////////

std::shared_ptr<NozzleEmitterData> NozzleEmitterData::createShared() {
  auto data = std::make_shared<NozzleEmitterData>();
  _initShared(data);
  //RegisterFloatXfPlug(NozzleEmitter, Lifespan, 0.0f, 20.0f, ged::OutPlugChoiceDelegate);
  //RegisterFloatXfPlug(NozzleEmitter, EmissionRate, 0.0f, 400.0f, ged::OutPlugChoiceDelegate);
  //RegisterFloatXfPlug(NozzleEmitter, EmissionVelocity, -100.0f, 100.0f, ged::OutPlugChoiceDelegate);
  //RegisterFloatXfPlug(NozzleEmitter, DispersionAngle, 0.0f, 1.0f, ged::OutPlugChoiceDelegate);
  //RegisterVect3XfPlug(NozzleEmitter, Offset, -100.0f, 100.0f, ged::OutPlugChoiceDelegate);
  //RegisterVect3XfPlug(NozzleEmitter, Direction, -1.0f, 1.0f, ged::OutPlugChoiceDelegate);
  //RegisterVect3XfPlug(NozzleEmitter, OffsetVelocity, -100.0f, 100.0f, ged::OutPlugChoiceDelegate);

  createInputPlug<FloatXfPlugTraits>(data, EPR_UNIFORM, "LifeSpan");
  createInputPlug<FloatXfPlugTraits>(data, EPR_UNIFORM, "EmissionRate");
  createInputPlug<FloatXfPlugTraits>(data, EPR_UNIFORM, "EmissionVelocity");
  createInputPlug<FloatXfPlugTraits>(data, EPR_UNIFORM, "DispersionAngle");
  createInputPlug<Vec3XfPlugTraits>(data, EPR_UNIFORM, "Direction");
  createInputPlug<Vec3XfPlugTraits>(data, EPR_UNIFORM, "Offset");
  createInputPlug<Vec3XfPlugTraits>(data, EPR_UNIFORM, "OffsetVelocity");

  return data;
}

//////////////////////////////////////////////////////////////////////////

dgmoduleinst_ptr_t NozzleEmitterData::createInstance() const {
  return std::make_shared<NozzleEmitterInst>(this);
}

//////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::particle
//////////////////////////////////////////////////////////////////////////

namespace ptcl = ork::lev2::particle;

ImplementReflectionX(ptcl::NozzleEmitterData, "psys::NozzleEmitterData");