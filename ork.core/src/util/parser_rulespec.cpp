////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <regex>
#include <stdlib.h>
#include <stdarg.h>
#include <ork/pch.h>
#include <ork/file/file.h>
#include <ork/util/parser.h>
#include <ork/kernel/string/deco.inl>
#include <ork/util/crc.h>
#include <csignal>
/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork {
/////////////////////////////////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::rule(const std::string& rule_name) {
  auto it = _matchers_by_name.find(rule_name);
  matcher_ptr_t rval;
  if (it != _matchers_by_name.end()) {
    rval = it->second;
  }
  return rval;
}

void Parser::on(const std::string& rule_name, matcher_notif_t fn) {
  auto it = _matchers_by_name.find(rule_name);
  if (it != _matchers_by_name.end()) {
    matcher_ptr_t matcher = it->second;
    matcher->_notif       = fn;
  } else {
    OrkAssert(false);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

enum class TokenClass : uint64_t {
  CrcEnum(WHITESPACE),
  CrcEnum(NEWLINE),
  CrcEnum(COLON),
  CrcEnum(SEMICOLON),
  CrcEnum(L_PAREN),
  CrcEnum(R_PAREN),
  CrcEnum(L_CURLY),
  CrcEnum(R_CURLY),
  CrcEnum(L_SQUARE),
  CrcEnum(R_SQUARE),
  CrcEnum(EQUALS),
  CrcEnum(STAR),
  CrcEnum(PLUS),
  CrcEnum(MINUS),
  CrcEnum(COMMA),
  CrcEnum(INTEGER),
  CrcEnum(KW_OR_ID),
  CrcEnum(QUOTED_REGEX),
  CrcEnum(QUOTED_STRING),
  CrcEnum(LEFT_ARROW),
};

/////////////////////////////////////////////////////////////////////////////////////////////////

namespace AST {

struct VariableReference;
struct Expression;
using expression_ptr_t = std::shared_ptr<Expression>;
using varref_ptr_t     = std::shared_ptr<VariableReference>;

struct AstNode {
  virtual ~AstNode() {
  }
};

struct ScannerRule{
  std::string _name;
  std::string _regex;
};
struct ScannerMacro{
  std::string _name;
  std::string _regex;
};

using scanner_rule_ptr_t = std::shared_ptr<ScannerRule>;
using scanner_macro_ptr_t = std::shared_ptr<ScannerMacro>;

} // namespace AST

/////////////////////////////////////////////////////////////////////////////////////////////////



static constexpr const char* block_regex = "(function|yo|xxx)";

struct RuleSpecImpl : public Parser {

  RuleSpecImpl() {
    loadScannerRules();
    loadGrammar();
  }
  /////////////////////////////////////////////////////////
  void loadScannerRules() { //
    try {
      _scanner = std::make_shared<Scanner>(block_regex);
      _scanner->addEnumClass("\\s+", TokenClass::WHITESPACE);
      _scanner->addEnumClass("[\\n\\r]+", TokenClass::NEWLINE);
      _scanner->addEnumClass("[a-zA-Z_][a-zA-Z0-9_]*", TokenClass::KW_OR_ID);
      _scanner->addEnumClass("=", TokenClass::EQUALS);
      _scanner->addEnumClass(",", TokenClass::COMMA);
      _scanner->addEnumClass(":", TokenClass::COLON);
      _scanner->addEnumClass(";", TokenClass::SEMICOLON);
      _scanner->addEnumClass("\\(", TokenClass::L_PAREN);
      _scanner->addEnumClass("\\)", TokenClass::R_PAREN);
      _scanner->addEnumClass("\\[", TokenClass::L_SQUARE);
      _scanner->addEnumClass("\\]", TokenClass::R_SQUARE);
      _scanner->addEnumClass("\\{", TokenClass::L_CURLY);
      _scanner->addEnumClass("\\}", TokenClass::R_CURLY);
      _scanner->addEnumClass("\\*", TokenClass::STAR);
      _scanner->addEnumClass("\\+", TokenClass::PLUS);
      _scanner->addEnumClass("\\-", TokenClass::MINUS);
      _scanner->addEnumClass("-?(\\d+)", TokenClass::INTEGER);
      //_scanner->addMacro("ESCAPED_CHAR", "\\\\[\"\\\\]");
      //_scanner->addMacro("ANY_ESCAPED", "\\\\.");
      //_scanner->addMacro("ASCII", "[\\\\x00-\\\\x21\\\\x23-\\\\x5B\\\\x5D-\\\\x7F]");
      //_scanner->addMacro("ASCII_WITHOUT_DBLQUOTE", "[\\\\x00-\\\\x21\\\\x23-\\\\x7F]");
      _scanner->addEnumClass(R"(\"[^\"]*\")", TokenClass::QUOTED_REGEX);
      _scanner->addEnumClass("<-", TokenClass::LEFT_ARROW);
        printf( "Building state machine\n");
      _scanner->buildStateMachine();
        printf( "done...\n");
    } catch (std::exception& e) {
      printf("EXCEPTION<%s>\n", e.what());
      OrkAssert(false);
    }
  }
  /////////////////////////////////////////////////////////
  void loadGrammar() { //
    printf( "Loading Grammar\n");
    ////////////////////
    // primitives
    ////////////////////
    auto plus         = matcherForTokenClass(TokenClass::PLUS, "plus");
    auto minus        = matcherForTokenClass(TokenClass::MINUS, "minus");
    auto star         = matcherForTokenClass(TokenClass::STAR, "star");
    auto equals       = matcherForTokenClass(TokenClass::EQUALS, "equals");
    auto colon        = matcherForTokenClass(TokenClass::COLON, "colon");
    auto semicolon    = matcherForTokenClass(TokenClass::SEMICOLON, "semicolon");
    auto comma        = matcherForTokenClass(TokenClass::COMMA, "comma");
    auto lparen       = matcherForTokenClass(TokenClass::L_PAREN, "lparen");
    auto rparen       = matcherForTokenClass(TokenClass::R_PAREN, "rparen");
    auto lsquare      = matcherForTokenClass(TokenClass::L_SQUARE, "lsquare");
    auto rsquare      = matcherForTokenClass(TokenClass::R_SQUARE, "rsquare");
    auto lcurly       = matcherForTokenClass(TokenClass::L_CURLY, "lcurly");
    auto rcurly       = matcherForTokenClass(TokenClass::R_CURLY, "rcurly");
    auto inttok       = matcherForTokenClass(TokenClass::INTEGER, "int");
    auto kworid       = matcherForTokenClass(TokenClass::KW_OR_ID, "kw_or_id");
    auto left_arrow   = matcherForTokenClass(TokenClass::LEFT_ARROW, "left_arrow");
    auto quoted_regex = matcherForTokenClass(TokenClass::QUOTED_REGEX, "quoted_regex");
    ////////////////////
    auto oneof = matcherForWord("oneOf");
    auto zom   = matcherForWord("zom");
    auto oom   = matcherForWord("oom");
    auto opt   = matcherForWord("opt");
    auto macro   = matcherForWord("macro");
    ////////////////////
    // scanner rules
    ////////////////////
    auto macro_item = sequence({macro, lparen, kworid, rparen }, "macro_item");
    auto scanner_key    = oneOf({macro_item,kworid}, "scanner_key");
    auto scanner_rule    = sequence({scanner_key, left_arrow, quoted_regex}, "scanner_rule");
    scanner_rule->_notif = [=](match_ptr_t match) {
      auto seq = match->_impl.getShared<Sequence>();
      auto rule_key_item = seq->_items[0]->_impl.getShared<OneOf>()->_selected;
      auto qrx = seq->_items[2]->_impl.getShared<ClassMatch>()->_token->text;
      auto rx = qrx.substr(1, qrx.size() - 2); // remove surrounding quotes
      if( auto as_classmatch = rule_key_item->_impl.tryAsShared<ClassMatch>()){
        auto rule_name = as_classmatch.value()->_token->text;
        auto rule = std::make_shared<AST::ScannerRule>();
        rule->_name = rule_name;
        rule->_regex = rx;
        auto it = _user_scanner_rules.find(rule_name);
        OrkAssert(it==_user_scanner_rules.end());
        printf( "ADDING SCANNER RULE<%s> <- %s\n", rule_name.c_str(), rx.c_str() );
        _user_scanner_rules[rule_name] = rule;
      }
      else if( auto as_seq = rule_key_item->_impl.tryAsShared<Sequence>() ){
        auto sub_seq = as_seq.value();
        auto macro_str = sub_seq->_items[0]->_impl.getShared<WordMatch>()->_token->text;
        OrkAssert(macro_str=="macro");
        auto macro_name = sub_seq->_items[2]->_impl.getShared<ClassMatch>()->_token->text;
        auto macro = std::make_shared<AST::ScannerMacro>();
        macro->_name = macro_name;
        macro->_regex = rx;
        auto it = _user_scanner_macros.find(macro_name);
        OrkAssert(it==_user_scanner_macros.end());
        printf( "ADDING SCANNER MACRO<%s> <- %s\n", macro_name.c_str(), rx.c_str() );
        _user_scanner_macros[macro_name] = macro;
      }
      else{
        OrkAssert(false);
      }
    };
    _rsi_scanner_matcher = zeroOrMore(scanner_rule, "scanner_rules");
    _rsi_scanner_matcher->_notif = [=](match_ptr_t match) {
      std::string _current_rule_name = "";
      try{
        _user_scanner = std::make_shared<Scanner>(block_regex);
        for( auto item : _user_scanner_macros ){
          auto macro = item.second;
          _current_rule_name = macro->_name;
          printf( "IMPLEMENT MACRO <%s : %s>\n", macro->_name.c_str(), macro->_regex.c_str() );
          _user_scanner->addMacro(macro->_name, macro->_regex);
        }
        for( auto item : _user_scanner_rules ){
          auto rule = item.second;
          uint64_t crc_id = CrcString(rule->_name.c_str()).hashed();
          _current_rule_name = rule->_name;
          printf( "IMPLEMENT EnumClass <%s : %zu : %s>\n", rule->_name.c_str(), crc_id, rule->_regex.c_str() );
          _user_scanner->addEnumClass(rule->_regex, crc_id);
        }
        _scanner->buildStateMachine();
      }
      catch(std::exception& e){
        printf( "EXCEPTION cur_rule<%s>  cause<%s>\n", _current_rule_name.c_str(), e.what() );
        OrkAssert(false);
      }
    };
    ////////////////////
    // parser rules
    ////////////////////
    auto rule_expression = declare("rule_expression");
    //
    auto rule_zom      = sequence({zom, lcurly, rule_expression, rcurly}, "rule_zom");
    auto rule_oom      = sequence({oom, lcurly, rule_expression, rcurly}, "rule_oom");
    auto rule_1of      = sequence({oneof, lcurly, rule_expression, rcurly}, "rule_oneof");
    auto rule_opt      = sequence({opt, lcurly, rule_expression, rcurly}, "rule_opt");
    auto rule_sequence = sequence({lsquare, zeroOrMore(rule_expression), rsquare}, "rule_sequence");
    auto rule_grp      = sequence({lparen, zeroOrMore(rule_expression), rparen}, "rule_grp");
    //
    sequence(
        rule_expression,
        {
            oneOf({// load previously declared rule_expression
                   rule_zom,
                   rule_oom,
                   rule_1of,
                   rule_opt,
                   rule_sequence,
                   rule_grp}),
            optional(sequence({colon, quoted_regex}), "expr_name"),
        });
    auto parser_rule    = sequence({kworid, left_arrow, rule_expression}, "parser_rule");
    _rsi_parser_matcher = zeroOrMore(parser_rule, "parser_rules");
  }
  /////////////////////////////////////////////////////////
  match_ptr_t parseScannerSpec(std::string inp_string) {
    try {
      _scanner->clear();
      _scanner->scanString(inp_string);
      _scanner->discardTokensOfClass(uint64_t(TokenClass::WHITESPACE));
      _scanner->discardTokensOfClass(uint64_t(TokenClass::NEWLINE));
    } catch (std::exception& e) {
      printf("EXCEPTION<%s>\n", e.what());
      OrkAssert(false);
    }
    auto top_view = _scanner->createTopView();
    top_view.dump("top_view");
    auto slv   = std::make_shared<ScannerLightView>(top_view);
    auto match = this->match(slv, _rsi_scanner_matcher);
    return match;
  }
  /////////////////////////////////////////////////////////
  match_ptr_t parseParserSpec(std::string inp_string) {
    try {
      _scanner->clear();
      _scanner->scanString(inp_string);
      _scanner->discardTokensOfClass(uint64_t(TokenClass::WHITESPACE));
      _scanner->discardTokensOfClass(uint64_t(TokenClass::NEWLINE));
    } catch (std::exception& e) {
      printf("EXCEPTION<%s>\n", e.what());
      OrkAssert(false);
    }
    auto top_view = _scanner->createTopView();
    top_view.dump("top_view");
    auto slv   = std::make_shared<ScannerLightView>(top_view);
    auto match = this->match(slv, _rsi_parser_matcher);
    return match;
  }
  /////////////////////////////////////////////////////////

  scanner_ptr_t _scanner;
  scanner_ptr_t _user_scanner;
  matcher_ptr_t _rsi_scanner_matcher;
  matcher_ptr_t _rsi_parser_matcher;

  std::map<std::string,AST::scanner_rule_ptr_t> _user_scanner_rules;
  std::map<std::string,AST::scanner_macro_ptr_t> _user_scanner_macros;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

using rulespec_impl_ptr_t = std::shared_ptr<RuleSpecImpl>;

rulespec_impl_ptr_t getRuleSpecImpl() {
  static auto rsi = std::make_shared<RuleSpecImpl>();
  return rsi;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Parser::loadScannerSpec(const std::string& spec) {
  auto rsi   = getRuleSpecImpl();
  auto match = rsi->parseScannerSpec(spec);
  OrkAssert(match);
}
void Parser::loadParserSpec(const std::string& spec) {
  auto rsi   = getRuleSpecImpl();
  auto match = rsi->parseParserSpec(spec);
  OrkAssert(match);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork
/////////////////////////////////////////////////////////////////////////////////////////////////
