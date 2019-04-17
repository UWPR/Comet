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
// This file defines an OS independent "interface" for threading-related
// functionalities, such as creating/destroying mutexes, semaphores and
// threads. It is meant to be used as a singleton - use Threading::Inst() to
// get a pointer to the single instance of the Threading object and use it
// to access the interface methods.
///////////////////////////////////////////////////////////////////////////////

#ifndef _THREADING_H
#define _THREADING_H

#include "OSSpecificThreading.h"

class Threading
{
public:

   Threading();
   ~Threading();

   // Mutex-specific methods
   static bool CreateMutex(Mutex* pMutex);
   static void LockMutex(Mutex& mutex);
   static void UnlockMutex(Mutex& mutex);
   static void DestroyMutex(Mutex& mutex);

   // Thread-specific methods
   static void BeginThread(ThreadProc pFunction, void* arg, ThreadId* pThreadId);
   static void ThreadSleep(unsigned long dwMilliseconds);
   static void EndThread();

   // Semaphore methods
   static void CreateSemaphore(Semaphore* pSem);
   static void WaitSemaphore(Semaphore& sem);
   static void SignalSemaphore(Semaphore& sem);
   static void DestroySemaphore(Semaphore& sem);

private:
    static ThreadId _threadId;
};

#endif // ifndef _THREADING_H
