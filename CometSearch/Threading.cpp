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

#include "Threading.h"

ThreadId Threading::_threadId;

#ifndef _WIN32
#include <unistd.h>

///////////////////////////////////////////////////////////////
// Implementations for Threading base class specific to POSIX
///////////////////////////////////////////////////////////////

Threading::Threading()
{
}

Threading::~Threading()
{
}

// Posix specific object destructor.
bool Threading::CreateMutex(Mutex* pMutex)
{
   pthread_mutex_init(pMutex, NULL);
   return true;
}

void Threading::LockMutex(Mutex& mutex)
{
   pthread_mutex_lock(&mutex);
}

void Threading::UnlockMutex(Mutex& mutex)
{
   pthread_mutex_unlock(&mutex);
}

void Threading::DestroyMutex(Mutex& mutex)
{
   pthread_mutex_destroy(&mutex);
}

void Threading::BeginThread(ThreadProc pFunction, void* arg, ThreadId* pThreadId)
{
   _threadId = *pThreadId;
   pthread_create(pThreadId, NULL, pFunction, arg);
}

void Threading::EndThread()
{
    pthread_exit((void*)&_threadId);
}

void Threading::ThreadSleep(unsigned long dwMilliseconds)
{
// usleep(dwMilliseconds);  // usleep deprecated
   struct timespec ts;
   ts.tv_sec = dwMilliseconds / 1000;
   ts.tv_nsec = (dwMilliseconds % 1000) * 1000000;
   nanosleep(&ts, NULL);
}

void Threading::CreateSemaphore(Semaphore* pSem)
{
   pthread_cond_init(&(pSem->condition), NULL);
   pthread_mutex_init(&(pSem->mutex), NULL);
   pSem->conditionSet = false;
}

void Threading::WaitSemaphore(Semaphore& sem)
{
   pthread_mutex_lock(&sem.mutex);
   while (!(sem.conditionSet))
   {
      pthread_cond_wait(&sem.condition, &sem.mutex);
   }
   sem.conditionSet = false;
   pthread_mutex_unlock(&sem.mutex);
}

void Threading::SignalSemaphore(Semaphore& sem)
{
   pthread_mutex_lock(&sem.mutex);
   sem.conditionSet = true;
   pthread_cond_signal(&sem.condition);
   pthread_mutex_unlock(&sem.mutex);
}

void Threading::DestroySemaphore(Semaphore& sem)
{
   pthread_cond_destroy(&sem.condition);
   pthread_mutex_destroy(&sem.mutex);
}

#else  // _WIN32
#include <process.h>

//////////////////////////////////////////////////////////////////////
// Implementations for Threading base class specific to the WIN32 OS
//////////////////////////////////////////////////////////////////////

Threading::Threading()
{
}

Threading::~Threading()
{
}

bool Threading::CreateMutex(Mutex* pMutex)
{
   InitializeCriticalSection(pMutex);
   return (pMutex!=NULL);
}

void Threading::LockMutex(Mutex& mutex)
{
   EnterCriticalSection(&mutex);
}

void Threading::UnlockMutex(Mutex& mutex)
{
   LeaveCriticalSection(&mutex);
}

void Threading::DestroyMutex(Mutex& mutex)
{
   DeleteCriticalSection(&mutex);
}

void Threading::BeginThread(ThreadProc pFunction, void* arg, ThreadId* pThreadId)
{
    _threadId = *pThreadId;
   _beginthreadex (NULL,
         0,
         (unsigned int(__stdcall*) ( void*)) pFunction,
         (void*) arg,
         0,
         pThreadId);
}

void Threading::EndThread()
{
    _endthreadex(0);
}

void Threading::ThreadSleep(unsigned long dwMilliseconds)
{
   Sleep(dwMilliseconds);
}

void Threading::CreateSemaphore(Semaphore* pSem)
{
   *pSem = CreateEvent(NULL,0,0,NULL);
}

void Threading::WaitSemaphore(Semaphore& sem)
{
   WaitForSingleObject(sem, INFINITE);
}

void Threading::SignalSemaphore(Semaphore& sem)
{
   SetEvent(sem);
}

void Threading::DestroySemaphore(Semaphore& sem)
{
   CloseHandle(sem);
}

#endif // ifdef _WIN32
