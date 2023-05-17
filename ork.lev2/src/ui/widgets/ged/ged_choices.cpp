////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/kernel/core_interface.h>
#include <ork/util/choiceman.h>

////////////////////////////////////////////////////////////////
namespace ork::lev2::ged {
////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

class TextureChoices : public ork::util::ChoiceList {
public:
  void EnumerateChoices(bool bforcenocache=false) final;
  TextureChoices();
};

///////////////////////////////////////////////////////////////////////////

TextureChoices::TextureChoices() {
  EnumerateChoices();
}

void TextureChoices::EnumerateChoices(bool bforcenocache) {
  clear();

  //auto items = lev2::TextureAsset::GetClassStatic()->EnumerateExisting();
  //for (const auto& i : items)
    //add(util::AttrChoiceValue(i.c_str(), i.c_str()));
}

///////////////////////////////////////////////////////////////////////////

static util::choicemanager_ptr_t _create_choicemanager(){
    auto chcman = std::make_shared<util::ChoiceManager>();

    auto tex_choices = chcman->createChoicelist<TextureChoices>("lev2tex");

    return chcman;
}

///////////////////////////////////////////////////////////////////////////

util::choicemanager_ptr_t choicemanager(){
    static auto chcman = _create_choicemanager();
    return chcman;
}

////////////////////////////////////////////////////////////////
} //namespace ork::lev2::ged {
////////////////////////////////////////////////////////////////
