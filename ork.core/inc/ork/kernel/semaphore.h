////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/kernel/atomic.h>
#include <ork/kernel/mutex.h>
#include <pthread.h>

namespace ork {

struct condition_variable {
  condition_variable(bool do_init = false);

  void init();
  void notify_one();
  void notify_all();
  int wait(unsigned long usec);

private:
  pthread_mutex_t mMutex;
  pthread_cond_t mCondVar;
  ork::atomic<int64_t> mWaitCount;
  ork::atomic<int64_t> mReleaseCount;
};

struct semaphore {
  semaphore(const char* name);
  void notify();
  void wait();

private:
  ork::mutex mMutex;
  std::condition_variable mCondition;
  ork::atomic<int> mCount;
};

} // namespace ork
