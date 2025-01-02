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
#include <iostream>

//#include "Threading.h"
//#include <mutex>
//#include <condition_variable>
#include <deque>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

#ifdef TPP_WIN32THREADS
#define _WIN32
#endif

#ifdef _WIN32
//#include <winsock2.h>
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#ifndef _WIN32
#include <pthread.h>
#include <sched.h>
#endif

#define VERBOSE 0
using namespace std;

class  thpldata
{
public:
   thpldata() {};
   void setThreadNum(int i)
   {
      thread_no = i;
   }

   int thread_no;
   void* tp;
};

#ifdef _WIN32
unsigned int  threadStart(void* ptr);
#else
void* threadStart(void* ptr);
#endif

class ThreadPool
{
public:

   ThreadPool () : shutdown_ (false)
   {
      running_count_ = 0;
   }

   ThreadPool (int threads) : shutdown_ (false)
   {
      // Create the specified number of threads
      running_count_ = 0;
      threads_.reserve (threads);
      fillPool(threads);

   }

   void fillPool(int threads)
   {


#ifdef _WIN32
      lock_ = CreateMutex(NULL, FALSE, NULL);

      if (lock_ == NULL)
      {
         printf("CreateMutex error: %d\n", (int)GetLastError());
         exit(1);
      }
      countlock_ = CreateMutex(NULL, FALSE, NULL);

      if (countlock_ == NULL)
      {
         printf("CreateMutex error: %d\n", (int)GetLastError());
         exit(1);
      }
#else
      pthread_mutex_init(&lock_, NULL);
      pthread_mutex_init(&countlock_, NULL);
#endif

#ifdef _WIN32
      HANDLE *pHandle = new HANDLE[threads];
#else
      pthread_t *pHandle = new pthread_t[threads];
#endif
      thpldata* data = NULL;
      
      threads_.reserve (threads);
      data_.reserve(threads);
      
      for (int i = 0; i < threads; ++i)
      {
         data = new thpldata();
         data->setThreadNum(i);
         data->tp = this;
         data_.push_back(data);

         this->LOCK(&this->lock_);
         threads_.push_back(pHandle[i]);
         this->UNLOCK(&this->lock_);
#ifdef _WIN32
         threads_[i] = (HANDLE)_beginthreadex(0, 0, (_beginthreadex_proc_type) &threadStart, (void*) data_[i], 0,NULL);
#else
         pthread_create(&threads_[i], NULL, threadStart, (void*) data_[i]);
#endif
      }
   }

   void wait_on_threads()
   {
      std::function <void (void)> job;
      while(haveJob())
      {
         this->LOCK(&this->lock_);

         if (this->jobs_.empty () || this->running_count_ < (int) this->threads_.size() )
         {
            this->UNLOCK(&this->lock_);
            // When threads are still busy and no jobs to do wait ...
#ifdef _WIN32
            SwitchToThread();            
#else
            sched_yield();
#endif    
         }
         else
         {
            // Threads are busy but there are jobs to do
            //std::cerr << "Thread " << i << " does a job" << std::endl;
            //int sz1 = this->jobs_.size();
            job = std::move (this->jobs_.front ());
            this->jobs_.pop_front();
            //int sz2 = this->jobs_.size();

            //if (!did_job)
            // this->running_count_ ++;

            this->UNLOCK(&this->lock_);
            // Do the job without holding any locks
            try
            {
               job();
               //did_job = true;
            }
            catch (std::exception& e)
            {
               cerr << "WARNING: running job exception ... " << e.what() << " ... exiting ... " <<  endl;
               return;
            }
         }
      }
   }

   void wait_for_available_thread()
   {
      this->LOCK(&countlock_);
      while(data_.size() > 0 && running_count_ >= (int)data_.size())
      {
         this->UNLOCK(&countlock_);
#ifdef _WIN32
         SwitchToThread();            
#else
         sched_yield();
#endif    
         this->LOCK(&countlock_);
      }
      this->UNLOCK(&countlock_);
   }

   void drainPool()
   {
      shutdown_ = true;
      for (size_t i =0 ; i < data_.size(); i++)
      {
#ifdef _WIN32
         WaitForSingleObject(threads_[i],INFINITE);
#else
         void* ignore = 0;
         pthread_join(threads_[i],&ignore);
#endif
      }
      
      if (data_.size()) delete data_[0];
      data_.clear();

#ifdef _WIN32
      CloseHandle(lock_);
      CloseHandle(countlock_);
#else
      pthread_mutex_destroy(&lock_);                                                                                               
      pthread_mutex_destroy(&countlock_);        
#endif   
   }

   ~ThreadPool ()
   {
     drainPool();
   }

   void doJob (std::function <void (void)> func)
   {
      // Place a job on the queu and unblock a thread

      this->LOCK(&lock_);
      jobs_.emplace_back (std::move (func));
      this->UNLOCK(&lock_);
   }

   void incrementRunningCount()
   {
      this->LOCK(&countlock_);
      running_count_++;
      this->UNLOCK(&countlock_);
   }

   void decrementRunningCount()
   {
      this->LOCK(&countlock_);
      running_count_--;
      this->UNLOCK(&countlock_);
   }

   bool haveJob()
   {
     //return !jobs_.empty() || (running_count_ > 0);
     bool rtn;
      this->LOCK(&lock_);
      rtn = !jobs_.empty() || running_count_;
      this->UNLOCK(&lock_);

      return rtn;
   }

   int getAvailableThreads(int user)
   {
      //Borrowed from Comet
      int iNumCPUCores;
#ifdef _WIN32
      SYSTEM_INFO sysinfo;
      GetSystemInfo( &sysinfo );
      iNumCPUCores = sysinfo.dwNumberOfProcessors;
#else
      iNumCPUCores= sysconf( _SC_NPROCESSORS_ONLN );
#endif

      iNumCPUCores += user;

      if (iNumCPUCores < 1)
         iNumCPUCores = 1;
      return iNumCPUCores;
   }

#ifdef _WIN32
   void LOCK(HANDLE* mutex)
   {
      WaitForSingleObject( *mutex, INFINITE);
   }

   void UNLOCK(HANDLE* mutex)
   {
      ReleaseMutex( *mutex );
   }
#else
   void LOCK(pthread_mutex_t* mutex)
   {
      pthread_mutex_lock( &*mutex );
   }

   void UNLOCK(pthread_mutex_t* mutex)
   {
      pthread_mutex_unlock( &*mutex );
   }
#endif

   int running_count_;
   bool shutdown_;
   std::deque <std::function <void (void)>> jobs_;

   vector<thpldata*> data_;

#ifdef _WIN32
   HANDLE lock_;
   HANDLE countlock_;

   std::vector<HANDLE> threads_;
#else
   pthread_mutex_t lock_;
   pthread_mutex_t countlock_;

   std::vector<pthread_t> threads_;
#endif

};


#ifdef _WIN32
inline unsigned int  threadStart(LPVOID ptr)
#else
inline void* threadStart(void* ptr)
#endif
{
   std::function <void (void)> job;

   thpldata* data = (thpldata *) ptr;
   int i = data->thread_no;
   ThreadPool* tp = (ThreadPool*)data->tp;
   bool did_job = false;
   while (1)
   {
      tp->LOCK(&tp->lock_);

      while (! tp->shutdown_ && tp->jobs_.empty())
      {
         tp->UNLOCK(&tp->lock_);

#ifdef _WIN32
         SwitchToThread();            
#else
         sched_yield();
#endif
         tp->LOCK(&tp->lock_);
         if (tp->threads_.size() == 0)
         {
            tp->UNLOCK(&tp->lock_);

#ifdef _WIN32
            return 1;
#else
            return NULL;
#endif
         }
    
         //sleep some before reacquiring lock
         //Threading::ThreadSleep(100);
         if  (did_job)
         {
            tp->running_count_ --;
            did_job = false;
         }
      }
      
      if (tp->jobs_.empty ())
      {
         // No jobs to do and we are shutting down
         if (VERBOSE)
            std::cerr << "Thread " << i << " terminates" << std::endl;

         tp->UNLOCK(&tp->lock_);
#ifdef _WIN32
         return 1;
#else
         return NULL;
#endif
         break;
      }
      else
      {
         //std::cerr << "Thread " << i << " does a job" << std::endl;
         job = std::move (tp->jobs_.front ());
         tp->jobs_.pop_front();

         if (!did_job)
            tp->running_count_ ++;

         tp->UNLOCK(&tp->lock_);
         // Do the job without holding any locks
         try
         {
            job();
            did_job = true;
         }
         catch (std::exception& e)
         {
            cerr << "WARNING: running job exception ... " << e.what() << " ... exiting ... " <<  endl;
#ifdef _WIN32
            return 1;
#else
            return NULL;
#endif
            break;
         }
      }
   }
}


#endif // _THREAD_POOL_H_
