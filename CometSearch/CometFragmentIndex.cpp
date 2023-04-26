/*
     Copyright 2012 University of Washington

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
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

#define USEFRAGMENTTHREADS 2


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


bool CometFragmentIndex::CreateFragmentIndex(size_t *tSizevRawPeptides,
                                             ThreadPool *tp)
{
   auto tStartTime= chrono::steady_clock::now();

   ReadPlainPeptideIndex();

   // now permute mods on the peptides
   PermuteIndexPeptideMods(g_vRawPeptides);

   // vector of modified peptides
   // - raw peptide via iWhichPeptide referencing entry in g_vRawPeptides to access peptide and protein(s)
   // - modification encoding index
   // - modification mass
g_massRange.dMaxFragmentMass = 2500; //FIX
   g_massRange.g_uiMaxFragmentArrayIndex = BIN(g_massRange.dMaxFragmentMass) + 1;
   g_arrvFragmentIndex = new vector<unsigned int>[g_massRange.g_uiMaxFragmentArrayIndex];

   *tSizevRawPeptides = g_vRawPeptides.size();

   // generate the modified peptides to calculate the fragment index
   GenerateFragmentIndex(g_vRawPeptides, tp);

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;

   cout << " - time generating fragment index: " << minutes << " minutes " << seconds  << " seconds" << endl;

   return true;
}


void CometFragmentIndex::PermuteIndexPeptideMods(vector<PlainPeptideIndex>& g_vRawPeptides)
{
   vector<string> ALL_MODS; // An array of all the user specified amino acids that can be modified

   // Pre-computed bitmask combinations for peptides of length MAX_PEPTIDE_LEN with up
   // to MAX_MODS_PER_MOD modified amino acids.

   // Maximum number of bits that can be set in a modifiable sequence for a given modification.
   // C(25, 5) = 53,130; C(25, 4) = 10,650; C(25, 3) = 2300.  This is more than MAX_COMBINATIONS (65,534)

   for (int i=0; i<VMODS; i++)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
         && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='-'))
      {
         ALL_MODS.push_back(g_staticParams.variableModParameters.varModList[i].szVarModChar);
      }
   }

   int MOD_CNT = (int)ALL_MODS.size();

   for (int i = 0; i < MOD_CNT; i++)
   {
      cout << " - mods: " << ALL_MODS[i] << endl;
   }

   unsigned long long* ALL_COMBINATIONS;
   int ALL_COMBINATION_CNT = 0;

   // Pre-compute the combinatorial bitmasks that specify the positions of a modified residue
   ModificationsPermuter::initCombinations(MAX_PEPTIDE_LEN, MAX_MODS_PER_MOD,
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
   size_t iWhichPeptide = 0;
   size_t iEndSize = g_vRawPeptides.size();
   int iNoModificationNumbers = 0;

   cout <<  " - calculating the bazillion peptide fragments ..." << endl;

   auto tStartTime = chrono::steady_clock::now();

   // Create the mutex we will use to protect vFragmentIndex
   Threading::CreateMutex(&_vFragmentIndexMutex);
   Threading::CreateMutex(&_vFragmentPeptidesMutex);

   ThreadPool *pFragmentIndexPool = tp;

   for (iWhichPeptide = 0; iWhichPeptide < iEndSize; ++iWhichPeptide)
   {
#if USEFRAGMENTTHREADS == 1
      pFragmentIndexPool->doJob(std::bind(AddFragmentsThreadProc, std::ref(g_vRawPeptides), iWhichPeptide, std::ref(iNoModificationNumbers), pFragmentIndexPool));
#else
      AddFragmentsThreadProc(std::ref(g_vRawPeptides), iWhichPeptide, std::ref(iNoModificationNumbers), pFragmentIndexPool);
#endif
   }

#if USEFRAGMENTTHREADS == 1
   printf(" Waiting on threads to process fragment ions for %ld peptides ...\n", iEndSize);
   pFragmentIndexPool->wait_on_threads();
#endif

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;
   cout << " - done with adding fragment ions to index: " << minutes << " minutes " << seconds  << " seconds" << endl;

// cout << endl << endl;
// cout << "peptides have no modification numbers or exceed MAX_COMBINATIONS: " << iNoModificationNumbers << " " << endl;
// cout << "size of gv_FragmentPeptides is " << g_vFragmentPeptides.size() << endl;

   // Walk through each g_arrvFragmentIndex[] and sort entries by increasing peptide mass

   tStartTime = chrono::steady_clock::now();

   for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; i++)
   {
      if (g_arrvFragmentIndex[i].size() > 0)
      {
#if USEFRAGMENTTHREADS > 0
         pFragmentIndexPool->doJob(std::bind(SortFragmentThreadProc, i, pFragmentIndexPool));
#else
         SortFragmentThreadProc(i, pFragmentIndexPool);
#endif
      }
   }

#if USEFRAGMENTTHREADS > 0
   pFragmentIndexPool->wait_on_threads();
#endif

   // Destroy the mutex we will use to protect vFragmentIndex
   Threading::DestroyMutex(_vFragmentIndexMutex);
   Threading::DestroyMutex(_vFragmentPeptidesMutex);

   tEndTime = chrono::steady_clock::now();
   duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   minutes = duration.count() / 60000;
   seconds = (duration.count() - minutes*60000) / 1000;
   cout << " - done with sorting fragment index: " << minutes << " minutes " << seconds  << " seconds" << endl;
}


void CometFragmentIndex::AddFragmentsThreadProc(vector<PlainPeptideIndex>& g_vRawPeptides,
                                                size_t iWhichPeptide,
                                                int& iNoModificationNumbers,
                                                ThreadPool *tp)
{
   // AddFragments(iWhichPeptide, modNumIdx) for unmodified peptide
   // FIX: if require variable mod is set, this would not be called here
   AddFragments(g_vRawPeptides, iWhichPeptide, -1);

   int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];

   if (modSeqIdx == -1)
   {
      iNoModificationNumbers += 1;
      // peptide is not modified, skip following permuting code
      return;
   }

   int startIdx = MOD_SEQ_MOD_NUM_START[modSeqIdx];
   if (startIdx == -1)
   {
      // should always permute to MAX_COMBINATIONS of mods for the peptide
//    cout << " ERROR should not get here if peptide contains mod residues; modSeqIdx " << modSeqIdx << ", peptide " << peptide << endl;
      return;
   }

   int modNumCount = MOD_SEQ_MOD_NUM_CNT[modSeqIdx];

   for (int modNumIdx = startIdx; modNumIdx < startIdx + modNumCount; modNumIdx++)
   {
      // add fragment ions for each modification permutation
      AddFragments(g_vRawPeptides, iWhichPeptide, modNumIdx);
   }
}


void CometFragmentIndex::SortFragmentThreadProc(int i,
                                                ThreadPool *tp)
{
   sort(g_arrvFragmentIndex[i].begin(), g_arrvFragmentIndex[i].end(), SortFragmentsByPepMass);
}


bool CometFragmentIndex::SortFragmentsByPepMass(unsigned int x, unsigned int y)
{
   return (g_vFragmentPeptides[x].dPepMass < g_vFragmentPeptides[y].dPepMass);
}


void CometFragmentIndex::AddFragments(vector<PlainPeptideIndex>& g_vRawPeptides,
                                      int iWhichPeptide,
                                      int modNumIdx)
{
   string sPeptide = g_vRawPeptides.at(iWhichPeptide).sPeptide;

   ModificationNumber modNum;
   char* mods = NULL;
   int modSeqIdx;
   string modSeq;

   if (modNumIdx != -1)  // set modified peptide info
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
   for (int i = 0; i <= iEndPos; i++)
   {
      dCalcPepMass += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];

      if (modNumIdx != -1) // handle the variable mods if present on peptide
      {
         if (sPeptide[i] == modSeq[j])
         {
            if (modNumIdx != -1 && mods[j] != -1)
            {
               dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)mods[j]].dVarModMass;
            }
            j++;
         }
      }
   }

   if (dCalcPepMass > g_massRange.dMaxMass)
      return;

   struct FragmentPeptidesStruct sTmp;
   sTmp.iWhichPeptide = iWhichPeptide;
   sTmp.modNumIdx = modNumIdx;
   sTmp.dPepMass = dCalcPepMass;

#if USEFRAGMENTTHREADS == 1
   Threading::LockMutex(_vFragmentPeptidesMutex);
#endif

   // store peptide representation based on sequence (iWhichPeptide), modification state (iModNumIdx), and mass (dPepMass)
   g_vFragmentPeptides.push_back(sTmp);
   unsigned int uiCurrentFragmentPeptide = g_vFragmentPeptides.size() - 1;  // index of current peptide in g_vFragmentPeptides
                                                                            //

/*
if (!(iWhichPeptide%5000))
{
   // print out the peptide
   printf("OK in AddFragments: ");
   j=0;
   for (int i = 0; i <= iEndPos; i++)
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
   for (int i = 0; i < iEndPos; i++)
   {
      iPosReverse = iEndPos - i;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)sPeptide[iPosReverse]];

      if (modNumIdx != -1) // handle the variable mods if present on peptide
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

      if (i > 1)  // skip first two low mass b- and y-ions ala sage
      {
//       Threading::LockMutex(_vFragmentIndexMutex);

         uiBinIon = BIN(dBion);

         if (uiBinIon < g_massRange.g_uiMaxFragmentArrayIndex)
            g_arrvFragmentIndex[uiBinIon].push_back(uiCurrentFragmentPeptide);

         uiBinIon = BIN(dYion);

         if (uiBinIon < g_massRange.g_uiMaxFragmentArrayIndex)
            g_arrvFragmentIndex[uiBinIon].push_back(uiCurrentFragmentPeptide);

//       Threading::UnlockMutex(_vFragmentIndexMutex);
      }
   }

#if USEFRAGMENTTHREADS == 1
   Threading::UnlockMutex(_vFragmentPeptidesMutex);
#endif
}


bool CometFragmentIndex::WritePlainPeptideIndex(ThreadPool *tp)
{
   FILE *fptr;
   bool bSucceeded;
   char szOut[SIZE_FILE+30];

   const int iIndex_SIZE_FILE=SIZE_FILE+4;
   char szIndexFile[iIndex_SIZE_FILE];
   sprintf(szIndexFile, "%s.idx", g_staticParams.databaseInfo.szDatabase);

   if ((fptr = fopen(szIndexFile, "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", szIndexFile);
      exit(1);
   }

   sprintf(szOut, " Creating peptide/protein index file: ");
   logout(szOut);
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
   sprintf(szOut, " - removing duplicates\n");
   logout(szOut);
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

   sprintf(szOut, " - writing file: %s\n", szIndexFile);
   logout(szOut);
   fflush(stdout);

   // write out index header
   fprintf(fptr, "Comet peptide index.  Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fptr, "InputDB:  %s\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fptr, "MassRange: %lf %lf\n", g_staticParams.options.dPeptideMassLow, g_staticParams.options.dPeptideMassHigh);
   fprintf(fptr, "MassType: %d %d\n", g_staticParams.massUtility.bMonoMassesParent, g_staticParams.massUtility.bMonoMassesFragment);
   fprintf(fptr, "Enzyme: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzymeName,
      g_staticParams.enzymeInformation.iSearchEnzymeOffSet, 
      g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, 
      g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
   fprintf(fptr, "Enzyme2: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzyme2Name,
      g_staticParams.enzymeInformation.iSearchEnzyme2OffSet, 
      g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA, 
      g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
   fprintf(fptr, "NumPeptides: %ld\n", (long)g_pvDBIndex.size());

   // write out static mod params A to Z is ascii 65 to 90 then terminal mods
   fprintf(fptr, "StaticMod:");
   for (int x = 65; x <= 90; ++x)
      fprintf(fptr, " %lf", g_staticParams.staticModifications.pdStaticMods[x]);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddNterminusPeptide);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddCterminusPeptide);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddNterminusProtein);
   fprintf(fptr, " %lf\n", g_staticParams.staticModifications.dAddCterminusProtein);

   fprintf(fptr, "\n");

   //FIX write VariableMod:

   size_t tTmp = (int)g_pvProteinNames.size();
   comet_fileoffset_t *lProteinIndex = new comet_fileoffset_t[tTmp];
   for (size_t i = 0; i < tTmp; ++i)
      lProteinIndex[i] = -1;

//FIX: get rid of writing out protein names; read from original fasta
/*
   // first just write out protein names. Track file position of each protein name
   int ctProteinNames = 0;
   for (auto it = g_pvProteinNames.begin(); it != g_pvProteinNames.end(); ++it)
   {
      lProteinIndex[ctProteinNames] = comet_ftell(fptr);

      // Write out file position in orig fasta; this requires orig fasta to read these
      fwrite(it->second.szProt, sizeof(char)*WIDTH_REFERENCE, 1, fptr);

      it->second.iWhichProtein = ctProteinNames;
      ctProteinNames++;
   }
*/

   comet_fileoffset_t lPeptidesFilePos = comet_ftell(fptr);
   size_t tNumPeptides = g_pvDBIndex.size();
   fwrite(&tNumPeptides, sizeof(size_t), 1, fptr);  // write # of peptides

   for (std::vector<DBIndex>::iterator it = g_pvDBIndex.begin(); it != g_pvDBIndex.end(); ++it)
   {
      int iLen = (int)strlen((*it).szPeptide);
      fwrite(&iLen, sizeof(int), 1, fptr);
      fwrite((*it).szPeptide, sizeof(char), iLen, fptr);
      fwrite(&((*it).dPepMass), sizeof(double), 1, fptr);
      fwrite(&((*it).lIndexProteinFilePosition), clSizeCometFileOffset, 1, fptr);
   }

   // Now write out: vector<vector<comet_fileoffset_t>> g_pvProteinsList
   comet_fileoffset_t lProteinsFilePos = comet_ftell(fptr);
   tTmp = g_pvProteinsList.size();
   fwrite(&tTmp, clSizeCometFileOffset, 1, fptr);
   for (auto it = g_pvProteinsList.begin(); it != g_pvProteinsList.end(); ++it)
   {
      tTmp = (*it).size();
      fwrite(&tTmp, sizeof(size_t), 1, fptr);
      for (size_t it2 = 0; it2 < tTmp; ++it2)
         fwrite(&((*it).at(it2)), clSizeCometFileOffset, 1, fptr);
   }

   fwrite(&lPeptidesFilePos, clSizeCometFileOffset, 1, fptr);
   fwrite(&lProteinsFilePos, clSizeCometFileOffset, 1, fptr);

   fclose(fptr);

   sprintf(szOut, " - done.  # peps %zd\n", tNumPeptides);
   logout(szOut);
   fflush(stdout);

// CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

   g_pvDBIndex.clear();
   g_pvProteinNames.clear();
   delete[] lProteinIndex;

   return bSucceeded;
}


bool CometFragmentIndex::WriteFragmentIndex(ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded;
   char szOut[SIZE_FILE+60];
   char szIndexFile[SIZE_FILE+30];
   size_t tSizevRawPeptides = 0;

   sprintf(szIndexFile, "%s.idx2", g_staticParams.databaseInfo.szDatabase);

   if ((fp = fopen(szIndexFile, "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", szIndexFile);
      exit(1);
   }

   sprintf(szOut, "\n Creating fragment index file:\n");
   logout(szOut);
   fflush(stdout);

   bSucceeded = CometFragmentIndex::CreateFragmentIndex(&tSizevRawPeptides, tp);

   if (!bSucceeded)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error creating fragment index. \n");
      logerr(szErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   auto tStartTime = chrono::steady_clock::now();
   sprintf(szOut, " - writing file: %s\n", szIndexFile);
   logout(szOut);

   // write out index header
   fprintf(fp, "Comet fragment index.  Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fp, "InputDB:  %s\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fp, "MassRange: %lf %lf\n", g_staticParams.options.dPeptideMassLow, g_staticParams.options.dPeptideMassHigh);
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

   // write out variable mods
   fprintf(fp, "VariableMod:");
   // write variable mods here
   fprintf(fp, "\n");

   comet_fileoffset_t clPosition = ftell(fp);

   // Write MOD_SEQS
   size_t tSize;
   unsigned int uiLen;
   int iTmp;
   string strTmp;

   tSize = MOD_SEQS.size();

   fwrite(&tSize, sizeof(size_t), 1, fp); // write # vector elements
   for (size_t i = 0; i < tSize; ++i)
   {
      uiLen = MOD_SEQS.at(i).length();
      fwrite(&uiLen, sizeof(unsigned int), 1, fp);
      fwrite(strTmp.c_str(), sizeof(char), uiLen, fp); // write out MOD_SEQ.at(i)
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
      fwrite(&(MOD_NUMBERS[i].modificationNumber), sizeof(int), 1, fp);
      iTmp = strlen(MOD_NUMBERS[i].modifications);
      fwrite(&iTmp, sizeof(int), 1, fp);
      fwrite(MOD_NUMBERS[i].modifications, sizeof(char), iTmp, fp);
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

   for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; ++i)
   {
      size_t tNumEntries = g_arrvFragmentIndex[i].size();
      fwrite(&tNumEntries, sizeof(size_t), 1, fp); // index

      if (tNumEntries > 0)
      {
         for (auto it=g_arrvFragmentIndex[i].begin(); it!=g_arrvFragmentIndex[i].end(); ++it)
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

   fwrite(&clPosition, clSizeCometFileOffset, 1, fp);  //write beginning of fragment index at array size

   fclose(fp);

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;

   cout << " - time writing fragment index file: " << minutes << " minutes " << seconds  << " seconds" << endl;

   return bSucceeded;
}


// read fragment index from disk
bool CometFragmentIndex::ReadFragmentIndex(ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded = true;
   char szIndexFile[SIZE_FILE+30];
   size_t tTmp;
   char szBuf[SIZE_BUF];

   // database already is .idx
   sprintf(szIndexFile, "%s2", g_staticParams.databaseInfo.szDatabase);

   if ((fp = fopen(szIndexFile, "rb")) == NULL)
   {
      printf(" Error - cannot open index file %s to read\n", szIndexFile);
      exit(1);
   }

   auto tStartTime = chrono::steady_clock::now();
   printf(" - reading file: %s\n", szIndexFile);

   bool bFoundStatic = false;
   bool bFoundVariable = false;

   memset(g_staticParams.staticModifications.pdStaticMods, 0, sizeof(g_staticParams.staticModifications.pdStaticMods));

   while (fgets(szBuf, SIZE_BUF, fp))
   {
      if (!strncmp(szBuf, "MassType:", 9))
      {
         sscanf(szBuf + 9, "%d %d", &g_staticParams.massUtility.bMonoMassesParent, &g_staticParams.massUtility.bMonoMassesFragment);
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
      }
      else if (!strncmp(szBuf, "VariableMod:", 12)) // read in variable mods
      {
         bFoundVariable = true;
         break;
      }
   }

   if (!bFoundStatic || !bFoundVariable)  // check to make sure mod entries are present & parsed in indexdb
   {
      char szErr[256];
      sprintf(szErr, " Error with fragment index database format. Modifications (%d/%d) not parsed.", bFoundStatic, bFoundVariable);
      logerr(szErr);
      fclose(fp);
      return false;
   }

   // read fp of index
   comet_fileoffset_t clPosition;  // position to start reading fragment index elements

   comet_fseek(fp, -clSizeCometFileOffset, SEEK_END);
   fread(&clPosition, clSizeCometFileOffset, 1, fp);

   comet_fseek(fp, clPosition, SEEK_SET);

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
   int iTmp;
   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&(sModNumTmp.modificationNumber), sizeof(int), 1, fp);
      fread(&iTmp, sizeof(int), 1, fp);
      char *mods = new char[iTmp];
      fread(mods, sizeof(char), iTmp, fp);
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

   delete[] g_arrvFragmentIndex;  // shouldn't be needed; hope it doesn't hurt
   g_arrvFragmentIndex = new vector<unsigned int>[g_massRange.g_uiMaxFragmentArrayIndex];
   for (unsigned int i = 0; i < g_massRange.g_uiMaxFragmentArrayIndex; ++i)
   {
      fread(&tTmp, sizeof(size_t), 1, fp); // index

      if (tTmp> 0)
      {
         g_arrvFragmentIndex[i].reserve(tTmp);
         for (size_t it = 0; it != tTmp; ++it)
         {
            fread(&uiTmp, sizeof(unsigned int), 1, fp);
            g_arrvFragmentIndex[i].push_back(uiTmp);
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

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;

   cout << " - time reading fragment index file: " << minutes << " minutes " << seconds  << " seconds" << endl;

   return bSucceeded;
}


// read the raw peptides from disk
bool CometFragmentIndex::ReadPlainPeptideIndex(void)
{
   FILE *fp;
   char szIndexFile[SIZE_FILE+30];
   size_t tTmp;
   char szBuf[SIZE_BUF];


   if (g_staticParams.options.bCreateIndex)
      sprintf(szIndexFile, "%s.idx", g_staticParams.databaseInfo.szDatabase);
   else // database already is .idx
      sprintf(szIndexFile, "%s", g_staticParams.databaseInfo.szDatabase);

   auto tStartTime = chrono::steady_clock::now();
   printf(" - reading file: %s\n", szIndexFile);

   if ((fp = fopen(szIndexFile, "rb")) == NULL)
   {
      printf(" Error - cannot open index file %s to read\n", szIndexFile);
      exit(1);
   }

   bool bFoundStatic = false;
   bool bFoundVariable= false;

   while (fgets(szBuf, SIZE_BUF, fp))
   {
      if (!strncmp(szBuf, "MassType:", 9))
      {
         sscanf(szBuf + 9, "%d %d", &g_staticParams.massUtility.bMonoMassesParent, &g_staticParams.massUtility.bMonoMassesFragment);
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
      }
      else if (!strncmp(szBuf, "VariableMod:", 12)) // read in variable mods
      {
         bFoundVariable = true;
         break;
      }
   }

   if (!bFoundStatic) // || !bFoundVariable)  // check to make sure mod entries are present & parsed in indexdb
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

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;

   cout << " - time reading plain peptide index file: " << minutes << " minutes " << seconds  << " seconds" << endl;

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

