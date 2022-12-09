#ifndef _MODIFICATIONSPERMUTER_H_
#define _MODIFICATIONSPERMUTER_H_

#include "Common.h"
#include "CometDataInternal.h"

class ModificationsPermuter
{
public:

   static chrono::time_point<chrono::steady_clock> startTime();
   static void endTime(chrono::time_point<chrono::steady_clock> start, string message);
   static long duration(chrono::time_point<chrono::steady_clock> start);
   static bool isModifiable(char aa, char* ALL_MODS, int MOD_CNT);
   static void printBits(long number);
   static void getCombinations(int n, int k, int nck, unsigned long* bitmasks);
   static void initCombinations(int maxPeptideLen,
                                int maxMods,
                                unsigned long** ALL_COMBINATIONS,
                                int* ALL_COMBINATION_CNT);
   static vector<string> readPeptides(string file);
   static string getModifiableAas(std::string peptide, char* ALL_MODS, int MOD_CNT);
   static vector<string> getModifiableSequences(vector<string> peptides, int* PEPTIDE_MOD_SEQ_IDXS, char* ALL_MODS, int MOD_CNT);
   static long getModBitmask(string modSeq, char modChar);
   static vector<vector<int>> getCombinationSets(int modCount);
   static int getTotalCombinationCount(vector<int> combinationCounts, vector<vector<int>> combinationSets);
   static bool combine(int* modNumbers,
                       unsigned long* bitmasks,
                       int modNumCount,
                       int modStringLen,
                       int *MOD_NUM);
   static void testCombine();
   static void testCombine2();
   static void generateModifications(string sequence,
                                     int max_mods_per_mod,
                                     int* ret_modNumStart,
                                     int* ret_modNumCount,
                                     char* ALL_MODS,
                                     int MOD_CNT,
                                     int ALL_COMBINATION_CNT,
                                     unsigned long* ALL_COMBINATIONS,
                                     int *MOD_NUM);
   static void getModificationCombinations(const vector<string> modifiableSeqs,
                                           int max_mods_per_mod,
                                           char* ALL_MODS,
                                           int MOD_CNT,
                                           int ALL_COMBINATION_CNT,
                                           unsigned long* ALL_COMBINATIONS);
   static void printModifiedPeptides(vector<string> peptides, vector<string> MOD_SEQS, int* PEPTIDE_MOD_SEQ_IDXS, char* ALL_MODS_SYM); //, string outputFile);

   ModificationsPermuter();
   ~ModificationsPermuter();
};

#endif // _MODIFICATIONSPERMUTER_H_
