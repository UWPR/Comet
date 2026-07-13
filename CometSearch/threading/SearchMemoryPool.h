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

// Owns the per-thread duplicate-fragment scratch arrays used during FI/PI search.
// Extracted from CometSearch static members (_pbSearchMemoryPool,
// _ppbDuplFragmentArr, AllocateMemory, DeallocateMemory, AcquirePoolSlot)
// and the paired globals g_searchMemoryPoolMutex, g_searchPoolCV,
// g_bCometSearchMemoryAllocated.

#ifndef _SEARCHMEMORYPOOL_H_
#define _SEARCHMEMORYPOOL_H_

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <vector>

class SearchMemoryPool
{
public:
   SearchMemoryPool() = default;
   ~SearchMemoryPool() { if (_allocated) _deallocate(_nSlots); }

   // Allocates nSlots scratch arrays each of iArraySize bools, plus nSlots each of
   // the PI_DB target/decoy ion-mass and precursor-NL scratch buffers (element
   // counts, not bytes -- caller computes these from MAX_FRAGMENT_CHARGE,
   // NUM_ION_SERIES, MAX_PEPTIDE_LEN, VMODS, MAX_PRECURSOR_NL_SIZE,
   // MAX_PRECURSOR_CHARGE so this header stays free of those dependencies).
   bool allocate(int nSlots, int iArraySize, size_t nBinnedIonMassesElems, size_t nBinnedPrecursorNLElems);

   // Frees all scratch arrays.
   void deallocate();

   // Blocks up to 240 s until a slot is free.
   // Returns slot index [0, nSlots) or -1 on timeout.
   int  acquireSlot();

   // Releases the slot and wakes one waiting acquireSlot() caller.
   void releaseSlot(int slot);

   // Returns the duplicate-fragment scratch array for a claimed slot.
   bool* duplFragmentArr(int slot) const { assert(slot >= 0 && slot < _nSlots); return _pool[slot]; }

   // Returns the PI_DB target/decoy ion-mass and precursor-NL scratch buffers for a
   // claimed slot. Caller reinterprets the flat buffer as the
   // [MAX_FRAGMENT_CHARGE+1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS+2] /
   // [MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE] array shape it was sized for.
   unsigned int* binnedIonMasses(int slot)      const { assert(slot >= 0 && slot < _nSlots); return _poolBinnedIonMasses[slot]; }
   unsigned int* binnedPrecursorNL(int slot)    const { assert(slot >= 0 && slot < _nSlots); return _poolBinnedPrecursorNL[slot]; }
   unsigned int* binnedIonMassesDecoy(int slot) const { assert(slot >= 0 && slot < _nSlots); return _poolBinnedIonMassesDecoy[slot]; }
   unsigned int* binnedPrecursorNLDecoy(int slot) const { assert(slot >= 0 && slot < _nSlots); return _poolBinnedPrecursorNLDecoy[slot]; }

   bool isAllocated() const { return _allocated; }
   int  slotCount()   const { return _nSlots; }

private:
   void _deallocate(int nSlots);

   int    _nSlots    = 0;
   bool** _pool      = nullptr;   // [_nSlots][iArraySize]: scratch buffers
   bool   _allocated = false;

   // Per-slot PI_DB scoring scratch, sized once at allocate() time instead of being
   // re-declared (and re-zeroed in full) on the stack for every scored candidate.
   unsigned int** _poolBinnedIonMasses        = nullptr;   // [_nSlots][nBinnedIonMassesElems]
   unsigned int** _poolBinnedPrecursorNL      = nullptr;   // [_nSlots][nBinnedPrecursorNLElems]
   unsigned int** _poolBinnedIonMassesDecoy   = nullptr;   // [_nSlots][nBinnedIonMassesElems]
   unsigned int** _poolBinnedPrecursorNLDecoy = nullptr;   // [_nSlots][nBinnedPrecursorNLElems]

   // Stack of currently-free slot indices.  A slot's presence here (rather than a
   // separate bool[] scanned linearly) is the sole source of truth for "is free",
   // so acquire/release are O(1) instead of O(nSlots) regardless of pool size.
   std::vector<int>        _freeSlots;
   std::mutex              _mutex;
   std::condition_variable _cv;
};

// RAII guard for a slot acquired via SearchMemoryPool::acquireSlot().  Releases the
// slot on scope exit (normal return or exception unwind) so a throw out of the
// search body never leaks the slot and stalls the next acquireSlot() caller for up
// to 240 s.  Construct only after checking the acquired slot is >= 0.
struct SearchMemoryPoolSlotGuard
{
   SearchMemoryPool& pool;
   int               slot;
   ~SearchMemoryPoolSlotGuard() { if (slot >= 0) pool.releaseSlot(slot); }
};

#endif // _SEARCHMEMORYPOOL_H_
