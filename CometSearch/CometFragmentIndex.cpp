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


#include "CometFragmentIndex.h"
#include "CometSearch.h"
#include "ThreadPool.h"
#include "CometStatus.h"
#include "CometMassSpecUtils.h"
#include "ModificationsPermuter.h"

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <bitset>


vector<ModificationNumber> MOD_NUMBERS;
vector<string> MOD_SEQS;    // Unique modifiable sequences.
int* MOD_SEQ_MOD_NUM_START; // Start index in the MOD_NUMBERS vector for a modifiable sequence; -1 if no modification numbers were generated
int* MOD_SEQ_MOD_NUM_CNT;   // Total modifications numbers for a modifiable sequence.
int* PEPTIDE_MOD_SEQ_IDXS;  // Index into the MOD_SEQS vector; -1 for peptides that have no modifiable amino acids; -2 if only terminal mods.
int MOD_NUM = 0;

Mutex CometFragmentIndex::_vFragmentPeptidesMutex;


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


bool CometFragmentIndex::CreateFragmentIndex(ThreadPool *tp)
{
   if (!g_bPlainPeptideIndexRead)
      ReadPlainPeptideIndex();

   // vFragmentPeptides is vector of modified peptides
   // - raw peptide via iWhichPeptide referencing entry in g_vRawPeptides to access peptide and protein(s)
   // - modification encoding index
   // - modification mass

   int iNumFragmentThreads = g_staticParams.options.iNumThreads > FRAGINDEX_MAX_THREADS ? FRAGINDEX_MAX_THREADS : g_staticParams.options.iNumThreads;

   for (int iWhichThread = 0; iWhichThread < iNumFragmentThreads; ++iWhichThread)
   {
      for (int iPrecursorBin = 0; iPrecursorBin < FRAGINDEX_PRECURSORBINS; ++iPrecursorBin)
      {
         g_iFragmentIndex[iWhichThread][iPrecursorBin] = new unsigned int*[g_massRange.g_uiMaxFragmentArrayIndex];
         g_iCountFragmentIndex[iWhichThread][iPrecursorBin] = new unsigned int[g_massRange.g_uiMaxFragmentArrayIndex];
      }
   }

   // generate the modified peptides to calculate the fragment index
   GenerateFragmentIndex(tp);

   return true;
}


void CometFragmentIndex::PermuteIndexPeptideMods(vector<PlainPeptideIndex>& g_vRawPeptides)
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

   auto tStartTime = chrono::steady_clock::now();
   cout <<  "   - get modification combinations ... "; fflush(stdout);
   // Get the modification combinations for each unique modifiable substring
   ModificationsPermuter::getModificationCombinations(MOD_SEQS, vMaxNumVarModsPerMod, ALL_MODS,
         MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);
   cout << ElapsedTime(tStartTime) << endl;
}


void CometFragmentIndex::GenerateFragmentIndex(ThreadPool *tp)
{
   cout <<  " - generate fragment index\n"; fflush(stdout);
   auto tStartTime = chrono::steady_clock::now();

   Threading::CreateMutex(&_vFragmentPeptidesMutex);

   ThreadPool *pFragmentIndexPool = tp;

   int iNumIndexingThreads = g_staticParams.options.iNumThreads;

   // Will create a fragment index for each thread, each of which will need
   // to be queried, so limit this number to FRAGINDEX_MAX_THREADS
   if (iNumIndexingThreads > FRAGINDEX_MAX_THREADS)
      iNumIndexingThreads = FRAGINDEX_MAX_THREADS;

   // Create N number of threads, each of which will iterate through
   // a subset of peptides to calculate their fragment ions

   cout <<  "   - count fragment index vector sizes ... "; fflush(stdout);
   // stupid workaround for Windows/Visual Studio performance ... first calculate all
   // fragments to find size of each fragment on index vector
   for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
   {
      for (int iPrecursorBin = 0; iPrecursorBin < FRAGINDEX_PRECURSORBINS; ++iPrecursorBin)
      {
         for (unsigned int iMass = 0; iMass < g_massRange.g_uiMaxFragmentArrayIndex; ++iMass)
         {
            g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iMass] = 0;
         }
      }
   }

   for (int iWhichThread = 0; iWhichThread<iNumIndexingThreads; ++iWhichThread)
      pFragmentIndexPool->doJob(std::bind(AddFragmentsThreadProc, iWhichThread, iNumIndexingThreads, 1, pFragmentIndexPool));

   pFragmentIndexPool->wait_on_threads();

   cout << ElapsedTime(tStartTime) << endl;

   tStartTime = chrono::steady_clock::now();
   cout << "   - reserve memory ... "; fflush(stdout);

   // now reserve memory for the fragment index vectors
   for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
   {
      for (int iPrecursorBin = 0; iPrecursorBin < FRAGINDEX_PRECURSORBINS; ++iPrecursorBin)
      {
         for (unsigned int iMass = 0; iMass < g_massRange.g_uiMaxFragmentArrayIndex; ++iMass)
         {
            if (g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iMass] > 0)
            {
               g_iFragmentIndex[iWhichThread][iPrecursorBin][iMass] = new unsigned int[g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iMass]];
               g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iMass] = 0;  // reset to zero as this will  be used to determine g_iFragmentIndex fill position
            }
            else
               g_iFragmentIndex[iWhichThread][iPrecursorBin][iMass] = NULL;
         }
      }
   }

   cout << ElapsedTime(tStartTime) << endl;

   tStartTime = chrono::steady_clock::now();
   cout <<  "   - populating index ... "; fflush(stdout);

   // now populate the fragment index vector
   for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
      pFragmentIndexPool->doJob(std::bind(AddFragmentsThreadProc, iWhichThread, iNumIndexingThreads, 0, pFragmentIndexPool));

   pFragmentIndexPool->wait_on_threads();

   cout << ElapsedTime(tStartTime) << endl;

   tStartTime = chrono::steady_clock::now();
   cout <<  "   - sorting fragment mass bins by peptide mass ... "; fflush(stdout);

   // sort list of peptides at each fragment bin by peptide mass
   for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
      pFragmentIndexPool->doJob(std::bind(SortFragmentThreadProc, iWhichThread, pFragmentIndexPool));

   pFragmentIndexPool->wait_on_threads();

   Threading::DestroyMutex(_vFragmentPeptidesMutex);

   cout << ElapsedTime(tStartTime) << endl;

   unsigned long long ullCount = 0;
   unsigned long long ullMax = 0;
   for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
   {
      for (int iPrecursorBin = 0; iPrecursorBin < FRAGINDEX_PRECURSORBINS; ++iPrecursorBin)
      {
         for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; ++i)
         {
            // count and report the # of entries in the fragment index
            unsigned long long ullTmp = g_iCountFragmentIndex[iWhichThread][iPrecursorBin][i];
            ullCount += ullTmp;
            if (ullTmp > ullMax)
               ullMax = ullTmp;
         }
      }
   }
   printf("   - total # of entries in the fragment index: %llu (max %llu)\n", ullCount, ullMax);
}


// will return string "XX min YY sec" of elasped time from tStartTime to now
// pass in tStartTime = chrono::steady_clock::now();
string CometFragmentIndex::ElapsedTime(std::chrono::time_point<std::chrono::steady_clock> tStartTime)
{
   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   auto minutes = duration.count() / 60000;
   auto seconds = (duration.count() - minutes * 60000) / 1000;

   string sReturn;
   if (minutes > 0)
      sReturn = std::to_string(minutes) + " min " + std::to_string(seconds) + " sec";
   else
      sReturn = std::to_string(seconds) + " sec";

   return sReturn;
}


void CometFragmentIndex::AddFragmentsThreadProc(int iWhichThread,
                                                int iNumIndexingThreads,
                                                bool bCountOnly,
                                                ThreadPool *tp)
{
   // each thread will loop through a subset of the g_vRawPeptides
   for (size_t iWhichPeptide = iWhichThread; iWhichPeptide < g_vRawPeptides.size(); iWhichPeptide += iNumIndexingThreads)
   {
      // AddFragments for unmodified peptide
      // FIX: if require variable mod is set, this would not be called here
      AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, -1, -1, bCountOnly);

      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];

      // Possibly analyze peptides with a terminal mod and no variable mod on any residue
      if (g_staticParams.variableModParameters.bVarTermModSearch)
      {
         // Add any n-term variable mods
         for (short ctNtermMod=0; ctNtermMod<FRAGINDEX_VMODS; ++ctNtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod
                     && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)))
            {
               AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, ctNtermMod, -1, bCountOnly);
            }
         }

         // Add any c-term variable mods
         for (short ctCtermMod=0; ctCtermMod< FRAGINDEX_VMODS; ++ctCtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod
                     && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod)))
            {
               AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, -1, ctCtermMod, bCountOnly);
            }
         }

         // Now consider combinations of n-term and c-term variable mods
         for (short ctNtermMod=0; ctNtermMod< FRAGINDEX_VMODS; ++ctNtermMod)
         {
            for (short ctCtermMod=0; ctCtermMod< FRAGINDEX_VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod
                     && g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod
                     && (!g_staticParams.variableModParameters.bVarModProteinFilter ||
                           (cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)
                         && cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod))))
               {
                  AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, ctNtermMod, ctCtermMod, bCountOnly);
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
               AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, -1, -1, bCountOnly);

               if (g_staticParams.variableModParameters.bVarTermModSearch)
               {
                  // Add any n-term variable mods
                  for (short ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
                  {
                     if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod
                              && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)))
                     {
                        AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, ctNtermMod, -1, bCountOnly);
                     }
                  }

                  // Add any c-term variable mods
                  for (short ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
                  {
                     if (g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod
                              && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod)))
                     {
                        AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, -1, ctCtermMod, bCountOnly);
                     }
                  }

                  // Now consider combinations of n-term and c-term variable mods
                  for (short ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
                  {
                     for (short ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
                     {
                        if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod
                                 && g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod
                                 && (!g_staticParams.variableModParameters.bVarModProteinFilter ||
                                       (cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctNtermMod)
                                     && cometbitcheck(g_vRawPeptides.at(iWhichPeptide).siVarModProteinFilter, ctCtermMod))))
                        {
                           AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, ctNtermMod, ctCtermMod, bCountOnly);
                        }
                     }
                  }
               }
            }
         }
      }
   }
}


void CometFragmentIndex::SortFragmentThreadProc(int iWhichThread,
                                                ThreadPool *tp)
{
   for (int iPrecursorBin = 0; iPrecursorBin < FRAGINDEX_PRECURSORBINS; ++iPrecursorBin)
   {
      for (unsigned int iFragmentBin = 0; iFragmentBin < g_massRange.g_uiMaxFragmentArrayIndex; ++iFragmentBin)
      {
         if (g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iFragmentBin] > 0)
         {
            std::sort(g_iFragmentIndex[iWhichThread][iPrecursorBin][iFragmentBin],
               g_iFragmentIndex[iWhichThread][iPrecursorBin][iFragmentBin] + g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iFragmentBin],
               SortFragmentsByPepMass);
         }
      }
   }
}


bool CometFragmentIndex::SortFragmentsByPepMass(unsigned int x, unsigned int y)
{
   if (g_vFragmentPeptides[x].dPepMass == g_vFragmentPeptides[y].dPepMass)
      return (x < y);
   else
      return (g_vFragmentPeptides[x].dPepMass < g_vFragmentPeptides[y].dPepMass);
}


void CometFragmentIndex::AddFragments(vector<PlainPeptideIndex>& g_vRawPeptides,
                                      int iWhichThread,
                                      int iWhichPeptide,
                                      int modNumIdx,
                                      short siNtermMod,
                                      short siCtermMod,
                                      bool bCountOnly)
{
   string sPeptide = g_vRawPeptides.at(iWhichPeptide).sPeptide;

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

   // first calculate peptide mass as that's needed in fragment loop
   j = 0;
   for (int i = 0; i <= iEndPos; ++i)
   {
      dCalcPepMass += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];

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

   if (siNtermMod >= 0)  // if -1, unused
   {
      dBion += g_staticParams.variableModParameters.varModList[(int)siNtermMod].dVarModMass;
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)siNtermMod].dVarModMass;
   }
   if (siCtermMod >= 0)  // if -1, unused
   {
      dYion += g_staticParams.variableModParameters.varModList[(int)siCtermMod].dVarModMass;
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)siCtermMod].dVarModMass;
   }

   if (dCalcPepMass > 99999.9)
   {
      printf(" Error, pepmass in AddFragments is %f, peptide %s, modNumIdx %d\n", dCalcPepMass, sPeptide.c_str(), modNumIdx);
      exit(1);
   }

   if (dCalcPepMass > g_massRange.dMaxMass || dCalcPepMass < g_staticParams.options.dPeptideMassLow)
      return;

   if (!g_staticParams.options.iFragIndexSkipReadPrecursors && !g_bIndexPrecursors[BIN(dCalcPepMass)])
      return;

   unsigned int uiCurrentFragmentPeptide = -1;

   if (!bCountOnly)
   {
      struct FragmentPeptidesStruct sTmp;
      sTmp.iWhichPeptide = iWhichPeptide;
      sTmp.modNumIdx = modNumIdx;
      sTmp.dPepMass = dCalcPepMass;
      sTmp.siNtermMod = siNtermMod;
      sTmp.siCtermMod = siCtermMod;

      // Store the current peptide; uiCurrentFragmentPeptide references this peptide entry
      // for use in the g_iFragmentIndex fragment index.  as this is a global list of
      // peptides, need to lock when updating to avoid thread conflicts

      Threading::LockMutex(_vFragmentPeptidesMutex);
      uiCurrentFragmentPeptide = (unsigned int)g_vFragmentPeptides.size();  // index of current peptide in g_vFragmentPeptides
      if (g_vFragmentPeptides.size() >= UINT_MAX)
      {
         printf(" Error in CometFragmentIndex; UINT_MAX (%d) peptides reached.\n", UINT_MAX);
         exit(1);
      }
      // store peptide representation based on sequence (iWhichPeptide), modification state (modNumIdx), and mass (dPepMass)
      g_vFragmentPeptides.push_back(sTmp);
      Threading::UnlockMutex(_vFragmentPeptidesMutex);
   }

/*
if (!(iWhichPeptide%5000))
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
            printf("%s", to_string(mods[j]).c_str());
         }
         j++;
      }
   }
   printf("\t%f\t%d\t%s\n", dCalcPepMass, modNumIdx, modSeq.c_str());
}
*/

   int iPrecursorBin = WhichPrecursorBin(dCalcPepMass);

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

            if (iBinBion >= g_massRange.g_uiMaxFragmentArrayIndex)
            {
               printf(" Error: FI dBion %lf too large, pep %s\n", dBion, sPeptide.c_str());
               exit(1);
            }

            if (bCountOnly)
               g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iBinBion] += 1;
            else
            {
               int iEntry = g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iBinBion];

               g_iFragmentIndex[iWhichThread][iPrecursorBin][iBinBion][iEntry] = uiCurrentFragmentPeptide;
               g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iBinBion] += 1;
            }
         }

         if (dYion > g_staticParams.options.dFragIndexMinMass && dYion < g_staticParams.options.dFragIndexMaxMass)
         {
            int iBinYion = BIN(dYion);

            if (iBinYion >= g_massRange.g_uiMaxFragmentArrayIndex)
            {
               printf(" Error: FI dYion %lf too large, pep %s\n", dYion, sPeptide.c_str());
               exit(1);
            }

            if (bCountOnly)
               g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iBinYion] += 1;
            else
            {
               int iEntry = g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iBinYion];

               g_iFragmentIndex[iWhichThread][iPrecursorBin][iBinYion][iEntry] = uiCurrentFragmentPeptide;
               g_iCountFragmentIndex[iWhichThread][iPrecursorBin][iBinYion] += 1;
            }
         }
      }
   }
}


bool CometFragmentIndex::WritePlainPeptideIndex(ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded;
   bool bSwapIdxExtension = false;
   string strOut;

   string strIndexFile;

   if (strstr(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
   {
      strIndexFile = g_staticParams.databaseInfo.szDatabase;  // .idx specified but not present to create it
      g_staticParams.databaseInfo.szDatabase[strlen(g_staticParams.databaseInfo.szDatabase) - 4] = '\0';
      bSwapIdxExtension = true;  // need to make database regular fasta, then RunSearch to get plain peptides, then swap back
   }
   else
      strIndexFile = g_staticParams.databaseInfo.szDatabase + string(".idx");  // fasta specified so add .idx extension

   if ((fp = fopen(strIndexFile.c_str(), "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", strIndexFile.c_str());
      exit(1);
   }

   strOut = " Creating plain peptide/protein index file for fragment ion indexing:\n";
   logout(strOut.c_str());
   fflush(stdout);
   strOut = " - parse peptides from database ... ";
   logout(strOut.c_str());
   fflush(stdout);

   // Allocate memory shared by threads during search
   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
       return bSucceeded;

   if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
      g_massRange.bNarrowMassRange = true;
   else
      g_massRange.bNarrowMassRange = false;

   if (bSucceeded)
   {
      g_staticParams.options.bCreateFragmentIndex = true;
      g_staticParams.iIndexDb = 0;

      // this step calls RunSearch just to pull out all peptides
      // to write into the .idx pepties/proteins file
      bSucceeded = CometSearch::RunSearch(0, 0, tp);

      g_staticParams.options.bCreateFragmentIndex = false;
      g_staticParams.iIndexDb = 1;
   }

   if (bSwapIdxExtension)
      strcat(g_staticParams.databaseInfo.szDatabase, ".idx");

   if (!bSucceeded)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error performing RunSearch() to create indexed database. \n");
      logerr(szErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // sanity check
   if (g_pvDBIndex.size() == 0)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error - no peptides in index; check the input database file.\n");
      logerr(szErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // remove duplicates
   strOut = " - remove duplicate peptides\n";
   logout(strOut.c_str());
   fflush(stdout);

   // first sort by peptide then protein file position
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CompareByPeptide);

   // At this point, need to create g_pvProteinsList protein file position vector of vectors to map each peptide
   // to every protein. g_pvdbindex.at().lproteinfileposition is now reference to protein vector entry
   vector<comet_fileoffset_t> temp;  // stores list of duplicate proteins which gets pushed to g_pvproteinslist

   // Create g_pvProteinsList.  This is a vector of vectors.  Each element is a vector list
   // of duplicate proteins (generated as "temp") ... these are generated by looping
   // through g_pvDBIndex and looking for consecutive, same peptides.  Once the "temp"
   // vector is assigned the lIndexProteinFilePosition offset, the g_pvDBIndex entry is
   // is assigned lProtCount to lIndexProteinFilePosition.  This is used later to look up
   // the right vector element of duplicate proteins later.
   long lProtCount = 0;
   for (size_t i = 0; i < g_pvDBIndex.size(); ++i)
   {
      if (i == 0)
      {
         temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
         g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
      }
      else
      {
         // each unique peptide will have the same list of matched proteins
         if (!strcmp(g_pvDBIndex.at(i).szPeptide, g_pvDBIndex.at(i-1).szPeptide))
         {
            // store protein as peptides are the same
            temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
            g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
         }
         else
         {
            // different peptide so go ahead and push temp onto g_pvProteinsList
            // and store current protein reference into new temp
            sort(temp.begin(), temp.end());
            temp.erase(unique(temp.begin(), temp.end()), temp.end() );
            g_pvProteinsList.push_back(temp);

            lProtCount++; // start new row in g_pvProteinsList
            temp.clear();
            temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
            g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
         }
      }
   }
   // now at end of loop, push last temp onto g_pvProteinsList
   sort(temp.begin(), temp.end());
   temp.erase(unique(temp.begin(), temp.end()), temp.end() );
   g_pvProteinsList.push_back(temp);

   g_pvDBIndex.erase(unique(g_pvDBIndex.begin(), g_pvDBIndex.end()), g_pvDBIndex.end());

   // sort by mass;
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CompareByMass);

   cout << " - write peptides/proteins to file" << endl;

   // write out index header
   fprintf(fp, "Comet fragment ion index plain peptides.  Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fp, "InputDB:  %s\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fp, "MassRange: %lf %lf\n", g_staticParams.options.dPeptideMassLow, g_staticParams.options.dPeptideMassHigh);
   fprintf(fp, "LengthRange: %d %d\n", g_staticParams.options.peptideLengthRange.iStart, g_staticParams.options.peptideLengthRange.iEnd);
   fprintf(fp, "MassType: %d %d\n", g_staticParams.massUtility.bMonoMassesParent, g_staticParams.massUtility.bMonoMassesFragment);
   fprintf(fp, "Enzyme: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzymeName,
      g_staticParams.enzymeInformation.iSearchEnzymeOffSet, 
      g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, 
      g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
   fprintf(fp, "Enzyme2: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzyme2Name,
      g_staticParams.enzymeInformation.iSearchEnzyme2OffSet, 
      g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA, 
      g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
   fprintf(fp, "NumPeptides: %ld\n", (long)g_pvDBIndex.size());

   // write out static mod params A to Z is ascii 65 to 90 then terminal mods
   fprintf(fp, "StaticMod:");
   for (int x = 65; x <= 90; ++x)
      fprintf(fp, " %lf", g_staticParams.staticModifications.pdStaticMods[x]);
   fprintf(fp, " %lf", g_staticParams.staticModifications.dAddNterminusPeptide);
   fprintf(fp, " %lf", g_staticParams.staticModifications.dAddCterminusPeptide);
   fprintf(fp, " %lf", g_staticParams.staticModifications.dAddNterminusProtein);
   fprintf(fp, " %lf\n", g_staticParams.staticModifications.dAddCterminusProtein);

   // write VariableMod:
   fprintf(fp, "VariableMod:");
   for (int x = 0; x < FRAGINDEX_VMODS; ++x)
   {
      fprintf(fp, " %s:%lf:%lf:%lf",
         g_staticParams.variableModParameters.varModList[x].szVarModChar,
         g_staticParams.variableModParameters.varModList[x].dVarModMass,
         g_staticParams.variableModParameters.varModList[x].dNeutralLoss,
         g_staticParams.variableModParameters.varModList[x].dNeutralLoss2);
   }
   fprintf(fp, "\n");

   // Variable mod protein filter:
   fprintf(fp, "ProteinModList: %d\n", g_staticParams.variableModParameters.bVarModProteinFilter?1:0);

   comet_fileoffset_t clPeptidesFilePos = comet_ftell(fp);
   size_t tNumPeptides = g_pvDBIndex.size();
   fwrite(&tNumPeptides, sizeof(size_t), 1, fp);  // write # of peptides

   for (std::vector<DBIndex>::iterator it = g_pvDBIndex.begin(); it != g_pvDBIndex.end(); ++it)
   {
      int iLen = (int)strlen((*it).szPeptide);
      struct PlainPeptideIndex sTmp;

      fwrite(&iLen, sizeof(int), 1, fp);
      fwrite((*it).szPeptide, sizeof(char), iLen, fp);
      fwrite(&((*it).dPepMass), sizeof(double), 1, fp);
      fwrite(&((*it).siVarModProteinFilter), sizeof(unsigned short), 1, fp);
      fwrite(&((*it).lIndexProteinFilePosition), clSizeCometFileOffset, 1, fp);

      sTmp.sPeptide = (*it).szPeptide;
      sTmp.lIndexProteinFilePosition = clSizeCometFileOffset;
      sTmp.dPepMass = (*it).dPepMass;
      sTmp.siVarModProteinFilter = (*it).siVarModProteinFilter;
      g_vRawPeptides.push_back(sTmp);
   }

   // Now write out: vector<vector<comet_fileoffset_t>> g_pvProteinsList
   comet_fileoffset_t clProteinsFilePos = comet_ftell(fp);
   size_t tTmp = g_pvProteinsList.size();
   fwrite(&tTmp, clSizeCometFileOffset, 1, fp);
   for (auto it = g_pvProteinsList.begin(); it != g_pvProteinsList.end(); ++it)
   {
      tTmp = (*it).size();
      fwrite(&tTmp, sizeof(size_t), 1, fp);
      for (size_t it2 = 0; it2 < tTmp; ++it2)
      {
         fwrite(&((*it).at(it2)), clSizeCometFileOffset, 1, fp);
      }
   }

   // now permute mods on the peptides
   PermuteIndexPeptideMods(g_vRawPeptides);
 
   unsigned long ulSizeModSeqs = (unsigned long)MOD_SEQS.size();              // size of MOD_SEQS
   unsigned long ulSizevRawPeptides = (unsigned long)g_vRawPeptides.size();   // size of g_vRawPeptides
   unsigned long ulModNumSize = (unsigned long)MOD_NUMBERS.size();            // size of MOD_NUMBERS

   comet_fileoffset_t clPermutationsFilePos = comet_ftell(fp);

   fwrite(&ulSizeModSeqs, sizeof(unsigned long), 1, fp);
   fwrite(&ulSizevRawPeptides, sizeof(unsigned long), 1, fp);
   fwrite(&ulModNumSize, sizeof(unsigned long), 1, fp);
   fwrite(MOD_SEQ_MOD_NUM_START, sizeof(int), ulSizeModSeqs, fp);
   fwrite(MOD_SEQ_MOD_NUM_CNT, sizeof(int), ulSizeModSeqs, fp);
   fwrite(PEPTIDE_MOD_SEQ_IDXS, sizeof(int), ulSizevRawPeptides, fp);
   int iTmp;
   for (unsigned long i = 0; i < ulSizeModSeqs; ++i)
   {
      iTmp = (int)MOD_SEQS[i].size();
      fwrite(&iTmp, sizeof(int), 1, fp); // write length
      fwrite(MOD_SEQS[i].c_str(), 1, iTmp, fp);
   }
   for (unsigned long i = 0; i < ulModNumSize; ++i)
   {
      fwrite(&(MOD_NUMBERS[i].modStringLen), sizeof(int), 1, fp);
      fwrite(MOD_NUMBERS[i].modifications, 1, MOD_NUMBERS[i].modStringLen, fp);
   }

   fwrite(&clPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   fwrite(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);
   fwrite(&clPermutationsFilePos, clSizeCometFileOffset, 1, fp);

   g_pvDBIndex.clear();

   fclose(fp);

   strOut = " - done. " + strIndexFile + " (" + to_string(tNumPeptides) + " peptides)\n\n";
   logout(strOut.c_str());
   fflush(stdout);

   return bSucceeded;
}


// read the raw peptides from disk
bool CometFragmentIndex::ReadPlainPeptideIndex(void)
{
   FILE *fp;
   size_t tTmp;
   char szBuf[SIZE_BUF];
   string strIndexFile;

   if (g_bPlainPeptideIndexRead)
      return 1;

   if (g_staticParams.options.bCreateFragmentIndex && !strstr(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
      strIndexFile = g_staticParams.databaseInfo.szDatabase + string(".idx");
   else // database already is .idx
      strIndexFile = g_staticParams.databaseInfo.szDatabase;

   if ((fp = fopen(strIndexFile.c_str(), "rb")) == NULL)
   {
      printf(" Error - cannot open index file %s to read\n", strIndexFile.c_str());
      exit(1);
   }

   bool bFoundStatic = false;
   bool bFoundVariable= false;

   while (fgets(szBuf, SIZE_BUF, fp))
   {
      if (!strncmp(szBuf, "MassType:", 9))
      {
         int iRet = sscanf(szBuf + 9, "%d %d", &g_staticParams.massUtility.bMonoMassesParent, &g_staticParams.massUtility.bMonoMassesFragment);
         
         if (iRet != 2)
         {
            char szErr[256];
            sprintf(szErr, " Error with raw peptide index database format. MassType: did not parse 2 values.");
            logerr(szErr);
            fclose(fp);
            return false;
         }
      }
      else if (!strncmp(szBuf, "LengthRange:", 12))
      {
         int iRet = sscanf(szBuf + 12, "%d %d", &g_staticParams.options.peptideLengthRange.iStart, &g_staticParams.options.peptideLengthRange.iEnd);

         if (iRet != 2)
         {
            char szErr[256];
            sprintf(szErr, " Error with raw peptide index database format. LengthRange: did not parse 2 values.");
            logerr(szErr);
            fclose(fp);
            return false;
         }
      }
      else if (!strncmp(szBuf, "Enzyme:", 7))
      {
         int iRet = sscanf(szBuf + 7, "%*s [%d %s %s]", &g_staticParams.enzymeInformation.iSearchEnzymeOffSet,
            g_staticParams.enzymeInformation.szSearchEnzymeBreakAA,
            g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);

         if (iRet != 3)
         {
            char szErr[256];
            sprintf(szErr, " Error with raw peptide index database format. Enzyme: did not parse 3 values.");
            logerr(szErr);
            fclose(fp);
            return false;
         }
      }
      else if (!strncmp(szBuf, "Enzyme2:", 8))
      {
         int iRet = sscanf(szBuf + 8, "%*s [%d %s %s]", &g_staticParams.enzymeInformation.iSearchEnzyme2OffSet,
            g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA,
            g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);

         if (iRet != 3)
         {
            char szErr[256];
            sprintf(szErr, " Error with raw peptide index database format. Enzyme2: did not parse 3 values.");
            logerr(szErr);
            fclose(fp);
            return false;
         }
      }
      else if (!strncmp(szBuf, "StaticMod:", 10)) // read in static mods
      {
         char *tok;
         char delims[] = " ";
         int x=65;

         // FIX:  hack here for setting static mods; need to reset masses ... fix later
         CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassFragment,
            g_staticParams.massUtility.bMonoMassesFragment,
            &g_staticParams.massUtility.dOH2fragment);

         bFoundStatic = true;
         tok=strtok(szBuf+11, delims);
         while (tok != NULL)
         {
            int iRet = sscanf(tok, "%lf", &(g_staticParams.staticModifications.pdStaticMods[x]));
            g_staticParams.massUtility.pdAAMassFragment[x] += g_staticParams.staticModifications.pdStaticMods[x];
            tok = strtok(NULL, delims);
            x++;
            if (x==95)  // 65-90 stores A-Z then next 4 (ascii 91-94) are n/c term peptide, n/c term protein
               break;
         }

         g_staticParams.staticModifications.dAddNterminusPeptide = g_staticParams.staticModifications.pdStaticMods[91];
         g_staticParams.staticModifications.dAddCterminusPeptide = g_staticParams.staticModifications.pdStaticMods[92];
         g_staticParams.staticModifications.dAddNterminusProtein = g_staticParams.staticModifications.pdStaticMods[93];
         g_staticParams.staticModifications.dAddCterminusProtein = g_staticParams.staticModifications.pdStaticMods[94];

         // have to set these here again once static mods are read
         g_staticParams.precalcMasses.dNtermProton = g_staticParams.staticModifications.dAddNterminusPeptide
            + PROTON_MASS;

         g_staticParams.precalcMasses.dCtermOH2Proton = g_staticParams.staticModifications.dAddCterminusPeptide
            + g_staticParams.massUtility.dOH2fragment
            + PROTON_MASS;

         g_staticParams.precalcMasses.dOH2ProtonCtermNterm = g_staticParams.massUtility.dOH2parent
            + PROTON_MASS
            + g_staticParams.staticModifications.dAddCterminusPeptide
            + g_staticParams.staticModifications.dAddNterminusPeptide;

         bFoundStatic = true;
      }
      else if (!strncmp(szBuf, "VariableMod:", 12)) // read in variable mods
      {
         string strMods = szBuf + 13;

         istringstream iss(strMods);

         int iNumMods = 0;

         do
         {
            string subStr;

            iss >> subStr;  // parse each word which is a colon delimited triplet pair for modmass:neutralloss:modchars
            std::replace(subStr.begin(), subStr.end(), ':', ' ');
            int iRet = sscanf(subStr.c_str(), "%s %lf %lf %lf",
               g_staticParams.variableModParameters.varModList[iNumMods].szVarModChar,
               &(g_staticParams.variableModParameters.varModList[iNumMods].dVarModMass),
               &(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss),
               &(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss2));

            if (g_staticParams.variableModParameters.varModList[iNumMods].dVarModMass != 0.0)
               g_staticParams.variableModParameters.bVarModSearch = true;

            if (g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss != 0.0)
               g_staticParams.variableModParameters.bUseFragmentNeutralLoss = true;

            iNumMods++;

            if (iNumMods == FRAGINDEX_VMODS)
               break;

         } while (iss);

         bFoundVariable = true;
      }
      else if (!strncmp(szBuf, "ProteinModList:", 15))
      {
         int iTmp;

         int iRet = sscanf(szBuf + 16, "%d", &iTmp);

         if (iTmp)
            g_staticParams.variableModParameters.bVarModProteinFilter = true;

         break;
      }
   }

   if (!bFoundStatic || !bFoundVariable)
   {
      char szErr[256];
      sprintf(szErr, " Error with raw peptide index database format. Modifications (%d/%d) not parsed.", bFoundStatic, bFoundVariable);
      logerr(szErr);
      fclose(fp);
      return false;
   }

   comet_fileoffset_t clTmp;
   comet_fileoffset_t clPeptidesFilePos;      // file position of raw peptides
   comet_fileoffset_t clProteinsFilePos;      // file position of g_pvProteinsList
   comet_fileoffset_t clPermutationsFilePos;  // file position of permutations variables

   comet_fseek(fp, -clSizeCometFileOffset*3, SEEK_END);
   tTmp = fread(&clPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   tTmp = fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);
   tTmp = fread(&clPermutationsFilePos, clSizeCometFileOffset, 1, fp);

   comet_fseek(fp, clPeptidesFilePos, SEEK_SET);

   size_t tNumPeptides;
   tTmp = fread(&tNumPeptides, sizeof(size_t), 1, fp);  // read # of peptides

   struct PlainPeptideIndex sTmp;
   int iLen;
   char szPeptide[MAX_PEPTIDE_LEN];

   g_vRawPeptides.clear();
   for (size_t it = 0; it < tNumPeptides; ++it)
   {
      tTmp = fread(&iLen, sizeof(int), 1, fp);
      tTmp = fread(szPeptide, sizeof(char), iLen, fp);
      szPeptide[iLen] = '\0';
      sTmp.sPeptide = szPeptide;
      tTmp = fread(&(sTmp.dPepMass), sizeof(double), 1, fp);
      tTmp = fread(&(sTmp.siVarModProteinFilter), sizeof(unsigned short), 1, fp);
      tTmp = fread(&(sTmp.lIndexProteinFilePosition), clSizeCometFileOffset, 1, fp);

      g_vRawPeptides.push_back(sTmp);
   }

   comet_fseek(fp, clProteinsFilePos, SEEK_SET);  // should be at this file position here anyways already

   // now read in: vector<vector<comet_fileoffset_t>> g_pvProteinsList
   size_t tSize;
   tTmp = fread(&tSize, clSizeCometFileOffset, 1, fp);
   vector<comet_fileoffset_t> vTmp;

   g_pvProteinsList.clear();
   g_pvProteinsList.reserve(tSize);
   for (size_t it = 0; it < tSize; ++it)
   {
      size_t tNumProteinOffsets;
      tTmp = fread(&tNumProteinOffsets, clSizeCometFileOffset, 1, fp);
      
      vTmp.clear();
      for (size_t it2 = 0; it2 < tNumProteinOffsets; ++it2)
      {
         tTmp = fread(&clTmp, clSizeCometFileOffset, 1, fp);
         vTmp.push_back(clTmp);
      }
      g_pvProteinsList.push_back(vTmp);
   }

   comet_fseek(fp, clPermutationsFilePos, SEEK_SET);  // should be at this file position here anyways already

   unsigned long ulSizeModSeqs;        // size of MOD_SEQS
   unsigned long ulSizevRawPeptides;   // size of g_vRawPeptides
   unsigned long ulModNumSize;         // size of MOD_NUMBERS

   tTmp = fread(&ulSizeModSeqs, sizeof(unsigned long), 1, fp);
   tTmp = fread(&ulSizevRawPeptides, sizeof(unsigned long), 1, fp);
   tTmp = fread(&ulModNumSize, sizeof(unsigned long), 1, fp);

   MOD_SEQ_MOD_NUM_START = new int[ulSizeModSeqs];
   MOD_SEQ_MOD_NUM_CNT = new int[ulSizeModSeqs];
   PEPTIDE_MOD_SEQ_IDXS = new int[ulSizevRawPeptides];

   tTmp = fread(MOD_SEQ_MOD_NUM_START, sizeof(int), ulSizeModSeqs, fp);
   tTmp = fread(MOD_SEQ_MOD_NUM_CNT, sizeof(int), ulSizeModSeqs, fp);
   tTmp = fread(PEPTIDE_MOD_SEQ_IDXS, sizeof(int), ulSizevRawPeptides, fp);  //FIX, why??

   int iTmp;
   char szTmp[MAX_PEPTIDE_LEN];
   MOD_SEQS.clear();
   for (unsigned long i = 0; i < ulSizeModSeqs; ++i)
   {
      tTmp = fread(&iTmp, sizeof(int), 1, fp); // read length
      tTmp = fread(szTmp, 1, iTmp, fp);
      szTmp[iTmp]='\0';
      MOD_SEQS.push_back(szTmp);
   }
   MOD_NUMBERS.clear();
   for (unsigned long i = 0; i < ulModNumSize; ++i)
   {
      ModificationNumber sTmp;
      tTmp = fread(&iTmp, sizeof(int), 1, fp); // read length
      tTmp = fread(szTmp, 1, iTmp, fp);
      szTmp[iTmp]='\0';
      sTmp.modStringLen = iTmp;
      sTmp.modifications = new char[iTmp];
      memcpy(sTmp.modifications, szTmp, iTmp);
      MOD_NUMBERS.push_back(sTmp);
   }

   fclose(fp);

   g_bPlainPeptideIndexRead = true;

   return true;
}


// for a given MH+ precursor mass, return the precursor bin value used in g_iFragmentIndex[][bin]
int CometFragmentIndex::WhichPrecursorBin(double dMass)
{
   // need to round up iBinSize
   int iBinSize = (int)(0.5 + (g_staticParams.options.dPeptideMassHigh - g_staticParams.options.dPeptideMassLow)/ FRAGINDEX_PRECURSORBINS);

   int iWhichBin = (int) ( (dMass - g_staticParams.options.dPeptideMassLow) / iBinSize);

   if (iWhichBin < 0)
      iWhichBin = 0;
   else if (iWhichBin > FRAGINDEX_PRECURSORBINS - 1)
      iWhichBin = FRAGINDEX_PRECURSORBINS - 1;

   return (iWhichBin);
}


bool CometFragmentIndex::CompareByPeptide(const DBIndex &lhs,
                                          const DBIndex &rhs)
{
   if (!strcmp(lhs.szPeptide, rhs.szPeptide))
   {
      // peptides are same here so look at mass next
      if (fabs(lhs.dPepMass - rhs.dPepMass) > FLOAT_ZERO)
      {
         // masses are different
         if (lhs.dPepMass < rhs.dPepMass)
            return true;
         else
            return false;
      }

      // FIX: if protein terminal mods are specified, address them

      // at this point, same peptide, same mass, same mods so return first protein
      if (lhs.lIndexProteinFilePosition < rhs.lIndexProteinFilePosition)
         return true;
      else
         return false;
   }

   // peptides are different
   if (strcmp(lhs.szPeptide, rhs.szPeptide)<0)
      return true;
   else
      return false;
};


// sort by mass, then peptide, then modification state, then protein fp location
bool CometFragmentIndex::CompareByMass(const DBIndex &lhs,
                                       const DBIndex &rhs)
{
   if (fabs(lhs.dPepMass - rhs.dPepMass) > FLOAT_ZERO)
   {
      // masses are different
      if (lhs.dPepMass < rhs.dPepMass)
         return true;
      else
         return false;
   }

   // at this point, peptides are same mass so next need to compare sequences

   if (!strcmp(lhs.szPeptide, rhs.szPeptide))
   {
      // same sequences and masses here so next look at mod state
      for (unsigned int i=0; i<strlen(lhs.szPeptide)+2; ++i)
      {
         if (lhs.pcVarModSites[i] != rhs.pcVarModSites[i])
         {
            if (lhs.pcVarModSites[i] > rhs.pcVarModSites[i])
               return true;
            else
               return false;
         }
      }

      // at this point, same peptide, same mass, same mods so return first protein
      if (lhs.lIndexProteinFilePosition < rhs.lIndexProteinFilePosition)
         return true;
      else
         return false;
   }

   // if here, peptide sequences are different (but w/same mass) so sort alphabetically
   if (strcmp(lhs.szPeptide, rhs.szPeptide) < 0)
      return true;
   else
      return false;

}

