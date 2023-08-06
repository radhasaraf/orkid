////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//  Scanner/Parser
//  this replaces CgFx for OpenGL 3.x and OpenGL ES 2.x
////////////////////////////////////////////////////////////////

#include <ork/lev2/gfx/shadlang.h>
#include <ork/file/file.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/pch.h>
#include <ork/util/crc.h>
#include <regex>
#include <stdlib.h>
#include <peglib.h>
#include <ork/util/logger.h>
#include <ork/kernel/string/string.h>
#include <ork/util/parser.inl>
#include "shadlang_impl.h"

#if defined(USE_ORKSL_LANG)

/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::shadlang::SHAST {
/////////////////////////////////////////////////////////////////////////////////////////////////

void AstNode::replaceInParent( astnode_ptr_t oldnode, //
                               astnode_ptr_t newnode) { //

  auto parent = oldnode->_parent;
  if (parent) {
    auto& children = parent->_children;
    auto it        = std::find(children.begin(), children.end(), oldnode);
    if( it!=children.end() ){
      *it              = newnode;
      newnode->_parent = parent;
    }
    else{
      logerrchannel()->log("AstNode::replaceInParent failed to find child to replace");
    }
  }
}

} // namespace ork::lev2::shadlang::SHAST

#endif 