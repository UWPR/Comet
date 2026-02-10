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

#ifndef _OSSPECIFICTHREADING_H_
#define _OSSPECIFICTHREADING_H_

// Modern C++ cross-platform threading using standard library
#include <mutex>
#include <condition_variable>
#include <thread>

// Type definitions for cross-platform threading
using Mutex = std::mutex;
using ThreadId = std::thread::id;

// Thread procedure signature
// Returns void* for compatibility with existing code
using ThreadProc = void* (*)(void*);

// Semaphore implementation using C++ standard library primitives
struct Semaphore
{
	std::condition_variable condition;
	std::mutex mutex;
	bool conditionSet;

	Semaphore() : conditionSet(false) {}
};

#endif // _OSSPECIFICTHREADING_H_