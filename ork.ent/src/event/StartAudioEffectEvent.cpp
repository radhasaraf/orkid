////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <pkg/ent/AudioComponent.h>
#include <pkg/ent/event/StartAudioEffectEvent.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/application/application.h>
///////////////////////////////////////////////////////////////////////////////
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::event::PlaySoundEvent, "PlaySoundEvent");
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent { namespace event {
///////////////////////////////////////////////////////////////////////////////
void PlaySoundEvent::Describe()
{
	ork::reflect::RegisterProperty("SoundName", &PlaySoundEvent::mSoundName);
	ork::reflect::AnnotatePropertyForEditor<PlaySoundEvent>( "SoundName",	"ged.userchoice.delegate", "AudioEventChoiceDelegate" );

}
///////////////////////////////////////////////////////////////////////////////
PlaySoundEvent::PlaySoundEvent(ork::PieceString name)
	: mSoundName(ork::AddPooledString(name))
{
}
///////////////////////////////////////////////////////////////////////////////
} } } // namespace ork::ent::event
///////////////////////////////////////////////////////////////////////////////
