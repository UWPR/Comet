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
   usleep(dwMilliseconds);
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
