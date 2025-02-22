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
namespace ork::lev2::shadlang {
/////////////////////////////////////////////////////////////////////////////////////////////////
static logchannel_ptr_t logchan         = logger()->createChannel("ORKSLIMPL", fvec3(1, 1, .9), false);
static logchannel_ptr_t logchan_grammar = logger()->createChannel("ORKSLGRAM", fvec3(1, 1, .8), false);
static logchannel_ptr_t logchan_lexer   = logger()->createChannel("ORKSLLEXR", fvec3(1, 1, .7), false);
/////////////////////////////////////////////////////////////////////////////////////////////////

void SHAST::_dumpAstTreeVisitor( //
    SHAST::astnode_ptr_t node,   //
    int indent,                  //
    std::string& out_str) {      //

  auto indentstr = std::string(indent * 2, ' ');
  out_str += FormatString("%s%s\n", indentstr.c_str(), node->desc().c_str());
  if (node->_descend) {
    for (auto c : node->_children) {
      _dumpAstTreeVisitor(c, indent + 1, out_str);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

SHAST::translationunit_ptr_t parseFromString(slpcache_ptr_t slpcache, //
                                             const std::string& shader_text) { //
  return parseFromString(slpcache,"---",shader_text);
}

///////////////////////////////////////////////////////////////////////////////

SHAST::translationunit_ptr_t parseFromString(slpcache_ptr_t slpcache, //
                                             const std::string& name, //
                                             const std::string& shader_text) { //
  auto parser = std::make_shared<impl::ShadLangParser>(slpcache);
  slpcache->_impl_stack.push_back(parser);
  parser->_name = name;
  OrkAssert(shader_text.length());
  auto rval = parser->parseString(name, shader_text);
  slpcache->_impl_stack.pop_back();
  return rval;
}

///////////////////////////////////////////////////////////////////////////////

SHAST::translationunit_ptr_t parseFromFile(slpcache_ptr_t slpcache, //
                                           file::Path shader_path) { //
  auto shader_data = File::readAsString(shader_path);
  if(shader_data == nullptr){
    printf( "ShaderFile not found<%s>\n", shader_path.c_str() );
    OrkAssert(false);
    return nullptr;
  }
  auto parser = std::make_shared<impl::ShadLangParser>(slpcache);
  slpcache->_impl_stack.push_back(parser);
  OrkAssert(shader_data->_data.length());
  parser->_shader_path = shader_path;
  auto rval = parser->parseString(shader_path.c_str(), shader_data->_data);
  slpcache->_impl_stack.pop_back();
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
namespace impl {
///////////////////////////////////////////////////////////////////////////////

struct Private{

  Private(){
    auto grammars_dir = ork::file::Path::data_dir() / "grammars"; 
    auto scanner_path = grammars_dir / "shadlang.scanner";
    auto parser_path = grammars_dir / "shadlang.parser";
    auto scanner_read_result  = ork::File::readAsString(scanner_path);
    auto parser_read_result = ork::File::readAsString(parser_path);
    _scanner_spec      = scanner_read_result->_data;
    _parser_spec      = parser_read_result->_data;
  }

  std::string _scanner_spec;
  std::string _parser_spec;

};

using private_ptr_t = std::shared_ptr<const Private>;


///////////////////////////////////////////////////////////////////////////////

void implStackDump(slpcache_ptr_t cache){
  size_t index = 0;
  for( auto item : cache->_impl_stack ){
    auto impl = item.get<std::shared_ptr<ShadLangParser>>();
    printf( "stack<%zu> impl<%p:%s>\n", index, (void*) impl.get(), impl->_name.c_str() );
    index++;
  }
}

///////////////////////////////////////////////////////////////////////////////

ShadLangParser::ShadLangParser(slpcache_ptr_t cache) 
  : _slp_cache(cache) {
  _name = "shadlang";
  _DEBUG_MATCH       = false;
  _DEBUG_INFO        = false;

  static private_ptr_t _private = std::make_shared<Private>();

  ///////////////////////////////////////////////////////////

  preDeclareAstNodes();

  ///////////////////////////////////////////////////////////

  bool OK = this->loadPEGSpec(_private->_scanner_spec,_private->_parser_spec);
  OrkAssert(OK);

  ///////////////////////////////////////////////////////////
  // parser should be compiled and linked at this point
  //  lets declare the AST node types
  //  and define any custom AST node handlers
  ///////////////////////////////////////////////////////////

  declareAstNodes();
  defineAstHandlers();

  ///////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

SHAST::astnode_ptr_t ShadLangParser::astNodeForMatch(match_ptr_t match) const {
  auto it = _match2astnode.find(match);
  if(it == _match2astnode.end()){
    match->dump1(0);
    printf( "Cannot find AST for match<%p> matcher<%s>\n", (void*) match.get(), match->_matcher->_name.c_str() ); 
    //OrkAssert(false);
    if( match->_children.size() == 1 ){
      return astNodeForMatch(match->_children[0]);
    }
    return nullptr;
  }
  return it->second;
}
match_ptr_t ShadLangParser::matchForAstNode(SHAST::astnode_ptr_t astnode) const {
  auto it = _astnode2match.find(astnode);
  OrkAssert(it != _astnode2match.end());
  return it->second;
}

///////////////////////////////////////////////////////////////////////////////

void ShadLangParser::_buildAstTreeVisitor(match_ptr_t the_match) {
  bool has_ast = the_match->_uservars.hasKey("astnode");
  if (has_ast) {
    auto ast = the_match->_uservars.typedValueForKey<SHAST::astnode_ptr_t>("astnode").value();

    if (_astnodestack.size() > 0) {
      auto parent = _astnodestack.back();
      parent->appendChild(ast);
      ast->_parent = parent;
    }
    _astnodestack.push_back(ast);
  }

  for (auto c : the_match->_children) {
    _buildAstTreeVisitor(c);
  }
  if (has_ast) {
    _astnodestack.pop_back();
  }
}

///////////////////////////////////////////////////////////////////////////////

bool ShadLangParser::walkUpAST( //
    SHAST::astnode_ptr_t node,  //
    SHAST::walk_visitor_fn_t visitor) {
  bool up = visitor(node);
  if (up and node->_parent) {
    bool cont = walkUpAST(node->_parent, visitor);
    if( not cont ){
      return false;
    }
  }
  return up;
}

///////////////////////////////////////////////////////////////////////////////

void ShadLangParser::removeFromParent(SHAST::astnode_ptr_t node){
    auto it1 = _astnode2match.find(node);
  OrkAssert(it1!=_astnode2match.end());
  auto match = it1->second;
  _astnode2match.erase(it1);
  auto it2 = _match2astnode.find(match);
  OrkAssert(it2!=_match2astnode.end());
  _match2astnode.erase(it2);

  SHAST::AstNode::treeops::removeFromParent(node);
}

///////////////////////////////////////////////////////////////////////////////

void ShadLangParser::replaceInParent(SHAST::astnode_ptr_t oldnode, SHAST::astnode_ptr_t newnode){
  auto it1 = _astnode2match.find(oldnode);
  OrkAssert(it1!=_astnode2match.end());
  auto match = it1->second;
  _astnode2match[ newnode ] = match;
  _match2astnode[ match ] = newnode;
  SHAST::AstNode::treeops::replaceInParent(oldnode,newnode);
}

///////////////////////////////////////////////////////////////////////////////

SHAST::translationunit_ptr_t ShadLangParser::parseString(std::string name, std::string parse_str) {

  _name = name;

  _scanner->scanString(parse_str);
  _scanner->discardTokensOfClass(uint64_t(TokenClass::WHITESPACE));
  _scanner->discardTokensOfClass(uint64_t(TokenClass::SINGLE_LINE_COMMENT));
  _scanner->discardTokensOfClass(uint64_t(TokenClass::MULTI_LINE_COMMENT));
  _scanner->discardTokensOfClass(uint64_t(TokenClass::NEWLINE));

  auto top_view = _scanner->createTopView();
  // top_view.dump("top_view");
  auto slv    = std::make_shared<ScannerLightView>(top_view);
  _tu_matcher = findMatcherByName("TranslationUnit");
  OrkAssert(_tu_matcher);
  auto match = this->match(_tu_matcher, slv, [this](match_ptr_t m) { _buildAstTreeVisitor(m); });
  OrkAssert(match);
  auto ast_top = match->_uservars.typedValueForKey<SHAST::astnode_ptr_t>("astnode").value();
  ///////////////////////////////////////////
  pruneAST(ast_top);
  semaAST(ast_top);
  auto top_as_tunit = std::dynamic_pointer_cast<SHAST::TranslationUnit>(ast_top);
  ///////////////////////////////////////////
  if(0){
    printf("///////////////////////////////\n");
    printf("// AST TREE parser<%s>\n", _name.c_str() );
    printf("///////////////////////////////\n");
    std::string ast_str = SHAST::toASTstring(ast_top);
    printf("%s\n", ast_str.c_str());
    printf("///////////////////////////////\n");
  }
  if(1){
    printf("///////////////////////////////\n");
    printf("// TU<%p> LIST parser<%s>\n", (void*) top_as_tunit.get(), _name.c_str() );
    printf("///////////////////////////////\n");
    auto sorted_translatables = sorted_vector_from_map( _slp_cache->_translatables );
    for( auto item : sorted_translatables ){
      auto name = item.first;
      auto node = item.second;
      auto raw_name = node->template typedValueForKey<std::string>("raw_name").value();
      printf( "  name<%s> raw_name<%s>\n", name.c_str(), raw_name.c_str() );

    }
    printf("///////////////////////////////\n");
  }
  return top_as_tunit;
}

} // namespace impl

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::shadlang
/////////////////////////////////////////////////////////////////////////////////////////////////

#endif
