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

// Per-slot checkout-and-reset-in-place pool for the fused FI_DB/PI_DB batch
// path's _pResults/_pDecoys (Results[iNumStored] arrays). See
// docs/20260723_ExtendFusedBatchPath.md, Phase 2b for the full design
// rationale.
//
// Unlike FusedSparseArena (a pure bump-and-forget arena, safe because its
// blocks are plain float[SPARSE_MATRIX_SIZE]), Results has non-trivial
// members (std::string, std::vector<ProteinEntryStruct>) that would leak
// their own backing heap allocations if a chunk's memory were ever reused
// without running their destructors. So this arena's chunks, once allocated,
// are NEVER destroyed and their Results objects are NEVER re-constructed --
// only reset in place via ResetOneResult() (core/Types.h), the same
// technique RtsScratch already uses for the RTS single-spectrum pool. This
// also means a slot reused across many rounds sees its pWhichProtein/
// site-score-string capacities stabilize over time instead of starting at
// zero every spectrum, since .clear() does not release capacity.
//
// One instance per worker slot, owned by SearchSession::resultsArenas (see
// that struct for why NOT thread_local). AllocResultsPair() is called only
// by the single worker task-closure that owns this slot for the current
// flush round -- exactly one writer at a time, no lock needed. ResetRound()
// is called once per round, from Pipeline.cpp's cleanupBatch lambda, only
// after tp->wait_on_threads() has returned -- i.e. only when no
// AllocResultsPair() call can possibly be in flight.

#ifndef _COMETFUSEDRESULTSARENA_H_
#define _COMETFUSEDRESULTSARENA_H_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>
#include "core/Types.h"   // Results, ResetOneResult

struct FusedResultsArena
{
   // Target bytes per chunk. Unlike FusedSparseArena's fixed-size blocks,
   // a Results[iNumStored] "block" scales with the run's iNumStored (1 to
   // 100+ depending on comet.params' num_output_lines), so spectra-per-chunk
   // is derived from this byte budget at first use rather than fixed --
   // see docs/20260723_ExtendFusedBatchPath.md Phase 2 "Risks".
   static constexpr size_t kChunkByteBudget = 32u * 1024u * 1024u;   // ~32 MB/chunk

   struct ResultsPair
   {
      Results* pResults;
      Results* pDecoys;   // nullptr when decoys are not requested
   };

   struct Chunk
   {
      std::unique_ptr<Results[]> results;   // iSpectraPerChunk * iNumStored entries
      std::unique_ptr<Results[]> decoys;    // same shape; null if !bWantDecoys
   };

   std::vector<Chunk> vChunks;      // append-only; existing entries never move/free
   size_t iSpectraPerChunk = 0;     // 0 = not yet initialised
   int    iNumStored       = 0;
   bool   bWantDecoys      = false;

   size_t iCurChunk     = 0;   // chunk currently being filled
   size_t iCurChunkUsed = 0;   // spectra (not Results elements) used in that chunk this round

   // Called once on first use for this arena. iNumStored/bWantDecoysIn are
   // assumed constant for the SearchSession's lifetime (comet.params is not
   // re-read mid-run), mirroring FusedLoadAndSearchSpectra's similar
   // assumption that iNumThreads does not change mid-run.
   void EnsureInitialized(int iNumStoredIn, bool bWantDecoysIn)
   {
      if (iSpectraPerChunk != 0)
         return;

      iNumStored  = iNumStoredIn;
      bWantDecoys = bWantDecoysIn;

      const size_t bytesPerSpectrum =
         sizeof(Results) * (size_t)iNumStored * (bWantDecoys ? 2 : 1);
      iSpectraPerChunk = std::max<size_t>(1, kChunkByteBudget / std::max<size_t>(bytesPerSpectrum, 1));
   }

   // Returns one spectrum's worth of freshly-reset Results storage. Must not
   // be called before EnsureInitialized().
   ResultsPair AllocResultsPair()
   {
      if (vChunks.empty() || iCurChunkUsed >= iSpectraPerChunk)
      {
         if (iCurChunk + 1 < vChunks.size())        // reuse a chunk grown in an earlier round
         {
            ++iCurChunk;
         }
         else
         {
            Chunk c;
            c.results.reset(new Results[iSpectraPerChunk * (size_t)iNumStored]);
            if (bWantDecoys)
               c.decoys.reset(new Results[iSpectraPerChunk * (size_t)iNumStored]);
            vChunks.push_back(std::move(c));
            iCurChunk = vChunks.size() - 1;
         }
         iCurChunkUsed = 0;
      }

      Results* pResults = vChunks[iCurChunk].results.get() + iCurChunkUsed * (size_t)iNumStored;
      Results* pDecoys  = bWantDecoys
         ? vChunks[iCurChunk].decoys.get() + iCurChunkUsed * (size_t)iNumStored
         : nullptr;
      ++iCurChunkUsed;

      for (int j = 0; j < iNumStored; ++j)
         ResetOneResult(pResults[j]);
      if (pDecoys != nullptr)
      {
         for (int j = 0; j < iNumStored; ++j)
            ResetOneResult(pDecoys[j]);
      }

      return ResultsPair{ pResults, pDecoys };
   }

   // Rewinds to the start of chunk 0 for the next round. All chunks (and the
   // Results objects within them) stay allocated and constructed -- steady-
   // state reuse across rounds -- so after the first several rounds this
   // does zero further heap traffic. Must only be called when no
   // AllocResultsPair() call for this slot can be in flight (see file header
   // comment).
   void ResetRound()
   {
      iCurChunk     = 0;
      iCurChunkUsed = 0;
   }
};

#endif // _COMETFUSEDRESULTSARENA_H_
