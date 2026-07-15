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


#ifndef _COMETPEPTIDEINDEX_H_
#define _COMETPEPTIDEINDEX_H_

#include "Common.h"
#include "CometDataInternal.h"
#include "CometFragmentIndex.h"
#include "CometMassSpecUtils.h"
#include "CometSearch.h"
#include "CometStatus.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

class CometPeptideIndex
{
public:
   CometPeptideIndex();
   ~CometPeptideIndex();

   // bIsRTS: true if called from the RTS single-spectrum-search init path
   // (InitializeSingleSpectrumSearch()), false if called from the batch
   // search path (via CometSearch::EnsurePeptideIndexLoaded()). Reserved for
   // RTS-vs-batch-specific behavior (e.g. logging); no such behavior exists yet.
   static bool ReadPeptideIndex(bool bIsRTS);
   static bool WritePeptideIndex(ThreadPool* tp);
   static bool ReadPeptideIndexEntry(struct DBIndex* sDBI, FILE* fp);

   // Phase B (docs/20260713_PIidxformat.md): walks g_vRawPeptides x valid mod
   // combinations (mirroring CometFragmentIndex::AddFragmentsThreadProc()'s
   // enumeration structure) and materializes full DBIndex entries with explicit
   // pcVarModSites, using the combinatorics tables built by a prior call to
   // CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides). Appends results
   // to vModifiedEntries; does not touch g_pvDBIndex itself.
   static bool MaterializeIndexPeptideMods(vector<DBIndex>& vModifiedEntries);



   // Parses the .idx text header (MassType, StaticMod, DecoySearch, Enzyme,
   // Enzyme2, VariableMod lines) from an already-open file pointer.
   // Updates g_staticParams in-place and must only be called once per index
   // load (guarded by g_bPeptideIndexRead). Called by both
   // SearchPeptideIndex(ThreadPool*) and InitializeMassesFromPeptideIndex()
   // to avoid duplication.
   static bool ParsePeptideIndexHeader(FILE* fp);

};

#endif // _COMETPEPTIDEINDEX_H_
