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

   static bool WritePlainPeptideIndex(ThreadPool *tp);
   static bool ReadPlainPeptideIndex(void);
   static bool CreateFragmentIndex(ThreadPool *tp);
   static string ElapsedTime(std::chrono::time_point<std::chrono::steady_clock> tStartTime);
   static int WhichPrecursorBin(double dMass);
   static bool CompareByPeptide(const DBIndex &lhs,
                                const DBIndex &rhs);
   static bool CompareByMass(const DBIndex &lhs,
                             const DBIndex &rhs);

private:

   static void PermuteIndexPeptideMods(vector<PlainPeptideIndex>& vRawPeptides);
   static void GenerateFragmentIndex(ThreadPool *tp);
   static void AddFragments(vector<PlainPeptideIndex>& vRawPeptides,
                            int iWhichThread,
                            int iWhichPeptide,
                            int modNumIdx,
                            short siNtermMod,
                            short siCtermMod,
                            bool bCountOnly);
   static void AddFragmentsThreadProc(int iWhichThread,
                                      int iNumIndexingThreads,
                                      bool bCountOnly,
                                      ThreadPool *tp);
   static bool SortFragmentsByPepMass(unsigned int x,
                                      unsigned int y);
   static void SortFragmentThreadProc(int iWhichThread,
                                      ThreadPool* tp);
/*
   unsigned int       _uiBinnedIonMasses[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 1];
   unsigned int       _uiBinnedIonMassesDecoy[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 1];
   unsigned int       _uiBinnedPrecursorNL[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE];
   unsigned int       _uiBinnedPrecursorNLDecoy[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE];
*/
   static bool *_pbSearchMemoryPool;    // Pool of memory to be shared by search threads
   static bool **_ppbDuplFragmentArr;   // Number of arrays equals number of threads

   static Mutex _vFragmentPeptidesMutex;
};

#endif // _COMETFRAGMENTINDEX_H_
