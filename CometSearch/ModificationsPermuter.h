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


#ifndef _MODIFICATIONSPERMUTER_H_
#define _MODIFICATIONSPERMUTER_H_

#include "Common.h"
#include "CometDataInternal.h"
#include "Modifications.h"

class ModificationsPermuter
{
public:
   static void initCombinations(int maxPeptideLen,
                                int maxMods,
                                unsigned long long** ALL_COMBINATIONS,
                                int* ALL_COMBINATION_CNT);
   static void getModificationCombinations(const vector<string> modifiableSeqs,
                                           Modifications& ALL_MODS,
                                           int MOD_CNT,
                                           int ALL_COMBINATION_CNT,
                                           unsigned long long* ALL_COMBINATIONS);
   static vector<string> getModifiableSequences(vector<PlainPeptideIndex>& vRawPeptides,
                                                int* PEPTIDE_MOD_SEQ_IDXS,
                                                Modifications& ALL_MODS);
   static string getModifiableAas(Peptide peptide,
                                  Modifications& ALL_MODS);
   static string getModSeqNoTerms(string modString);

private:
   static bool isModifiable(char aa,
                            Modifications& ALL_MODS);
   static void printBits(unsigned long long number);
   static void getCombinations(int n,
                               int k,
                               unsigned long long nck,
                               unsigned long long* bitmasks);
   static vector<string> readPeptides(string file);
   static unsigned long long getModBitmask(string* modSeq);
   static vector<vector<int>> getCombinationSets(size_t modCount);
   static long long getTotalCombinationCount(vector<unsigned long long> combinationCounts,
                                             vector<vector<int>> combinationSets);
   static bool combine(int* modNumbers,
                       unsigned long long* bitmasks,
                       size_t modNumCount,
                       string modString);
   static void generateModifications(string* sequence,
                                     int* ret_modNumStart,
                                     int* ret_modNumCount,
                                     Modifications& ALL_MODS,
                                     int MOD_CNT,
                                     int ALL_COMBINATION_CNT,
                                     unsigned long long* ALL_COMBINATIONS);
   static bool ignorePeptidesWithTooManyMods(void);

   ModificationsPermuter();
   ~ModificationsPermuter();
};

#endif // _MODIFICATIONSPERMUTER_H_
