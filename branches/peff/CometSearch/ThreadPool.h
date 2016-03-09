/*
   Copyright 2012 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

///////////////////////////////////////////////////////////////////////////////
// Templated class for creating a pool of threads that hang around and wait
// to do work. It should work for both Win32 threads and Posix threads. If the
// user specifies a max number of threads to allow in the pool, jobs will be
// queued until a thread is free to do the work.
//
// User can specify the following:
//     * A parameter type to pass to the threads
//     * Thread function the threads will run
//     * Minimum number of threads to keep around in the pool
//     * The maximum number of threads to allow in the pool
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <vector>
#include <deque>
#include <stdint.h>
#include "Threading.h"
#include "CometStatus.h"

template<class T> class ThreadManager;

// This is the pool where the threads reside. New threads are created as
// needed, but without exceeding the max number specified by the user.
// Jobs are queued if necessary, until a thread is free to do the work.
template <class T> class ThreadPool
{
public:
   typedef void (*ThreadProc) (T param);

public:
   static const int UnlimitedThreads = -1;

// typedef enum NextThreadState { Run, Sleep, Die } ;
   enum NextThreadState { Run, Sleep, Die } ;

   ThreadPool(ThreadProc threadProc, int minThreads, int maxThreads, int maxNumParamsToQueue = -1)
   {
      _minThreads = minThreads;
      _maxThreads = maxThreads;
      _threadProc = threadProc;
      _maxQueuedParams = maxNumParamsToQueue;

      Threading::CreateMutex(&_poolAccessMutex);

      Threading::CreateSemaphore(&_queueParamsSemaphore);

      for (_numCurrThreads=0; _numCurrThreads < _minThreads; _numCurrThreads++)
      {
         new ThreadManager<T>(this);
      }
   }

   ~ThreadPool()
   {
     for (unsigned int i = 0; i < _threads.size(); i++)
     {
         _threads[i]->End();
     }

     Threading::DestroyMutex(_poolAccessMutex);

     Threading::DestroySemaphore(_queueParamsSemaphore);
   }

   ThreadProc GetThreadProc() { return _threadProc; }

   void Launch(T param)
   {
      Threading::LockMutex(_poolAccessMutex);
      if (!_threads.empty())
      {
         ThreadManager<T> *pThreadMgr = _threads.back();
         _threads.pop_back();
         Threading::UnlockMutex(_poolAccessMutex);
         pThreadMgr->Wake(param);
         return;
      }

      if (_numCurrThreads < _maxThreads)
      {
         new ThreadManager<T>(this);
         _numCurrThreads++;
      }

      _params.push_back(param);
      Threading::UnlockMutex(_poolAccessMutex);
   }

   NextThreadState RejoinPool(ThreadManager<T> *pThreadMgr)
   {
      Threading::LockMutex(_poolAccessMutex);

      // Any parameters queued?  If yes, give it to the thread to process.
      if (!_params.empty())
      {
         T param = _params.front();
         _params.pop_front();
         Threading::UnlockMutex(_poolAccessMutex);
         pThreadMgr->SetParam(param);
         return ThreadPool<T>::Run;
      }

      // Have we exceeded the min num threads allowed?  If yes, reduce
      // the number back down.
      if (_numCurrThreads > _minThreads)
      {
         _numCurrThreads--;
         Threading::UnlockMutex(_poolAccessMutex);
         return ThreadPool<T>::Die;
      }

      // Has there been an error, or has search been cancelled?
      // If so, we need to kill the thread.
      if (g_cometStatus.IsError() || g_cometStatus.IsCancel())
      {
         _numCurrThreads--;
         Threading::UnlockMutex(_poolAccessMutex);
         return ThreadPool<T>::Die;
      }

      // No params queued; ask thread to go to sleep & wait mode.
      _threads.push_back(pThreadMgr);
      Threading::UnlockMutex(_poolAccessMutex);
      return ThreadPool<T>::Sleep;
   }

   void WaitForThreads()
   {
      // This is not the best solution. Try to think of a better way to
      // handle this - maybe using a semaphore?

      // Give the threads a chance to start doing work - 1 second at the most!
      const unsigned long ulMaxTotalWaitMilliseconds = 1000;
      const unsigned long ulWaitMilliseconds = 10;
      unsigned long ulElapsedTimeMilliseconds = 0;
      while (NumActiveThreads() == 0)
      {
         if (ulElapsedTimeMilliseconds >= ulMaxTotalWaitMilliseconds)
         {
            break;
         }
         Threading::ThreadSleep(ulWaitMilliseconds);
         ulElapsedTimeMilliseconds += ulWaitMilliseconds;
      }

      // Now wait for all of them to go to sleep
      while (NumActiveThreads() != 0)
      {
         Threading::ThreadSleep(100);
      }
   }

   void WaitForQueuedParams()
   {
      if (ShouldCheckQueuedParams() && (NumParamsQueued() > _maxQueuedParams))
      {
         Threading::WaitSemaphore(_queueParamsSemaphore);
      }
   }

   void CheckQueuedParams()
   {
       if (ShouldCheckQueuedParams())
       {
           if (NumParamsQueued() <= _maxQueuedParams)
           {
               QueueMoreParams();
           }
       }
   }

   int NumParamsQueued()
   {
      Threading::LockMutex(_poolAccessMutex);
      int numParamsQueued = _params.size();
      Threading::UnlockMutex(_poolAccessMutex);
      return numParamsQueued;
   }

   int NumActiveThreads()
   {
      Threading::LockMutex(_poolAccessMutex);
      int numActiveThreads = _numCurrThreads - _threads.size();
      Threading::UnlockMutex(_poolAccessMutex);
      return numActiveThreads;
   }

protected:
   bool ShouldCheckQueuedParams()
   {
       // Only check for queued params if we have a valid number for _maxQueuedParams
       return _maxQueuedParams != -1;
   }

   void QueueMoreParams()
   {
      Threading::SignalSemaphore(_queueParamsSemaphore);
   }

   ThreadProc                     _threadProc;
   std::vector<ThreadManager<T>*> _threads;
   std::deque<T>                  _params;
   Mutex                          _poolAccessMutex;
   int                            _maxThreads;
   int                            _minThreads;
   int                            _numCurrThreads;
   int                            _maxQueuedParams;
   Semaphore                      _queueParamsSemaphore;
};


// Thread manager for individual threads - calls specified thread proc function
// and takes a prameter of type T. It manages the interactions with the thread
// pool.
template<class T> class ThreadManager
{
public:
   static uint32_t ThreadRoutingFunction(void* pParam)
   {
      ThreadManager<T> *pThreadMgr = reinterpret_cast<ThreadManager<T>*>(pParam);

      // Infinite loop in here until instructed to die.
      return pThreadMgr->Run();
   }

   ThreadManager(ThreadPool<T> *pPool)
   {
      _endThread = false;

      ThreadManager<T>::_pPool = pPool;

      Threading::CreateSemaphore(&_wakeSemaphore);
      Threading::CreateSemaphore(&_sleepSemaphore);

      // Begin the thread that is being managed
      Threading::BeginThread(
            reinterpret_cast<ThreadProc>(ThreadManager<T>::ThreadRoutingFunction),
            this,   // Parameter to the thread
            &_threadIdentifier);
   }

   ~ThreadManager()
   {
      Threading::DestroySemaphore(_wakeSemaphore);
      Threading::DestroySemaphore(_sleepSemaphore);
   }

   void SetParam(T param) { ThreadManager::_param = param; }

   T GetParam() { return _param; }

   void Sleep()
   {
      Threading::SignalSemaphore(_sleepSemaphore);
      Threading::WaitSemaphore(_wakeSemaphore);
   }

   void Wake(T param)
   {
      Threading::WaitSemaphore(_sleepSemaphore);
      SetParam(param);
      Threading::SignalSemaphore(_wakeSemaphore);
   }

   void End()
   {
      Threading::WaitSemaphore(_sleepSemaphore);
      _endThread = true;
      Threading::SignalSemaphore(_wakeSemaphore);
   }

   uint32_t Run()
   {
      typename ThreadPool<T>::NextThreadState nextState;
      while ((nextState = _pPool->RejoinPool(this)) != ThreadPool<T>::Die)
      {
         // Rejoin will set pParam if possible
         if (nextState == ThreadPool<T>::Sleep)
         {
            Sleep();
         }

         // We've been awoken. First check to see if we should die. If not,
         // we run the function we're bound to.
         if (_endThread)
         {
             break;
         }

         (*_pPool->GetThreadProc()) (GetParam());

         // So we don't loop endlessly with the same parameter
         SetParam(NULL);

         _pPool->CheckQueuedParams();
      }

      // The thread exits if the pool won't accept it's rejoin - deleting itself on exit
      Threading::EndThread();
      delete this;
      return 0;
   }

protected:
   T             _param;
   ThreadPool<T> *_pPool;
   Semaphore     _sleepSemaphore;
   Semaphore     _wakeSemaphore;
   ThreadId      _threadIdentifier;
   bool          _endThread;
};

#endif
