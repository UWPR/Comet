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

   static bool WriteFIPlainPeptideIndex(ThreadPool *tp);
   static bool GeneratePlainPeptideIndex(ThreadPool *tp, vector<pair<size_t,size_t>>& slices);

   // bIsRTS: true if called (directly or via CreateFragmentIndex()) from the RTS
   // single-spectrum-search init path (InitializeSingleSpectrumSearch()), false
   // if called from a batch search path. Reserved for RTS-vs-batch-specific
   // behavior (e.g. logging); no such behavior exists yet.
   static bool ReadPlainPeptideIndex(bool bIsRTS);
   static bool CreateFragmentIndex(ThreadPool *tp, bool bIsRTS);
   static int WhichPrecursorBin(double dMass);

   // Public for reuse by CometPeptideIndex (PI_DB build, see
   // docs/20260713_PIidxformat.md Phase B): builds MOD_SEQS/MOD_NUMBERS/
   // MOD_SEQ_MOD_NUM_START/CNT/PEPTIDE_MOD_SEQ_IDXS from g_vRawPeptides.
   static void PermuteIndexPeptideMods(vector<PlainPeptideIndexStruct>& vRawPeptides);

private:

   static void GenerateFragmentIndex(ThreadPool *tp);
   static void AddFragments(vector<PlainPeptideIndexStruct>& vRawPeptides,
                            size_t iWhichPeptide,
                            size_t iWhichFragmentPeptide,
                            int modNumIdx,
                            char cNtermMod,
                            char cCtermMod,
                            bool bCountOnly);
   static void AddFragmentsThreadProc(bool bCountOnly,
                                      ThreadPool *tp);

   static bool *_pbSearchMemoryPool;    // Pool of memory to be shared by search threads
   static bool **_ppbDuplFragmentArr;   // Number of arrays equals number of threads

   static Mutex _vFragmentPeptidesMutex;
};

#endif // _COMETFRAGMENTINDEX_H_
