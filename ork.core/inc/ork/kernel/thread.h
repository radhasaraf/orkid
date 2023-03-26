////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/kernel/any.h>
#include <ork/kernel/atomic.h>
#include <ork/orktypes.h>
#include <string>

#define USE_STD_THREAD
#include <thread>

namespace ork
{
	void SetCurrentThreadName(const char* threadName);

	struct Thread
	{		
        typedef std::function<void(anyp)> thread_lambda_t;

		Thread(const std::string& thread_name = "", anyp data=nullptr);
		Thread(thread_lambda_t l,
		       anyp data=nullptr,
		       const std::string& thread_name = "" );
		virtual ~Thread() {}

		virtual void run() {}

		void runSynchronous();
		void start();
		void start( const thread_lambda_t& l );
		bool join();

		anyp& userdata() { return _userdata; }

		std::thread*       _threadh;
		std::string		   _threadname;
		anyp		       _userdata;
		ork::atomic<int>   _state;
		bool 			   _running;
		thread_lambda_t    _lambda;
		static void* VirtualThreadImpl(void*data);
   		static void* LambdaThreadImpl(void* pdat);
		
	};
	

using thread_ptr_t = std::shared_ptr<Thread>;


} // namespace ork
