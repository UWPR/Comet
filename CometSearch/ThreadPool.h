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
#include <iostream>
#include <string>

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
   // Callback type for reporting task failures to the application error system.
   // Set via setErrorHandler() so that doJob() can propagate exceptions without
   // needing to #include CometStatus.h (which would create a circular include
   // through Common.h -> ThreadPool.h -> CometStatus.h -> Common.h).
   using ErrorHandler = std::function<void(const std::string&)>;

   ThreadPool() : pool_(nullptr), thread_count_(0)
   {
   }

   ThreadPool(int threads) : pool_(nullptr), thread_count_(0)
   {
      fillPool(threads);
   }

   // Delete copy and move operations to prevent dangling pointer in jobs_proxy
   ThreadPool(const ThreadPool&) = delete;
   ThreadPool& operator=(const ThreadPool&) = delete;
   ThreadPool(ThreadPool&&) = delete;
   ThreadPool& operator=(ThreadPool&&) = delete;

   /// @brief Register a callback invoked when a task throws an exception.
   /// Typically set once after construction:
   ///   tp->setErrorHandler([](const std::string& msg) {
   ///       g_cometStatus.SetStatus(CometResult_Failed, msg);
   ///   });
   void setErrorHandler(ErrorHandler handler)
   {
      errorHandler_ = std::move(handler);
   }

   /// @brief Initialize the thread pool with the specified number of threads.
   /// @param threads Must be >= 1. If <= 0, g_staticParams.options.iNumThreads was not set
   ///        correctly by InitializeStaticParams(); throws std::invalid_argument.
   void fillPool(int threads)
   {
      if (threads <= 0)
      {
         throw std::invalid_argument(
            "fillPool() called with invalid thread count " + std::to_string(threads)
            + ". iNumThreads must be >= 1 after InitializeStaticParams().");
      }

      // Guard against calling fillPool when pool is already initialized.
      // Drain existing pool first to avoid silently dropping in-flight tasks.
      if (pool_)
      {
         drainPool();
      }

      thread_count_ = static_cast<size_t>(threads);
      pool_ = std::make_unique<BS::thread_pool<>>(threads);
   }

   /// @brief Wait for all currently queued and running tasks to complete
   /// @throws std::logic_error if called before fillPool()
   void wait_on_threads()
   {
      if (!pool_)
      {
         throw std::logic_error("ThreadPool::wait_on_threads() called before fillPool()");
      }
      pool_->wait();
   }

   /// @brief Wait until at least one thread becomes available (uses progressive backoff)
   /// @throws std::logic_error if called before fillPool()
   void wait_for_available_thread()
   {
      if (!pool_)
      {
         throw std::logic_error("ThreadPool::wait_for_available_thread() called before fillPool()");
      }

      // Wait until at least one thread is available
      // Use progressive backoff to reduce CPU usage:
      // 1. Yield a few times (low latency, low cost)
      // 2. Then sleep with increasing duration (reduces CPU burn)
      static constexpr int MAX_YIELD_ATTEMPTS = 10;
      static constexpr int MAX_SHORT_SLEEP_ATTEMPTS = 20;
      static constexpr int MAX_BACKOFF_LEVEL = MAX_YIELD_ATTEMPTS + MAX_SHORT_SLEEP_ATTEMPTS;
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
         attempts++;
      }
   }

   void drainPool()
   {
      // BS::thread_pool destructor handles cleanup
      pool_.reset();
      thread_count_ = 0;
   }

   ~ThreadPool()
   {
      drainPool();
   }

   /// @brief Submit a task to the thread pool
   /// @param func Function to execute in the thread pool
   /// @throws std::logic_error if called before fillPool()
   void doJob(std::function<void(void)> func)
   {
      if (!pool_)
      {
         throw std::logic_error("ThreadPool::doJob() called before fillPool() - work would be silently dropped");
      }

      // Capture the error handler by value so the lambda is self-contained.
      // If no handler was set, exceptions are still logged to stderr.
      ErrorHandler handler = errorHandler_;

      auto wrapped_func = [func = std::move(func), handler = std::move(handler)]() {
         try
         {
            func();
         }
         catch (const std::exception& e)
         {
            std::string strErrorMsg = " Error: Exception in thread pool task: "
               + std::string(e.what()) + "\n";
            std::cerr << strErrorMsg;
            if (handler)
               handler(strErrorMsg);
         }
         catch (...)
         {
            std::string strErrorMsg = " Error: Unknown exception in thread pool task\n";
            std::cerr << strErrorMsg;
            if (handler)
               handler(strErrorMsg);
         }
         };

      pool_->detach_task(std::move(wrapped_func));
   }

   /// @brief Check if the thread pool has been initialized
   /// @return true if fillPool() has been called, false otherwise
   bool is_initialized() const
   {
      return pool_ != nullptr;
   }

   /// @brief Get the number of threads in the pool
   /// @return Number of threads, or 0 if not initialized
   size_t get_thread_count() const
   {
      return thread_count_;
   }

   /// @brief Get the number of tasks currently running
   /// @return Number of running tasks, or 0 if not initialized
   size_t get_tasks_running() const
   {
      if (!pool_)
      {
         return 0;
      }
      return pool_->get_tasks_running();
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
         if (!tp_ || !tp_->pool_)
            return 0;
         return tp_->pool_->get_tasks_queued();
      }

      bool empty() const
      {
         return size() == 0;
      }

   private:
      ThreadPool* tp_;
   };

   jobs_proxy jobs_{ this };

private:
   std::unique_ptr<BS::thread_pool<>> pool_;
   size_t thread_count_;  // Changed from int to size_t for consistency
   ErrorHandler errorHandler_;  // Optional callback for propagating task errors
};

#endif // _THREAD_POOL_H_