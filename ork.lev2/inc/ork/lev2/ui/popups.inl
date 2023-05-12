#pragma once

#include <ork/lev2/ui/context.h>
#include <ork/lev2/ui/box.h>
#include <ork/lev2/ui/lineedit.h>
#include <ork/lev2/ui/choicelist.h>
#include <ork/lev2/ui/layoutgroup.inl>
#include <ork/kernel/core_interface.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/glfw/ctx_glfw.h>

namespace ork::ui {

    inline std::string popupLineEdit(lev2::Context* ctx,
                              int x, int y, int w, int h, //
                              const std::string& initial_value){

        lev2::PopupWindow popwin(ctx, x,y,w,h);
        auto uic = popwin._uicontext;
        auto root = uic->makeTop<ui::LayoutGroup>("lg",0,0,w,h);
        auto lineedit_item = root->makeChild<ui::LineEdit>("LineEdit",fvec4(1,1,0,1),0,0,0,0);
        auto lineedit_layout = lineedit_item._layout;
        auto lineedit = std::dynamic_pointer_cast<ui::LineEdit>(lineedit_item._widget);
        //auto lineedit_item = root->makeChild<ui::Box>("HI",fvec4(1,1,1,1),0,0,0,0);
        auto root_layout = root->_layout;
        lineedit_layout->top()->anchorTo(root_layout->top());
        lineedit_layout->left()->anchorTo(root_layout->left());
        lineedit_layout->right()->anchorTo(root_layout->right());
        lineedit_layout->bottom()->anchorTo(root_layout->bottom());
        root_layout->updateAll();
        lineedit->setValue(initial_value);
        popwin.mainThreadLoop();        
        return lineedit->_value;
    }
    inline std::string popupChoiceList(lev2::Context* ctx,
                                int x, int y, //
                                const std::vector<std::string>& choices){

        fvec2 dimensions = ui::ChoiceList::computeDimensions(choices);
        int w = int(dimensions.x);
        int h = int(dimensions.y);
        lev2::PopupWindow popwin(ctx, x,y,w,h);
        auto uic = popwin._uicontext;
        auto root = uic->makeTop<ui::LayoutGroup>("lg",0,0,w,h);
        auto choicelist_item = root->makeChild<ui::ChoiceList>("ChoiceList",fvec4(1,1,0,1),0,0,0,0);
        auto choicelist_layout = choicelist_item._layout;
        auto choicelist = std::dynamic_pointer_cast<ui::ChoiceList>(choicelist_item._widget);
        choicelist->_choices = choices;
        //auto choicelist_item = root->makeChild<ui::Box>("HI",fvec4(1,1,1,1),0,0,0,0);
        auto root_layout = root->_layout;
        choicelist_layout->top()->anchorTo(root_layout->top());
        choicelist_layout->left()->anchorTo(root_layout->left());
        choicelist_layout->right()->anchorTo(root_layout->right());
        choicelist_layout->bottom()->anchorTo(root_layout->bottom());
        root_layout->updateAll();
        //choicelist->setValue(initial_value);
        popwin.mainThreadLoop();        
        return choicelist->_value;
    }
}