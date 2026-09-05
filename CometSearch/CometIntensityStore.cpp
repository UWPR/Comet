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

#include "Common.h"
#include "CometIntensityStore.h"
#include "CometPredictedMask.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

// tools/carafe_cps_to_inten.py's INTEN_FILE_MAGIC / CHANNEL_NAMES
static const char* INTEN_FILE_MAGIC = "Comet Carafe intensity v3\n";
static const char* CHANNEL_NAMES[8] = { "b_z1", "y_z1", "b_modloss_z1", "y_modloss_z1",
                                        "b_z2", "y_z2", "b_modloss_z2", "y_modloss_z2" };

bool CometIntensityStore::s_bEnabled = false;
bool CometIntensityStore::s_bHasModloss = false;
bool CometIntensityStore::s_bChannelPresent[CometIntensityStore::NUM_CH] = { false };
int  CometIntensityStore::s_iNlSlot = -1;
std::vector<unsigned char> CometIntensityStore::s_blob;
std::vector<uint64_t> CometIntensityStore::s_offsets;
bool CometIntensityStore::s_bPerCharge = false;
std::atomic<uint64_t> CometIntensityStore::s_missing(0);


bool CometIntensityStore::IsEnabled()
{
   return s_bEnabled;
}


uint64_t CometIntensityStore::MissingLookups()
{
   return s_missing.load();
}


void CometIntensityStore::Free()
{
   s_bEnabled = false;
   s_bHasModloss = false;
   for (int i = 0; i < NUM_CH; ++i)
      s_bChannelPresent[i] = false;
   s_iNlSlot = -1;
   s_bPerCharge = false;
   std::vector<unsigned char>().swap(s_blob);
   std::vector<uint64_t>().swap(s_offsets);
   s_missing.store(0);
}


bool CometIntensityStore::Fail(const std::string& strMsg)
{
   g_cometStatus.SetStatus(CometResult_Failed, strMsg);
   logerr(strMsg);
   Free();
   return false;
}


bool CometIntensityStore::KeyLess(const KeyOff& a, const KeyOff& b)
{
   if (a.iWhichPeptide != b.iWhichPeptide) return a.iWhichPeptide < b.iWhichPeptide;
   if (a.modNumIdx != b.modNumIdx)         return a.modNumIdx < b.modNumIdx;
   if (a.cNtermMod != b.cNtermMod)         return a.cNtermMod < b.cNtermMod;
   return a.cCtermMod < b.cCtermMod;
}


// Reads the magic line, the "Key: Value" header lines up to the blank line, and the u64
// entry count. Leaves fp positioned at the first entry.
bool CometIntensityStore::ParseHeader(FILE* fp, const std::string& strFile,
                                      std::vector<std::pair<std::string, std::string>>& header,
                                      uint64_t& count)
{
   size_t magicLen = strlen(INTEN_FILE_MAGIC);
   std::vector<char> magic(magicLen + 1, '\0');
   if (fread(magic.data(), 1, magicLen, fp) != magicLen || strncmp(magic.data(), INTEN_FILE_MAGIC, magicLen) != 0)
   {
      return Fail(" Error - \"" + strFile + "\" is not a Comet Carafe intensity v3 file (bad magic line).\n");
   }

   char szLine[4096];
   while (true)
   {
      if (fgets(szLine, sizeof(szLine), fp) == NULL)
         return Fail(" Error - \"" + strFile + "\": unterminated header.\n");
      if (szLine[0] == '\n')
         break;
      std::string line(szLine);
      while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
         line.pop_back();
      size_t sep = line.find(": ");
      if (sep == std::string::npos)
         return Fail(" Error - \"" + strFile + "\": malformed header line \"" + line + "\".\n");
      header.push_back(std::make_pair(line.substr(0, sep), line.substr(sep + 2)));
   }

   if (fread(&count, sizeof(uint64_t), 1, fp) != 1)
      return Fail(" Error - \"" + strFile + "\": cannot read entry count.\n");
   return true;
}


bool CometIntensityStore::LoadAndBind(const std::string& strFile, const VariantArray& variants)
{
   Free();

   if (strFile.empty())
      return true;   // intensity score disabled -- not an error

   FILE* fp = fopen(strFile.c_str(), "rb");
   if (fp == NULL)
      return Fail(" Error - cannot open predicted-intensity file \"" + strFile + "\".\n");

   std::vector<std::pair<std::string, std::string>> headerList;
   uint64_t count = 0;
   if (!ParseHeader(fp, strFile, headerList, count))
   {
      fclose(fp);
      return false;
   }
   std::map<std::string, std::string> header(headerList.begin(), headerList.end());

   const char* required[] = { "SourceIdxFingerprint", "SourceIdxNumRawPeptides", "VarModConfig",
                              "Channels", "Transform", "Quant", "PerCharge" };
   for (const char* key : required)
   {
      if (header.find(key) == header.end())
      {
         fclose(fp);
         return Fail(" Error - \"" + strFile + "\": header lacks required field \"" + std::string(key) + "\".\n");
      }
   }
   s_bPerCharge = (header["PerCharge"] == "1");
   if (header["Transform"] != "sqrt" || header["Quant"] != "u8")
   {
      fclose(fp);
      return Fail(" Error - \"" + strFile + "\": unsupported Transform/Quant (" + header["Transform"]
         + "/" + header["Quant"] + "); this build scores Transform: sqrt, Quant: u8.\n");
   }

   // --- bind to the loaded .idx and live params, exactly as CometPredictedMask::Load() ---
   unsigned long liveFingerprint = 0;
   uint64_t liveNumRaw = 0;
   if (!CometPredictedMask::ComputeIdxFingerprint(liveFingerprint, liveNumRaw))
   {
      fclose(fp);
      return false;   // ComputeIdxFingerprint() already set g_cometStatus
   }
   char szLive[16];
   snprintf(szLive, sizeof(szLive), "%08lx", liveFingerprint & 0xffffffffUL);
   if (header["SourceIdxFingerprint"] != szLive)
   {
      fclose(fp);
      return Fail(" Error - predicted-intensity file \"" + strFile + "\" was built for a different .idx: its "
         + "SourceIdxFingerprint " + header["SourceIdxFingerprint"] + " != loaded .idx fingerprint "
         + std::string(szLive) + ". Rebuild it against the current index (tools/carafe.py inten).\n");
   }
   if (std::to_string(liveNumRaw) != header["SourceIdxNumRawPeptides"])
   {
      fclose(fp);
      return Fail(" Error - predicted-intensity file \"" + strFile + "\": SourceIdxNumRawPeptides "
         + header["SourceIdxNumRawPeptides"] + " != loaded .idx raw-peptide count " + std::to_string(liveNumRaw) + ".\n");
   }
   std::string liveVarModCfg = CometPredictedMask::ComputeVarModConfigString();
   if (header["VarModConfig"] != liveVarModCfg)
   {
      fclose(fp);
      return Fail(" Error - predicted-intensity file \"" + strFile + "\" was built for a different variable-mod "
         + "configuration:\n   file:  " + header["VarModConfig"] + "\n   live:  " + liveVarModCfg
         + "\n Its modNumIdx keys are meaningless against the current comet.params; rebuild it.\n");
   }

   // --- channels: "code=name" pairs; the code is fixed per name (CHANNEL_NAMES), so a
   // file simply lists the subset it carries. Any pair this build doesn't know is an error
   // rather than a silent skip -- a code collision would misassign peaks. ---
   {
      std::string ch = header["Channels"];
      size_t start = 0;
      int nPairs = 0;
      while (start < ch.size())
      {
         size_t comma = ch.find(',', start);
         if (comma == std::string::npos) comma = ch.size();
         std::string pair = ch.substr(start, comma - start);
         start = comma + 1;
         size_t eq = pair.find('=');
         int code = -1;
         if (eq != std::string::npos && eq > 0)
         {
            try { code = std::stoi(pair.substr(0, eq)); } catch (...) { code = -1; }
         }
         std::string name = (eq == std::string::npos) ? pair : pair.substr(eq + 1);
         if (code < 0 || code >= NUM_CH || name != CHANNEL_NAMES[code] || s_bChannelPresent[code])
         {
            fclose(fp);
            return Fail(" Error - \"" + strFile + "\": Channels field \"" + ch + "\" -- entry \"" + pair
               + "\" is not one this build understands (expected code=name pairs from: 0=b_z1, 1=y_z1, "
               + "2=b_modloss_z1, 3=y_modloss_z1, 4=b_z2, 5=y_z2, 6=b_modloss_z2, 7=y_modloss_z2).\n");
         }
         s_bChannelPresent[code] = true;
         ++nPairs;
      }
      if (!s_bChannelPresent[CH_B] || !s_bChannelPresent[CH_Y])
      {
         fclose(fp);
         return Fail(" Error - \"" + strFile + "\": Channels field must include 0=b_z1 and 1=y_z1.\n");
      }
      s_bHasModloss = s_bChannelPresent[CH_B_ML] || s_bChannelPresent[CH_Y_ML]
                   || s_bChannelPresent[CH_B2_ML] || s_bChannelPresent[CH_Y2_ML];
   }

   // Which variable-mod slot the modloss channels correspond to: the first slot carrying a
   // fragment neutral loss (the phospho -98 loss in the validated datasets). The score reads
   // that slot's NL-shifted bin, uiBinnedIonMasses[..][iNlSlot + 1 + 0].
   s_iNlSlot = -1;
   if (s_bHasModloss && g_staticParams.variableModParameters.bUseFragmentNeutralLoss)
   {
      for (int i = 0; i < VMODS; ++i)
      {
         if (g_staticParams.iDbType == DbType::FI_DB && i >= FRAGINDEX_VMODS)
            break;
         if (g_staticParams.variableModParameters.varModList[i].dNeutralLoss != 0.0)
         {
            s_iNlSlot = i;
            break;
         }
      }
   }

   // --- read the entry section verbatim (64-bit offsets: the phospho-scale file is 3.5 GB,
   // past what a Windows 32-bit long can address) ---
   comet_fileoffset_t entriesStart = comet_ftell(fp);
   comet_fseek(fp, 0, SEEK_END);
   comet_fileoffset_t fileEnd = comet_ftell(fp);
   comet_fseek(fp, entriesStart, SEEK_SET);
   if (fileEnd < entriesStart)
   {
      fclose(fp);
      return Fail(" Error - \"" + strFile + "\": truncated.\n");
   }
   size_t blobSize = (size_t)(fileEnd - entriesStart);
   try
   {
      s_blob.resize(blobSize);
   }
   catch (std::bad_alloc&)
   {
      fclose(fp);
      return Fail(" Error - cannot allocate " + std::to_string(blobSize) + " bytes for predicted-intensity file \"" + strFile + "\".\n");
   }
   size_t got = 0;
   while (got < blobSize)
   {
      size_t n = fread(s_blob.data() + got, 1, std::min<size_t>(blobSize - got, (size_t)1 << 24), fp);
      if (n == 0)
         break;
      got += n;
   }
   fclose(fp);
   if (got != blobSize)
      return Fail(" Error - \"" + strFile + "\": short read (" + std::to_string(got) + " of " + std::to_string(blobSize) + " bytes).\n");

   // --- index the entries: walk once, collecting (key, offset); enforce sorted+unique ---
   std::vector<KeyOff> keys;
   try
   {
      keys.reserve((size_t)count);
   }
   catch (std::bad_alloc&)
   {
      return Fail(" Error - cannot allocate the key index for predicted-intensity file \"" + strFile + "\".\n");
   }
   uint64_t off = 0;
   uint64_t nEntries = 0;
   uint64_t nPeaksTotal = 0;
   int iPrevCharge = -1;
   while (off + ENTRY_HDR_SIZE <= blobSize)
   {
      KeyOff k;
      memcpy(&k.iWhichPeptide, s_blob.data() + off, 4);
      memcpy(&k.modNumIdx, s_blob.data() + off + 4, 4);
      k.cNtermMod = (signed char)s_blob[off + 8];
      k.cCtermMod = (signed char)s_blob[off + 9];
      k.offset = off;
      int iCharge = s_blob[off + KEY_SIZE];
      unsigned int nPeaks = s_blob[off + KEY_SIZE + 1];
      uint64_t entryLen = ENTRY_HDR_SIZE + (uint64_t)nPeaks * PEAK_SIZE;
      if (off + entryLen > blobSize)
         return Fail(" Error - \"" + strFile + "\": entry " + std::to_string(nEntries) + " runs past end of file.\n");
      // strictly increasing by (key, charge); only a key's FIRST entry goes in the index --
      // its other per-charge entries follow it consecutively and Score() walks them
      bool bSameKey = !keys.empty() && !KeyLess(keys.back(), k) && !KeyLess(k, keys.back());
      if (bSameKey)
      {
         if (iCharge <= iPrevCharge)
            return Fail(" Error - \"" + strFile + "\": entries not strictly increasing by (key, charge) at entry " + std::to_string(nEntries) + ".\n");
      }
      else
      {
         if (!keys.empty() && !KeyLess(keys.back(), k))
            return Fail(" Error - \"" + strFile + "\": entries not strictly increasing by key at entry " + std::to_string(nEntries) + ".\n");
         keys.push_back(k);
      }
      iPrevCharge = iCharge;
      nPeaksTotal += nPeaks;
      off += entryLen;
      ++nEntries;
   }
   if (off != blobSize)
      return Fail(" Error - \"" + strFile + "\": " + std::to_string(blobSize - off) + " trailing bytes after the last entry.\n");
   if (nEntries != count)
      return Fail(" Error - \"" + strFile + "\": header count " + std::to_string(count) + " != entries found " + std::to_string(nEntries) + ".\n");

   // --- bind: variant index -> entry offset ---
   size_t nVariants = variants.size();
   try
   {
      s_offsets.assign(nVariants, NO_RECORD);
   }
   catch (std::bad_alloc&)
   {
      return Fail(" Error - cannot allocate the variant binding table for predicted-intensity file \"" + strFile + "\".\n");
   }
   size_t nBound = 0;
   for (size_t v = 0; v < nVariants; ++v)
   {
      KeyOff probe;
      probe.iWhichPeptide = variants.vuiWhichPeptide[v];
      probe.modNumIdx = variants.GetModNumIdx(v);
      probe.cNtermMod = variants.GetNtermMod(v);
      probe.cCtermMod = variants.GetCtermMod(v);
      probe.offset = 0;
      std::vector<KeyOff>::const_iterator it = std::lower_bound(keys.begin(), keys.end(), probe, KeyLess);
      if (it != keys.end() && it->iWhichPeptide == probe.iWhichPeptide && it->modNumIdx == probe.modNumIdx
         && it->cNtermMod == probe.cNtermMod && it->cCtermMod == probe.cCtermMod)
      {
         s_offsets[v] = it->offset;
         ++nBound;
      }
   }
   std::vector<KeyOff>().swap(keys);

   s_bEnabled = true;
   s_missing.store(0);

   char szMsg[512];
   snprintf(szMsg, sizeof(szMsg), " Predicted-intensity file: %llu entries%s, %llu peaks; bound %zu of %zu index variants (%.2f%%)%s%s\n",
      (unsigned long long)nEntries, (s_bPerCharge ? " (per precursor charge)" : ""),
      (unsigned long long)nPeaksTotal, nBound, nVariants,
      nVariants ? 100.0 * nBound / nVariants : 0.0,
      (s_bHasModloss ? (s_iNlSlot >= 0 ? "; modloss channels active" : "; modloss channels present but no fragment NL active -- ignored") : ""),
      ((s_bChannelPresent[CH_B2] || s_bChannelPresent[CH_Y2]) ? "; z2 channels present" : "; z1 channels only"));
   logout(szMsg);
   if (nBound < nVariants)
   {
      snprintf(szMsg, sizeof(szMsg), " Warning - %zu index variants have no predicted-intensity record and will score 0.0.\n",
         nVariants - nBound);
      logout(szMsg);
   }
   return true;
}


bool CometIntensityStore::Decode(unsigned int uiVariant, const Query* pQuery, int iLenPeptide, Decoded& out)
{
   out.bValid = false;
   out.dPredNorm2 = 0.0;
   if (!s_bEnabled || uiVariant == NO_VARIANT)
      return false;
   if (uiVariant >= s_offsets.size() || s_offsets[uiVariant] == NO_RECORD)
   {
      s_missing.fetch_add(1);
      return false;
   }

   // Pick this variant's record: the file holds its entries consecutively from s_offsets
   // (one per predicted precursor charge when PerCharge, else a single charge-0 merge).
   // Exact precursor-charge match wins; else a charge-0 (merged) record; else the nearest
   // lower predicted charge; else the lowest higher one.
   const unsigned char* pBase = s_blob.data();
   const unsigned char* pEnd = pBase + s_blob.size();
   const unsigned char* pEntry = pBase + s_offsets[uiVariant];
   const unsigned char* pChosen = NULL;
   if (s_bPerCharge)
   {
      int iWantZ = pQuery->_spectrumInfoInternal.usiChargeState;
      int iBestLower = -1, iBestHigher = 1000;
      const unsigned char* pLower = NULL;
      const unsigned char* pHigher = NULL;
      const unsigned char* pMerged = NULL;
      const unsigned char* q = pEntry;
      while (q + ENTRY_HDR_SIZE <= pEnd && memcmp(q, pEntry, KEY_SIZE) == 0)
      {
         int z = q[KEY_SIZE];
         if (z == iWantZ)
         {
            pChosen = q;
            break;
         }
         if (z == 0)
            pMerged = q;
         else if (z < iWantZ && z > iBestLower)
         {
            iBestLower = z;
            pLower = q;
         }
         else if (z > iWantZ && z < iBestHigher)
         {
            iBestHigher = z;
            pHigher = q;
         }
         q += ENTRY_HDR_SIZE + (size_t)q[KEY_SIZE + 1] * PEAK_SIZE;
      }
      if (pChosen == NULL)
         pChosen = pMerged ? pMerged : (pLower ? pLower : pHigher);
      if (pChosen == NULL)
         return false;
   }
   else
      pChosen = pEntry;

   // Fragment charges this spectrum's ladder actually holds: 1..min(2, usiMaxFragCharge).
   int iMaxZ = pQuery->_spectrumInfoInternal.usiMaxFragCharge;
   if (iMaxZ > 2)
      iMaxZ = 2;

   // Dense predicted ladder, sqrt(rel) = q/255. Channels 4-7 are the z2 predictions; they
   // are skipped (and left out of |p|) when the spectrum does not score z2 fragments.
   const unsigned char* p = pChosen + KEY_SIZE + 1;   // skip key + charge byte
   unsigned int nPeaks = *p++;
   memset(out.pred, 0, sizeof(out.pred));
   int iLenMinus1 = iLenPeptide - 1;
   for (unsigned int i = 0; i < nPeaks; ++i, p += PEAK_SIZE)
   {
      unsigned int code = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
      unsigned int ch = code >> 8;
      unsigned int pos = code & 0xFF;
      float v = (float)p[2] / 255.0f;
      if (ch < NUM_CH && (int)pos < iLenMinus1 && (ch < CH_B2 || iMaxZ >= 2))
      {
         out.pred[ch][pos] = v;
         out.dPredNorm2 += (double)v * v;
      }
   }
   out.bValid = (out.dPredNorm2 > 0.0);
   return out.bValid;
}


double CometIntensityStore::GlobalXcorrValue(const Query* pQuery, int bin, bool bFlank)
{
   const int iArraySize = pQuery->_spectrumInfoInternal.iArraySize;
   const int iMax = iArraySize / SPARSE_MATRIX_SIZE;
   float** ppSp = pQuery->ppfSparseSpScoreData;
   const int iOffset = g_staticParams.iXcorrProcessingOffset;

   auto obs = [&](int b) -> double
   {
      if (b <= 0 || b >= iArraySize)
         return 0.0;
      int xx = b / SPARSE_MATRIX_SIZE;
      if (xx > iMax || ppSp[xx] == NULL)
         return 0.0;
      return ppSp[xx][b - xx * SPARSE_MATRIX_SIZE];
   };
   // o(b) minus the mean of the 2*iOffset neighbouring bins (0 past the edges, like XCorr)
   auto bgsub = [&](int b) -> double
   {
      if (b <= 0 || b >= iArraySize)
         return 0.0;
      double o = obs(b);
      double w = 0.0;
      int lo = b - iOffset, hi = b + iOffset;
      if (lo < 1) lo = 1;
      if (hi > iArraySize - 1) hi = iArraySize - 1;
      for (int bb = lo; bb <= hi; ++bb)
      {
         int xx = bb / SPARSE_MATRIX_SIZE;
         if (xx <= iMax && ppSp[xx] != NULL)
            w += ppSp[xx][bb - xx * SPARSE_MATRIX_SIZE];
      }
      return iOffset > 0 ? o - (w - o) / (2.0 * iOffset) : o;
   };

   double v = bgsub(bin);
   if (bFlank)
      v += 0.5 * (bgsub(bin - 1) + bgsub(bin + 1));
   return 0.5 * v;   // SP array is normalized to 100; XCorr's convention is 50
}


double CometIntensityStore::Score(const Decoded& d,
                                  const unsigned int uiBinnedIonMasses[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 2],
                                  int iLenPeptide,
                                  int iFoundVariableMod,
                                  const Query* pQuery,
                                  double& dScoreBg)
{
   dScoreBg = 0.0;
   if (!d.bValid)
      return 0.0;

   int iMaxZ = pQuery->_spectrumInfoInternal.usiMaxFragCharge;
   if (iMaxZ > 2)
      iMaxZ = 2;
   int iLenMinus1 = iLenPeptide - 1;

   // Observed: this candidate's b/y bins looked up in the SP-score sparse array (binned sqrt
   // intensities, max 100). |o| ranges over every ladder position with a valid bin,
   // including positions the prediction leaves at zero.
   int iMax = pQuery->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE;
   float** ppSp = pQuery->ppfSparseSpScoreData;
   bool bModloss = s_bHasModloss && s_iNlSlot >= 0 && iFoundVariableMod == 2;
   double dDot = 0.0;
   double dObsNorm2 = 0.0;
   double dShiftDot = 0.0;   // sum_i p_i * (W_i - o_i): the 2*offset shifted dot products summed
   const int iOffset = g_staticParams.iXcorrProcessingOffset;
   const int iArraySize = pQuery->_spectrumInfoInternal.iArraySize;

   // Observed value at a bin, 0 past either edge or in an unallocated sparse block.
   auto obs = [&](int b) -> double
   {
      if (b <= 0 || b >= iArraySize)
         return 0.0;
      int xx = b / SPARSE_MATRIX_SIZE;
      if (xx > iMax || ppSp[xx] == NULL)
         return 0.0;
      return ppSp[xx][b - xx * SPARSE_MATRIX_SIZE];
   };
   // Sum of observed values over [b - iOffset, b + iOffset] (the shifted-spectrum window).
   auto window = [&](int b) -> double
   {
      double w = 0.0;
      int lo = b - iOffset, hi = b + iOffset;
      if (lo < 1) lo = 1;
      if (hi > iArraySize - 1) hi = iArraySize - 1;
      for (int bb = lo; bb <= hi; ++bb)
      {
         int xx = bb / SPARSE_MATRIX_SIZE;
         if (xx <= iMax && ppSp[xx] != NULL)
            w += ppSp[xx][bb - xx * SPARSE_MATRIX_SIZE];
      }
      return w;
   };

   for (int ctCharge = 1; ctCharge <= iMaxZ; ++ctCharge)
   {
      int chOffset = (ctCharge == 2) ? CH_B2 : CH_B;   // z2 codes = z1 codes + 4
      for (int ctIonSeries = 0; ctIonSeries < g_staticParams.ionInformation.iNumIonSeriesUsed; ++ctIonSeries)
      {
         int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];
         int ch;
         if (iWhichIonSeries == ION_SERIES_B)
            ch = chOffset + CH_B;
         else if (iWhichIonSeries == ION_SERIES_Y)
            ch = chOffset + CH_Y;
         else
            continue;   // no predictions for a/c/x/z series
         int chMl = ch + CH_B_ML;   // modloss code = unshifted code + 2

         for (int ctLen = 0; ctLen < iLenMinus1; ++ctLen)
         {
            unsigned int bin = uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen][0];
            if (bin > 0)
            {
               double o = obs((int)bin);
               dDot += d.pred[ch][ctLen] * o;
               dObsNorm2 += o * o;
               if (d.pred[ch][ctLen] > 0.0f)
                  dShiftDot += d.pred[ch][ctLen] * (window((int)bin) - o);
            }
            // else: bin 0 (e.g. duplicate-collapsed) -- position contributes nothing observed

            if (bModloss)
            {
               bin = uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen][s_iNlSlot + 1];
               if (bin > 0)
               {
                  double o = obs((int)bin);
                  dDot += d.pred[chMl][ctLen] * o;
                  dObsNorm2 += o * o;
                  if (d.pred[chMl][ctLen] > 0.0f)
                     dShiftDot += d.pred[chMl][ctLen] * (window((int)bin) - o);
               }
            }
         }
      }
   }

   if (dObsNorm2 <= 0.0)
      return 0.0;
   double dNorm = std::sqrt(d.dPredNorm2 * dObsNorm2);
   double dCos = dDot / dNorm;
   if (dCos > 1.0)
      dCos = 1.0;
   // background: mean of the 2*offset nonzero shifts of the normalized spectrum
   double dMeanShift = (iOffset > 0) ? dShiftDot / (2.0 * iOffset) / dNorm : 0.0;
   dScoreBg = std::round((dCos - dMeanShift) * 10000.0) / 10000.0;
   if (dDot <= 0.0)
      return 0.0;
   return std::round(dCos * 10000.0) / 10000.0;
}
