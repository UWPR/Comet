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

#include "threading/SearchMemoryPool.h"
#include "Common.h"
#include "CometStatus.h"
#include <string>


bool SearchMemoryPool::allocate(int nSlots, int iArraySize)
{
   if (_allocated)
      return true;

   try
   {
      _pool  = new bool*[nSlots]();   // value-init to nullptr so partial allocs are safe to delete[]
      for (int i = 0; i < nSlots; ++i)
         _pool[i] = new bool[iArraySize]();
      _freeSlots.reserve(nSlots);
      for (int i = 0; i < nSlots; ++i)
         _freeSlots.push_back(i);
      _nSlots    = nSlots;
      _allocated = true;
      return true;
   }
   catch (const std::bad_alloc& ba)
   {
      // Free whatever was allocated before the throw.
      if (_pool)
      {
         for (int k = 0; k < nSlots; ++k)
            delete[] _pool[k];   // safe: unset slots are nullptr after value-init above
         delete[] _pool;
         _pool = nullptr;
      }
      _freeSlots.clear();
      std::string strErrorMsg = " Error - SearchMemoryPool::allocate failed. bad_alloc: " + std::string(ba.what()) + ".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      _allocated = false;
      return false;
   }
}


void SearchMemoryPool::_deallocate(int nSlots)
{
   for (int i = 0; i < nSlots; ++i)
      delete[] _pool[i];
   delete[] _pool;
   _pool      = nullptr;
   _freeSlots.clear();
   _allocated = false;
}


void SearchMemoryPool::deallocate()
{
   if (_allocated)
      _deallocate(_nSlots);
}


int SearchMemoryPool::acquireSlot()
{
   std::unique_lock<std::mutex> lock(_mutex);
   bool found = _cv.wait_for(lock, std::chrono::seconds(240), [this]() { return !_freeSlots.empty(); });
   if (!found)
      return -1;
   int slot = _freeSlots.back();
   _freeSlots.pop_back();
   return slot;
}


void SearchMemoryPool::releaseSlot(int slot)
{
   { std::lock_guard<std::mutex> lk(_mutex); _freeSlots.push_back(slot); }
   _cv.notify_one();
}
