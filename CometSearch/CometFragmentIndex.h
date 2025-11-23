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

class CometFragmentIndex
{
public:
   CometFragmentIndex();
   ~CometFragmentIndex();

   static bool WritePlainPeptideIndex(ThreadPool *tp);
   static bool ReadPlainPeptideIndex(void);
   static bool CreateFragmentIndex(ThreadPool *tp);
   static int WhichPrecursorBin(double dMass);

private:

   static void PermuteIndexPeptideMods(std::vector<PlainPeptideIndexStruct>& vRawPeptides);
   static void GenerateFragmentIndex(ThreadPool *tp);
   static void AddFragments(std::vector<PlainPeptideIndexStruct>& vRawPeptides,
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
