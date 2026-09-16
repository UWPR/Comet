// Copyright 2012-2026 Jimmy Eng
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

// Sequences skipped because they would generate more than FRAGINDEX_MAX_COMBINATIONS
// combinations (or exceed MAX_BITCOUNT). Defined in CometModificationsPermuter.cpp.
extern int IGNORED_SEQ_CNT;

class ModificationsPermuter
{
public:

   // Terminal positions in a modifiable sequence (docs/20260915_permuter_terminal_mods.md,
   // section 3.2).  When terminal-mod permutation is enabled, every peptide's modifiable
   // sequence is prefixed with two sentinel characters -- [N-sentinel][C-sentinel] -- chosen
   // per peptide from its flanking residues, so the two terminal positions are the two most
   // significant bits of the permuter's bitmask and bytes 0 and 1 of every pool entry
   // (byte 0 = N-term slot, byte 1 = C-term slot, bytes 2.. = residues).  The sentinels are
   // internal, never persisted, and never appear in szVarModChar, so they collide with
   // nothing.  Which sentinel a peptide gets is what makes protein-terminus eligibility part
   // of the modifiable-sequence dedup key: a peptide at the protein N-terminus gets
   // TERM_PROT_N, any other peptide gets TERM_PEP_N.  A mod declared with 'n' (any peptide
   // N-terminus) is translated to match both; one declared with '^' (protein N-terminus only)
   // matches only TERM_PROT_N.  See TranslateModCharsForPermuter().
   static const char TERM_PEP_N  = '<';   // peptide N-terminus, not at the protein N-terminus
   static const char TERM_PROT_N = '{';   // protein N-terminus (cPrevAA == '-')
   static const char TERM_PEP_C  = '>';   // peptide C-terminus, not at the protein C-terminus
   static const char TERM_PROT_C = '}';   // protein C-terminus (cNextAA == '-')
   static const int  TERM_SLOT_BYTES = 2; // sentinel positions per modifiable sequence when enabled

   // Map a variable mod's szVarModChar to the character set the permuter matches against a
   // modifiable sequence: residues pass through; 'n' -> TERM_PEP_N + TERM_PROT_N,
   // '^' -> TERM_PROT_N, 'c' -> TERM_PEP_C + TERM_PROT_C, '$' -> TERM_PROT_C.
   static string TranslateModCharsForPermuter(const char* szVarModChar);

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
   // Fill the modifiable-sequence pool for every raw peptide.  With bIncludeTermini the
   // sequence is prefixed with the two terminal sentinels (see above) and is therefore
   // never empty; without it the behavior is the historical residue-only one and
   // PEPTIDE_MOD_SEQ_IDXS is -1 for peptides with no modifiable residue.
   static void getModifiableSequences(const RawPeptideTable& vRawPeptides,
                                      int* PEPTIDE_MOD_SEQ_IDXS,
                                      vector<string>& ALL_MODS,
                                      bool bIncludeTermini);
   static unsigned long long getModBitmask(const char* modSeq,
                                           int iLen,
                                           const string& sModChars);
   static vector<vector<int>> getCombinationSets(int modCount);
   static int getTotalCombinationCount(vector<int> combinationCounts,
                                       vector<vector<int>> combinationSets);
   static bool combine(int* modNumbers,
                       unsigned long long* bitmasks,
                       int modNumCount,
                       int modStringLen);
   static void generateModifications(const char* sequence,
                                     int iSeqLen,
                                     vector<int>& vMaxNumVarModsPerMod,
                                     int* ret_modNumStart,
                                     int* ret_modNumCount,
                                     vector<string>& ALL_MODS,
                                     int MOD_CNT,
                                     int ALL_COMBINATION_CNT,
                                     unsigned long long* ALL_COMBINATIONS);
   static void getModificationCombinations(vector<int>& vMaxNumVarModsPerMod,
                                           vector<string>& ALL_MODS,
                                           int MOD_CNT,
                                           int ALL_COMBINATION_CNT,
                                           unsigned long long* ALL_COMBINATIONS);
   static bool ignorePeptidesWithTooManyMods(void);

   ModificationsPermuter();
   ~ModificationsPermuter();
};

#endif // _COMETMODIFICATIONSPERMUTER_H_
