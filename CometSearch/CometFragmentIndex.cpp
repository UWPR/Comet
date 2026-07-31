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
#include "CometSearch.h"
#include "ThreadPool.h"
#include "CometStatus.h"
#include "CometMassSpecUtils.h"
#include "CometModificationsPermuter.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <bitset>
#include <limits>
#include <queue>


vector<ModificationNumber> MOD_NUMBERS;
vector<string> MOD_SEQS;    // Unique modifiable sequences.
int* MOD_SEQ_MOD_NUM_START; // Start index in the MOD_NUMBERS vector for a modifiable sequence; -1 if no modification numbers were generated
int* MOD_SEQ_MOD_NUM_CNT;   // Total modifications numbers for a modifiable sequence.
int* PEPTIDE_MOD_SEQ_IDXS;  // Index into the MOD_SEQS vector; -1 for peptides that have no modifiable amino acids; -2 if only terminal mods.
int MOD_NUM = 0;
size_t tTmp;

Mutex CometFragmentIndex::_vFragmentPeptidesMutex;

// Temporary write-position array used only during the index fill pass.
// Initialized to g_iFragmentIndexOffset[0..n-1] before filling, freed after.
static uint64_t* s_iWritePos = nullptr;


#ifdef _WIN32
#ifdef _WIN64
comet_fileoffset_t clSizeCometFileOffset = sizeof(comet_fileoffset_t);              //win64
#else
comet_fileoffset_t clSizeCometFileOffset = (long long)sizeof(comet_fileoffset_t);   //win32
#endif
#else
comet_fileoffset_t clSizeCometFileOffset = sizeof(comet_fileoffset_t);              //linux
#endif


CometFragmentIndex::CometFragmentIndex()
{
}

CometFragmentIndex::~CometFragmentIndex()
{
}


bool CometFragmentIndex::CreateFragmentIndex(ThreadPool *tp, bool bIsRTS)
{
   // Reads the shared unified .idx format (docs/20260730_PI_reduction.md Phase 0) --
   // g_vRawPeptides plus the persisted MOD_SEQS/MOD_NUMBERS/etc. permutation tables that
   // GenerateFragmentIndex() (via AddFragmentsThreadProc()) reads directly below, exactly as
   // it already did against the pre-unification plain-index format -- no PermuteIndexPeptideMods()
   // call needed here, before or after this unification. g_vDBIndexVariants also gets
   // populated but is unused in this FI_DB path (PI_DB-only).
   if (!g_bPlainPeptideIndexRead)
      CometPeptideIndex::ReadPeptideIndex(bIsRTS);

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
   GenerateFragmentIndex(tp);

   return true;
}


void CometFragmentIndex::PermuteIndexPeptideMods(vector<PlainPeptideIndexStruct>& g_vRawPeptides)
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

   // Get the unique modifiable sequences from the peptides
   PEPTIDE_MOD_SEQ_IDXS = new int[g_vRawPeptides.size()];

   MOD_SEQS = ModificationsPermuter::getModifiableSequences(g_vRawPeptides, PEPTIDE_MOD_SEQ_IDXS, ALL_MODS);

   // Get the modification combinations for each unique modifiable substring
   ModificationsPermuter::getModificationCombinations(MOD_SEQS, vMaxNumVarModsPerMod, ALL_MODS,
         MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);
}


void CometFragmentIndex::GenerateFragmentIndex(ThreadPool *tp)
{
   cout <<  " - generate fragment ion index\n"; fflush(stdout);

   auto tFIGlobalStartTime = chrono::steady_clock::now();

   Threading::InitMutex(&_vFragmentPeptidesMutex);

   ThreadPool *pFragmentIndexPool = tp;

   // Create N number of threads, each of which will iterate through
   // a subset of peptides to calculate their fragment ions

   // Sort the peptides by mass

   cout <<  "   - store peptide list and reserve memory ... "; fflush(stdout);
   auto tStartTime = chrono::steady_clock::now();
   // stupid workaround for Windows/Visual Studio performance ... first calculate all
   // fragments to find size of each fragment on index vector
   AddFragmentsThreadProc(1, pFragmentIndexPool);
   pFragmentIndexPool->wait_on_threads();

   // Convert per-bin counts (stored in g_iFragmentIndexOffset[0..n-1] during count pass)
   // to CSR prefix-sum offsets, then allocate the single flat data array.
   // Use uint64_t accumulator: non-enzymatic searches against large databases can
   // exceed UINT_MAX total entries, silently corrupting the index with unsigned int.
   {
      uint64_t uiTotal = 0;
      for (unsigned int iMass = 0; iMass < g_massRange.uiMaxFragmentArrayIndex; ++iMass)
      {
         uint64_t uiCnt = g_iFragmentIndexOffset[iMass];
         g_iFragmentIndexOffset[iMass] = uiTotal;
         uiTotal += uiCnt;
      }
      g_iFragmentIndexOffset[g_massRange.uiMaxFragmentArrayIndex] = uiTotal;  // sentinel
      g_iFragmentIndex = new unsigned int[uiTotal];
   }

   // Initialize per-bin write positions as a copy of the base offsets.
   s_iWritePos = new uint64_t[g_massRange.uiMaxFragmentArrayIndex];
   memcpy(s_iWritePos, g_iFragmentIndexOffset, sizeof(uint64_t) * g_massRange.uiMaxFragmentArrayIndex);

   cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

   // now sort g_vFragmentPeptides by mass; this was filled in the above AddFragmentsThreadProc calls
   tStartTime = chrono::steady_clock::now();
   cout << "   - sort peptides by mass ... "; fflush(stdout);
   sort(g_vFragmentPeptides.begin(), g_vFragmentPeptides.end(), [](const FragmentPeptidesStruct& a, const FragmentPeptidesStruct& b)
      {
         return a.dPepMass < b.dPepMass;
      });
   cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

   // In the for loop below, peptide references (iWhichFragmentPeptide) are stored in the FI.
   // As the FI is an array of unsigned int pointers, need to ensure that iWhichFragmentPeptide
   // will fit into an unsigned int.
   // NOTE: explicitly use (std::numeric_limits<unsigned int>::max)() to avoid macro expansion on Windows.
   if (g_vFragmentPeptides.size() > (std::numeric_limits<unsigned int>::max)())
   {
      // handle error: value too large to fit in unsigned int
      throw std::overflow_error(" Error: g_vFragmentPeptides.size() too large for unsigned int");
   }

   // now populate the fragment index vector
   tStartTime = chrono::steady_clock::now();
   cout <<  "   - populate index ... "; fflush(stdout);
   for (size_t iWhichFragmentPeptide = 0; iWhichFragmentPeptide < g_vFragmentPeptides.size(); ++iWhichFragmentPeptide)
   {
      auto& fp = g_vFragmentPeptides[iWhichFragmentPeptide];
      AddFragments(g_vRawPeptides, fp.iWhichPeptide, iWhichFragmentPeptide, fp.modNumIdx, fp.cNtermMod, fp.cCtermMod, 0);
   }
   pFragmentIndexPool->wait_on_threads();
   cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

   // Write positions no longer needed after fill.
   delete[] s_iWritePos;
   s_iWritePos = nullptr;

   Threading::DestroyMutex(_vFragmentPeptidesMutex);

   // Total entry count is the CSR sentinel value.
   unsigned long long ullCount = g_iFragmentIndexOffset[g_massRange.uiMaxFragmentArrayIndex];

   if (g_vFragmentPeptides.size() > 1e6)
      printf("   - %0.3e total peptides, ", (double)g_vFragmentPeptides.size());
   else
      printf("   - %zu total peptides, ", g_vFragmentPeptides.size());
   if (ullCount > 1e6)
      printf("%0.3e FI entries", (double)ullCount);
   else
      printf("%llu FI entries", ullCount);

   cout << " ... " << CometMassSpecUtils::ElapsedTime(tFIGlobalStartTime) << endl;

}


void CometFragmentIndex::AddFragmentsThreadProc(bool bCountOnly,
                                                ThreadPool* /*tp*/)
{
   size_t iWhichFragmentPeptide = 0;  // unused here for counting only

   // each thread will loop through a subset of the g_vRawPeptides
   for (size_t iWhichPeptide = 0; iWhichPeptide < g_vRawPeptides.size(); ++iWhichPeptide)
   {
      // AddFragments for unmodified peptide; only if no variable mods are required
      if (!g_staticParams.variableModParameters.iRequireVarMod)
         AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, -1, -1, bCountOnly);

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
               AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, ctNtermMod, -1, bCountOnly);
            }
         }

         // Add any c-term variable mods
         for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
               && (!g_staticParams.variableModParameters.bVarModProteinFilter
                  || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod)))
            {
               AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, -1, ctCtermMod, bCountOnly);
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
                  AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, -1, ctNtermMod, ctCtermMod, bCountOnly);
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

            // if protein variable mod filter is applied, check mods[] against the peptides siVarModProteinFilter
            if (g_staticParams.variableModParameters.bVarModProteinFilter)
            {
               char* mods = MOD_NUMBERS.at(modNumIdx).modifications;

               for (int i = 0; i < MOD_NUMBERS.at(modNumIdx).modStringLen; ++i)
               {
                  // if mods[i] is not set to 1 in siVarModProteinFilter, do not apply this mod
                  if (!cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, mods[i]))
                  {
                     bPass = false;
                     break;
                  }
               }
            }

            if (bPass)
            {
               AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, -1, -1, bCountOnly);

               if (g_staticParams.variableModParameters.bVarTermModSearch)
               {
                  // Add any n-term variable mods
                  for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
                  {
                     if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                        && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)))
                     {
                        AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, ctNtermMod, -1, bCountOnly);
                     }
                  }

                  // Add any c-term variable mods
                  for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
                  {
                     if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                        && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod)))
                     {
                        AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, -1, ctCtermMod, bCountOnly);
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
                           AddFragments(g_vRawPeptides, iWhichPeptide, iWhichFragmentPeptide, modNumIdx, ctNtermMod, ctCtermMod, bCountOnly);
                        }
                     }
                  }
               }
            }
         }
      }
   }
}


void CometFragmentIndex::AddFragments(vector<PlainPeptideIndexStruct>& g_vRawPeptides,
                                      size_t iWhichPeptide,
                                      size_t iWhichFragmentPeptide,
                                      int modNumIdx,
                                      char cNtermMod,
                                      char cCtermMod,
                                      bool bCountOnly)
{
   string sPeptide = g_vRawPeptides.at(iWhichPeptide).szPeptide;

   ModificationNumber modNum;
   char* mods = NULL;
   int modSeqIdx = -1;
   string modSeq;

   if (modNumIdx >= 0)  // set modified peptide info
   {
      modNum = MOD_NUMBERS.at(modNumIdx);
      mods = modNum.modifications;
      modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
      modSeq = MOD_SEQS.at(modSeqIdx);
   }

   double dCalcPepMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm;
   double dBion = g_staticParams.precalcMasses.dNtermProton;
   double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;
   int iPosReverse;  // points to residue in reverse order

   int j = 0; // track count of each modifiable residue
   int k = 0; // track count of each modifiable residue in reverse
   int iEndPos = (int)sPeptide.length() - 1;

   // Search-time peptide_length_range narrower than what's baked into g_vRawPeptides (see
   // ParsePeptideIndexHeader()'s inward-only clamp of peptideLengthRange from the .idx's
   // LengthRange: line) further restricts which raw peptides get fragmented here, on every
   // rebuild of the fragment index. A wider search-time range is already a no-op: no raw
   // peptide outside the original digestion's length bounds exists in g_vRawPeptides to admit.
   if (iEndPos + 1 < g_staticParams.options.peptideLengthRange.iStart
      || iEndPos + 1 > g_staticParams.options.peptideLengthRange.iEnd)
      return;

   // first calculate peptide mass as that's needed in fragment loop
   j = 0;
   double dResidueOnlyMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm;
   for (int i = 0; i <= iEndPos; ++i)
   {
      dCalcPepMass += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];
      dResidueOnlyMass += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];

      if (modNumIdx >= 0) // handle the variable mods if present on peptide
      {
         if (sPeptide[i] == modSeq[j])
         {
            if (mods[j] != -1)
            {
               dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)mods[j]].dVarModMass;
            }
            j++;
         }
      }
   }

   // Hardening check: for the plain, unmodified variant of a raw peptide, the mass
   // recomputed here from sPeptide (a NUL-terminated string reconstructed from the
   // stored szPeptide char[]) must match the authoritative unmodified mass that was
   // computed directly from the protein sequence at digestion time and stored
   // independently on the raw-peptide entry (CometSearch.cpp:3446-3488). A large
   // divergence means szPeptide was truncated/corrupted after storage -- exactly how
   // the 'U'/B/J/O/X/Z packing-table bug in core/Types.h manifested (see
   // docs/20260709_sprankjitter.md) -- rather than a benign mono/avg mass-type or
   // protein-terminal-mod difference, which is at most a few Da for realistic
   // peptide lengths. Non-fatal: logs and continues so a batch build doesn't abort
   // over a single bad entry.
   if (modNumIdx < 0 && cNtermMod < 0 && cCtermMod < 0)
   {
      constexpr double MASS_CHECK_TOL = 10.0;  // Da; generous enough to absorb mono/avg or terminal-mod drift
      double dStoredMass = g_vRawPeptides.at(iWhichPeptide).dPepMass;
      double dDelta = fabs(dResidueOnlyMass - dStoredMass);
      if (dDelta > MASS_CHECK_TOL)
      {
         logerr(" Warning - AddFragments mass mismatch for peptide '" + sPeptide
            + "' (iWhichPeptide=" + std::to_string(iWhichPeptide)
            + "): recomputed mass " + std::to_string(dResidueOnlyMass)
            + " vs stored " + std::to_string(dStoredMass)
            + ", delta " + std::to_string(dDelta)
            + ". Possible truncated/corrupted peptide string.\n");
      }
   }

   if (cNtermMod >= 0)  // if -1, unused
   {
      dBion += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
   }
   if (cCtermMod >= 0)  // if -1, unused
   {
      dYion += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;
   }

   if (dCalcPepMass > 99999.9)
   {
      printf(" Error, pepmass in AddFragments is %f, peptide %s, modNumIdx %d\n", dCalcPepMass, sPeptide.c_str(), modNumIdx);
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

      // Store the current peptide; iWhichFragmentPeptide references this peptide entry
      // for use in the g_iFragmentIndex fragment index.  As this is a global list of
      // peptides, need to lock when updating to avoid thread conflicts
      if (g_vFragmentPeptides.size() >= UINT_MAX)
      {
         printf(" Error in CometFragmentIndex; UINT_MAX (%d) peptides reached.\n", UINT_MAX);
         exit(1);
      }
      // store peptide representation based on sequence (iWhichPeptide), modification state (modNumIdx), and mass (dPepMass)
      g_vFragmentPeptides.push_back(sTmp);
   }

/*
if (!(iWhichPeptide%1000))
{
   // print out the peptide
   printf("OK in AddFragments: ");
   j=0;
   for (int i = 0; i <= iEndPos; ++i)
   {
      printf("%c", (char)sPeptide[i]);
      if (sPeptide[i] == modSeq[j])
      {
         if (modNumIdx != -1 && mods[j] != -1)
         {
            printf("%s", std::to_string(mods[j]).c_str());
         }
         j++;
      }
   }
   printf("\t%f\t%d\t%s\n", dCalcPepMass, modNumIdx, modSeq.c_str());
}
*/

   j = 0;
   k = (int)modSeq.size() - 1;

   for (int i = 0; i < iEndPos; ++i)
   {
      iPosReverse = iEndPos - i;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[iPosReverse]];

      if (modNumIdx >= 0) // handle the variable mods if present on peptide
      {
         if (sPeptide[i] == modSeq[j])
         {
            dBion += g_staticParams.variableModParameters.varModList[mods[j] - 1].dVarModMass;
            j++;
         }

         if (sPeptide[iPosReverse] == modSeq[k])
         {
            dYion += g_staticParams.variableModParameters.varModList[mods[k] - 1].dVarModMass;
            k--;
         }
      }

      if (dBion > g_staticParams.options.dFragIndexMaxMass && dYion > g_staticParams.options.dFragIndexMaxMass)
         break;

      if (i > 1)  // skip first two low mass b- and y-ions
      {
         if (dBion > g_staticParams.options.dFragIndexMinMass && dBion < g_staticParams.options.dFragIndexMaxMass)
         {
            int iBinBion = BIN(dBion);

            if ((unsigned int)iBinBion >= g_massRange.uiMaxFragmentArrayIndex)
            {
               printf(" Error: FI dBion %lf too large, pep %s\n", dBion, sPeptide.c_str());
               exit(1);
            }

            if (bCountOnly)
               g_iFragmentIndexOffset[iBinBion] += 1;
            else
               g_iFragmentIndex[s_iWritePos[iBinBion]++] = static_cast<unsigned int>(iWhichFragmentPeptide);
         }

         if (dYion > g_staticParams.options.dFragIndexMinMass && dYion < g_staticParams.options.dFragIndexMaxMass)
         {
            int iBinYion = BIN(dYion);

            if ((unsigned int)iBinYion >= g_massRange.uiMaxFragmentArrayIndex)
            {
               printf(" Error: FI dYion %lf too large, pep %s\n", dYion, sPeptide.c_str());
               exit(1);
            }

            if (bCountOnly)
               g_iFragmentIndexOffset[iBinYion] += 1;
            else
               g_iFragmentIndex[s_iWritePos[iBinYion]++] = static_cast<unsigned int>(iWhichFragmentPeptide);
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
bool CometFragmentIndex::GeneratePlainPeptideIndex(ThreadPool* tp, vector<pair<size_t,size_t>>& slices)
{
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
      vector<comet_fileoffset_t> prots_flat;  // all protein file offsets concatenated
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

         vector<comet_fileoffset_t> prot;
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
               dbi.siVarModProteinFilter     = rep.siVarModProteinFilter;
               dbi.lIndexProteinFilePosition = (comet_fileoffset_t)(r.prots_cnt.size() - 1);
               dbi.pcVarModSites.clear();
               r.dbIdx.push_back(dbi);

               iRunStart = i;
            }
            if (i < buf.size())
               prot.push_back(buf[i].lProteinFileOffset);
         }

         vector<PepGenTuple>().swap(buf);
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
         vector<comet_fileoffset_t> prot;
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
               dbi.siVarModProteinFilter     = rep.siVarModProteinFilter;
               dbi.lIndexProteinFilePosition = (comet_fileoffset_t)(r.prots_cnt.size() - 1);
               dbi.pcVarModSites.clear();
               r.dbIdx.push_back(dbi);

               iRunStart = i;
            }
            if (i < buf.size())
               prot.push_back(buf[i].lProteinFileOffset);
         }

         vector<PepGenTupleShort>().swap(buf);
      });
   }

   tp->wait_on_threads();
   g_vvvPepGenShort.clear();
   g_vvvPepGenLong.clear();

   // Sequential merge: fix up lIndexProteinFilePosition (local->global offset),
   // append to global vectors, and record each non-empty length's slice boundary.
   // Long results first to match submission order, then short results.
   size_t iProtBase = 0;
   auto mergeResults = [&](vector<LenResult>& results)
   {
      for (auto& r : results)
      {
         for (auto& dbi : r.dbIdx)
            dbi.lIndexProteinFilePosition += (comet_fileoffset_t)iProtBase;
         iProtBase += r.prots_cnt.size();
         g_pvProteinsList.append_flat(r.prots_flat, r.prots_cnt);
         if (!r.dbIdx.empty())
         {
            const size_t iStart = g_pvDBIndex.size();
            const size_t iCount = r.dbIdx.size();
            for (auto& dbi : r.dbIdx)
               g_pvDBIndex.push_back(std::move(dbi));
            slices.push_back({iStart, iCount});
         }
      }
   };

   mergeResults(longResults);
   mergeResults(shortResults);

   if (g_pvDBIndex.empty())
      return true;   // caller prints the "no peptides" error

   // Parallel per-length mass sort: each slice is a disjoint range of g_pvDBIndex;
   // tasks run concurrently with no data races.  Replaces the single global sort.
   for (auto& [iStart, iCount] : slices)
   {
      tp->doJob([iStart, iCount]()
      {
         sort(g_pvDBIndex.begin() + iStart,
              g_pvDBIndex.begin() + iStart + iCount,
              CometMassSpecUtils::DBICompareByMass);
      });
   }
   tp->wait_on_threads();

   return true;
}


// WriteFIPlainPeptideIndex() and ReadPlainPeptideIndex() retired
// (docs/20260730_PI_reduction.md Phase 0): both are superseded by
// CometPeptideIndex::WritePeptideIndex()/ReadPeptideIndex(), which produce/consume the
// unified index format shared by PI_DB and FI_DB search modes. See CreateFragmentIndex()
// above (build dispatch is in CometSearchManager.cpp).
