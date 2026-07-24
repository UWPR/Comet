// Copyright 2026 Jimmy Eng
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

// Per-slot bump arena for the fused FI_DB/PI_DB batch path's per-spectrum
// heap allocations that are pure POD data (safe to bump-allocate and forget,
// unlike Results -- see core/FusedResultsArena.h for that case). See
// docs/20260723_ExtendFusedBatchPath.md (Phase 1 and Phase 2a) for the full
// design rationale.
//
// FusedBumpArena<T, kChunkBytes> is a template so the same append-only,
// never-move chunk-growth logic serves two purposes without a second,
// hand-duplicated copy of the same tricky invariants:
//   - FusedSparseArena (Phase 1): fixed-size float[SPARSE_MATRIX_SIZE] child
//     blocks for Query::ppfSparseSpScoreData/ppfSparseFastXcorrData/
//     ppfSparseFastXcorrDataNL, via AllocBlock().
//   - FusedPointerArena (Phase 2a): variable-length float* spans for the
//     outer ppfSparse*[...] pointer arrays themselves (length varies per
//     spectrum with the mass-dependent bin count), via AllocSpan(n).
//
// One instance per worker slot, owned by SearchSession::sparseArenas /
// pointerArenas (NOT thread_local -- see the design doc's "why the existing
// pool can't be reused unchanged" section for why keying by slot index
// instead of OS thread identity is required here). AllocBlock()/AllocSpan()
// are called only by the single worker task-closure that owns this slot for
// the current flush round -- exactly one writer at a time, no lock needed.
// ResetRound() is called once per round, from Pipeline.cpp's cleanupBatch
// lambda, only after tp->wait_on_threads() has returned -- i.e. only when no
// AllocBlock()/AllocSpan() call can possibly be in flight.

#ifndef _COMETFUSEDSPARSEARENA_H_
#define _COMETFUSEDSPARSEARENA_H_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>
#include "CometData.h"   // SPARSE_MATRIX_SIZE

template<typename T, size_t kChunkBytes>
struct FusedBumpArena
{
   // Target element count per chunk, derived from a target byte budget so the
   // same template reads naturally for both float (Phase 1, 4 bytes/element)
   // and float* (Phase 2a, 8 bytes/element on x64) instantiations. Not
   // measurement-tuned -- see docs/20260723_ExtendFusedBatchPath.md "Risks".
   static constexpr size_t kChunkElements =
      (kChunkBytes / sizeof(T)) > 0 ? (kChunkBytes / sizeof(T)) : 1;

   struct ChunkSlot
   {
      std::unique_ptr<T[]> data;
      size_t               capacity;   // usually kChunkElements; larger only for an
                                        // oversized single request (see AllocSpan).
   };

   // Append-only: existing entries are never moved, resized, or freed once
   // allocated, so pointers/spans handed out by AllocSpan()/AllocBlock() stay
   // valid for the life of the arena (i.e. across ResetRound() calls too --
   // ResetRound() only rewinds the fill cursor, it does not release chunks).
   std::vector<ChunkSlot> vChunks;

   size_t iCurChunk     = 0;   // chunk currently being filled
   size_t iCurChunkUsed = 0;   // elements used in that chunk so far this round

   // Returns a zeroed (or null-initialised, for T=float*), contiguous span of
   // n elements. If a single request exceeds the normal per-chunk element
   // count, a one-off, exactly-sized chunk is allocated for it instead of
   // risking any possibility of overflowing a fixed-size chunk -- this
   // guarantees correctness regardless of how large iArraySizeGlobal-derived
   // sizes turn out to be for a given comet.params, rather than relying on a
   // compile-time upper bound.
   T* AllocSpan(size_t n)
   {
      if (vChunks.empty() || iCurChunkUsed + n > vChunks[iCurChunk].capacity)
      {
         if (iCurChunk + 1 < vChunks.size() && n <= vChunks[iCurChunk + 1].capacity)
         {
            ++iCurChunk;                              // reuse a chunk grown in an earlier round
         }
         else
         {
            const size_t cap = std::max(kChunkElements, n);
            ChunkSlot slot{ std::unique_ptr<T[]>(new T[cap]), cap };
            vChunks.push_back(std::move(slot));
            iCurChunk = vChunks.size() - 1;
         }
         iCurChunkUsed = 0;
      }

      T* p = vChunks[iCurChunk].data.get() + iCurChunkUsed;
      std::fill(p, p + n, T());
      iCurChunkUsed += n;
      return p;
   }

   // Fixed-size convenience wrapper -- Phase 1's original usage.
   T* AllocBlock() { return AllocSpan(SPARSE_MATRIX_SIZE); }

   // Rewinds to the start of chunk 0 for the next round. All chunks stay
   // allocated (steady-state reuse across rounds) -- after the first several
   // rounds this does zero further heap traffic. Must only be called when no
   // AllocSpan()/AllocBlock() call for this slot can be in flight (see file
   // header comment).
   void ResetRound()
   {
      iCurChunk     = 0;
      iCurChunkUsed = 0;
   }
};

// Phase 1: float[SPARSE_MATRIX_SIZE] sparse-matrix child blocks. 65536 blocks/
// chunk * 100 floats/block * 4 bytes/float = ~25 MB/chunk, matching the
// original (pre-generalization) FusedSparseArena sizing exactly.
using FusedSparseArena = FusedBumpArena<float, 65536u * SPARSE_MATRIX_SIZE * sizeof(float)>;

// Phase 2a: variable-length float* spans for the outer sparse pointer arrays.
// ~2 MB/chunk (256K pointers on x64) -- not measurement-tuned.
using FusedPointerArena = FusedBumpArena<float*, 2u * 1024u * 1024u>;

#endif // _COMETFUSEDSPARSEARENA_H_
