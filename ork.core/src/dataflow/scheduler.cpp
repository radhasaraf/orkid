////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>

#include <ork/application/application.h>
#include <ork/dataflow/all.h>
#include <ork/reflect/properties/register.h>
#include <ork/kernel/orklut.hpp>
#include <ork/kernel/string/string.h>
#include <ork/kernel/opq.h>


///////////////////////////////////////////////////////////////////////////////
static const int kINFOPER = 1;
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace dataflow {
///////////////////////////////////////////////////////////////////////////////

const Affinity scheduler::CpuAffinity = 0xffff;
const Affinity scheduler::GpuAffinity = 0x10000;

workunit::workunit( dgmoduleinst_ptr_t pmod, cluster* pclus, int imwuidx )
	: mAffinity( 0 )
	, mpModule( pmod )
	, mModuleWuIndex(imwuidx)
	, mCluster(pclus)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

cluster::cluster()
	: miNumWorkUnitsAssigned(0)
	, miNumWorkUnitsDone(0)
	, miNumWorkUnits(0)
{
}
void cluster::AddWorkUnit(workunit*wu)
{
	orkvector<workunit*>& wuvec = mWorkUnits.LockForWrite();
	{
		wuvec.push_back(wu);
	}
	miNumWorkUnits = (int) wuvec.size();
	mWorkUnits.UnLock();
}

workunit* cluster::GetPendingWorkUnit()
{
	const orkvector<workunit*>& wuvec = mWorkUnits.LockForRead();
	workunit* wu = wuvec[ miNumWorkUnitsAssigned++ ];
	mWorkUnits.UnLock();
	return wu;
}
int cluster::GetNumAssignedWorkUnits() const
{
	return miNumWorkUnitsAssigned;
}
int cluster::GetNumPendingWorkUnits() const
{
	const orkvector<workunit*>& wuvec = mWorkUnits.LockForRead();
	int inumleft = (int)(wuvec.size()-miNumWorkUnitsAssigned);
	mWorkUnits.UnLock();
	return inumleft;
}

int cluster::GetNumCompletedWorkUnits() const
{
	return miNumWorkUnitsDone;
}
int cluster::GetNumWorkUnits() const
{
	return miNumWorkUnits;
}
void cluster::NotifyWorkUnitFinished(workunit*wu)
{
	int icount = GetNumWorkUnits();
	if( (miNumWorkUnitsDone+1) == icount )
	{
		const orkset<dgmoduleinst_ptr_t>& Modules = mModules.LockForRead();

		for( orkset<dgmoduleinst_ptr_t>::const_iterator it=Modules.begin(); it!=Modules.end(); it++ )
		{
			dgmoduleinst_ptr_t pmod = (*it);
			//pmod->CombineWork( this );
		}
		mModules.UnLock();
		//mSequence->NotifyClusterFinished(this);
	}
	miNumWorkUnitsDone++;
	OrkAssert( miNumWorkUnitsDone<=icount );
}

void cluster::AddModule(dgmoduleinst_ptr_t pmod)
{
	orkset<dgmoduleinst_ptr_t>& Modules = mModules.LockForWrite();
	{
		Modules.insert(pmod);
	}
	mModules.UnLock();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

processor::processor( const char* name )
	: mbTerminate( false )
	, miNumProcessed( 0 )
	, mName( AddPooledString(name) )
	, mBusy( false )
{
	mCurrentWorkUnit.LockForWrite() = 0;
	mCurrentWorkUnit.UnLock();

}
///////////////////////////////////////////////////////////////////////////////
bool processor::IsBusy() const
{
	return mBusy;
}
///////////////////////////////////////////////////////////////////////////////
float processor::AffinityScore( const Affinity& ain ) const
{
	float workloadtype = (ain&mMyAffinity) ? 1.0f : 0.0f;
	float loadmodifier = 1.0f;
	float result = workloadtype/loadmodifier;
	return result;
}
///////////////////////////////////////////////////////////////////////////////
void processor::terminate()
{
	mbTerminate = true;
}
///////////////////////////////////////////////////////////////////////////////
void processor::ProcessorThreadMain()
{
	SetCurrentThreadName( mName.c_str() );

	while( false == mbTerminate )
	{
		workunit* & curworkunit = mCurrentWorkUnit.LockForWrite();

		if( curworkunit )
		{
			//curworkunit->GetModule()->Compute( curworkunit );
			curworkunit->GetCluster()->NotifyWorkUnitFinished(curworkunit);
			orkprintf( "processor<%s> finished wu<%p>\n", mName.c_str(), curworkunit );
			curworkunit = 0;
			miNumProcessed++;

			if( miNumProcessed%kINFOPER == 0 )
			{
				//orkprintf( "Processor<%s> NumProcessed<%d>\n", mName.c_str(), miNumProcessed );
			}
			mBusy = false;
		}
		mCurrentWorkUnit.UnLock();
		ork::msleep(std::rand()%5);
		ork::msleep(1);
	}

	orkprintf( "Processor<%p><%s> Terminating...\n", this, mName.c_str() );
}
///////////////////////////////////////////////////////////////////////////////
void processor::AssignWorkUnit( workunit* wu )
{
	workunit* & curworkunit = mCurrentWorkUnit.LockForWrite();
	{
		orkprintf( "processor<%s> assigned wu<%p>\n", mName.c_str(), wu );
		OrkAssert( 0 == curworkunit );
		curworkunit = wu;
		mBusy = true;
	}
	mCurrentWorkUnit.UnLock();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

scheduler::scheduler()
{
}

///////////////////////////////////////////////////////////////////////////////

void scheduler::AddProcessor( processor* proc )
{
	mProcessors.LockForWrite().insert(proc);
	mProcessors.UnLock();
}

///////////////////////////////////////////////////////////////////////////////

void scheduler::terminate()
{
//	orkprintf( "scheduler terminating  NumWorkSetsCompleted<%d>\n",  mSeqQueue.GetSerialNumber() );

	const orkset<processor*>& Processors = mProcessors.LockForRead();

	for( orkset<processor*>::const_iterator it=Processors.begin(); it!=Processors.end(); it++ )
	{
		(*it)->terminate();
	}
	mProcessors.UnLock();
}

///////////////////////////////////////////////////////////////////////////////

void scheduler::AssignWorkUnit( workunit* wu )
{
	////////////////////////////////////////
	// wait for a processor for this workunit
	////////////////////////////////////////

	processor* proc = 0;
	float faffinity_hi = 0.0f;

	const orkset<processor*>& Processors = mProcessors.LockForRead();
	size_t inumproc = Processors.size();
	mProcessors.UnLock();

	while( 0 == proc )
	{
		const orkset<processor*>& Processors = mProcessors.LockForRead();
		for( orkset<processor*>::const_iterator	it=Processors.begin();
												it!=Processors.end();
												it++ )
		{
			processor* testproc = *it;

			if( testproc->IsBusy() == false )
			{
				float fscore = testproc->AffinityScore( wu->GetAffinity() );
				if( fscore > faffinity_hi )
				{
					proc = testproc;
					faffinity_hi = fscore;
				}
			}
		}
		mProcessors.UnLock();
		if( proc == 0 )
		{
			ork::msleep(1);
		}
	}
	/////////////////////////////////////////////
	// got a processor, assign the workunit to it
	/////////////////////////////////////////////

	proc->AssignWorkUnit( wu );

}

///////////////////////////////////////////////////////////////////////////////

void scheduler::Process()
{
	int inumwuassigned = 0;
	const graph_set_t& GraphSet = mGraphSet.LockForRead();
	{
		for( graph_set_t::const_iterator it=GraphSet.begin(); it!=GraphSet.end(); it++ )
		{
			graphinst_ptr_t graf =(*it);

			/*const orklut<int,dgmoduleinst_ptr_t>& toposorted = graf->LockTopoSortedChildrenForRead(0);
			{
				size_t inum = toposorted.size();

				for( size_t itopo=0; itopo<inum; itopo++ )
				{
					size_t inum2 = toposorted.size();

					OrkAssert( inum==inum2 );

					dgmoduleinst_ptr_t pmod = toposorted.GetItemAtIndex( (int) itopo ).second;

					bool bmoddirty = pmod->IsDirty();

					if( bmoddirty )
					{
						static int iclussn = 0;

						cluster myclus;
						myclus.SetSerialNumber( iclussn );
						iclussn++;

						pmod->DivideWork( *this, & myclus );

						#if 1
						////////////////////////////////
						// wait until all work assigned
						////////////////////////////////
						while( myclus.GetNumAssignedWorkUnits() < myclus.GetNumWorkUnits() )
						{
							workunit* wu = myclus.GetPendingWorkUnit();
							AssignWorkUnit( wu );
							inumwuassigned++;
						}
						////////////////////////////////
						// wait until all work completed
						////////////////////////////////
						while( myclus.GetNumCompletedWorkUnits() < myclus.GetNumWorkUnits() )
						{
							ork::msleep(1);
						}
						//break;
						#endif
					}
				}
				graf->UnLockTopoSortedChildren();
				ork::msleep(1);
			}*/
		}
	}
	mGraphSet.UnLock();
	ork::msleep(1);
}

///////////////////////////////////////////////////////////////////////////////

int scheduler::GetNumProcessors( const Affinity& affin ) const
{
	int iret = 0;

	const orkset<processor*>& Processors = mProcessors.LockForRead();
	for( orkset<processor*>::const_iterator it=Processors.begin(); it!=Processors.end(); it++ )
	{
		processor* proc = (*it);

		if( (proc->GetAffinity()&affin) != 0 )
		{
			iret++;
		}
	}
	mProcessors.UnLock();
	return iret;
}

///////////////////////////////////////////////////////////////////////////////

void scheduler::QueueModule( moduleinst_ptr_t pmod )
{

}

///////////////////////////////////////////////////////////////////////////////

void scheduler::AddGraph( graphinst_ptr_t graf )
{
	graph_set_t& gset = mGraphSet.LockForWrite();
	graph_set_t::const_iterator it = gset.find(graf);
	OrkAssert( it == gset.end() );
	gset.insert( graf );
	mGraphSet.UnLock();
}

///////////////////////////////////////////////////////////////////////////////

void scheduler::RemoveGraph( graphinst_ptr_t graf )
{
	graph_set_t& gset = mGraphSet.LockForWrite();
	graph_set_t::iterator it = gset.find(graf);
	OrkAssert( it != gset.end() );
	gset.erase( it );
	mGraphSet.UnLock();
}

///////////////////////////////////////////////////////////////////////////////

void scheduler::WaitForIdle()
{
	mGraphSet.LockForRead();
	mGraphSet.UnLock();
}

/*void scheduler::Clear()
{
	mGraphSet.LockForWrite().clear();
	mGraphSet.UnLock();
}*/

///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////
