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
