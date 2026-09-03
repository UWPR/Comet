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

// tools/carafe_cps_to_inten.py's INTEN_FILE_MAGIC
static const char* INTEN_FILE_MAGIC = "Comet Carafe intensity v1\n";

bool CometIntensityStore::s_bEnabled = false;
bool CometIntensityStore::s_bHasModloss = false;
int  CometIntensityStore::s_iNlSlot = -1;
std::vector<unsigned char> CometIntensityStore::s_blob;
std::vector<uint64_t> CometIntensityStore::s_offsets;
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
   s_iNlSlot = -1;
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
      return Fail(" Error - \"" + strFile + "\" is not a Comet Carafe intensity v1 file (bad magic line).\n");
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
                              "Channels", "Transform", "Quant" };
   for (const char* key : required)
   {
      if (header.find(key) == header.end())
      {
         fclose(fp);
         return Fail(" Error - \"" + strFile + "\": header lacks required field \"" + std::string(key) + "\".\n");
      }
   }
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

   // --- channels: which of the file's channel codes we score ---
   {
      std::string ch = header["Channels"];
      std::vector<std::string> names;
      size_t start = 0;
      while (start <= ch.size())
      {
         size_t comma = ch.find(',', start);
         if (comma == std::string::npos) comma = ch.size();
         names.push_back(ch.substr(start, comma - start));
         start = comma + 1;
      }
      static const char* expected[NUM_CH] = { "b_z1", "y_z1", "b_modloss_z1", "y_modloss_z1" };
      // The builder writes channel codes in Channels order, so name i must be the code-i
      // channel; anything beyond the four known codes is ignored (warned once).
      for (size_t i = 0; i < names.size(); ++i)
      {
         if (i < NUM_CH)
         {
            if (names[i] != expected[i])
            {
               fclose(fp);
               return Fail(" Error - \"" + strFile + "\": Channels field \"" + ch + "\" -- channel code "
                  + std::to_string(i) + " is \"" + names[i] + "\", this build expects \"" + expected[i] + "\".\n");
            }
         }
         else
         {
            logout(" Warning - predicted-intensity file declares channel \"" + names[i] + "\" which this build does not score; ignored.\n");
         }
      }
      s_bHasModloss = names.size() >= NUM_CH;
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
   while (off + KEY_SIZE + 1 <= blobSize)
   {
      KeyOff k;
      memcpy(&k.iWhichPeptide, s_blob.data() + off, 4);
      memcpy(&k.modNumIdx, s_blob.data() + off + 4, 4);
      k.cNtermMod = (signed char)s_blob[off + 8];
      k.cCtermMod = (signed char)s_blob[off + 9];
      k.offset = off;
      unsigned int nPeaks = s_blob[off + KEY_SIZE];
      uint64_t entryLen = KEY_SIZE + 1 + (uint64_t)nPeaks * PEAK_SIZE;
      if (off + entryLen > blobSize)
         return Fail(" Error - \"" + strFile + "\": entry " + std::to_string(nEntries) + " runs past end of file.\n");
      if (!keys.empty() && !KeyLess(keys.back(), k))
         return Fail(" Error - \"" + strFile + "\": entries not strictly increasing by key at entry " + std::to_string(nEntries) + ".\n");
      keys.push_back(k);
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
   snprintf(szMsg, sizeof(szMsg), " Predicted-intensity file: %llu entries, %llu peaks; bound %zu of %zu index variants (%.2f%%)%s\n",
      (unsigned long long)nEntries, (unsigned long long)nPeaksTotal, nBound, nVariants,
      nVariants ? 100.0 * nBound / nVariants : 0.0,
      (s_bHasModloss ? (s_iNlSlot >= 0 ? "; modloss channels active" : "; modloss channels present but no fragment NL active -- ignored") : ""));
   logout(szMsg);
   if (nBound < nVariants)
   {
      snprintf(szMsg, sizeof(szMsg), " Warning - %zu index variants have no predicted-intensity record and will score 0.0.\n",
         nVariants - nBound);
      logout(szMsg);
   }
   return true;
}


double CometIntensityStore::Score(unsigned int uiVariant,
                                  const unsigned int uiBinnedIonMasses[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 2],
                                  int iLenPeptide,
                                  int iFoundVariableMod,
                                  const Query* pQuery)
{
   if (!s_bEnabled || uiVariant == NO_VARIANT)
      return 0.0;
   if (uiVariant >= s_offsets.size() || s_offsets[uiVariant] == NO_RECORD)
   {
      s_missing.fetch_add(1);
      return 0.0;
   }

   // Decode the sparse record into a dense predicted ladder, sqrt(rel) = q/255.
   const unsigned char* p = s_blob.data() + s_offsets[uiVariant] + KEY_SIZE;
   unsigned int nPeaks = *p++;
   float pred[NUM_CH][MAX_PEPTIDE_LEN];
   memset(pred, 0, sizeof(pred));
   double dPredNorm2 = 0.0;
   int iLenMinus1 = iLenPeptide - 1;
   for (unsigned int i = 0; i < nPeaks; ++i, p += PEAK_SIZE)
   {
      unsigned int code = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
      unsigned int ch = code >> 8;
      unsigned int pos = code & 0xFF;
      float v = (float)p[2] / 255.0f;
      if (ch < NUM_CH && (int)pos < iLenMinus1)
      {
         pred[ch][pos] = v;
         dPredNorm2 += (double)v * v;
      }
   }
   if (dPredNorm2 <= 0.0)
      return 0.0;

   // Observed: charge-1 b/y bins of this candidate, looked up in the SP-score sparse array
   // (binned sqrt intensities, max 100). |o| ranges over every ladder position with a
   // valid bin, including positions the prediction leaves at zero.
   int iMax = pQuery->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE;
   float** ppSp = pQuery->ppfSparseSpScoreData;
   bool bModloss = s_bHasModloss && s_iNlSlot >= 0 && iFoundVariableMod == 2;
   double dDot = 0.0;
   double dObsNorm2 = 0.0;

   for (int ctIonSeries = 0; ctIonSeries < g_staticParams.ionInformation.iNumIonSeriesUsed; ++ctIonSeries)
   {
      int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];
      int ch, chMl;
      if (iWhichIonSeries == ION_SERIES_B)
      {
         ch = CH_B;
         chMl = CH_B_ML;
      }
      else if (iWhichIonSeries == ION_SERIES_Y)
      {
         ch = CH_Y;
         chMl = CH_Y_ML;
      }
      else
         continue;   // no predictions for a/c/x/z series

      for (int ctLen = 0; ctLen < iLenMinus1; ++ctLen)
      {
         unsigned int bin = uiBinnedIonMasses[1][ctIonSeries][ctLen][0];
         int x = (int)(bin / SPARSE_MATRIX_SIZE);
         if (bin > 0 && x <= iMax && ppSp[x] != NULL)
         {
            double o = ppSp[x][bin - x * SPARSE_MATRIX_SIZE];
            dDot += pred[ch][ctLen] * o;
            dObsNorm2 += o * o;
         }
         // else: bin 0 (e.g. duplicate-collapsed) -- position contributes nothing observed

         if (bModloss)
         {
            bin = uiBinnedIonMasses[1][ctIonSeries][ctLen][s_iNlSlot + 1];
            x = (int)(bin / SPARSE_MATRIX_SIZE);
            if (bin > 0 && x <= iMax && ppSp[x] != NULL)
            {
               double o = ppSp[x][bin - x * SPARSE_MATRIX_SIZE];
               dDot += pred[chMl][ctLen] * o;
               dObsNorm2 += o * o;
            }
         }
      }
   }

   if (dObsNorm2 <= 0.0 || dDot <= 0.0)
      return 0.0;
   double dCos = dDot / std::sqrt(dPredNorm2 * dObsNorm2);
   if (dCos > 1.0)
      dCos = 1.0;
   return std::round(dCos * 10000.0) / 10000.0;
}
