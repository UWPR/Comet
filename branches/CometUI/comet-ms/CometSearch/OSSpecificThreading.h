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
// This file defines generic types for OS-specific threading resources
///////////////////////////////////////////////////////////////////////////////

#ifndef _OSSPECIFICTHREADING_H
#define _OSSPECIFICTHREADING_H

#ifdef _WIN32

///////////////////////////////////////////////////////////////////////////////
//  Windows-specific definitions for threading
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>

typedef CRITICAL_SECTION Mutex;
typedef HANDLE Semaphore;
typedef unsigned int ThreadId;
typedef void* (__cdecl *ThreadProc)(void*);

#else

///////////////////////////////////////////////////////////////////////////////
//  Posix-specific definitions for threading
///////////////////////////////////////////////////////////////////////////////
#include <pthread.h>

typedef pthread_mutex_t Mutex;
typedef pthread_t ThreadId;
typedef void* (*ThreadProc)(void*);
typedef struct PosixSemaphore {
   pthread_mutex_t mutex;
   pthread_cond_t condition;
   bool conditionSet;
} Semaphore;

#endif // ifdef _WIN32

#endif  // ifndef _OSSPECIFICTHREADING_H
