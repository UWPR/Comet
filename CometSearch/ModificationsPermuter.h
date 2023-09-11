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

#ifndef _MODIFICATIONSPERMUTER_H_
#define _MODIFICATIONSPERMUTER_H_

#include "Common.h"
#include "CometDataInternal.h"

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
                                     int max_mods_per_mod,
                                     int* ret_modNumStart,
                                     int* ret_modNumCount,
                                     vector<string>& ALL_MODS,
                                     int MOD_CNT,
                                     int ALL_COMBINATION_CNT,
                                     unsigned long long* ALL_COMBINATIONS);
   static void getModificationCombinations(const vector<string> modifiableSeqs,
                                           int max_mods_per_mod,
                                           vector<string>& ALL_MODS,
                                           int MOD_CNT,
                                           int ALL_COMBINATION_CNT,
                                           unsigned long long* ALL_COMBINATIONS);
   static bool ignorePeptidesWithTooManyMods(void);

   ModificationsPermuter();
   ~ModificationsPermuter();
};

#endif // _MODIFICATIONSPERMUTER_H_
