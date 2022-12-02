// ModificationsPermuter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#include "pch.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <chrono>
#include <unordered_set>
#include "CombinatoricsUtils.h"
#include "ModificationNumber.h"
#include "ModificationsPermuter.h"
#include "Common.h"

//using namespace std;

ModificationsPermuter::ModificationsPermuter()
{
}
ModificationsPermuter::~ModificationsPermuter()
{
}


//bool DEBUG = false;

//char* ALL_MODS; // An array of all the user specified amino acids that can be modified
//char* ALL_MODS_SYM; // Symbols representing the modifications
//int MOD_CNT; // Size of ALL_MODS array

// Pre-computed bitmask combinations for peptides of length MAX_PEPTIDE_LEN with up to MAX_MODS_PER_MOD modified amino acids.
unsigned long* ALL_COMBINATIONS; 
int ALL_COMBINATION_CNT;

// Maximum number of bits that can be set in a modifiable sequence for a given modification.
// C(25, 5) = 53,130; C(25, 4) = 10,650; C(25, 3) = 2300.  This is more than MAX_COMBINATIONS (65,534)
int MAX_BITCOUNT = 24;
int MAX_K_VAL = 10;

int IGNORED_SEQ_CNT = 0; // Sequences that were ignored because they would generate more than MAX_COMBINATIONS combinations.
//vector<string> MOD_SEQS; // Unique modifiable sequences.
//int* PEPTIDE_MOD_SEQ_IDXS; // Index into the MOD_SEQS vector; -1 for peptides that have no modifiable amino acids.
int MOD_NUM = 0;
std::vector<ModificationNumber> MOD_NUMBERS;
int* MOD_SEQ_MOD_NUM_START; // Start index in the MOD_NUMBERS vector for a modifiable sequence; -1 if no modification numbers were generated
int* MOD_SEQ_MOD_NUM_CNT; // Total modifications numbers for a modifiable sequence.

long TIME_IN_STEP1 = 0;
long TIME_IN_STEP2 = 0;
long TIME_IN_STEP3 = 0;
long TIME_IN_COMBINE = 0;
long TIME_GEN_MODS = 0;


// https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
chrono::time_point<chrono::steady_clock> ModificationsPermuter::startTime()
{
        // Changed from high_resolution_clock::now to steady_clock::now based on this SO thread
        // https://stackoverflow.com/questions/70396570/mismatched-types-stdchrono-v2steady-clock-and-stdchrono-v2system
   return chrono::steady_clock::now();
}

void ModificationsPermuter::endTime(chrono::time_point<chrono::steady_clock> start, string message)
{
   const auto stop = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
   cout << message << " in " << duration.count() << "ms" << endl;
}

long ModificationsPermuter::duration(chrono::time_point<chrono::steady_clock> start)
{
   const auto stop = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
   return static_cast<long>(duration.count());
}

bool ModificationsPermuter::isModifiable(char aa,
                                         char* ALL_MODS,
                                         int MOD_CNT)
{
   for (int i = 0; i < MOD_CNT; i ++)
   {
      if (aa == ALL_MODS[i]) return true;
   }
   return false;
}

void ModificationsPermuter::printBits(long number)
{
   std::bitset<64> x(number);
   cout << x << endl;
}

// Generate all the n-choose-k bitmask combinations for sequences of length 'n' with 'k' modified residues
void ModificationsPermuter::getCombinations(int n, int k, int nck, unsigned long* bitmasks)
{
   int** combinations = CombinatoricsUtils::makeCombinations(n, k, nck);

   for (int i = 0; i < nck; i++)
   {
      unsigned long bitmask = 0;
      int *combination = combinations[i];
      for (int j = 0; j < k; j++)
      {
         int toOr = 1 << (combination[j]);
         // cout << combination[j] << " " << toOr << endl;
         bitmask = bitmask | toOr;
      }
      bitmasks[i] = bitmask;

      // cout << bitmasks.at(i) << " - ";
      // std::bitset<64> x(bitmasks.at(i));
      // cout << x << endl;
   }
   for (int i = 0; i < nck; i++)
   {
      delete[] combinations[i];
   }
   delete[] combinations;
}

// Generate all the bitmask combinations for sequences of length 'maxPeptideLen' with up to 'maxMods' modified residues
void ModificationsPermuter::initCombinations(int maxPeptideLen, int maxMods)
{
   cout << "Initializing combinations for peptide length " << maxPeptideLen << " and max modifications " << maxMods << endl;

   vector<unsigned long> allCombinations;
   int totalCount = 0;
   int i = maxMods;

   auto start = startTime();
   unsigned long* allCombos = new unsigned long[0];
   int currentAllCount = 0;
   while (i >= 1)
   {
      int nck = CombinatoricsUtils::nChooseK(maxPeptideLen, i);
      cout << maxPeptideLen << " choose " << i << ": " << nck << endl;
      totalCount += nck;

      unsigned long* combos = new unsigned long[nck];
      getCombinations(maxPeptideLen, i, nck, combos);

      if (totalCount == nck)
      {
         allCombos = combos;
      }
      else
      {
         // Combine the arrays so that we have all the combination bitmasks in sorted order.
         unsigned long* temp = new unsigned long[totalCount];
         int l = 0; int j = 0; int k = 0;
         while (l < currentAllCount || j < nck)
         {
            if (l >= currentAllCount)
            {
               temp[k] = combos[j];
               j++;
            }
            else if (j >= nck)
            {
               temp[k] = allCombos[l];
               l++;
            }
            else
            {
               unsigned long x = allCombos[l];
               unsigned long y = combos[j];
               if (x < y)
               {
                  temp[k] = x;
                  l++;
               }
               else if (y < x)
               {
                  temp[k] = y;
                  j++;
               }
               else
               {
                  cout << "ERROR: same value!!" << endl;
               }
            }
            k++;
         }
         
         allCombos = temp;
      }
      currentAllCount = totalCount;
      i--;
   }

   std::cout << "Total combinations: " << totalCount << endl;

/*
   if (DEBUG)
   {
      // for (int i = 0; i < totalCount; i++)
      // {
      //    cout << allCombos[i] << " - ";
      //    std::bitset<64> x(allCombos[i]);
      //    cout << x << endl;
      // }
   }
*/

   endTime(start, "Combinations initialized");
   
   ALL_COMBINATIONS =  allCombos;
   ALL_COMBINATION_CNT = totalCount;
}

vector<string> ModificationsPermuter::readPeptides(string file)
{
   vector <string> peptides;
   string line;

   cout << "Reading peptides from file " << file << endl;
   ifstream myfile(file);
   int pepCnt = 0;
   if (myfile.is_open())
   {
      const auto start = startTime();
      while (std::getline(myfile, line))
      {
         peptides.push_back(line);
         pepCnt++;
         // if (pepCnt >= 2000) break;
      }
      myfile.close();
      endTime(start, "Read " + std::to_string(peptides.size()) + " peptides");
   }
   else cout << "ERROR: Unable to open file" << endl;

   return peptides;
}

// Return a sequence comprising the amino acids in the given peptide that can have a modification. 
string ModificationsPermuter::getModifiableAas(std::string peptide,
                                               char* ALL_MODS,
                                               int MOD_CNT)
{
   string modifiableAas = "";

   int i = 0;
   for (char aa : peptide)
   {
      if (isModifiable(aa, ALL_MODS, MOD_CNT))
      {
         modifiableAas += aa;
      }
      i++;
   }
   if (modifiableAas.size() == 0) return {};
   return modifiableAas;
}

vector<string> ModificationsPermuter::getModifiableSequences(vector<string> peptides, int* PEPTIDE_MOD_SEQ_IDXS, char* ALL_MODS, int MOD_CNT)
{
   const auto start = startTime();
   std::unordered_map<string, int> modifiableSeqMap;
   vector<string> ret;
   int pepIdx = 0;
   int modSeqIdx = 0;
   int modifiablePeptides = 0;
   for (string peptide : peptides)
   {
      string modifiableAas = getModifiableAas(peptide, ALL_MODS, MOD_CNT);

      if (!modifiableAas.empty())
      {
         modifiablePeptides++;
         std::unordered_map<string, int>::iterator iter = modifiableSeqMap.find(modifiableAas);
         if (iter == modifiableSeqMap.end())
         {
            modifiableSeqMap[modifiableAas] = modSeqIdx;
            ret.push_back(modifiableAas);
            PEPTIDE_MOD_SEQ_IDXS[pepIdx] = modSeqIdx;
            modSeqIdx++;
         }
         else
         {
            int idx = iter->second;
            PEPTIDE_MOD_SEQ_IDXS[pepIdx] = idx;
         }
      }
      else
      {
         PEPTIDE_MOD_SEQ_IDXS[pepIdx] = -1;
      }
      pepIdx++;
   }

   cout << "Modifiable peptides - " << std::to_string(modifiablePeptides) << endl;
   endTime(start, "Found " + std::to_string(ret.size()) + " unique modifiable sequences");
   return ret;
}

// Iterate over the modSeq and set the bit to 1 if the amino acid at an index matches the given modChar
// Example: CMHQQQMK -> 01000010 (for modChar = 'M')
long ModificationsPermuter::getModBitmask(string modSeq, char modChar)
{
   long bitMask = 0L;
   long len = modSeq.size();
   for (int i = 0; i < len; i++)
   {
      if (modSeq[i] == modChar)
      {
         bitMask = bitMask | (1L << (len - i - 1));
      }
   }

   return bitMask;
}

// Return the number of ways we can combine the given number of modifications. 
// Example: modCount = 3
// Possible ways to combine: {1}, {2}, {3}, {1, 2}, {1, 3}, {2, 3}, {1, 2, 3}
vector<vector<int>> ModificationsPermuter::getCombinationSets(int modCount)
{
   vector<vector<int>> allSets;
   for (int i = modCount; i > 0; i--)
   {
      int nck = CombinatoricsUtils::nChooseK(modCount, i);

      int** combinationSets = CombinatoricsUtils::makeCombinations(modCount, i, nck);
      for (int j = 0; j < nck; j++)
      {
         int *combination = combinationSets[j];
         vector<int> set;
         for (int k = 0; k < i; k++)
         {
            set.push_back(combination[k]);
         }
         allSets.push_back(set);
      }

      for (int j = 0; j < nck; j++)
      {
         delete[] combinationSets[j];
      }
      delete[] combinationSets;
   }

   return allSets;
}

// Returns the number of possible modification combinations.
// Sequence: MSMSK
// combinationCounts: 1(M) -> 3 {10000, 00100, 10100}
//                    2(K) -> 1 {00001}
//                    3(S) -> 3 {01000, 00010, 01010}
// combinationSets: {1}, {2}, {3}, {1, 2}, {1, 3}, {2, 3}, {1, 2, 3}
// combinations:     3 +  1 +  3 +  3 +     9 +     3 +     9       = 31
int ModificationsPermuter::getTotalCombinationCount(vector<int> combinationCounts, vector<vector<int>> combinationSets)
{
   int allCombos = 0;
   for (vector<vector<int>>::iterator it = combinationSets.begin(); it != combinationSets.end(); ++it)
   {
      int combos = 1;
      vector<int> set = *it;
      for (unsigned int j = 0; j < set.size(); j++)
      {
         int s = set.at(j);
         combos *= combinationCounts.at(s);
         if (combos > MAX_COMBINATIONS)
         {
            return -1;
         }
      }
      allCombos += combos;
   }
   return allCombos > MAX_COMBINATIONS ? -1 : allCombos;
}

bool ModificationsPermuter::combine(int* modNumbers, unsigned long* bitmasks, int modNumCount, int modStringLen)
{
   const auto start = startTime();

   for (int j = 0; j < modNumCount; j++)
   {
      for (int k = j+1; k < modNumCount; k++)
      {
         // cout << "1: "; printBits(bitmasks[j]);
         // cout << "2: "; printBits(bitmasks[k]);
         unsigned long combined = bitmasks[j] & bitmasks[k];
         // cout << "3: "; printBits(combined);
         if (combined != 0)
         {
            // If any two modification combinations have the same bit set then this is not a valid combination.
            // This can happen when more than one modification is defined on the same amino acid.
            return false; 
         }
      }
   }

   char *mods = new char[modStringLen];
   for (int i = 0; i < modStringLen; i++) mods[i] = -1;
   char modNum;
   for (int i = 0; i < modStringLen; i++)
   {
      int idx = modStringLen - i - 1;
      for (int j = 0; j < modNumCount; j++)
      {
         long btm = bitmasks[j];
         modNum = modNumbers[j];

         // extract the i-th bit in the bitmask
         const unsigned long b = (btm & 1L << i);
         if (b > 0)
         {
            mods[idx] = modNum;
            break; // We found the modification at this index. Go to the next index.
         }
      }
   }
   
   ModificationNumber modification;
   modification.modificationNumber = MOD_NUM++;
   modification.modifications = mods;
   MOD_NUMBERS.push_back(modification);

   TIME_IN_COMBINE += duration(start);
   return true;
}

void ModificationsPermuter::testCombine()
{
   string seq = "MSMMK";
   const int modNumCount = 3;
   int modNumbers[modNumCount] = { 1,3,2 };
   unsigned long bitmasks[modNumCount] = { 20,8,1 }; // 10100 , 01000, 00001
   int seqLen = seq.length();
   combine(modNumbers, bitmasks, modNumCount, seqLen);
   cout << "Sequence " << seq << endl;
   ModificationNumber combined = MOD_NUMBERS.at(0);
   for (int i = 0; i < seqLen; i++)
   {
      cout << "ModificationNumber at " << std::to_string(i) << ": " << std::to_string(combined.modifications[i]) << endl;
   }
}

void ModificationsPermuter::testCombine2()
{
   string seq = "MSMMK";
   const int modNumCount = 3;
   int modNumbers[modNumCount] = { 1,3,2 };
   unsigned long bitmasks[modNumCount] = { 20,22,1 }; // 10100 , 10110, 00001
   int seqLen = seq.length();
   bool valid = combine(modNumbers, bitmasks, modNumCount, seqLen);
   cout << "Valid combination: " << valid << endl;
}

// Generate all the modification combinations for the given sequence of modifiable amino acids.
void ModificationsPermuter::generateModifications(string sequence, int max_mods_per_mod, int* ret_modNumStart, int* ret_modNumCount, char* ALL_MODS, int MOD_CNT)
{
   //auto startGenMods = startTime();
   vector<unsigned long> modBitmasks; // One entry in the vector for each user specified modification found in the sequence
   vector<int> modIndices; // Indices in the ALL_MODS array of the modifications found in the given sequence
   vector<int> combinationCounts; // Number of combinations that would be generated for each modification

   *ret_modNumStart = -1;
   *ret_modNumCount = 0;

   // Step 1: Get a bitmask representing each user specified modification found in the sequence.
   //const auto startStep1 = startTime();
   for (int m = 0; m < MOD_CNT; m++)
   {
      const char modChar = ALL_MODS[m];
      const long bitmask = getModBitmask(sequence, modChar); // Example: CMHQQQMK -> 01000010 (for modChar = 'M')
      if (bitmask != 0)
      {
         std::bitset<64> x(bitmask);
         long bitCount = x.count();
         if (bitCount > MAX_BITCOUNT)
         {
            IGNORED_SEQ_CNT++;
            return; 
         }
         int combinationCount = CombinatoricsUtils::getCombinationCount(int(bitCount), max_mods_per_mod); // nCk + nCk-1 +...+nC1
         if (combinationCount > MAX_COMBINATIONS)
         {
            IGNORED_SEQ_CNT++;
            return;
         }

         modIndices.push_back(m);
         modBitmasks.push_back(bitmask);
         combinationCounts.push_back(combinationCount);
      }
   }
   //TIME_IN_STEP1 += duration(startStep1);

   // Step 2: Generate all the bitmask combinations for each modification found in the sequence
   //const auto startStep2 = startTime();
   int idx = 0;
   unsigned long ** combinationsForAllMods = new unsigned long*[modIndices.size()];
   // Iterate over the bitmasks for the modifications found in the given sequence
   for (vector<unsigned long>::iterator it = modBitmasks.begin(); it != modBitmasks.end(); ++it)
   {
      unsigned long modBitmask = *it; // bitmask for a modification
      const int combinationsCount = combinationCounts[idx]; // number of possible combinations

      unsigned long *combinationsForMod = new unsigned long[combinationsCount];

      long notMod = ~modBitmask;
      // Iterate over the pre-computed bitmask combinations. Keep the ones where one or more of bits set in the given bitmask
      // are also set in the combination bitmask.
      // Example: for MGGMAVMK
      // Bitmask for M: 10010010 (3 methionines)
      // Possible bitmask combinations for the modification: 
      // 00000010
      // 00010000
      // 00010010
      // 10000000
      // 10000010
      // 10010000
      // 10010010
      int comboForModIdx = 0;
      for (int j = 0; j < ALL_COMBINATION_CNT; j++)
      {
         const unsigned long combination = ALL_COMBINATIONS[j];
         if (combination > modBitmask) break;
         if ((combination & notMod) == 0) // Only the bits set in the modification bitmask should be set in the combination.
         {
            combinationsForMod[comboForModIdx++] = combination;
         }
      }
      if (comboForModIdx != combinationsCount) // Number of combinations found should be the same as the expected number.
      {
         cout << "ERROR: Unexpected combination count; comboForModIdx is " << to_string(comboForModIdx) 
              << "; combinationsCount is " << to_string(combinationsCount) << endl;
      }
      combinationsForAllMods[idx++] = combinationsForMod;
   }
   //TIME_IN_STEP2 += duration(startStep2);

/*
   if (DEBUG)
   {
      // for (int i = 0; i < modIndex.size(); i++)
      // {
      //    unsigned long* bitmasks = combinationsForAllMods[i];
      //    int combinationCount = combinationCounts[i];
      //    for (int j = 0; j < combinationCount; j++)
      //    {
      //       unsigned long bitmask = bitmasks[j];
      //       // cout << bitmasks.at(i) << " - ";
      //       std::bitset<64> x(bitmask);
      //       cout << x << endl;
      //    }
      // }
   }
*/

   // Step 3: We have all the possible combinations for each modification. Combine them to generate modification numbers.
   const auto startStep3 = startTime();

   vector<vector<int>>   combinationSets = getCombinationSets(modIndices.size());
   const int combinationCount = getTotalCombinationCount(combinationCounts, combinationSets);
   
   if (combinationCount != -1)
   {
      int totalModNumCount = 0;
      const int startModNum = MOD_NUM;

//      if (DEBUG) cout << "set count " << combinationSets.size() << endl;

      for (vector<vector<int>>::iterator it = combinationSets.begin(); it != combinationSets.end(); ++it)
      {   
         vector<int> set = *it;

         const int modNumCount = set.size();
         int* modIndicesToMerge = new int[modNumCount];
         int* combinationCountsToMerge = new int[modNumCount];
         int s = 0;
         for (vector<int>::iterator set_it = set.begin(); set_it != set.end(); ++set_it)
         {
            const int set_el = *set_it;
            modIndicesToMerge[s] = set_el;
            combinationCountsToMerge[s] = combinationCounts.at(set_el);
            s++;
         }
         
         int *currIdx = new int[modNumCount]; // Current index for each modification
         for (int i = 0; i < modNumCount; i++) currIdx[i] = 0;

         int modNumCalculated = 0;
         
         while (true)
         {
            unsigned long* toCombine = new unsigned long[modNumCount];

            int c = 0;
            for (int i = 0; i < modNumCount; i++)
            {
               const int modIdx = modIndicesToMerge[i];
               unsigned long* combinationsForModIdx = combinationsForAllMods[modIdx];
               const int y = currIdx[i];
               const unsigned long combination = combinationsForModIdx[y];
               toCombine[c++] = combination;
            }

            int *modNumbers = new int[modNumCount]; // index of the modification in ALL_MODS
            for (int k = 0; k < modNumCount; k++)
            {
               modNumbers[k] = modIndices.at(modIndicesToMerge[k]);
            }
            if (combine(modNumbers, toCombine, modNumCount, sequence.length()))
            {
               modNumCalculated++;
            }

            delete[] modNumbers;
            delete[] toCombine;
            
            int next = modNumCount - 1;
            while (next >= 0 && (currIdx[next] == combinationCountsToMerge[next] - 1))
            {
               next--;
            }
            if (next < 0) break; // We are done

            currIdx[next]++;  // Go on to the next element in the list

            for (int i = next + 1; i < modNumCount; i++)
            {
               currIdx[i] = 0;
            }
         }
//         if (DEBUG) cout << "merged" << endl;

         totalModNumCount += modNumCalculated;

         delete[] currIdx;
         delete[] modIndicesToMerge;
         delete[] combinationCountsToMerge;
      }
      if (MOD_NUM != startModNum + totalModNumCount)
      {
         cout << "Unexpected end index: " + std::to_string(MOD_NUM) << "; Expected: " << std::to_string(startModNum + totalModNumCount) << endl;
      }
      *ret_modNumStart = startModNum;
      *ret_modNumCount = totalModNumCount;
   }
   else
   {
      IGNORED_SEQ_CNT++; // Number of possible combinations exceed the cutoff.
   }

   TIME_IN_STEP3 += duration(startStep3);

   combinationSets.clear();
   for (unsigned int i = 0; i < modIndices.size(); ++i) 
   {
      delete[] combinationsForAllMods[i];
   }
   delete[] combinationsForAllMods;

   modBitmasks.clear();
   modIndices.clear();
   combinationCounts.clear();

   //TIME_GEN_MODS += duration(startGenMods);
}

void ModificationsPermuter::getModificationCombinations(const vector<string> modifiableSeqs, int max_mods_per_mod, char* ALL_MODS, int MOD_CNT)
{
   const auto start = startTime();

   MOD_SEQ_MOD_NUM_START = new int[modifiableSeqs.size()];
   MOD_SEQ_MOD_NUM_CNT = new int[modifiableSeqs.size()];

   CombinatoricsUtils::initBinomialCoefficients(MAX_BITCOUNT, MAX_K_VAL);

   int i = 0;
   for (auto it = begin(modifiableSeqs); it != end(modifiableSeqs); ++it)
   {
      string modSeq = *it;
//      if (DEBUG) cout << "Calculating modifications for " << to_string(i) << " " << modSeq << endl;
      int modNumStart = -1, modNumCount = 0;
      generateModifications(modSeq, max_mods_per_mod, &modNumStart, &modNumCount, ALL_MODS, MOD_CNT);
      MOD_SEQ_MOD_NUM_START[i] = modNumStart;
      MOD_SEQ_MOD_NUM_CNT[i] = modNumCount;
      if (i > 0 && i % 10000 == 0) cout << "Done " << to_string(i) << endl;
      i++;
   }
   cout << "Ignored sequences: " << to_string(IGNORED_SEQ_CNT) << endl;
   endTime(start, "Generated modification combinations");
   // cout << "Time in Step1 (prepare): " << to_string(TIME_IN_STEP1) << endl;
   // cout << "Time in Step2 (get all combinations): " << to_string(TIME_IN_STEP2) << endl;
   cout << "Time in Step3 (merge combinations): " << to_string(TIME_IN_STEP3) << endl;
   cout << "Time in combine: " << to_string(TIME_IN_COMBINE) << endl;
   // cout << "Total time generating mods: " << to_string(TIME_GEN_MODS) << endl;
}

void ModificationsPermuter::printModifiedPeptides(vector<string> peptides,
                                                  vector<string> MOD_SEQS,
                                                  int* PEPTIDE_MOD_SEQ_IDXS,
                                                  char* ALL_MODS_SYM)  //, string outputFile)
{
// cout << "Printing modified peptides to " << outputFile << endl;
   cout << "\nPrinting modified peptides" << endl;

   std::string buffer;
   const int BUFFER_SIZE = 1024 * 10 * 10;
   buffer.reserve(BUFFER_SIZE);

   const auto start = startTime();
//   ofstream output;
//   output.open(outputFile);

   int i = 0;
   for (vector<string>::iterator it = peptides.begin(); it != peptides.end(); ++it)
   {
      string peptide = *it;
      
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[i];
      i++;

      if (modSeqIdx == -1)
      {
//         if (DEBUG) cout << "Not modified - " << to_string(i) << ". " << peptide << endl;
         continue;
      }
      int startIdx = MOD_SEQ_MOD_NUM_START[modSeqIdx];

      if (startIdx == -1)
      {
//         if (DEBUG) cout << "No modification numbers - " << to_string(i) << ". " << peptide << endl;
         continue;
      }

      string modSeq = MOD_SEQS.at(modSeqIdx);
      int modSeqLen = modSeq.size();
      int* indexInPep = new int[modSeqLen];
      int pepLen = peptide.length();
      int j = 0;
      int k = 0;
      for (; j < modSeqLen; j++)
      {
         char mod_aa = modSeq[j];
         while(k < pepLen)
         {
            char aa = peptide[k];
            if (mod_aa == aa)
            {
               indexInPep[j] = k++;
               break;
            }
            k++;
         }
      }

      // output << "--------------------------------------" << endl;
      // output << to_string(i) << ". " << peptide << endl;
      buffer.append("--------------------------------------\n");
      buffer.append(to_string(i) + ". " + peptide + " -->" + modSeq + "\n");
      // buffer.append(to_string(i) + ". " + peptide + "\n");

      int modNumCount = MOD_SEQ_MOD_NUM_CNT[modSeqIdx];
      for (int modNumIdx = startIdx; modNumIdx < startIdx + modNumCount; modNumIdx++)
      {
         ModificationNumber modNum = MOD_NUMBERS.at(modNumIdx);
         char* mods = modNum.modifications;
         string modifiedPep = "";
         int last = 0;
         for (int l = 0; l < modSeqLen; l++)
         {
            int iInPep = indexInPep[l];
            modifiedPep += peptide.substr(last, iInPep - last + 1);
            if (mods[l] != -1)
            {
               modifiedPep += mods[i]; //ALL_MODS_SYM[(int)mods[l]]; //  '*';
            }
            last = iInPep + 1;
         }
         if (last < pepLen)
         {
            modifiedPep += peptide.substr(last);
         }

         if (buffer.length() + modifiedPep.length() + 1 > BUFFER_SIZE)
         {
//          output << buffer;
            cout << buffer;
            buffer.resize(0);
         }
         buffer.append(modifiedPep.append("\n"));
//          output << modifiedPep << endl;
            cout << modifiedPep << endl;
      }

      delete[] indexInPep;
   }
// output << buffer;
// output.close();
   endTime(start, "Done printing");
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
