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

// Owns all mutable state for one batch search run.
// Created at the top of CometSearchManager::DoSearch() per input-file iteration.
// Passed by reference to pipeline functions that read or write per-run state.
//
// Read-only index globals (g_iFragmentIndex, g_vFragmentPeptides, g_vRawPeptides,
// g_vSpecLib, g_pvProteinsList, g_pvProteinNameCache, g_pvDBIndex, …) are NOT moved
// here — they are large, initialised once, and shared read-only across all threads.
//
// Phase 4 migration note:
//   g_pvQueryMutex, g_bPlainPeptideIndexRead, g_bSpecLibRead, and g_cometStatus
//   remain as globals because they are also accessed from the RTS path
//   (InitializeSingleSpectrumSearch / DoSingleSpectrumSearchMultiResults), which
//   does not use SearchSession.  Full removal is deferred to Phase 5.

#ifndef _SEARCHSESSION_H_
#define _SEARCHSESSION_H_

#include "core/Params.h"
#include "core/Types.h"
#include "CometStatus.h"
#include <mutex>
#include <vector>

struct SearchSession
{
   // Run parameters — set once before the file loop, then read-only.
   const StaticParams& params;

   // Per-batch MS2 result accumulator.
   // Guarded by queriesMutex in the batch path.
   std::vector<Query*>    queries;

   // Per-batch MS1 result accumulator (batch path only).
   std::vector<QueryMS1*> ms1Queries;

   // Mutex protecting queries and ms1Queries during parallel spectrum loading.
   std::mutex             queriesMutex;

   // Run-time flags (replace the five batch-path-only globals).
   bool bPerformDatabaseSearch = false;
   bool bPerformSpecLibSearch  = false;
   bool bIdxNoFasta            = false;
   bool bPlainPeptideIndexRead = false;
   bool bSpecLibRead           = false;

   // Error / cancel state for this run.
   // g_cometStatus remains as a global for the RTS path (Phase 5 will unify).
   CometStatus status;

   explicit SearchSession(const StaticParams& p) : params(p) {}
   SearchSession(const SearchSession&)            = delete;
   SearchSession& operator=(const SearchSession&) = delete;
};

#endif // _SEARCHSESSION_H_