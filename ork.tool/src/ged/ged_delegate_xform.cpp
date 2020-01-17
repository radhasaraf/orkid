////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/qtui/qtui_tool.h>
///////////////////////////////////////////////////////////////////////////////
#include <orktool/ged/ged.h>
#include <orktool/ged/ged_delegate.h>
#include <orktool/ged/ged_io.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/reflect/IProperty.h>
#include <ork/reflect/IObjectProperty.h>
#include <ork/reflect/IObjectPropertyObject.h>
#include <ork/reflect/IObjectPropertyType.h>
#include "ged_delegate.hpp"
#include <ork/math/TransformNode.h>
//#include <ork/reflect/serialize/LayerDeserializer.h>
//#include <ork/reflect/serialize/ShallowSerializer.h>
//#include <ork/reflect/serialize/NullSerializer.h>
//#include <ork/reflect/serialize/NullDeserializer.h>
template class ork::tool::ged::GedSimpleNode<ork::tool::ged::GedIoDriver<ork::TransformNode>, ork::TransformNode>;
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace tool { namespace ged {
///////////////////////////////////////////////////////////////////////////////

class GedTransformWidget : public GedItemNode {
  std::string mCurrentValue;

  void DoDraw(lev2::Context* pTARG) { // virtual
    //////////////////////////////////////
    int ilabw = propnameWidth() + 16;
    // GedItemNode::DoDraw( pTARG );
    //////////////////////////////////////
    GetSkin()->DrawBgBox(this, miX + ilabw, miY + 2, miW - ilabw, miH - 3, GedSkin::ESTYLE_BACKGROUND_2);
    // GetSkin()->DrawText(this, miX + ilabw, miY + 5, mCurrentValue.c_str());
    GetSkin()->DrawText(this, miX + 6, miY + 5, _propname.c_str());
    //////////////////////////////////////
  }

public:
  GedTransformWidget(ObjModel& mdl, const char* name, const reflect::IObjectProperty* prop, ork::Object* obj)
      : GedItemNode(mdl, name, prop, obj) {
    //, ValueStrings(0)
    //, NumStrings(0)
    //, mbEmitModelRefresh(false)
    //, meReader(obj, prop)
    //
    //, meWriter(obj, prop) {
    ReSync();
  }

  void ReSync() {
    // mCurrentValue = meReader.GetCurrentValue();
  }

  void OnMouseDoubleClicked(const ork::ui::Event& ev) final {
    /*QMenu* pMenu = new QMenu(0);

    ///////////////////////////////////////////

    const orkmap<std::string, int>& enummap = meReader.GetEnumMap();

    ///////////////////////////////////////////

    for (orkmap<std::string, int>::const_iterator it = enummap.begin(); it != enummap.end(); it++) {
      const std::string& str = it->first;
      QAction* pchildmenu    = pMenu->addAction(str.c_str());
      pchildmenu->setData(QVariant(str.c_str()));
    }

    QAction* pact = pMenu->exec(QCursor::pos());

    if (pact) {
      QVariant UserData = pact->data();
      QString UserName  = UserData.toString();
      mCurrentValue     = UserName.toStdString();
      meWriter.SetValue(mCurrentValue.c_str());
      SigInvalidateProperty();
  }
  */
  }
}; // class GedTransformWidget

///////////////////////////////////////////////////////////////////////////////

GedItemNode*
GedFactoryTransform::CreateItemNode(ObjModel& mdl, const ConstString& Name, const reflect::IObjectProperty* prop, Object* obj)
    const {

  /*ConstString anno_mkgroup = prop->GetAnnotation("editor.mktag");

  GedEnumWidget* PropContainerW = new GedEnumWidget(mdl, Name.c_str(), prop, obj);
  if (anno_mkgroup.length()) {
    if (anno_mkgroup == "true") {
      // std::string grpname = CreateFormattedString( "%s:%s", name, valstring.c_str() );
      GedItemNode* parent = mdl.GetGedWidget()->ParentItemNode();
      PropTypeString valstring;
      // prop->GetValueString( obj, valstring );
      parent->mTags[Name.c_str()] = valstring.c_str();
    }
  }
  return PropContainerW;*/
  OrkAssert(false);
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void GedFactoryTransform::Describe() {
}

///////////////////////////////////////////////////////////////////////////////
}}} // namespace ork::tool::ged
///////////////////////////////////////////////////////////////////////////////
INSTANTIATE_TRANSPARENT_RTTI(ork::tool::ged::GedFactoryTransform, "ged.factory.xform");
