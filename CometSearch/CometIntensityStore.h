// Copyright 2026 University of Washington
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _COMETINTENSITYSTORE_H_
#define _COMETINTENSITYSTORE_H_

#include "CometDataInternal.h"
#include "CometStatus.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

// Carafe predicted-intensity store for the intensity score
// (docs/20260903_IntensityScore_design.md Sections 2.1-2.3).
//
// Loads a .carafe_inten file (tools/carafe_cps_to_inten.py) -- per peptide-index variant, a
// sparse list of (channel, ladder position, quantized sqrt relative intensity) peaks -- binds
// it to the variant array the current search mode uses (g_fragmentPeptides for FI_DB,
// g_dbIndexVariants for PI_DB) so score-time lookup is O(1) by variant index, and computes
// the cosine between a candidate's predicted fragment intensities and the observed spectrum
// (Query::ppfSparseSpScoreData, binned sqrt intensities normalized to 100) over the
// candidate's b/y (and, when active, neutral-loss-shifted) ladder.
//
// Everything is static and read-only after LoadAndBind(); Score() is safe to call from any
// number of search threads concurrently (the RTS path calls it from C# Task threads).
class CometIntensityStore
{
public:
   // "No variant identity" sentinel for Score() -- e.g. PI_DB's on-the-fly reversed decoys,
   // which have no record. Scores 0.0 without counting as a missing lookup.
   static constexpr unsigned int NO_VARIANT = 0xFFFFFFFFu;

   // Loads strFile (a no-op returning true, IsEnabled() stays false, when strFile is empty)
   // and binds every entry to its index in `variants` (which must be final: mass-sorted and
   // complete). Verifies the magic/version line, the SourceIdxFingerprint /
   // SourceIdxNumRawPeptides header fields against the currently-loaded .idx
   // (g_staticParams.databaseInfo.szDatabase) and VarModConfig against live comet.params --
   // returns false (sets g_cometStatus, logs) on any mismatch rather than scoring against
   // the wrong peptide universe. Logs coverage (variants with a record / total).
   static bool LoadAndBind(const std::string& strFile, const VariantArray& variants);

   static bool IsEnabled();

   // Cosine similarity in [0, 1] between the variant's predicted peaks and the observed
   // spectrum, over the candidate's charge-1 b/y ladder as binned in uiBinnedIonMasses
   // (slot [1][series][ladderPos][0], plus the NL-shifted slot [..][iNlSlot+1] when the
   // file carries modloss channels and iFoundVariableMod == 2). Rounded to 4 decimals.
   // Returns 0.0 when disabled, for NO_VARIANT, or when the variant has no record (the
   // latter increments the missing-lookup counter reported by MissingLookups()).
   static double Score(unsigned int uiVariant,
                       const unsigned int uiBinnedIonMasses[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 2],
                       int iLenPeptide,
                       int iFoundVariableMod,
                       const Query* pQuery);

   static uint64_t MissingLookups();

   // Releases everything (called from the search strategies' finalize()).
   static void Free();

private:
   // Channel codes as written by tools/carafe_cps_to_inten.py (high byte of the peak code).
   // Only these four are scored; a file declaring other channel names (e.g. a future z2
   // set) has those peaks ignored, with one warning at load.
   enum Channel { CH_B = 0, CH_Y = 1, CH_B_ML = 2, CH_Y_ML = 3, NUM_CH = 4 };

   static constexpr size_t KEY_SIZE = 10;   // u32 iWhichPeptide, i32 modNumIdx, i8 nterm, i8 cterm
   static constexpr size_t PEAK_SIZE = 3;   // u16 code, u8 q
   static constexpr uint64_t NO_RECORD = ~0ULL;

   struct KeyOff
   {
      unsigned int iWhichPeptide;
      int modNumIdx;
      signed char cNtermMod;
      signed char cCtermMod;
      uint64_t offset;   // byte offset of the entry (its key) in s_blob
   };
   static bool KeyLess(const KeyOff& a, const KeyOff& b);

   static bool s_bEnabled;
   static bool s_bHasModloss;             // file's Channels include the two modloss channels
   static int  s_iNlSlot;                 // varModList slot whose dNeutralLoss the modloss channels shift by; -1 = none
   static std::vector<unsigned char> s_blob;       // the file's entry section, verbatim
   static std::vector<uint64_t> s_offsets;         // by variant index -> entry offset in s_blob, or NO_RECORD
   static std::atomic<uint64_t> s_missing;

   static bool Fail(const std::string& strMsg);
   static bool ParseHeader(FILE* fp, const std::string& strFile,
                           std::vector<std::pair<std::string, std::string>>& header, uint64_t& count);
};

#endif // _COMETINTENSITYSTORE_H_
