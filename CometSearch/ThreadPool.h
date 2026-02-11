// Copyright 2023 Jimmy Eng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "BS_thread_pool.hpp"
#include <functional>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sched.h>
#endif

// Wrapper class to maintain API compatibility with existing code
class ThreadPool
{
public:
   ThreadPool() : pool_(nullptr), thread_count_(0)
   {
   }

   ThreadPool(int threads) : pool_(nullptr), thread_count_(threads)
   {
      fillPool(threads);
   }

   void fillPool(int threads)
   {
       // Map non-positive values to a single worker thread to preserve
       // original "total thread count" semantics and avoid BS::thread_pool's
       // special meaning for 0 ("use hardware_concurrency default").
       int pool_threads = threads;
       if (pool_threads <= 0)
       {
           pool_threads = 1;
       }
       thread_count_ = pool_threads;
       pool_ = std::make_unique<BS::thread_pool<>>(pool_threads);
   }

   void wait_on_threads()
   {
      if (pool_)
      {
         pool_->wait();
      }
   }

   void wait_for_available_thread()
   {
      if (pool_)
      {
         // Wait until at least one thread is available
         // Use progressive backoff to reduce CPU usage:
         // 1. Yield a few times (low latency, low cost)
         // 2. Then sleep with increasing duration (reduces CPU burn)
         constexpr int MAX_YIELD_ATTEMPTS = 10;
         constexpr int MAX_SHORT_SLEEP_ATTEMPTS = 20;
         constexpr int MAX_BACKOFF_LEVEL = MAX_YIELD_ATTEMPTS + MAX_SHORT_SLEEP_ATTEMPTS;
         int attempts = 0;
         
         while (pool_->get_tasks_running() >= pool_->get_thread_count())
         {
            if (attempts < MAX_YIELD_ATTEMPTS)
            {
               // Fast path: just yield to other threads
#ifdef _WIN32
               SwitchToThread();
#else
               sched_yield();
#endif
            }
            else if (attempts < MAX_BACKOFF_LEVEL)
            {
               // Medium path: short sleep (1ms)
               std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else
            {
               // Long wait path: longer sleep (5ms) to avoid burning CPU
               std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            
            // Cap counter at final backoff level
            attempts = std::min(attempts + 1, MAX_BACKOFF_LEVEL);
         }
      }
   }

   void drainPool()
   {
      // BS::thread_pool destructor handles cleanup
      pool_.reset();
   }

   ~ThreadPool()
   {
      drainPool();
   }

   void doJob(std::function<void(void)> func)
   {
      if (!pool_)
      {
         throw std::logic_error("ThreadPool::doJob() called before fillPool() - work would be silently dropped");
      }
      pool_->detach_task(std::move(func));
   }

   int getAvailableThreads(int user)
   {
      // Borrowed from original Comet implementation
      int iNumCPUCores;
#ifdef _WIN32
      SYSTEM_INFO sysinfo;
      GetSystemInfo(&sysinfo);
      iNumCPUCores = sysinfo.dwNumberOfProcessors;
#else
      iNumCPUCores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

      iNumCPUCores += user;

      if (iNumCPUCores < 1)
         iNumCPUCores = 1;
      return iNumCPUCores;
   }

   // Expose jobs_ member to maintain compatibility with code that checks queue size
   struct jobs_proxy
   {
      jobs_proxy(ThreadPool* tp) : tp_(tp) {}
      
      size_t size() const
      {
         if (tp_->pool_)
            return tp_->pool_->get_tasks_queued();
         return 0;
      }
      
   private:
      ThreadPool* tp_;
   };
   
   jobs_proxy jobs_{this};

private:
   std::unique_ptr<BS::thread_pool<>> pool_;
   int thread_count_;
};

#endif // _THREAD_POOL_H_
