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
// candidate's b/y (and, when active, neutral-loss-shifted) ladder at fragment charges 1 and
// 2. Channels whose charge exceeds the spectrum's usiMaxFragCharge are excluded from both
// the dot product and |p| -- for a 2+ precursor (usiMaxFragCharge 1) the z2 predictions are
// ignored entirely, exactly as XCorr never scores z2 fragments there.
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

   // Channel codes as written by tools/carafe_cps_to_inten.py (high byte of the peak code;
   // its CHANNEL_NAMES order). The file's Channels header lists "code=name" pairs for the
   // subset it carries; codes are fixed per name.
   enum Channel { CH_B = 0, CH_Y = 1, CH_B_ML = 2, CH_Y_ML = 3,
                  CH_B2 = 4, CH_Y2 = 5, CH_B2_ML = 6, CH_Y2_ML = 7, NUM_CH = 8 };

   // A variant's predicted ladder, decoded once per candidate: sqrt(rel) = q/255 per
   // (channel, ladderPos), zero where nothing was predicted; z2 channels are dropped when
   // the spectrum's usiMaxFragCharge < 2 so every consumer sees exactly the ladder Comet
   // scores. bValid is false when the store is disabled, the variant has no record (counted
   // in MissingLookups()) or the record has no usable peak.
   struct Decoded
   {
      bool   bValid;
      double dPredNorm2;
      float  pred[NUM_CH][MAX_PEPTIDE_LEN];
   };
   static bool Decode(unsigned int uiVariant, const Query* pQuery, int iLenPeptide, Decoded& out);

   // Cosine similarity in [0, 1] between the decoded prediction and the observed spectrum,
   // over the candidate's b/y ladder as binned in uiBinnedIonMasses for fragment charges
   // 1..min(2, usiMaxFragCharge) (slot [z][series][ladderPos][0], plus the NL-shifted slot
   // [..][iNlSlot+1] when the file carries modloss channels and iFoundVariableMod == 2).
   // Rounded to 4 decimals; 0.0 when !d.bValid.
   //
   // dScoreBg receives the background-subtracted variant: the same cosine minus the mean,
   // over the 2*xcorr_processing_offset nonzero bin shifts k, of the shifted normalized
   // dot product sum_i p_i o_{bin_i+k} / (|p| |o|) -- i.e. the normalized spectrum is
   // shifted, the denominator stays the unshifted one (XCorr's own construction applied
   // to the cosine). Linear in o, so it reduces to one 151-bin window sum per ladder
   // position; bins past either array edge count as 0 like XCorr. May be negative.
   static double Score(const Decoded& d,
                       const unsigned int uiBinnedIonMasses[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 2],
                       int iLenPeptide,
                       int iFoundVariableMod,
                       const Query* pQuery,
                       double& dScoreBg);

   // Predicted-intensity-weighted XCorr (docs/20260903_IntensityScore_design.md Phase 3a):
   // XcorrScoreI() multiplies each ladder term of its fast-xcorr sum by this weight,
   // WEIGHT_FLOOR + (1 - WEIGHT_FLOOR) * sqrt(predicted relative intensity) for a b/y
   // position at fragment charge 1/2 (slot 0 unshifted, slot iNlSlot+1 the modloss channel)
   // and WEIGHT_FLOOR for everything without a prediction (other ion series, z3+, other
   // NL slots, precursor-loss peaks). Only meaningful when d.bValid.
   static constexpr float WEIGHT_FLOOR = 0.1f;

   // For xcorr_pred_g (Phase 3b): the observed value XCorr would see at `bin` if the spectrum
   // were normalized ONCE (base peak 50 over the whole spectrum, sqrt intensities -- i.e. the
   // SP-score array scaled by 0.5) instead of MakeCorrData()'s 10-window normalization, then
   // background-subtracted by the same +/- xcorr_processing_offset mean; with bFlank the
   // +/-1-bin half-weight flanking terms XCorr adds when theoretical_fragment_ions = 0.
   // No 5%-of-window floor. Costs one 151-bin window sum per term (three with flanking).
   static double GlobalXcorrValue(const Query* pQuery, int bin, bool bFlank);
   static inline float Weight(const Decoded& d, int iWhichIonSeries, int ctCharge, int ctLen, int iSlot)
   {
      if (!d.bValid)
         return WEIGHT_FLOOR;
      int ch;
      if (iWhichIonSeries == ION_SERIES_B)
         ch = CH_B;
      else if (iWhichIonSeries == ION_SERIES_Y)
         ch = CH_Y;
      else
         return WEIGHT_FLOOR;
      if (ctCharge == 2)
         ch += CH_B2;
      else if (ctCharge != 1)
         return WEIGHT_FLOOR;
      if (iSlot != 0)
      {
         if (s_iNlSlot >= 0 && iSlot == s_iNlSlot + 1)
            ch += CH_B_ML;
         else
            return WEIGHT_FLOOR;
      }
      return WEIGHT_FLOOR + (1.0f - WEIGHT_FLOOR) * d.pred[ch][ctLen];
   }

   // Same as Weight() but linear in the predicted RELATIVE intensity (no sqrt): the stored
   // value is sqrt(rel), so rel = pred^2. For xcorr_pred_lin (Phase 3c).
   static inline float WeightLin(const Decoded& d, int iWhichIonSeries, int ctCharge, int ctLen, int iSlot)
   {
      float w = Weight(d, iWhichIonSeries, ctCharge, ctLen, iSlot);
      if (w <= WEIGHT_FLOOR)
         return WEIGHT_FLOOR;
      float p = (w - WEIGHT_FLOOR) / (1.0f - WEIGHT_FLOOR);   // back to sqrt(rel)
      return WEIGHT_FLOOR + (1.0f - WEIGHT_FLOOR) * p * p;
   }

   static uint64_t MissingLookups();

   // Releases everything (called from the search strategies' finalize()).
   static void Free();

private:
   static constexpr size_t KEY_SIZE = 10;   // u32 iWhichPeptide, i32 modNumIdx, i8 nterm, i8 cterm
   // entry = key, u8 precursor charge (0 = merged over all predicted charges), u8 nPeaks, peaks
   static constexpr size_t ENTRY_HDR_SIZE = KEY_SIZE + 2;
   static constexpr size_t PEAK_SIZE = 3;   // u16 code, u8 q
   static constexpr uint64_t NO_RECORD = ~0ULL;

   struct KeyOff
   {
      unsigned int iWhichPeptide;
      int modNumIdx;
      signed char cNtermMod;
      signed char cCtermMod;
      uint64_t offset;   // byte offset of the variant's FIRST entry (its key) in s_blob;
                         // the variant's other per-charge entries follow consecutively
   };
   static bool KeyLess(const KeyOff& a, const KeyOff& b);

   static bool s_bEnabled;
   static bool s_bHasModloss;             // file's Channels include modloss channels
   static bool s_bChannelPresent[NUM_CH]; // which channel codes the file declares
   static int  s_iNlSlot;                 // varModList slot whose dNeutralLoss the modloss channels shift by; -1 = none
   static std::vector<unsigned char> s_blob;       // the file's entry section, verbatim
   static std::vector<uint64_t> s_offsets;         // by variant index -> first entry offset in s_blob, or NO_RECORD
   static bool s_bPerCharge;                        // file has one record per (variant, precursor charge)
   static std::atomic<uint64_t> s_missing;

   static bool Fail(const std::string& strMsg);
   static bool ParseHeader(FILE* fp, const std::string& strFile,
                           std::vector<std::pair<std::string, std::string>>& header, uint64_t& count);
};

#endif // _COMETINTENSITYSTORE_H_
