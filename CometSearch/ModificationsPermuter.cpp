// Copyright 2023 Vagisha Sharma, Jimmy Eng
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
#include "Modifications.h"
#include "ModificationRule.h"
#include "ModificationsPermuter.h"
#include "CometFragmentIndex.h"
#include "Common.h"


using namespace std;


ModificationsPermuter::ModificationsPermuter()
{
}
ModificationsPermuter::~ModificationsPermuter()
{
}

#define DEBUG false

// Pre-computed bitmask combinations for peptides of length MAX_PEPTIDE_LEN with up to FRAGINDEX_MAX_MODS_PER_MOD modified amino acids.
unsigned long long* ALL_COMBINATIONS; 
unsigned long long ALL_COMBINATION_CNT;

//int IGNORED_SEQ_CNT = 0; // Sequences that were ignored because they would generate more than FRAGINDEX_MAX_COMBINATIONS combinations.

const bool RUN_TESTS = false;

void ModificationsPermuter::printBits(unsigned long long number)
{
   std::bitset<64> x(number);
   cout << x << endl;
}

// Generate all the n-choose-k bitmask combinations for sequences of length 'n' with 'k' modified residues
void ModificationsPermuter::getCombinations(int n, int k, unsigned long long nck, unsigned long long* bitmasks)
{
   int** combinations = CombinatoricsUtils::makeCombinations(n, k, nck);

   for (unsigned long long i = 0; i < nck; ++i)
   {
      unsigned long long bitmask = 0ULL;
      int *combination = combinations[i];
      for (int j = 0; j < k; ++j)
      {
         unsigned long long toOr = 1ULL << (combination[j]);
         // cout << combination[j] << " " << toOr << endl;
         bitmask = bitmask | toOr;
      }
      bitmasks[i] = bitmask;

      // cout << bitmasks[i] << " - ";
      // std::bitset<64> x(bitmask);
      // cout << x << endl;
   }
   for (unsigned long long i = 0; i < nck; ++i)
   {
      delete[] combinations[i];
   }
   delete[] combinations;
}

// Generate all the bitmask combinations for sequences of length 'maxPeptideLen' with up to 'maxMods' modified residues
void ModificationsPermuter::initCombinations(int maxPeptideLen,
                                             int maxMods,
                                             unsigned long long** ALL_COMBINATIONS,
                                             int* ALL_COMBINATION_CNT)
{
   cout << " - initializing combinations (peptide length " << maxPeptideLen << ", max mods " << maxMods;

   vector<unsigned long long> allCombinations;
   unsigned long long totalCount = 0;
   int i = maxMods;

   unsigned long long* allCombos = 0; //new unsigned long long[0];
   unsigned long long currentAllCount = 0;

   while (i >= 1)
   {
      unsigned long long nck = CombinatoricsUtils::nChooseK(maxPeptideLen, i);
//    cout << maxPeptideLen << " choose " << i << ": " << nck << endl;
      totalCount += nck;

      unsigned long long* combos = new unsigned long long[nck];
      getCombinations(maxPeptideLen, i, nck, combos);

      if (totalCount == nck)
      {
         allCombos = combos;
      }
      else
      {
         // Combine the arrays so that we have all the combination bitmasks in sorted order.
         unsigned long long* temp = new unsigned long long[totalCount];
         unsigned long long l = 0;
         unsigned long long j = 0;
         unsigned long long k = 0;

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
               unsigned long long x = allCombos[l];
               unsigned long long y = combos[j];
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

   std::cout << ", total combinations: " << totalCount << ")" << endl;

/*
   if (DEBUG)
   {
       for (unsigned long long i = 0; i < totalCount; ++i)
       {
           std::bitset<64> x(allCombos[i]);
           cout << x << " - "  << allCombos[i] << endl;
       }
   }
*/
   
   *ALL_COMBINATIONS =  allCombos;
   *ALL_COMBINATION_CNT = totalCount;
}

// Return a sequence comprising the amino acids in the given peptide that can have a modification.
string ModificationsPermuter::getModifiableAas(Peptide peptide,
                                               Modifications& ALL_MODS)
{
   string modifiableAas = ALL_MODS.getModifiableSequence(peptide);

   if (modifiableAas.size() == 0)
      return {};

   return modifiableAas;
}

vector<string> ModificationsPermuter::getModifiableSequences(vector<PlainPeptideIndex>& vRawPeptides,
                                                             int* PEPTIDE_MOD_SEQ_IDXS,
                                                             Modifications& ALL_MODS)
{
   std::unordered_map<string, int> modifiableSeqMap;
   vector<string> ret;
   int pepIdx = 0;
   int modSeqIdx = 0;
   int modifiablePeptides = 0;

   for (auto it = vRawPeptides.begin(); it != vRawPeptides.end(); ++it)
   {
      Peptide pPeptide;

      pPeptide.sequence = (*it).sPeptide;
      pPeptide.nterm = false;    // FIX set these
      pPeptide.cterm = false;

      string modifiableAas = getModifiableAas(pPeptide, ALL_MODS);

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

   cout << " - " << std::to_string(modifiablePeptides) << " modifiable peptides; " << std::to_string(ret.size()) << " unique modifiable sequences" << endl;
   return ret;
}


// Return the number of ways we can combine the given number of modifications. 
// Example: modCount = 3
// Possible ways to combine: {1}, {2}, {3}, {1, 2}, {1, 3}, {2, 3}, {1, 2, 3}
vector<vector<int>> ModificationsPermuter::getCombinationSets(size_t modCount)
{
   vector<vector<int>> allSets;
   for (int i = modCount; i > 0; i--)
   {
      unsigned long long nck = CombinatoricsUtils::nChooseK(modCount, i);

      int** combinationSets = CombinatoricsUtils::makeCombinations(modCount, i, nck);
      for (unsigned long long j = 0; j < nck; ++j)
      {
         int *combination = combinationSets[j];
         vector<int> set;
         for (int k = 0; k < i; ++k)
         {
            set.push_back(combination[k]);
         }
         allSets.push_back(set);
      }

      for (unsigned long long j = 0; j < nck; ++j)
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
long long ModificationsPermuter::getTotalCombinationCount(vector<unsigned long long> combinationCounts, vector<vector<int>> combinationSets)
{
   unsigned long long allCombos = 0;
   for (vector<vector<int>>::iterator it = combinationSets.begin(); it != combinationSets.end(); ++it)
   {
      unsigned long long combos = 1;
      vector<int> set = *it;
      for (unsigned int j = 0; j < set.size(); ++j)
      {
         int s = set.at(j);
         combos *= combinationCounts.at(s);
         if (combos > FRAGINDEX_MAX_COMBINATIONS)
         {
            return -1;
         }
      }
      allCombos += combos;
   }
   return allCombos > FRAGINDEX_MAX_COMBINATIONS ? -1 : allCombos;
}

string ModificationsPermuter::getModSeqNoTerms(string modString)
{
   string modStringNoTerms = modString;
   if (modStringNoTerms[0] == Modifications::NTERM) modStringNoTerms = modStringNoTerms.substr(1);
   if (modStringNoTerms.length() > 0 && modStringNoTerms[0] == Modifications::CTERM) modStringNoTerms = modStringNoTerms.substr(1);
   return modStringNoTerms;
}

bool ModificationsPermuter::combine(int* modNumbers,
                                    unsigned long long* bitmasks,
                                    size_t modNumCount,
                                    string modString)
{
   int modStringLen = modString.length();

   for (size_t j = 0; j < modNumCount; ++j)
   {
      for (size_t k = j+1; k < modNumCount; ++k)
      {
         // cout << "1: "; printBits(bitmasks[j]);
         // cout << "2: "; printBits(bitmasks[k]);
         unsigned long long combined = bitmasks[j] & bitmasks[k];
         // cout << "3: "; printBits(combined);
         if (combined != 0)
         {
            // If any two modification combinations have the same bit set then this is not a valid combination.
            // This can happen when more than one modification is defined on the same amino acid.
            return false; 
         }
      }
   }

   string modStringNoTerms = getModSeqNoTerms(modString);
   int modStringLenPlusTerms = (int)modStringNoTerms.length() + 2;

   char *mods = new char[modStringLenPlusTerms]; // First two elements in this array will be the n-term and c-term modifications
   for (int i = 0; i < modStringLenPlusTerms; ++i) mods[i] = -1;

   char modNum; 
   int shift = modStringLenPlusTerms - modStringLen;

   for (int i = 0; i < modStringLen; ++i) // i is the index in the sequence
   {
      bool isNterm = (i == 0 && modString[i] == Modifications::NTERM); // Is the N-term set in the sequence
      bool isCterm = ((i == 0 || i == 1) && modString[i] == Modifications::CTERM); // Is the C-term set in the sequence

      int bitPos = modStringLen - i - 1; // bit position in the bitmask that corresponds to i. 
                                         // Example: nMSMMKc; bitmask for modified 'S' is 0010000; i = 2; bitPos = 4 (7 - 2 - 1) 

      for (size_t j = 0; j < modNumCount; ++j)
      {
         unsigned long long btm = bitmasks[j];
         modNum = modNumbers[j];

         // extract the bitPos-th bit in the bitmask
         const unsigned long long b = (btm & 1ULL << bitPos);
         if (b > 0)
         {
            if (isNterm) mods[0] = modNum;
            else if (isCterm) mods[1] = modNum;
             else mods[i + shift] = modNum;
            break; // We found the modification at this index. Go to the next index.
         }
      }
   }
   
   ModificationNumber modification;
   modification.modifications = mods;
   modification.lengthModifications = modStringLenPlusTerms;
   MOD_NUMBERS.push_back(modification);

   MOD_NUM++;

   return true;
}

// Generate all the modification combinations for the given sequence of modifiable amino acids.
void ModificationsPermuter::generateModifications(string* sequence,
                                                  int* ret_modNumStart,
                                                  int* ret_modNumCount,
                                                  Modifications& ALL_MODS,
                                                  int MOD_CNT,
                                                  int ALL_COMBINATION_CNT,
                                                  unsigned long long* ALL_COMBINATIONS)
{
   vector<unsigned long long> modBitmasks; // One entry in the vector for each user specified modification found in the sequence
   vector<int> modIndices; // Indices in the ALL_MODS array of the modifications found in the given sequence
   vector<unsigned long long> combinationCounts; // Number of combinations that would be generated for each modification

   *ret_modNumStart = -1;
   *ret_modNumCount = 0;

   // Step 1: Get a bitmask representing each user specified modification found in the sequence.
   for (int m = 0; m < MOD_CNT; ++m)
   {
      ModificationRule *modRule = ALL_MODS.getModificationRule(m);
      
      const unsigned long long bitmask = modRule->getModBitmask(*sequence); // Example: CMHQQQMK -> 01000010 (for modChars = 'M')
                                                                           // Example: PMHSSMTK -> 00011010 (for modChars = 'STY')
      if (bitmask != 0)
      {      
         unsigned long long combinationCount = modRule->getCombinationCount(bitmask);

         if (combinationCount > FRAGINDEX_MAX_COMBINATIONS)
         {
//          IGNORED_SEQ_CNT++;
            return;
         }

         modIndices.push_back(m);
         modBitmasks.push_back(bitmask);
         combinationCounts.push_back(combinationCount);
      }
   }

   // Step 2: Generate all the bitmask combinations for each modification found in the sequence
   int idx = 0;
   unsigned long long ** combinationsForAllMods = new unsigned long long*[modIndices.size()];

   // Iterate over the bitmasks for the modifications found in the given sequence
   for (vector<unsigned long long>::iterator it = modBitmasks.begin(); it != modBitmasks.end(); ++it)
   {
      unsigned long long modBitmask = *it; // bitmask for a modification
      const unsigned long long combinationsCount = combinationCounts[idx]; // number of possible combinations
      int modIndex = modIndices[idx];
      ModificationRule *modRule = ALL_MODS.getModificationRule(modIndex);

      unsigned long long *combinationsForMod = new unsigned long long[combinationsCount];

      unsigned long long foundCombinationCount = modRule->getCombinations(modBitmask,
            combinationsForMod, ALL_COMBINATIONS, ALL_COMBINATION_CNT);

      // Number of combinations found should be the same as the expected number.
      if (foundCombinationCount != combinationsCount)
      {
         cout << "ERROR: Unexpected combination count; Found combination count " << to_string(foundCombinationCount)
              << "; Expected calculated count is " << to_string(combinationsCount) << endl;
      }
      combinationsForAllMods[idx++] = combinationsForMod;
   }

   if (DEBUG)
   {
      cout << endl;
      for (size_t i = 0; i < modIndices.size(); ++i)
      {
         unsigned long long* bitmasks = combinationsForAllMods[i];
         int combinationCount = combinationCounts[i];
         for (int j = 0; j < combinationCount; ++j)
         {
            unsigned long long bitmask = bitmasks[j];
            // cout << bitmasks.at(i) << " - ";
            std::bitset<64> x(bitmask);
            cout << x << endl;
         }
      }
   }

   // Step 3: We have all the possible combinations for each modification. Combine them to generate modification numbers.
   vector<vector<int>>   combinationSets = getCombinationSets(modIndices.size());
   const long long combinationCount = getTotalCombinationCount(combinationCounts, combinationSets);
   
   if (combinationCount != -1)
   {
      int totalModNumCount = 0;
      const int startModNum = MOD_NUM;

      if (DEBUG)
         cout << "set count " << combinationSets.size() << endl;

      for (vector<vector<int>>::iterator it = combinationSets.begin(); it != combinationSets.end(); ++it)
      {   
         vector<int> set = *it;

         const size_t modNumCount = set.size();
         int* modIndicesToMerge = new int[modNumCount];
         unsigned long long* combinationCountsToMerge = new unsigned long long[modNumCount];
         int s = 0;

         for (vector<int>::iterator set_it = set.begin(); set_it != set.end(); ++set_it)
         {
            const int set_el = *set_it;
            modIndicesToMerge[s] = set_el;
            combinationCountsToMerge[s] = combinationCounts.at(set_el);
            s++;
         }
         
         unsigned int *currIdx = new unsigned int[modNumCount]; // Current index for each modification

         for (size_t i = 0; i < modNumCount; ++i)
            currIdx[i] = 0;

         int modNumCalculated = 0;
         
         while (true)
         {
            unsigned long long* toCombine = new unsigned long long[modNumCount];
            int c = 0;

            for (size_t i = 0; i < modNumCount; ++i)
            {
               const int modIdx = modIndicesToMerge[i];
               unsigned long long* combinationsForModIdx = combinationsForAllMods[modIdx];
               const int y = currIdx[i];
               const unsigned long long combination = combinationsForModIdx[y];
               toCombine[c++] = combination;
            }

            int *modNumbers = new int[modNumCount]; // index of the modification in ALL_MODS

            for (size_t k = 0; k < modNumCount; ++k)
               modNumbers[k] = modIndices.at(modIndicesToMerge[k]);

            if (combine(modNumbers, toCombine, modNumCount, *sequence))
               modNumCalculated++;

            delete[] modNumbers;
            delete[] toCombine;
            
            int next = modNumCount - 1;

            while (next >= 0 && (currIdx[next] == combinationCountsToMerge[next] - 1))
               next--;

            if (next < 0)
               break; // We are done

            currIdx[next]++;  // Go on to the next element in the list

            for (size_t i = next + 1; i < modNumCount; ++i)
               currIdx[i] = 0;
         }
         if (DEBUG)
            cout << "merged" << endl;

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
//    IGNORED_SEQ_CNT++; // Number of possible combinations exceed the cutoff.
   }

   combinationSets.clear();
   for (unsigned int i = 0; i < modIndices.size(); ++i) 
   {
      delete[] combinationsForAllMods[i];
   }
   delete[] combinationsForAllMods;

   modBitmasks.clear();
   modIndices.clear();
   combinationCounts.clear();
}

void ModificationsPermuter::getModificationCombinations(const vector<string> modifiableSeqs,
                                                        Modifications& ALL_MODS,
                                                        int MOD_CNT,
                                                        int ALL_COMBINATION_CNT,
                                                        unsigned long long* ALL_COMBINATIONS)
{
   CombinatoricsUtils::initBinomialCoefficients(g_staticParams.options.peptideLengthRange.iEnd, FRAGINDEX_MAX_MODS_PER_MOD);

   int i = 0;
   for (auto it = begin(modifiableSeqs); it != end(modifiableSeqs); ++it)
   {
      string modSeq = *it;

      if (DEBUG)
         cout << "Calculating modifications for " << to_string(i) << " " << modSeq << endl;

      int modNumStart = -1, modNumCount = 0;

      generateModifications(&modSeq, &modNumStart, &modNumCount, ALL_MODS, MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);

      MOD_SEQ_MOD_NUM_START[i] = modNumStart;
      MOD_SEQ_MOD_NUM_CNT[i] = modNumCount;

      i++;
   }
}
