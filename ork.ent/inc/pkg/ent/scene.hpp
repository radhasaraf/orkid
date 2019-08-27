////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/application/application.h>
#include <ork/kernel/orklut.hpp>
#include <ork/rtti/downcast.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork {

class CCameraData;

namespace lev2 {
class XgmModel;
}
namespace lev2 {
class XgmModelInst;
}
namespace lev2 {
class Renderer;
}
namespace lev2 {
class LightManager;
}

namespace ent {

///////////////////////////////////////////////////////////////////////////////
/// SceneData is the "model" of the scene that is serialized and edited, and
/// thats it.... this should never get subclassed
///////////////////////////////////////////////////////////////////////////////

template <typename T>
std::set<EntData *> SceneData::FindEntitiesWithComponent() const {
  std::set<EntData *> rval;
  return rval;
}

template <typename T>
std::set<EntData *> SceneData::FindEntitiesOfArchetype() const {
  std::set<EntData *> rval;

  for (auto item : _sceneObjects) {
    EntData *isa_ent = rtti::autocast(item.second);

    if (isa_ent) {
      auto arch = isa_ent->GetArchetype();

      if (arch && arch->GetClass() == T::GetClassStatic()) {
        rval.insert(isa_ent);
      }
    }
  }

  return rval;
}

template <typename T> T *SceneData::FindTypedObject(const PoolString &pstr) {
  SceneObject *cd = FindSceneObjectByName(pstr);
  T *pret = (cd == 0) ? 0 : rtti::safe_downcast<T *>(cd);
  return pret;
}
template <typename T>
const T *SceneData::FindTypedObject(const PoolString &pstr) const {
  const SceneObject *cd = FindSceneObjectByName(pstr);
  T *pret = (cd == 0) ? 0 : rtti::safe_downcast<T *>(cd);
  return pret;
}
template <typename T> T *SceneData::getTypedSystemData() const {
  auto systemdataclass = T::GetClassStatic();
  auto it = _systemDatas.find(systemdataclass);
  if( it == _systemDatas.end() ){
    // CREATE IT !
    auto sdat = new T;
    _systemDatas[systemdataclass] = sdat;
  }
  else {
    return rtti::autocast(it->second);
  }
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> T *SceneComposer::Register() {
  ork::object::ObjectClass *pclass = T::GetClassStatic();
  auto it = mpSceneData->_systemDatas.find(pclass);
  SystemData *pobj = (it == mpSceneData->_systemDatas.end()) ? 0 : it->second;
  if(nullptr == pobj) {
    pobj = rtti::autocast(pclass->CreateObject());
    mpSceneData->_systemDatas.AddSorted(pclass, pobj);
  }
  T *rval = rtti::autocast(pobj);
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
/// SceneInst is all the work data associated with running a scene
/// this might be subclassed
///////////////////////////////////////////////////////////////////////////////

template <typename T>
T *SceneInst::FindTypedEntityComponent(const PoolString &entname) const {
  T *pret = 0;
  Entity *pent = FindEntity(entname);
  if (pent) {
    pret = pent->GetTypedComponent<T>();
  }
  printf("FINDENT<%s:%p> comp<%p>\n", entname.c_str(), pent, pret);
  return pret;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> T *SceneInst::findSystem() const {
  systemkey_t pclass = T::SystemType;
  auto it = _systems.find(pclass);
  return (it == _systems.end()) ? nullptr : dynamic_cast<T*>(it->second);
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
T *SceneInst::FindTypedEntityComponent(const char *entname) const {
  return FindTypedEntityComponent<T>(AddPooledString(entname));
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ent
} // namespace ork
