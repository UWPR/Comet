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


#ifndef _COMETFRAGMENTINDEX_H_
#define _COMETFRAGMENTINDEX_H_

#include "Common.h"
#include "CometSearch.h"
#include <functional>

class CometFragmentIndex
{
public:
   CometFragmentIndex();
   ~CometFragmentIndex();

   // WriteFIPlainPeptideIndex()/ReadPlainPeptideIndex() retired (docs/20260730_PI_reduction.md
   // Phase 0) -- superseded by CometPeptideIndex::WritePeptideIndex()/ReadPeptideIndex(),
   // which produce/consume the unified index format shared by PI_DB and FI_DB search modes.
   static bool GeneratePlainPeptideIndex(ThreadPool *tp, vector<pair<size_t,size_t>>& slices);

   static bool CreateFragmentIndex(ThreadPool *tp, bool bIsRTS);
   static int WhichPrecursorBin(double dMass);

   // Public for reuse by CometPeptideIndex (PI_DB build, see
   // docs/20260713_PIidxformat.md Phase B): builds MOD_SEQS/MOD_NUMBERS/
   // MOD_SEQ_MOD_NUM_START/CNT/PEPTIDE_MOD_SEQ_IDXS from g_vRawPeptides.
   static void PermuteIndexPeptideMods(vector<PlainPeptideIndexStruct>& vRawPeptides);

private:

   static bool GenerateFragmentIndex(ThreadPool *tp);

   // P1: destination for a qualifying b/y ion (or, for the count pass, a qualifying
   // peptide variant) is selected by exactly one of the three trailing pointer
   // parameters being non-null -- see the three callers below for which one each
   // pass/sub-pass supplies:
   //   pLocalFragPeptides non-null (count pass): push the accepted variant onto this
   //     thread's own local vector (no lock -- each thread owns a disjoint vector,
   //     concatenated in partition order by the caller after wait_on_threads() so the
   //     result is byte-identical to the old single-threaded traversal order); bin
   //     counts go into the shared g_iFragmentIndexOffset via std::atomic_ref (safe:
   //     the final per-bin count is a commutative sum, so increment order doesn't
   //     affect the result or determinism).
   //   pFillBinCounts non-null (fill-count sub-pass): accumulate into this thread's own
   //     local per-bin counter array (no atomics needed -- exclusively owned by the
   //     calling partition).
   //   pFillWriteCursor non-null (fill-write sub-pass): write into g_iFragmentIndex at
   //     this thread's own local per-bin cursor (pre-seeded by the caller from a
   //     prefix sum over pFillBinCounts across partitions in a fixed partition order --
   //     see GenerateFragmentIndex() -- so the physical write ranges per bin are
   //     disjoint across threads and the relative order within a bin is deterministic
   //     regardless of actual thread scheduling, preserving T18's byte-identical-build
   //     guarantee).
   // Count-pass vs. fill-pass mode is derived inside the function from which destination
   // pointer is non-null -- see the definition -- so there is no separate bCountOnly flag:
   // pLocalFragPeptides non-null selects the count pass; null selects a fill sub-pass,
   // where pFillWriteCursor non-null vs. null further selects fill-write vs. fill-count.
   static void AddFragments(vector<PlainPeptideIndexStruct>& vRawPeptides,
                            size_t iWhichPeptide,
                            size_t iWhichFragmentPeptide,
                            int modNumIdx,
                            char cNtermMod,
                            char cCtermMod,
                            const vector<int>& vModSlotForAllModsIdx,
                            double dKnownPepMass,
                            vector<FragmentPeptidesStruct>* pLocalFragPeptides,
                            uint64_t* pFillBinCounts,
                            uint64_t* pFillWriteCursor);

   // Count pass, one raw-peptide index range per thread. Enumerates every mod
   // combination for peptides [iPeptideStart, iPeptideEnd) exactly as the old
   // single-threaded AddFragmentsThreadProc() did, but writes accepted variants into
   // this thread's own localFragPeptides instead of the (formerly mutex-guarded)
   // global g_vFragmentPeptides.
   static void AddFragmentsThreadProcRange(size_t iPeptideStart,
                                           size_t iPeptideEnd,
                                           vector<FragmentPeptidesStruct>& localFragPeptides);

   static bool *_pbSearchMemoryPool;    // Pool of memory to be shared by search threads
   static bool **_ppbDuplFragmentArr;   // Number of arrays equals number of threads
};

#endif // _COMETFRAGMENTINDEX_H_
