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

#ifndef _COMETFRAGMENTINDEX_H_
#define _COMETFRAGMENTINDEX_H_

#include "Common.h"
#include "CometDataInternal.h"
#include "CometSearch.h"
#include <functional>

class CometFragmentIndex
{
public:
   CometFragmentIndex();
   ~CometFragmentIndex();

   // Manages memory in the search memory pool
   static bool AllocateMemory(int maxNumThreads);
   static bool DeallocateMemory(int maxNumThreads);
   static bool WriteFragmentIndex(ThreadPool *tp);
   static bool ReadFragmentIndex(ThreadPool *tp);

private:

   // Core search functions
   static int BinarySearchIndexMass(int start,
                                    int end,
                                    double dQueryMass,
                                    int *iFragmentMass);
   static void XcorrScoreI(char *szProteinSeq,
                   int iStartPos,
                   int iEndPos,
                   int iFoundVariableMod,
                   double dCalcPepMass,
                   bool bDecoyPep,
                   int iWhichQuery,
                   int iLenPeptide,
                   int *piVarModSites,
                   struct sDBEntry *dbe,
                   unsigned int uiBinnedIonMasses[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN][BIN_MOD_COUNT]);
   bool CheckMassMatch(int iWhichQuery,
                       double dCalcPepMass);
   static double GetFragmentIonMass(int iWhichIonSeries,
                             int i,
                             int ctCharge,
                             double *pdAAforward,
                             double *pdAAreverse);
   static void StorePeptideI(int iWhichQuery,
                     int iStartPos,
                     int iEndPos,
                     int iFoundVariableMod,
                     char *szProteinSeq,
                     double dCalcPepMass,
                     double dXcorr,
                     bool bStoreSeparateDecoy,
                     int *piVarModSites,
                     struct sDBEntry *dbe);
   static void PermuteIndexPeptideMods(vector<PlainPeptideIndex>& vRawPeptides);
   static void GenerateFragmentIndex(vector<PlainPeptideIndex>& vRawPeptides,
                              ThreadPool *tp);
   static void PrintFragmentIndex(vector<PlainPeptideIndex>& vRawPeptides);
   static void AddFragments(vector<PlainPeptideIndex>& vRawPeptides,
                            int iWhichPeptide,
                            int modNumIdx);
   static void ReadPlainPeptideIndexEntry(struct PlainPeptideIndex *sDBI,
                                          FILE *fp);
   static void AddFragmentsThreadProc(vector<PlainPeptideIndex>& vRawPeptides,
                                      size_t iWhichPeptide,
                                      int& iNoModificationNumbers,
                                      ThreadPool *tp);
   static bool SortFragmentsByPepMass(unsigned int x,
                                      unsigned int y);
   static void SortFragmentThreadProc(int i,
                                      ThreadPool *tp);
   static bool CreateFragmentIndex(size_t *tSizevRawPeptides,
                                   ThreadPool *tp);


   // Cleaning up
   void CleanUp();

   unsigned int       _uiBinnedIonMasses[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN][BIN_MOD_COUNT];
   unsigned int       _uiBinnedIonMassesDecoy[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN][BIN_MOD_COUNT];
   unsigned int       _uiBinnedPrecursorNL[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE];
   unsigned int       _uiBinnedPrecursorNLDecoy[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE];

   static bool *_pbSearchMemoryPool;    // Pool of memory to be shared by search threads
   static bool **_ppbDuplFragmentArr;   // Number of arrays equals number of threads

   static Mutex _vFragmentIndexMutex;
   static Mutex _vFragmentPeptidesMutex;

};

#endif // _COMETFRAGMENTINDEX_H_
