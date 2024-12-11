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

class ModificationsPermuter
{
public:

   static chrono::time_point<chrono::steady_clock> startTime();
   static long duration(chrono::time_point<chrono::steady_clock> start);
   static bool isModifiable(char aa,
                            vector<string>& ALL_MODS);
   static void printBits(unsigned long long number);
   static void getCombinations(int n,
                               int k,
                               int nck,
                               unsigned long long* bitmasks);
   static void initCombinations(int maxPeptideLen,
                                int maxMods,
                                unsigned long long** ALL_COMBINATIONS,
                                int* ALL_COMBINATION_CNT);
   static vector<string> readPeptides(string file);
   static string getModifiableAas(std::string peptide,
                                  vector<string>& ALL_MODS);
   static vector<string> getModifiableSequences(vector<PlainPeptideIndex>& vRawPeptides,
                                                int* PEPTIDE_MOD_SEQ_IDXS,
                                                vector<string>& ALL_MODS);
   static unsigned long long getModBitmask(string* modSeq,
                                           string sModChars);
   static vector<vector<int>> getCombinationSets(int modCount);
   static int getTotalCombinationCount(vector<int> combinationCounts,
                                       vector<vector<int>> combinationSets);
   static bool combine(int* modNumbers,
                       unsigned long long* bitmasks,
                       int modNumCount,
                       int modStringLen);
   static void generateModifications(string* sequence,
                                     vector<int>& vMaxNumVarModsPerMod,
                                     int* ret_modNumStart,
                                     int* ret_modNumCount,
                                     vector<string>& ALL_MODS,
                                     int MOD_CNT,
                                     int ALL_COMBINATION_CNT,
                                     unsigned long long* ALL_COMBINATIONS);
   static void getModificationCombinations(const vector<string> modifiableSeqs,
                                           vector<int>& vMaxNumVarModsPerMod,
                                           vector<string>& ALL_MODS,
                                           int MOD_CNT,
                                           int ALL_COMBINATION_CNT,
                                           unsigned long long* ALL_COMBINATIONS);
   static bool ignorePeptidesWithTooManyMods(void);

   ModificationsPermuter();
   ~ModificationsPermuter();
};

#endif // _MODIFICATIONSPERMUTER_H_
