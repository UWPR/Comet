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


#include "CometPredictedMask.h"
#include "zlib.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>

extern comet_fileoffset_t clSizeCometFileOffset;

// Bumped independently of tools/carafe_ms2_to_fi_mask.py's own Phase 2a->2c v1->v2 bump
// (bModlossMask/yModlossMask fields) -- v3 additionally switched the fingerprint algorithm
// from SHA-256 to CRC-32 (see CometPredictedMask.h's ComputeIdxFingerprint() doc comment), a
// second breaking change to the header's SourceIdxFingerprint field's meaning. A stale v1/v2
// mask file (or a v3 mask read by a stale reader) fails this magic check cleanly rather than
// misinterpreting the fingerprint or the binary entry layout.
static const char* MASK_FILE_MAGIC = "Comet Carafe FI mask v3\n";

std::vector<CometPredictedMask::Entry> CometPredictedMask::s_entries;
bool CometPredictedMask::s_bEnabled = false;
std::vector<uint64_t> CometPredictedMask::s_cache;
int CometPredictedMask::s_cacheStride = 0;

// Entry (CometPredictedMask.h) is declared #pragma pack(1) specifically so this holds --
// byte-for-byte matching tools/carafe_ms2_to_fi_mask.py's ENTRY_FMT = "<IibbQQQQ" (little-
// endian, unpadded). Comet only builds/runs on little-endian platforms (x86/x64/ARM64 all
// little-endian in practice here), so Load() below can fread() the file straight into
// s_entries with no byte-swap and no separate staging/conversion pass -- matches every other
// fixed-width .idx section read elsewhere in this codebase (CometPeptideIndex.cpp's raw
// peptide table etc.), none of which byte-swap either. (The static_assert on this lives in
// the header, right after Entry's definition -- Entry is private, so it can't be named from
// free-standing code out here.)


bool CometPredictedMask::EntryKeyLess(const Entry& a, const Entry& b)
{
   if (a.iWhichPeptide != b.iWhichPeptide) return a.iWhichPeptide < b.iWhichPeptide;
   if (a.modNumIdx != b.modNumIdx) return a.modNumIdx < b.modNumIdx;
   if (a.cNtermMod != b.cNtermMod) return a.cNtermMod < b.cNtermMod;
   return a.cCtermMod < b.cCtermMod;
}


bool CometPredictedMask::ComputeIdxFingerprint(unsigned long& fingerprint, uint64_t& numRawPeptides)
{
   FILE* fp = fopen(g_staticParams.databaseInfo.szDatabase, "rb");
   if (fp == NULL)
   {
      string strErrorMsg = " Error - cannot open \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" to fingerprint for predicted-mask validation.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Mirrors CometPeptideIndex::ReadPeptideIndex()'s own magic/footer read exactly -- kept as
   // an independent, minimal re-read here (not routed through ReadPeptideIndex(), which
   // doesn't keep the file open or expose these offsets afterward) rather than threading new
   // output parameters through that already-large function for a single caller. Version
   // pinned to the exact current format (v4, docs/20260811_restore_idx_header_mods.md), same
   // policy ParsePeptideIndexHeader() uses -- keep both in sync on any future header bump.
   char szMagic[32] = { 0 };
   if (fread(szMagic, 1, sizeof(szMagic) - 1, fp) == 0
      || strncmp(szMagic, "Comet index database v4", strlen("Comet index database v4")) != 0)
   {
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" is not a v4 unified .idx file; cannot fingerprint for predicted-mask validation.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   comet_fseek(fp, 0, SEEK_END);
   comet_fileoffset_t clFileSize = comet_ftell(fp);
   comet_fseek(fp, -2 * (comet_fileoffset_t)clSizeCometFileOffset, SEEK_END);

   comet_fileoffset_t clPeptidesFilePos = 0, clProteinsFilePos = 0;
   (void)fread(&clPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   (void)fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);
   comet_fileoffset_t clFooterPos = clFileSize - 2 * (comet_fileoffset_t)clSizeCometFileOffset;

   if (clFileSize < 2 * (comet_fileoffset_t)clSizeCometFileOffset
      || clPeptidesFilePos < 0
      || clPeptidesFilePos >= clProteinsFilePos
      || clProteinsFilePos >= clFooterPos)
   {
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" has corrupt or out-of-order section offsets in its footer; cannot fingerprint.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   comet_fseek(fp, clPeptidesFilePos, SEEK_SET);
   (void)fread(&numRawPeptides, sizeof(uint64_t), 1, fp);
   comet_fseek(fp, clPeptidesFilePos, SEEK_SET);

   uLong crc = crc32(0L, Z_NULL, 0);
   comet_fileoffset_t remaining = clFooterPos - clPeptidesFilePos;
   // Not named CHUNK -- MSToolkit/include/mzParser.h (pulled in transitively via Common.h)
   // #defines that identifier (no include guard around it), which would otherwise silently
   // rewrite this declaration.
   const size_t FINGERPRINT_CHUNK_SIZE = 1 << 20;
   std::vector<unsigned char> buf(FINGERPRINT_CHUNK_SIZE);
   while (remaining > 0)
   {
      size_t n = (size_t)std::min<comet_fileoffset_t>((comet_fileoffset_t)FINGERPRINT_CHUNK_SIZE, remaining);
      size_t nread = fread(buf.data(), 1, n, fp);
      if (nread == 0)
         break;
      crc = crc32(crc, buf.data(), (uInt)nread);
      remaining -= (comet_fileoffset_t)nread;
   }
   fclose(fp);

   fingerprint = (unsigned long)crc;
   return true;
}


bool CometPredictedMask::IsEnabled()
{
   return s_bEnabled;
}


void CometPredictedMask::FreeAfterIndexBuild()
{
   std::vector<Entry>().swap(s_entries);
}


void CometPredictedMask::ReserveCache(size_t numVariants, bool bIncludeModloss)
{
   // 4 uint64_t/entry when this search can ever consult bModlossMask/yModlossMask (some
   // active variable mod has a neutral-loss delta), 2 when it can't (docs/20260822_
   // carafe_prerun.md Section 7.6.2) -- StoreCached()/LookupCached() below key off the same
   // s_cacheStride to know which layout is in effect.
   s_cacheStride = bIncludeModloss ? 4 : 2;

   // Default ~0ULL ("all bits set"), not value-initialized 0 -- matches Lookup()'s own
   // "not found -> fully unfiltered" default (AddFragments()'s maskB/maskY/... locals start
   // the same way). Every index gets overwritten by the parallel StoreCached() pass that
   // follows in GenerateFragmentIndex(), covering the full [0, numVariants) range with no
   // gaps -- this default only matters as a safety net if that invariant is ever violated.
   s_cache.assign(numVariants * (size_t)s_cacheStride, ~0ULL);
}


void CometPredictedMask::StoreCached(size_t iWhichFragmentPeptide, uint64_t bMask, uint64_t yMask,
                                     uint64_t bModlossMask, uint64_t yModlossMask)
{
   uint64_t* p = &s_cache[iWhichFragmentPeptide * (size_t)s_cacheStride];
   p[0] = bMask;
   p[1] = yMask;
   if (s_cacheStride == 4)
   {
      p[2] = bModlossMask;
      p[3] = yModlossMask;
   }
   // else: bModlossMask/yModlossMask discarded -- the GenerateFragmentIndex() caller only
   // ever computes non-default values for these when bFragmentNL is active search-wide, which
   // is exactly when ReserveCache() was told bIncludeModloss == true.
}


void CometPredictedMask::LookupCached(size_t iWhichFragmentPeptide, uint64_t& bMask, uint64_t& yMask,
                                      uint64_t& bModlossMask, uint64_t& yModlossMask)
{
   const uint64_t* p = &s_cache[iWhichFragmentPeptide * (size_t)s_cacheStride];
   bMask = p[0];
   yMask = p[1];
   if (s_cacheStride == 4)
   {
      bModlossMask = p[2];
      yModlossMask = p[3];
   }
   else
   {
      // Never read by AddFragments() when bFragmentNL is false search-wide (the only case
      // s_cacheStride == 2 occurs), but default to fully-unfiltered anyway, matching every
      // other "not applicable here" convention in this class rather than leaving the
      // out-params unset.
      bModlossMask = ~0ULL;
      yModlossMask = ~0ULL;
   }
}


std::string CometPredictedMask::ComputeVarModConfigString()
{
   // Matches CometFragmentIndex::PermuteIndexPeptideMods()'s own "for (int i = 0; i <
   // FRAGINDEX_VMODS; ++i)" scope exactly -- only these slots participate in FI builds at all,
   // so slots beyond FRAGINDEX_VMODS are irrelevant to whether this mask's modNumIdx keys
   // still mean the same thing.
   std::ostringstream oss;
   oss << std::fixed << std::setprecision(6);
   for (int i = 0; i < FRAGINDEX_VMODS; ++i)
   {
      const VarMods& vm = g_staticParams.variableModParameters.varModList[i];
      if (i > 0)
         oss << "|";
      oss << vm.dVarModMass << vm.szVarModChar
          << (vm.bNtermMod ? 'n' : '-') << (vm.bCtermMod ? 'c' : '-')
          << vm.dNeutralLoss;
   }
   return oss.str();
}


bool CometPredictedMask::Load(const std::string& strMaskFile)
{
   // Process-lifetime guard, matching CometPeptideIndex::ReadPeptideIndex()'s own
   // g_bPeptideIndexRead pattern (docs/20260805_carafe.md; see that function's "process-
   // lifetime, not session-lifetime" note) -- CreateFragmentIndex() calls this unconditionally
   // every time it runs, but the actual load only needs to happen once per process. Like that
   // precedent, a second call after a FAILED first attempt would return true (silently
   // "succeeding" as disabled) rather than re-reporting the original error -- unreachable
   // today since every current caller (batch comet.exe, RTS) calls CreateFragmentIndex() at
   // most once per process and aborts immediately on a false return either way; not a
   // real gap unless that changes.
   static bool s_bLoadAttempted = false;
   if (s_bLoadAttempted)
      return true;
   s_bLoadAttempted = true;

   s_bEnabled = false;
   s_entries.clear();
   std::vector<Entry>().swap(s_entries);

   if (strMaskFile.empty())
      return true;   // masking disabled -- not an error

   FILE* fp = fopen(strMaskFile.c_str(), "rb");
   if (fp == NULL)
   {
      string strErrorMsg = " Error - cannot open predicted-fragment mask file \"" + strMaskFile + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   size_t magicLen = strlen(MASK_FILE_MAGIC);
   std::vector<char> magicBuf(magicLen);
   if (fread(magicBuf.data(), 1, magicLen, fp) != magicLen
      || strncmp(magicBuf.data(), MASK_FILE_MAGIC, magicLen) != 0)
   {
      string strErrorMsg = " Error - \"" + strMaskFile + "\" is not a \""
         + string(MASK_FILE_MAGIC).substr(0, magicLen - 1) + "\" mask file (wrong version, or "
         + "not a mask file at all -- rebuild it with the current tools/carafe_ms2_to_fi_mask.py).\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   // Header: "Key: value\n" lines, terminated by a blank line -- mirrors
   // tools/carafe_ms2_to_fi_mask.py's write_mask_file()/read_mask_file() exactly.
   std::map<std::string, std::string> header;
   char lineBuf[1024];
   while (fgets(lineBuf, sizeof(lineBuf), fp) != NULL)
   {
      std::string line(lineBuf);
      while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
         line.pop_back();
      if (line.empty())
         break;
      size_t colon = line.find(": ");
      if (colon == std::string::npos)
         continue;
      header[line.substr(0, colon)] = line.substr(colon + 2);
   }

   auto itFp = header.find("SourceIdxFingerprint");
   auto itNumRaw = header.find("SourceIdxNumRawPeptides");
   auto itVarModCfg = header.find("VarModConfig");
   if (itFp == header.end() || itNumRaw == header.end() || itVarModCfg == header.end())
   {
      string strErrorMsg = " Error - \"" + strMaskFile + "\" is missing required header field(s) "
         + "(SourceIdxFingerprint/SourceIdxNumRawPeptides/VarModConfig) -- not a valid mask file, "
         + "or built with an older tools/carafe_ms2_to_fi_mask.py that predates the VarModConfig "
         + "guard (docs/20260805_carafe.md Section 8 items 12-14) -- rebuild it.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   // Closes the gap the .idx fingerprint alone leaves open (Section 6.10's closing note):
   // modNumIdx numbering depends on live comet.params' variable mods, not just the .idx, since
   // Phase 0.5 stopped persisting them on disk. A mask built against different variable mods
   // than are live right now would otherwise pass the fingerprint check while carrying
   // modNumIdx keys that mean something else entirely -- rejected here, loudly, rather than
   // relying solely on Lookup()'s "not found -> fully unfiltered" fallback to paper over it
   // (that fallback only helps when a stale key happens not to collide with a real one).
   std::string liveVarModCfg = ComputeVarModConfigString();
   if (itVarModCfg->second != liveVarModCfg)
   {
      string strErrorMsg = " Error - predicted-fragment mask file \"" + strMaskFile
         + "\" was built against different variable mods than are live in the current search "
         + "parameters (mask: \"" + itVarModCfg->second + "\", live: \"" + liveVarModCfg + "\") -- "
         + "modNumIdx numbering is not comparable across different variable-mod configurations "
         + "(docs/20260805_carafe.md Section 8 items 12-14). Rebuild the mask with `comet.exe -x` and "
         + "tools/carafe_ms2_to_fi_mask.py using the SAME comet.params as this search.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   unsigned long fingerprint = 0;
   uint64_t numRawPeptides = 0;
   if (!ComputeIdxFingerprint(fingerprint, numRawPeptides))
   {
      fclose(fp);
      return false;
   }

   char szFingerprintHex[16];
   snprintf(szFingerprintHex, sizeof(szFingerprintHex), "%08lx", fingerprint);

   uint64_t maskNumRawPeptides = 0;
   try
   {
      maskNumRawPeptides = std::stoull(itNumRaw->second);
   }
   catch (const std::exception&)
   {
      string strErrorMsg = " Error - \"" + strMaskFile + "\" has a malformed SourceIdxNumRawPeptides header value \""
         + itNumRaw->second + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   // Two independent checks (fingerprint over the actual bytes, plus the cheap human-readable
   // count) -- matches tools/carafe_ms2_to_fi_mask.py's idx_fingerprint()'s own "hash +
   // NumRawPeptides" pairing (Section 4.3). Proves iWhichPeptide numbering is safe; the
   // VarModConfig check above (which proves modNumIdx numbering is safe) is what closes
   // Section 6.10's closing note -- both are required, neither alone is sufficient.
   if (itFp->second != szFingerprintHex || maskNumRawPeptides != numRawPeptides)
   {
      string strErrorMsg = " Error - predicted-fragment mask file \"" + strMaskFile
         + "\" does not match the currently-loaded .idx (fingerprint " + itFp->second + " vs computed "
         + szFingerprintHex + "; NumRawPeptides " + itNumRaw->second + " vs computed "
         + std::to_string(numRawPeptides) + ") -- rebuild the mask against this exact .idx "
         + "(tools/carafe_ms2_to_fi_mask.py).\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   uint64_t count = 0;
   if (fread(&count, sizeof(uint64_t), 1, fp) != 1)
   {
      string strErrorMsg = " Error - \"" + strMaskFile + "\" truncated (missing entry count).\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   // Read directly into s_entries -- Entry is #pragma pack(1) and byte-for-byte matches the
   // on-disk record, so no separate staging buffer/field-by-field conversion pass is needed
   // (previously: a same-size second buffer, momentarily co-resident with s_entries here --
   // see the header's comment above Entry's definition).
   s_entries.resize((size_t)count);
   if (count > 0 && fread(s_entries.data(), sizeof(Entry), (size_t)count, fp) != (size_t)count)
   {
      string strErrorMsg = " Error - \"" + strMaskFile + "\" truncated (expected "
         + std::to_string(count) + " entries).\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      std::vector<Entry>().swap(s_entries);
      return false;
   }
   fclose(fp);

   // write_mask_file() already writes entries pre-sorted (Section 4.3), but don't rely on that
   // invariant holding for a hand-edited or future-tool-generated file -- Lookup()'s binary
   // search requires it, so (re-)establish it unconditionally rather than trust the file.
   std::sort(s_entries.begin(), s_entries.end(), EntryKeyLess);

   s_bEnabled = true;
   logout("   - loaded " + std::to_string(s_entries.size())
      + " predicted-fragment mask entries from \"" + strMaskFile + "\"\n");
   return true;
}


bool CometPredictedMask::Lookup(unsigned int iWhichPeptide, int modNumIdx, signed char cNtermMod, signed char cCtermMod,
                                uint64_t& bMask, uint64_t& yMask, uint64_t& bModlossMask, uint64_t& yModlossMask)
{
   Entry key;
   key.iWhichPeptide = iWhichPeptide;
   key.modNumIdx = modNumIdx;
   key.cNtermMod = cNtermMod;
   key.cCtermMod = cCtermMod;

   auto it = std::lower_bound(s_entries.begin(), s_entries.end(), key, EntryKeyLess);

   if (it == s_entries.end() || it->iWhichPeptide != iWhichPeptide || it->modNumIdx != modNumIdx
      || it->cNtermMod != cNtermMod || it->cCtermMod != cCtermMod)
      return false;

   bMask = it->bMask;
   yMask = it->yMask;
   bModlossMask = it->bModlossMask;
   yModlossMask = it->yModlossMask;
   return true;
}
