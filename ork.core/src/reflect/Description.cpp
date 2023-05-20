////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/reflect/Command.h>
#include <ork/reflect/Description.h>
#include <ork/reflect/IDeserializer.h>
#include <ork/reflect/properties/ObjectProperty.h>
#include <ork/reflect/ISerializer.h>

namespace ork { namespace reflect {

Description::Description()
    : _parentDescription(NULL) {
}

void Description::SetParentDescription(const Description* parent) {
  _parentDescription = parent;
}

void Description::addProperty(const char* key, ObjectProperty* value) {
  mProperties.AddSorted(key, value);
  value->_name = key;
}

Description::PropertyMapType& Description::properties() {
  return mProperties;
}

const Description::PropertyMapType& Description::properties() const {
  return mProperties;
}

const ObjectProperty* Description::property(const ConstString& name) const {
  for (const Description* description = this; description != NULL; description = description->_parentDescription) {
    const PropertyMapType& map         = description->properties();
    PropertyMapType::const_iterator it = map.find(name);

    if (it != map.end()) {
      return (*it).second;
    }
  }

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void Description::addFunctor(std::string key, IObjectFunctor* functor) {
  mFunctions.AddSorted(key, functor);
}

const IObjectFunctor* Description::findFunctor(const std::string& name) const {
  for (const Description* description = this; //
       description != NULL; //
       description = description->_parentDescription) { //
    const FunctorMapType& map         = description->mFunctions;
    FunctorMapType::const_iterator it = map.find(name);

    if (it != map.end()) {
      return (*it).second;
    }
  }

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void Description::addSignal(std::string key, object::Signal Object::*pmember) {
  mSignals.AddSorted(key, pmember);
}
void Description::addAutoSlot(std::string key, object::AutoSlot Object::*pmember) {
  mAutoSlots.AddSorted(key, pmember);
}

object::Signal Object::*Description::findSignal(const std::string& named) const {
  for (const Description* description = this; //
       description != NULL; //
       description = description->_parentDescription) { //
    const SignalMapType& map         = description->mSignals;
    SignalMapType::const_iterator it = map.find(named);

    if (it != map.end()) {
      return (*it).second;
    }
  }

  return NULL;
}

object::AutoSlot Object::*Description::FindAutoSlot(const std::string& named) const {
  for (const Description* description = this; //
      description != NULL; //
      description = description->_parentDescription) { //
    const AutoSlotMapType& map         = description->mAutoSlots;
    AutoSlotMapType::const_iterator it = map.find(named);

    if (it != map.end()) {
      return (*it).second;
    }
  }

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void Description::annotateProperty(const ConstString& propname, const ConstString& key, const ConstString& val) {
  const PropertyMapType& map         = properties();
  PropertyMapType::const_iterator it = map.find(propname);
  if (it == map.end()) {
    OrkAssert(false);
  } else {
    (*it).second->Annotate(key, val);
  }
}

ConstString Description::propertyAnnotation(const ConstString& propname, const ConstString& key) const {
  for (const Description* description = this; description != NULL; description = description->_parentDescription) {
    const PropertyMapType& map         = description->properties();
    PropertyMapType::const_iterator it = map.find(propname);
    if (it != map.end()) {
      return (*it).second->GetAnnotation(key);
    }
  }
  return "";
}

void Description::annotateClass(const ConstString& key, const Description::anno_t& val) {
  orklut<ConstString, anno_t>::const_iterator it = mClassAnnotations.find(key);
  if (it == mClassAnnotations.end()) {
    mClassAnnotations.AddSorted(key, val);
  } else {
    OrkAssert(false);
  }
}

const Description::anno_t& Description::classAnnotation(const ConstString& key) const {
  for (const Description* description = this; //
       description != NULL; //
       description = description->_parentDescription) { //
    orklut<ConstString, anno_t>::const_iterator it = description->mClassAnnotations.find(key);
    if (it != description->mClassAnnotations.end()) {
      return (*it).second;
    }
  }
  static const anno_t empty_anno(nullptr);
  return empty_anno;
}

}} // namespace ork::reflect
