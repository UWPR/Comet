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


#include "Threading.h"
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>

///////////////////////////////////////////////////////////////
// Cross-platform implementations using C++ standard library
///////////////////////////////////////////////////////////////

ThreadId Threading::_threadId;

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

void Threading::DestroyMutex(Mutex& mutex)
{
    // std::mutex destructor handles cleanup automatically
    // Ensure mutex is unlocked before destruction
    // (caller's responsibility to ensure proper unlocking)
}

// Thread-specific methods
void Threading::BeginThread(ThreadProc pFunction, void* arg, ThreadId* pThreadId)
{
    // Create a new thread
    auto threadPtr = std::make_unique<std::thread>([pFunction, arg]() {
        // Execute the thread procedure
        pFunction(arg);
        });

    // Get the thread ID before moving the thread
    ThreadId newThreadId = threadPtr->get_id();

    // Store the thread ID
    if (pThreadId != nullptr)
    {
        *pThreadId = newThreadId;
    }
    _threadId = newThreadId;

    // Detach the thread to allow independent execution
    threadPtr->detach();
}

void Threading::ThreadSleep(unsigned long dwMilliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}

// Semaphore methods
void Threading::InitSemaphore(Semaphore* pSem)
{
    // Semaphore members are already initialized by the constructor
    // Just ensure the condition flag is set to false
    if (pSem != nullptr)
    {
        pSem->conditionSet = false;
    }
}

void Threading::WaitSemaphore(Semaphore& sem)
{
    std::unique_lock<std::mutex> lock(sem.mutex);
    // Wait until condition is set
    sem.condition.wait(lock, [&sem] { return sem.conditionSet; });
    // Reset the condition after being signaled
    sem.conditionSet = false;
}

void Threading::SignalSemaphore(Semaphore& sem)
{
    {
        std::lock_guard<std::mutex> lock(sem.mutex);
        sem.conditionSet = true;
    }
    // Notify one waiting thread
    sem.condition.notify_one();
}

void Threading::DestroySemaphore(Semaphore& sem)
{
    // std::condition_variable and std::mutex destructors handle cleanup automatically
    // Wake up any waiting threads before destruction
    {
        std::lock_guard<std::mutex> lock(sem.mutex);
        sem.conditionSet = true;
    }
    sem.condition.notify_all();
}