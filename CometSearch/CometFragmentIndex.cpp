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


#include "CometFragmentIndex.h"
#include "CometPeptideIndex.h"
#include "CometPredictedMask.h"
#include "CometIntensityStore.h"
#include "CometSearch.h"
#include "ThreadPool.h"
#include "CometStatus.h"
#include "CometMassSpecUtils.h"
#include "CometModificationsPermuter.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <bitset>
#include <limits>
#include <queue>

// std::atomic_ref (P1's lock-free relaxed fetch_add on plain, non-atomic
// g_iFragmentIndexOffset entries -- see the comment above GenerateFragmentIndex() for why
// this doesn't need a real atomic<uint64_t> array) needs library support beyond just
// -std=c++20: MSVC's STL and libstdc++ (GCC) both define it, but Apple Clang's libc++ on
// the macos-14 CI runner doesn't yet, and fails to compile "no member named 'atomic_ref'
// in namespace 'std'" even though the compiler itself is otherwise C++20-capable.
// __atomic_fetch_add is a GCC/Clang builtin (independent of the C++ standard library
// version), so it's available everywhere std::atomic_ref isn't.
#if defined(__cpp_lib_atomic_ref) && __cpp_lib_atomic_ref >= 201806L
#define FRAGINDEX_ATOMIC_FETCH_ADD(x) std::atomic_ref<uint64_t>(x).fetch_add(1, std::memory_order_relaxed)
#else
#define FRAGINDEX_ATOMIC_FETCH_ADD(x) __atomic_fetch_add(&(x), 1, __ATOMIC_RELAXED)
#endif


vector<char> MOD_NUMBERS_POOL;         // flat pool: every mod-combination entry's modifications[] array, concatenated (see core/Types.h)
uint64_t* MOD_SEQ_MOD_NUM_POOL_START;  // per modifiable sequence: offset in MOD_NUMBERS_POOL of its first entry
vector<char> MOD_SEQS_POOL;            // unique modifiable sequences, concatenated, no NUL terminators
vector<unsigned int> MOD_SEQS_OFFSET;  // GetNumModSeqs()+1 offsets into MOD_SEQS_POOL; [0] == 0
int* MOD_SEQ_MOD_NUM_START; // Start mod-combination entry index for a modifiable sequence; -1 if no modification numbers were generated
int* MOD_SEQ_MOD_NUM_CNT;   // Total modifications numbers for a modifiable sequence.
int* PEPTIDE_MOD_SEQ_IDXS;  // Index into the modifiable-sequence tables; -1 for peptides that have no modifiable amino acids; -2 if only terminal mods.
int MOD_NUM = 0;
size_t tTmp;



#ifdef _WIN32
#ifdef _WIN64
comet_fileoffset_t clSizeCometFileOffset = sizeof(comet_fileoffset_t);              //win64
#else
comet_fileoffset_t clSizeCometFileOffset = (long long)sizeof(comet_fileoffset_t);   //win32
#endif
#else
comet_fileoffset_t clSizeCometFileOffset = sizeof(comet_fileoffset_t);              //linux
#endif

// .idx binary I/O throughout CometPeptideIndex.cpp reads/writes size_t objects
// (tNumRaw, tNumProteinEntries, tNumProteins, ProteinsListCSR row sizes, etc.) using
// clSizeCometFileOffset (== sizeof(comet_fileoffset_t), always 8) as the byte count for
// both fread() and fwrite() -- e.g. fwrite(&tTmp, clSizeCometFileOffset, 1, fptr) where
// tTmp is size_t. That's only correct if sizeof(size_t) itself is also 8: on a
// hypothetical 32-bit build (sizeof(size_t) == 4), fwrite would read 4 bytes past the
// variable (heap/stack overread) and fread would write 4 bytes past it (memory
// corruption) instead of failing loudly. Comet has only ever shipped 64-bit builds, so
// this is unreachable today -- catch it at compile time if that ever changes, rather
// than relying on it staying unreachable by convention.
static_assert(sizeof(size_t) == sizeof(comet_fileoffset_t),
   "CometPeptideIndex.cpp's .idx binary I/O reads/writes size_t objects using "
   "clSizeCometFileOffset (sizeof(comet_fileoffset_t)) as the byte count; this build's "
   "sizeof(size_t) doesn't match, which would silently overread/overwrite past those "
   "variables during .idx read/write.");


CometFragmentIndex::CometFragmentIndex()
{
}

CometFragmentIndex::~CometFragmentIndex()
{
}


bool CometFragmentIndex::CreateFragmentIndex(ThreadPool *tp, bool bIsRTS)
{
   // Reads the shared unified .idx format (docs/20260730_PI_reduction.md Phase 0/0.5) --
   // g_vRawPeptides plus the MOD_SEQS_POOL/MOD_NUMBERS_POOL/etc. permutation tables that
   // GenerateFragmentIndex() (via AddFragmentsThreadProcRange()) reads directly below.
   // ReadPeptideIndex() itself calls CometFragmentIndex::PermuteIndexPeptideMods() once per
   // session to build those tables fresh from live comet.params (Phase 0.5 -- they're no
   // longer persisted in the .idx file), so no separate call is needed here. g_dbIndexVariants
   // also gets populated (by ReadPeptideIndex(), for PI_DB mode only) but is unused on this
   // FI_DB path.
   if (!g_bPlainPeptideIndexRead && !CometPeptideIndex::ReadPeptideIndex(bIsRTS))
      return false;   // ReadPeptideIndex() already reported the specific error

   // Phase 3 (docs/20260805_carafe.md Section 4.4/9): load the predicted-fragment mask, if
   // configured, once g_vRawPeptides/MOD_NUMBERS/MOD_SEQS are populated (needed for the .idx
   // fingerprint and VarModConfig checks) and before GenerateFragmentIndex() below reads it
   // per-variant. A no-op (returns true) when fragment_index_predicted_mask_file is unset.
   if (!CometPredictedMask::Load(g_staticParams.options.sFragIndexPredictedMaskFile))
      return false;   // CometPredictedMask::Load() already reported the specific error

   // vFragmentPeptides is vector of modified peptides
   // - raw peptide via iWhichPeptide referencing entry in g_vRawPeptides to access peptide and protein(s)
   // - modification encoding index
   // - modification mass

   // FragmentPeptidesStruct::iWhichPeptide is unsigned int (narrowed from size_t to shrink the
   // struct -- see AddFragments() below); g_vRawPeptides is now fully populated (either just read
   // via CometPeptideIndex::ReadPeptideIndex() above, or already built earlier in this process), so this is the
   // one place that covers every path into GenerateFragmentIndex() below.
   if (g_vRawPeptides.size() > (size_t)std::numeric_limits<unsigned int>::max())
   {
      string strErrorMsg = " Error - " + std::to_string(g_vRawPeptides.size())
         + " raw peptides exceeds the " + std::to_string(std::numeric_limits<unsigned int>::max())
         + " entries FragmentPeptidesStruct::iWhichPeptide (unsigned int) can address. Reduce the "
         + "database/digest size, or widen iWhichPeptide back to size_t if this database size is "
         + "intentional.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // CSR layout: allocate offset array now (size+1 for sentinel); flat data allocated after counting.
   g_iFragmentIndexOffset = new uint64_t[g_massRange.uiMaxFragmentArrayIndex + 1]();

   // generate the modified peptides to calculate the fragment index
   if (!GenerateFragmentIndex(tp))
      return false;   // GenerateFragmentIndex() (via the thread pool's error handler) already reported the specific error

   // AddFragments() (called from GenerateFragmentIndex() above) was the mask's only consumer --
   // free its lookup table now rather than holding it resident for the rest of the search (see
   // CometPredictedMask::FreeAfterIndexBuild()'s header comment for the ~GB-scale motivation and
   // the shared safety precondition with Load()'s one-shot guard).
   CometPredictedMask::FreeAfterIndexBuild();

   // Intensity score (docs/20260903_IntensityScore_design.md Section 2.3): load the predicted-
   // intensity file, if configured, and bind it to the now-final FI variant array so
   // XcorrScoreI() can look a candidate's record up by uiWhichVariant. No-op when unset.
   // Freed in FiStrategy::finalize().
   if (!CometIntensityStore::LoadAndBind(g_staticParams.options.sPredictedIntensityFile, g_fragmentPeptides))
      return false;   // LoadAndBind() already reported the specific error

   return true;
}


void CometFragmentIndex::PermuteIndexPeptideMods(const RawPeptideTable& g_vRawPeptides)
{
   vector<string> ALL_MODS; // An array of all the user specified amino acids that can be modified
   vector<int> vMaxNumVarModsPerMod;  // replciates iMaxNumVarModAAPerMod

   // Pre-computed bitmask combinations for peptides of length MAX_PEPTIDE_LEN with up
   // to FRAGINDEX_MAX_MODS_PER_MOD modified amino acids.

   // Maximum number of bits that can be set in a modifiable sequence for a given modification.
   // C(25, 5) = 53,130; C(25, 4) = 10,650; C(25, 3) = 2300.  This is more than FRAGINDEX_MAX_COMBINATIONS (65,534)

   // iMaxNumVariableMods is the maximum # of mods per any variable_modXX entry used in the bitmasks
   int iMaxNumVariableMods = 0;

   for (int i = 0; i < FRAGINDEX_VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
         && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='-'))
      {
         ALL_MODS.push_back(g_staticParams.variableModParameters.varModList[i].szVarModChar);
         vMaxNumVarModsPerMod.push_back(g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod);

         if (iMaxNumVariableMods < g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod)
            iMaxNumVariableMods = g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod;
      }
   }

   int MOD_CNT = (int)ALL_MODS.size();

   cout << " - mods: ";
   for (int i = 0; i < MOD_CNT; ++i)
   {
      if (i==0)
         cout << ALL_MODS[i];
      else
         cout << ", " << ALL_MODS[i];
   }
   cout << endl;

   unsigned long long* ALL_COMBINATIONS;
   int ALL_COMBINATION_CNT = 0;

   if (FRAGINDEX_MAX_MODS_PER_MOD < iMaxNumVariableMods)
      iMaxNumVariableMods = FRAGINDEX_MAX_MODS_PER_MOD;
   if (g_staticParams.variableModParameters.iMaxVarModPerPeptide < iMaxNumVariableMods)
      iMaxNumVariableMods = g_staticParams.variableModParameters.iMaxVarModPerPeptide;

   // Pre-compute the combinatorial bitmasks that specify the positions of a modified residue
   // iEnd is one larger than max peptide length
   ModificationsPermuter::initCombinations(g_staticParams.options.peptideLengthRange.iEnd, iMaxNumVariableMods,
         &ALL_COMBINATIONS, &ALL_COMBINATION_CNT);

   // Get the unique modifiable sequences from the peptides (fills the MOD_SEQS_POOL/
   // MOD_SEQS_OFFSET flat pool -- docs/20260827_PI_memory.md Phase 1)
   PEPTIDE_MOD_SEQ_IDXS = new int[g_vRawPeptides.size()];

   ModificationsPermuter::getModifiableSequences(g_vRawPeptides, PEPTIDE_MOD_SEQ_IDXS, ALL_MODS);

   // Get the modification combinations for each unique modifiable substring
   ModificationsPermuter::getModificationCombinations(vMaxNumVarModsPerMod, ALL_MODS,
         MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);
}


bool CometFragmentIndex::GenerateFragmentIndex(ThreadPool *tp)
{
   cout <<  " - generate fragment ion index\n"; fflush(stdout);

   auto tFIGlobalStartTime = chrono::steady_clock::now();

   ThreadPool *pFragmentIndexPool = tp;
   const int iNumThreads = g_staticParams.options.iNumThreads;

   // Fetched once for the whole build rather than once per AddFragments() call below (a
   // hot loop over every raw peptide/fragment-peptide variant, potentially millions of calls).
   const vector<int>& vModSlotForAllModsIdx = CometPeptideIndex::GetVModSlotForAllModsIdx();

   // P1: count and fill passes are each O(variants x peptide length) -- the dominant cost
   // of a large FI build -- and embarrassingly parallel: count partitions g_vRawPeptides by
   // peptide-index range, one thread per range, each accumulating into its own local
   // FragmentPeptidesStruct vector (no lock) with bin counts folded into the shared
   // g_iFragmentIndexOffset via std::atomic_ref (safe -- a commutative sum, order doesn't
   // matter); fill partitions the now mass-sorted g_vFragmentPeptides the same way, using a
   // fill-count sub-pass to give each partition its own disjoint, deterministically-ordered
   // write-cursor range per bin (see AddFragments()'s doc comment in the header for the full
   // per-sub-pass destination-pointer contract) before the fill-write sub-pass actually
   // populates g_iFragmentIndex -- so no partition ever needs a lock or an atomic RMW on the
   // fragment-index array itself, and the result is byte-identical to the old single-threaded
   // traversal (T18's build-determinism guarantee), just computed on iNumThreads threads.

   cout <<  "   - store peptide list and reserve memory ... "; fflush(stdout);
   auto tStartTime = chrono::steady_clock::now();

   const size_t iNumRawPeptides = g_vRawPeptides.size();
   vector<pair<size_t,size_t>> vCountRanges(iNumThreads);
   vector<vector<FragmentPeptidesStruct>> vLocalFragPeptides(iNumThreads);
   for (int t = 0; t < iNumThreads; ++t)
   {
      vCountRanges[t].first  = t * iNumRawPeptides / iNumThreads;
      vCountRanges[t].second = (t + 1) * iNumRawPeptides / iNumThreads;
   }
   for (int t = 0; t < iNumThreads; ++t)
   {
      pFragmentIndexPool->doJob([t, &vCountRanges, &vLocalFragPeptides]()
      {
         AddFragmentsThreadProcRange(vCountRanges[t].first, vCountRanges[t].second, vLocalFragPeptides[t]);
      });
   }
   pFragmentIndexPool->wait_on_threads();

   // ThreadPool::doJob() catches a worker exception (e.g. bad_alloc partway through a
   // partition) and only reports it through the error handler wired in
   // CometSearchManager's constructor, which sets g_cometStatus -- it does not stop the
   // other queued partitions or this function. Left unchecked, a partial/empty
   // vLocalFragPeptides[t] here would silently concatenate into g_vFragmentPeptides as if
   // every partition had succeeded, and every later pass (mass sort, fill-count,
   // fill-write) would build on top of that corrupt data and still return true.
   if (g_cometStatus.IsError() || g_cometStatus.IsCancel())
      return false;

   // Concatenate in ascending partition order (== ascending raw-peptide-index order), the
   // exact traversal order the old single-threaded AddFragmentsThreadProc() produced, so the
   // subsequent mass sort below sees byte-identical input to before. Since the FI_DB Phase 2
   // port (docs/20260827_PI_memory.md Section 7.1), this concatenated 24B/entry AoS is a
   // page-granular STAGING buffer, not g_fragmentPeptides itself: it feeds the mass sort and
   // both posting-list fill sub-passes below (which need the exact double masses), then gets
   // transcoded into the 13B/entry SoA and released, page by page, as the transcode walks.
   size_t tNumFragPeptides = 0;
   FragmentPeptidesStruct* pStaging = NULL;
   {
      size_t iTotal = 0;
      for (auto& v : vLocalFragPeptides)
         iTotal += v.size();

      if (iTotal >= UINT_MAX)
      {
         printf(" Error in CometFragmentIndex; UINT_MAX (%d) peptides reached.\n", UINT_MAX);
         exit(1);
      }

      tNumFragPeptides = iTotal;
      if (tNumFragPeptides > 0)
      {
         pStaging = (FragmentPeptidesStruct*)CometPeptideIndex::AllocStagingPages(
            tNumFragPeptides * sizeof(FragmentPeptidesStruct));
         if (pStaging == NULL)
         {
            string strErrorMsg = " Error - cannot allocate the fragment-index variant staging buffer.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         size_t tCursor = 0;
         for (auto& v : vLocalFragPeptides)
         {
            if (!v.empty())
            {
               memcpy(pStaging + tCursor, v.data(), v.size() * sizeof(FragmentPeptidesStruct));
               tCursor += v.size();
            }
            vector<FragmentPeptidesStruct>().swap(v);
         }
      }
      vector<vector<FragmentPeptidesStruct>>().swap(vLocalFragPeptides);
   }

   // Convert per-bin counts (accumulated into g_iFragmentIndexOffset[0..n-1] during the
   // count pass above) to CSR prefix-sum offsets. Use uint64_t accumulator: non-enzymatic
   // searches against large databases can exceed UINT_MAX total entries, silently corrupting
   // the index with unsigned int. Deliberately does NOT allocate g_iFragmentIndex itself yet
   // (see the mask-caching block below, right before the fill-count sub-pass, for why that
   // allocation is now split out and moved later) -- this loop only touches
   // g_iFragmentIndexOffset, which at uiMaxFragmentArrayIndex+1 entries is a few hundred KB
   // even at whole-proteome scale, nowhere near big enough to matter for that ordering.
   uint64_t uiTotal = 0;
   for (unsigned int iMass = 0; iMass < g_massRange.uiMaxFragmentArrayIndex; ++iMass)
   {
      uint64_t uiCnt = g_iFragmentIndexOffset[iMass];
      g_iFragmentIndexOffset[iMass] = uiTotal;
      uiTotal += uiCnt;
   }
   g_iFragmentIndexOffset[g_massRange.uiMaxFragmentArrayIndex] = uiTotal;  // sentinel

   cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

   // now sort the staging array by mass; this was filled in the above count pass. Same
   // element type, comparator, and input order as the pre-SoA vector sort (raw pointers
   // resolve to the same std::sort instantiation), so the sorted order -- equal-mass ties
   // included, which determine the posting lists' index values -- is byte-identical.
   tStartTime = chrono::steady_clock::now();
   cout << "   - sort peptides by mass ... "; fflush(stdout);
   sort(pStaging, pStaging + tNumFragPeptides, [](const FragmentPeptidesStruct& a, const FragmentPeptidesStruct& b)
      {
         return a.dPepMass < b.dPepMass;
      });
   cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

   // In the for loop below, peptide references (iWhichFragmentPeptide) are stored in the FI.
   // As the FI is an array of unsigned int pointers, need to ensure that iWhichFragmentPeptide
   // will fit into an unsigned int. This is already guaranteed by the `iTotal >= UINT_MAX`
   // check above: g_vFragmentPeptides is populated with exactly iTotal elements (reserve()
   // + move-insert, nothing else appends to it) and then only reordered (mass sort), so
   // size() == iTotal here, and iTotal < UINT_MAX was already enforced -- a second,
   // differently-phrased check here would be unreachable dead code.

   // now populate the fragment index vector -- two parallel sub-passes over the same
   // mass-sorted-array partitioning (see the P1 comment above this function)
   tStartTime = chrono::steady_clock::now();
   cout <<  "   - populate index ... "; fflush(stdout);

   const size_t iNumFragPeptides = tNumFragPeptides;
   const uint64_t uiNumBins = g_massRange.uiMaxFragmentArrayIndex;
   vector<pair<size_t,size_t>> vFillRanges(iNumThreads);
   for (int t = 0; t < iNumThreads; ++t)
   {
      vFillRanges[t].first  = t * iNumFragPeptides / iNumThreads;
      vFillRanges[t].second = (t + 1) * iNumFragPeptides / iNumThreads;
   }

   // Cache each variant's predicted-fragment-mask decision ONCE here, in the final mass-sorted
   // index order the fill-count and fill-write sub-passes below both use -- then free the much
   // bigger, tuple-keyed CometPredictedMask::s_entries immediately, before either sub-pass (and
   // g_iFragmentIndex's own fill-write) actually run, instead of after both finish
   // (docs/20260822_carafe_prerun.md Section 7.6). Reuses vFillRanges' partitioning; no-op (and
   // no allocation) when masking isn't enabled. bUseFragmentNeutralLoss is a fixed, search-wide
   // setting (not per-variant), so it's safe to decide the cache's per-entry width once here --
   // when it's false, AddFragments() never reaches the modloss-masked insertion branch for ANY
   // variant, so ReserveCache() halves the cache by dropping bModlossMask/yModlossMask entirely
   // (docs/20260822_carafe_prerun.md Section 7.6.2).
   if (CometPredictedMask::IsEnabled())
   {
      cout << "   - cache predicted-fragment mask decisions ... "; fflush(stdout);
      auto tCacheStartTime = chrono::steady_clock::now();
      CometPredictedMask::ReserveCache(iNumFragPeptides, g_staticParams.variableModParameters.bUseFragmentNeutralLoss);
      for (int t = 0; t < iNumThreads; ++t)
      {
         pFragmentIndexPool->doJob([t, &vFillRanges, pStaging]()
         {
            for (size_t i = vFillRanges[t].first; i < vFillRanges[t].second; ++i)
            {
               // pStaging is already mass-sorted here, so index i is the variant's final
               // index-order position -- the same i AddFragments() stores in posting lists
               // and StoreCached() keys the decision cache by.
               const FragmentPeptidesStruct& fp = pStaging[i];
               uint64_t bMask = ~0ULL, yMask = ~0ULL, bModlossMask = ~0ULL, yModlossMask = ~0ULL;
               CometPredictedMask::Lookup(fp.iWhichPeptide, fp.modNumIdx, fp.cNtermMod, fp.cCtermMod,
                  bMask, yMask, bModlossMask, yModlossMask);
               CometPredictedMask::StoreCached(i, bMask, yMask, bModlossMask, yModlossMask);
            }
         });
      }
      pFragmentIndexPool->wait_on_threads();
      CometPredictedMask::FreeAfterIndexBuild();
      cout << CometMassSpecUtils::ElapsedTime(tCacheStartTime) << endl;
   }

   // Allocate the CSR flat data array only now, after the mask cache above has been built and
   // CometPredictedMask::s_entries freed (when masking is enabled) -- deferred from right after
   // the CSR-offset conversion (this function's very first block) specifically so this
   // allocation's own memory commit and CometPredictedMask::s_entries' resident lifetime never
   // overlap (docs/20260822_carafe_prerun.md Section 7.6): allocating it earlier, while
   // s_entries was still alive, defeated the whole point of freeing s_entries before the fill
   // sub-passes -- the two were simultaneously resident regardless of when s_entries was freed,
   // since the peak had already been set the moment this array's pages were touched/committed.
   g_iFragmentIndex = new unsigned int[uiTotal];

   // Fill-count sub-pass: each thread re-walks its own partition's already-known
   // (peptide, mods, mass) entries -- no recomputation of dPepMass itself (P2's
   // dKnownPepMass short-circuit still applies) -- purely to learn how many b/y ions
   // its partition contributes to each bin.
   vector<vector<uint64_t>> vPartitionBinCounts(iNumThreads, vector<uint64_t>(uiNumBins, 0));
   for (int t = 0; t < iNumThreads; ++t)
   {
      pFragmentIndexPool->doJob([t, &vFillRanges, &vPartitionBinCounts, &vModSlotForAllModsIdx, pStaging]()
      {
         uint64_t* pLocalCounts = vPartitionBinCounts[t].data();
         for (size_t i = vFillRanges[t].first; i < vFillRanges[t].second; ++i)
         {
            const FragmentPeptidesStruct& fp = pStaging[i];
            AddFragments(g_vRawPeptides, fp.iWhichPeptide, i, fp.modNumIdx, fp.cNtermMod, fp.cCtermMod,
               vModSlotForAllModsIdx, fp.dPepMass, nullptr, pLocalCounts, nullptr);
         }
      });
   }
   pFragmentIndexPool->wait_on_threads();

   if (g_cometStatus.IsError() || g_cometStatus.IsCancel())
   {
      CometPeptideIndex::FreeStagingPages(pStaging, tNumFragPeptides * sizeof(FragmentPeptidesStruct));
      return false;
   }

   // Prefix sum across partitions (in the same fixed partition order used above), per bin:
   // the write-cursor partition t should start at is the bin's base CSR offset plus every
   // earlier partition's contribution to that bin. This gives every partition a disjoint
   // sub-range within each bin, and the relative order across partitions is fixed
   // (partition 0's entries first, then partition 1's, ...) regardless of actual thread
   // scheduling -- the same order the old single-threaded fill loop produced, since it also
   // processed iWhichFragmentPeptide in ascending (== partition-ascending) order.
   vector<vector<uint64_t>> vPartitionWriteCursor(iNumThreads, vector<uint64_t>(uiNumBins));
   for (uint64_t bin = 0; bin < uiNumBins; ++bin)
   {
      uint64_t cursor = g_iFragmentIndexOffset[bin];
      for (int t = 0; t < iNumThreads; ++t)
      {
         vPartitionWriteCursor[t][bin] = cursor;
         cursor += vPartitionBinCounts[t][bin];
      }
   }
   vector<vector<uint64_t>>().swap(vPartitionBinCounts);

   // Fill-write sub-pass: each thread re-walks the same partition a second time, this time
   // actually writing, using only its own local write-cursor array -- no shared cursor, so
   // no lock/atomic needed on g_iFragmentIndex itself.
   for (int t = 0; t < iNumThreads; ++t)
   {
      pFragmentIndexPool->doJob([t, &vFillRanges, &vPartitionWriteCursor, &vModSlotForAllModsIdx, pStaging]()
      {
         uint64_t* pLocalCursor = vPartitionWriteCursor[t].data();
         for (size_t i = vFillRanges[t].first; i < vFillRanges[t].second; ++i)
         {
            const FragmentPeptidesStruct& fp = pStaging[i];
            AddFragments(g_vRawPeptides, fp.iWhichPeptide, i, fp.modNumIdx, fp.cNtermMod, fp.cCtermMod,
               vModSlotForAllModsIdx, fp.dPepMass, nullptr, nullptr, pLocalCursor);
         }
      });
   }
   pFragmentIndexPool->wait_on_threads();

   if (g_cometStatus.IsError() || g_cometStatus.IsCancel())
   {
      CometPeptideIndex::FreeStagingPages(pStaging, tNumFragPeptides * sizeof(FragmentPeptidesStruct));
      return false;
   }

   cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

   // Hardening check (docs/20260805_carafe.md Section 7): every bin's final write cursor must
   // land EXACTLY on the next bin's CSR offset -- proving the fill-write sub-pass wrote
   // precisely as many entries into this bin as the fill-count sub-pass reserved for it, no
   // more (which would silently overflow into the next bin's region, corrupting unrelated
   // masses) and no fewer (which would leave stale/uninitialized entries in this bin's tail).
   // This is the single highest-severity correctness risk Phase 3's predicted-fragment masking
   // introduces (a count/fill mismatch doesn't crash cleanly), but the check itself is generic
   // -- it catches a count/fill divergence from ANY cause, not just masking, and costs
   // O(bins), negligible next to the O(peptides) fill pass it follows.
   // Under the P1 partitioned fill (see the comment above this function), the last partition
   // (iNumThreads-1) writes each bin's tail-most entries -- vPartitionWriteCursor[iNumThreads-1][bin]
   // was pre-seeded to that partition's start-of-bin offset and advanced in place by every
   // entry it wrote, so once every thread has finished it should equal the bin's end-of-range
   // CSR offset exactly.
   for (uint64_t iBin = 0; iBin < uiNumBins; ++iBin)
   {
      if (vPartitionWriteCursor[iNumThreads - 1][iBin] != g_iFragmentIndexOffset[iBin + 1])
      {
         string strErrorMsg = " Error - fragment index count/fill mismatch at bin " + std::to_string(iBin)
            + ": fill pass wrote to offset " + std::to_string(vPartitionWriteCursor[iNumThreads - 1][iBin])
            + " but count pass reserved up to " + std::to_string(g_iFragmentIndexOffset[iBin + 1])
            + " -- the index is corrupt. This should be unreachable (the mask check, if any, "
            + "sits before the bCountOnly branch so both passes see identical logic); please "
            + "report this.\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         exit(1);
      }
   }

   // Transcode the sorted staging into the 13B/entry SoA (docs/20260827_PI_memory.md
   // Section 7.1 -- the FI_DB port of PI_DB's Phase 2), releasing consumed staging pages
   // to the OS as the walk advances so the SoA's growth is offset by the staging's
   // shrinkage (reserve + push_back, NOT resize: see GenerateVariantArray()'s equivalent
   // comment). The posting-list fill passes above are done with the exact doubles by this
   // point; searches recompute any candidate's exact mass bit-identically via
   // ComputeIndexedPepMass().
   g_fragmentPeptides.clear();
   if (tNumFragPeptides > 0)
   {
      // The 4-byte fixed-point key must be able to represent the largest (last) mass.
      if (!(pStaging[tNumFragPeptides - 1].dPepMass * VariantArray::MASS_KEY_SCALE < 4294967295.0))
      {
         CometPeptideIndex::FreeStagingPages(pStaging, tNumFragPeptides * sizeof(FragmentPeptidesStruct));
         string strErrorMsg = " Error - peptide mass " + std::to_string(pStaging[tNumFragPeptides - 1].dPepMass)
            + " exceeds the variant mass-key range; reduce peptide_mass_range.\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }

      g_fragmentPeptides.vuiMassKey.reserve(tNumFragPeptides);
      g_fragmentPeptides.vuiWhichPeptide.reserve(tNumFragPeptides);
      g_fragmentPeptides.vuiModNumIdx.reserve(tNumFragPeptides);
      g_fragmentPeptides.vucTermMods.reserve(tNumFragPeptides);

      const size_t tChunkEntries = 4 * 1024 * 1024;  // release granularity: ~96 MB of staging
      size_t tReleasedBytes = 0;

      for (size_t i = 0; i < tNumFragPeptides; ++i)
      {
         const FragmentPeptidesStruct& s = pStaging[i];
         g_fragmentPeptides.vuiMassKey.push_back((unsigned int)llround(s.dPepMass * VariantArray::MASS_KEY_SCALE));
         g_fragmentPeptides.vuiWhichPeptide.push_back(s.iWhichPeptide);
         g_fragmentPeptides.vuiModNumIdx.push_back((s.modNumIdx < 0) ? 0xFFFFFFFFu : (unsigned int)s.modNumIdx);
         g_fragmentPeptides.vucTermMods.push_back((unsigned char)(((s.cNtermMod + 1) << 4) | (s.cCtermMod + 1)));

         if (((i + 1) % tChunkEntries) == 0)
         {
            size_t tConsumedBytes = (i + 1) * sizeof(FragmentPeptidesStruct);
            CometPeptideIndex::DecommitStagingRange(pStaging, tReleasedBytes, tConsumedBytes);
            tReleasedBytes = tConsumedBytes;
         }
      }
   }
   CometPeptideIndex::FreeStagingPages(pStaging, tNumFragPeptides * sizeof(FragmentPeptidesStruct));

   // Total entry count is the CSR sentinel value.
   unsigned long long ullCount = g_iFragmentIndexOffset[g_massRange.uiMaxFragmentArrayIndex];

   if (g_fragmentPeptides.size() > 1e6)
      printf("   - %0.3e total peptides, ", (double)g_fragmentPeptides.size());
   else
      printf("   - %zu total peptides, ", g_fragmentPeptides.size());
   if (ullCount > 1e6)
      printf("%0.3e FI entries", (double)ullCount);
   else
      printf("%llu FI entries", ullCount);

   cout << " ... " << CometMassSpecUtils::ElapsedTime(tFIGlobalStartTime) << endl;

   return true;
}


void CometFragmentIndex::AddFragmentsThreadProcRange(size_t iPeptideStart,
                                                      size_t iPeptideEnd,
                                                      vector<FragmentPeptidesStruct>& localFragPeptides)
{
   size_t iWhichFragmentPeptide = 0;  // unused here for counting only

   // mods[]/MOD_NUMBERS_POOL entry values are 0-based COMPACTED variable-mod-slot
   // indices requiring translation through this compacted-to-real-slot map before use as a
   // varModList index or an siVarModProteinFilter bit position (both real-slot-indexed) --
   // see AddFragments()'s own copy of this note for the full explanation. Fetched once here,
   // used below at the protein-mod-filter check (the one place in this function that still
   // needs it -- ctNtermMod/ctCtermMod elsewhere in this function are already raw slots).
   const vector<int>& vModSlotForAllModsIdx = CometPeptideIndex::GetVModSlotForAllModsIdx();

   // P1: this thread owns [iPeptideStart, iPeptideEnd) exclusively -- every accepted
   // variant goes into this thread's own localFragPeptides (see AddFragments()'s doc
   // comment), so there's no shared state here to race on.
   for (size_t iWhichPeptide = iPeptideStart; iWhichPeptide < iPeptideEnd; ++iWhichPeptide)
   {
      // AddFragments for unmodified peptide; only if no variable mods are required
      if (!g_staticParams.variableModParameters.iRequireVarMod)
         AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, -1, -1, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);

      // FIX: need to see if individual required varmods are met
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];

      // Possibly analyze peptides with a terminal mod and no variable mod on any residue
      if (g_staticParams.variableModParameters.bVarTermModSearch)
      {
         // Add any n-term variable mods
         for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
               && (!g_staticParams.variableModParameters.bVarModProteinFilter
                  || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)))
            {
               AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, ctNtermMod, -1, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);
            }
         }

         // Add any c-term variable mods
         for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
               && (!g_staticParams.variableModParameters.bVarModProteinFilter
                  || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod)))
            {
               AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, -1, ctCtermMod, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);
            }
         }

         // Now consider combinations of n-term and c-term variable mods
         for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
         {
            for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                  && g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                  && (!g_staticParams.variableModParameters.bVarModProteinFilter ||
                     (cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)
                        && cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod))))
               {
                  AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, ctNtermMod, ctCtermMod, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);
               }
            }
         }
      }

      if (modSeqIdx < 0)
      {
         // peptide is not modified, skip following permuting code
         continue;
      }

      int startIdx = MOD_SEQ_MOD_NUM_START[modSeqIdx];
      if (startIdx == -1)
         continue;

      int modNumCount = MOD_SEQ_MOD_NUM_CNT[modSeqIdx];

      for (int modNumIdx = startIdx; modNumIdx < startIdx + modNumCount; ++modNumIdx)
      {
         if (modNumIdx >= 0)
         {
            bool bPass = true;

            // if protein variable mod filter is applied, check mods[] against the peptide's
            // siVarModProteinFilter -- shared with CometPeptideIndex.cpp's EnumerateIndexPeptideMods(),
            // which previously carried its own independent (and briefly inconsistent -- see
            // CometPeptideIndex::PassesVarModProteinFilter()'s doc comment) copy of this exact check.
            if (g_staticParams.variableModParameters.bVarModProteinFilter)
            {
               int iModSeqLen;
               GetModSeq(modSeqIdx, iModSeqLen);
               bPass = CometPeptideIndex::PassesVarModProteinFilter(vModSlotForAllModsIdx,
                  GetModNumEntry(modNumIdx, modSeqIdx, iModSeqLen), iModSeqLen,
                  g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter);
            }

            if (bPass)
            {
               AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, -1, -1, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);

               if (g_staticParams.variableModParameters.bVarTermModSearch)
               {
                  // Add any n-term variable mods
                  for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
                  {
                     if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                        && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)))
                     {
                        AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, ctNtermMod, -1, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);
                     }
                  }

                  // Add any c-term variable mods
                  for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
                  {
                     if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                        && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod)))
                     {
                        AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, -1, ctCtermMod, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);
                     }
                  }

                  // Now consider combinations of n-term and c-term variable mods
                  for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
                  {
                     for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
                     {
                        if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                           && g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                           && (!g_staticParams.variableModParameters.bVarModProteinFilter ||
                              (cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)
                                 && cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod))))
                        {
                           AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, ctNtermMod, ctCtermMod, vModSlotForAllModsIdx, -1.0, &localFragPeptides, nullptr, nullptr);
                        }
                     }
                  }
               }
            }
         }
      }
   }
}


// See the declaration comment in CometFragmentIndex.h: AddFragments()'s residue-by-residue
// precursor-mass computation, factored out so FI_DB's search path can recompute a stored
// variant's mass bit-identically (docs/20260827_PI_memory.md Section 7.1).
double CometFragmentIndex::ComputeIndexedPepMass(size_t iWhichPeptide,
                                                 int modNumIdx,
                                                 char cNtermMod,
                                                 char cCtermMod,
                                                 const vector<int>& vModSlotForAllModsIdx,
                                                 double* pdResidueOnlyMass)
{
   const RawPeptideView raw = g_vRawPeptides.at(iWhichPeptide);
   const char* pszPeptide = raw.szPeptide;
   const int iEndPos = raw.iLen - 1;

   const char* mods = NULL;
   const char* pModSeq = NULL;
   int iModSeqLen = 0;
   if (modNumIdx >= 0)
   {
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
      pModSeq = GetModSeq(modSeqIdx, iModSeqLen);
      mods = GetModNumEntry(modNumIdx, modSeqIdx, iModSeqLen);
   }

   int j = 0;
   double dCalcPepMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm;
   double dResidueOnlyMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm;
   for (int i = 0; i <= iEndPos; ++i)
   {
      dCalcPepMass += g_staticParams.massUtility.pdAAMassFragment[(int)pszPeptide[i]];
      dResidueOnlyMass += g_staticParams.massUtility.pdAAMassFragment[(int)pszPeptide[i]];

      if (modNumIdx >= 0) // handle the variable mods if present on peptide
      {
         // j bound + compacted-slot translation: see AddFragments()'s fragment-ladder loop
         if (j < iModSeqLen && pszPeptide[i] == pModSeq[j])
         {
            int iSlot = CometPeptideIndex::TranslateVarModSlot(vModSlotForAllModsIdx, mods[j]);
            if (iSlot >= 0)
               dCalcPepMass += g_staticParams.variableModParameters.varModList[iSlot].dVarModMass;
            j++;
         }
      }
   }

   if (cNtermMod >= 0)  // if -1, unused
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
   if (cCtermMod >= 0)  // if -1, unused
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;

   if (pdResidueOnlyMass != NULL)
      *pdResidueOnlyMass = dResidueOnlyMass;

   return dCalcPepMass;
}


void CometFragmentIndex::AddFragments(const RawPeptideTable& g_vRawPeptides,
                                      size_t iWhichPeptide,
                                      size_t iWhichFragmentPeptide,
                                      int modNumIdx,
                                      char cNtermMod,
                                      char cCtermMod,
                                      const vector<int>& vModSlotForAllModsIdx,
                                      double dKnownPepMass,
                                      vector<FragmentPeptidesStruct>* pLocalFragPeptides,
                                      uint64_t* pFillBinCounts,
                                      uint64_t* pFillWriteCursor)
{
   // Count-pass vs. fill-pass mode is fully determined by which of the three mutually
   // exclusive destination pointers the caller supplied: count pass passes
   // pLocalFragPeptides (non-null) and leaves the other two null; both fill sub-passes pass
   // pLocalFragPeptides == nullptr and exactly one of pFillBinCounts/pFillWriteCursor. A
   // separate bCountOnly bool was redundant with this and could theoretically be passed
   // inconsistently; derive it here instead of carrying it as its own parameter.
   const bool bCountOnly = (pLocalFragPeptides != nullptr);
   // P2: was `string sPeptide = ...szPeptide;`, a heap-backed copy on every one of the
   // 10^8+ calls a whole-proteome build makes (twice per accepted variant: once in the
   // count pass, once in the fill pass). szPeptide is already a NUL-terminated char[] on
   // the raw-peptide entry, so read it directly instead of copying it into a std::string.
   const char* pszPeptide = g_vRawPeptides.at(iWhichPeptide).szPeptide;

   const char* mods = NULL;

   // Same reasoning as pszPeptide above: the MOD_SEQS_POOL/MOD_NUMBERS_POOL flat pools are
   // read-only global tables for the duration of the fill/count passes, so reference their
   // entries directly (pointer + length; pool entries are NOT NUL-terminated) rather than
   // copying into a local std::string per call.
   const char* pModSeq = NULL;
   int iModSeqLen = 0;

   // mods[] (this entry's MOD_NUMBERS_POOL slice) values are 0-based indices into this
   // COMPACTED active-variable-mod-slot list, not raw varModList indices -- see
   // CometPeptideIndex::GetVModSlotForAllModsIdx()'s own doc comment, and its
   // MaterializeOneEntry() (the PI_DB-mode equivalent of this function), which already
   // performs this same translation correctly. Passed in by both callers (which fetch it once
   // per thread/pass rather than this function re-fetching it on every one of the millions of
   // per-peptide-variant calls in a full index build).
   if (modNumIdx >= 0)  // set modified peptide info
   {
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
      pModSeq = GetModSeq(modSeqIdx, iModSeqLen);
      mods = GetModNumEntry(modNumIdx, modSeqIdx, iModSeqLen);
   }

   double dCalcPepMass;
   double dBion = g_staticParams.precalcMasses.dNtermProton;
   double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;
   int iPosReverse;  // points to residue in reverse order

   int j = 0; // track count of each modifiable residue
   int k = 0; // track count of each modifiable residue in reverse
   int iEndPos = (int)strlen(pszPeptide) - 1;

   // Search-time peptide_length_range narrower than what's baked into g_vRawPeptides (see
   // ParsePeptideIndexHeader()'s inward-only clamp of peptideLengthRange from the .idx's
   // LengthRange: line) further restricts which raw peptides get fragmented here, on every
   // rebuild of the fragment index. A wider search-time range is already a no-op: no raw
   // peptide outside the original digestion's length bounds exists in g_vRawPeptides to admit.
   if (iEndPos + 1 < g_staticParams.options.peptideLengthRange.iStart
      || iEndPos + 1 > g_staticParams.options.peptideLengthRange.iEnd)
      return;

   // P2: the fill pass (bCountOnly == false) calls this with the exact same
   // (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuple the count pass already
   // accepted, mass, range/precursor filters, and hardening-check included -- the caller
   // (GenerateFragmentIndex()) passes that already-computed mass back in as dKnownPepMass
   // (fp.dPepMass) instead of leaving this function to redo the full O(peptide length)
   // residue-by-residue recompute a second time. The count pass (dKnownPepMass < 0,
   // its default) still computes it fresh, since that's the one place it's not yet known.
   if (dKnownPepMass >= 0.0)
   {
      dCalcPepMass = dKnownPepMass;
   }
   else
   {
      // first calculate peptide mass as that's needed in fragment loop -- via
      // ComputeIndexedPepMass() (the same residue-by-residue summation this branch used to
      // inline), which FI_DB's search path also calls per candidate to recompute the mass a
      // variant was stored with, bit-identically (docs/20260827_PI_memory.md Section 7.1)
      double dResidueOnlyMass;
      dCalcPepMass = ComputeIndexedPepMass(iWhichPeptide, modNumIdx, cNtermMod, cCtermMod,
         vModSlotForAllModsIdx, &dResidueOnlyMass);

      // Hardening check: for the plain, unmodified variant of a raw peptide, the mass
      // recomputed above from pszPeptide (the raw-peptide entry's stored sequence) must
      // match the authoritative unmodified mass that was computed directly from the
      // protein sequence at digestion time and stored independently on the raw-peptide
      // entry (CometSearch.cpp:3446-3488). A large divergence means the sequence was
      // truncated/corrupted after storage -- exactly how the 'U'/B/J/O/X/Z packing-table
      // bug in core/Types.h manifested (see docs/20260709_sprankjitter.md) -- rather than a
      // benign mono/avg mass-type or protein-terminal-mod difference, which is at most a
      // few Da for realistic peptide lengths. Non-fatal: logs and continues so a batch
      // build doesn't abort over a single bad entry. Only reachable here (the fresh-compute
      // path) -- the fill pass's dKnownPepMass already went through this exact check once,
      // during the count pass, for this same peptide variant.
      if (modNumIdx < 0 && cNtermMod < 0 && cCtermMod < 0)
      {
         constexpr double MASS_CHECK_TOL = 10.0;  // Da; generous enough to absorb mono/avg or terminal-mod drift
         double dStoredMass = g_vRawPeptides.at(iWhichPeptide).dPepMass;
         double dDelta = fabs(dResidueOnlyMass - dStoredMass);
         if (dDelta > MASS_CHECK_TOL)
         {
            logerr(" Warning - AddFragments mass mismatch for peptide '" + string(pszPeptide)
               + "' (iWhichPeptide=" + std::to_string(iWhichPeptide)
               + "): recomputed mass " + std::to_string(dResidueOnlyMass)
               + " vs stored " + std::to_string(dStoredMass)
               + ", delta " + std::to_string(dDelta)
               + ". Possible truncated/corrupted peptide string.\n");
         }
      }
   }

   // dBion/dYion's own terminal-mod contribution is needed for the fragment ladder built
   // below regardless of which branch above computed dCalcPepMass.
   if (cNtermMod >= 0)  // if -1, unused
      dBion += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
   if (cCtermMod >= 0)  // if -1, unused
      dYion += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;

   if (dCalcPepMass > 99999.9)
   {
      printf(" Error, pepmass in AddFragments is %f, peptide %s, modNumIdx %d\n", dCalcPepMass, pszPeptide, modNumIdx);
      exit(1);
   }

   if (dCalcPepMass > g_massRange.dMaxMass || dCalcPepMass < g_massRange.dMinMass)
      return;

   if (!g_staticParams.options.iFragIndexSkipReadPrecursors && !g_bIndexPrecursors[BIN(dCalcPepMass)])
      return;

   if (bCountOnly)
   {
      struct FragmentPeptidesStruct sTmp;

      // Safe: g_vRawPeptides.size() <= UINT_MAX is checked once in CreateFragmentIndex()
      // before any AddFragments() call can reach here.
      sTmp.iWhichPeptide = static_cast<unsigned int>(iWhichPeptide);
      sTmp.modNumIdx = modNumIdx;
      sTmp.dPepMass = dCalcPepMass;
      sTmp.cNtermMod = cNtermMod;
      sTmp.cCtermMod = cCtermMod;

      // P1: pLocalFragPeptides is this calling thread's own vector (one per
      // AddFragmentsThreadProcRange() partition) -- no lock needed since no other thread
      // ever touches it; the caller concatenates all partitions' vectors, in partition
      // order, into g_vFragmentPeptides once every thread has finished (GenerateFragmentIndex()).
      pLocalFragPeptides->push_back(sTmp);
   }

/*
if (!(iWhichPeptide%1000))
{
   // print out the peptide
   printf("OK in AddFragments: ");
   j=0;
   for (int i = 0; i <= iEndPos; ++i)
   {
      printf("%c", (char)pszPeptide[i]);
      if (j < iModSeqLen && pszPeptide[i] == pModSeq[j])
      {
         if (modNumIdx != -1 && mods[j] != -1)
         {
            printf("%s", std::to_string(mods[j]).c_str());
         }
         j++;
      }
   }
   printf("\t%f\t%d\t%s\n", dCalcPepMass, modNumIdx, string(pModSeq, iModSeqLen).c_str());
}
*/

   j = 0;
   k = iModSeqLen - 1;

   // Fragment neutral loss (docs/20260805_carafe.md Section 6.5 / Phase 2b): mirrors
   // CometSearch.cpp's iPositionNLB/iPositionNLY exactly in effect (single loss event per
   // eligible fragment, gated on the nearest occurrence of an NL-bearing variable mod from the
   // relevant terminus -- first-from-N-term for b, first-from-C-term for y), but computed
   // incrementally in this loop's own ladder-index space rather than classic search's separate
   // residue-position pre-scan. Verified algebraically and against 10 hand-checked cases
   // (including multi-occurrence and edge-of-peptide mods) to produce identical eligibility
   // decisions to the residue-position form -- see the Phase 0-style validation script this
   // change was derived from, referenced in the commit message.
   //
   // Both arrays use the SAME sentinel (999, larger than any real ladder index) and the SAME
   // "<= i" eligibility test, unlike classic search's asymmetric sentinels (999 for NLB, -1 for
   // NLY) -- a deliberate simplification enabled by working in ladder-index space instead of
   // residue-position space, not a behavioral difference. Indexed by raw variable-mod slot
   // (0..FRAGINDEX_VMODS-1, matching cNtermMod/cCtermMod's own convention), scope is
   // residue-based variable mods only (terminal mods cNtermMod/cCtermMod are not NL-eligible --
   // classic search's iPositionNLB/iPositionNLY have no terminal-mod input either).
   int iPositionNLB[FRAGINDEX_VMODS];
   int iPositionNLY[FRAGINDEX_VMODS];
   bool bFragmentNL = g_staticParams.variableModParameters.bUseFragmentNeutralLoss && modNumIdx >= 0;
   // Largest primary neutral-loss delta across active variable-mod slots -- used below to keep
   // the below-loop's early-exit break sound once NL-shifted insertions are in play (a
   // NL-shifted ion can still land inside [min, max] even after its unshifted parent has
   // crossed max, as long as it hasn't crossed by more than dMaxNL; see the break itself for
   // the full explanation). 0.0 when bFragmentNL is false, which reduces the break to its
   // original, pre-Phase-2b form.
   double dMaxNL = 0.0;
   if (bFragmentNL)
   {
      for (int x = 0; x < FRAGINDEX_VMODS; ++x)
      {
         iPositionNLB[x] = 999;
         iPositionNLY[x] = 999;

         double dNL = g_staticParams.variableModParameters.varModList[x].dNeutralLoss;
         if (dNL > dMaxNL)
            dMaxNL = dNL;
      }
   }

   // Phase 3 predicted-fragment masking (docs/20260805_carafe.md Section 4.4/9): looked up
   // ONCE per call (the tuple key is constant for this whole variant, unlike the per-ladder-
   // index bit test below) rather than once per fragment -- this function runs potentially
   // hundreds of millions of times for a whole-proteome no-enzyme build, so a per-fragment
   // lookup would multiply that by ~(peptide length) for no benefit. Default to all-bits-set
   // ("fully unfiltered") so the SAME bit-test code below runs unconditionally whether masking
   // is disabled, this variant has no mask entry (Section 8 item 2's fallback), or a real mask
   // was found -- no separate "is masking active here" branch needed at any insertion site.
   //
   // Two different lookup paths (docs/20260822_carafe_prerun.md Section 7.6): bCountOnly is
   // the enumeration pass, which runs BEFORE g_vFragmentPeptides has its final mass-sorted
   // index -- iWhichFragmentPeptide isn't meaningful yet, so this is the one call site that
   // still needs Lookup()'s key-based binary search. Both fill sub-passes (fill-count and
   // fill-write) always run AFTER the sort, visiting variants in that same final index order,
   // so GenerateFragmentIndex() has already cached each one's decision by the time either
   // runs (right after the sort, before g_iFragmentIndex is populated) -- they read it back
   // in O(1) via LookupCached() instead of re-binary-searching s_entries a second and third
   // time, and s_entries itself is already freed by this point (CometPredictedMask::
   // FreeAfterIndexBuild(), called right after the cache is built).
   uint64_t maskB = ~0ULL, maskY = ~0ULL, maskBModloss = ~0ULL, maskYModloss = ~0ULL;
   if (CometPredictedMask::IsEnabled())
   {
      if (bCountOnly)
         CometPredictedMask::Lookup(static_cast<unsigned int>(iWhichPeptide), modNumIdx, cNtermMod, cCtermMod,
            maskB, maskY, maskBModloss, maskYModloss);
      else
         CometPredictedMask::LookupCached(iWhichFragmentPeptide, maskB, maskY, maskBModloss, maskYModloss);
      // Lookup()/LookupCached() leave maskB/maskY/maskBModloss/maskYModloss untouched (still
      // all-bits-set) when there's no entry for this variant -- deliberately not checked here;
      // "not found" and "found, fully-unfiltered" are handled identically by construction.
   }

   for (int i = 0; i < iEndPos; ++i)
   {
      iPosReverse = iEndPos - i;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pszPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pszPeptide[iPosReverse]];

      if (modNumIdx >= 0) // handle the variable mods if present on peptide
      {
         // j bound: see the same guard in the precursor-mass loop above
         if (j < iModSeqLen && pszPeptide[i] == pModSeq[j])
         {
            // Bugfix: mods[j] is a compacted index (see the note above this function's
            // declaration), not a raw varModList index -- the previous
            // "varModList[mods[j] - 1]" read varModList[-1] (undefined behavior --
            // empirically dVarModMass=0.0 observed) whenever the first configured
            // variable-mod type applied, silently computing modified b/y ion fragment masses
            // as if unmodified. For configs with more than one active mod type and a gap
            // (e.g. variable_mod01 unused, variable_mod02 set), the equivalent bug (using the
            // compacted index directly rather than the real slot) applied the wrong mod's
            // mass instead of crashing.
            //
            // Bugfix: mods[j] == -1 is the normal case for a modifiable candidate residue
            // that isn't modified in this particular combination (CometModificationsPermuter::
            // combine() leaves it at its initialized -1) -- e.g. any peptide with more
            // modifiable sites than max_variable_mods_in_peptide allows. Casting -1 to size_t
            // and indexing vModSlotForAllModsIdx with it directly read far outside the
            // vector's buffer; TranslateVarModSlot() now guards this uniformly everywhere.
            int slotB = CometPeptideIndex::TranslateVarModSlot(vModSlotForAllModsIdx, mods[j]);
            if (slotB >= 0)
            {
               dBion += g_staticParams.variableModParameters.varModList[slotB].dVarModMass;

               // Fragment neutral loss (docs/20260805_carafe.md Section 6.5 / Phase 2b):
               // record the nearest-from-N-term ladder index at which this NL-bearing slot
               // first becomes part of the b-ion, for the NL-shifted-entry insertion below.
               if (bFragmentNL)
               {
                  if (iPositionNLB[slotB] == 999
                     && g_staticParams.variableModParameters.varModList[slotB].dNeutralLoss != 0.0)
                  {
                     iPositionNLB[slotB] = i;
                  }
               }
            }
            j++;
         }

         if (k >= 0 && pszPeptide[iPosReverse] == pModSeq[k])
         {
            // see bugfix note above
            int slotY = CometPeptideIndex::TranslateVarModSlot(vModSlotForAllModsIdx, mods[k]);
            if (slotY >= 0)
            {
               dYion += g_staticParams.variableModParameters.varModList[slotY].dVarModMass;

               if (bFragmentNL)
               {
                  if (iPositionNLY[slotY] == 999
                     && g_staticParams.variableModParameters.varModList[slotY].dNeutralLoss != 0.0)
                  {
                     iPositionNLY[slotY] = i;
                  }
               }
            }
            k--;
         }
      }

      // Early-exit once neither ion can still land in [min, max] -- but a NL-shifted insertion
      // (below) uses dBion-dNL/dYion-dNL, not dBion/dYion directly, so an unshifted sum that's
      // crossed dFragIndexMaxMass by less than dMaxNL can still produce a valid in-window
      // NL-shifted entry at this or a later ladder position. Subtracting dMaxNL here (0.0 when
      // bFragmentNL is false, making this identical to the original unshifted-only check) keeps
      // the exit sound in both cases instead of silently dropping those later NL-shifted
      // insertions.
      if (dBion - dMaxNL > g_staticParams.options.dFragIndexMaxMass && dYion - dMaxNL > g_staticParams.options.dFragIndexMaxMass)
         break;

      if (i > 1)  // skip first two low mass b- and y-ions
      {
         // Phase 3 predicted-fragment mask bit for this ladder position (docs/20260805_carafe.md
         // Section 4.4/9) -- bit (i-2), matching tools/carafe_ms2_to_fi_mask.py's documented
         // convention exactly (bit 0 == i==2 == length 3). maskB/maskY/maskBModloss/
         // maskYModloss default to all-bits-set (see this function's setup above), so this
         // check is always a no-op unless masking is actually enabled AND this variant has a
         // real mask entry AND that entry actually clears this specific bit.
         uint64_t maskBit = 1ULL << (i - 2);

         if ((maskB & maskBit)
            && dBion > g_staticParams.options.dFragIndexMinMass && dBion < g_staticParams.options.dFragIndexMaxMass)
         {
            int iBinBion = BIN(dBion);

            if ((unsigned int)iBinBion >= g_massRange.uiMaxFragmentArrayIndex)
            {
               printf(" Error: FI dBion %lf too large, pep %s\n", dBion, pszPeptide);
               exit(1);
            }

            // P1: three mutually-exclusive destinations selected by which pointer the
            // caller supplied -- see this function's doc comment in the header.
            if (bCountOnly)
               FRAGINDEX_ATOMIC_FETCH_ADD(g_iFragmentIndexOffset[iBinBion]);
            else if (pFillWriteCursor != nullptr)
               g_iFragmentIndex[pFillWriteCursor[iBinBion]++] = static_cast<unsigned int>(iWhichFragmentPeptide);
            else
               pFillBinCounts[iBinBion] += 1;
         }

         if ((maskY & maskBit)
            && dYion > g_staticParams.options.dFragIndexMinMass && dYion < g_staticParams.options.dFragIndexMaxMass)
         {
            int iBinYion = BIN(dYion);

            if ((unsigned int)iBinYion >= g_massRange.uiMaxFragmentArrayIndex)
            {
               printf(" Error: FI dYion %lf too large, pep %s\n", dYion, pszPeptide);
               exit(1);
            }

            if (bCountOnly)
               FRAGINDEX_ATOMIC_FETCH_ADD(g_iFragmentIndexOffset[iBinYion]);
            else if (pFillWriteCursor != nullptr)
               g_iFragmentIndex[pFillWriteCursor[iBinYion]++] = static_cast<unsigned int>(iWhichFragmentPeptide);
            else
               pFillBinCounts[iBinYion] += 1;
         }

         // Neutral-loss-shifted variants: inserted as SEPARATE FI entries alongside the
         // unshifted ones above (decision: insert both, docs/20260805_carafe.md Section 8
         // item 11), one per NL-bearing variable-mod slot whose nearest relevant-terminus
         // occurrence this fragment already reaches. Primary loss (dNeutralLoss) only --
         // dNeutralLoss2 deferred (Section 8 item 10). Gated by maskBModloss/maskYModloss
         // (Phase 3, Section 8 items 12-14) -- an INDEPENDENT bit pool from maskB/maskY at this same bit
         // position, matching the mask builder's own independent thresholding of the
         // unshifted vs. modloss channels.
         if (bFragmentNL)
         {
            for (int x = 0; x < FRAGINDEX_VMODS; ++x)
            {
               double dNL = g_staticParams.variableModParameters.varModList[x].dNeutralLoss;
               if (dNL == 0.0)
                  continue;

               if ((maskBModloss & maskBit) && iPositionNLB[x] <= i)
               {
                  double dNLBion = dBion - dNL;
                  if (dNLBion > g_staticParams.options.dFragIndexMinMass && dNLBion < g_staticParams.options.dFragIndexMaxMass)
                  {
                     int iBinNLBion = BIN(dNLBion);

                     if ((unsigned int)iBinNLBion >= g_massRange.uiMaxFragmentArrayIndex)
                     {
                        printf(" Error: FI dNLBion %lf too large, pep %s\n", dNLBion, pszPeptide);
                        exit(1);
                     }

                     if (bCountOnly)
                        FRAGINDEX_ATOMIC_FETCH_ADD(g_iFragmentIndexOffset[iBinNLBion]);
                     else if (pFillWriteCursor != nullptr)
                        g_iFragmentIndex[pFillWriteCursor[iBinNLBion]++] = static_cast<unsigned int>(iWhichFragmentPeptide);
                     else
                        pFillBinCounts[iBinNLBion] += 1;
                  }
               }

               if ((maskYModloss & maskBit) && iPositionNLY[x] <= i)
               {
                  double dNLYion = dYion - dNL;
                  if (dNLYion > g_staticParams.options.dFragIndexMinMass && dNLYion < g_staticParams.options.dFragIndexMaxMass)
                  {
                     int iBinNLYion = BIN(dNLYion);

                     if ((unsigned int)iBinNLYion >= g_massRange.uiMaxFragmentArrayIndex)
                     {
                        printf(" Error: FI dNLYion %lf too large, pep %s\n", dNLYion, pszPeptide);
                        exit(1);
                     }

                     if (bCountOnly)
                        FRAGINDEX_ATOMIC_FETCH_ADD(g_iFragmentIndexOffset[iBinNLYion]);
                     else if (pFillWriteCursor != nullptr)
                        g_iFragmentIndex[pFillWriteCursor[iBinNLYion]++] = static_cast<unsigned int>(iWhichFragmentPeptide);
                     else
                        pFillBinCounts[iBinNLYion] += 1;
                  }
               }
            }
         }
      }
   }
}


// Populate g_pvDBIndex and g_pvProteinsList from the FASTA database using the
// fast per-thread PepGenTuple path (avoids heap-allocating one std::string per
// peptide instance in g_pvDBIndex, which OOMs on 32 GB machines for no-enzyme
// human proteome searches with ~900 M peptide instances).
//
// Pre-conditions:
//   - CometSearch memory pool already allocated (AllocateMemory called)
//   - g_pvProteinNames already populated (read by caller before this call)
//   - g_massRange set
//
// Post-conditions:
//   - g_pvDBIndex:     unique peptides, sorted by mass, lIndexProteinFilePosition
//                      is an index into g_pvProteinsList
//   - g_pvProteinsList: ProteinsListCSR (flat CSR), one row per unique peptide
//   - g_vvvPepGenShort / g_vvvPepGenLong: cleared (memory freed)
bool CometFragmentIndex::GeneratePlainPeptideIndex(ThreadPool* tp)
{
   // docs/20260827_PI_memory.md Phase 4: ProteinsListCSR stores protein references as
   // uint32 -- at build time these are FASTA byte offsets, so the FASTA must fit in 32
   // bits. Check once up front (making every later narrowing safe by construction) rather
   // than per-offset in the digestion hot path; a >= 4 GB FASTA fails loudly here.
   {
      FILE* fpCheck = fopen(g_staticParams.databaseInfo.szDatabase, "rb");
      if (fpCheck != NULL)
      {
         comet_fseek(fpCheck, 0, SEEK_END);
         comet_fileoffset_t lFastaSize = comet_ftell(fpCheck);
         fclose(fpCheck);
         if ((uint64_t)lFastaSize > 0xFFFFFFFFull)
         {
            string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
               + "\" is larger than 4 GB, which indexed searches cannot digest (protein\n"
               + " references are stored as 32-bit FASTA offsets during an index build).\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }
   }

   int iNumThreads = g_staticParams.options.iNumThreads;
   const int iMinLen = g_staticParams.options.peptideLengthRange.iStart;
   const int iMaxLen = g_staticParams.options.peptideLengthRange.iEnd;

   // Short: lengths [iMinLen, min(12, iMaxLen)]; li = iPepLen - iMinLen (push site)
   const int iShortMax  = (iMaxLen < 12) ? iMaxLen : 12;
   const int nShortLens = (iMinLen <= iShortMax) ? (iShortMax - iMinLen + 1) : 0;

   // Long: lengths [13, iMaxLen]; li = iPepLen - 13 (push site)
   const int nLongLens  = (iMaxLen >= 13) ? (iMaxLen - 13 + 1) : 0;

   g_vvvPepGenShort.assign(nShortLens, vector<vector<PepGenTupleShort>>(iNumThreads));
   g_vvvPepGenLong.assign(nLongLens,   vector<vector<PepGenTuple>>(iNumThreads));

   // Save/restore rather than hardcode: this function is reused by the shared
   // CometPeptideIndex::WritePeptideIndex() build path (docs/20260730_PI_reduction.md
   // Phase 0, both -i and -j), which enters with iDbType == FASTA_DB.
   // Hardcoding the post-call value to FI_DB would be wrong for a PI_DB caller.
   const bool  bCreateFragmentIndexSave = g_staticParams.options.bCreateFragmentIndex;
   const DbType iDbTypeSave             = g_staticParams.iDbType;

   g_staticParams.options.bCreateFragmentIndex = true;
   g_staticParams.options.bFastPlainPeptideIdx = true;
   g_staticParams.iDbType = DbType::FASTA_DB;

   vector<Query*> emptyQueries;
   bool bSucceeded = CometSearch::RunSearch(0, 0, tp, emptyQueries);

   g_staticParams.options.bCreateFragmentIndex = bCreateFragmentIndexSave;
   g_staticParams.options.bFastPlainPeptideIdx = false;
   g_staticParams.iDbType = iDbTypeSave;

   if (!bSucceeded)
      return false;

   const bool bIL = g_staticParams.options.bTreatSameIL;

   // Per-length result: sort+dedup builds local dbIdx/prots; a sequential
   // merge step below fixes up lIndexProteinFilePosition and appends to globals.
   //
   // prots is stored as a flat CSR (prots_flat + prots_cnt) rather than
   // vector<vector<>> to avoid one heap allocation per unique peptide.
   // For a no-enzyme human proteome build that is ~189M allocations avoided.
   struct LenResult
   {
      vector<DBIndex>            dbIdx;
      vector<unsigned int>       prots_flat;  // all protein FASTA offsets concatenated (uint32-safe: FASTA size checked above)
      vector<uint32_t>           prots_cnt;   // number of proteins per peptide row
   };

   vector<LenResult> longResults(nLongLens);
   vector<LenResult> shortResults(nShortLens);

   // Submit long lengths first -- O(iLen) comparator makes them slower per
   // element, so they should reach threads before the fast short sorts.
   for (int li = 0; li < nLongLens; ++li)
   {
      tp->doJob([li, bIL, iNumThreads, &longResults]()
      {
         const int iLen = 13 + li;
         LenResult& r = longResults[li];

         size_t iTotal = 0;
         for (int s = 0; s < iNumThreads; ++s)
            iTotal += g_vvvPepGenLong[li][s].size();

         if (iTotal == 0)
            return;

         vector<PepGenTuple> buf;
         buf.reserve(iTotal);
         for (int s = 0; s < iNumThreads; ++s)
         {
            auto& v = g_vvvPepGenLong[li][s];
            buf.insert(buf.end(), make_move_iterator(v.begin()), make_move_iterator(v.end()));
            vector<PepGenTuple>().swap(v);
         }

         sort(buf.begin(), buf.end(), [iLen, bIL](const PepGenTuple& a, const PepGenTuple& b) {
            for (int k = 0; k < iLen; ++k)
            {
               char ca = (bIL && a.sPeptide[k] == 'L') ? 'I' : a.sPeptide[k];
               char cb = (bIL && b.sPeptide[k] == 'L') ? 'I' : b.sPeptide[k];
               if (ca != cb) return ca < cb;
            }
            return a.lProteinFileOffset < b.lProteinFileOffset;
         });

         auto bCanonEqual = [iLen, bIL](const PepGenTuple& a, const PepGenTuple& b) {
            for (int k = 0; k < iLen; ++k)
            {
               char ca = (bIL && a.sPeptide[k] == 'L') ? 'I' : a.sPeptide[k];
               char cb = (bIL && b.sPeptide[k] == 'L') ? 'I' : b.sPeptide[k];
               if (ca != cb) return false;
            }
            return true;
         };

         vector<unsigned int> prot;
         // OR'd (not just the representative's) across every occurrence in the dedup run:
         // with protein_modslist_file active, a peptide shared between a listed and an
         // unlisted protein must not silently lose the listed protein's allowed-mod bits just
         // because the unlisted protein happened to sort first (smallest file offset) and
         // become buf[iRunStart]. Bit i allowed by ANY occurrence should stay allowed for the
         // merged peptide -- the per-protein occurrence list (prot/prots_flat above) already
         // records which specific proteins back this peptide.
         unsigned short siVarModFilterUnion = 0;
         size_t iRunStart = 0;
         for (size_t i = 0; i <= buf.size(); ++i)
         {
            bool bFlush = (i == buf.size()) ||
                          (i > 0 && !bCanonEqual(buf[i], buf[i - 1]));
            if (bFlush && i > 0)
            {
               sort(prot.begin(), prot.end());
               prot.erase(unique(prot.begin(), prot.end()), prot.end());
               r.prots_cnt.push_back((uint32_t)prot.size());
               r.prots_flat.insert(r.prots_flat.end(), prot.begin(), prot.end());
               prot.clear();

               const PepGenTuple& rep = buf[iRunStart];
               DBIndex dbi;
               memcpy(dbi.sPeptide, rep.sPeptide, iLen);
               dbi.sPeptide[iLen] = '\0';
               dbi.dPepMass                  = rep.dPepMass;
               dbi.cPrevAA                   = rep.cPrevAA;
               dbi.cNextAA                   = rep.cNextAA;
               dbi.siVarModProteinFilter     = siVarModFilterUnion;
               dbi.lIndexProteinFilePosition = (comet_fileoffset_t)(r.prots_cnt.size() - 1);
               dbi.pcVarModSites.clear();
               r.dbIdx.push_back(dbi);

               iRunStart = i;
               siVarModFilterUnion = 0;
            }
            if (i < buf.size())
            {
               prot.push_back((unsigned int)buf[i].lProteinFileOffset);   // fits: FASTA < 4 GB checked at function entry
               siVarModFilterUnion |= buf[i].siVarModProteinFilter;
            }
         }

         vector<PepGenTuple>().swap(buf);

         // Mass-sort this length's entries here, inside the per-length job (still fully
         // parallel across lengths), instead of the former post-merge parallel slice sort
         // over g_pvDBIndex: the merge below now pools entries straight into
         // g_vRawPeptides, so the 88B DBIndex form must already be in its final (on-disk)
         // order when merged. Same comparator, element type, and per-length input order as
         // the slice sort it replaces -- the resulting order, ties included, is identical.
         sort(r.dbIdx.begin(), r.dbIdx.end(), CometMassSpecUtils::DBICompareByMass);
      });
   }

   // Submit short lengths after longs -- integer sort finishes quickly and
   // fills in behind the heavier long-length tasks.
   for (int li = 0; li < nShortLens; ++li)
   {
      tp->doJob([li, bIL, iNumThreads, iMinLen, &shortResults]()
      {
         const int iLen = iMinLen + li;
         LenResult& r = shortResults[li];

         size_t iTotal = 0;
         for (int s = 0; s < iNumThreads; ++s)
            iTotal += g_vvvPepGenShort[li][s].size();

         if (iTotal == 0)
            return;

         vector<PepGenTupleShort> buf;
         buf.reserve(iTotal);
         for (int s = 0; s < iNumThreads; ++s)
         {
            auto& v = g_vvvPepGenShort[li][s];
            buf.insert(buf.end(), make_move_iterator(v.begin()), make_move_iterator(v.end()));
            vector<PepGenTupleShort>().swap(v);
         }

         sort(buf.begin(), buf.end(), [](const PepGenTupleShort& a, const PepGenTupleShort& b) {
            if (a.uPackedPep != b.uPackedPep)
               return a.uPackedPep < b.uPackedPep;
            return a.lProteinFileOffset < b.lProteinFileOffset;
         });

         char szSeq[MAX_PEPTIDE_LEN + 1];
         vector<unsigned int> prot;
         // Same fix as the long-length path above: OR the mask across the whole dedup run
         // instead of taking only the representative occurrence's mask.
         unsigned short siVarModFilterUnion = 0;
         size_t iRunStart = 0;
         for (size_t i = 0; i <= buf.size(); ++i)
         {
            bool bFlush = (i == buf.size()) ||
                          (i > 0 && buf[i].uPackedPep != buf[i - 1].uPackedPep);
            if (bFlush && i > 0)
            {
               sort(prot.begin(), prot.end());
               prot.erase(unique(prot.begin(), prot.end()), prot.end());
               r.prots_cnt.push_back((uint32_t)prot.size());
               r.prots_flat.insert(r.prots_flat.end(), prot.begin(), prot.end());
               prot.clear();

               const PepGenTupleShort& rep = buf[iRunStart];
               UnpackPeptide(rep.uPackedPep, iLen, szSeq);
               if (bIL)
                  for (uint16_t mask = rep.uILMask, k = 0; mask; mask >>= 1, ++k)
                     if (mask & 1) szSeq[k] = 'L';

               DBIndex dbi;
               strcpy(dbi.sPeptide, szSeq);
               dbi.dPepMass                  = rep.dPepMass;
               dbi.cPrevAA                   = rep.cPrevAA;
               dbi.cNextAA                   = rep.cNextAA;
               dbi.siVarModProteinFilter     = siVarModFilterUnion;
               dbi.lIndexProteinFilePosition = (comet_fileoffset_t)(r.prots_cnt.size() - 1);
               dbi.pcVarModSites.clear();
               r.dbIdx.push_back(dbi);

               iRunStart = i;
               siVarModFilterUnion = 0;
            }
            if (i < buf.size())
            {
               prot.push_back((unsigned int)buf[i].lProteinFileOffset);   // fits: FASTA < 4 GB checked at function entry
               siVarModFilterUnion |= buf[i].siVarModProteinFilter;
            }
         }

         vector<PepGenTupleShort>().swap(buf);

         // Mass-sort this length's entries here, inside the per-length job (still fully
         // parallel across lengths), instead of the former post-merge parallel slice sort
         // over g_pvDBIndex: the merge below now pools entries straight into
         // g_vRawPeptides, so the 88B DBIndex form must already be in its final (on-disk)
         // order when merged. Same comparator, element type, and per-length input order as
         // the slice sort it replaces -- the resulting order, ties included, is identical.
         sort(r.dbIdx.begin(), r.dbIdx.end(), CometMassSpecUtils::DBICompareByMass);
      });
   }

   tp->wait_on_threads();
   g_vvvPepGenShort.clear();
   g_vvvPepGenLong.clear();

   // Sequential merge: fix up lIndexProteinFilePosition (local->global offset) and append
   // to the global structures -- since the g_pvDBIndex-transient removal
   // (docs/20260827_PI_memory.md build-path follow-up), entries pool STRAIGHT into
   // g_vRawPeptides (already mass-sorted per length by the jobs above) and each length's
   // 88B/entry DBIndex staging is freed as soon as it is consumed, instead of a second
   // full-size 88B/entry copy accumulating in g_pvDBIndex (~17.5 GB of transient at the
   // 199M-peptide MHC scale; the reason the ~400M-peptide target-decoy MHC config could
   // not be built in 54 GB at all). Long results first to match submission order, then
   // short results -- the same on-disk order as before.
   {
      size_t tTotalPeptides = 0;
      size_t tTotalSeqBytes = 0;
      for (const auto& r : longResults)
      {
         tTotalPeptides += r.dbIdx.size();
         for (const auto& dbi : r.dbIdx)
            tTotalSeqBytes += strlen(dbi.sPeptide) + 1;
      }
      for (const auto& r : shortResults)
      {
         tTotalPeptides += r.dbIdx.size();
         for (const auto& dbi : r.dbIdx)
            tTotalSeqBytes += strlen(dbi.sPeptide) + 1;
      }
      g_vRawPeptides.clear();
      g_vRawPeptides.reserve(tTotalPeptides, tTotalSeqBytes);
   }

   size_t iProtBase = 0;
   auto mergeResults = [&](vector<LenResult>& results)
   {
      for (auto& r : results)
      {
         for (auto& dbi : r.dbIdx)
            dbi.lIndexProteinFilePosition += (comet_fileoffset_t)iProtBase;
         iProtBase += r.prots_cnt.size();
         if (!g_pvProteinsList.append_flat(r.prots_flat, r.prots_cnt))
         {
            string strErrorMsg = " Error - protein list exceeds the uint32 CSR limit"
               " (>4.29e9 (peptide, protein) pairs); reduce the database/digest size.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         for (const auto& dbi : r.dbIdx)
         {
            if (!g_vRawPeptides.push_back(dbi.sPeptide, (int)strlen(dbi.sPeptide),
                  dbi.cPrevAA, dbi.cNextAA, dbi.dPepMass, dbi.siVarModProteinFilter,
                  dbi.lIndexProteinFilePosition))
            {
               // Unreachable on this path (values are g_pvProteinsList row indices,
               // bounded by the peptide count) -- fail loudly rather than truncate.
               string strErrorMsg = " Error - protein reference out of range while pooling the raw peptide table.\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               return false;
            }
         }
         vector<DBIndex>().swap(r.dbIdx);   // free this length's 88B/entry staging now
      }

      return true;
   };

   if (!mergeResults(longResults) || !mergeResults(shortResults))
      return false;

   return true;
}


// WriteFIPlainPeptideIndex() and ReadPlainPeptideIndex() retired
// (docs/20260730_PI_reduction.md Phase 0): both are superseded by
// CometPeptideIndex::WritePeptideIndex()/ReadPeptideIndex(), which produce/consume the
// unified index format shared by PI_DB and FI_DB search modes. See CreateFragmentIndex()
// above (build dispatch is in CometSearchManager.cpp).
