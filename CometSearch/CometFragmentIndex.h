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

   static bool WritePlainPeptideIndex(ThreadPool *tp);
   static bool WriteFragmentIndex(string strIndexFile,
                                  comet_fileoffset_t lPeptidesFilePos,
                                  comet_fileoffset_t lProteinsFilePos,
                                  ThreadPool *tp);
   static bool ReadFragmentIndex(ThreadPool *tp);
   static bool ReadPlainPeptideIndex(void);
   static bool CreateFragmentIndex(ThreadPool *tp);
   static string ElapsedTime(std::chrono::time_point<std::chrono::steady_clock> tStartTime);

private:

   static void PermuteIndexPeptideMods(vector<PlainPeptideIndex>& vRawPeptides);
   static void GenerateFragmentIndex(vector<PlainPeptideIndex>& vRawPeptides,
                              ThreadPool *tp);
   static void AddFragments(vector<PlainPeptideIndex>& vRawPeptides,
                            int iWhichThread,
                            int iWhichPeptide,
                            int modNumIdx,
                            short siNtermMod,
                            short siCtermMod);
   static void AddFragmentsThreadProc(vector<PlainPeptideIndex>& vRawPeptides,
                                      int iWhichThread,
                                      int iNumIndexingThreads,
                                      int& iNoModificationNumbers,
                                      ThreadPool *tp);
   static bool SortFragmentsByPepMass(unsigned int x,
                                      unsigned int y);
   static void SortFragmentThreadProc(int i,
                                      int iNumIndexingThreads,
                                      ThreadPool *tp);
   static bool CompareByPeptide(const DBIndex &lhs,
                                const DBIndex &rhs);
   static bool CompareByMass(const DBIndex &lhs,
                             const DBIndex &rhs);

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
