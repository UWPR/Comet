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
// g_vSpecLib, g_pvProteinsList, g_pvProteinNameCache, g_pvDBIndex, ...) are NOT moved
// here -- they are large, initialised once, and shared read-only across all threads.
//
// g_pvQueryMutex, g_bPlainPeptideIndexRead, and g_bSpecLibRead remain as globals,
// not SearchSession members, and this is permanent rather than a pending migration
// step: they are also read/written by the RTS path (InitializeSingleSpectrumSearch /
// DoSingleSpectrumSearchMultiResults), which is intentionally not moved into the
// strategy/Pipeline pattern (see docs/20260612_architecture_migration.md, "RTS path" --
// the RTS entry points are wrapper-compatibility-sensitive and out of scope for the
// migration). Since a single process can serve both RTS and batch requests, this
// once-per-process init state must stay process-global so both paths observe the same
// value; it cannot move into a per-batch-run SearchSession. SearchSession does not
// shadow these globals; all code reads the globals directly.
//
// g_cometStatus is exposed here as statusRef: a reference to the process-wide
// singleton. Pipeline and strategy code use session.statusRef so they are not
// coupled to the global name; deep core files (CometSearch.cpp, CometPreprocess.cpp,
// etc.) still reference g_cometStatus directly because they have no SearchSession
// in scope. Both spellings touch the same object.

#ifndef _SEARCHSESSION_H_
#define _SEARCHSESSION_H_

#include "core/Params.h"
#include "core/Types.h"
#include "CometStatus.h"
#include <mutex>
#include <vector>

struct SearchSession
{
   // Per-batch MS2 result accumulator.
   // On the fused FI_DB batch path (FusedLoadAndSearchSpectra), workers append to
   // their own per-slot vectors lock-free and this is filled by a single serial
   // concatenation after the thread-pool join -- not mutex-guarded there.  Other
   // batch paths (LoadAndPreprocessSpectra, PreprocessSingleSpectrum, etc.) still
   // guard direct pushes to this vector with queriesMutex.
   std::vector<Query*>    queries;

   // Per-batch MS1 result accumulator (batch path only).
   std::vector<QueryMS1*> ms1Queries;

   // Mutex protecting queries and ms1Queries during parallel spectrum loading on
   // the non-fused batch paths.
   std::mutex             queriesMutex;

   // Run-time flags (replace the batch-path-only globals).
   bool bPerformDatabaseSearch = false;
   bool bPerformSpecLibSearch  = false;
   bool bIdxNoFasta            = false;

   // Reference to the process-wide status singleton (g_cometStatus).
   CometStatus& statusRef;

   explicit SearchSession(CometStatus& st) : statusRef(st) {}
   SearchSession(const SearchSession&)            = delete;
   SearchSession& operator=(const SearchSession&) = delete;
};

#endif // _SEARCHSESSION_H_
