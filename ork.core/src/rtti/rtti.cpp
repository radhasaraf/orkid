////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>
#include <ork/rtti/ICastable.h>
#include <ork/orkstd.h>

namespace ork {	namespace rtti {

Class *ICastable::GetClassStatic() { return NULL; }

Class *ForceLink(Class *c)
{
	return c;
}

} }
