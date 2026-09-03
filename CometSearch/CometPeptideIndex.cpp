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


#include "CometPeptideIndex.h"
#include "CometPredictedMask.h"
#include "CometIntensityStore.h"

// For GenerateVariantArray()'s page-granular staging buffer (AllocStagingPages() et al.):
// mmap/madvise/munmap on POSIX; Windows uses plain malloc/free (see AllocStagingPages()'s
// comment for why NOT VirtualAlloc/VirtualFree there).
#ifndef _WIN32
#include <sys/mman.h>
#endif

extern comet_fileoffset_t clSizeCometFileOffset;


CometPeptideIndex::CometPeptideIndex()
{
}

CometPeptideIndex::~CometPeptideIndex()
{
}

// Read the unified index (.idx) file into global read-only structures, shared by both
// PI_DB and FI_DB search modes (docs/20260730_PI_reduction.md Phase 0/0.5):
//   g_vRawPeptides         - one entry per unique unmodified peptide (sequence, protein
//                            reference, flank AAs, unmodified mass); shared across every
//                            mod-variant of that peptide. The only peptide-level data
//                            persisted on disk -- everything below is generated fresh in
//                            memory every session, never written to the .idx file.
//   MOD_SEQS_POOL/MOD_NUMBERS_POOL/MOD_SEQ_MOD_NUM_START/CNT/POOL_START/PEPTIDE_MOD_SEQ_IDXS -
//                            the mod-permutation tables (flat-pooled, see core/Types.h and
//                            docs/20260827_PI_memory.md Phase 1), rebuilt once per session by
//                            CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides)
//                            from whichever variable mods are active in comet.params at
//                            that moment (Phase 0.5 -- previously persisted in the .idx
//                            file and read back as-is; see the Phase 0.5 section of
//                            docs/20260730_PI_reduction.md for why that changed).
//   g_dbIndexVariants      - one compact entry per (peptide, mod combination) pair (a
//                            fixed-point mass key plus a reference back into
//                            g_vRawPeptides; VariantArray SoA, core/Types.h), sorted by
//                            mass. Built
//                            once per session by GenerateVariantArray() below, PI_DB mode
//                            only. A full DBIndex is materialized on demand from one of
//                            these entries via CometPeptideIndex::MaterializeOneEntry(),
//                            called once per mass-window candidate from
//                            CometSearch::SearchPeptideIndex(). FI_DB mode leaves this
//                            empty -- it builds its own posting list from
//                            MOD_SEQS_POOL/MOD_NUMBERS_POOL/etc. above via
//                            CometFragmentIndex::GenerateFragmentIndex(), unchanged by
//                            this unification.
//   g_pvProteinsList       - vector-of-vectors mapping peptide to protein file positions
//   g_pvProteinNames       - map of file offset to protein name string
//   g_bPeptideIndexRead / g_bPlainPeptideIndexRead - guard flags (both set; PI_DB code checks
//                            the former, FI_DB code the latter -- see CometFragmentIndex::
//                            CreateFragmentIndex())
//
// The .idx binary layout (written by WritePeptideIndex()):
//   [text header lines ending with blank line -- MassType/StaticMod/DecoySearch/Enzyme/
//    Enzyme2 only; variable-mod settings are not persisted, see Phase 0.5 above]
//   [protein names: each WIDTH_REFERENCE chars -- length/count not stored; only ever
//    addressed via the file offsets embedded in the proteins list, never iterated]
//   [raw peptide table @ clPeptidesFilePos: count(uint64_t), then per-entry: iLen(int),
//    szPeptide(iLen chars), cPrevAA(char), cNextAA(char), dPepMass(double),
//    siVarModProteinFilter(unsigned short), lIndexProteinFilePosition(comet_fileoffset_t)]
//   [proteins list @ clProteinsFilePos: count then per-entry (size + file offsets)]
//   [footer: clPeptidesFilePos, clProteinsFilePos (2 x comet_fileoffset_t) -- each
//    section's read size is derived from the distance to the next pointer (or to the
//    footer itself, for the last section)]
//
// Old-format files (the pre-Phase-0.5 unified format, the pre-unification PI_DB-only v2
// format, or FI_DB's separate pre-unification plain-index format) are rejected by the
// header check below with a clear rebuild message rather than being misread.
bool CometPeptideIndex::ReadPeptideIndex(bool bIsRTS, bool bForceExportMode)
{
   (void)bIsRTS;   // reserved for RTS-vs-batch-specific behavior; not yet used

   if (g_bPeptideIndexRead)
      return true;

   FILE* fp;

   if ((fp = fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
   {
      string strErrorMsg = " Error - cannot open index file \""
         + string(g_staticParams.databaseInfo.szDatabase) + "\" for reading.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
   setvbuf(fp, NULL, _IOFBF, 32 * 1024 * 1024);

   // Parse the text header (magic/version, IndexSearchType, MassType, StaticMod,
   // VariableMod, ProteinModList, RequireVariableMod, DecoySearch, Enzyme, Enzyme2) into
   // g_staticParams -- including which mode (PI_DB/FI_DB) this file was built for.
   // ParsePeptideIndexHeader() does its own rewind()/read of fp starting from byte 0 and
   // validates the magic string/version itself, so no separate pre-check is needed here.
   if (!ParsePeptideIndexHeader(fp))
   {
      fclose(fp);
      return false;
   }

   // See the bForceExportMode doc comment in CometPeptideIndex.h: ParsePeptideIndexHeader()
   // just unconditionally set g_staticParams.iDbType from this file's own IndexSearchType:
   // line, undoing ExportPeptideIndexVariants()'s explicit PI_DB force for an FI_DB-tagged
   // .idx. Re-apply it here so the "iDbType == PI_DB" gate below (and ExportVariants()'s own
   // check) see what the caller actually asked for.
   if (bForceExportMode)
      g_staticParams.iDbType = DbType::PI_DB;

   // The protein-name section starts immediately after the text header's terminating blank
   // line, i.e. exactly where ParsePeptideIndexHeader() left the stream: one WIDTH_REFERENCE-
   // sized block per protein, back-to-back. Phase 4 (docs/20260827_PI_memory.md) stores
   // name-section ORDINALS, not file offsets, in g_pvProteinsList, so the load below needs
   // this base (and the section's block count) to translate and validate the on-disk
   // offsets. The on-disk format itself is unchanged.
   comet_fileoffset_t clNamesBase = comet_ftell(fp);

   // --- Read the two-pointer footer at true EOF ---
   comet_fseek(fp, 0, SEEK_END);
   comet_fileoffset_t clFileSize = comet_ftell(fp);
   comet_fseek(fp, -2 * (comet_fileoffset_t)clSizeCometFileOffset, SEEK_END);

   comet_fileoffset_t clPeptidesFilePos, clProteinsFilePos;
   (void)fread(&clPeptidesFilePos, clSizeCometFileOffset, 1, fp);
   (void)fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);
   comet_fileoffset_t clFooterPos = clFileSize - 2 * (comet_fileoffset_t)clSizeCometFileOffset;

   // Harden against a truncated/corrupt .idx: every section-size computation below is a
   // subtraction between two of these footer offsets (or between one of them and
   // clFooterPos), each immediately used to size a vector<char> or as a fread() count.
   // An out-of-order or out-of-range offset -- from a file truncated mid-write, or simply
   // corrupt -- would underflow that subtraction (all unsigned size_t/comet_fileoffset_t
   // types) into a huge value, triggering a multi-exabyte allocation attempt rather than a
   // clean error. Requiring strict monotonic ordering within [0, clFooterPos] up front makes
   // every later subtraction between these values provably safe without re-checking each
   // one individually.
   // clPeptidesFilePos must be at least sizeof(uint64_t) bytes before clProteinsFilePos, not
   // just "before" it: the raw-peptide section-size computation below subtracts
   // sizeof(uint64_t) (its own tNumRaw count field) from (clProteinsFilePos -
   // clPeptidesFilePos), and a gap of 1-7 bytes -- still "before" under a plain >= check --
   // would underflow that subtraction to a near-SIZE_MAX section size.
   if (clFileSize < 2 * (comet_fileoffset_t)clSizeCometFileOffset
      || clPeptidesFilePos < 0
      || clPeptidesFilePos + (comet_fileoffset_t)sizeof(uint64_t) > clProteinsFilePos
      || clProteinsFilePos >= clFooterPos)
   {
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" has corrupt or out-of-order section offsets in its footer; the file is "
         + "likely truncated or corrupt. Rebuild it with -i or -j.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   // --- Raw peptide table: read the whole section in one buffered read, then parse from
   // memory (avoids tNumRaw individual small fread() calls). ---
   comet_fseek(fp, clPeptidesFilePos, SEEK_SET);
   uint64_t tNumRaw;
   if (fread(&tNumRaw, sizeof(uint64_t), 1, fp) != 1)
   {
      fclose(fp);
      logout(" Error - failed to read raw peptide count from .idx file; file may be truncated or corrupt.\n");
      return false;
   }

   size_t tSectionSize = (size_t)(clProteinsFilePos - clPeptidesFilePos) - sizeof(uint64_t);

   // Sanity-bound tNumRaw against the section it has to fit in before it ever reaches
   // reserve()/the parse loop below: the smallest an on-disk entry can possibly be (an
   // empty, iLen==0 peptide) bounds how large tNumRaw could legitimately be for a section
   // of this size. A corrupt/garbage count otherwise reaches reserve() directly and throws
   // std::length_error/bad_alloc instead of the clean error below.
   const size_t tMinEntrySize = sizeof(int) + 2 + sizeof(double) + sizeof(unsigned short) + (size_t)clSizeCometFileOffset;
   if (tNumRaw > tSectionSize / tMinEntrySize)
   {
      fclose(fp);
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" has an implausible raw peptide count in its .idx file; the file is likely "
         + "truncated or corrupt. Rebuild it with -i or -j.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Streamed read of the raw-peptide section (docs/20260827_PI_memory.md follow-up to
   // Phase 4): a fixed carry buffer replaces the former whole-section vBuf, which at
   // MHC scale held ~8 GB alongside the growing table and -- with the pool over-reserve's
   // shrink_to_fit() copy on top -- WAS the process's peak-RSS moment. The on-disk fixed
   // overhead is exactly 24 B/entry (iLen int + 2 flanks + double + ushort + 8-byte protein
   // reference) and the pool stores (iLen + 1) bytes per entry, so the pool's final size is
   // computable up front: tSectionSize - 23 * tNumRaw. Reserving exactly makes the trailing
   // shrink_to_fit() a no-op instead of a multi-GB copy.
   const size_t tFixedDiskBytesPerEntry = sizeof(int) + 2 + sizeof(double)
      + sizeof(unsigned short) + (size_t)clSizeCometFileOffset;   // == tMinEntrySize above
   size_t tSeqPoolBytes = tSectionSize - (size_t)tNumRaw * (tFixedDiskBytesPerEntry - 1);

   g_vRawPeptides.clear();
   g_vRawPeptides.reserve((size_t)tNumRaw, tSeqPoolBytes);
   {
      // largest possible entry: fixed fields + a (MAX_PEPTIDE_LEN - 1)-residue sequence
      const size_t tMaxEntryBytes = tFixedDiskBytesPerEntry + MAX_PEPTIDE_LEN;
      const size_t tBufCap = 16 * 1024 * 1024;
      vector<char> vBuf(tBufCap);
      size_t tHave = 0;                          // valid bytes currently in vBuf
      size_t tPos = 0;                           // parse cursor within vBuf
      uint64_t tFileRemaining = tSectionSize;    // section bytes not yet read from disk

      for (uint64_t i = 0; i < tNumRaw; ++i)
      {
         // refill when the window can no longer be guaranteed to hold one whole entry
         if (tHave - tPos < tMaxEntryBytes && tFileRemaining > 0)
         {
            memmove(vBuf.data(), vBuf.data() + tPos, tHave - tPos);
            tHave -= tPos;
            tPos = 0;
            size_t tToRead = tBufCap - tHave;
            if ((uint64_t)tToRead > tFileRemaining)
               tToRead = (size_t)tFileRemaining;
            if (fread(vBuf.data() + tHave, 1, tToRead, fp) != tToRead)
            {
               fclose(fp);
               logout(" Error - failed to read raw peptide table from .idx file; file may be truncated or corrupt.\n");
               return false;
            }
            tHave += tToRead;
            tFileRemaining -= tToRead;
         }

         int iLen;

         if (tHave - tPos < sizeof(int))
         {
            fclose(fp);
            logout(" Error - raw peptide table ran short of its section in .idx file at entry " + to_string(i) + ".\n");
            return false;
         }
         memcpy(&iLen, vBuf.data() + tPos, sizeof(int)); tPos += sizeof(int);
         if (iLen < 0 || iLen >= MAX_PEPTIDE_LEN)
         {
            fclose(fp);
            logout(" Error - corrupt raw peptide entry " + to_string(i) + " in .idx file.\n");
            return false;
         }
         // Every fixed-size field this entry still needs, checked in one shot: the peptide
         // sequence itself (iLen bytes) plus cPrevAA/cNextAA/dPepMass/siVarModProteinFilter/
         // lIndexProteinFilePosition. tNumRaw not matching what the section actually holds
         // (a corrupt/truncated file) would otherwise walk the cursor past the window -- a
         // heap-buffer-overread -- instead of erroring cleanly here.
         if (tHave - tPos < (size_t)iLen + 2 + sizeof(double) + sizeof(unsigned short) + (size_t)clSizeCometFileOffset)
         {
            fclose(fp);
            logout(" Error - raw peptide table ran short of its section in .idx file at entry " + to_string(i) + ".\n");
            return false;
         }
         const char* pSeq = vBuf.data() + tPos; tPos += iLen;
         char cPrevAA = vBuf[tPos++];
         char cNextAA = vBuf[tPos++];
         double dPepMass;
         unsigned short siVarModProteinFilter;
         comet_fileoffset_t lIndexProteinFilePosition;
         memcpy(&dPepMass, vBuf.data() + tPos, sizeof(double)); tPos += sizeof(double);
         memcpy(&siVarModProteinFilter, vBuf.data() + tPos, sizeof(unsigned short)); tPos += sizeof(unsigned short);
         memcpy(&lIndexProteinFilePosition, vBuf.data() + tPos, clSizeCometFileOffset); tPos += (size_t)clSizeCometFileOffset;
         if (!g_vRawPeptides.push_back(pSeq, iLen, cPrevAA, cNextAA, dPepMass,
               siVarModProteinFilter, lIndexProteinFilePosition))
         {
            // push_back only refuses an out-of-range protein row index (see
            // RawPeptideTable's class comment) -- possible only for a corrupt file.
            fclose(fp);
            logout(" Error - corrupt protein reference in raw peptide entry " + to_string(i) + " in .idx file.\n");
            return false;
         }
      }
      g_vRawPeptides.shrink_to_fit();   // no-op when the exact pool estimate held
   }

   // --- Proteins list (ProteinsListCSR) ---
   comet_fseek(fp, clProteinsFilePos, SEEK_SET);
   size_t tNumProteinEntries;
   if (fread(&tNumProteinEntries, clSizeCometFileOffset, 1, fp) != 1)
   {
      fclose(fp);
      logout(" Error - failed to read protein-entry count from .idx file; file may be truncated or corrupt.\n");
      return false;
   }

   // Every count/offset in this section is bounded by the total bytes actually available
   // (clFooterPos - clProteinsFilePos, already validated above): the whole section -- the
   // tNumProteinEntries count itself, then one tNumProteins count plus that many
   // clSizeCometFileOffset-sized entries per iteration -- has to fit in that many bytes, so
   // no single count read from the file can legitimately need more than
   // tMaxProteinSectionEntries clSizeCometFileOffset-sized slots. A corrupt/garbage count
   // otherwise reaches reserve()/resize() directly and throws std::length_error/bad_alloc
   // instead of the clean error below.
   const size_t tMaxProteinSectionEntries = (size_t)(clFooterPos - clProteinsFilePos) / (size_t)clSizeCometFileOffset;
   if (tNumProteinEntries > tMaxProteinSectionEntries)
   {
      fclose(fp);
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" has an implausible protein-entry count in its .idx file; the file is likely "
         + "truncated or corrupt. Rebuild it with -i or -j.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // The name section spans [clNamesBase, clPeptidesFilePos) in fixed WIDTH_REFERENCE
   // blocks; its block count both sizes the sequential name-cache read below and bounds
   // the ordinal validation here.
   if (clPeptidesFilePos < clNamesBase
      || ((clPeptidesFilePos - clNamesBase) % (comet_fileoffset_t)WIDTH_REFERENCE) != 0)
   {
      fclose(fp);
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" has a malformed protein-name section in its .idx file; the file is likely "
         + "truncated or corrupt. Rebuild it with -i or -j.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
   const uint64_t tNumProteinsInFile = (uint64_t)(clPeptidesFilePos - clNamesBase) / WIDTH_REFERENCE;

   // Read directly into flat CSR staging buffers instead of one throwaway vector per row --
   // avoids tNumProteinEntries individual heap allocations, the same per-row allocation
   // cost append_flat() was built to eliminate on the build side (see its comment in
   // core/Types.h). The on-disk values are 8-byte name-section file offsets (format
   // unchanged); each is translated to its name-section ORDINAL here -- Phase 4
   // (docs/20260827_PI_memory.md) halves the resident structure by storing uint32 ordinals,
   // and the stride/range validation below is a stronger corruption check than the old
   // min/max bound on raw offsets.
   {
      vector<unsigned int> vFlatProteinOrdinals;
      vector<uint32_t> vProteinCounts;
      vector<comet_fileoffset_t> vRowBuf;
      vProteinCounts.reserve(tNumProteinEntries);

      for (size_t i = 0; i < tNumProteinEntries; ++i)
      {
         size_t tNumProteins;
         if (fread(&tNumProteins, clSizeCometFileOffset, 1, fp) != 1)
         {
            fclose(fp);
            logout(" Error - failed to read protein count from .idx file at entry " + to_string(i) + ".\n");
            return false;
         }
         if (tNumProteins > tMaxProteinSectionEntries)
         {
            fclose(fp);
            logout(" Error - implausible protein count in .idx file at entry " + to_string(i) + "; file may be truncated or corrupt.\n");
            return false;
         }

         vRowBuf.resize(tNumProteins);
         if (fread(vRowBuf.data(), clSizeCometFileOffset, tNumProteins, fp) != tNumProteins)
         {
            fclose(fp);
            logout(" Error - failed to read protein offsets from .idx file at entry " + to_string(i) + "; file may be truncated or corrupt.\n");
            return false;
         }

         for (size_t j = 0; j < tNumProteins; ++j)
         {
            comet_fileoffset_t lOffset = vRowBuf[j];
            if (lOffset < clNamesBase
               || ((lOffset - clNamesBase) % (comet_fileoffset_t)WIDTH_REFERENCE) != 0
               || (uint64_t)(lOffset - clNamesBase) / WIDTH_REFERENCE >= tNumProteinsInFile)
            {
               fclose(fp);
               logout(" Error - out-of-range protein-name offset in .idx file at entry " + to_string(i)
                  + "; the file is likely truncated or corrupt. Rebuild it with -i or -j.\n");
               return false;
            }
            vFlatProteinOrdinals.push_back((unsigned int)((lOffset - clNamesBase) / WIDTH_REFERENCE));
         }

         vProteinCounts.push_back((uint32_t)tNumProteins);
      }

      g_pvProteinsList.clear();
      g_pvProteinsList.reserve(tNumProteinEntries);
      if (!g_pvProteinsList.append_flat(vFlatProteinOrdinals, vProteinCounts))
      {
         fclose(fp);
         logout(" Error - protein list exceeds the uint32 CSR limit (see ProteinsListCSR::append_flat); file may be corrupt.\n");
         return false;
      }
   }

   // Build the in-memory protein name cache before closing the file: one sequential read
   // of the whole name section (tNumProteinsInFile x WIDTH_REFERENCE, already validated
   // above), one string per protein, indexed by ordinal -- Phase 4
   // (docs/20260827_PI_memory.md) replaced the former offset-keyed unordered_map (built
   // from the distinct referenced offsets via a min/max-bounded bulk read) with this
   // complete, ordinal-indexed vector; every ordinal stored in g_pvProteinsList is in
   // range by the validation above, so lookups are a bounds check + index, and the old
   // per-protein file-read fallback is unnecessary.
   {
      g_pvProteinNameCache.clear();
      g_pvProteinNameCache.reserve((size_t)tNumProteinsInFile);

      comet_fseek(fp, clNamesBase, SEEK_SET);

      // read in chunks so a pathologically large name section doesn't demand one giant buffer
      const size_t tChunkBlocks = 65536;   // 16 MB per chunk at WIDTH_REFERENCE == 256
      vector<char> vNameBuf(tChunkBlocks * WIDTH_REFERENCE);
      uint64_t tRemaining = tNumProteinsInFile;

      while (tRemaining > 0)
      {
         size_t tBlocks = (tRemaining < tChunkBlocks) ? (size_t)tRemaining : tChunkBlocks;
         if (fread(vNameBuf.data(), WIDTH_REFERENCE, tBlocks, fp) != tBlocks)
         {
            // Do NOT continue with a partial cache: decoy classification looks peptides'
            // proteins up in g_pvProteinNameCache and treats a miss as "target", so a
            // short cache silently misclassifies decoys and destroys the target/decoy
            // split. Fail the index load loudly instead.
            string strErrorMsg = " Error - cannot read the protein-name section from the"
               " .idx file; the file is likely truncated or corrupt. Re-create the .idx file.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            fclose(fp);
            return false;
         }
         for (size_t b = 0; b < tBlocks; ++b)
         {
            const char* pProtBuf = vNameBuf.data() + b * WIDTH_REFERENCE;
            g_pvProteinNameCache.emplace_back(pProtBuf, strnlen(pProtBuf, WIDTH_REFERENCE - 1));
         }
         tRemaining -= tBlocks;
      }
   }

   fclose(fp);

   if (bIsRTS)
   {
      logout(" Read index: " + to_string(tNumRaw) + " unmodified peptides, "
         + to_string(tNumProteinEntries) + " protein groups\n");
   }
   else
   {
      logout("\n   - Read index: " + to_string(tNumRaw) + " unmodified peptides, "
         + to_string(tNumProteinEntries) + " protein groups\n");
   }

   // Phase 0.5 (docs/20260730_PI_reduction.md): regenerate the mod-permutation tables --
   // and, for PI_DB, the compact variant array -- fresh from g_vRawPeptides + whatever
   // variable mods are active in g_staticParams right now (comet.params for batch; RTS's
   // explicit SetParam() calls for RTS, which never loads a params file -- see
   // docs/RealTimeSearch.md's "RTS variable-mod source"). Neither is persisted in the .idx
   // file any more.
   //
   // PROCESS-LIFETIME, NOT PER-SESSION: g_bPeptideIndexRead/g_bPlainPeptideIndexRead below
   // guard ReadPeptideIndex() against re-entry for as long as the process lives -- neither
   // FinalizeSingleSpectrumSearch() nor anything else ever clears them, so a hypothetical
   // second InitializeSingleSpectrumSearch() call in the same process (after Finalize) would
   // skip this regeneration and keep serving whatever the first call built, even if
   // g_staticParams' variable mods had since changed. This is not new: g_staticParams itself
   // is documented process-lifetime-immutable-after-init (CLAUDE.md's Key Globals table),
   // and every current caller (RealtimeSearch.exe's Main(), tests/rts_repro/rts_repro.cpp,
   // batch comet.exe) calls Initialize.../Finalize... exactly once each, right before the
   // process exits -- so this is unreachable today, not merely untested. It also isn't a
   // one-line fix: CometFragmentIndex::PermuteIndexPeptideMods() and
   // GenerateVariantArray()/EnumerateIndexPeptideMods() below allocate
   // MOD_SEQ_MOD_NUM_START/CNT/POOL_START and PEPTIDE_MOD_SEQ_IDXS with raw new[] that
   // nothing currently frees (the MOD_NUMBERS_POOL/MOD_SEQS_POOL flat pools would at least
   // free cleanly, but are likewise never torn down), and GetVModSlotForAllModsIdx() caches
   // its result in a function-local static for the same reason -- all of these would need an
   // explicit, ordered teardown (not just clearing these two bools) to support real
   // re-parameterization safely. Do not "fix" this by resetting only the two bools below:
   // that would make a reused process regenerate the mod pools/g_dbIndexVariants against
   // the new params while GetVModSlotForAllModsIdx() kept translating them with the OLD
   // compacted mod-slot mapping -- silently wrong scoring, worse than today's clean (if
   // surprising) stale-reuse.
   //
   // FI_DB doesn't need an equivalent call for its own posting list -- GenerateFragmentIndex()
   // (via AddFragmentsThreadProc()) already consumes MOD_NUMBERS_POOL/MOD_SEQS_POOL/etc. from wherever
   // they came from, unchanged whether that's a disk read (pre-Phase-0.5) or this in-memory
   // regeneration.
   CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides);

   if (g_staticParams.iDbType == DbType::PI_DB)
   {
      if (!GenerateVariantArray())
         return false;

      // Intensity score (docs/20260903_IntensityScore_design.md Section 2.3): bind the
      // predicted-intensity file, if configured, to the final PI variant array (position i in
      // g_dbIndexVariants is what SearchPeptideIndex() hands XcorrScoreI()). FI_DB binds to
      // g_fragmentPeptides in CometFragmentIndex::CreateFragmentIndex() instead. Freed in
      // PiStrategy::finalize().
      if (!CometIntensityStore::LoadAndBind(g_staticParams.options.sPredictedIntensityFile, g_dbIndexVariants))
         return false;
   }

   LogIndexMemoryReport();

   // Both guard flags set: PI_DB code checks g_bPeptideIndexRead, FI_DB code (still, after
   // this unification) checks g_bPlainPeptideIndexRead -- see
   // CometFragmentIndex::CreateFragmentIndex(). g_staticParams.iDbType was already set above
   // by ParsePeptideIndexHeader()'s IndexSearchType: parse (docs/20260811_restore_idx_header_mods.md)
   // -- the file records its own mode, so no external index_search_type parameter is
   // consulted here.
   g_bPeptideIndexRead = true;
   g_bPlainPeptideIndexRead = true;

   return true;
}


// See docs/20260713_PIidxformat.md section 8 (Phase B) and
// docs/20260730_PI_reduction.md Phase 1. Mirrors
// CometFragmentIndex::AddFragmentsThreadProc()'s enumeration structure (which
// mod/n-term/c-term combinations to try per peptide) and AddFragments()'s mass
// computation, but pushes a compact FragmentPeptidesStruct reference
// {iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, dPepMass} per valid combination
// instead of materializing a full DBIndex (sequence + explicit pcVarModSites) --
// that reconstruction now happens lazily, per scored candidate, via
// MaterializeOneEntry() below (called from CometSearch::SearchPeptideIndex()).
//
// Two deliberate deviations from AddFragments(), both scoped to this function
// only -- FI_DB's own code is untouched:
//
//  1. Mass is computed by adding variable-mod deltas onto
//     g_vRawPeptides[iWhichPeptide].dPepMass (the authoritative unmodified mass,
//     already correct including protein-terminal static mods, computed once during
//     Phase A digestion) rather than recomputing residue-by-residue from
//     dOH2ProtonCtermNterm. AddFragments()'s from-scratch recompute is documented
//     (CometFragmentIndex.cpp:440-465) as tolerant of a protein-terminal-static-mod
//     discrepancy of up to 10 Da -- acceptable for fragment-ion binning, but not for
//     PI_DB's mass-tolerance-based precursor search.
//
//  2. The bVarModProteinFilter bitmask check skips any candidate position where
//     mods[i] == -1 (not modified in this combination) before bit-testing it.
//     AddFragmentsThreadProc's equivalent check (CometFragmentIndex.cpp:323-335)
//     does not skip these, which passes -1 (a "no mod here" sentinel, not a slot
//     index) to cometbitcheck()'s shift operand -- undefined behavior for a
//     combination that leaves any candidate position unmodified. Also translates
//     through vModSlotForAllModsIdx (see below) rather than bit-testing the
//     compacted index directly.
//
// FI_DB's ModificationsPermuter compacts active variable_modNN slots into ALL_MODS,
// skipping inactive/blank slots, so MOD_NUMBERS_POOL entry values are indices
// into that compacted list, not direct varModList slot indices. vModSlotForAllModsIdx
// rebuilds the exact same compaction order as
// CometFragmentIndex::PermuteIndexPeptideMods()'s ALL_MODS-building loop to translate
// between the two -- must stay in sync with that loop if it ever changes. The same
// translation, and the same VarModSites::MAX_SITES bound (checked here via a site
// count so an over-the-limit combination is rejected at build time with a clear
// error, exactly as before, rather than surfacing later as a silent per-candidate
// skip in MaterializeOneEntry()), is repeated there since that function needs to
// produce an explicit pcVarModSites, not just a count.
// g_staticParams.variableModParameters is read-only for the lifetime of a search (see
// CLAUDE.md's Key Globals table), so this only ever needs to be computed once. Computed via
// a function-local static under C++11's thread-safe "magic statics" guarantee, since
// MaterializeOneEntry() (called through this function) runs concurrently across PI_DB's
// search threads. "Once" here means once per process, not once per search session -- see
// the PROCESS-LIFETIME note above ReadPeptideIndex()'s PermuteIndexPeptideMods() call; a
// process that re-parameterized variable mods and re-entered ReadPeptideIndex() would still
// get this magic static's first-call value here.
const vector<int>& CometPeptideIndex::GetVModSlotForAllModsIdx()
{
   static const vector<int> vModSlotForAllModsIdx = []
   {
      vector<int> v;
      for (int i = 0; i < FRAGINDEX_VMODS; ++i)
      {
         if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0] != '-'))
         {
            v.push_back(i);
         }
      }
      return v;
   }();
   return vModSlotForAllModsIdx;
}


int CometPeptideIndex::TranslateVarModSlot(const vector<int>& vModSlotForAllModsIdx, int compactedIdx)
{
   if (compactedIdx < 0 || (size_t)compactedIdx >= vModSlotForAllModsIdx.size())
      return -1;
   return vModSlotForAllModsIdx[(size_t)compactedIdx];
}


bool CometPeptideIndex::PassesVarModProteinFilter(const vector<int>& vModSlotForAllModsIdx,
   const char* mods, int modStringLen, unsigned short siVarModProteinFilter)
{
   for (int i = 0; i < modStringLen; ++i)
   {
      int iSlot = TranslateVarModSlot(vModSlotForAllModsIdx, mods[i]);
      if (iSlot >= 0 && !cometbitcheck(siVarModProteinFilter, iSlot))
         return false;
   }
   return true;
}


// Dual-mode since Phase 2 (docs/20260827_PI_memory.md): with pStaging == NULL this is a
// count-only pass (*ptCursor is incremented per accepted tuple, nothing stored); with
// pStaging set, accepted tuples are written at pStaging[*ptCursor++]. Both passes run the
// exact same enumeration, filters, and mass computation, so the fill pass's element count
// always equals the count pass's -- GenerateVariantArray() verifies this. tStagingCap
// bounds the fill pass as corruption defense (ignored in count mode).
bool CometPeptideIndex::EnumerateIndexPeptideMods(FragmentPeptidesStruct* pStaging,
   size_t tStagingCap, size_t* ptCursor)
{
   const vector<int>& vModSlotForAllModsIdx = GetVModSlotForAllModsIdx();

   bool bModSitesOverflow = false;
   bool bStagingOverflow = false;

   auto tryPush = [&](size_t iWhichPeptide, int modNumIdx, char cNtermMod, char cCtermMod)
   {
      const RawPeptideView raw = g_vRawPeptides.at(iWhichPeptide);
      const int iLen = raw.iLen;

      double dCalcPepMass = raw.dPepMass;
      int cNumSites = 0;

      if (modNumIdx >= 0)
      {
         int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
         int iModSeqLen;
         const char* pModSeq = GetModSeq(modSeqIdx, iModSeqLen);
         const char* mods = GetModNumEntry(modNumIdx, modSeqIdx, iModSeqLen);

         int j = 0;
         for (int i = 0; i < iLen; ++i)
         {
            // j bound: pool mod-seq entries are not NUL-terminated, so the former
            // std::string behavior (operator[] at size() safely returning '\0', which never
            // matches a residue) is replaced by an explicit stop once every modifiable
            // position has been consumed -- same guard as MaterializeOneEntry() below.
            if (j >= iModSeqLen)
               break;
            if (raw.szPeptide[i] == pModSeq[j])
            {
               int iSlot = TranslateVarModSlot(vModSlotForAllModsIdx, mods[j]);
               if (iSlot >= 0)
               {
                  dCalcPepMass += g_staticParams.variableModParameters.varModList[iSlot].dVarModMass;
                  ++cNumSites;
               }
               j++;
            }
         }
      }

      if (cNtermMod >= 0)
      {
         dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
         ++cNumSites;
      }
      if (cCtermMod >= 0)
      {
         dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;
         ++cNumSites;
      }

      // Same VarModSites::MAX_SITES bound MaterializeOneEntry() will need to respect when it
      // later builds an explicit pcVarModSites for this same tuple -- checked here (as a count,
      // no VarModSites object needed yet) so an over-the-limit combination fails the build
      // loudly instead of silently at search time.
      if (cNumSites > VarModSites::MAX_SITES)
      {
         bModSitesOverflow = true;
         return;
      }

      if (dCalcPepMass > g_massRange.dMaxMass || dCalcPepMass < g_massRange.dMinMass)
         return;

      if (pStaging != NULL)
      {
         if (*ptCursor >= tStagingCap)
         {
            bStagingOverflow = true;   // fill pass disagrees with count pass -- internal error
            return;
         }
         FragmentPeptidesStruct& sVariant = pStaging[*ptCursor];
         sVariant.dPepMass = dCalcPepMass;
         sVariant.iWhichPeptide = (unsigned int)iWhichPeptide;
         sVariant.modNumIdx = modNumIdx;
         sVariant.cNtermMod = cNtermMod;
         sVariant.cCtermMod = cCtermMod;
      }
      ++*ptCursor;
   };

   for (size_t iWhichPeptide = 0; iWhichPeptide < g_vRawPeptides.size(); ++iWhichPeptide)
   {
      const RawPeptideView raw = g_vRawPeptides.at(iWhichPeptide);
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];

      if (g_staticParams.variableModParameters.bVarTermModSearch)
      {
         for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
               && (!g_staticParams.variableModParameters.bVarModProteinFilter
                  || cometbitcheck(raw.siVarModProteinFilter, ctNtermMod)))
            {
               tryPush(iWhichPeptide, -1, ctNtermMod, -1);
            }
         }

         for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
               && (!g_staticParams.variableModParameters.bVarModProteinFilter
                  || cometbitcheck(raw.siVarModProteinFilter, ctCtermMod)))
            {
               tryPush(iWhichPeptide, -1, -1, ctCtermMod);
            }
         }

         for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
         {
            for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                  && g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                  && (!g_staticParams.variableModParameters.bVarModProteinFilter ||
                     (cometbitcheck(raw.siVarModProteinFilter, ctNtermMod)
                        && cometbitcheck(raw.siVarModProteinFilter, ctCtermMod))))
               {
                  tryPush(iWhichPeptide, -1, ctNtermMod, ctCtermMod);
               }
            }
         }
      }

      if (modSeqIdx < 0)
         continue;

      int startIdx = MOD_SEQ_MOD_NUM_START[modSeqIdx];
      if (startIdx == -1)
         continue;

      int modNumCount = MOD_SEQ_MOD_NUM_CNT[modSeqIdx];

      for (int modNumIdx = startIdx; modNumIdx < startIdx + modNumCount; ++modNumIdx)
      {
         bool bPass = true;

         if (g_staticParams.variableModParameters.bVarModProteinFilter)
         {
            int iModSeqLen;
            GetModSeq(modSeqIdx, iModSeqLen);
            bPass = PassesVarModProteinFilter(vModSlotForAllModsIdx,
               GetModNumEntry(modNumIdx, modSeqIdx, iModSeqLen), iModSeqLen,
               raw.siVarModProteinFilter);
         }

         if (!bPass)
            continue;

         tryPush(iWhichPeptide, modNumIdx, -1, -1);

         if (g_staticParams.variableModParameters.bVarTermModSearch)
         {
            for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                  && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(raw.siVarModProteinFilter, ctNtermMod)))
               {
                  tryPush(iWhichPeptide, modNumIdx, ctNtermMod, -1);
               }
            }

            for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                  && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(raw.siVarModProteinFilter, ctCtermMod)))
               {
                  tryPush(iWhichPeptide, modNumIdx, -1, ctCtermMod);
               }
            }

            for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
            {
               for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
               {
                  if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                     && g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                     && (!g_staticParams.variableModParameters.bVarModProteinFilter ||
                        (cometbitcheck(raw.siVarModProteinFilter, ctNtermMod)
                           && cometbitcheck(raw.siVarModProteinFilter, ctCtermMod))))
                  {
                     tryPush(iWhichPeptide, modNumIdx, ctNtermMod, ctCtermMod);
                  }
               }
            }
         }
      }
   }

   if (bModSitesOverflow)
   {
      string strErrorMsg = " Error - a peptide's variable modification count exceeds VarModSites::MAX_SITES ("
         + std::to_string(VarModSites::MAX_SITES) + "). Reduce max_variable_mods_in_peptide or the number "
         + "of active variable mod types, or widen VarModSites::MAX_SITES (core/Types.h) if this "
         + "configuration is intentional.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   if (bStagingOverflow)
   {
      string strErrorMsg = " Error - EnumerateIndexPeptideMods() fill pass produced more variants "
         "than its count pass; internal error in the variant enumeration.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   return true;
}


// See docs/20260730_PI_reduction.md Phase 3. Single-entry counterpart to
// EnumerateIndexPeptideMods()'s tryPush lambda above -- same reconstruction logic, but building
// an explicit DBIndex (sequence, pcVarModSites) instead of just a mass, so it can be called once
// per mass-window candidate at search time instead of once per peptide at build time. Unlike tryPush,
// this does not re-check dCalcPepMass against g_massRange: that check only matters for deciding
// whether a (peptide, mod combination) tuple gets included in the index at all, which already
// happened once, at build time, when this exact tuple was written to the compact variant array --
// re-checking it here would be redundant at best.
bool CometPeptideIndex::MaterializeOneEntry(size_t iWhichPeptide, int modNumIdx, char cNtermMod,
   char cCtermMod, DBIndex& out)
{
   const vector<int>& vModSlotForAllModsIdx = GetVModSlotForAllModsIdx();

   // iWhichPeptide, like modNumIdx below, comes directly from an on-disk FragmentPeptidesStruct
   // record read straight off disk by CometSearch::SearchPeptideIndex() -- untrusted for a
   // corrupt/truncated .idx, same as the modNumIdx/modSeqIdx checks below. Checked explicitly
   // here (rather than relying on g_vRawPeptides.at()'s bounds-checked exception) so a bad
   // value fails this one candidate cleanly instead of risking an uncaught exception mid-search;
   // ulSizevRawPeptides == g_vRawPeptides.size() is already guaranteed by ReadPeptideIndex()'s
   // own load-time check, so this same bound also covers PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide]
   // below, which -- unlike g_vRawPeptides -- is a raw array with no bounds-checked accessor.
   if (iWhichPeptide >= g_vRawPeptides.size())
      return false;

   const RawPeptideView raw = g_vRawPeptides.at(iWhichPeptide);
   const int iLen = raw.iLen;

   double dCalcPepMass = raw.dPepMass;
   VarModSites pcVarModSites;

   if (modNumIdx >= 0)
   {
      // Defense against a malformed compact-variant tuple: fail this one candidate (the
      // caller already treats a false return as "skip it") rather than let a bad tuple reach
      // an out-of-bounds pool read. Beyond the modSeqIdx range check, modNumIdx must lie
      // inside its own mod-seq's [MOD_SEQ_MOD_NUM_START, +CNT) block for GetModNumEntry()'s
      // pooled-offset arithmetic to address the right entry -- a stronger constraint than the
      // former global bounds test against MOD_NUMBERS.size(), which a (peptide, modNumIdx)
      // pairing from mismatched mod-seqs could still have passed.
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
      if (modSeqIdx < 0 || modSeqIdx >= GetNumModSeqs())
         return false;
      int iModNumStart = MOD_SEQ_MOD_NUM_START[modSeqIdx];
      if (iModNumStart < 0 || modNumIdx < iModNumStart
         || modNumIdx >= iModNumStart + MOD_SEQ_MOD_NUM_CNT[modSeqIdx])
         return false;
      int iModSeqLen;
      const char* pModSeq = GetModSeq(modSeqIdx, iModSeqLen);
      const char* mods = GetModNumEntry(modNumIdx, modSeqIdx, iModSeqLen);

      int j = 0;
      for (int i = 0; i < iLen; ++i)
      {
         // j reaching iModSeqLen before i reaches iLen is normal, not corruption -- it
         // means every modifiable position in the peptide has already been consumed and the
         // remaining residues are all non-modifiable (e.g. any peptide whose last modifiable
         // residue isn't also its literal last residue). The pre-pool code relied on
         // std::string::operator[](size()) safely returning '\0' here, which never matches a
         // real residue, to no-op through the rest of the peptide; an earlier version of this
         // fix instead treated reaching the end of modSeq as corruption and rejected the
         // (entirely valid) candidate outright -- caught via a real ~24% PI_DB PSM-count drop
         // on comet-debug3/4's data (17,660 -> 13,410) that a synthetic-corruption-only test
         // didn't surface. Stop considering matches once modSeq is exhausted instead. This
         // bound also covers mods[j]: the pool entry's length equals the mod-seq's length by
         // construction (see GetModNumEntry()), so the former separate modStringLen check
         // is subsumed here.
         if (j >= iModSeqLen)
            break;
         if (raw.szPeptide[i] == pModSeq[j])
         {
            // TranslateVarModSlot() returns -1 for both mods[j] == -1 (ordinary "not modified
            // here") and mods[j] out of range (corrupt/mismatched .idx) -- the two are
            // distinguished explicitly below because only the latter should reject this whole
            // candidate; the former is normal and simply contributes no mass/site here.
            int iSlot = TranslateVarModSlot(vModSlotForAllModsIdx, mods[j]);
            if (mods[j] != -1 && iSlot < 0)
               return false;
            if (iSlot >= 0)
            {
               dCalcPepMass += g_staticParams.variableModParameters.varModList[iSlot].dVarModMass;
               if (!pcVarModSites.set(i, (char)(iSlot + 1)))
                  return false;
            }
            j++;
         }
      }
   }

   if (cNtermMod >= 0)
   {
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
      if (!pcVarModSites.set(iLen, (char)(cNtermMod + 1)))
         return false;
   }
   if (cCtermMod >= 0)
   {
      dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;
      if (!pcVarModSites.set(iLen + 1, (char)(cCtermMod + 1)))
         return false;
   }

   out.pcVarModSites = pcVarModSites;
   out.lIndexProteinFilePosition = raw.lIndexProteinFilePosition;
   out.dPepMass = dCalcPepMass;
   out.siVarModProteinFilter = raw.siVarModProteinFilter;
   out.cPrevAA = raw.cPrevAA;
   out.cNextAA = raw.cNextAA;
   strcpy(out.sPeptide, raw.szPeptide);

   return true;
}


bool CometPeptideIndex::ExportVariants(const string& strOutputFile)
{
   if (g_staticParams.iDbType != DbType::PI_DB || g_dbIndexVariants.empty())
   {
      string strErrorMsg = " Error - ExportVariants() requires a PI_DB session with a "
         "populated variant array (ReadPeptideIndex() must have run in PI_DB mode first).\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   FILE* fp = fopen(strOutputFile.c_str(), "w");
   if (fp == NULL)
   {
      string strErrorMsg = " Error - cannot open \"" + strOutputFile + "\" for writing.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Leading comment line, read by tools/idx_to_carafe.py and propagated through into
   // tools/carafe_ms2_to_fi_mask.py's mask-file header (docs/20260805_carafe.md Section 8 items 12-14) --
   // lets Phase 3's CometPredictedMask::Load() reject a mask built against different variable
   // mods than are live in the search that's about to consume it, since modNumIdx numbering
   // (unlike iWhichPeptide) isn't provable safe from the .idx fingerprint alone any more
   // (Section 6.10's closing note). Written here, not derived after the fact, since this
   // export IS the live comet.params session whose variable mods define modNumIdx numbering.
   fprintf(fp, "# VarModConfig: %s\n", CometPredictedMask::ComputeVarModConfigString().c_str());
   fprintf(fp, "iWhichPeptide\tmodNumIdx\tcNtermMod\tcCtermMod\tmass\tsequence\tsites\n");

   DBIndex entry;
   std::ostringstream oss;
   for (size_t tWhichVariant = 0; tWhichVariant < g_dbIndexVariants.size(); ++tWhichVariant)
   {
      // ported to the VariantArray SoA (docs/20260827_PI_memory.md Phase 2): field reads
      // become accessor calls; the exported values are identical to the former 24B-AoS
      // export (MaterializeOneEntry()'s recomputed mass is bit-identical to what the old
      // array stored per entry)
      const unsigned int uiWhichPeptide = g_dbIndexVariants.vuiWhichPeptide[tWhichVariant];
      const int modNumIdx = g_dbIndexVariants.GetModNumIdx(tWhichVariant);
      const char cNtermMod = g_dbIndexVariants.GetNtermMod(tWhichVariant);
      const char cCtermMod = g_dbIndexVariants.GetCtermMod(tWhichVariant);

      if (!MaterializeOneEntry(uiWhichPeptide, modNumIdx, cNtermMod, cCtermMod, entry))
      {
         string strErrorMsg = " Error - failed to materialize variant (iWhichPeptide="
            + std::to_string(uiWhichPeptide) + ", modNumIdx=" + std::to_string(modNumIdx)
            + ", cNtermMod=" + std::to_string((int)cNtermMod)
            + ", cCtermMod=" + std::to_string((int)cCtermMod) + ") during export.\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         fclose(fp);
         return false;
      }

      oss.str("");
      oss.clear();
      // pcVarModSites entries are already in ascending position order (VarModSites::set()'s
      // own contract, see core/Types.h) -- not required for correctness here (Python parses
      // each "pos:mass" token independently) but kept for readable/diffable output.
      for (unsigned char i = 0; i < entry.pcVarModSites.cNumSites; ++i)
      {
         int iSlot = (int)entry.pcVarModSites.residue[i] - 1;   // see MaterializeOneEntry()'s set(pos, slot+1)
         double dMass = g_staticParams.variableModParameters.varModList[iSlot].dVarModMass;
         if (i > 0)
            oss << ";";
         oss << (int)entry.pcVarModSites.position[i] << ":" << std::setprecision(10) << dMass;
      }

      fprintf(fp, "%u\t%d\t%d\t%d\t%.10f\t%s\t%s\n",
         uiWhichPeptide, modNumIdx, (int)cNtermMod, (int)cCtermMod,
         entry.dPepMass, entry.sPeptide, oss.str().c_str());
   }

   fclose(fp);

   logout("   - exported " + std::to_string(g_dbIndexVariants.size()) + " peptide-index variants to \""
      + strOutputFile + "\"\n");

   return true;
}


// docs/20260827_PI_memory.md Phase 0: one-shot, structure-by-structure memory report,
// logged at the end of ReadPeptideIndex() when the COMET_MEMREPORT environment variable is
// set. Reports load-time state: in FI_DB mode, g_vFragmentPeptides and the fragment-ion
// posting list (g_iFragmentIndex) are built after this point
// (CometFragmentIndex::CreateFragmentIndex()) and so legitimately show as empty here.
// Capacities (not sizes) are what's charged, since allocator-held growth slack is real
// process memory.
void CometPeptideIndex::LogIndexMemoryReport()
{
   if (getenv("COMET_MEMREPORT") == NULL)
      return;

   auto mb = [](size_t tBytes) -> string
   {
      char szBuf[64];
      snprintf(szBuf, sizeof(szBuf), "%.1f MB", tBytes / (1024.0 * 1024.0));
      return string(szBuf);
   };

   size_t tNumRaw = g_vRawPeptides.size();
   int iNumModSeqs = GetNumModSeqs();

   // MOD_SEQ_MOD_NUM_START/CNT (int each) + MOD_SEQ_MOD_NUM_POOL_START (uint64_t), all
   // sized per modifiable sequence, + the MOD_SEQS_OFFSET offsets array.
   size_t tModAuxBytes = (size_t)iNumModSeqs * (2 * sizeof(int) + sizeof(uint64_t))
      + MOD_SEQS_OFFSET.capacity() * sizeof(unsigned int);

   size_t tNameBytes = 0;
   for (const auto& sName : g_pvProteinNameCache)
      tNameBytes += sName.size();

   std::ostringstream oss;
   oss << " Index memory report (COMET_MEMREPORT), load-time state:\n";
   oss << "   g_vRawPeptides:       " << tNumRaw << " entries (pooled), "
       << mb(g_vRawPeptides.heap_bytes()) << "\n";
   oss << "   g_dbIndexVariants:    " << g_dbIndexVariants.size() << " entries (SoA), "
       << mb(g_dbIndexVariants.heap_bytes()) << "\n";
   oss << "   g_fragmentPeptides:   " << g_fragmentPeptides.size() << " entries (SoA), "
       << mb(g_fragmentPeptides.heap_bytes()) << " (FI_DB fills this after load)\n";
   oss << "   mod combinations:     " << MOD_NUM << " entries; MOD_NUMBERS_POOL "
       << mb(MOD_NUMBERS_POOL.capacity()) << "\n";
   oss << "   modifiable sequences: " << iNumModSeqs << "; MOD_SEQS_POOL "
       << mb(MOD_SEQS_POOL.capacity()) << "; aux arrays " << mb(tModAuxBytes) << "\n";
   oss << "   PEPTIDE_MOD_SEQ_IDXS: " << mb(tNumRaw * sizeof(int)) << "\n";
   oss << "   g_pvProteinsList:     " << g_pvProteinsList.size() << " rows, "
       << g_pvProteinsList.total_offsets() << " offsets, "
       << mb(g_pvProteinsList.heap_bytes()) << "\n";
   oss << "   g_pvProteinNameCache: " << g_pvProteinNameCache.size() << " names (by ordinal), "
       << mb(tNameBytes) << " of string payload\n";
   logout(oss.str());
}


// Page-granular staging allocation for GenerateVariantArray()'s Phase 2 transcode
// (docs/20260827_PI_memory.md): a vector can't return already-consumed memory to the OS
// mid-walk, so on POSIX the sorted staging array lives in raw mmap pages and
// DecommitStagingRange() progressively releases the fully-transcoded prefix while the
// transcode is still running, keeping the step's peak at ~24B/entry (the staging alone)
// instead of 24B + 13B.
//
// Windows deliberately uses plain malloc/free with NO progressive release: the original
// VirtualAlloc + chunked VirtualFree(MEM_DECOMMIT) implementation pattern-matched an
// endpoint-protection behavioral heuristic -- CrowdStrike Falcon quarantined the freshly
// built Comet.exe on its first execution (observed 2026-08-27; that memory-management
// idiom is common in packers/loaders). Cost of the fallback: the Windows transcode
// transient is ~37B/entry (staging + SoA together) instead of ~24B/entry -- still at or
// below the pre-Phase-2 steady state (24B x ~1.5x vector growth capacity plus the
// mod-table overhead Phase 1 removed).
void* CometPeptideIndex::AllocStagingPages(size_t tBytes)
{
#ifdef _WIN32
   return malloc(tBytes);
#else
   void* p = mmap(NULL, tBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   return (p == MAP_FAILED) ? NULL : p;
#endif
}

// POSIX: returns the physical pages backing [pBase + tFrom, pBase + tTo) to the OS; the
// address range stays reserved (a read after this would see zero-filled pages, but the
// transcode only ever walks forward and never re-reads a released range). Bounds align
// inward to page boundaries, so partially-covered pages at either end are left committed.
// Windows: no-op -- see AllocStagingPages() above.
void CometPeptideIndex::DecommitStagingRange(void* pBase, size_t tFrom, size_t tTo)
{
#ifdef _WIN32
   (void)pBase;
   (void)tFrom;
   (void)tTo;
#else
   const size_t tPage = 4096;
   size_t tBegin = (tFrom + tPage - 1) & ~(tPage - 1);
   size_t tEnd = tTo & ~(tPage - 1);
   if (tEnd <= tBegin)
      return;
   madvise((char*)pBase + tBegin, tEnd - tBegin, MADV_DONTNEED);
#endif
}

void CometPeptideIndex::FreeStagingPages(void* pBase, size_t tBytes)
{
   if (pBase == NULL)
      return;
#ifdef _WIN32
   free(pBase);
   (void)tBytes;
#else
   munmap(pBase, tBytes);
#endif
}


// docs/20260730_PI_reduction.md Phase 0.5; docs/20260827_PI_memory.md Phase 2. PI_DB's
// counterpart to what used to be part of WritePeptideIndex()'s build-time work -- builds
// the compact per-variant array (g_dbIndexVariants) from g_vRawPeptides + the
// mod-permutation tables a prior call to CometFragmentIndex::PermuteIndexPeptideMods()
// just built, using whichever variable mods are active in comet.params right now. Called
// once per search session from ReadPeptideIndex(), not at build time -- nothing about the
// modified-peptide variant list is persisted to disk.
//
// Phase 2 shape: the enumeration runs twice -- a count-only pass, then a fill pass into an
// exactly-sized staging array of FragmentPeptidesStruct (no vector growth slack; measured
// at 553 MB of pure overshoot at the reference benchmark scale before this change). The
// staging is sorted with the same element type, comparator, and input order as the
// pre-Phase-2 vector<FragmentPeptidesStruct> sort, so the final candidate ordering --
// equal-mass ties included, which are common (positional mod isomers share a mass) and
// observable through result tie-breaking -- is identical to the old implementation's.
// The sorted staging is then transcoded into the 13B/entry SoA (VariantArray,
// core/Types.h), releasing consumed staging pages to the OS as it goes.
bool CometPeptideIndex::GenerateVariantArray()
{
   g_dbIndexVariants.clear();

   const size_t tNumRaw = g_vRawPeptides.size();
   const bool bIncludeUnmodified = !g_staticParams.variableModParameters.iRequireVarMod;

   // Pass 1: count only. require_variable_mod: every entry must carry a required mod, so
   // the fully-unmodified variant (modNumIdx == -1, no terminal mods) is only included when
   // that's not required (matching CometFragmentIndex::AddFragmentsThreadProc()'s
   // equivalent check); when included, it's every raw peptide unconditionally -- raw
   // peptides were already mass/length-filtered at digestion.
   size_t tNumVariants = bIncludeUnmodified ? tNumRaw : 0;
   if (!EnumerateIndexPeptideMods(NULL, 0, &tNumVariants))
   {
      logerr(" Error in EnumerateIndexPeptideMods() while sizing the peptide-index variant list.\n");
      return false;
   }

   if (tNumVariants == 0)
   {
      string strErrorMsg = " Error: no peptides in generated index; check the input database file or search parameters.\n";
      logerr(strErrorMsg);
      return false;
   }

   // Pass 2: fill, sort, transcode.
   const size_t tStagingBytes = tNumVariants * sizeof(FragmentPeptidesStruct);
   FragmentPeptidesStruct* pStaging = (FragmentPeptidesStruct*)AllocStagingPages(tStagingBytes);
   if (pStaging == NULL)
   {
      logerr(" Error - cannot allocate the " + std::to_string(tStagingBytes)
         + "-byte variant staging buffer.\n");
      return false;
   }

   size_t tCursor = 0;
   if (bIncludeUnmodified)
   {
      for (size_t i = 0; i < tNumRaw; ++i)
      {
         FragmentPeptidesStruct& sVariant = pStaging[tCursor++];
         sVariant.dPepMass = g_vRawPeptides[i].dPepMass;
         sVariant.iWhichPeptide = (unsigned int)i;
         sVariant.modNumIdx = -1;
         sVariant.cNtermMod = -1;
         sVariant.cCtermMod = -1;
      }
   }

   if (!EnumerateIndexPeptideMods(pStaging, tNumVariants, &tCursor))
   {
      FreeStagingPages(pStaging, tStagingBytes);
      logerr(" Error in EnumerateIndexPeptideMods() while generating the peptide-index variant list.\n");
      return false;
   }

   if (tCursor != tNumVariants)
   {
      // The two passes run identical, deterministic enumeration; disagreement is an
      // internal bug, not a data problem -- fail loudly rather than search a partial array.
      FreeStagingPages(pStaging, tStagingBytes);
      logerr(" Error - variant enumeration count mismatch (counted " + std::to_string(tNumVariants)
         + ", filled " + std::to_string(tCursor) + "); internal error.\n");
      return false;
   }

   // No dedup pass: Phase A/B produces exactly one g_vRawPeptides row per unique peptide
   // and EnumerateIndexPeptideMods() enumerates each valid mod combination exactly once
   // per raw peptide -- there is no source of duplication to defend against (see
   // docs/20260730_PI_reduction.md Section 8, Open Question 1).

   // Sort by mass (FragmentPeptidesStruct::operator< compares dPepMass) -- over raw
   // pointers rather than vector iterators, which resolve to the same std::sort
   // instantiation on this element type.
   sort(pStaging, pStaging + tNumVariants);

   // The 4-byte fixed-point key must be able to represent the largest (last) mass.
   if (!(pStaging[tNumVariants - 1].dPepMass * VariantArray::MASS_KEY_SCALE < 4294967295.0))
   {
      FreeStagingPages(pStaging, tStagingBytes);
      logerr(" Error - peptide mass " + std::to_string(pStaging[tNumVariants - 1].dPepMass)
         + " exceeds the variant mass-key range; reduce peptide_mass_range.\n");
      return false;
   }

   // reserve(), not resize(): resize() value-initializes and therefore touches every page
   // of all four arrays up front, making the whole 13B/entry SoA resident BEFORE the
   // staging release below can give anything back -- measured as a 5.3 GiB transient peak
   // (vs. 4.4 GiB steady) at the reference benchmark scale. reserve() only maps the
   // address space; pages become resident as push_back writes them, in step with the
   // staging pages being released, so the transcode's peak stays ~24B/entry (the staging).
   g_dbIndexVariants.vuiMassKey.reserve(tNumVariants);
   g_dbIndexVariants.vuiWhichPeptide.reserve(tNumVariants);
   g_dbIndexVariants.vuiModNumIdx.reserve(tNumVariants);
   g_dbIndexVariants.vucTermMods.reserve(tNumVariants);

   {
      const size_t tChunkEntries = 4 * 1024 * 1024;  // release granularity: ~96 MB of staging
      size_t tReleasedBytes = 0;

      for (size_t i = 0; i < tNumVariants; ++i)
      {
         const FragmentPeptidesStruct& s = pStaging[i];
         g_dbIndexVariants.vuiMassKey.push_back((unsigned int)llround(s.dPepMass * VariantArray::MASS_KEY_SCALE));
         g_dbIndexVariants.vuiWhichPeptide.push_back(s.iWhichPeptide);
         g_dbIndexVariants.vuiModNumIdx.push_back((s.modNumIdx < 0) ? 0xFFFFFFFFu : (unsigned int)s.modNumIdx);
         g_dbIndexVariants.vucTermMods.push_back((unsigned char)(((s.cNtermMod + 1) << 4) | (s.cCtermMod + 1)));

         if (((i + 1) % tChunkEntries) == 0)
         {
            size_t tConsumedBytes = (i + 1) * sizeof(FragmentPeptidesStruct);
            DecommitStagingRange(pStaging, tReleasedBytes, tConsumedBytes);
            tReleasedBytes = tConsumedBytes;
         }
      }
   }

   FreeStagingPages(pStaging, tStagingBytes);

   std::string strNumVariants;
   if (g_dbIndexVariants.size() > 1e6)
   {
      std::ostringstream oss;
      oss << std::scientific << std::setprecision(3) << static_cast<double>(g_dbIndexVariants.size());
      strNumVariants = oss.str();
   }
   else
      strNumVariants = std::to_string(g_dbIndexVariants.size());

   logout("   - " + strNumVariants + " modified peptides\n");

   return true;
}


bool CometPeptideIndex::WritePeptideIndex(ThreadPool* tp)
{
   bool bSucceeded;
   FILE* fptr;
   string strIndexFile;
   bool bSwapIdxExtension = false;

   // RTS's "auto-build if the requested .idx is missing" path (InitializeSingleSpectrumSearch())
   // passes a database_name that already ends in ".idx" (the file that doesn't exist yet) rather
   // than a plain FASTA path -- GeneratePlainPeptideIndex() below needs the real FASTA path to
   // read from, so temporarily strip the ".idx" suffix in place, then restore it once the FASTA
   // has been fully digested (before writing the header, so "InputDB:" reflects the originally
   // requested path either way). Mirrors CometFragmentIndex::WriteFIPlainPeptideIndex()'s
   // pre-unification handling of the same case -- lost when that function was retired in favor of
   // this one (docs/20260730_PI_reduction.md Phase 0) until this fix restored it.
   size_t databaseLen = strlen(g_staticParams.databaseInfo.szDatabase);
   if (databaseLen >= 4 && !strcmp(g_staticParams.databaseInfo.szDatabase + databaseLen - 4, ".idx"))
   {
      strIndexFile = g_staticParams.databaseInfo.szDatabase;
      g_staticParams.databaseInfo.szDatabase[databaseLen - 4] = '\0';
      bSwapIdxExtension = true;
   }
   else
      strIndexFile = string(g_staticParams.databaseInfo.szDatabase) + ".idx";

   if ((fptr = fopen(strIndexFile.c_str(), "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", strIndexFile.c_str());
      exit(1);
   }

   logout(" Creating index file: ");
   fflush(stdout);

   auto tPeptideIndexStartTime = chrono::steady_clock::now();

   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);

   // these are used in call to RunSearch to generate peptides
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
      g_massRange.bNarrowMassRange = true;
   else
      g_massRange.bNarrowMassRange = false;

   // Phase A: generate the deduplicated unmodified peptide list using the fast
   // per-thread digestion path (same code FI_DB's plain-peptide-index build uses),
   // instead of the legacy RunSearch() path (one heap-allocated DBIndex push per
   // protein occurrence, under a global mutex). See docs/20260713_PIidxformat.md.
   if (bSucceeded)
      bSucceeded = CometFragmentIndex::GeneratePlainPeptideIndex(tp);

   if (bSwapIdxExtension)
      strcat(g_staticParams.databaseInfo.szDatabase, ".idx");

   if (!bSucceeded)
   {
      string strErrorMsg = " Error in GeneratePlainPeptideIndex() for index creation.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      // fptr was opened above but nothing has been written to it yet on this path -- close
      // and delete it rather than leaking the handle and leaving a 0-byte .idx on disk that
      // a later run could mistake for a real (if empty) index.
      fclose(fptr);
      remove(strIndexFile.c_str());
      return false;
   }

   // g_vRawPeptides was populated (pooled, mass-sorted per length) directly by
   // GeneratePlainPeptideIndex()'s merge -- the former 88B/entry g_pvDBIndex intermediate,
   // which briefly DOUBLED the peptide table's footprint here, is gone
   // (docs/20260827_PI_memory.md build-path follow-up). Kept alive (not cleared) for the
   // rest of this function: it's written to the .idx file below and read back at search
   // time (CometPeptideIndex::ReadPeptideIndex()).

   // Phase 0.5 (docs/20260730_PI_reduction.md): no mod-related build step here any more.
   // g_vRawPeptides -- unmodified peptides only, filtered by peptide mass/length range and
   // carrying static mods -- is the only peptide-level data this function writes to disk.
   // MOD_NUMBERS_POOL/MOD_SEQS_POOL and the modified-peptide variant array are generated fresh once
   // per search session instead, from g_vRawPeptides + whatever comet.params has active at
   // that moment (see CometPeptideIndex::ReadPeptideIndex()/GenerateVariantArray()) -- so
   // there's nothing here to enumerate, sort, or sanity-check against an empty result: a
   // build with zero raw peptides already fails inside GeneratePlainPeptideIndex() above.

   logout(" - writing file\n");
   fflush(stdout);

   // Unified index format shared by PI_DB and FI_DB search modes (docs/20260730_PI_reduction.md
   // Phase 0/0.5; restored to be fully self-describing by docs/20260811_restore_idx_header_mods.md)
   // -- one build path (-i and -j both call this function), one on-disk layout, one reader
   // (ReadPeptideIndex(), below). Unlike the pre-121 separate PI_DB/FI_DB formats, both modes
   // share this exact binary layout; which mode a given file was built for is instead recorded
   // explicitly via the IndexSearchType: line immediately below, set from whichever of -i/-j
   // (bCreateFragmentIndex/bCreatePeptideIndex) triggered this build. A search against an
   // existing .idx reads that line back and needs no index_search_type parameter of its own --
   // that parameter is only consulted when a .idx named on the command line/comet.params
   // doesn't exist yet and must be auto-built first (CometSearchManager.cpp), since there's
   // nothing to read the mode from until then. Old-format files (v3's per-slot-only VariableMod:
   // line, v2's index_search_type-only dispatch, or anything pre-unification) are rejected by
   // the version check in ParsePeptideIndexHeader() with a clear rebuild message rather than
   // being misread.
   fprintf(fptr, "Comet index database v4.  Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fptr, "IndexSearchType: %s\n",
      g_staticParams.options.bCreatePeptideIndex ? "peptide index" : "fragment ion index");
   fprintf(fptr, "InputDB:  %s\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fptr, "MassRange: %lf %lf\n", g_staticParams.options.dPeptideMassLow, g_staticParams.options.dPeptideMassHigh);
   fprintf(fptr, "LengthRange: %d %d\n", g_staticParams.options.peptideLengthRange.iStart, g_staticParams.options.peptideLengthRange.iEnd);
   fprintf(fptr, "MassType: %d %d\n", g_staticParams.massUtility.bMonoMassesParent, g_staticParams.massUtility.bMonoMassesFragment);
   fprintf(fptr, "DecoySearch: %d\n", g_staticParams.options.iDecoySearch);
   // AnalyzePeptideIndex() (PI_DB) classifies a candidate as target vs. decoy purely by
   // matching its stored protein name against g_staticParams.szDecoyPrefix -- unlike every
   // other build-time-baked identity in this header (Enzyme:, StaticMod:, VariableMod:),
   // decoy_prefix was never persisted, so a PI_DB target-decoy .idx searched later with a
   // mismatched (or default) decoy_prefix would silently score every real decoy as a
   // target. Restored here for the same reason StaticMod:/VariableMod: are authoritative.
   fprintf(fptr, "DecoyPrefix: %s\n", g_staticParams.szDecoyPrefix);
   fprintf(fptr, "Enzyme: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzymeName,
      g_staticParams.enzymeInformation.iSearchEnzymeOffSet,
      g_staticParams.enzymeInformation.szSearchEnzymeBreakAA,
      g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
   fprintf(fptr, "Enzyme2: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzyme2Name,
      g_staticParams.enzymeInformation.iSearchEnzyme2OffSet,
      g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA,
      g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
   // Digestion-time settings baked into which peptides exist in the raw-peptide table below;
   // restored so pepXML/mzIdentML's search_summary/SpectrumIdentificationProtocol enzyme
   // metadata (which reads these live at search time) can't silently disagree with what this
   // .idx was actually built with.
   fprintf(fptr, "NumEnzymeTermini: %d\n", g_staticParams.options.iEnzymeTermini);
   fprintf(fptr, "AllowedMissedCleavage: %d\n", g_staticParams.enzymeInformation.iAllowedMissedCleavage);
   fprintf(fptr, "ClipNtermMethionine: %d\n", g_staticParams.options.bClipNtermMet ? 1 : 0);
   fprintf(fptr, "NumPeptides: %ld\n", (long)g_vRawPeptides.size());

   // write out static mod params A to Z is ascii 65 to 90 then terminal mods
   fprintf(fptr, "StaticMod:");
   for (int x = 65; x <= 90; x++)
      fprintf(fptr, " %lf", g_staticParams.staticModifications.pdStaticMods[x]);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddNterminusPeptide);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddCterminusPeptide);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddNterminusProtein);
   fprintf(fptr, " %lf\n", g_staticParams.staticModifications.dAddCterminusProtein);

   // Restore of the pre-Phase-0.5 variable-mod header lines (docs/20260811_restore_idx_header_mods.md)
   // -- an .idx built from these settings carries them permanently, so a later search against
   // this file needs no variable_modNN/require_variable_mod/protein_modslist_file/
   // max_variable_mods_in_peptide params of its own (ParsePeptideIndexHeader() below overwrites
   // whatever comet.params/RTS SetParam() supplied, the same override precedent StaticMod: above
   // already established).
   //
   // v4 adds a 5th :-delimited field per slot -- iMaxNumVarModAAPerMod, the per-mod count cap --
   // and the new MaxVariableModsInPeptide: line below for the global cap. Neither pre-121 format
   // nor this format's own v3 predecessor ever persisted these (confirmed against v2025.03.0):
   // CometFragmentIndex::PermuteIndexPeptideMods()/CometModificationsPermuter always read them
   // from whatever was live in g_staticParams at search time, from comet.params/SetParam() --
   // this is the first version where an index is self-consistent for its own mod *counts*, not
   // just mod *identity*. iVarModTermDistance/iWhichTerm (peptide/protein N/C-term mod
   // restriction) remain unsupported for FI_DB/PI_DB and are not persisted here either --
   // CometFragmentIndex.cpp/CometModificationsPermuter.cpp/CometPeptideIndex.cpp have never
   // referenced either field; only the plain-FASTA search path (CometSearch.cpp) enforces them.
   fprintf(fptr, "VariableMod:");
   for (int x = 0; x < FRAGINDEX_VMODS; ++x)
   {
      fprintf(fptr, " %s:%lf:%lf:%lf:%d",
         g_staticParams.variableModParameters.varModList[x].szVarModChar,
         g_staticParams.variableModParameters.varModList[x].dVarModMass,
         g_staticParams.variableModParameters.varModList[x].dNeutralLoss,
         g_staticParams.variableModParameters.varModList[x].dNeutralLoss2,
         g_staticParams.variableModParameters.varModList[x].iMaxNumVarModAAPerMod);
   }
   fprintf(fptr, "\n");

   fprintf(fptr, "ProteinModList: %d\n", g_staticParams.variableModParameters.bVarModProteinFilter ? 1 : 0);

   fprintf(fptr, "RequireVariableMod: %d", g_staticParams.variableModParameters.iRequireVarMod);
   for (int x = 0; x < FRAGINDEX_VMODS; ++x)
      fprintf(fptr, " %d", g_staticParams.variableModParameters.varModList[x].iRequireThisMod);
   fprintf(fptr, "\n");

   fprintf(fptr, "MaxVariableModsInPeptide: %d\n\n", g_staticParams.variableModParameters.iMaxVarModPerPeptide);

   int iTmp = (int)g_pvProteinNames.size();
   comet_fileoffset_t* lProteinIndex = new comet_fileoffset_t[iTmp];
   for (int i = 0; i < iTmp; i++)
      lProteinIndex[i] = -1;

   // first just write out protein names. Track file position of each protein name
   int ctProteinNames = 0;
   for (auto it = g_pvProteinNames.begin(); it != g_pvProteinNames.end(); ++it)
   {
      lProteinIndex[ctProteinNames] = comet_ftell(fptr);
      fwrite(it->second.szProt, sizeof(char) * WIDTH_REFERENCE, 1, fptr);
      it->second.iWhichProtein = ctProteinNames;
      ctProteinNames++;
   }

   // --- Raw peptide table: one entry per unique unmodified peptide, sequence/protein/flank
   // data shared by every mod-variant of that peptide. ---
   comet_fileoffset_t clPeptidesFilePos = comet_ftell(fptr);
   uint64_t tNumRaw = (uint64_t)g_vRawPeptides.size();
   fwrite(&tNumRaw, sizeof(uint64_t), 1, fptr);
   for (const auto raw : g_vRawPeptides)
   {
      int iLen = raw.iLen;
      fwrite(&iLen, sizeof(int), 1, fptr);
      fwrite(raw.szPeptide, sizeof(char), iLen, fptr);
      fwrite(&raw.cPrevAA, sizeof(char), 1, fptr);
      fwrite(&raw.cNextAA, sizeof(char), 1, fptr);
      fwrite(&raw.dPepMass, sizeof(double), 1, fptr);
      fwrite(&raw.siVarModProteinFilter, sizeof(unsigned short), 1, fptr);
      fwrite(&raw.lIndexProteinFilePosition, sizeof(comet_fileoffset_t), 1, fptr);
   }

   // --- Proteins list (ProteinsListCSR, already built by Phase A -- see the no-dedup-needed
   // comment above for why no local rebuild is needed here). ---
   comet_fileoffset_t clProteinsFilePos = comet_ftell(fptr);
   size_t tTmp = g_pvProteinsList.size();
   int iWhichProtein;
   fwrite(&tTmp, clSizeCometFileOffset, 1, fptr);
   for (size_t iRow = 0; iRow < g_pvProteinsList.size(); ++iRow)
   {
      ProteinsListCSR::Row row = g_pvProteinsList[iRow];
      tTmp = row.size();
      fwrite(&tTmp, clSizeCometFileOffset, 1, fptr);

      for (size_t it2 = 0; it2 < tTmp; ++it2)
      {
         iWhichProtein = -1;

         // find protein by matching g_pvProteinNames.lProteinFilePosition to g_pvProteinNames.lProteinIndex;
         auto result = g_pvProteinNames.find(row[it2]);
         if (result != g_pvProteinNames.end())
         {
            iWhichProtein = result->second.iWhichProtein;
         }

         if (iWhichProtein == -1)
         {
            string strErrorMsg = " Error in WritePeptideIndex(): cannot find protein file position in protein names map.\n";
            logerr(strErrorMsg);
            fclose(fptr);
            remove(strIndexFile.c_str());
            delete[] lProteinIndex;
            return false;
         }

         fwrite(&lProteinIndex[iWhichProtein], clSizeCometFileOffset, 1, fptr);
      }
   }

   delete[] lProteinIndex;

   // Footer: two section-start pointers, read from EOF by ReadPeptideIndex(). Each section's
   // size is derived at read time from the distance to the next pointer (or to the footer
   // itself, for the last section), so no section needs its own stored count beyond what it
   // already writes inline (raw peptide count) for reserve()/loop-bound purposes. No
   // permutation-table or compact-variant-array sections any more (Phase 0.5) -- neither is
   // persisted, see the comment above the magic-string line for why.
   fwrite(&clPeptidesFilePos, clSizeCometFileOffset, 1, fptr);
   fwrite(&clProteinsFilePos, clSizeCometFileOffset, 1, fptr);

   // ferror() reflects the OR of every fwrite/fprintf's error state since the stream was
   // opened, so one check here covers the entire header/protein-name/raw-peptide/
   // proteins-list/footer write sequence above without needing a per-call check on each of
   // the dozens of individual fwrite()s -- disk-full (or any other write failure) mid-build
   // previously went undetected, silently reporting success with a truncated index (the
   // exact failure mode CLAUDE.md's T24 note describes).
   bool bWriteError = (ferror(fptr) != 0);
   fclose(fptr);

   if (bWriteError)
   {
      string strErrorMsg = " Error - failed writing \"" + strIndexFile + "\" (disk full?); "
         "the partial file has been removed. Free up space and rebuild with -i or -j.\n";
      logerr(strErrorMsg);
      remove(strIndexFile.c_str());
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   std::string strNumPeps;
   if (tNumRaw > 1e6)
   {
      std::ostringstream oss;
      oss << std::scientific << std::setprecision(3) << static_cast<double>(tNumRaw);
      strNumPeps = oss.str();
   }
   else
   {
      strNumPeps = std::to_string(tNumRaw);
   }

   string strOut = " - created: " + strIndexFile + " (" + strNumPeps + " unmodified peptides)\n";
   strOut += " - done. (" + CometMassSpecUtils::ElapsedTime(tPeptideIndexStartTime);

   string strMem = CometMassSpecUtils::GetPeakMemory();
   if (!strMem.empty())
      strOut += ", " + strMem + ")";
   else
      strOut += ")";

   strOut += "\n\n";

   logout(strOut);
   fflush(stdout);

   CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

   return bSucceeded;
}


// Parses the .idx text header (magic/version, IndexSearchType:, MassRange:,
// LengthRange:, MassType:, StaticMod:, VariableMod:, ProteinModList:,
// RequireVariableMod:, MaxVariableModsInPeptide:, DecoySearch:, DecoyPrefix:, Enzyme:,
// Enzyme2:, NumEnzymeTermini:, AllowedMissedCleavage:, ClipNtermMethionine:) from fp into
// g_staticParams. Reads until the blank line that separates the header from the
// protein-name section. Restored by docs/20260811_restore_idx_header_mods.md to be the
// single authoritative source for static AND variable-mod settings, including (v4) the
// per-mod/global mod *count* limits (Phase 0.5 had dropped the mod-identity lines
// entirely, requiring search-time comet.params/RTS SetParam() calls instead; the count
// limits were never persisted even pre-121 -- see that doc's history section) -- an .idx
// built today is fully self-contained and needs none of that at search time.
// DecoyPrefix:/NumEnzymeTermini:/AllowedMissedCleavage:/ClipNtermMethionine: are a later
// addition (2026-08-21) closing a gap the original restoration left: decoy_prefix is
// what AnalyzePeptideIndex() (PI_DB) uses to classify a candidate as target vs. decoy by
// its stored protein name, and the other three are digestion-time settings already baked
// into which peptides exist -- none of the four were previously self-describing, so a
// search-time comet.params/SetParam() mismatch could silently corrupt the PI_DB target/
// decoy split (decoy_prefix) or misreport enzyme metadata in pepXML/mzIdentML output
// (the other three). All four are optional in the parse (no bFound.../error-if-missing
// gate, matching DecoySearch:/Enzyme:/Enzyme2:'s existing precedent) so older .idx files
// built before this addition still load, just without this restore.
// iVarModTermDistance/iWhichTerm remain unsupported for FI_DB/PI_DB (never referenced by
// CometFragmentIndex.cpp/CometModificationsPermuter.cpp/CometPeptideIndex.cpp) and are
// not part of the header.
//
// Also validates the magic string/version (rejecting anything other than the current
// "Comet index database v4" with a clear rebuild message) and parses IndexSearchType:
// into g_staticParams.iDbType (PI_DB vs FI_DB), so this same helper is what makes an
// existing .idx file self-describing -- see WritePeptideIndex()'s header-writing
// comment for the full rationale.
//
// Both EnsurePeptideIndexLoaded() (via ReadPeptideIndex()) and
// InitializeMassesFromPeptideIndex() call this helper so that any future
// .idx header format changes only need to be made in one place.
//
// IMPORTANT: resets pdAAMassFragment AND pdAAMassParent via AssignMass()
// before applying static mods, so this is safe to call whether or not
// InitializeStaticParams() has already applied static mods.
bool CometPeptideIndex::ParsePeptideIndexHeader(FILE* fp)
{
   char szBuf[SIZE_BUF];
   bool bFoundStatic = false;

   // Ignore any static/variable mod settings from comet.params (batch) or the RTS
   // SetParam() calls; only the values baked into the .idx header are authoritative
   // for an index search (docs/20260811_restore_idx_header_mods.md).
   memset(g_staticParams.staticModifications.pdStaticMods, 0,
      sizeof(g_staticParams.staticModifications.pdStaticMods));

   for (int x = 0; x < FRAGINDEX_VMODS; ++x)
   {
      g_staticParams.variableModParameters.varModList[x].dVarModMass = 0.0;
      g_staticParams.variableModParameters.varModList[x].dNeutralLoss = 0.0;
      g_staticParams.variableModParameters.varModList[x].dNeutralLoss2 = 0.0;
      g_staticParams.variableModParameters.varModList[x].iRequireThisMod = 0;
      g_staticParams.variableModParameters.varModList[x].iMaxNumVarModAAPerMod = 0;
      g_staticParams.variableModParameters.varModList[x].bNtermMod = false;
      g_staticParams.variableModParameters.varModList[x].bCtermMod = false;
      strcpy(g_staticParams.variableModParameters.varModList[x].szVarModChar, "X");
   }
   g_staticParams.variableModParameters.bVarModSearch = false;
   g_staticParams.variableModParameters.bUseFragmentNeutralLoss = false;
   g_staticParams.variableModParameters.bVarModProteinFilter = false;
   g_staticParams.variableModParameters.bVarTermModSearch = false;
   g_staticParams.variableModParameters.iRequireVarMod = 0;
   g_staticParams.variableModParameters.iMaxVarModPerPeptide = 0;

   rewind(fp);

   if (fgets(szBuf, SIZE_BUF, fp) == NULL
      || strncmp(szBuf, "Comet index database v4", sizeof("Comet index database v4") - 1) != 0)
   {
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" is not a v4 unified index file; rebuild it with -i or -j.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   bool bFoundIndexSearchType = false;
   bool bFoundMaxVarModsInPeptide = false;

   while (fgets(szBuf, SIZE_BUF, fp))
   {
      // Blank line: end of header, start of the protein-name section. MaxVariableModsInPeptide:
      // is always the last populated header line now, so this is the only terminator this
      // loop needs.
      if (szBuf[0] == '\n' || szBuf[0] == '\r')
         break;

      if (!strncmp(szBuf, "IndexSearchType:", 16))
      {
         char szType[32] = "";
         sscanf(szBuf + 16, " %31[^\n\r]", szType);

         if (!strcmp(szType, "peptide index"))
            g_staticParams.iDbType = DbType::PI_DB;
         else if (!strcmp(szType, "fragment ion index"))
            g_staticParams.iDbType = DbType::FI_DB;
         else
         {
            string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
               + "\" has an unrecognized IndexSearchType: value \"" + string(szType) + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         bFoundIndexSearchType = true;
      }
      else if (!strncmp(szBuf, "MassRange:", 10))
      {
         // Peptides outside this range were never generated when the index was built
         // (Phase A/B, WritePeptideIndex()) -- a search-time digest_mass_range wider than
         // this has nothing to admit beyond what's already in the file, so only clamp
         // inward: a narrower search-time range further restricts g_massRange (applied by
         // both PI_DB's SearchPeptideIndex() and FI_DB's AddFragments()); a wider one is a
         // silent no-op, not an error.
         double dIdxMassLow = 0.0, dIdxMassHigh = 0.0;
         sscanf(szBuf + 10, "%lf %lf", &dIdxMassLow, &dIdxMassHigh);

         if (g_massRange.dMinMass < dIdxMassLow)
            g_massRange.dMinMass = dIdxMassLow;
         if (g_massRange.dMaxMass > dIdxMassHigh)
            g_massRange.dMaxMass = dIdxMassHigh;
      }
      else if (!strncmp(szBuf, "LengthRange:", 12))
      {
         // Same inward-only clamp as MassRange: above, applied to peptide length instead
         // of mass.
         int iIdxLenStart = 0, iIdxLenEnd = 0;
         sscanf(szBuf + 12, "%d %d", &iIdxLenStart, &iIdxLenEnd);

         if (g_staticParams.options.peptideLengthRange.iStart < iIdxLenStart)
            g_staticParams.options.peptideLengthRange.iStart = iIdxLenStart;
         if (g_staticParams.options.peptideLengthRange.iEnd > iIdxLenEnd)
            g_staticParams.options.peptideLengthRange.iEnd = iIdxLenEnd;
      }
      else if (!strncmp(szBuf, "MassType:", 9))
      {
         sscanf(szBuf + 10, "%d %d",
            &g_staticParams.massUtility.bMonoMassesParent,
            &g_staticParams.massUtility.bMonoMassesFragment);
      }
      else if (!strncmp(szBuf, "StaticMod:", 10))
      {
         char* tok;
         char* saveptr = NULL;   // strtok_r context, not shared/reentrant-unsafe like plain strtok's
         char  delims[] = " ";
         int   x = 65;  // ASCII 'A'

         // Reset BOTH mass arrays to bare (unmodified) residue masses before
         // adding the static mods stored in the .idx header.  This prevents
         // double-application when InitializeStaticParams() has already added
         // static mods to pdAAMassParent.
         CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassFragment,
            g_staticParams.massUtility.bMonoMassesFragment,
            &g_staticParams.massUtility.dOH2fragment);

         CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassParent,
            g_staticParams.massUtility.bMonoMassesParent,
            &g_staticParams.massUtility.dOH2parent);

         bFoundStatic = true;
         tok = strtok_r(szBuf + 11, delims, &saveptr);
         while (tok != NULL)
         {
            sscanf(tok, "%lf", &(g_staticParams.staticModifications.pdStaticMods[x]));
            g_staticParams.massUtility.pdAAMassFragment[x] += g_staticParams.staticModifications.pdStaticMods[x];
            g_staticParams.massUtility.pdAAMassParent[x] += g_staticParams.staticModifications.pdStaticMods[x];
            tok = strtok_r(NULL, delims, &saveptr);
            x++;
            // 65-90 = A-Z; 91-94 = n/c-term peptide, n/c-term protein
            if (x == 95)
               break;
         }

         g_staticParams.staticModifications.dAddNterminusPeptide = g_staticParams.staticModifications.pdStaticMods[91];
         g_staticParams.staticModifications.dAddCterminusPeptide = g_staticParams.staticModifications.pdStaticMods[92];
         g_staticParams.staticModifications.dAddNterminusProtein = g_staticParams.staticModifications.pdStaticMods[93];
         g_staticParams.staticModifications.dAddCterminusProtein = g_staticParams.staticModifications.pdStaticMods[94];

         // Recalculate the precalculated masses that depend on terminal static mods.
         g_staticParams.precalcMasses.dNtermProton =
            g_staticParams.staticModifications.dAddNterminusPeptide + PROTON_MASS;

         g_staticParams.precalcMasses.dCtermOH2Proton =
            g_staticParams.staticModifications.dAddCterminusPeptide
            + g_staticParams.massUtility.dOH2fragment
            + PROTON_MASS;

         g_staticParams.precalcMasses.dOH2ProtonCtermNterm =
            g_staticParams.massUtility.dOH2parent
            + PROTON_MASS
            + g_staticParams.staticModifications.dAddCterminusPeptide
            + g_staticParams.staticModifications.dAddNterminusPeptide;
      }
      else if (!strncmp(szBuf, "VariableMod:", 12))
      {
         string strMods = szBuf + 13;
         istringstream iss(strMods);
         int iNumMods = 0;
         string subStr;

         // while (iss >> subStr), not do/while: extraction failure (fewer tokens than
         // FRAGINDEX_VMODS on a truncated line) must stop the loop immediately rather than
         // running the body once more on an empty subStr -- the header is authoritative now
         // (docs/20260811_restore_idx_header_mods.md), so a malformed line has to fail the
         // parse, not silently leave a slot at its reset-to-default identity.
         while (iNumMods < FRAGINDEX_VMODS && (iss >> subStr))
         {
            // colon-delimited quintuplet (v4): mod_chars:mass:NL1:NL2:maxPerMod. %31s caps
            // szVarModChar's write at its declared size (MAX_VARMOD_AA, CometData.h) --
            // sscanf's %s is otherwise unbounded and this field comes straight from the file.
            std::replace(subStr.begin(), subStr.end(), ':', ' ');
            if (sscanf(subStr.c_str(), "%31s %lf %lf %lf %d",
                  g_staticParams.variableModParameters.varModList[iNumMods].szVarModChar,
                  &(g_staticParams.variableModParameters.varModList[iNumMods].dVarModMass),
                  &(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss),
                  &(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss2),
                  &(g_staticParams.variableModParameters.varModList[iNumMods].iMaxNumVarModAAPerMod)) != 5)
            {
               string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
                  + "\" has a malformed VariableMod: entry (slot " + to_string(iNumMods)
                  + "): \"" + subStr + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               return false;
            }

            if (!isEqual(g_staticParams.variableModParameters.varModList[iNumMods].dVarModMass, 0.0))
               g_staticParams.variableModParameters.bVarModSearch = true;

            if (!isEqual(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss, 0.0))
               g_staticParams.variableModParameters.bUseFragmentNeutralLoss = true;

            // bNtermMod/bCtermMod/bVarTermModSearch gate AddFragmentsThreadProc()'s and
            // EnumerateIndexPeptideMods()'s terminal-mod enumeration (CometFragmentIndex.cpp,
            // CometPeptideIndex.cpp) -- unlike iWhichTerm/iVarModTermDistance (peptide vs.
            // protein N/C-term restriction), which FI_DB/PI_DB never reference at all, these
            // three must be derived from the .idx header's szVarModChar the same way
            // InitializeStaticParams() derives them from comet.params, or an index built with
            // an n/c-term variable mod silently searches without it.
            if (strchr(g_staticParams.variableModParameters.varModList[iNumMods].szVarModChar, 'n'))
            {
               g_staticParams.variableModParameters.varModList[iNumMods].bNtermMod = true;
               g_staticParams.variableModParameters.bVarTermModSearch = true;
            }

            if (strchr(g_staticParams.variableModParameters.varModList[iNumMods].szVarModChar, 'c'))
            {
               g_staticParams.variableModParameters.varModList[iNumMods].bCtermMod = true;
               g_staticParams.variableModParameters.bVarTermModSearch = true;
            }

            iNumMods++;
         }

         if (iNumMods != FRAGINDEX_VMODS)
         {
            string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
               + "\" has a truncated VariableMod: line (expected " + to_string(FRAGINDEX_VMODS)
               + " entries, found " + to_string(iNumMods) + ").\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }
      else if (!strncmp(szBuf, "ProteinModList:", 15))
      {
         int iTmp = 0;
         sscanf(szBuf + 16, "%d", &iTmp);

         if (iTmp)
            g_staticParams.variableModParameters.bVarModProteinFilter = true;
      }
      else if (!strncmp(szBuf, "RequireVariableMod:", 19))
      {
         string strMods = szBuf + 20;
         istringstream iss(strMods);
         int iNumMods = 0;
         string subStr;

         // while (iss >> subStr), not do/while -- see the identical comment on VariableMod:'s
         // parse above for why: a truncated line must fail the parse, not silently stop mid-way
         // through with iRequireVarMod/iRequireThisMod partially populated.
         while (iNumMods < FRAGINDEX_VMODS + 1 && (iss >> subStr))
         {
            int iIntData = 0;

            if (sscanf(subStr.c_str(), "%d", &iIntData) != 1)
            {
               string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
                  + "\" has a malformed RequireVariableMod: entry (slot " + to_string(iNumMods)
                  + "): \"" + subStr + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               return false;
            }

            if (iNumMods == 0)   // first value is the global require-variable-mod flag
            {
               if (iIntData > 0)
                  g_staticParams.variableModParameters.iRequireVarMod |= 1UL << 0;
               else
                  g_staticParams.variableModParameters.iRequireVarMod = 0;
            }
            else   // subsequent values are per variable mod
            {
               if (iIntData > 0)
                  g_staticParams.variableModParameters.iRequireVarMod |= 1UL << iNumMods;
               g_staticParams.variableModParameters.varModList[iNumMods - 1].iRequireThisMod = iIntData;
            }

            iNumMods++;
         }

         if (iNumMods != FRAGINDEX_VMODS + 1)
         {
            string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
               + "\" has a truncated RequireVariableMod: line (expected " + to_string(FRAGINDEX_VMODS + 1)
               + " entries, found " + to_string(iNumMods) + ").\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }
      else if (!strncmp(szBuf, "MaxVariableModsInPeptide:", 25))
      {
         if (sscanf(szBuf + 25, "%d", &(g_staticParams.variableModParameters.iMaxVarModPerPeptide)) != 1)
         {
            string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
               + "\" has a malformed MaxVariableModsInPeptide: line.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         bFoundMaxVarModsInPeptide = true;
      }
      else if (!strncmp(szBuf, "DecoySearch:", 12))
      {
         sscanf(szBuf, "DecoySearch: %d", &(g_staticParams.options.iDecoySearch));
      }
      else if (!strncmp(szBuf, "DecoyPrefix:", 12))
      {
         // Authoritative for PI_DB: AnalyzePeptideIndex() classifies a candidate as
         // target vs. decoy by matching its stored protein name against this prefix, so
         // it must match what the .idx was actually built with, not whatever comet.params/
         // SetParam() happens to supply at search time (see the write-side comment above).
         char szTmp[256] = "";
         if (sscanf(szBuf + 12, " %255[^\n\r]", szTmp) == 1)
         {
            strncpy(g_staticParams.szDecoyPrefix, szTmp, sizeof(g_staticParams.szDecoyPrefix) - 1);
            g_staticParams.szDecoyPrefix[sizeof(g_staticParams.szDecoyPrefix) - 1] = '\0';
            g_staticParams.sDecoyPrefix = g_staticParams.szDecoyPrefix;
            CometMassSpecUtils::EscapeString(g_staticParams.sDecoyPrefix);
         }
      }
      else if (!strncmp(szBuf, "Enzyme:", 7))
      {
         // Width-limited like the VariableMod:/RequireVariableMod: parses above --
         // szSearchEnzymeName is ENZYME_NAME_LEN (48) and szSearchEnzymeBreakAA/
         // szSearchEnzymeNoBreakAA are MAX_ENZYME_AA (20); a bare %s here let an
         // oversized file-supplied token overflow these fixed buffers.
         sscanf(szBuf, "Enzyme: %47s [%d %19s %19s]",
            g_staticParams.enzymeInformation.szSearchEnzymeName,
            &(g_staticParams.enzymeInformation.iSearchEnzymeOffSet),
            g_staticParams.enzymeInformation.szSearchEnzymeBreakAA,
            g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
      }
      else if (!strncmp(szBuf, "Enzyme2:", 8))
      {
         sscanf(szBuf, "Enzyme2: %47s [%d %19s %19s]",
            g_staticParams.enzymeInformation.szSearchEnzyme2Name,
            &(g_staticParams.enzymeInformation.iSearchEnzyme2OffSet),
            g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA,
            g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
      }
      else if (!strncmp(szBuf, "NumEnzymeTermini:", 17))
      {
         // Digestion-time only (baked into which peptides exist in the raw-peptide table);
         // restored purely so pepXML/mzIdentML's enzyme metadata can't disagree with what
         // this .idx was actually built with (see the write-side comment above).
         sscanf(szBuf + 17, "%d", &(g_staticParams.options.iEnzymeTermini));
      }
      else if (!strncmp(szBuf, "AllowedMissedCleavage:", 22))
      {
         sscanf(szBuf + 22, "%d", &(g_staticParams.enzymeInformation.iAllowedMissedCleavage));
      }
      else if (!strncmp(szBuf, "ClipNtermMethionine:", 20))
      {
         int iTmp = 0;
         if (sscanf(szBuf + 20, "%d", &iTmp) == 1)
            g_staticParams.options.bClipNtermMet = (iTmp != 0);
      }
   }

   if (!bFoundIndexSearchType)
   {
      string strErrorMsg = " Error with index database format. IndexSearchType: line not found.\n";
      logerr(strErrorMsg);
      return false;
   }

   if (!bFoundStatic)
   {
      string strErrorMsg = " Error with index database format. StaticMod: line not found.\n";
      logerr(strErrorMsg);
      return false;
   }

   if (!bFoundMaxVarModsInPeptide)
   {
      string strErrorMsg = " Error with index database format. MaxVariableModsInPeptide: line not found.\n";
      logerr(strErrorMsg);
      return false;
   }

   return true;
}
