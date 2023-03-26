////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <utpp/UnitTest++.h>
#include <cmath>
#include <limits>
#include <string.h>

#include <ork/util/choiceman.h>

using namespace ork;
using namespace ork::util;

TEST(ChoiceMan1) {

  auto chcman   = std::make_shared<ChoiceManager>();
  auto chclist1 = std::make_shared<ChoiceList>();
  auto chclist2 = std::make_shared<ChoiceList>();
  chcman->AddChoiceList("choicelist1", chclist1);
  chcman->AddChoiceList("choicelist2", chclist2);

  chclist1->add(AttrChoiceValue("/nam/a", "val-a", "a"));
  chclist1->add(AttrChoiceValue("/nam/a/b", "val-b", "b"));
  chclist1->add(AttrChoiceValue("/nam/a/b/c", "val-c", "c"));
  chclist1->add(AttrChoiceValue("/nam/d", "val-d", "d"));

  chclist1->dump();

  CHECK_EQUAL(chclist1->FindFromLongName("/nam/a")->shortname(), "a");
  CHECK_EQUAL(chclist1->FindFromLongName("/nam/a/b")->shortname(), "b");
  CHECK_EQUAL(chclist1->FindFromLongName("/nam/a/b/c")->shortname(), "c");
  CHECK_EQUAL(chclist1->FindFromLongName("/nam/d")->shortname(), "d");
}
