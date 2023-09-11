/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

///////////////////////////////////////////////////////////////////////////////
// This file defines an OS independent "interface" for threading-related
// functionalities, such as creating/destroying mutexes, semaphores and
// threads. It is meant to be used as a singleton - use Threading::Inst() to
// get a pointer to the single instance of the Threading object and use it
// to access the interface methods.
///////////////////////////////////////////////////////////////////////////////

#ifndef _THREADING_H_
#define _THREADING_H_

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

#endif // ifndef _THREADING_H_
