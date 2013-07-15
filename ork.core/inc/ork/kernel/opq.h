///////////////////////////////////////////////////////////////////////////////
// Orkid - Copyright 2012 Michael T. Mayers 
///////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////
//#include <boost/threadpool.hpp>
//#include <boost/tr1/memory.hpp>
#include <ork/kernel/concurrent_queue.h>
#include <ork/kernel/any.h>
#define _DEBUG_OPQ
#include <ork/orkstl.h>
#include <ork/util/Context.h>


#include <tbb/atomic.h>

#include "mutex.h"
#include "semaphore.h"

#include <set>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

struct Future;

void SetCurrentThreadName(const char* threadName);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef any128 op_wrap_t;
typedef std::function<void()> void_lambda_t;

struct Opq;

struct BarrierSyncReq
{
	BarrierSyncReq(Future&f):mFuture(f){}
	Future& mFuture;
};

struct Op
{
	op_wrap_t mWrapped;	
	std::string mName;

	Op(const Op& oth);
	Op(const BarrierSyncReq& op,const std::string& name="");
	Op(const void_lambda_t& op,const std::string& name="");
	Op();
	~Op();
	void SetOp(const op_wrap_t& op);

	void QueueASync(Opq&q) const;
	void QueueSync(Opq&q) const;
};

//////////////////////////////////////////////////////////////////////

struct IOpqSynchrComparison;

struct OpqSynchro
{
	typedef std::function<bool(OpqSynchro*psyn,int icomparator)> comparison_block_t;
	typedef std::unique_lock<std::mutex> mtx_lock_t;

	OpqSynchro();

	void AddItem(); // mOpCounter++
	void RemItem(); // mOpCounter--;
	void WaitOnCondition(const IOpqSynchrComparison& comparator); // wait til mOpCounter changes
	int NumOps() const;

	tbb::atomic<int> 				mOpCounter;
	std::condition_variable			mOpWaitCV;
	std::mutex 						mOpWaitMtx;

};

//////////////////////////////////////////////////////////////////////

struct IOpqSynchrComparison
{
	virtual bool IsConditionMet(const OpqSynchro& synchro) const = 0;
};

//////////////////////////////////////////////////////////////////////

struct OpGroup
{

	OpGroup(Opq*popq, const char* pname);
	void push( const Op& the_op );
	bool try_pop( Op& out_op );
	void drain();
	void MakeSerial() { mLimitMaxOpsInFlight=1; }

	////////////////////////////////

	std::string mGroupName;
	Opq* mpOpQ;

	////////////////////////////////

	MpMcBoundedQueue<Op,4096> 		mOps;
	tbb::atomic<int>			 	mOpsInFlightCounter;
	tbb::atomic<int>			 	mOpSerialIndex;

	OpqSynchro						mSynchro;

	////////////////////////////////

	int 							mLimitMaxOpsInFlight;
	int 							mLimitMaxOpsQueued;
};

//////////////////////////////////////////////////////////////////////

struct Opq
{
	Opq(int inumthreads, const char* name = "DefOpQ");
	~Opq();

	void push(const Op& the_op);
	void push(const void_lambda_t& l,const std::string& name="");
	void push(const BarrierSyncReq& s);
	void push_sync(const Op& the_op);
	void sync();
	void drain();

	OpGroup* CreateOpGroup(const char* pname);

	static Opq* GlobalConQ();
	static Opq* GlobalSerQ();

	bool Process();

	OpGroup* 					mDefaultGroup;
	//tbb::atomic<int>			 mOpCounter;
	tbb::atomic<int>			 mGroupCounter;

	OpqSynchro						mSynchro;

	std::set<OpGroup*> 			mOpGroups;

	ork::semaphore mSemaphore;

	bool mbGoingDown;
	tbb::atomic<int> mThreadsRunning;
	std::string mName;
};

//////////////////////////////////////////////////////////////////////

struct OpqTest : public ork::util::ContextTLS<OpqTest>
{
	OpqTest( Opq* popq ) : mOPQ( popq ) {}
	Opq* mOPQ;
};
void AssertOnOpQ2( Opq& the_opQ );
void AssertOnOpQ( Opq& the_opQ );
void AssertNotOnOpQ( Opq& the_opQ );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Opq& UpdateSerialOpQ();
Opq& EditorOpQ();

Opq& MainThreadOpQ();
Opq& ConcurrentOpQ();

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////
