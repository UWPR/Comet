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


#ifndef _COMETMODIFICATIONSPERMUTER_H_
#define _COMETMODIFICATIONSPERMUTER_H_

class ModificationsPermuter
{
public:

   static std::chrono::time_point<std::chrono::steady_clock> startTime();
   static long duration(std::chrono::time_point<std::chrono::steady_clock> start);
   static bool isModifiable(char aa,
                            std::vector<std::string>& ALL_MODS);
   static void printBits(unsigned long long number);
   static void getCombinations(int n,
                               int k,
                               int nck,
                               unsigned long long* bitmasks);
   static void initCombinations(int maxPeptideLen,
                                int maxMods,
                                unsigned long long** ALL_COMBINATIONS,
                                int* ALL_COMBINATION_CNT);
   static std::vector<std::string> readPeptides(std::string file);
   static std::string getModifiableAas(std::string peptide,
                                  std::vector<std::string>& ALL_MODS);
   static std::vector<std::string> getModifiableSequences(std::vector<PlainPeptideIndexStruct>& vRawPeptides,
                                                int* PEPTIDE_MOD_SEQ_IDXS,
                                                std::vector<std::string>& ALL_MODS);
   static unsigned long long getModBitmask(std::string* modSeq,
                                           std::string sModChars);
   static std::vector<std::vector<int>> getCombinationSets(int modCount);
   static int getTotalCombinationCount(std::vector<int> combinationCounts,
                                       std::vector<std::vector<int>> combinationSets);
   static bool combine(int* modNumbers,
                       unsigned long long* bitmasks,
                       int modNumCount,
                       int modStringLen);
   static void generateModifications(std::string* sequence,
                                     std::vector<int>& vMaxNumVarModsPerMod,
                                     int* ret_modNumStart,
                                     int* ret_modNumCount,
                                     std::vector<std::string>& ALL_MODS,
                                     int MOD_CNT,
                                     int ALL_COMBINATION_CNT,
                                     unsigned long long* ALL_COMBINATIONS);
   static void getModificationCombinations(const std::vector<std::string> modifiableSeqs,
                                           std::vector<int>& vMaxNumVarModsPerMod,
                                           std::vector<std::string>& ALL_MODS,
                                           int MOD_CNT,
                                           int ALL_COMBINATION_CNT,
                                           unsigned long long* ALL_COMBINATIONS);
   static bool ignorePeptidesWithTooManyMods(void);

   ModificationsPermuter();
   ~ModificationsPermuter();
};

#endif // _COMETMODIFICATIONSPERMUTER_H_
