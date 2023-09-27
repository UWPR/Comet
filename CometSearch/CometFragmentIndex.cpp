/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Common.h"
#include "CometFragmentIndex.h"
#include "CometSearch.h"
#include "ThreadPool.h"
#include "CometStatus.h"
//#include "CometPostAnalysis.h"
#include "CometMassSpecUtils.h"
#include "ModificationsPermuter.h"

#include <stdio.h>
#include <sstream>
#include <bitset>

#define USEFRAGMENTTHREADS 2    //0=no, 1=calc fragments, 2=sort only


vector<ModificationNumber> MOD_NUMBERS;
vector<string> MOD_SEQS;    // Unique modifiable sequences.
int* MOD_SEQ_MOD_NUM_START; // Start index in the MOD_NUMBERS vector for a modifiable sequence; -1 if no modification numbers were generated
int* MOD_SEQ_MOD_NUM_CNT;   // Total modifications numbers for a modifiable sequence.
int* PEPTIDE_MOD_SEQ_IDXS;  // Index into the MOD_SEQS vector; -1 for peptides that have no modifiable amino acids.
int MOD_NUM = 0;

Mutex CometFragmentIndex::_vFragmentIndexMutex;
Mutex CometFragmentIndex::_vFragmentPeptidesMutex;

//comet_fileoffset_t clSizeCometFileOffset;
#ifdef _WIN32
#ifdef _WIN64
comet_fileoffset_t   clSizeCometFileOffset = sizeof(comet_fileoffset_t);              //win64
#else
comet_fileoffset_t   clSizeCometFileOffset = (long long)sizeof(comet_fileoffset_t);   //win32
#endif
#else
comet_fileoffset_t   clSizeCometFileOffset = sizeof(comet_fileoffset_t);              //linux
#endif

CometFragmentIndex::CometFragmentIndex()
{
}

CometFragmentIndex::~CometFragmentIndex()
{
}


bool CometFragmentIndex::CreateFragmentIndex(ThreadPool *tp)
{
   if (!g_vPlainPeptideIndexRead)
      ReadPlainPeptideIndex();

   // now permute mods on the peptides
   PermuteIndexPeptideMods(g_vRawPeptides);

   // vector of modified peptides
   // - raw peptide via iWhichPeptide referencing entry in g_vRawPeptides to access peptide and protein(s)
   // - modification encoding index
   // - modification mass

   g_massRange.g_uiMaxFragmentArrayIndex = BIN(g_staticParams.options.dMaxFragIndexMass) + 1;

   for (int iWhichThread=0; iWhichThread < (g_staticParams.options.iNumThreads> MAX_FRAGMENTINDEX_THREADS ? MAX_FRAGMENTINDEX_THREADS : g_staticParams.options.iNumThreads) ; ++iWhichThread)
   {
      g_arrvFragmentIndex[iWhichThread] = new vector<unsigned int>[g_massRange.g_uiMaxFragmentArrayIndex];
   }

   // generate the modified peptides to calculate the fragment index
   GenerateFragmentIndex(g_vRawPeptides, tp);

   return true;
}


void CometFragmentIndex::PermuteIndexPeptideMods(vector<PlainPeptideIndex>& g_vRawPeptides)
{
   vector<string> ALL_MODS; // An array of all the user specified amino acids that can be modified

   // Pre-computed bitmask combinations for peptides of length MAX_PEPTIDE_LEN with up
   // to MAX_MODS_PER_MOD modified amino acids.

   // Maximum number of bits that can be set in a modifiable sequence for a given modification.
   // C(25, 5) = 53,130; C(25, 4) = 10,650; C(25, 3) = 2300.  This is more than MAX_COMBINATIONS (65,534)

   for (int i = 0; i < VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
         && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='-'))
      {
         ALL_MODS.push_back(g_staticParams.variableModParameters.varModList[i].szVarModChar);
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

   // Pre-compute the combinatorial bitmasks that specify the positions of a modified residue
   // iEnd is one larger than max peptide length
   ModificationsPermuter::initCombinations(g_staticParams.options.peptideLengthRange.iEnd - 1, MAX_MODS_PER_MOD,
         &ALL_COMBINATIONS, &ALL_COMBINATION_CNT);

   // Get the unique modifiable sequences from the peptides
   PEPTIDE_MOD_SEQ_IDXS = new int[g_vRawPeptides.size()];

   MOD_SEQS = ModificationsPermuter::getModifiableSequences(g_vRawPeptides, PEPTIDE_MOD_SEQ_IDXS, ALL_MODS);

   // Get the modification combinations for each unique modifiable substring
   ModificationsPermuter::getModificationCombinations(MOD_SEQS, MAX_MODS_PER_MOD, ALL_MODS,
         MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);
}


void CometFragmentIndex::GenerateFragmentIndex(vector<PlainPeptideIndex>& g_vRawPeptides,
                                               ThreadPool *tp)
{
   size_t iEndSize = g_vRawPeptides.size();
   int iNoModificationNumbers = 0;

   cout <<  " - calculate fragments for " << iEndSize << " raw peptides ... "; fflush(stdout);

   auto tStartTime = chrono::steady_clock::now();

   // Create the mutex we will use to protect vFragmentIndex
   Threading::CreateMutex(&_vFragmentIndexMutex);
   Threading::CreateMutex(&_vFragmentPeptidesMutex);

   ThreadPool *pFragmentIndexPool = tp;

   int iNumIndexingThreads = g_staticParams.options.iNumThreads;

   if (iNumIndexingThreads > 8)
      iNumIndexingThreads = 8;

   for (int iWhichThread = 0; iWhichThread<iNumIndexingThreads; ++iWhichThread)
   {
      pFragmentIndexPool->doJob(std::bind(AddFragmentsThreadProc, std::ref(g_vRawPeptides),
               iWhichThread, iNumIndexingThreads, std::ref(iNoModificationNumbers), pFragmentIndexPool));
   }

   pFragmentIndexPool->wait_on_threads();

   cout << ElapsedTime(tStartTime) << endl;

// cout << endl << endl;
// cout << "peptides have no modification numbers or exceed MAX_COMBINATIONS: " << iNoModificationNumbers << " " << endl;
// cout << "size of gv_FragmentPeptides is " << g_vFragmentPeptides.size() << endl;

   cout << " - sorting each fragment mass index bin by peptide mass ... "; fflush(stdout);

   tStartTime = chrono::steady_clock::now();

   // combine each g_arrvFragmentIndex[iWhichThread[]
   for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; ++i)
   {
      // Walk through each g_arrvFragmentIndex[] and sort entries by increasing peptide mass
      // First merge individual g_arrvFragmentIndex[] into g_arrvFragmentIndex[0]
      pFragmentIndexPool->doJob(std::bind(SortFragmentThreadProc, i, iNumIndexingThreads, pFragmentIndexPool));
   }

   pFragmentIndexPool->wait_on_threads();

   // Destroy the mutex we will use to protect vFragmentIndex
   Threading::DestroyMutex(_vFragmentIndexMutex);
   Threading::DestroyMutex(_vFragmentPeptidesMutex);

   cout << ElapsedTime(tStartTime) << endl;
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


void CometFragmentIndex::AddFragmentsThreadProc(vector<PlainPeptideIndex>& g_vRawPeptides,
                                                int iWhichThread,
                                                int iNumIndexingThreads,
                                                int& iNoModificationNumbers,
                                                ThreadPool *tp)
{
   for (size_t iWhichPeptide = iWhichThread; iWhichPeptide < g_vRawPeptides.size(); iWhichPeptide += iNumIndexingThreads)
   {
      // AddFragments(iWhichPeptide, modNumIdx) for unmodified peptide
      // FIX: if require variable mod is set, this would not be called here
      AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, -1, -1);

      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];

      // Possibly analyze peptides with a terminal mod and no variable mod on any residue
      if (g_staticParams.variableModParameters.bVarTermModSearch)
      {
         // Add any n-term variable mods
         for (short ctNtermMod=0; ctNtermMod<VMODS; ++ctNtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod)
               AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, ctNtermMod, -1);
         }

         // Add any c-term variable mods
         for (short ctCtermMod=0; ctCtermMod<VMODS; ++ctCtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod)
               AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, -1, ctCtermMod);
         }

         // Now consider combinations of n-term and c-term variable mods
         for (short ctNtermMod=0; ctNtermMod<VMODS; ++ctNtermMod)
         {
            for (short ctCtermMod=0; ctCtermMod<VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod
                     && g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod)
               {
                  AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, -1, ctNtermMod, ctCtermMod);
               }
            }
         }
      }

      if (modSeqIdx < 0)
      {
         iNoModificationNumbers += 1;
         // peptide is not modified, skip following permuting code
         continue;
      }

      int startIdx = MOD_SEQ_MOD_NUM_START[modSeqIdx];
      if (startIdx == -1)
         continue;

      int modNumCount = MOD_SEQ_MOD_NUM_CNT[modSeqIdx];

      for (int modNumIdx = startIdx; modNumIdx < startIdx + modNumCount; ++modNumIdx)
      {
         AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, -1, -1);

         if (g_staticParams.variableModParameters.bVarTermModSearch)
         {
            // Add any n-term variable mods
            for (short ctNtermMod=0; ctNtermMod<VMODS; ++ctNtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod)
                  AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, ctNtermMod, -1);
            }

            // Add any c-term variable mods
            for (short ctCtermMod=0; ctCtermMod<VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod)
                  AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, -1, ctCtermMod);
            }

            // Now consider combinations of n-term and c-term variable mods
            for (short ctNtermMod=0; ctNtermMod<VMODS; ++ctNtermMod)
            {
               for (short ctCtermMod=0; ctCtermMod<VMODS; ++ctCtermMod)
               {
                  if (g_staticParams.variableModParameters.varModList[ctNtermMod].bNtermMod
                        && g_staticParams.variableModParameters.varModList[ctCtermMod].bCtermMod)
                  {
                     AddFragments(g_vRawPeptides, iWhichThread, iWhichPeptide, modNumIdx, ctNtermMod, ctCtermMod);
                  }
               }
            }
         }
      }
   }
}


void CometFragmentIndex::SortFragmentThreadProc(int iBin,
                                                int iNumIndexingThreads,
                                                ThreadPool *tp)
{
   for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
   {
      sort(g_arrvFragmentIndex[iWhichThread][iBin].begin(), g_arrvFragmentIndex[iWhichThread][iBin].end(), SortFragmentsByPepMass);

      g_arrvFragmentIndex[iWhichThread][iBin].erase(unique(g_arrvFragmentIndex[iWhichThread][iBin].begin(),
               g_arrvFragmentIndex[iWhichThread][iBin].end()), g_arrvFragmentIndex[iWhichThread][iBin].end());
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
                                      short siCtermMod)
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

   if (dCalcPepMass > g_massRange.dMaxMass)
      return;

   struct FragmentPeptidesStruct sTmp;
   sTmp.iWhichPeptide = iWhichPeptide;
   sTmp.modNumIdx = modNumIdx;
   sTmp.dPepMass = dCalcPepMass;
   sTmp.siNtermMod = siNtermMod;
   sTmp.siCtermMod = siCtermMod;
   unsigned int uiCurrentFragmentPeptide;

   Threading::LockMutex(_vFragmentPeptidesMutex);
   // store peptide representation based on sequence (iWhichPeptide), modification state (modNumIdx), and mass (dPepMass)
   g_vFragmentPeptides.push_back(sTmp);
   uiCurrentFragmentPeptide = g_vFragmentPeptides.size() - 1;  // index of current peptide in g_vFragmentPeptides
   Threading::UnlockMutex(_vFragmentPeptidesMutex);


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

   j = 0;
   k = modSeq.size() - 1;
   unsigned int uiBinIon;
   for (int i = 0; i < iEndPos; ++i)
   {
      iPosReverse = iEndPos - i;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[iPosReverse]];

      if (modNumIdx >= 0) // handle the variable mods if present on peptide
      {
         if (sPeptide[i] == modSeq[j])
         {
            if (modNumIdx != -1 && mods[j] != -1)
            {
               dBion += g_staticParams.variableModParameters.varModList[(int)mods[j]].dVarModMass;
            }
            j++;
         }

         if (sPeptide[iPosReverse] == modSeq[k])
         {
            if (modNumIdx != -1 && mods[k] != -1)
            {
               dYion += g_staticParams.variableModParameters.varModList[(int)mods[k]].dVarModMass;
            }
            k--;
         }
      }

      if (dBion > g_staticParams.options.dMaxFragIndexMass && dYion > g_staticParams.options.dMaxFragIndexMass)
         break;

      if (i > 1)  // skip first two low mass b- and y-ions
      {
         if (dBion > g_staticParams.options.dMinFragIndexMass && dBion < g_staticParams.options.dMaxFragIndexMass)
         {
            uiBinIon = BIN(dBion);

            if (uiBinIon < g_massRange.g_uiMaxFragmentArrayIndex)
               g_arrvFragmentIndex[iWhichThread][uiBinIon].push_back(uiCurrentFragmentPeptide);
         }

         if (dYion > g_staticParams.options.dMinFragIndexMass && dYion < g_staticParams.options.dMaxFragIndexMass)
         {
            uiBinIon = BIN(dYion);

            if (uiBinIon < g_massRange.g_uiMaxFragmentArrayIndex)
               g_arrvFragmentIndex[iWhichThread][uiBinIon].push_back(uiCurrentFragmentPeptide);
         }
      }
   }
}


bool CometFragmentIndex::WritePlainPeptideIndex(ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded;
   string strOut;

   string strIndexFile;

   strIndexFile = g_staticParams.databaseInfo.szDatabase + string(".idx");

   if ((fp = fopen(strIndexFile.c_str(), "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", strIndexFile.c_str());
      exit(1);
   }

   strOut = " Creating plain peptide/protein index file: ";
   logout(strOut.c_str());
   fflush(stdout);

   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);

   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   tp->fillPool( g_staticParams.options.iNumThreads < 0 ? 0 : g_staticParams.options.iNumThreads-1);  
   if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
      g_massRange.bNarrowMassRange = true;
   else
      g_massRange.bNarrowMassRange = false;

   if (bSucceeded)
     bSucceeded = CometSearch::RunSearch(0, 0, tp);

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
   strOut = " - removing duplicate peptides\n";
   logout(strOut.c_str());
   fflush(stdout);

   // first sort by peptide then protein file position
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CompareByPeptide);

   // At this point, need to create g_pvProteinsList protein file position vector of vectors to map each peptide
   // to every protein. g_pvDBIndex.at().lProteinFilePosition is now reference to protein vector entry
   vector<comet_fileoffset_t> temp;  // stores list of duplicate proteins which gets pushed to g_pvProteinsList

   // Create g_pvProteinsList.  This is a vector of vectors.  Each element is vector list
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
                // (deprecated, raw peptides only) temp can have duplicates due to mod forms of peptide so make unique here
                //sort(temp.begin(), temp.end());
                //temp.erase(unique(temp.begin(), temp.end()), temp.end() );
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

   cout << " - writing file: " << strIndexFile << endl;

   // write out index header
   fprintf(fp, "Comet peptide index.  Comet version %s\n", g_sCometVersion.c_str());
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

   fprintf(fp, "\n");

   // write VariableMod:
   fprintf(fp, "VariableMod:");
   for (int x = 0; x < VMODS; ++x)
      fprintf(fp, " %lf:%s", g_staticParams.variableModParameters.varModList[x].dVarModMass,
         g_staticParams.variableModParameters.varModList[x].szVarModChar);
   fprintf(fp, "\n");

   size_t tTmp = (int)g_pvProteinNames.size();
   comet_fileoffset_t *lProteinIndex = new comet_fileoffset_t[tTmp];
   for (size_t i = 0; i < tTmp; ++i)
      lProteinIndex[i] = -1;

   comet_fileoffset_t lPeptidesFilePos = comet_ftell(fp);
   size_t tNumPeptides = g_pvDBIndex.size();
   fwrite(&tNumPeptides, sizeof(size_t), 1, fp);  // write # of peptides

   for (std::vector<DBIndex>::iterator it = g_pvDBIndex.begin(); it != g_pvDBIndex.end(); ++it)
   {
      int iLen = (int)strlen((*it).szPeptide);
      fwrite(&iLen, sizeof(int), 1, fp);
      fwrite((*it).szPeptide, sizeof(char), iLen, fp);
      fwrite(&((*it).dPepMass), sizeof(double), 1, fp);
      fwrite(&((*it).lIndexProteinFilePosition), clSizeCometFileOffset, 1, fp);
   }

   // Now write out: vector<vector<comet_fileoffset_t>> g_pvProteinsList
   comet_fileoffset_t lProteinsFilePos = comet_ftell(fp);
   tTmp = g_pvProteinsList.size();
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

   fwrite(&lPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   fwrite(&lProteinsFilePos, clSizeCometFileOffset, 1, fp);

   g_pvDBIndex.clear();
   g_pvProteinNames.clear();
   delete[] lProteinIndex;

   fclose(fp);

// bSucceeded = WriteFragmentIndex(strIndexFile, lPeptidesFilePos, lProteinsFilePos, tp);

   strOut = " - done.  # peps " + to_string(tNumPeptides) + string("\n");
   logout(strOut.c_str());
   fflush(stdout);

// CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

   return bSucceeded;
}

bool fWriteFragmentIndex(string strIndexFile,
                                            comet_fileoffset_t lPeptidesFilePos,
                                            comet_fileoffset_t lProteinsFilePos,
                                            ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded;
   size_t tSizevRawPeptides = 0;
   string strOut;

   strOut = "\n Adding fragment index to index file:\n";
   logout(strOut.c_str());
   fflush(stdout);

   bSucceeded = CometFragmentIndex::CreateFragmentIndex(tp);
   tSizevRawPeptides = g_vRawPeptides.size();

   if (!bSucceeded)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error creating fragment index. \n");
      logerr(szErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   auto tStartTime = chrono::steady_clock::now();
   cout <<  " - writing fragment index ... ";
   fflush(stdout);

   if ((fp = fopen(strIndexFile.c_str(), "ab")) == NULL)
   {
      printf(" Error - cannot open index file %s to append to\n", strIndexFile.c_str());
      exit(1);
   }

   fseek(fp, 0, SEEK_END);

   comet_fileoffset_t clBeginFragIndexPosition = ftell(fp);

   // Write MOD_SEQS
   size_t tSize;
   unsigned int uiLen;
   int iTmp;

   tSize = MOD_SEQS.size();
   fwrite(&tSize, sizeof(size_t), 1, fp); // write # vector elements
   for (size_t i = 0; i < tSize; ++i)
   {
      uiLen = MOD_SEQS.at(i).length();
      fwrite(&uiLen, sizeof(unsigned int), 1, fp);
      fwrite(MOD_SEQS.at(i).c_str(), sizeof(char), uiLen, fp); // write out MOD_SEQ.at(i)
   }

   // write MOD_SEQ_MOD_NUM_START; size is MOD_SEQS.size()
   for (size_t i = 0; i < tSize; ++i)
   {
      iTmp = MOD_SEQ_MOD_NUM_START[i];
      fwrite(&iTmp, sizeof(int), 1, fp);
   }

   // write MOD_SEQ_MOD_NUM_CNT; size is MOD_SEQS.size()
   for (size_t i = 0; i < tSize; ++i)
   {
      iTmp = MOD_SEQ_MOD_NUM_CNT[i];
      fwrite(&iTmp, sizeof(int), 1, fp);
   }

   // write MOD_NUMBERS
   tSize = MOD_NUMBERS.size();
   fwrite(&tSize, sizeof(size_t), 1, fp); // write # vector elements
   for (size_t i = 0; i < tSize; ++i)
   {
      fwrite(&(MOD_NUMBERS[i].modStringLen), sizeof(int), 1, fp);
      fwrite(MOD_NUMBERS[i].modifications, sizeof(char), MOD_NUMBERS[i].modStringLen, fp);
   }

   // write PEPTIDE_MOD_SEQ_IDXS; size is g_vRawPeptides.size()
   fwrite(&tSizevRawPeptides, sizeof(size_t), 1, fp);
   for (size_t i = 0; i < tSizevRawPeptides; ++i)
   {
      iTmp = PEPTIDE_MOD_SEQ_IDXS[i];
      fwrite(&iTmp, sizeof(int), 1, fp);
   }
    
   // size of g_arrvFragmentIndex which is g_uiMaxFragmentIndexArray
   fwrite(&g_massRange.g_uiMaxFragmentArrayIndex, sizeof(unsigned int), 1, fp); // array size

   int iWhichThread = 0;  //FIX
   for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; ++i)
   {
      size_t tNumEntries = g_arrvFragmentIndex[iWhichThread][i].size();
      fwrite(&tNumEntries, sizeof(size_t), 1, fp); // index

      if (tNumEntries > 0)
      {
         for (auto it=g_arrvFragmentIndex[iWhichThread][i].begin(); it!=g_arrvFragmentIndex[iWhichThread][i].end(); ++it)
         {
            fwrite(&(*it), sizeof(unsigned int), 1, fp);
         }
      }
   }

   // write g_vFragmentPeptides
   tSize = g_vFragmentPeptides.size();
   fwrite(&tSize, sizeof(size_t), 1, fp);
   for (size_t i = 0; i < tSize; ++i)
   {
      fwrite(&(g_vFragmentPeptides[i]), sizeof(struct FragmentPeptidesStruct), 1, fp);
   }

   fwrite(&clBeginFragIndexPosition, clSizeCometFileOffset, 1, fp);  //write beginning of fragment index at array size
   fwrite(&lPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   fwrite(&lProteinsFilePos, clSizeCometFileOffset, 1, fp);

   cout << CometFragmentIndex::ElapsedTime(tStartTime) << endl;

   return bSucceeded;
}


// read fragment index from disk
bool CometFragmentIndex::ReadFragmentIndex(ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded = true;
   string strIndexFile;
   size_t tTmp;
   char szBuf[SIZE_BUF];

   if (g_vFragmentIndexRead)
      return 1;

   // database already is .idx
   strIndexFile = g_staticParams.databaseInfo.szDatabase;

   if ((fp = fopen(strIndexFile.c_str(), "rb")) == NULL)
   {
      printf(" Error - cannot open index file %s to read\n", strIndexFile.c_str());
      exit(1);
   }

   auto tStartTime = chrono::steady_clock::now();
   cout << " - reading file : " << strIndexFile;
   fflush(stdout);

   // read fp of index
   comet_fileoffset_t clBeginFragIndexPosition;  // position to start reading fragment index elements

   comet_fseek(fp, -(3*clSizeCometFileOffset), SEEK_END);
   fread(&clBeginFragIndexPosition, clSizeCometFileOffset, 1, fp);

   comet_fseek(fp, clBeginFragIndexPosition, SEEK_SET);

   // Read MOD_SEQS
   size_t tSize;
   unsigned int uiLen;
   unsigned int uiTmp;

   fread(&tSize, sizeof(size_t), 1, fp); // read # vector elements

   MOD_SEQ_MOD_NUM_START = new int[tSize];
   MOD_SEQ_MOD_NUM_CNT = new int[tSize];

   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&uiLen, sizeof(unsigned int), 1, fp);
      if (uiLen >= SIZE_BUF)
      {
         printf(" Error MOD_SEQ length %d too large.\n", uiLen);
         exit(1);
      }

      fread(szBuf, sizeof(char), uiLen, fp); // length of the mod sequence
      szBuf[uiLen] = '\0';
      MOD_SEQS.push_back(szBuf);
   }

   // read MOD_SEQ_MOD_NUM_START; size is MOD_SEQS.size()
   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&(MOD_SEQ_MOD_NUM_START[i]), sizeof(int), 1, fp);
   }

   // read MOD_SEQ_MOD_NUM_CNT; size is MOD_SEQS.size()
   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&(MOD_SEQ_MOD_NUM_CNT[i]), sizeof(int), 1, fp);
   }

   // read MOD_NUMBERS
   fread(&tSize, sizeof(size_t), 1, fp); // write # vector elements
   struct ModificationNumber sModNumTmp;
   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&sModNumTmp.modStringLen, sizeof(int), 1, fp);
      char *mods = new char[sModNumTmp.modStringLen];
      fread(mods, sizeof(char), sModNumTmp.modStringLen, fp);
      sModNumTmp.modifications = mods;
      MOD_NUMBERS.push_back(sModNumTmp);
   }

   // read PEPTIDE_MOD_SEQ_IDXS; size is g_vRawPeptides.size()
   fread(&tTmp, sizeof(size_t), 1, fp);
   PEPTIDE_MOD_SEQ_IDXS = new int[tTmp];
   for (size_t i = 0; i < tTmp; ++i)
   {
      fread(&(PEPTIDE_MOD_SEQ_IDXS[i]), sizeof(int), 1, fp);
   }

   // size of g_arrvFragmentIndex which is g_uiMaxFragmentIndexArray
   fread(&g_massRange.g_uiMaxFragmentArrayIndex, sizeof(unsigned int), 1, fp); // array size

// delete[] g_arrvFragmentIndex;  // shouldn't be needed; hope it doesn't hurt

   for (int iWhichThread=0; iWhichThread < (g_staticParams.options.iNumThreads> MAX_FRAGMENTINDEX_THREADS ? MAX_FRAGMENTINDEX_THREADS : g_staticParams.options.iNumThreads) ; ++iWhichThread)
   {
      g_arrvFragmentIndex[iWhichThread] = new vector<unsigned int>[g_massRange.g_uiMaxFragmentArrayIndex];
   }

   int iWhichThread = 0;  // FIX if this ever gets used; currently unused as index is generated on-the-fly

   for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; ++i)
   {
      fread(&tTmp, sizeof(size_t), 1, fp); // index

      if (tTmp> 0)
      {
         g_arrvFragmentIndex[iWhichThread][i].reserve(tTmp);
         for (size_t it = 0; it != tTmp; ++it)
         {
            fread(&uiTmp, sizeof(unsigned int), 1, fp);
            g_arrvFragmentIndex[iWhichThread][i].push_back(uiTmp);
         }
      }
   }

   // size of g_vFragmentPeptides which is g_uiMaxFragmentIndexArray
   fread(&tSize, sizeof(size_t), 1, fp); // size of g_vFragmentPeptides
   g_vFragmentPeptides.clear();
   g_vFragmentPeptides.reserve(tSize);
   FragmentPeptidesStruct sTmp;
   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&sTmp, sizeof(struct FragmentPeptidesStruct), 1, fp);
      g_vFragmentPeptides.push_back(sTmp);
   }

   fclose(fp);

   cout << " (" << ElapsedTime(tStartTime) << ")" << endl;

   return bSucceeded;
}


// read the raw peptides from disk
bool CometFragmentIndex::ReadPlainPeptideIndex(void)
{
   FILE *fp;
   size_t tTmp;
   char szBuf[SIZE_BUF];
   string strIndexFile;

   if (g_vPlainPeptideIndexRead)
      return 1;

   if (g_staticParams.options.bCreateIndex)
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
            sscanf(tok, "%lf", &(g_staticParams.staticModifications.pdStaticMods[x]));
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
         char szMod1[512];
         char szMod2[512];
         char szMod3[512];
         char szMod4[512];
         char szMod5[512];
         char szMod6[512];
         char szMod7[512];
         char szMod8[512];
         char szMod9[512];

         sscanf(szBuf + 12, "%s %s %s %s %s %s %s %s %s",
            szMod1, szMod2, szMod3, szMod4, szMod5, szMod6, szMod7, szMod8, szMod9);
         
         sscanf(szMod1, "%lf:%s", &(g_staticParams.variableModParameters.varModList[0].dVarModMass),
            g_staticParams.variableModParameters.varModList[0].szVarModChar);

         sscanf(szMod2, "%lf:%s", &(g_staticParams.variableModParameters.varModList[1].dVarModMass),
            g_staticParams.variableModParameters.varModList[1].szVarModChar);

         sscanf(szMod3, "%lf:%s", &(g_staticParams.variableModParameters.varModList[2].dVarModMass),
            g_staticParams.variableModParameters.varModList[2].szVarModChar);

         sscanf(szMod4, "%lf:%s", &(g_staticParams.variableModParameters.varModList[3].dVarModMass),
            g_staticParams.variableModParameters.varModList[3].szVarModChar);

         sscanf(szMod5, "%lf:%s", &(g_staticParams.variableModParameters.varModList[4].dVarModMass),
            g_staticParams.variableModParameters.varModList[4].szVarModChar);

         sscanf(szMod6, "%lf:%s", &(g_staticParams.variableModParameters.varModList[5].dVarModMass),
            g_staticParams.variableModParameters.varModList[5].szVarModChar);

         sscanf(szMod7, "%lf:%s", &(g_staticParams.variableModParameters.varModList[6].dVarModMass),
            g_staticParams.variableModParameters.varModList[6].szVarModChar);

         sscanf(szMod8, "%lf:%s", &(g_staticParams.variableModParameters.varModList[7].dVarModMass),
            g_staticParams.variableModParameters.varModList[7].szVarModChar);

         sscanf(szMod9, "%lf:%s", &(g_staticParams.variableModParameters.varModList[8].dVarModMass),
            g_staticParams.variableModParameters.varModList[8].szVarModChar);

         for (int x = 0; x < VMODS; ++x)
         {
            if (g_staticParams.variableModParameters.varModList[x].dVarModMass != 0.0)
            {
               g_staticParams.variableModParameters.bVarModSearch = true;
               break;
            }
         }

         bFoundVariable = true;
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
   comet_fileoffset_t clPeptidesFilePos;  // file position of raw peptides
   comet_fileoffset_t clProteinsFilePos;  // file position of g_pvProteinsList

   comet_fseek(fp, -clSizeCometFileOffset*2, SEEK_END);
   fread(&clPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);

   comet_fseek(fp, clPeptidesFilePos, SEEK_SET);

   size_t tNumPeptides;
   fread(&tNumPeptides, sizeof(size_t), 1, fp);  // read # of peptides

   struct PlainPeptideIndex sTmp;
   int iLen;
// long lNumMatchedProteins;
   char szPeptide[MAX_PEPTIDE_LEN];
   for (size_t it = 0; it < tNumPeptides; ++it)
   {
      fread(&iLen, sizeof(int), 1, fp);
      fread(szPeptide, sizeof(char), iLen, fp);
      szPeptide[iLen] = '\0';
      sTmp.sPeptide = szPeptide;
      fread(&(sTmp.dPepMass), sizeof(double), 1, fp);
      fread(&(sTmp.lIndexProteinFilePosition), clSizeCometFileOffset, 1, fp);

      g_vRawPeptides.push_back(sTmp);
   }

   comet_fseek(fp, clProteinsFilePos, SEEK_SET);  // should be at this file position here anyways already

   // now read in: vector<vector<comet_fileoffset_t>> g_pvProteinsList
   size_t tSize;
   fread(&tSize, clSizeCometFileOffset, 1, fp);
   vector<comet_fileoffset_t> vTmp;

   g_pvProteinsList.clear();
   g_pvProteinsList.reserve(tSize);

   for (size_t it = 0; it < tSize; ++it)
   {
      fread(&tTmp, clSizeCometFileOffset, 1, fp);
      
      vTmp.clear();
      for (size_t it2 = 0; it2 < tTmp; ++it2)
      {
         fread(&clTmp, clSizeCometFileOffset, 1, fp);
         vTmp.push_back(clTmp);
      }
      g_pvProteinsList.push_back(vTmp);
   }

   fclose(fp);

   return true;
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

/*
      // same sequences and masses here so next look at mod state
      int iLen = (int)strlen(lhs.szPeptide)+2;
      for (int i=0; i<iLen; ++i)
      {
         if (lhs.pcVarModSites[i] != rhs.pcVarModSites[i])
         {
            // different mod state
            if (lhs.pcVarModSites[i] > rhs.pcVarModSites[i])
               return true;
            else
               return false;
         }
      }
*/

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

