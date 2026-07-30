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

   // Phase B (docs/20260713_PIidxformat.md, docs/20260730_PI_reduction.md Phase 1): walks
   // g_vRawPeptides x valid mod combinations (mirroring
   // CometFragmentIndex::AddFragmentsThreadProc()'s enumeration structure) and appends a
   // compact FragmentPeptidesStruct reference {iWhichPeptide, modNumIdx, cNtermMod, cCtermMod,
   // dPepMass} per valid combination, using the combinatorics tables built by a prior call to
   // CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides). Does not include the
   // fully-unmodified variant for each raw peptide -- see WritePeptideIndex() for that.
   static bool EnumerateIndexPeptideMods(vector<FragmentPeptidesStruct>& vVariants);

   // Single-entry version of EnumerateIndexPeptideMods()'s tryPush lambda,
   // factored out so it can be called per-candidate at search time (see
   // docs/20260730_PI_reduction.md Phase 3), not just once-per-peptide at build
   // time. Reconstructs a full DBIndex (sequence, explicit pcVarModSites, mass,
   // flank AAs, protein reference) from a compact (iWhichPeptide, modNumIdx,
   // cNtermMod, cCtermMod) reference into g_vRawPeptides, using the
   // MOD_NUMBERS/MOD_SEQS/PEPTIDE_MOD_SEQ_IDXS tables built by a prior call to
   // CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides). modNumIdx == -1
   // means "no body modification" (only possibly cNtermMod/cCtermMod);
   // cNtermMod/cCtermMod == -1 means "no terminal modification". Returns false
   // only if a mod-site encoding would exceed VarModSites::MAX_SITES -- should
   // not happen in practice since Phase 1's build-time enumeration already
   // validated every (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuple it
   // wrote to the compact array.
   static bool MaterializeOneEntry(size_t iWhichPeptide, int modNumIdx, char cNtermMod,
      char cCtermMod, DBIndex& out);



   // Parses the .idx text header (MassType, StaticMod, DecoySearch, Enzyme,
   // Enzyme2, VariableMod lines) from an already-open file pointer.
   // Updates g_staticParams in-place and must only be called once per index
   // load (guarded by g_bPeptideIndexRead). Called by both
   // SearchPeptideIndex(ThreadPool*) and InitializeMassesFromPeptideIndex()
   // to avoid duplication.
   static bool ParsePeptideIndexHeader(FILE* fp);

};

#endif // _COMETPEPTIDEINDEX_H_
