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

// Per-slot bump arena for the fused FI_DB/PI_DB batch path's sparse XCorr matrix
// child blocks (Query::ppfSparseSpScoreData / ppfSparseFastXcorrData /
// ppfSparseFastXcorrDataNL leaf blocks). See docs/20260723_ExtendFusedBatchPath.md
// for the full design rationale.
//
// One instance per worker slot, owned by SearchSession::sparseArenas (NOT
// thread_local -- see the design doc's "why the existing pool can't be reused
// unchanged" section for why keying by slot index instead of OS thread identity
// is required here). AllocBlock() is called only by the single worker task-closure
// that owns this slot for the current flush round -- exactly one writer at a time,
// no lock needed. ResetRound() is called once per round, from Pipeline.cpp's
// cleanupBatch lambda, only after tp->wait_on_threads() has returned -- i.e. only
// when no AllocBlock() call can possibly be in flight.

#ifndef _COMETFUSEDSPARSEARENA_H_
#define _COMETFUSEDSPARSEARENA_H_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>
#include "CometData.h"   // SPARSE_MATRIX_SIZE

struct FusedSparseArena
{
   // ~25 MB/chunk at SPARSE_MATRIX_SIZE=100 floats/block. Not measurement-tuned yet
   // (see docs/20260723_ExtendFusedBatchPath.md "Risks / open questions").
   static constexpr size_t kChunkBlocks = 65536;

   // Append-only: existing entries are never moved, resized, or freed once
   // allocated, so pointers handed out by AllocBlock() stay valid for the life of
   // the arena (i.e. across ResetRound() calls too -- ResetRound() only rewinds
   // the fill cursor, it does not release chunks).
   std::vector<std::unique_ptr<float[]>> vChunks;

   size_t iCurChunk     = 0;   // chunk currently being filled
   size_t iCurChunkUsed = 0;   // blocks used in that chunk so far this round

   // Returns a zeroed float[SPARSE_MATRIX_SIZE] block. Zero-on-issue preserves the
   // same pre-zeroed guarantee RtsScratch::AllocSparseChild() provides for the RTS
   // single-spectrum pool (CometPreprocess.cpp) -- several downstream sparse-matrix
   // consumers (e.g. CometPostAnalysis.cpp) implicitly rely on never-written bins
   // reading as zero.
   float* AllocBlock()
   {
      if (vChunks.empty() || iCurChunkUsed >= kChunkBlocks)
      {
         if (iCurChunk + 1 < vChunks.size())        // reuse a chunk grown in an earlier round
         {
            ++iCurChunk;
         }
         else
         {
            vChunks.push_back(std::make_unique<float[]>(kChunkBlocks * SPARSE_MATRIX_SIZE));
            iCurChunk = vChunks.size() - 1;
         }
         iCurChunkUsed = 0;
      }

      float* p = vChunks[iCurChunk].get() + iCurChunkUsed * SPARSE_MATRIX_SIZE;
      std::fill(p, p + SPARSE_MATRIX_SIZE, 0.0f);
      ++iCurChunkUsed;
      return p;
   }

   // Rewinds to the start of chunk 0 for the next round. All chunks stay allocated
   // (steady-state reuse across rounds) -- after the first several rounds this does
   // zero further heap traffic for sparse child blocks. Must only be called when no
   // AllocBlock() call for this slot can be in flight (see file header comment).
   void ResetRound()
   {
      iCurChunk     = 0;
      iCurChunkUsed = 0;
   }
};

#endif // _COMETFUSEDSPARSEARENA_H_
