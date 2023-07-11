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
#include <ork/util/logger.h>
#include <ork/kernel/string/deco.inl>

/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork {
//////////////////////////////////////////////////////////////////////
static logchannel_ptr_t logchan_parser = logger()->createChannel("RULESPEC", fvec3(0.5, 0.7, 0.5), true);

static std::atomic<int> g_matcher_id(0);
void Match::dump(int indent) const {

  auto indentstr = std::string(indent, ' ');

  if (_view->empty()) {
    logchan_parser->log(
        "%s DUMP Match<%p> matcher<%p:%s> view (empty)", indentstr.c_str(), this, (void*)_matcher.get(), _matcher->_name.c_str());
  } else {
    logchan_parser->log(
        "%s DUMP Match<%p> matcher<%p:%s> view [%zu..%zu]",
        indentstr.c_str(),
        this,
        (void*)_matcher.get(),
        _matcher->_name.c_str(),
        _view->_start,
        _view->_end);
  }

  if (auto as_seq = tryAs<sequence_ptr_t>()) {
    auto seq = as_seq.value();
    logchan_parser->log("%s   SEQ<%p>", indentstr.c_str(), (void*)seq.get());
    for (auto i : seq->_items) {
      i->dump(indent + 3);
    }
  } else if (auto as_nom = tryAs<n_or_more_ptr_t>()) {
    auto nom = as_nom.value();
    logchan_parser->log("%s   NOM%zu<%p>", indentstr.c_str(), nom->_minmatches, (void*)nom.get());
    for (auto i : nom->_items) {
      i->dump(indent + 3);
    }
  } else if (auto as_grp = tryAs<group_ptr_t>()) {
    auto grp = as_grp.value();
    logchan_parser->log("%s   GRP<%p>", indentstr.c_str(), (void*)grp.get());
    for (auto i : grp->_items) {
      i->dump(indent + 3);
    }
  } else if (auto as_opt = tryAs<optional_ptr_t>()) {
    auto opt = as_opt.value();
    logchan_parser->log("%s   OPT<%p>", indentstr.c_str(), (void*)opt.get());
    if (opt->_subitem)
      opt->_subitem->dump(indent + 3);
    else {
      logchan_parser->log("%s     EMPTY", indentstr.c_str());
    }
  }
}

bool Match::matcherInStack(matcher_ptr_t matcher) const{
  bool rval = false;
  for( auto m : _matcherstack ){
    if( m == matcher ){
      rval = true;
      break;
    }
  }
  return rval;
}

//////////////////////////////////////////////////////////////////////
// packrat support
//////////////////////////////////////////////////////////////////////

uint64_t Matcher::hash(scannerlightview_constptr_t slv) const {
  boost::Crc64 the_crc;
  the_crc.init();
  the_crc.accumulateItem<uint64_t>(slv->hash());
  _hash(the_crc);
  the_crc.finish();
  return the_crc.result();
}

void Matcher::_hash(boost::Crc64& crc_out) const {
  crc_out.accumulateItem<uint64_t>((uint64_t)this);
}

//////////////////////////////////////////////////////////////////////

Matcher::Matcher(matcher_fn_t match_fn)
    : _match_fn(match_fn) {
}

//////////////////////////////////////////////////////////////////////

match_ptr_t filtered_match(matcher_ptr_t matcher, match_ptr_t the_match){
  if( the_match ){
    auto filter = matcher->_match_filter;
    if(filter){
      bool good = filter(the_match);
      if( not good ){
        the_match = nullptr;
      }
    }
  }
  return the_match;
}



//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::optional(matcher_ptr_t sub_matcher, std::string name) {
  if (name == "") {
    name = FormatString("optional-%d", g_matcher_id++);
  }
  auto matcher       = declare(name);
  matcher->_match_fn = [=](matcher_ptr_t par_matcher,                        //
                           scannerlightview_constptr_t slv) -> match_ptr_t { //
    auto the_match      = std::make_shared<Match>();
    the_match->_matcher = par_matcher;
    auto the_opt        = the_match->_impl.makeShared<Optional>();
    ////////////////////////////////////////////////////
    // if sub_matcher matches, return the match
    ////////////////////////////////////////////////////
    if (not slv->empty()) {
      MatchContextItem mci { sub_matcher, slv };
      auto sub_match = _match(mci);
      if (sub_match) {
        auto match_str = deco::string("MATCH", 0, 255, 0);
        log_match("OPT<%s> %s sub<%s>", name.c_str(), match_str.c_str(), sub_match->_matcher->_name.c_str());
        the_match->_view = sub_match->_view;
        the_match->_view->validate();
        the_opt->_subitem = sub_match;
        return the_match;
      }
    }
    auto empty_str = deco::string("EMPTY", 255, 255, 0);
    log_match("OPT<%s> %s", name.c_str(), empty_str.c_str());
    ////////////////////////////////////////////////////
    //  otherwise return "empty" but valid match (start=end=-1)
    ////////////////////////////////////////////////////
    the_match->_view = std::make_shared<ScannerLightView>(*slv);
    the_match->_view->clear();
    return filtered_match(par_matcher,the_match);
  };
  matcher->_on_link = [=]() -> bool {
    if (matcher->_notif == nullptr) {
      matcher->_notif = [=](match_ptr_t the_match) { //
        auto match_str = deco::format(255, 0, 255, "MATCHED OPT<%s>", name.c_str());
        log_match("%s", match_str.c_str());
      };
    }
    return true;
  }; // _on_link
  return matcher;
}

//////////////////////////////////////////////////////////////////////

void Parser::_sequence(matcher_ptr_t matcher, std::vector<matcher_ptr_t> sub_matchers) {
  matcher->_match_fn = [=](matcher_ptr_t par_matcher, scannerlightview_constptr_t slv) -> match_ptr_t {
    match_ptr_t the_match = std::make_shared<Match>();
    the_match->_matcher   = par_matcher;
    log_match("SEQ<%s>: beg_match len<%zu>", matcher->_name.c_str(), sub_matchers.size());
    auto slv_iter     = std::make_shared<ScannerLightView>(*slv);
    auto slv_match    = std::make_shared<ScannerLightView>(*slv);
    auto the_sequence = the_match->_impl.makeShared<Sequence>();
    the_match->_view  = slv_match;
    size_t iter       = 0;
    size_t num_iter   = sub_matchers.size();
    for (auto sub_matcher : sub_matchers) {
      if (0)
        log_match_begin(
            "SEQSS<%s> : match_item<%s> iter<%zu/%zu> itrst<%d> itren<%d> slvst<%d> slvend<%d> ",
            matcher->_name.c_str(),
            sub_matcher->_name.c_str(),
            iter,
            num_iter,
            slv_iter->_start,
            slv_iter->_end,
            slv->_start,
            slv->_end);
      if (slv_iter->_start > slv_iter->_end) {
        break;
      }
      auto tok0       = slv_iter->token(0);
      MatchContextItem mci { sub_matcher, slv_iter };
      auto match_item = _match(mci);
      log_match_begin(
          "SEQ<%s> : match_item<%s> tok0<%s> iter<%zu/%zu> st<%d> end<%d> ",
          matcher->_name.c_str(),
          sub_matcher->_name.c_str(),
          tok0->text.c_str(),
          iter,
          num_iter,
          slv_iter->_start,
          slv_iter->_end);
      if (match_item) {
        size_t item_index = the_sequence->_items.size();
        if (match_item->_view->empty()) {
          auto empty_str = deco::string("MATCH: EMPTY", 0, 255, 255);
          log_match_continue("%s\n", empty_str.c_str());
        } else {
          if (0)
            slv_iter->dump("slv_iter_pre");
          auto match_str = deco::string("MATCH", 0, 255, 0);
          log_match_continue("%s\n", match_str.c_str());
          slv_match->_end  = match_item->_view->_end;
          slv_iter->_start = match_item->_view->_end + 1;
          if (0)
            slv_iter->dump("slv_iter_post");
          // slv_iter->validate();
          // slv_match->validate();
        }
        the_sequence->_items.push_back(match_item);
        OrkAssert(match_item->_view);
      } else {
        auto match_str = deco::string("NO-MATCH", 255, 0, 0);
        log_match_continue("%s\n", match_str.c_str());
        the_match = nullptr;
        break;
      }
      iter++;
    }
    if (the_match) {
      /////////////////////////////////////////////////
      // prune
      /////////////////////////////////////////////////
      int num_matches = the_sequence->_items.size();
      int num_emptys  = 0;
      for (auto match : the_sequence->_items) {
        if (match->_view->empty()) {
          num_emptys++;
        }
      }
      if (num_emptys == num_matches) {
        the_match->_view->clear();
      } else {
        auto slv_out = std::make_shared<ScannerLightView>(*slv);
        for (auto match : the_sequence->_items) {
          if (not match->_view->empty()) {
            slv_out->_end = match->_view->_end;
          }
        }
        the_match->_view = slv_out;
      }
      /////////////////////////////////////////////////
      if (0)
        log_match("SEQ<%s> end the_match<%p> st<%zu> en<%zu> count<%zu>", //
            matcher->_name.c_str(),                                 //
            (void*)the_match.get(),                                 //
            the_match->_view->_start,                               //
            the_match->_view->_end,                                 //
            the_sequence->_items.size());
    } else {
      if (0)
        log_match("SEQ<%s> end NO_MATCH", matcher->_name.c_str());
    }
    return filtered_match(par_matcher,the_match);
  };
  matcher->_on_link = [=]() -> bool {
    if (matcher->_notif == nullptr) {
      matcher->_notif = [=](match_ptr_t the_match) { //
        auto match_str = deco::format(255, 0, 255, "MATCHED SEQ<%s>", matcher->_name.c_str());
        log_match("%s", match_str.c_str());
      };
    }
    return true;
  };
}

//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::sequence(std::vector<matcher_ptr_t> sub_matchers, std::string name) {
  if (name == "") {
    name = FormatString("sequence-%d", g_matcher_id++);
  }
  auto matcher = declare(name);
  _sequence(matcher, sub_matchers);
  return matcher;
}

//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::sequence(std::string name, std::vector<matcher_ptr_t> matchers) {
  return sequence(matchers, name);
}

//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::group(std::vector<matcher_ptr_t> matchers, std::string name) {
  if (name == "") {
    name = FormatString("group-%d", g_matcher_id++);
  }
  auto matcher       = declare(name);
  matcher->_match_fn = [=](matcher_ptr_t par_matcher, scannerlightview_constptr_t slv_inp) -> match_ptr_t {
    match_ptr_t the_match = std::make_shared<Match>();
    the_match->_matcher   = par_matcher;
    auto slv_iter         = std::make_shared<ScannerLightView>(*slv_inp);
    auto the_group        = the_match->_impl.makeShared<Group>();
    for (auto sub_matcher : matchers) {
      MatchContextItem mci { sub_matcher, slv_iter };
      auto sub_match = _match(mci);
      if (sub_match) {
        the_group->_items.push_back(sub_match);
        slv_iter->_start = sub_match->_view->_end + 1;
      } else {
        return nullptr;
      }
    }
    OrkAssert(the_group->_items.size());
    auto slv_out    = std::make_shared<ScannerLightView>(*slv_inp);
    slv_out->_start = the_group->_items.front()->_view->_start;
    slv_out->_end   = the_group->_items.back()->_view->_end;
    slv_out->validate();
    the_match->_view = slv_out;
    return filtered_match(par_matcher,the_match);
  };
  matcher->_on_link = [=]() -> bool { return true; };
  return matcher;
}

//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::oneOf(std::vector<matcher_ptr_t> matchers, std::string name) {
  if (name == "") {
    name = FormatString("oneof-%d", g_matcher_id++);
  }
  auto matcher       = declare(name);
  matcher->_match_fn = [=](matcher_ptr_t par_matcher, scannerlightview_constptr_t slv) -> match_ptr_t {
    // log_match( "oneOf<%s>: begin num_subs<%zu>\n", name.c_str(), matchers.size() );
    for (auto sub_matcher : matchers) {
      MatchContextItem mci { sub_matcher, slv };
      auto sub_match = _match(mci);
      if (sub_match) {
        auto match_str = deco::string("MATCH", 0, 255, 0);
        log_match("1OF<%s>: %s sub_matcher<%s>", name.c_str(), match_str.c_str(), sub_matcher->_name.c_str());

        auto the_match      = std::make_shared<Match>();
        the_match->_matcher = par_matcher;
        the_match->_view    = sub_match->_view;
        auto the_oo         = the_match->_impl.makeShared<OneOf>();
        the_oo->_selected   = sub_match;
        return filtered_match(par_matcher,the_match);
      }
    }
    auto match_str = deco::string("NO-MATCH", 255, 0, 0);
    log_match("1OF<%s>: %s", name.c_str(), match_str.c_str());
    return nullptr;
  };
  return matcher;
}

//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::oneOf(std::string name, std::vector<matcher_ptr_t> matchers) {
  return oneOf(matchers, name);
}
//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::nOrMore(matcher_ptr_t sub_matcher, size_t minMatches, std::string name, bool mustConsumeAll) {
  if (name == "") {
    name = FormatString("(%d)OrMore-%d", minMatches, g_matcher_id++);
  }
  auto matcher       = declare(name);
  matcher->_match_fn = [=](matcher_ptr_t par_matcher, scannerlightview_constptr_t input_slv) -> match_ptr_t {
    match_ptr_t the_match    = std::make_shared<Match>();
    the_match->_matcher      = par_matcher;
    auto the_nom             = the_match->_impl.makeShared<NOrMore>();
    the_nom->_mustConsumeAll = mustConsumeAll;
    the_nom->_minmatches     = minMatches;
    bool keep_going          = true;
    auto slv_iter            = std::make_shared<ScannerLightView>(*input_slv);
    int item_index           = 0;
    log_match("NOM%zu<%s>: beg_match sub_matcher<%s>", minMatches, name.c_str(), sub_matcher->_name.c_str());
    ////////////////////////////////////////////////////////////////
    while (keep_going) {
      MatchContextItem mci { sub_matcher, slv_iter };
      auto sub_match = _match(mci);
      keep_going     = false;
      log_match_begin(
          "NOM%d<%s> try_match iter<%d> ", //
          int(minMatches),
          name.c_str(),
          item_index);
      if (sub_match) {
        the_nom->_items.push_back(sub_match);
        item_index++;
        slv_iter->_start = sub_match->_view->_end + 1;
        keep_going       = slv_iter->_start <= slv_iter->_end;
        if (sub_match->_view->empty()) {
          keep_going     = false;
          auto empty_str = deco::string("EMPTY", 0, 255, 255);
          log_match_continue("%s\n", empty_str.c_str());
        } else {
          auto match_str = deco::string("MATCH", 0, 255, 0);
          log_match_continue("%s start<%zu> end<%zu>\n", match_str.c_str(), sub_match->_view->_start, sub_match->_view->_end);
        }
      } else {
        auto nomatch_str = deco::string("NO-MATCH", 255, 0, 0);
        log_match_continue("%s\n", nomatch_str.c_str());
      }
    }
    ////////////////////////////////////////////////////////////////
    if (the_nom->_items.size() >= minMatches) {
      auto slv_out = std::make_shared<ScannerLightView>(*input_slv);
      if (minMatches == 0 and the_nom->_items.size() == 0) {
        slv_out->clear();
      } else {
        slv_out->_start = the_nom->_items.front()->_view->_start;
        slv_out->_end   = the_nom->_items.back()->_view->_end;
        slv_out->validate();
      }
      the_match->_view = slv_out;
      if ((slv_out->_end < input_slv->_end) and the_nom->_mustConsumeAll) {
        auto no_match_str = deco::string("NO-MATCH (not all tokens consumed)", 255, 0, 0);
        log_match("NOM%zu<%s>: %s count<%zu>", minMatches, name.c_str(), no_match_str.c_str(), the_nom->_items.size());
        return nullptr;
      } else {
        auto match_str = deco::string("MATCH", 0, 255, 0);
        log_match("NOM%zu<%s>: %s count<%zu> st<%zu> en<%zu> slen<%d>",
            minMatches,
            name.c_str(),
            match_str.c_str(),
            the_nom->_items.size(),
            slv_out->_start,
            slv_out->_end,
            input_slv->_end);
      }
      return filtered_match(par_matcher,the_match);
    } else if (minMatches == 0) {
      auto slv_out = std::make_shared<ScannerLightView>(*input_slv);
      slv_out->clear();
      the_match->_view  = slv_out;
      auto no_match_str = deco::string("NO-MATCH", 255, 0, 0);
      log_match("NOM%zu<%s>: %s count<0>", minMatches, name.c_str(), no_match_str.c_str());
      return filtered_match(par_matcher,the_match);
    }
    OrkAssert(false); // should never get here?
    // log_match( "NOM%zu<%s>: end_match (NOMATCH)", minMatches, name.c_str() );
    return nullptr;
  };
  matcher->_on_link = [=]() -> bool {
    if (matcher->_notif == nullptr) {
      matcher->_notif = [=](match_ptr_t the_match) {
        auto match_str = deco::format(255, 0, 255, "MATCHED NOM<%s>", name.c_str());
        log_match("%s", match_str.c_str());
      };
    }
    return true;
  };
  return matcher;
}

matcher_ptr_t Parser::oneOrMore(matcher_ptr_t matcher, std::string name) {
  return nOrMore(matcher, 1, name);
}
matcher_ptr_t Parser::zeroOrMore(matcher_ptr_t matcher, std::string name, bool mustConsumeAll) {
  return nOrMore(matcher, 0, name, mustConsumeAll);
}

//////////////////////////////////////////////////////////////

matcher_ptr_t Parser::matcherForTokenClassID(uint64_t tokclass, std::string name) {
  auto matcher       = declare(name);
  matcher->_match_fn = [tokclass, this](matcher_ptr_t par_matcher, scannerlightview_constptr_t slv) -> match_ptr_t {
    auto tok0         = slv->token(0);
    auto slv_tokclass = tok0->_class;
    log_match("MATCHER.CLASSID<%zu> matcher_name<%s> tok<%s> tokclass<%zu>", //
        tokclass,
        par_matcher->_name.c_str(),
        tok0->text.c_str(),
        slv_tokclass);
    if (slv_tokclass == tokclass) {
      match_ptr_t the_match     = std::make_shared<Match>();
      auto the_classmatch       = the_match->_impl.makeShared<ClassMatch>();
      the_match->_matcher       = par_matcher;
      the_classmatch->_tokclass = tokclass;
      the_classmatch->_token    = tok0;
      auto slv_out              = std::make_shared<ScannerLightView>(*slv);
      slv_out->_end             = slv_out->_start;
      the_match->_view          = slv_out;
      slv_out->validate();
      return filtered_match(par_matcher,the_match);
    }
    return nullptr;
  };
  matcher->_on_link = [=]() -> bool {
    if (matcher->_notif == nullptr) {
      matcher->_notif = [=](match_ptr_t the_match) {
        auto tok       = the_match->_view->token(0);
        auto match_str = deco::format(255, 0, 255, "MATCHED tok<%s> mname<%s>", tok->text.c_str(), name.c_str());
        log_match("%s", match_str.c_str());
      };
    }
    return true;
  };
  return matcher;
}

//////////////////////////////////////////////////////////////

matcher_ptr_t Parser::matcherForWord(std::string word, std::string name) {
  if (name == "") {
    name = word;
  }
  auto matcher       = declare(name);
  matcher->_match_fn = [word](matcher_ptr_t par_matcher, scannerlightview_constptr_t inp_view) -> match_ptr_t {
    auto tok0 = inp_view->token(0);
    if (tok0->text == word) {
      auto slv              = std::make_shared<ScannerLightView>(*inp_view);
      slv->_end             = slv->_start;
      auto the_match        = std::make_shared<Match>();
      auto the_wordmatch    = the_match->_impl.makeShared<WordMatch>();
      the_wordmatch->_token = tok0;
      the_match->_matcher   = par_matcher;
      the_match->_view      = slv;
      slv->validate();
      return filtered_match(par_matcher,the_match);
    } else {
      return nullptr;
    }
  };
  matcher->_on_link = [=]() -> bool {
    if (matcher->_notif == nullptr) {
      matcher->_notif = [=](match_ptr_t the_match) { //
        auto match_str = deco::format(255, 0, 255, "MATCHED word<%s>", word.c_str());
        log_match("%s", match_str.c_str());
      };
    }
    return true;
  };
  return matcher;
}

//////////////////////////////////////////////////////////////////////

match_ptr_t Parser::_match(MatchContextItem& mci) {

  auto matcher  = mci._matcher;
  auto inp_view = mci._view;

  OrkAssert(matcher);
  if (matcher->_match_fn == nullptr) {
    logerrchannel()->log("matcher<%s> has no match function", matcher->_name.c_str());
    OrkAssert(false);
  }
  _matchctx._stack.push_back(mci);
  inp_view->validate();
  match_ptr_t match;
  //////////////////////////////////
  // check packrat cache
  //////////////////////////////////
  uint64_t hash = matcher->hash(inp_view);
  auto it       = _packrat_cache.find(hash);
  if (it != _packrat_cache.end()) {
    match = it->second;
    _cache_hits++;
  }
  //////////////////////////////////
  // not in cache, perform match operations, and cache it
  //////////////////////////////////
  else {
    match                = matcher->_match_fn(matcher, inp_view);
    if(match){
      //match->_matcherstack = _matcherstack;
    }
    _packrat_cache[hash] = match;
    _cache_misses++;
  }
  //////////////////////////////////
  // notify if matched
  //////////////////////////////////
  if (match and matcher->_notif) {
    matcher->_notif(match);
  }
  //////////////////////////////////
  _matchctx._stack.pop_back();
  return match;
}

//////////////////////////////////////////////////////////////

match_ptr_t Parser::match(matcher_ptr_t topmatcher, scannerlightview_constptr_t topview) {
  if (topmatcher == nullptr) {
    logerrchannel()->log("Parser<%p> no top match function", this);
    OrkAssert(false);
  }
  _matchctx._stack.clear();
  _matchctx._topmatcher = topmatcher;
  _matchctx._topview    = topview;
  MatchContextItem mci { topmatcher, topview };
  auto match = _match(mci);
  return match;
}

//////////////////////////////////////////////////////////////////////

matcher_ptr_t Parser::declare(std::string name) {
  log_info_begin("DECLARE MATCHER<%s> ", name.c_str());
  auto it = _matchers_by_name.find(name);
  if (it != _matchers_by_name.end()) {
    log_match_continue( "pre-exists<%p>\n", (void*)it->second.get() );
    return it->second;
  }
  ///////////////////////////////////////////////
  auto rval = std::make_shared<Matcher>(nullptr);
  if (name == "") {
    size_t count = _matchers_by_name.size();
    name         = FormatString("anon_%zu", count);
  }
  _matchers.insert(rval);
  rval->_name             = name;
  _matchers_by_name[name] = rval;
    log_match_continue( "new<%p>\n", (void*) rval.get() );
  ///////////////////////////////////////////////
  return rval;
}

//////////////////////////////////////////////////////////////////////

void Parser::link() {
  std::vector<matcher_ptr_t> unlinked;
  for (auto matcher : _matchers) {
    unlinked.push_back(matcher);
  }
  int bad_iters = 0;
  while(unlinked.size()){
    auto it = unlinked.begin();
    unlinked.erase(it);
    auto matcher = *it;
    auto matcher_name = matcher->_name;
    bool OK = true;
    if (matcher->_on_link) {
      OK = matcher->_on_link();
    }
    if( OK ){
      log_info("MATCHER<%s> LINKED...", matcher_name.c_str());
    }
    else{
      log_info("MATCHER<%s> LINK FAILED, will reattempt..", matcher_name.c_str());
      unlinked.push_back(matcher);
      bad_iters++;
    }
    OrkAssert(bad_iters<1000);
  }
}

//////////////////////////////////////////////////////////////////////

void Parser::_log_valist(const char* pMsgFormat, va_list args) const {
  char buf[1024];
  vsnprintf_s(buf, sizeof(buf), pMsgFormat, args);
  size_t indent  = _matchctx._stack.size();
  auto indentstr = std::string(indent * 2, ' ');
  printf("[PARSER : %s] %s%s\n", _name.c_str(), indentstr.c_str(), buf);
}
void Parser::_log_valist_begin(const char* pMsgFormat, va_list args) const {
  char buf[1024];
  vsnprintf_s(buf, sizeof(buf), pMsgFormat, args);
  size_t indent  = _matchctx._stack.size();
  auto indentstr = std::string(indent * 2, ' ');
  printf("[PARSER] %s%s", indentstr.c_str(), buf);
}
void Parser::_log_valist_continue(const char* pMsgFormat, va_list args) const {
  char buf[1024];
  vsnprintf_s(buf, sizeof(buf), pMsgFormat, args);
  printf("%s", buf);
}

//////////////////////////////////////////////////////////////////////

void Parser::log_match(const char* pMsgFormat, ...) const {
  if (_DEBUG_MATCH) {
    va_list args;
    va_start(args, pMsgFormat);
    _log_valist(pMsgFormat, args);
    va_end(args);
  }
}
void Parser::log_match_begin(const char* pMsgFormat, ...) const {
  if (_DEBUG_MATCH) {
    va_list args;
    va_start(args, pMsgFormat);
    _log_valist_begin(pMsgFormat, args);
    va_end(args);
  }
}
void Parser::log_match_continue(const char* pMsgFormat, ...) const {
  if (_DEBUG_MATCH) {
    va_list args;
    va_start(args, pMsgFormat);
    _log_valist_continue(pMsgFormat, args);
    va_end(args);
  }
}

//////////////////////////////////////////////////////////////////////

void Parser::log_info(const char* pMsgFormat, ...) const {
  if (_DEBUG_INFO) {
    va_list args;
    va_start(args, pMsgFormat);
    _log_valist(pMsgFormat, args);
    va_end(args);
  }
}
void Parser::log_info_begin(const char* pMsgFormat, ...) const {
  if (_DEBUG_INFO) {
    va_list args;
    va_start(args, pMsgFormat);
    _log_valist_begin(pMsgFormat, args);
    va_end(args);
  }
}
void Parser::log_info_continue(const char* pMsgFormat, ...) const {
  if (_DEBUG_INFO) {
    va_list args;
    va_start(args, pMsgFormat);
    _log_valist_continue(pMsgFormat, args);
    va_end(args);
  }
}

//////////////////////////////////////////////////////////////////////


Parser::Parser() {
  _name                                    = FormatString("%p", (void*)this);
  static constexpr const char* block_regex = "(function|yo|xxx)";
  _scanner                                 = std::make_shared<Scanner>(block_regex);
}

matcher_ptr_t Parser::findMatcherByName(const std::string& name) const {
  auto it = _matchers_by_name.find(name);
  if (it != _matchers_by_name.end()) {
    return it->second;
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork
/////////////////////////////////////////////////////////////////////////////////////////////////