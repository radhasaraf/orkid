////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

///////////////////////////////////////////////////////////////////////////////

#include <ork/kernel/Array.h>
#include <ork/kernel/orkvector.h>
#include <ork/kernel/prop_basic.h>
#include <ork/kernel/prop.h>
#include <ork/kernel/prop.hpp>
#include <ork/kernel/prop_container.h>
#include <ork/entity/entity.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

template<>
void Array<Entity*>::ClassInit()
{
	GetClassStatic();

	AddProperty(new CObjectVectorContainerProp("Array",
												CProp::EFLAG_NONE, 
												PROP_OFFSET(Array<Entity*>, mArray), Entity::GetClassNameStatic()));

	AnnotateProperty("Array", "Container.BaseClass", Entity::GetClassNameStatic());
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork
///////////////////////////////////////////////////////////////////////////////
