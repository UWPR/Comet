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


#ifndef _COMETPEPTIDEINDEX_H_
#define _COMETPEPTIDEINDEX_H_

#include "Common.h"
//#include "CometDataInternal.h"
#include "CometPostAnalysis.h"
#include "CometSearch.h"
#include "CometStatus.h"
#include "CometMassSpecUtils.h"
#include "CometFragmentIndex.h"
#include "ThreadPool.h"



class CometPeptideIndex
{
public:
   CometPeptideIndex();
   ~CometPeptideIndex();

   static bool WritePeptideIndex(ThreadPool *tp);
   static void ReadPeptideIndexEntry(struct DBIndex *sDBI, FILE *fp);

private:

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

#endif // _COMETPEPTIDEINDEX_H_
