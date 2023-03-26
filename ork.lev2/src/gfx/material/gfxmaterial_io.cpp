////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/kernel/prop.h>
#include <ork/kernel/prop.hpp>
#include <ork/lev2/gfx/shadman.h>
#include <ork/lev2/gfx/camera/uicam.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/gfx/gfxenv_enum.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/gfx/gfxmaterial.h>
#include <ork/application/application.h>
#include <ork/file/tinyxml/tinyxml.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 {
///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool LoadMaterialMap(const ork::file::Path& pth, MaterialMap& mmap) {
  bool rval = false;

  TiXmlDocument XmlDoc;
  if (XmlDoc.LoadFile(pth.toAbsolute().c_str())) {
    const TiXmlElement* RootNode = XmlDoc.FirstChild("miniork_materials")->ToElement();
    printf("LoadMaterialMap<%s> RootNode<%p>\n", pth.c_str(), (void*) RootNode);

    if (RootNode) {
      for (const TiXmlElement* MtlNode = RootNode->FirstChildElement(); MtlNode; MtlNode = MtlNode->NextSiblingElement()) {
        if (strcmp(MtlNode->Value(), "material") == 0) {
          const char* MaterialName  = MtlNode->Attribute("name");
          const char* MaterialClass = MtlNode->Attribute("class");
          printf(" found material<%s> class<%s>\n", MaterialName, MaterialClass);

          auto pclazz = dynamic_cast<object::ObjectClass*>(rtti::Class::FindClass(MaterialClass));

          if (pclazz) {
            auto pmtlobj = pclazz->createShared();
            auto pmtl    = std::dynamic_pointer_cast<lev2::GfxMaterial>(pmtlobj);
            OrkAssert(pmtl != 0);

            for (const TiXmlElement* PropNode = MtlNode->FirstChildElement(); PropNode; PropNode = PropNode->NextSiblingElement()) {
              OrkAssert(0 == strcmp(PropNode->Value(), "prop"));

              const char* KeyName  = PropNode->Attribute("key");
              const char* ValueStr = PropNode->Attribute("value");

              printf("   property<%s> value<%s>\n", KeyName, ValueStr);

              pmtl->SetMaterialProperty(KeyName, ValueStr);
            }

            mmap[MaterialName] = pmtl;
          }
        }
      }
    }
  }

  rval = mmap.size() != 0;

  return rval;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
}} // namespace ork::lev2
