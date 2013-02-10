////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef	ORK_REFERENCEARCHETYPE_H
#define ORK_REFERENCEARCHETYPE_H

#include <ork/rtti/RTTI.h>

#include <ork/asset/Asset.h>
#include <ork/asset/AssetManager.h>

#include <pkg/ent/entity.h>

namespace ork { namespace ent {

class ArchetypeAsset : public asset::Asset
{
	RttiDeclareConcrete( ArchetypeAsset, asset::Asset );
public:

	ArchetypeAsset();
	/*virtual*/ ~ArchetypeAsset();

	Archetype* GetArchetype() const { return mArchetype; }
	void SetArchetype(Archetype* archetype) { mArchetype = archetype; }

protected:
	Archetype* mArchetype;
};

///////////////////////////////////////////////////////////

class ReferenceArchetype : public Archetype
{
	RttiDeclareConcrete( ReferenceArchetype, Archetype );

public:

	ReferenceArchetype();

	ArchetypeAsset* GetAsset() const { return mArchetypeAsset; }
	void SetAsset(ArchetypeAsset* passet) { mArchetypeAsset = passet; }

private:
	/*virtual*/ void DoCompose(ork::ent::ArchComposer& composer);
	/*virtual*/ void DoComposeEntity( Entity *pent ) const;
	/*virtual*/ void DoLinkEntity(SceneInst* inst, Entity *pent) const;
	/*virtual*/ void DoStartEntity(SceneInst* psi, const CMatrix4 &world, Entity *pent ) const;
	/*virtual*/ void DoStopEntity(SceneInst* psi, Entity *pent) const;
	/*virtual*/ void DoComposePooledEntities(SceneInst *inst);
	/*virtual*/ void DoLinkPooledEntities(SceneInst *inst);

	ArchetypeAsset* mArchetypeAsset;
};

} }

#endif // ORK_REFERENCEARCHETYPE_H
