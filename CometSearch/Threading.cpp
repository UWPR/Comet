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


#include "Threading.h"
#include <mutex>

///////////////////////////////////////////////////////////////
// Cross-platform implementations using C++ standard library
///////////////////////////////////////////////////////////////

Threading::Threading()
{
}

Threading::~Threading()
{
}

// Mutex-specific methods
bool Threading::InitMutex(Mutex* pMutex)
{
    // std::mutex is constructed by default constructor
    // Nothing needed here as the mutex is already initialized
    return (pMutex != nullptr);
}

void Threading::LockMutex(Mutex& mutex)
{
    mutex.lock();
}

void Threading::UnlockMutex(Mutex& mutex)
{
    mutex.unlock();
}

void Threading::DestroyMutex(Mutex& /*mutex*/)
{
    // std::mutex destructor handles cleanup automatically
    // Ensure mutex is unlocked before destruction
    // (caller's responsibility to ensure proper unlocking)
}