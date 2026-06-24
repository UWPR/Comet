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

// Unit tests for CometSearch and CometPreprocess public/testable methods.
// Framework: MiniTest.h, a dependency-free in-repo harness (see that file
// for rationale -- avoids vendoring gtest or any CMake/NuGet fetch step).
//
// Build notes:
//   Add this file and link against CometSearch (static lib).
//   The tests that rely on global state (g_staticParams) call a minimal
//   SetupMinimalStaticParams() helper defined below.

#include "MiniTest.h"
#include <cstring>
#include <vector>
#include <cmath>

// Project headers - adjust relative paths if the test binary lives elsewhere.
#include "../../CometSearch/Common.h"
#include "../../CometSearch/CometDataInternal.h"
#include "../../CometSearch/CometSearch.h"
#include "../../CometSearch/CometPreprocess.h"
#include "../../CometSearch/CometStatus.h"

// ============================================================
//  Minimal global-state bootstrap required by several methods
// ============================================================
namespace
{

   // Populate only the fields exercised by the tested methods.
   void SetupMinimalStaticParams()
   {
      // Enzyme: trypsin-like (c-term offset, break on K/R, no-break before P)
      // NOTE: enzyme termini mode lives on g_staticParams.options, not
      // g_staticParams.enzymeInformation -- EnzymeInfo has no such member.
      g_staticParams.options.iEnzymeTermini = ENZYME_DOUBLE_TERMINI;
      g_staticParams.enzymeInformation.iSearchEnzymeOffSet = 1;   // c-term cleavage
      g_staticParams.enzymeInformation.iSearchEnzyme2OffSet = 1;
      g_staticParams.enzymeInformation.bNoEnzymeSelected = 0;
      // CheckEnzymeStartTermini/CheckEnzymeEndTermini only evaluate their
      // real per-position logic when both enzymes are "active" (see
      // CometSearch.cpp's `!bNoEnzymeSelected && !bNoEnzyme2Selected` guard);
      // otherwise they unconditionally return true. Since szSearchEnzyme2*
      // below is "-" (never matches a real residue), enabling enzyme2 here
      // is a safe no-op for every check except unlocking that guard.
      g_staticParams.enzymeInformation.bNoEnzyme2Selected = false;
      strncpy(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, "KR", sizeof(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA) - 1);
      strncpy(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, "P", sizeof(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA) - 1);
      strncpy(g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA, "-", sizeof(g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA) - 1);
      strncpy(g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA, "-", sizeof(g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA) - 1);

      g_staticParams.tolerances.iIsotopeError = 0;
      g_staticParams.tolerances.dInputTolerancePlus = 1.0;
      g_staticParams.tolerances.iMassToleranceUnits = 0; // amu
      g_staticParams.tolerances.iMassToleranceType = 0; // MH+
      g_staticParams.tolerances.dMS1BinSize = 1.0;

      g_staticParams.options.dPeptideMassLow = 100.0;
      g_staticParams.options.dPeptideMassHigh = 10000.0;
      g_staticParams.options.iSpectrumBatchSize = 0; // no batch limit
      g_staticParams.options.iDecoySearch = 0;

      g_staticParams.iArraySizeGlobal = 16000;
      g_staticParams.iDbType = DbType::FASTA_DB;

      g_staticParams.vectorMassOffsets.clear();

      // Binning parameters (1 Da bins, 0.4 offset)
      g_staticParams.dInverseBinWidth = 1.0;
      g_staticParams.dOneMinusBinOffset = 0.6;

      // Status: clear any error/cancel flags
      g_cometStatus.SetStatus(CometResult_Succeeded, "");
   }

} // anonymous namespace

// ============================================================
//  Test fixture that resets global state before every test
// ============================================================
class CometSearchTest : public minitest::Test
{
protected:
   void SetUp()
   {
      SetupMinimalStaticParams();
   }
};

class CometPreprocessTest : public minitest::Test
{
protected:
   void SetUp()
   {
      SetupMinimalStaticParams();
   }
};

// ============================================================
//  CometSearch - CheckEnzymeTermini
//  Trypsin (offset=1): break on K/R, not before P
// ============================================================

// Helper: build a CometSearch object whose _proteinInfo is initialised.
static CometSearch MakeSearchObj(int seqLen)
{
   CometSearch cs;
   cs._proteinInfo.iProteinSeqLength = seqLen;
   cs._proteinInfo.iTmpProteinSeqLength = seqLen;
   cs._proteinInfo.iPeffOrigResiduePosition = NO_PEFF_VARIANT;
   cs._proteinInfo.iPeffNewResidueCount = 0;
   return cs;
}

// Protein N-terminus peptide with valid K at end: "MAAAK" in "MAAAKDDD"
TEST_F(CometSearchTest, CheckEnzymeTermini_FullyTryptic_Passes)
{
   const char* szSeq = "MAAAKDDD";
   // iStartPos=0 (protein N-term), iEndPos=4 ('K')
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_TRUE(cs.CheckEnzymeTermini(szSeq, 0, 4));
}

// K followed by P should NOT be a trypsin cleavage site (no-break before P)
TEST_F(CometSearchTest, CheckEnzymeTermini_NocleavageBeforeP_Fails)
{
   const char* szSeq = "MAAAKPAAA";
   // iStartPos=0, iEndPos=4 ('K' before 'P'): c-term end is invalid for trypsin
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_FALSE(cs.CheckEnzymeTermini(szSeq, 0, 4));
}

// Internal peptide with K at start (previous residue K) and K at end
TEST_F(CometSearchTest, CheckEnzymeTermini_InternalTryptic_Passes)
{
   // NOTE: the original version of this test used "AAAKDDDKPPP" with
   // iEndPos=7 ('K' immediately followed by 'P' at position 8), which is
   // *not* a valid tryptic C-terminus -- trypsin does not cleave K|P.
   // Using 'A' instead of 'P' after the second K makes this genuinely
   // fully-tryptic on both ends.
   const char* szSeq = "AAAKDDDKAAA";
   // K before pos 4 is valid n-term, K at pos 7 (followed by 'A', not 'P') is valid c-term
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_TRUE(cs.CheckEnzymeTermini(szSeq, 4, 7));
}

// No cleavage site at either end - should fail strict double-termini
TEST_F(CometSearchTest, CheckEnzymeTermini_NonTryptic_Fails)
{
   const char* szSeq = "MAAADDDAAA";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   // Internal, no K/R at either boundary
   EXPECT_FALSE(cs.CheckEnzymeTermini(szSeq, 3, 7));
}

// Protein C-terminus peptide: valid because C-terminus is always allowed
TEST_F(CometSearchTest, CheckEnzymeTermini_ProteinCterm_Passes)
{
   const char* szSeq = "MAAAKDDD";
   // "DDD" is the last peptide (pos 5-7): it ends at protein c-term
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_TRUE(cs.CheckEnzymeTermini(szSeq, 5, 7));
}

// Single-termini mode: only one enzyme end needs to match
TEST_F(CometSearchTest, CheckEnzymeTermini_SingleTermini_OneMatch)
{
   g_staticParams.options.iEnzymeTermini = ENZYME_SINGLE_TERMINI;
   const char* szSeq = "MAAADDDKPPP";
   // c-term at pos 7 is K, but pos 8 is P - would fail double-termini check;
   // verify single-termini still works when c-term is at end of protein
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_TRUE(cs.CheckEnzymeTermini(szSeq, 5, 10)); // ends at C-terminus of protein
}

// ============================================================
//  CometSearch - CheckEnzymeStartTermini / CheckEnzymeEndTermini
// ============================================================

TEST_F(CometSearchTest, CheckEnzymeStartTermini_AtProteinNterm_Passes)
{
   const char* szSeq = "MKAAADDD";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_TRUE(cs.CheckEnzymeStartTermini(szSeq, 0)); // protein N-term always valid start
}

TEST_F(CometSearchTest, CheckEnzymeStartTermini_AfterCleavageSite_Passes)
{
   const char* szSeq = "MAAAKDDD";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   // Position 5 follows 'K' at position 4 -> valid trypsin start
   EXPECT_TRUE(cs.CheckEnzymeStartTermini(szSeq, 5));
}

TEST_F(CometSearchTest, CheckEnzymeStartTermini_AfterNonCleavage_Fails)
{
   const char* szSeq = "MAAADDD";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   // Position 3 follows 'A' -> not a trypsin cleavage site
   EXPECT_FALSE(cs.CheckEnzymeStartTermini(szSeq, 3));
}

TEST_F(CometSearchTest, CheckEnzymeEndTermini_AtProteinCterm_Passes)
{
   const char* szSeq = "MAAADDD";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   EXPECT_TRUE(cs.CheckEnzymeEndTermini(szSeq, 6)); // last residue is always valid end
}

TEST_F(CometSearchTest, CheckEnzymeEndTermini_AtCleavageSite_Passes)
{
   const char* szSeq = "MAAAKDDD";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   // 'K' at position 4, next residue 'D' - not P -> valid trypsin c-term
   EXPECT_TRUE(cs.CheckEnzymeEndTermini(szSeq, 4));
}

TEST_F(CometSearchTest, CheckEnzymeEndTermini_KBeforeP_Fails)
{
   const char* szSeq = "MAAAKPAAA";
   CometSearch cs = MakeSearchObj(static_cast<int>(strlen(szSeq)));
   // 'K' at position 4 before 'P' at 5 -> trypsin no-break
   EXPECT_FALSE(cs.CheckEnzymeEndTermini(szSeq, 4));
}

// ============================================================
//  CometSearch - BinarySearchMass / CheckMassMatch
// ============================================================

static Query* MakeQuery(double dLow, double dHigh)
{
   Query* q = new Query();
   q->_pepMassInfo.dPeptideMassToleranceMinus = dLow;
   q->_pepMassInfo.dPeptideMassTolerancePlus = dHigh;
   return q;
}

class BinarySearchMassFixture : public minitest::Test
{
protected:
   std::vector<Query*> queries;
   CometSearch cs;

   void SetUp()
   {
      SetupMinimalStaticParams();
   }

   void TearDown()
   {
      for (auto* q : queries) delete q;
      queries.clear();
   }
};

TEST_F(BinarySearchMassFixture, CheckMassMatchStatic_WithinTolerance_ReturnsTrue)
{
   Query* q = MakeQuery(1000.0, 1002.0);
   q->_pepMassInfo.dPeptideMassToleranceLow = 999.5;
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1002.5;
   EXPECT_TRUE(CometSearch::CheckMassMatch(q, 1001.0));
   delete q;
}

TEST_F(BinarySearchMassFixture, CheckMassMatchStatic_BelowTolerance_ReturnsFalse)
{
   Query* q = MakeQuery(1000.0, 1002.0);
   q->_pepMassInfo.dPeptideMassToleranceLow = 999.5;
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1002.5;
   EXPECT_FALSE(CometSearch::CheckMassMatch(q, 990.0));
   delete q;
}

TEST_F(BinarySearchMassFixture, CheckMassMatchStatic_AboveTolerance_ReturnsFalse)
{
   Query* q = MakeQuery(1000.0, 1002.0);
   q->_pepMassInfo.dPeptideMassToleranceLow = 999.5;
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1002.5;
   EXPECT_FALSE(CometSearch::CheckMassMatch(q, 1010.0));
   delete q;
}

TEST_F(BinarySearchMassFixture, CheckMassMatchStatic_AtLowerBound_ReturnsTrue)
{
   Query* q = MakeQuery(1000.0, 1002.0);
   q->_pepMassInfo.dPeptideMassToleranceLow = 1000.0;
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1002.0;
   EXPECT_TRUE(CometSearch::CheckMassMatch(q, 1000.0));
   delete q;
}

TEST_F(BinarySearchMassFixture, CheckMassMatchStatic_AtUpperBound_ReturnsTrue)
{
   Query* q = MakeQuery(1000.0, 1002.0);
   q->_pepMassInfo.dPeptideMassToleranceLow = 1000.0;
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1002.0;
   EXPECT_TRUE(CometSearch::CheckMassMatch(q, 1002.0));
   delete q;
}

// ============================================================
//  CometSearch - CheckMassMatch (static overload) with isotope errors
// ============================================================

TEST_F(CometSearchTest, CheckMassMatchStatic_IsotopeError1_C13Match_ReturnsTrue)
{
   // NOTE: dPeptideMassToleranceMinus/Plus is the *wide*, isotope-expanded
   // gate checked first; dPeptideMassToleranceLow/High is the *narrow*,
   // per-candidate window checked inside the isotope-compensation loop
   // (see how CometPreprocess.cpp populates these: Minus/Plus get widened
   // by the isotope offset, Low/High do not). The original version of this
   // test had the two pairs swapped, so dCalcPepMass never passed the
   // outer gate and the isotope loop was never reached.
   g_staticParams.tolerances.iIsotopeError = 1; // 0, +1 C13
   Query* q = MakeQuery(999.0, 1002.0);             // wide outer gate
   q->_pepMassInfo.dPeptideMassToleranceLow = 1000.5;  // narrow per-candidate window
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1001.5;
   // dCalcPepMass = 1000.0 is within the wide [999, 1002] gate but outside
   // the narrow [1000.5, 1001.5] window directly; adding one C13 unit
   // (~1.00335) gives 1001.00335, which falls inside the narrow window.
   EXPECT_TRUE(CometSearch::CheckMassMatch(q, 1000.0));
   delete q;
   g_staticParams.tolerances.iIsotopeError = 0;
}

TEST_F(CometSearchTest, CheckMassMatchStatic_IsotopeError0_OutsideWindow_ReturnsFalse)
{
   g_staticParams.tolerances.iIsotopeError = 0;
   Query* q = MakeQuery(1000.0, 1002.0);
   q->_pepMassInfo.dPeptideMassToleranceLow = 999.5;
   q->_pepMassInfo.dPeptideMassToleranceHigh = 1002.5;
   EXPECT_FALSE(CometSearch::CheckMassMatch(q, 999.0));
   delete q;
}

// ============================================================
//  CometSearch - AllocateMemory / DeallocateMemory
// ============================================================

TEST_F(CometSearchTest, AllocateAndDeallocateMemory_OnlyOneThread_Succeeds)
{
   g_bCometSearchMemoryAllocated = false;
   g_staticParams.iArraySizeGlobal = 1000; // small for tests
   EXPECT_TRUE(CometSearch::AllocateMemory(1));
   EXPECT_TRUE(g_bCometSearchMemoryAllocated);
   EXPECT_TRUE(CometSearch::DeallocateMemory(1));
   EXPECT_FALSE(g_bCometSearchMemoryAllocated);
}

TEST_F(CometSearchTest, AllocateMemory_AlreadyAllocated_ReturnsTrue)
{
   g_bCometSearchMemoryAllocated = false;
   g_staticParams.iArraySizeGlobal = 1000;
   EXPECT_TRUE(CometSearch::AllocateMemory(1));
   EXPECT_TRUE(CometSearch::AllocateMemory(1)); // idempotent
   CometSearch::DeallocateMemory(1);
}

TEST_F(CometSearchTest, DeallocateMemory_WhenNotAllocated_ReturnsTrue)
{
   g_bCometSearchMemoryAllocated = false;
   EXPECT_TRUE(CometSearch::DeallocateMemory(1));
}

// ============================================================
//  CometSearch - GetAA (DNA codon -> amino acid translation)
//  GetAA is private; exposed via a thin public wrapper subclass.
//  For the forward strand (direction = +1), GetAA reads positions i, i+1, i+2.
// ============================================================

class CometSearchTestable : public CometSearch
{
public:
   char PublicGetAA(int i, int iDirection, char* seq) { return GetAA(i, iDirection, seq); }
};

TEST_F(CometSearchTest, GetAA_ATG_ForwardStrand_ReturnsM)
{
   CometSearchTestable cs;
   char dna[] = "ATG";
   EXPECT_EQ('M', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_TAA_StopCodon_ReturnsAt)
{
   CometSearchTestable cs;
   char dna[] = "TAA";
   EXPECT_EQ('@', cs.PublicGetAA(0, 1, dna)); // stop codon encodes as '@'
}

TEST_F(CometSearchTest, GetAA_GGT_ForwardStrand_ReturnsG)
{
   CometSearchTestable cs;
   char dna[] = "GGT";
   EXPECT_EQ('G', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_TTC_ForwardStrand_ReturnsF)
{
   CometSearchTestable cs;
   char dna[] = "TTC";
   EXPECT_EQ('F', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_CTG_ForwardStrand_ReturnsL)
{
   CometSearchTestable cs;
   char dna[] = "CTG";
   EXPECT_EQ('L', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_CCG_ForwardStrand_ReturnsP)
{
   CometSearchTestable cs;
   char dna[] = "CCG";
   EXPECT_EQ('P', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_ACT_ForwardStrand_ReturnsT)
{
   CometSearchTestable cs;
   char dna[] = "ACT";
   EXPECT_EQ('T', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_GAC_ForwardStrand_ReturnsD)
{
   CometSearchTestable cs;
   char dna[] = "GAC";
   EXPECT_EQ('D', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_GAA_ForwardStrand_ReturnsE)
{
   CometSearchTestable cs;
   char dna[] = "GAA";
   EXPECT_EQ('E', cs.PublicGetAA(0, 1, dna));
}

TEST_F(CometSearchTest, GetAA_UnknownCodon_ReturnsStar)
{
   CometSearchTestable cs;
   char dna[] = "NNN";
   EXPECT_EQ('*', cs.PublicGetAA(0, 1, dna));
}

// Reverse strand: direction = -1, reads from position i backwards
TEST_F(CometSearchTest, GetAA_ReverseStrand_ATG_at2_ReturnsM)
{
   CometSearchTestable cs;
   char dna[] = "GTA"; // reverse: A(2), T(1), G(0) -> ATG -> M
   EXPECT_EQ('M', cs.PublicGetAA(2, -1, dna));
}

// NOTE: the original version of this file had a
// "Constructor_InitializesSizesCorrectly" test here that read
// cs._usiSizepiVarModSites directly. That member is private (CometSearch.h)
// and has no externally-observable effect on its own -- it's an internal
// sizing field consumed later during var-mod site allocation. Unlike GetAA,
// there's no real behavior to test by exposing it, so the test is dropped
// rather than widening the class's encapsulation just to read a private
// implementation detail.

// ============================================================
//  CometPreprocess - IsValidInputType
// ============================================================

TEST_F(CometPreprocessTest, IsValidInputType_MZXML_ReturnsTrue)
{
   EXPECT_TRUE(CometPreprocess::IsValidInputType(InputType_MZXML));
}

TEST_F(CometPreprocessTest, IsValidInputType_RAW_ReturnsTrue)
{
   EXPECT_TRUE(CometPreprocess::IsValidInputType(InputType_RAW));
}

TEST_F(CometPreprocessTest, IsValidInputType_MS2_ReturnsFalse)
{
   EXPECT_FALSE(CometPreprocess::IsValidInputType(InputType_MS2));
}

TEST_F(CometPreprocessTest, IsValidInputType_MZML_ReturnsFalse)
{
   EXPECT_FALSE(CometPreprocess::IsValidInputType(InputType_MZML));
}

TEST_F(CometPreprocessTest, IsValidInputType_MGF_ReturnsFalse)
{
   EXPECT_FALSE(CometPreprocess::IsValidInputType(InputType_MGF));
}

TEST_F(CometPreprocessTest, IsValidInputType_Unknown_ReturnsFalse)
{
   EXPECT_FALSE(CometPreprocess::IsValidInputType(InputType_UNKNOWN));
}

// ============================================================
//  CometPreprocess - GetMassCushion
// ============================================================

TEST_F(CometPreprocessTest, GetMassCushion_AMU_NoMZType_EqualsTolPlusConstant)
{
   g_staticParams.tolerances.iMassToleranceUnits = 0; // amu
   g_staticParams.tolerances.iMassToleranceType = 0; // MH+ (not m/z)
   g_staticParams.tolerances.dInputTolerancePlus = 2.0;
   double dCushion = CometPreprocess::GetMassCushion(1000.0);
   EXPECT_DOUBLE_EQ(2.0 + 5.0, dCushion); // dInputTolerancePlus + 5.0
}
TEST_F(CometPreprocessTest, GetMassCushion_AMU_MZType_MultipliedByEight)
{
   g_staticParams.tolerances.iMassToleranceUnits = 0; // amu
   g_staticParams.tolerances.iMassToleranceType = 1; // precursor m/z
   g_staticParams.tolerances.dInputTolerancePlus = 2.0;
   double dCushion = CometPreprocess::GetMassCushion(1000.0);
   EXPECT_DOUBLE_EQ(2.0 * 8.0 + 5.0, dCushion);
}

TEST_F(CometPreprocessTest, GetMassCushion_MMU_NoMZType_Correct)
{
   g_staticParams.tolerances.iMassToleranceUnits = 1; // mmu
   g_staticParams.tolerances.iMassToleranceType = 0;
   g_staticParams.tolerances.dInputTolerancePlus = 200.0; // 200 mmu = 0.2 amu
   double dCushion = CometPreprocess::GetMassCushion(1000.0);
   EXPECT_DOUBLE_EQ(200.0 * 0.001 + 5.0, dCushion);
}

TEST_F(CometPreprocessTest, GetMassCushion_PPM_ScalesWithMass)
{
   g_staticParams.tolerances.iMassToleranceUnits = 2; // ppm
   g_staticParams.tolerances.iMassToleranceType = 0;
   g_staticParams.tolerances.dInputTolerancePlus = 10.0; // 10 ppm
   double dMass = 2000.0;
   double dCushion = CometPreprocess::GetMassCushion(dMass);
   double dExpected = 10.0 * dMass / 1.0e6 + 5.0;
   EXPECT_DOUBLE_EQ(dExpected, dCushion);
}

TEST_F(CometPreprocessTest, GetMassCushion_AlwaysAddsBaseline5)
{
   g_staticParams.tolerances.iMassToleranceUnits = 0;
   g_staticParams.tolerances.iMassToleranceType = 0;
   g_staticParams.tolerances.dInputTolerancePlus = 0.0;
   double dCushion = CometPreprocess::GetMassCushion(500.0);
   EXPECT_DOUBLE_EQ(5.0, dCushion);
}

// ============================================================
//  CometPreprocess - CheckExit
// ============================================================

TEST_F(CometPreprocessTest, CheckExit_ErrorStatus_ReturnsTrue)
{
   g_cometStatus.SetStatus(CometResult_Failed, "forced error");
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_EntireFile, 100, 200, 0, 200, 5, false));
   g_cometStatus.SetStatus(CometResult_Succeeded, "");
}

TEST_F(CometPreprocessTest, CheckExit_SpecificScan_AlwaysExits)
{
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_SpecificScan, 42, 0, 0, 0, 1, false));
}

TEST_F(CometPreprocessTest, CheckExit_ScanRange_ScanNumBelowLastScan_ReturnsFalse)
{
   EXPECT_FALSE(CometPreprocess::CheckExit(
      AnalysisType_SpecificScanRange, 5, 0, 10, 100, 1, false));
}

TEST_F(CometPreprocessTest, CheckExit_ScanRange_ScanNumAtLastScan_ReturnsTrue)
{
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_SpecificScanRange, 10, 0, 10, 100, 1, false));
}

TEST_F(CometPreprocessTest, CheckExit_ScanRange_ScanNumAboveLastScan_ReturnsTrue)
{
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_SpecificScanRange, 15, 0, 10, 100, 1, false));
}
TEST_F(CometPreprocessTest, CheckExit_EntireFile_ScanNumZero_WithValidType_ReturnsTrue)
{
   g_staticParams.inputFile.iInputType = InputType_MZXML;
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_EntireFile, 0, 0, 0, 100, 1, false));
}
TEST_F(CometPreprocessTest, CheckExit_EntireFile_ScanNumNonZero_NonValidType_ReturnsFalse)
{
   g_staticParams.inputFile.iInputType = InputType_MS2;
   g_staticParams.options.iSpectrumBatchSize = 0;
   EXPECT_FALSE(CometPreprocess::CheckExit(
      AnalysisType_EntireFile, 5, 10, 0, 100, 1, false));
}
TEST_F(CometPreprocessTest, CheckExit_BatchSizeReached_ReturnsTrue)
{
   g_staticParams.inputFile.iInputType = InputType_MS2;
   g_staticParams.options.iSpectrumBatchSize = 5;
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_EntireFile, 5, 10, 0, 100, 5, false));
}

TEST_F(CometPreprocessTest, CheckExit_BatchSizeFlagIgnored_ReturnsFalse)
{
   g_staticParams.inputFile.iInputType = InputType_MS2;
   g_staticParams.options.iSpectrumBatchSize = 5;
   EXPECT_FALSE(CometPreprocess::CheckExit(
      AnalysisType_EntireFile, 5, 10, 0, 100, 5, true)); // bIgnoreSpectrumBatchSize
}
TEST_F(CometPreprocessTest, CheckExit_TotalScansExceedsReaderLastScan_ReturnsTrue)
{
   g_staticParams.inputFile.iInputType = InputType_MZXML;
   // iTotalScans (101) > iReaderLastScan (100) triggers done for valid input types
   EXPECT_TRUE(CometPreprocess::CheckExit(
      AnalysisType_EntireFile, 5, 101, 0, 100, 1, false));
}

// ============================================================
//  CometPreprocess - AllocateMemory / DeallocateMemory
// ============================================================

TEST_F(CometPreprocessTest, AllocateAndDeallocateMemory_Succeeds)
{
   g_bCometPreprocessMemoryAllocated = false;
   g_staticParams.iArraySizeGlobal = 1000;
   EXPECT_TRUE(CometPreprocess::AllocateMemory(1));
   EXPECT_TRUE(g_bCometPreprocessMemoryAllocated);
   EXPECT_TRUE(CometPreprocess::DeallocateMemory(1));
   EXPECT_FALSE(g_bCometPreprocessMemoryAllocated);
}

TEST_F(CometPreprocessTest, AllocateMemory_AlreadyAllocated_ReturnsTrue)
{
   g_bCometPreprocessMemoryAllocated = false;
   g_staticParams.iArraySizeGlobal = 1000;
   EXPECT_TRUE(CometPreprocess::AllocateMemory(1));
   EXPECT_TRUE(CometPreprocess::AllocateMemory(1)); // idempotent
   CometPreprocess::DeallocateMemory(1);
}

TEST_F(CometPreprocessTest, DeallocateMemory_WhenNotAllocated_ReturnsTrue)
{
   g_bCometPreprocessMemoryAllocated = false;
   EXPECT_TRUE(CometPreprocess::DeallocateMemory(1));
}

// ============================================================
//  CometPreprocess - Reset / DoneProcessingAllSpectra
// ============================================================

TEST_F(CometPreprocessTest, Reset_ClearsDoneFlag)
{
   CometPreprocess::Reset();
   EXPECT_FALSE(CometPreprocess::DoneProcessingAllSpectra());
}

TEST_F(CometPreprocessTest, DoneProcessingAllSpectra_InitiallyFalse)
{
   CometPreprocess::Reset();
   EXPECT_FALSE(CometPreprocess::DoneProcessingAllSpectra());
}

MINITEST_MAIN()