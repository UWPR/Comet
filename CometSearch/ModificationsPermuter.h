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
