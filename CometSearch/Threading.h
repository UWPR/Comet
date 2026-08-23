// Copyright 2012-2026 Jimmy Eng
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


///////////////////////////////////////////////////////////////////////////////
// This file defines an OS independent "interface" for mutex-related
// functionality. It is meant to be used as a singleton - use Threading::Inst()
// to get a pointer to the single instance of the Threading object and use it
// to access the interface methods.
//
// Now implemented using C++ standard library for cross-platform consistency.
//
// Thread-creation and semaphore methods (BeginThread/ThreadSleep/*Semaphore)
// were removed here (2026-08-21) -- dead code with no callers anywhere in the
// repo, and the hand-rolled Semaphore was a lossy single-bit condition flag
// (a second SignalSemaphore() before WaitSemaphore() wakes loses the first
// signal, unlike a real counting semaphore) that would have bitten whoever
// revived it. All real thread creation in this codebase goes through
// ThreadPool/std::thread directly.
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
    // Renamed to avoid Windows API macro conflicts
    static bool InitMutex(Mutex* pMutex);
    static void LockMutex(Mutex& mutex);
    static void UnlockMutex(Mutex& mutex);
    static void DestroyMutex(Mutex& mutex);
};

#endif // ifndef _THREADING_H_