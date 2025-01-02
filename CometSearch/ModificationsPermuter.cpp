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


#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <chrono>
#include <unordered_set>
#include "Common.h"
#include "CometDataInternal.h"
#include "ModificationsPermuter.h"
#include "CombinatoricsUtils.h"
#include "CometFragmentIndex.h"

//using namespace std;

ModificationsPermuter::ModificationsPermuter()
{
}
ModificationsPermuter::~ModificationsPermuter()
{
}


//bool DEBUG = false;

// Maximum number of bits that can be set in a modifiable sequence for a given modification.
// C(25, 5) = 53,130; C(25, 4) = 10,650; C(25, 3) = 2300.  This is more than FRAGINDEX_MAX_COMBINATIONS (65,534)
unsigned int MAX_BITCOUNT = 24;
int MAX_K_VAL = 10;

int IGNORED_SEQ_CNT = 0; // Sequences that were ignored because they would generate more than FRAGINDEX_MAX_COMBINATIONS combinations.

long TIME_IN_COMBINE = 0;
long TIME_GEN_MODS = 0;

// https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
chrono::time_point<chrono::steady_clock> ModificationsPermuter::startTime()
{
   // Changed from high_resolution_clock::now to steady_clock::now based on this SO thread
   // https://stackoverflow.com/questions/70396570/mismatched-types-stdchrono-v2steady-clock-and-stdchrono-v2system
   return chrono::steady_clock::now();
}


long ModificationsPermuter::duration(chrono::time_point<chrono::steady_clock> start)
{
   const auto stop = chrono::steady_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
   return static_cast<long>(duration.count());
}


bool ModificationsPermuter::ignorePeptidesWithTooManyMods()
{
   return (FRAGINDEX_KEEP_ALL_PEPTIDES == 1);
}


bool ModificationsPermuter::isModifiable(char aa,
                                         vector<string>& ALL_MODS)
{
   for (auto it = ALL_MODS.begin(); it != ALL_MODS.end(); ++it)
   {
      if ( (*it).find(aa) != std::string::npos )
         return true;
   }

   return false;
}


void ModificationsPermuter::printBits(unsigned long long number)
{
   std::bitset<64> x(number);
   cout << x << endl;
}


// Generate all the n-choose-k bitmask combinations for sequences of length 'n' with 'k' modified residues
void ModificationsPermuter::getCombinations(int n,
                                            int k,
                                            int nck,
                                            unsigned long long* bitmasks)
{
   int** combinations = CombinatoricsUtils::makeCombinations(n, k, nck);

   for (int i = 0; i < nck; ++i)
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

      // std::bitset<64> x(bitmasks.at(i));
      // cout << x << " - "  << allCombos[i] << endl;
   }
   for (int i = 0; i < nck; ++i)
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
   int totalCount = 0;
   int i = maxMods;

   unsigned long long* allCombos = 0;  // = new unsigned long long[0]; // FIX: confirm no need for this init allocation
   int currentAllCount = 0;
   while (i >= 1)
   {
      int nck = CombinatoricsUtils::nChooseK(maxPeptideLen, i);
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

         delete[] combos;  //FIX: confirm this is correct delete to avoid memory leak
      }
      currentAllCount = totalCount;
      i--;
   }

   std::cout << ", total combinations " << totalCount << ")" << endl;

/*
   if (DEBUG)
   {
      for (int i = 0; i < totalCount; ++i)
      {
         std::bitset<64> x(allCombos[i]);
         cout << allCombos[i] << " - " << x << endl;
      }
   }
*/

//   endTime(start, "Combinations initialized");
   
   *ALL_COMBINATIONS =  allCombos;
   *ALL_COMBINATION_CNT = totalCount;
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
      }
      myfile.close();

      cout << "Read " << std::to_string(peptides.size()) << " peptides" << CometFragmentIndex::ElapsedTime(start);
   }
   else cout << "ERROR: Unable to open file" << endl;

   return peptides;
}


// Return a sequence comprising the amino acids in the given peptide that can have a modification. 
string ModificationsPermuter::getModifiableAas(std::string peptide,
                                               vector<string>& ALL_MODS)
{
   string modifiableAas = "";

   int i = 0;

   for (char aa : peptide)
   {
      if (isModifiable(aa, ALL_MODS))
      {
         modifiableAas += aa;
      }
      i++;
   }

   if (modifiableAas.size() == 0)
      return {};

   return modifiableAas;
}


vector<string> ModificationsPermuter::getModifiableSequences(vector<PlainPeptideIndex>& vRawPeptides,
                                                             int* PEPTIDE_MOD_SEQ_IDXS,
                                                             vector<string>& ALL_MODS)
{
   std::unordered_map<string, int> modifiableSeqMap;
   vector<string> ret;
   int pepIdx = 0;
   int modSeqIdx = 0;
   int modifiablePeptides = 0;

   for (auto it = vRawPeptides.begin(); it != vRawPeptides.end(); ++it)
   {
      //FIX: put restriction here for protein mod filter
      string modifiableAas = getModifiableAas((*it).sPeptide, ALL_MODS);

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
      else if (g_staticParams.variableModParameters.bVarTermModSearch)
      {
         PEPTIDE_MOD_SEQ_IDXS[pepIdx] = -2;
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


// Iterate over the modSeq and set the bit to 1 if the amino acid at an index matches the given modChar
// Example: CMHQQQMK -> 01000010 (for modChar = 'M')
unsigned long long ModificationsPermuter::getModBitmask(string* modSeq,
                                                        string sModChars)
{
   uint64_t bitMask = 0ULL;
   long len = (long)(*modSeq).size();
   for (int i = 0; i < len; ++i)
   {
      if (sModChars.find((*modSeq)[i]) != string::npos)
      {
         bitMask |= (static_cast <uint64_t> (1ULL) << (len - i - 1));
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
      for (int j = 0; j < nck; ++j)
      {
         int *combination = combinationSets[j];
         vector<int> set;
         for (int k = 0; k < i; ++k)
         {
            set.push_back(combination[k]);
         }
         allSets.push_back(set);
      }

      for (int j = 0; j < nck; ++j)
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
      for (unsigned int j = 0; j < set.size(); ++j)
      {
         int s = set.at(j);
         combos *= combinationCounts.at(s);
         if (ignorePeptidesWithTooManyMods() && combos > FRAGINDEX_MAX_COMBINATIONS)
         {
            return -1;
         }
      }
      allCombos += combos;
   }
   return ignorePeptidesWithTooManyMods() && allCombos > FRAGINDEX_MAX_COMBINATIONS ? -1 : allCombos; 
}


bool ModificationsPermuter::combine(int* modNumbers,
                                    unsigned long long* bitmasks,
                                    int modNumCount,
                                    int modStringLen)
{
// const auto start = startTime();

   unsigned long long combinedBitmasks = 0;  // use this combined bitmask to check total # of mods

   for (int j = 0; j < modNumCount; ++j)
   {
      for (int k = j+1; k < modNumCount; ++k)
      {
         // cout << "1: "; printBits(bitmasks[j]);
         // cout << "2: "; printBits(bitmasks[k]);
         unsigned long long combined = bitmasks[j] & bitmasks[k];
         combinedBitmasks = bitmasks[j] | bitmasks[k];
         // cout << "3: "; printBits(combined);
         if (combined != 0)
         {
            // If any two modification combinations have the same bit set then this is not a valid combination.
            // This can happen when more than one modification is defined on the same amino acid.
            return false;
         }
      }
   }

   // now check if number of mods after combining is greater than total allowed in peptide
   std::bitset<64> x(combinedBitmasks);
   unsigned long long bitCount = x.count();
   if (bitCount > (unsigned long long)g_staticParams.variableModParameters.iMaxVarModPerPeptide)
      return false;

   char *mods = new char[modStringLen];
   char modNum;

   for (int i = 0; i < modStringLen; ++i)
      mods[i] = -1;

   for (int i = 0; i < modStringLen; ++i)
   {
      int idx = modStringLen - i - 1;
      for (int j = 0; j < modNumCount; ++j)
      {
         unsigned long long btm = bitmasks[j];
         modNum = modNumbers[j];

         // extract the i-th bit in the bitmask
         const unsigned long long b = (btm & 1ULL << i);
         if (b > 0)
         {
            mods[idx] = modNum;
            break; // We found the modification at this index. Go to the next index.
         }
      }
   }

   ModificationNumber modification;
   MOD_NUM++;  //FIX:  confirm this is not needed either
   modification.modifications = mods;
   modification.modStringLen = modStringLen;

   MOD_NUMBERS.push_back(modification);

// TIME_IN_COMBINE += duration(start);
   return true;
}


// Generate all the modification combinations for the given sequence of modifiable amino acids.
void ModificationsPermuter::generateModifications(string* sequence,
                                                  vector<int>& vMaxNumVarModsPerMod,
                                                  int* ret_modNumStart,
                                                  int* ret_modNumCount,
                                                  vector<string>& ALL_MODS,
                                                  int MOD_CNT,
                                                  int ALL_COMBINATION_CNT,
                                                  unsigned long long* ALL_COMBINATIONS)
{
   vector<unsigned long long> modBitmasks; // One entry in the vector for each user specified modification found in the sequence
   vector<int> modIndices; // Indices in the ALL_MODS array of the modifications found in the given sequence
   vector<int> combinationCounts; // Number of combinations that would be generated for each modification

   *ret_modNumStart = -1;
   *ret_modNumCount = 0;

   vector<unsigned int> vMaxMods;  // store value from vMaxNumVarModsPerMod for each modBitmasks

   // Step 1: Get a bitmask representing each user specified modification found in the sequence.
   for (int m = 0; m < MOD_CNT; ++m)
   {
      //FIX: apply protein modifications filter here??
      string sModChars = ALL_MODS[m];

      const unsigned long long bitmask = getModBitmask(sequence, sModChars); // Example: CMHQQQMK -> 01000010 (for modChar = 'M')

      if (bitmask != 0)
      {
         std::bitset<64> x(bitmask);
         unsigned long long bitCount = x.count();

         if (ignorePeptidesWithTooManyMods() && bitCount > MAX_BITCOUNT)
         {
            IGNORED_SEQ_CNT++;
            return; 
         }

         int combinationCount = CombinatoricsUtils::getCombinationCount(int(bitCount), vMaxNumVarModsPerMod[m]); // nCk + nCk-1 +...+nC1

         if (ignorePeptidesWithTooManyMods() && combinationCount > FRAGINDEX_MAX_COMBINATIONS)
         {
            IGNORED_SEQ_CNT++;
            return;
         }

         modIndices.push_back(m);

         modBitmasks.push_back(bitmask);
         vMaxMods.push_back(vMaxNumVarModsPerMod[m]);
         combinationCounts.push_back(combinationCount);
      }
   }

   // Step 2: Generate all the bitmask combinations for each modification found in the sequence
   int idx = 0;
   unsigned long long ** combinationsForAllMods = new unsigned long long*[modIndices.size()];
   // Iterate over the bitmasks for the modifications found in the given sequence
   for (auto it = modBitmasks.begin(); it != modBitmasks.end(); ++it)
   {
      unsigned long long modBitmask = *it; // bitmask for a modification

      const int calculatedCombinationsCount = combinationCounts[idx]; // number of calculated possible combinations

      if (ignorePeptidesWithTooManyMods() && calculatedCombinationsCount > FRAGINDEX_MAX_COMBINATIONS)
      {
         // If we are ignoring peptides with > FRAGINDEX_MAX_COMBINATIONS or reducing FRAGINDEX_MAX_MODS_PER_MOD to get the number of
         // modified peptides withing the threshold, then the calculated combinations should not exceed FRAGINDEX_MAX_COMBINATIONS
         cout << "ERROR: calculated combination count exceeds FRAGINDEX_MAX_COMBINATIONS (" << to_string(FRAGINDEX_MAX_COMBINATIONS) +
            ") but FRAGINDEX_KEEP_ALL_PEPTIDES is set to " << to_string(FRAGINDEX_KEEP_ALL_PEPTIDES);
         // TODO: exit here?
      }

      // Some peptides will have > MAX_COMBINATION modification combinations if we are
      // considering all peptides. In this case, keep only up to FRAGINDEX_MAX_COMBINATIONS
      int combinationsForModArrLen = FRAGINDEX_KEEP_ALL_PEPTIDES && calculatedCombinationsCount > FRAGINDEX_MAX_COMBINATIONS ? FRAGINDEX_MAX_COMBINATIONS : calculatedCombinationsCount;
      unsigned long long *combinationsForMod = new unsigned long long[combinationsForModArrLen];

      unsigned long long notMod = ~modBitmask;
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
      int combinationsFound = 0;

      for (int j = 0; j < ALL_COMBINATION_CNT; ++j)
      {
         if (combinationsFound >= FRAGINDEX_MAX_COMBINATIONS)
            break;

         const unsigned long long combination = ALL_COMBINATIONS[j];

         if (combination > modBitmask)
            break;

         std::bitset<64> x(combination);
         unsigned long long bitCount = x.count();

         if ((combination & notMod) == 0 && bitCount <= vMaxMods.at(idx)) // Only the bits set in the modification bitmask should be set in the combination.
            combinationsForMod[combinationsFound++] = combination;
      }
      if (combinationsFound != combinationsForModArrLen) // Number of combinations found should be the same as the expected number.
      {
         cout << "ERROR: Unexpected combination count; Found combination count " << to_string(combinationsFound) 
              << "; Expected calculated count is " << to_string(combinationsForModArrLen) << "; sequence " << *sequence << endl;
      }
      combinationsForAllMods[idx++] = combinationsForMod;
   }

/*
   if (DEBUG)
   {
      for (auto i = 0; i < (int)modBitmasks.size(); ++i)
      {
         unsigned long long* bitmasks = combinationsForAllMods[i];
         int combinationCount = combinationCounts[i];
         for (int j = 0; j < combinationCount; ++j)
         {
            unsigned long long bitmask = bitmasks[j];
            // cout << bitmasks.at(i) << " - ";
            std::bitset<64> x(bitmask);
            cout << "OK " << i << " " << x << endl;
         }
      }
   }
*/

   // Step 3: We have all the possible combinations for each modification. Combine them to generate modification numbers.

   vector<vector<int>> combinationSets = getCombinationSets((int)modIndices.size());

   const int combinationCount = getTotalCombinationCount(combinationCounts, combinationSets);
   
   if (combinationCount != -1)
   {
      int totalModNumCount = 0;
      const int startModNum = MOD_NUM;

      for (vector<vector<int>>::iterator it = combinationSets.begin(); it != combinationSets.end(); ++it)
      {
         vector<int> set = *it;

         const int modNumCount = (int)set.size();
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
         for (int i = 0; i < modNumCount; ++i)
            currIdx[i] = 0;

         int modNumCalculated = 0;
         
         while (true)
         {
            unsigned long long* toCombine = new unsigned long long[modNumCount];

            int c = 0;
            for (int i = 0; i < modNumCount; ++i)
            {
               const int modIdx = modIndicesToMerge[i];
               unsigned long long* combinationsForModIdx = combinationsForAllMods[modIdx];
               const int y = currIdx[i];
               const unsigned long long combination = combinationsForModIdx[y];
               toCombine[c++] = combination;
            }

            int *modNumbers = new int[modNumCount]; // index of the modification in ALL_MODS
            for (int k = 0; k < modNumCount; ++k)
            {
               modNumbers[k] = modIndices.at(modIndicesToMerge[k]);
            }

            if (combine(modNumbers, toCombine, modNumCount, (int)(*sequence).length()))
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
            if (next < 0)
               break; // We are done

            currIdx[next]++;  // Go on to the next element in the list

            for (int i = next + 1; i < modNumCount; ++i)
            {
               currIdx[i] = 0;
            }
            if (FRAGINDEX_KEEP_ALL_PEPTIDES && totalModNumCount + modNumCalculated >= FRAGINDEX_MAX_COMBINATIONS)
            {
               break; // Don't keep more than FRAGINDEX_MAX_COMBINATIONS modifications
            }
         }

//       if (DEBUG)
//          cout << "merged" << endl;

         totalModNumCount += modNumCalculated;

         delete[] currIdx;
         delete[] modIndicesToMerge;
         delete[] combinationCountsToMerge;

         if (FRAGINDEX_KEEP_ALL_PEPTIDES && totalModNumCount >= FRAGINDEX_MAX_COMBINATIONS)
            break; // Don't keep more than FRAGINDEX_MAX_COMBINATIONS modifications
      }

      if (MOD_NUM != startModNum + totalModNumCount)
      {
         cout << "Error: Unexpected end index: " + std::to_string(MOD_NUM) << "; Expected: " << std::to_string(startModNum + totalModNumCount) << endl;
      }

      *ret_modNumStart = startModNum;
      *ret_modNumCount = totalModNumCount;

   }
   else
   {
      IGNORED_SEQ_CNT++; // Number of possible combinations exceed the cutoff.
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

// TIME_GEN_MODS += duration(startGenMods);
}


void ModificationsPermuter::getModificationCombinations(const vector<string> modifiableSeqs,
                                                        vector<int>& vMaxNumVarModsPerMod,
                                                        vector<string>& ALL_MODS,
                                                        int MOD_CNT,
                                                        int ALL_COMBINATION_CNT,
                                                        unsigned long long* ALL_COMBINATIONS)
{
   MOD_SEQ_MOD_NUM_START = new int[modifiableSeqs.size()];
   MOD_SEQ_MOD_NUM_CNT = new int[modifiableSeqs.size()];

   CombinatoricsUtils::initBinomialCoefficients(g_staticParams.options.peptideLengthRange.iEnd, MAX_K_VAL);

   int i = 0;
   for (auto it = modifiableSeqs.begin(); it != modifiableSeqs.end(); ++it)
   {
      string modSeq = *it;

      int modNumStart = -1;
      int modNumCount = 0;

      generateModifications(&modSeq, vMaxNumVarModsPerMod, &modNumStart, &modNumCount, ALL_MODS, MOD_CNT, ALL_COMBINATION_CNT, ALL_COMBINATIONS);

      MOD_SEQ_MOD_NUM_START[i] = modNumStart;
      MOD_SEQ_MOD_NUM_CNT[i] = modNumCount;

      i++;
   }
}
