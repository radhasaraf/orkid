////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

namespace ork::dataflow {

typedef std::string MorphKey;
typedef std::string MorphGroup;

struct floatxfdata;
struct floatxfinst;
struct fvec3xfdata;
struct fvec3xfinst;

struct FloatPlugTraits{
  using data_type_t = float;
  using inst_type_t = float;
  static constexpr size_t max_fanout = 0;
};
struct Vec3fPlugTraits{
  using data_type_t = fvec3;
  using inst_type_t = fvec3;
  static constexpr size_t max_fanout = 0;
};
struct FloatXfPlugTraits{
  using data_type_t = floatxfdata;
  using inst_type_t = floatxfinst;
  static constexpr size_t max_fanout = 0;
};
struct Vec3XfPlugTraits{
  using data_type_t = fvec3xfdata;
  using inst_type_t = fvec3xfinst;
  static constexpr size_t max_fanout = 0;
};


struct morph_event : public event::Event {
public:
  EMorphEventType meType;
  float mfMorphValue;
  MorphGroup mMorphGroup;

  morph_event()
      : meType(EMET_END)
      , mfMorphValue(0.0f) {
  }
};

struct imorphtarget {
  virtual void Apply() = 0;
};
struct float_morphtarget : public imorphtarget {
  float mfValue;
};

struct morphitem {
  MorphKey mKey;
  float mWeight;

  morphitem()
      : mWeight(0.0f) {
  }
};

struct morphable {
  void HandleMorphEvent(const morph_event* me);
  virtual void WriteMorphTarget(MorphKey name, float flerpval) = 0;
  virtual void RecallMorphTarget(MorphKey name)                = 0;
  virtual void Morph1D(const morph_event* pevent)              = 0;

  static const int kmaxweights = 2;
  morphitem mMorphItems[kmaxweights];
};

///////////////////////////////////////////////////////////////////////////////

struct PlugData : public ork::Object {
  DeclareAbstractX(PlugData, ork::Object);

public:

  PlugData(moduledata_ptr_t pmod, EPlugDir edir, EPlugRate epr, const std::type_info& tid, const char* pname);

  const std::type_info& GetDataTypeId() const {
    return mTypeId;
  }

  template <typename T> std::shared_ptr<T> typedModuleData(){
    return std::dynamic_pointer_cast<T>(_parent_module);
  }

  moduledata_ptr_t _parent_module;
  EPlugDir _plugdir;
  EPlugRate _plugrate;
  const std::type_info& mTypeId;
  std::string _name;

};

template <typename T> std::shared_ptr<T> typedPlugData(plugdata_ptr_t p){
    return std::dynamic_pointer_cast<T>(p);
}


///////////////////////////////////////////////////////////////////////////////

struct InPlugData : public PlugData {

  DeclareAbstractX(InPlugData, PlugData);

  static constexpr size_t NOPATH = 0xffffffffffffffff;

public:

  InPlugData(moduledata_ptr_t pmod, EPlugRate epr, const std::type_info& tid, const char* pname);
  ~InPlugData();

  bool isConnected() const;
  bool isMorphable() const;
  size_t computeMinDepth(dgmoduledata_constptr_t to_module ) const;
  virtual inpluginst_ptr_t createInstance() const;

  outplugdata_ptr_t _connectedOutput;                       // which EXTERNAL output plug are we connected to
  orkvector<outplugdata_ptr_t> _internalOutputConnections; // which output plugs IN THE SAME MODULE are connected to me ?
  morphable_ptr_t mpMorphable;
};

///////////////////////////////////////////////////////////////////////////////

struct OutPlugData : public PlugData {

  DeclareAbstractX(OutPlugData, PlugData);

public:

  OutPlugData(moduledata_ptr_t pmod, EPlugRate epr, const std::type_info& tid, const char* pname);
  ~OutPlugData();

  virtual size_t maxFanOut() const {
    return 0;
  }

  bool isConnected() const {
    return (numConnections() != 0);
  }

  size_t numConnections() const {
    return _connections.size();
  }
  inplugdata_ptr_t connected(size_t idx) const {
    return _connections[idx];
  }

  void _disconnect(inplugdata_ptr_t pinplug);

  virtual outpluginst_ptr_t createInstance() const;

  mutable orkvector<inplugdata_ptr_t> _connections;

};

///////////////////////////////////////////////////////////////////////////////

template <typename traits> //
struct outplugdata : public OutPlugData {
  DeclareTemplateAbstractX(outplugdata<traits>, OutPlugData);

public:

  using traits_t = traits;
  using data_type_t = typename traits_t::data_type_t;
  using data_type_ptr_t = std::shared_ptr<data_type_t>;

  inline explicit outplugdata( moduledata_ptr_t pmod, //
                               EPlugRate epr,
                               const char* pname) //
      : OutPlugData(pmod, epr, typeid(data_type_t), pname) { //

      _value = std::make_shared<data_type_t>();

  }

  inline size_t maxFanOut() const override {
    return traits::max_fanout;
  }

  inline const data_type_t& value() const {
    return (*_value);
  }

  inline void setValue(const data_type_t& v) {
    (*_value) = v;
  }


  outpluginst_ptr_t createInstance() const override;

  data_type_ptr_t _value;
};

///////////////////////////////////////////////////////////////////////////////

template <typename traits> struct inplugdata : public InPlugData {
  DeclareTemplateAbstractX(inplugdata<traits>, InPlugData);

public:

  using traits_t = traits;
  using data_type_t = typename traits_t::inst_type_t;
  using data_type_ptr_t = std::shared_ptr<data_type_t>;

  inline explicit inplugdata( moduledata_ptr_t pmod, //
                              EPlugRate epr, //
                              const char* pname) //
      : InPlugData(pmod, epr, typeid(traits), pname) { //
        _value = std::make_shared<data_type_t>();
  }

  inline const data_type_t& value() const {
    return (*_value);
  }

  inline void setValue(const data_type_t& v) {
    (*_value) = v;
  }

  data_type_ptr_t _value;
  inpluginst_ptr_t createInstance() const override;

};

///////////////////////////////////////////////////////////////////////////////

struct floatinplugdata : public inplugdata<FloatPlugTraits> {
  DeclareAbstractX(floatinplugdata, inplugdata<FloatPlugTraits>);

public:
  floatinplugdata(moduledata_ptr_t pmod, EPlugRate epr, const char* pname)
      : inplugdata<FloatPlugTraits>(pmod, epr, pname) {
  }
};

///////////////////////////////////////////////////////////////////////////////

struct vect3inplugdata : public inplugdata<Vec3fPlugTraits> {
  DeclareAbstractX(vect3inplugdata, inplugdata<Vec3fPlugTraits>);

public:
  vect3inplugdata(moduledata_ptr_t pmod, EPlugRate epr, const char* pname)
      : inplugdata<Vec3fPlugTraits>(pmod, epr, pname) {
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename transform_type> struct floatinplugxfdata : public floatinplugdata {

  DeclareTemplateAbstractX(floatinplugxfdata<transform_type>, floatinplugdata);

public:
  explicit floatinplugxfdata(moduledata_ptr_t pmod, EPlugRate epr, const char* pname)
      : floatinplugdata(pmod, epr, pname)
      , mtransform() {
  }
  ///////////////////////////////////////////////////////////////

  /*inline const float& GetValue() // virtual
  {
    outplugdata<float>* connected = 0;
    GetTypedInput(connected);
    mtransformed = mtransform.transform((connected != 0) ? (connected->GetValue()) : mDefault);
    return mtransformed;
  }*/

  transform_type mtransform;
  //mutable float mtransformed;
};

///////////////////////////////////////////////////////////////////////////////

template <typename transform_type> struct vect3inplugxfdata : public vect3inplugdata {
  DeclareTemplateAbstractX(vect3inplugxfdata<transform_type>, vect3inplugdata);

public:
  explicit vect3inplugxfdata(moduledata_ptr_t pmod, EPlugRate epr, const char* pname)
      : vect3inplugdata(pmod, epr, pname){
  }

  ///////////////////////////////////////////////////////////////

  /*inline const fvec3& GetValue() // virtual
  {
    outplugdata<fvec3>* connected = 0;
    typedInput(connected);
    mtransformed = mtransform.transform((connected != 0) ? (connected->GetValue()) : mDefault);
    return mtransformed;
  }*/

  transform_type _transform;
};

///////////////////////////////////////////////////////////////////////////////
struct modscabiasdata : public ork::Object {
  DeclareConcreteX(modscabiasdata, ork::Object);

public:

  float _mod = 0.0f;
  float _scale = 1.0f;
  float _bias = 0.0f;
};

///////////////////////////////////////////////////////////////////////////////

struct floatxfitembasedata : public ork::Object {
  DeclareAbstractX(floatxfitembasedata, ork::Object);

public:
  virtual float transform(float inp) const = 0;
};

using floatxfitembasedata_ptr_t = std::shared_ptr<floatxfitembasedata>;

///////////////////////////////////////////////////////////////////////////////

struct floatxfmsbcurvedata : public floatxfitembasedata {
  DeclareConcreteX(floatxfmsbcurvedata, floatxfitembasedata);
public:

  float transform(float input) const override; // virtual

  ork::MultiCurve1D _multicurve;
  modscabiasdata _modscalebias;
  bool _domodscalebias = false;
  bool _docurve = false;
};

///////////////////////////////////////////////////////////////////////////////

struct floatxfmodstepdata : public floatxfitembasedata {
  DeclareConcreteX(floatxfmodstepdata, floatxfitembasedata);

public:

  float transform(float input) const override; // virtual

  float _mod = 1.0f;
  int _steps = 4;
  float _outputScale = 1.0f;
  float _outputBias = 1.0f;
};

///////////////////////////////////////////////////////////////////////////////

struct floatxfdata : public ork::Object {
  DeclareAbstractX(floatxfdata, ork::Object);

public:
  float transform(float inp) const;
  floatxfdata();
  ~floatxfdata();

  orklut<std::string, floatxfitembasedata_ptr_t> _transforms;
  int _test = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct fvec3xfdata : public ork::Object {
  DeclareConcreteX(fvec3xfdata, ork::Object);
public:

  fvec3 transform(const fvec3& input) const;

  floatxfdata _transformX;
  floatxfdata _transformY;
  floatxfdata _transformZ;
};


using floatoutplug_ptr_t = std::shared_ptr<outplugdata<FloatPlugTraits>>;
using floatinpplug_ptr_t = std::shared_ptr<inplugdata<FloatPlugTraits>>;

using floatxfinplugdata = floatinplugxfdata<floatxfdata> ;
using vect3xfinplugdata = vect3inplugxfdata<fvec3xfdata>;
using floatxfinplugdata_ptr_t = std::shared_ptr<floatxfinplugdata>;


} //namespace ork { namespace dataflow {
