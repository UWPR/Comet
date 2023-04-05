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
unsigned int g_uiMaxFragmentArrayIndex;

//vector<vector<unsigned int>> g_vFragmentIndex;  // stores fragment index; g_pvFragmentIndex[BIN(mass)][which g_vFragmentPeptides entries]
//vector<struct FragmentPeptidesStruct> g_vFragmentPeptides;  // each peptide is represented here iWhichPeptide, which mod if any, calculated mass

Mutex CometFragmentIndex::_vFragmentIndexMutex;
Mutex CometFragmentIndex::_vFragmentPeptidesMutex;

CometFragmentIndex::CometFragmentIndex()
{
}

CometFragmentIndex::~CometFragmentIndex()
{
}


bool CometFragmentIndex::CreateFragmentIndex(size_t *tSizevRawPeptides,
                                             ThreadPool *tp)
{
   comet_fileoffset_t lEndOfPeptides;
   FILE *fp;

   auto tStartTime= chrono::steady_clock::now();

   char szIDX1[SIZE_FILE+4];
   sprintf(szIDX1, "%s.idx", g_staticParams.databaseInfo.szDatabase);

   if ((fp = fopen(szIDX1, "rb")) == NULL)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error - cannot read indexed database file \"%s\" %s.\n", szIDX1, strerror(errno));
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   // read fp of index
   comet_fileoffset_t clTmp;
#ifdef _WIN32
#ifdef _WIN64
   clTmp = sizeof(comet_fileoffset_t);              //win64
#else
   clTmp = (long long)sizeof(comet_fileoffset_t);   //win32
#endif
#else
   clTmp = sizeof(comet_fileoffset_t);              //linux
#endif

   comet_fseek(fp, -clTmp, SEEK_END);
   fread(&lEndOfPeptides, sizeof(comet_fileoffset_t), 1, fp);
// fread(&lEndOfFragmentIndex, sizeof(comet_fileoffset_t), 1, fp);

   // read database params
   int iMinMass = 0;
   int iMaxMass = 0;
   uint64_t tNumPeptides = 0;
   comet_fseek(fp, lEndOfPeptides, SEEK_SET);
   fread(&iMinMass, sizeof(int), 1, fp);
   fread(&iMaxMass, sizeof(int), 1, fp);
   fread(&tNumPeptides, sizeof(uint64_t), 1, fp);

   // sanity checks
   if (iMinMass < 0 || iMinMass > 20000 || iMaxMass < 0 || iMaxMass > 20000)
   {
      char szErr[256];
      sprintf(szErr, " Error reading .idx database:  min mass %d, max mass %d, num peptides %zu\n", iMinMass, iMaxMass, tNumPeptides);
      logerr(szErr);
      fclose(fp);
      return false;
   }

   // read precursor index
   int iMaxPeptideMass10 = iMaxMass * 10;
   comet_fileoffset_t *lReadIndex = new comet_fileoffset_t[iMaxPeptideMass10];
   for (int i=0; i< iMaxPeptideMass10; i++)
      lReadIndex[i] = -1;
   fread(lReadIndex, sizeof(comet_fileoffset_t), iMaxPeptideMass10, fp);

   int iStart = (int)(g_massRange.dMinMass - 0.5);  // smallest mass/index start

   if (iStart > iMaxMass)  // Nothing to search as smallest input mass is greater than what's stored in index.
   {                       // Presumably should report this somehow if not using RTS.
      delete[] lReadIndex;
      fclose(fp);
      return true;
   }

   int iStart10 = (int)(g_massRange.dMinMass*10.0 - 0.5);  // lReadIndex is at 0.1 resolution for index value so scale iStart/iEnd to be same
   int iEnd10 = (int)(g_massRange.dMaxMass*10.0 + 0.5);

   if (iStart10 < iMinMass*10)
      iStart10 = iMinMass*10;
   if (iEnd10 > iMaxMass*10)
      iEnd10 = iMaxMass*10;

   // At this point, have read in the peptide index; now read in unmodified peptides

   // Loop through all lReadIndex until you hit a mass index value that's not -1
   while (lReadIndex[iStart10] == -1)
      iStart10++;

   comet_fseek(fp, lReadIndex[iStart10], SEEK_SET);
   struct PlainPeptideIndex sDBI;
   vector<PlainPeptideIndex> vRawPeptides;

   // Even though we just generated this plain peptide index; we're reading
   // it back in here, vs. just storing/using what was previously generated,
   // 'cause this was how it was originally developed as a separate step.
   ReadPlainPeptideIndexEntry(&sDBI, fp);

   while ((int)(sDBI.dPepMass * 10) <= iEnd10)
   {
      vRawPeptides.push_back(sDBI);
 
      if (ftell(fp) < lEndOfPeptides)
         ReadPlainPeptideIndexEntry(&sDBI, fp);
      else
         break;
   }

   delete [] lReadIndex;
   fclose(fp);

   // now permute mods on the peptides
   PermuteIndexPeptideMods(vRawPeptides);

   // vector of modified peptides
   // - raw peptide via iWhichPeptide referencing entry in vRawPeptides to access peptide and protein(s)
   // - modification encoding index
   // - modification mass
   g_uiMaxFragmentArrayIndex = BIN(g_massRange.dMaxMass);
   g_vFragmentIndex.resize(g_uiMaxFragmentArrayIndex);

   // generate the modified peptides to calculate the fragment index
   GenerateFragmentIndex(vRawPeptides, tp);

   *tSizevRawPeptides = vRawPeptides.size();

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;

   cout << " - time generating fragment index: " << minutes << " minutes " << seconds  << " seconds" << endl;

   return true;
}


void CometFragmentIndex::ReadPlainPeptideIndexEntry(struct PlainPeptideIndex *sDBI,
                                                    FILE *fp)
{
   int iLen;
   char szPeptide[MAX_PEPTIDE_LEN];

   fread(&iLen, sizeof(int), 1, fp);
   fread(szPeptide, sizeof(char), iLen, fp);
   szPeptide[iLen] = '\0';

   sDBI->sPeptide = szPeptide;
   sDBI->szPrevNextAA[0] = '-';  // currently not storing prev & next AA so set to '-' here
   sDBI->szPrevNextAA[1] = '-';

   fread(&(sDBI->dPepMass), sizeof(double), 1, fp);
   fread(&(sDBI->lIndexProteinFilePosition), sizeof(comet_fileoffset_t), 1, fp);

   // Now read in the protein file positions here
   sDBI->lIndexProteinFilePosition = comet_ftell(fp);

   long lSize;
   fread(&lSize, sizeof(long), 1, fp);

   for (long x = 0; x < lSize; x++)
   {
      comet_fileoffset_t tmpoffset;
      fread(&tmpoffset, sizeof(comet_fileoffset_t), 1, fp);
   }
}


void CometFragmentIndex::PermuteIndexPeptideMods(vector<PlainPeptideIndex>& vRawPeptides)
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

   cout << endl;
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
   PEPTIDE_MOD_SEQ_IDXS = new int[(int)vRawPeptides.size()];

   MOD_SEQS = ModificationsPermuter::getModifiableSequences(vRawPeptides, PEPTIDE_MOD_SEQ_IDXS, ALL_MODS);

   // Get the modification combinations for each unique modifiable substring
   ModificationsPermuter::getModificationCombinations(MOD_SEQS, MAX_MODS_PER_MOD, ALL_MODS,
         MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);
}


void CometFragmentIndex::GenerateFragmentIndex(vector<PlainPeptideIndex>& vRawPeptides,
                                               ThreadPool *tp)
{
   size_t iWhichPeptide = 0;
   size_t iEndSize = vRawPeptides.size();
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
      pFragmentIndexPool->doJob(std::bind(AddFragmentsThreadProc, std::ref(vRawPeptides), iWhichPeptide, std::ref(iNoModificationNumbers), pFragmentIndexPool));
#else
      AddFragmentsThreadProc(std::ref(vRawPeptides), iWhichPeptide, std::ref(iNoModificationNumbers), pFragmentIndexPool);
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

   // Walk through each g_vFragmentIndex[] and sort entries by increasing peptide mass

   tStartTime = chrono::steady_clock::now();

   for (unsigned int i = 0; i < g_uiMaxFragmentArrayIndex; i++)
   {
      if (g_vFragmentIndex[i].size() > 0)
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


void CometFragmentIndex::AddFragmentsThreadProc(vector<PlainPeptideIndex>& vRawPeptides,
                                                size_t iWhichPeptide,
                                                int& iNoModificationNumbers,
                                                ThreadPool *tp)
{
   // AddFragments(iWhichPeptide, modNumIdx) for unmodified peptide
   // FIX: if require variable mod is set, this would not be called here
   AddFragments(vRawPeptides, iWhichPeptide, -1);

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
      AddFragments(vRawPeptides, iWhichPeptide, modNumIdx);
   }
}


void CometFragmentIndex::SortFragmentThreadProc(int i,
                                                ThreadPool *tp)
{
   sort(g_vFragmentIndex[i].begin(), g_vFragmentIndex[i].end(), SortFragmentsByPepMass);
}


bool CometFragmentIndex::SortFragmentsByPepMass(unsigned int x, unsigned int y)
{
   return (g_vFragmentPeptides[x].dPepMass < g_vFragmentPeptides[y].dPepMass);
}


void CometFragmentIndex::AddFragments(vector<PlainPeptideIndex>& vRawPeptides,
                                      int iWhichPeptide,
                                      int modNumIdx)
{
   string sPeptide = vRawPeptides.at(iWhichPeptide).sPeptide;

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

         if (uiBinIon < g_uiMaxFragmentArrayIndex)
            g_vFragmentIndex[uiBinIon].push_back(uiCurrentFragmentPeptide);
         else
            printf("ERROR in AddFragments: pep %s, dBion %f, bin %d >= max %d\n", sPeptide.c_str(), dBion, uiBinIon, g_uiMaxFragmentArrayIndex);

         uiBinIon = BIN(dYion);

         if (uiBinIon < g_uiMaxFragmentArrayIndex)
            g_vFragmentIndex[uiBinIon].push_back(uiCurrentFragmentPeptide);
         else
            printf("ERROR in AddFragments: pep %s, dYion %f, bin %d >= max %d\n", sPeptide.c_str(), dYion, uiBinIon, g_uiMaxFragmentArrayIndex);

//       Threading::UnlockMutex(_vFragmentIndexMutex);
      }
   }

#if USEFRAGMENTTHREADS == 1
   Threading::UnlockMutex(_vFragmentPeptidesMutex);
#endif
}


bool CometFragmentIndex::WriteFragmentIndex(ThreadPool *tp)
{
   FILE *fp;
   bool bSucceeded;
   char szOut[SIZE_FILE+60];
   char szIndexFile[SIZE_FILE+30];
   size_t tSizevRawPeptides;

   sprintf(szIndexFile, "%s.idx2", g_staticParams.databaseInfo.szDatabase);

   if ((fp = fopen(szIndexFile, "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", szIndexFile);
      exit(1);
   }

   sprintf(szOut, "\n Creating fragment index file: ");
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

   // write PEPTIDE_MOD_SEQ_IDXS; size is vRawPeptides.size()
   fwrite(&tSizevRawPeptides, sizeof(size_t), 1, fp);
   for (size_t i = 0; i < tSizevRawPeptides; ++i)
   {
      iTmp = PEPTIDE_MOD_SEQ_IDXS[i];
      fwrite(&iTmp, sizeof(int), 1, fp);
   }
    
   // size of g_vFragmentIndex which is g_uiMaxFragmentIndexArray
   fwrite(&g_uiMaxFragmentArrayIndex, sizeof(unsigned int), 1, fp); // array size

   for (unsigned int i = 0; i < g_uiMaxFragmentArrayIndex; ++i)
   {
      size_t tNumEntries = g_vFragmentIndex[i].size();
      fwrite(&tNumEntries, sizeof(size_t), 1, fp); // index

      if (tNumEntries > 0)
      {
         for (auto it=g_vFragmentIndex[i].begin(); it!=g_vFragmentIndex[i].end(); ++it)
         {
            fwrite(&(*it), sizeof(unsigned int), 1, fp);
         }
      }
   }

   fwrite(&clPosition, sizeof(comet_fileoffset_t), 1, fp);  //write beginning of fragment index at array size

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
   bool bSucceeded;
   char szOut[SIZE_FILE+60];
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
   sprintf(szOut, " - reading file: %s\n", szIndexFile);
   logout(szOut);

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
      sprintf(szErr, " Error with index database format. Modifications (%d/%d) not parsed.", bFoundStatic, bFoundVariable);
      logerr(szErr);
      fclose(fp);
      return false;
   }


   // read fp of index
   comet_fileoffset_t clTmp;
   comet_fileoffset_t clPosition;  // position to start reading fragment index elements
#ifdef _WIN32
#ifdef _WIN64
   clTmp = sizeof(comet_fileoffset_t);              //win64
#else
   clTmp = (long long)sizeof(comet_fileoffset_t);   //win32
#endif
#else
   clTmp = sizeof(comet_fileoffset_t);              //linux
#endif

   comet_fseek(fp, -clTmp, SEEK_END);
   fread(&clPosition, sizeof(comet_fileoffset_t), 1, fp);

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

   // write MOD_SEQ_MOD_NUM_CNT; size is MOD_SEQS.size()
   for (size_t i = 0; i < tSize; ++i)
   {
      fread(&(MOD_SEQ_MOD_NUM_CNT[i]), sizeof(int), 1, fp);
   }

   // write PEPTIDE_MOD_SEQ_IDXS; size is vRawPeptides.size()
   fread(&tTmp, sizeof(size_t), 1, fp);
   PEPTIDE_MOD_SEQ_IDXS = new int[tTmp];
   for (size_t i = 0; i < tTmp; ++i)
   {
      fread(&(PEPTIDE_MOD_SEQ_IDXS[i]), sizeof(int), 1, fp);
   }

   // size of g_vFragmentIndex which is g_uiMaxFragmentIndexArray
   fread(&g_uiMaxFragmentArrayIndex, sizeof(unsigned int), 1, fp); // array size

   g_vFragmentIndex.clear();
   g_vFragmentIndex.resize(g_uiMaxFragmentArrayIndex);
   for (unsigned int i = 0; i < g_uiMaxFragmentArrayIndex; ++i)
   {
      fread(&tTmp, sizeof(size_t), 1, fp); // index

      if (tTmp> 0)
      {
         g_vFragmentIndex[i].reserve(tTmp);
         for (size_t it = 0; it != tTmp; ++it)
         {
            fread(&uiTmp, sizeof(unsigned int), 1, fp);
            g_vFragmentIndex[i].push_back(uiTmp);
         }
      }
   }

   fclose(fp);

   auto tEndTime = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(tEndTime - tStartTime);
   long minutes = duration.count() / 60000;
   long seconds = (duration.count() - minutes*60000) / 1000;

   cout << " - time reading fragment index file: " << minutes << " minutes " << seconds  << " seconds" << endl;

   return bSucceeded;
}
