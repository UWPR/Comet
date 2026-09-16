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

// Unit tests for ModificationsPermuter (CometSearch/CometModificationsPermuter.cpp):
// the first direct coverage of the permuter, added with the terminal-mod work in
// docs/20260915_permuter_terminal_mods.md (Phase 1, tests P1-P13).
//
// Framework: MiniTest.h (see TestCometSearchAndPreprocess.cpp, which also holds
// MINITEST_MAIN()). Compiled into the same CometUnitTests binary.
//
// What is under test: given raw peptides (sequence + flanking residues) and variable-mod
// residue strings, the permuter must (a) build the right modifiable sequence per peptide
// -- with the two terminal sentinel positions prefixed when terminal-mod permutation is
// enabled, chosen from the protein context -- and (b) emit exactly the legal permutation
// entries, in its documented order, honoring the per-peptide and per-mod caps. Entries
// are read back from the flat pool exactly as CometFragmentIndex / CometPeptideIndex do.

#include "MiniTest.h"
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include "../../CometSearch/Common.h"
#include "../../CometSearch/CometDataInternal.h"
#include "../../CometSearch/CometModificationsPermuter.h"
#include "../../CometSearch/CombinatoricsUtils.h"

namespace
{
   typedef std::vector<int> Entry;              // one pool entry, one int per position (-1 = unmodified)
   typedef std::vector<Entry> EntryList;

   struct TestPeptide
   {
      std::string sSeq;
      char cPrevAA;
      char cNextAA;
   };

   // Release the permuter's raw-new[] tables and clear its pools so each test starts clean.
   void ResetPermuterGlobals()
   {
      MOD_NUMBERS_POOL.clear();
      MOD_SEQS_POOL.clear();
      MOD_SEQS_OFFSET.clear();
      delete[] MOD_SEQ_MOD_NUM_START;      MOD_SEQ_MOD_NUM_START = nullptr;
      delete[] MOD_SEQ_MOD_NUM_CNT;        MOD_SEQ_MOD_NUM_CNT = nullptr;
      delete[] MOD_SEQ_MOD_NUM_POOL_START; MOD_SEQ_MOD_NUM_POOL_START = nullptr;
      delete[] PEPTIDE_MOD_SEQ_IDXS;       PEPTIDE_MOD_SEQ_IDXS = nullptr;
      MOD_NUM = 0;
   }

   // Drive the permuter the way CometFragmentIndex::PermuteIndexPeptideMods() does.
   // vMods are raw szVarModChar strings ('n', 'c', '^', '$' allowed); the compacted mod
   // index the entries carry is the position in vMods.
   void RunPermuter(const std::vector<TestPeptide>& vPeptides,
                    const std::vector<std::string>& vMods,
                    const std::vector<int>& vMaxPerMod,
                    int iMaxVarModPerPeptide,
                    bool bIncludeTermini,
                    int iMaxSeqLen)
   {
      ResetPermuterGlobals();

      g_staticParams.variableModParameters.iMaxVarModPerPeptide = iMaxVarModPerPeptide;
      g_staticParams.variableModParameters.bVarTermModSearch = bIncludeTermini;
      g_staticParams.options.peptideLengthRange.iEnd = MAX_PEPTIDE_LEN - 1;

      RawPeptideTable table;
      for (const TestPeptide& p : vPeptides)
         table.push_back(p.sSeq.c_str(), (int)p.sSeq.size(), p.cPrevAA, p.cNextAA, 1000.0, 0, 0);

      std::vector<std::string> ALL_MODS;
      for (const std::string& s : vMods)
         ALL_MODS.push_back(ModificationsPermuter::TranslateModCharsForPermuter(s.c_str()));
      std::vector<int> vMax(vMaxPerMod);

      int iMaxMods = 0;
      for (int m : vMax)
         iMaxMods = (std::max)(iMaxMods, m);
      iMaxMods = (std::min)(iMaxMods, iMaxVarModPerPeptide);
      if (iMaxMods < 1)
         iMaxMods = 1;

      unsigned long long* ALL_COMBINATIONS = nullptr;
      int ALL_COMBINATION_CNT = 0;
      ModificationsPermuter::initCombinations(iMaxSeqLen, iMaxMods, &ALL_COMBINATIONS, &ALL_COMBINATION_CNT);

      PEPTIDE_MOD_SEQ_IDXS = new int[table.size()];
      ModificationsPermuter::getModifiableSequences(table, PEPTIDE_MOD_SEQ_IDXS, ALL_MODS, bIncludeTermini);
      ModificationsPermuter::getModificationCombinations(vMax, ALL_MODS, (int)ALL_MODS.size(),
         ALL_COMBINATION_CNT, ALL_COMBINATIONS);

      delete[] ALL_COMBINATIONS;
   }

   std::string ModSeqOf(int iPep)
   {
      int idx = PEPTIDE_MOD_SEQ_IDXS[iPep];
      if (idx < 0)
         return "";
      int iLen;
      const char* p = GetModSeq(idx, iLen);
      return std::string(p, (size_t)iLen);
   }

   EntryList EntriesOf(int iPep)
   {
      EntryList out;
      int idx = PEPTIDE_MOD_SEQ_IDXS[iPep];
      if (idx < 0 || MOD_SEQ_MOD_NUM_START[idx] < 0)
         return out;
      int iLen;
      GetModSeq(idx, iLen);
      for (int k = 0; k < MOD_SEQ_MOD_NUM_CNT[idx]; ++k)
      {
         const char* e = GetModNumEntry(MOD_SEQ_MOD_NUM_START[idx] + k, idx, iLen);
         Entry entry;
         for (int i = 0; i < iLen; ++i)
            entry.push_back((int)(signed char)e[i]);
         out.push_back(entry);
      }
      return out;
   }

   std::set<Entry> AsSet(const EntryList& l) { return std::set<Entry>(l.begin(), l.end()); }

   int CountModified(const Entry& e)
   {
      int n = 0;
      for (int v : e)
         if (v >= 0)
            ++n;
      return n;
   }

   struct PermuterTest : public minitest::Test
   {
      void SetUp() { ResetPermuterGlobals(); }
      void TearDown() { ResetPermuterGlobals(); }
   };

   const char N_PEP = ModificationsPermuter::TERM_PEP_N;
   const char N_PROT = ModificationsPermuter::TERM_PROT_N;
   const char C_PEP = ModificationsPermuter::TERM_PEP_C;
   const char C_PROT = ModificationsPermuter::TERM_PROT_C;

   std::string WithTerm(char a, char b, const std::string& rest) { std::string s; s += a; s += b; return s + rest; }
}

// P1: the four protein-context classes yield four distinct modifiable sequences that differ
// only in their sentinel prefix.
TEST_F(PermuterTest, P1_ModSeq_SentinelContexts)
{
   std::vector<TestPeptide> peps = { {"ACMK",'K','S'}, {"ACMK",'-','S'}, {"ACMK",'K','-'}, {"ACMK",'-','-'} };
   RunPermuter(peps, {"M"}, {1}, 3, true, 8);

   EXPECT_EQ(4, GetNumModSeqs());
   EXPECT_EQ(WithTerm(N_PEP,  C_PEP,  "M"), ModSeqOf(0));
   EXPECT_EQ(WithTerm(N_PROT, C_PEP,  "M"), ModSeqOf(1));
   EXPECT_EQ(WithTerm(N_PEP,  C_PROT, "M"), ModSeqOf(2));
   EXPECT_EQ(WithTerm(N_PROT, C_PROT, "M"), ModSeqOf(3));
}

// P2: with terminal permutation disabled the historical residue-only behavior holds:
// one shared sequence, no sentinels, -1 for a peptide with no modifiable residue.
TEST_F(PermuterTest, P2_ResidueOnly_NoSentinels)
{
   std::vector<TestPeptide> peps = { {"ACMK",'K','S'}, {"ACMK",'-','S'}, {"ACMK",'K','-'}, {"AAAK",'-','-'} };
   RunPermuter(peps, {"M"}, {1}, 3, false, 8);

   EXPECT_EQ(1, GetNumModSeqs());
   EXPECT_EQ(std::string("M"), ModSeqOf(0));
   EXPECT_EQ(0, PEPTIDE_MOD_SEQ_IDXS[0]);
   EXPECT_EQ(0, PEPTIDE_MOD_SEQ_IDXS[1]);
   EXPECT_EQ(0, PEPTIDE_MOD_SEQ_IDXS[2]);
   EXPECT_EQ(-1, PEPTIDE_MOD_SEQ_IDXS[3]);

   EntryList e = EntriesOf(0);
   EXPECT_EQ((size_t)1, e.size());
   EXPECT_EQ(Entry({0}), e[0]);
}

// P3: szVarModChar -> permuter character set.
TEST_F(PermuterTest, P3_TranslateModChars)
{
   auto has = [](const std::string& s, char c) { return s.find(c) != std::string::npos; };

   std::string n = ModificationsPermuter::TranslateModCharsForPermuter("n");
   EXPECT_EQ((size_t)2, n.size()); EXPECT_TRUE(has(n, N_PEP)); EXPECT_TRUE(has(n, N_PROT));

   std::string pn = ModificationsPermuter::TranslateModCharsForPermuter("^");
   EXPECT_EQ(std::string(1, N_PROT), pn);

   std::string c = ModificationsPermuter::TranslateModCharsForPermuter("c");
   EXPECT_EQ((size_t)2, c.size()); EXPECT_TRUE(has(c, C_PEP)); EXPECT_TRUE(has(c, C_PROT));

   std::string pc = ModificationsPermuter::TranslateModCharsForPermuter("$");
   EXPECT_EQ(std::string(1, C_PROT), pc);

   // 'n^' is just 'n'
   EXPECT_EQ(n, ModificationsPermuter::TranslateModCharsForPermuter("n^"));

   // residues pass through, and a dual residue+terminus mod keeps both
   std::string nk = ModificationsPermuter::TranslateModCharsForPermuter("nK");
   EXPECT_EQ((size_t)3, nk.size()); EXPECT_TRUE(has(nk, 'K')); EXPECT_TRUE(has(nk, N_PEP)); EXPECT_TRUE(has(nk, N_PROT));
   EXPECT_EQ(std::string("STY"), ModificationsPermuter::TranslateModCharsForPermuter("STY"));
}

// P4: terminal and residue mods combine; layout is [N][C][residues...].
TEST_F(PermuterTest, P4_Entries_TerminalAndResidue)
{
   RunPermuter({ {"ACMK",'K','S'} }, {"M", "n"}, {1, 1}, 2, true, 8);

   EXPECT_EQ(WithTerm(N_PEP, C_PEP, "M"), ModSeqOf(0));
   std::set<Entry> got = AsSet(EntriesOf(0));
   std::set<Entry> want = { Entry({-1,-1,0}), Entry({1,-1,-1}), Entry({1,-1,0}) };
   EXPECT_EQ((size_t)3, got.size());
   EXPECT_TRUE(got == want);
}

// P5: a terminal mod counts toward max_variable_mods_in_peptide (D2).
TEST_F(PermuterTest, P5_TerminalCountsTowardCap)
{
   RunPermuter({ {"ACMK",'K','S'} }, {"M", "n"}, {1, 1}, 1, true, 8);

   std::set<Entry> got = AsSet(EntriesOf(0));
   std::set<Entry> want = { Entry({-1,-1,0}), Entry({1,-1,-1}) };
   EXPECT_EQ((size_t)2, got.size());
   EXPECT_TRUE(got == want);
}

// P6: a protein-N-term-only mod ('^') applies only to a protein-N-terminal peptide, and two
// N-term mods never occupy the terminus together.
TEST_F(PermuterTest, P6_ProteinNtermOnly)
{
   // internal peptide: only the 'n' mod (index 0) is legal
   RunPermuter({ {"ACAK",'K','S'} }, {"n", "^"}, {1, 1}, 3, true, 8);
   {
      std::set<Entry> got = AsSet(EntriesOf(0));
      std::set<Entry> want = { Entry({0,-1}) };
      EXPECT_TRUE(got == want);
   }

   // protein-N-terminal peptide: both are legal, one at a time
   RunPermuter({ {"ACAK",'-','S'} }, {"n", "^"}, {1, 1}, 3, true, 8);
   {
      std::set<Entry> got = AsSet(EntriesOf(0));
      std::set<Entry> want = { Entry({0,-1}), Entry({1,-1}) };
      EXPECT_TRUE(got == want);
   }
}

// P7: a single-peptide protein is both termini; '^' and '$' can co-occur and count as two.
TEST_F(PermuterTest, P7_BothProteinTermini)
{
   RunPermuter({ {"ACAK",'-','-'} }, {"^", "$"}, {1, 1}, 2, true, 8);
   {
      EXPECT_EQ(WithTerm(N_PROT, C_PROT, ""), ModSeqOf(0));
      std::set<Entry> got = AsSet(EntriesOf(0));
      std::set<Entry> want = { Entry({0,-1}), Entry({-1,1}), Entry({0,1}) };
      EXPECT_TRUE(got == want);
   }

   RunPermuter({ {"ACAK",'-','-'} }, {"^", "$"}, {1, 1}, 1, true, 8);
   {
      std::set<Entry> got = AsSet(EntriesOf(0));
      std::set<Entry> want = { Entry({0,-1}), Entry({-1,1}) };
      EXPECT_TRUE(got == want);
   }

   // and an internal peptide gets neither
   RunPermuter({ {"ACAK",'R','S'} }, {"^", "$"}, {1, 1}, 2, true, 8);
   EXPECT_EQ((size_t)0, EntriesOf(0).size());
}

// P8: a dual residue+terminus mod ('nK', max 1 per peptide for this mod) places its one
// occurrence on the terminus or on one lysine, never two of them.
TEST_F(PermuterTest, P8_PerModCapCoversTerminus)
{
   RunPermuter({ {"AKK",'R','S'} }, {"nK"}, {1}, 3, true, 8);

   EXPECT_EQ(WithTerm(N_PEP, C_PEP, "KK"), ModSeqOf(0));
   EntryList e = EntriesOf(0);
   std::set<Entry> want = { Entry({0,-1,-1,-1}), Entry({-1,-1,0,-1}), Entry({-1,-1,-1,0}) };
   EXPECT_TRUE(AsSet(e) == want);
   for (const Entry& x : e)
      EXPECT_EQ(1, CountModified(x));
}

// P9: guards -- a mod with per-mod max 0 contributes nothing (and does not crash), and a
// sequence whose combination count exceeds FRAGINDEX_MAX_COMBINATIONS is skipped and
// counted in IGNORED_SEQ_CNT.
// P13: the per-occurrence context rule shared by the build-time check and the output filters
// (ProteinsListCSR::flagsSatisfy()/hasContext()). A protein holding a peptide once at each
// terminus ORs to N|C but must NOT satisfy a '^'+'$' variant; only a whole-protein occurrence
// (PROT_BOTH_TERM_HERE) does.
TEST_F(PermuterTest, P13_ContextFlags_BothTerminiNeedOneOccurrence)
{
   const unsigned char N = ProteinsListCSR::PROT_NTERM_HERE;
   const unsigned char C = ProteinsListCSR::PROT_CTERM_HERE;
   const unsigned char B = ProteinsListCSR::PROT_BOTH_TERM_HERE;

   EXPECT_EQ((unsigned char)(N | C | B), PepOccurrenceContext('-', '-'));
   EXPECT_EQ(N, PepOccurrenceContext('-', 'K'));
   EXPECT_EQ(C, PepOccurrenceContext('R', '-'));
   EXPECT_EQ((unsigned char)0, PepOccurrenceContext('R', 'K'));

   EXPECT_TRUE(ProteinsListCSR::flagsSatisfy(N, N));
   EXPECT_TRUE(ProteinsListCSR::flagsSatisfy((unsigned char)(N | C), N));
   EXPECT_TRUE(ProteinsListCSR::flagsSatisfy((unsigned char)(N | C), C));
   EXPECT_FALSE(ProteinsListCSR::flagsSatisfy((unsigned char)(N | C), (unsigned char)(N | C)));   // two copies, one per terminus
   EXPECT_TRUE(ProteinsListCSR::flagsSatisfy((unsigned char)(N | C | B), (unsigned char)(N | C)));  // whole-protein occurrence
   EXPECT_FALSE(ProteinsListCSR::flagsSatisfy(C, N));
   EXPECT_TRUE(ProteinsListCSR::flagsSatisfy((unsigned char)0, (unsigned char)0));

   // Row-level: [protein A: N|C via two copies][protein B: N only] -> no '^'+'$' support;
   // add a whole-protein occurrence and it passes.
   ProteinsListCSR list;
   std::vector<unsigned int>  flat  = { 10u, 20u, 30u };
   std::vector<uint32_t>      cnt   = { 2u, 1u };
   std::vector<unsigned char> flags = { (unsigned char)(N | C), N, (unsigned char)(N | C | B) };
   EXPECT_TRUE(list.append_flat(flat, cnt, flags));
   EXPECT_TRUE(list.at(0).hasContext(N));
   EXPECT_TRUE(list.at(0).hasContext(C));
   EXPECT_FALSE(list.at(0).hasContext((unsigned char)(N | C)));
   EXPECT_TRUE(list.at(1).hasContext((unsigned char)(N | C)));
}

TEST_F(PermuterTest, P9_ZeroCombinationAndOverflowGuards)
{
   RunPermuter({ {"MK",'R','S'} }, {"M", "K"}, {0, 1}, 3, false, 8);
   {
      std::set<Entry> got = AsSet(EntriesOf(0));
      std::set<Entry> want = { Entry({-1,1}) };   // only the K mod (compacted index 1) can be placed
      EXPECT_TRUE(got == want);
   }

   int iIgnoredBefore = IGNORED_SEQ_CNT;
   std::string sAllM(50, 'M');
   RunPermuter({ {sAllM,'-','-'} }, {"M"}, {5}, 5, true, 52);
   EXPECT_EQ(iIgnoredBefore + 1, IGNORED_SEQ_CNT);
   EXPECT_EQ((size_t)0, EntriesOf(0).size());
}

// P10: two identical runs produce byte-identical pools (what T18's determinism rests on).
TEST_F(PermuterTest, P10_Deterministic)
{
   std::vector<TestPeptide> peps = { {"MVTDSSDMK",'R','S'}, {"MVTDSSDMK",'-','S'}, {"ACSTYK",'K','-'} };
   std::vector<std::string> mods = {"M", "nK", "STY", "$"};
   std::vector<int> maxes = {2, 1, 2, 1};

   RunPermuter(peps, mods, maxes, 3, true, 12);
   std::vector<char> pool1(MOD_NUMBERS_POOL);
   std::vector<char> seqs1(MOD_SEQS_POOL);
   int num1 = MOD_NUM;

   RunPermuter(peps, mods, maxes, 3, true, 12);
   EXPECT_TRUE(pool1 == MOD_NUMBERS_POOL);
   EXPECT_TRUE(seqs1 == MOD_SEQS_POOL);
   EXPECT_EQ(num1, MOD_NUM);
   EXPECT_TRUE(num1 > 0);
}

// P11: ported from upstream vagisha/ModificationsPermuter Tests.cpp
// TestModificationsPermuter_testModificationRules2 -- four mods with overlapping residues
// (M, and three distinct K mods) on MVTDSSDMK: exactly 15 entries, in the engine's order
// (mod-type subsets largest-first, last mod varying fastest, per-mod bitmasks ascending),
// and no lysine ever carries two mods. Residue-only, so no sentinels.
TEST_F(PermuterTest, P11_UpstreamOverlappingKMods_OrderAndCount)
{
   RunPermuter({ {"MVTDSSDMK",'R','S'} }, {"M", "K", "K", "K"}, {3, 3, 3, 3}, 5, false, 12);

   EXPECT_EQ(std::string("MMK"), ModSeqOf(0));   // M0, M7, K8
   EntryList e = EntriesOf(0);
   EntryList want = {
      {-1, 0, 1}, { 0,-1, 1}, { 0, 0, 1},    // {M, K#}
      {-1, 0, 2}, { 0,-1, 2}, { 0, 0, 2},    // {M, K$}
      {-1, 0, 3}, { 0,-1, 3}, { 0, 0, 3},    // {M, K%}
      {-1, 0,-1}, { 0,-1,-1}, { 0, 0,-1},    // {M}
      {-1,-1, 1}, {-1,-1, 2}, {-1,-1, 3},    // {K#}, {K$}, {K%}
   };
   EXPECT_EQ(want.size(), e.size());
   EXPECT_TRUE(e == want);
}

// P12: ported from upstream TestModifications_Combine, adapted to Comet's fixed two-sentinel
// prefix (upstream reserves the two leading entry slots unconditionally; Comet's combine()
// simply writes whatever bit positions the modifiable string has, so the layout falls out of
// the sentinel prefix). Mod indices follow upstream: T=0 M=1 K=2 S=3 n=4 c=5.
TEST_F(PermuterTest, P12_UpstreamCombineCases)
{
   g_staticParams.variableModParameters.iMaxVarModPerPeptide = 5;

   auto run = [&](const char* sz, std::vector<int> mods, std::vector<unsigned long long> masks) -> std::pair<bool, Entry>
   {
      size_t tBefore = MOD_NUMBERS_POOL.size();
      int iLen = (int)strlen(sz);
      bool ok = ModificationsPermuter::combine(mods.data(), masks.data(), (int)mods.size(), iLen);
      Entry out;
      if (ok)
         for (size_t i = tBefore; i < MOD_NUMBERS_POOL.size(); ++i)
            out.push_back((int)(signed char)MOD_NUMBERS_POOL[i]);
      else
         EXPECT_EQ(tBefore, MOD_NUMBERS_POOL.size());
      return { ok, out };
   };

   // MSMMK with M*(idx1) at positions 0,2 (10100=20), S$(3) at 1 (01000=8), K@(2) at 4 (00001=1)
   auto r = run("MSMMK", {1, 3, 2}, {20, 8, 1});
   EXPECT_TRUE(r.first);
   EXPECT_EQ(Entry({1, 3, 1, -1, 2}), r.second);

   // both termini + three residue mods = 6 modified positions > cap 5 -> rejected
   std::string ncMSMMK = WithTerm(N_PEP, C_PEP, "MSMMK");
   r = run(ncMSMMK.c_str(), {4, 5, 1, 3, 2}, {64, 32, 20, 8, 1});
   EXPECT_FALSE(r.first);

   // N-term only
   std::string nAACLCFR = WithTerm(N_PEP, C_PEP, "AACLCFR");
   r = run(nAACLCFR.c_str(), {4}, {256});
   EXPECT_TRUE(r.first);
   EXPECT_EQ(Entry({4, -1, -1, -1, -1, -1, -1, -1, -1}), r.second);

   // C-term only
   r = run(nAACLCFR.c_str(), {5}, {128});
   EXPECT_TRUE(r.first);
   EXPECT_EQ(Entry({-1, 5, -1, -1, -1, -1, -1, -1, -1}), r.second);

   // N-term mod together with a mod on residue 0 (M*) and one on S$
   std::string nMVEDASIK = WithTerm(N_PEP, C_PEP, "MVEDASIK");
   r = run(nMVEDASIK.c_str(), {1, 3, 4}, {128, 4, 512});   // 10 positions: N-term is bit 9
   EXPECT_TRUE(r.first);
   EXPECT_EQ(Entry({4, -1, 1, -1, -1, -1, -1, 3, -1, -1}), r.second);

   // overlapping bit (22 = 10110 shares bit 4 with 20 and bit 1 with... itself) -> rejected
   r = run("MSMMK", {1, 3, 2}, {20, 22, 1});
   EXPECT_FALSE(r.first);
}
