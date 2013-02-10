////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef ORK_ENT_DATAFLOW_H
#define ORK_ENT_DATAFLOW_H

///////////////////////////////////////////////////////////////////////////////

#include <pkg/ent/entity.h>
#include <ork/dataflow/dataflow.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace ent {

class DataflowRecieverComponentData : public ComponentData
{
	RttiDeclareConcrete(DataflowRecieverComponentData, ComponentData)

public:

	DataflowRecieverComponentData();

	virtual ComponentInst *CreateComponent(Entity *pent) const;

	ork::orklut<ork::PoolString,float>& GetFloatValues() { return mFloatValues; }
	const ork::orklut<ork::PoolString,float>& GetFloatValues() const { return mFloatValues; }
	ork::orklut<ork::PoolString,CVector3>& GetVect3Values() { return mVect3Values; }
	const ork::orklut<ork::PoolString,CVector3>& GetVect3Values() const { return mVect3Values; }

private:

	ork::orklut<ork::PoolString,float>		mFloatValues;
	ork::orklut<ork::PoolString,CVector3>	mVect3Values;

	bool Notify(const event::Event *event); // virtual

	const char* GetShortSelector() const { return "dfr"; } // virtual

};

///////////////////////////////////////////////////////////////////////////////

class DataflowRecieverComponentInst : public ComponentInst
{
	RttiDeclareAbstract(DataflowRecieverComponentInst, ComponentInst)

public:

	DataflowRecieverComponentInst(const DataflowRecieverComponentData &data, Entity *pent);

	//////////////////////////////////////////////////////////////
	// call BindExternalValue in external components dolink
	//  for binding to data in the external component
	//////////////////////////////////////////////////////////////

	void BindExternalValue( PoolString name, const float* psource );
	void BindExternalValue( PoolString name, const CVector3* psource );

	dataflow::dyn_external&					RefExternal() { return mExternal; }
	const dataflow::dyn_external&			RefExternal() const { return mExternal; }

	bool Notify(const event::Event *event); // virtual

private:

	const DataflowRecieverComponentData&	mData;
	dataflow::dyn_external					mExternal;
	orklut<PoolString,float>				mMutableFloatValues;
	orklut<PoolString,CVector3>				mMutableVect3Values;

	void DoUpdate( ork::ent::SceneInst* psi );

};

///////////////////////////////////////////////////////////////////////////////

}} // namespace ork { namespace ent {

#endif
