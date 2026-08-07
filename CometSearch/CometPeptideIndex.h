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

   // Compacted list of active variable_modNN slot indices (0-based into
   // g_staticParams.variableModParameters.varModList), built in the same compaction order
   // CometFragmentIndex::PermuteIndexPeptideMods()'s ALL_MODS-building loop uses --
   // MOD_NUMBERS[].modifications[] values are indices into *this* compacted list, not direct
   // varModList slot indices, so both EnumerateIndexPeptideMods() (build time) and
   // MaterializeOneEntry() (search time, called per mass-window candidate) need the exact
   // same translation. Single shared implementation rather than two independently-maintained
   // copies of the same loop -- previously duplicated verbatim between the two with only a
   // comment asking future edits to keep them in sync, which a change to either copy alone
   // could silently violate.
   static const vector<int>& GetVModSlotForAllModsIdx();

   // Translates a single compacted variable-mod-slot index (as read from
   // MOD_NUMBERS[...].modifications[]) into the real varModList[] slot it refers to, via
   // GetVModSlotForAllModsIdx()'s translation table above. Returns -1, uniformly, for both
   // legitimate cases callers must treat as "no real slot here": compactedIdx == -1 (the
   // ordinary "not modified at this candidate position in this combination" sentinel) and
   // compactedIdx out of range for vModSlotForAllModsIdx (only reachable via a corrupt/
   // mismatched on-disk .idx, or the compaction order here and in
   // CometFragmentIndex::PermuteIndexPeptideMods()'s ALL_MODS-building loop falling out of
   // sync -- a logic bug, not user input). A prior version of this codebase had five near-
   // identical copies of this translate-or-skip logic hand-copied across CometPeptideIndex.cpp
   // and CometFragmentIndex.cpp/CometSearch.cpp, each with a different guard/bounds-checking
   // policy (guarded+.at(), guarded+unchecked[], unguarded+unchecked[]) -- that inconsistency
   // is exactly how one copy shipped with its -1 guard missing entirely. Single shared,
   // exception-free implementation now used everywhere instead.
   static int TranslateVarModSlot(const vector<int>& vModSlotForAllModsIdx, int compactedIdx);

   // Returns true if every candidate position actually modified in this combination (i.e.
   // mods[i] != -1, for i in [0, modStringLen)) translates to a slot allowed by
   // siVarModProteinFilter's bitmask -- the protein-level variable-mod restriction feature.
   // Shared by PI_DB's EnumerateIndexPeptideMods() and FI_DB's AddFragmentsThreadProc(), which
   // previously carried two independently-maintained copies of this exact check.
   static bool PassesVarModProteinFilter(const vector<int>& vModSlotForAllModsIdx,
      const char* mods, int modStringLen, unsigned short siVarModProteinFilter);

};

#endif // _COMETPEPTIDEINDEX_H_
