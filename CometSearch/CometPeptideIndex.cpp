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

extern comet_fileoffset_t clSizeCometFileOffset;


CometPeptideIndex::CometPeptideIndex()
{
}

CometPeptideIndex::~CometPeptideIndex()
{
}

// Read the compact peptide index (.idx) file into global read-only structures:
//   g_vRawPeptides      - one entry per unique unmodified peptide (sequence, protein
//                         reference, flank AAs, unmodified mass); shared across every
//                         mod-variant of that peptide
//   g_vDBIndexVariants  - one compact entry per (peptide, mod combination) pair (mass plus
//                         a reference back into g_vRawPeptides), sorted by mass. A full
//                         DBIndex is materialized on demand from one of these entries via
//                         CometPeptideIndex::MaterializeOneEntry(), called once per
//                         mass-window candidate from CometSearch::SearchPeptideIndex() --
//                         see docs/20260730_PI_reduction.md.
//   g_pvProteinsList    - vector-of-vectors mapping peptide to protein file positions
//   g_pvProteinNames    - map of file offset to protein name string
//   g_bPeptideIndexRead - guard flag
//
// The .idx binary layout (written by WritePeptideIndex()), read in this exact order --
// purely sequentially from the protein-names/proteins-list boundary onward, unlike the
// pre-v2 format's footer-driven mass-index/peptide-block seeking:
//   [text header lines ending with blank line]
//   [protein names: each WIDTH_REFERENCE chars -- length/count not stored; only ever
//    addressed via the file offsets embedded in the proteins list below, never iterated]
//   [proteins list: count then per-entry (size + file offsets)]
//   [raw peptide count (uint64_t), then per-entry: iLen(int), szPeptide(iLen chars),
//    cPrevAA(char), cNextAA(char), dPepMass(double), lIndexProteinFilePosition(comet_fileoffset_t)]
//   [variant count (uint64_t), then per-entry: dPepMass(double), iWhichPeptide(unsigned int),
//    modNumIdx(int), cNtermMod(char), cCtermMod(char)]
//   [footer: clProteinsFilePos(comet_fileoffset_t) -- the one seek target needed, to jump
//    from end-of-header past the protein-names region to the start of the proteins list]
//
// Does NOT rebuild the MOD_NUMBERS/MOD_SEQS/PEPTIDE_MOD_SEQ_IDXS tables the compact variant
// entries reference by index -- that must happen after this function returns, once
// InitializeMassesFromPeptideIndex()'s call to ParsePeptideIndexHeader() has overwritten
// g_staticParams.variableModParameters with this .idx file's baked-in build-time mod
// settings (both ReadPeptideIndex() callers -- CometSearch::EnsurePeptideIndexLoaded() and
// InitializeSingleSpectrumSearch() -- call InitializeMassesFromPeptideIndex() immediately
// after this function). Calling CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides)
// any earlier would build those tables from whatever mod settings happened to be active
// from the search-time comet.params instead -- silently wrong if they differ from what the
// index was built with (see docs/20260730_PI_reduction.md Section 8, Open Question 2, and
// InitializeMassesFromPeptideIndex()'s own call to PermuteIndexPeptideMods() in CometSearch.cpp).
bool CometPeptideIndex::ReadPeptideIndex(bool bIsRTS)
{
   (void)bIsRTS;   // reserved for RTS-vs-batch-specific behavior; not yet used

   if (g_bPeptideIndexRead)
      return true;

   FILE* fp;
   char szBuf[SIZE_BUF];

   if ((fp = fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
   {
      string strErrorMsg = " Error - cannot open peptide index file \""
         + string(g_staticParams.databaseInfo.szDatabase) + "\" for reading.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Verify this is a v2 (compact-format) peptide index file. Older v1 files (one fully
   // materialized DBIndex -- duplicated sequence + explicit mod-site array -- per modified
   // peptide) are deliberately rejected rather than misread; rebuild with -j to get a v2
   // file (docs/20260730_PI_reduction.md Section 5).
   if (fgets(szBuf, SIZE_BUF, fp) == NULL)
   {
      fclose(fp);
      return false;
   }
   if (strncmp(szBuf, "Comet peptide index database v2", sizeof("Comet peptide index database v2") - 1) != 0)
   {
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" is not a v2 peptide index file; rebuild it with -j.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      fclose(fp);
      return false;
   }

   // Skip remaining header lines until blank line
   while (fgets(szBuf, SIZE_BUF, fp) != NULL)
   {
      if (szBuf[0] == '\n' || szBuf[0] == '\r')
         break;
   }

   // --- Read the one-value footer, then seek past the protein-names region ---
   comet_fseek(fp, -1 * (comet_fileoffset_t)clSizeCometFileOffset, SEEK_END);

   comet_fileoffset_t clProteinsFilePos;
   (void)fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);

   comet_fseek(fp, clProteinsFilePos, SEEK_SET);

   // --- Read proteins list (ProteinsListCSR) ---
   size_t tNumProteinEntries;
   (void)fread(&tNumProteinEntries, clSizeCometFileOffset, 1, fp);

   // Read directly into flat CSR staging buffers instead of one throwaway
   // vector<comet_fileoffset_t> per row -- avoids tNumProteinEntries individual
   // heap allocations (each immediately freed by ProteinsListCSR::push_back's
   // swap-to-release), the same per-row allocation cost append_flat() was
   // built to eliminate on the build side (see its comment in core/Types.h).
   vector<comet_fileoffset_t> vFlatProteinOffsets;
   vector<uint32_t> vProteinCounts;
   vProteinCounts.reserve(tNumProteinEntries);

   for (size_t i = 0; i < tNumProteinEntries; ++i)
   {
      size_t tNumProteins;
      (void)fread(&tNumProteins, clSizeCometFileOffset, 1, fp);

      size_t tOldSize = vFlatProteinOffsets.size();
      vFlatProteinOffsets.resize(tOldSize + tNumProteins);
      (void)fread(&vFlatProteinOffsets[tOldSize], clSizeCometFileOffset, tNumProteins, fp);

      vProteinCounts.push_back((uint32_t)tNumProteins);
   }

   g_pvProteinsList.clear();
   g_pvProteinsList.reserve(tNumProteinEntries);
   g_pvProteinsList.append_flat(vFlatProteinOffsets, vProteinCounts);

   // --- Read the raw-peptide table (sequential; file position is already right after the
   // proteins list, exactly where WritePeptideIndex() started writing it) ---
   uint64_t tNumRaw;
   (void)fread(&tNumRaw, sizeof(uint64_t), 1, fp);

   g_vRawPeptides.clear();
   g_vRawPeptides.reserve((size_t)tNumRaw);

   for (uint64_t i = 0; i < tNumRaw; ++i)
   {
      PlainPeptideIndexStruct sRaw;
      int iLen;
      size_t tTmp;

      tTmp = fread(&iLen, sizeof(int), 1, fp);
      if (tTmp != 1) iLen = -1;
      if (iLen < 0 || iLen >= MAX_PEPTIDE_LEN || fread(sRaw.szPeptide, sizeof(char), (size_t)iLen, fp) != (size_t)iLen)
      {
         g_vRawPeptides.clear();
         fclose(fp);
         logout(" Error - failed to read raw peptide entry " + to_string(i)
            + " from .idx file; file may be truncated or corrupt.\n");
         return false;
      }
      sRaw.szPeptide[iLen] = '\0';

      if (fread(&sRaw.cPrevAA, sizeof(char), 1, fp) != 1
         || fread(&sRaw.cNextAA, sizeof(char), 1, fp) != 1
         || fread(&sRaw.dPepMass, sizeof(double), 1, fp) != 1
         || fread(&sRaw.lIndexProteinFilePosition, sizeof(comet_fileoffset_t), 1, fp) != 1)
      {
         g_vRawPeptides.clear();
         fclose(fp);
         logout(" Error - failed to read raw peptide entry " + to_string(i)
            + " from .idx file; file may be truncated or corrupt.\n");
         return false;
      }
      sRaw.siVarModProteinFilter = 0;  // not persisted -- only meaningful during index build

      g_vRawPeptides.push_back(sRaw);
   }

   // --- Read the compact variant array (sequential; already sorted by mass) ---
   uint64_t tNumVariants;
   (void)fread(&tNumVariants, sizeof(uint64_t), 1, fp);

   g_vDBIndexVariants.clear();
   g_vDBIndexVariants.reserve((size_t)tNumVariants);

   for (uint64_t i = 0; i < tNumVariants; ++i)
   {
      FragmentPeptidesStruct sVariant;

      if (fread(&sVariant.dPepMass, sizeof(double), 1, fp) != 1
         || fread(&sVariant.iWhichPeptide, sizeof(unsigned int), 1, fp) != 1
         || fread(&sVariant.modNumIdx, sizeof(int), 1, fp) != 1
         || fread(&sVariant.cNtermMod, sizeof(char), 1, fp) != 1
         || fread(&sVariant.cCtermMod, sizeof(char), 1, fp) != 1)
      {
         g_vRawPeptides.clear();
         g_vDBIndexVariants.clear();
         fclose(fp);
         logout(" Error - failed to read peptide index variant " + to_string(i)
            + " from .idx file; file may be truncated or corrupt.\n");
         return false;
      }

      g_vDBIndexVariants.push_back(sVariant);
   }

   // Build in-memory protein name cache before closing the file.
   {
      char szProtBuf[WIDTH_REFERENCE];
      g_pvProteinNameCache.clear();
      for (const auto& vProts : g_pvProteinsList)
      {
         for (const comet_fileoffset_t lOffset : vProts)
         {
            if (g_pvProteinNameCache.find(lOffset) == g_pvProteinNameCache.end())
            {
               comet_fseek(fp, lOffset, SEEK_SET);
               if (fread(szProtBuf, sizeof(char), WIDTH_REFERENCE, fp) == (size_t)WIDTH_REFERENCE)
               {
                  szProtBuf[WIDTH_REFERENCE - 1] = '\0';
                  g_pvProteinNameCache.emplace(lOffset, string(szProtBuf, strnlen(szProtBuf, WIDTH_REFERENCE - 1)));
               }
            }
         }
      }
   }

   fclose(fp);

   // CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides) intentionally NOT called
   // here -- see the comment above this function's signature for why it must wait until
   // after InitializeMassesFromPeptideIndex() applies this .idx file's mod settings.

   if (bIsRTS)
   {
      logout(" Read peptide index: " + to_string(tNumVariants) + " peptides, "
         + to_string(tNumProteinEntries) + " protein groups\n");
   }
   else
   {
      logout("\n   - Read peptide index: " + to_string(tNumVariants) + " peptides, "
         + to_string(tNumProteinEntries) + " protein groups\n");
   }

   g_staticParams.iDbType = DbType::PI_DB;
   g_bPeptideIndexRead = true;

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
// skipping inactive/blank slots, so MOD_NUMBERS[].modifications[] values are indices
// into that compacted list, not direct varModList slot indices. vModSlotForAllModsIdx
// rebuilds the exact same compaction order as
// CometFragmentIndex::PermuteIndexPeptideMods()'s ALL_MODS-building loop to translate
// between the two -- must stay in sync with that loop if it ever changes. The same
// translation, and the same VarModSites::MAX_SITES bound (checked here via a site
// count so an over-the-limit combination is rejected at build time with a clear
// error, exactly as before, rather than surfacing later as a silent per-candidate
// skip in MaterializeOneEntry()), is repeated there since that function needs to
// produce an explicit pcVarModSites, not just a count.
bool CometPeptideIndex::EnumerateIndexPeptideMods(vector<FragmentPeptidesStruct>& vVariants)
{
   vector<int> vModSlotForAllModsIdx;
   for (int i = 0; i < FRAGINDEX_VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
         && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0] != '-'))
      {
         vModSlotForAllModsIdx.push_back(i);
      }
   }

   bool bModSitesOverflow = false;

   auto tryPush = [&](size_t iWhichPeptide, int modNumIdx, char cNtermMod, char cCtermMod)
   {
      const PlainPeptideIndexStruct& raw = g_vRawPeptides.at(iWhichPeptide);
      const int iLen = (int)strlen(raw.szPeptide);

      double dCalcPepMass = raw.dPepMass;
      int cNumSites = 0;

      if (modNumIdx >= 0)
      {
         const ModificationNumber& modNum = MOD_NUMBERS.at(modNumIdx);
         char* mods = modNum.modifications;
         int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
         const string& modSeq = MOD_SEQS.at(modSeqIdx);

         int j = 0;
         for (int i = 0; i < iLen; ++i)
         {
            if (raw.szPeptide[i] == modSeq[j])
            {
               if (mods[j] != -1)
               {
                  int iSlot = vModSlotForAllModsIdx.at((size_t)mods[j]);
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

      FragmentPeptidesStruct sVariant;
      sVariant.dPepMass = dCalcPepMass;
      sVariant.iWhichPeptide = (unsigned int)iWhichPeptide;
      sVariant.modNumIdx = modNumIdx;
      sVariant.cNtermMod = cNtermMod;
      sVariant.cCtermMod = cCtermMod;
      vVariants.push_back(sVariant);
   };

   for (size_t iWhichPeptide = 0; iWhichPeptide < g_vRawPeptides.size(); ++iWhichPeptide)
   {
      const PlainPeptideIndexStruct& raw = g_vRawPeptides.at(iWhichPeptide);
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
            char* mods = MOD_NUMBERS.at(modNumIdx).modifications;
            for (int i = 0; i < MOD_NUMBERS.at(modNumIdx).modStringLen; ++i)
            {
               if (mods[i] != -1
                  && !cometbitcheck(raw.siVarModProteinFilter, vModSlotForAllModsIdx.at((size_t)mods[i])))
               {
                  bPass = false;
                  break;
               }
            }
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
   vector<int> vModSlotForAllModsIdx;
   for (int i = 0; i < FRAGINDEX_VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
         && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0] != '-'))
      {
         vModSlotForAllModsIdx.push_back(i);
      }
   }

   const PlainPeptideIndexStruct& raw = g_vRawPeptides.at(iWhichPeptide);
   const int iLen = (int)strlen(raw.szPeptide);

   double dCalcPepMass = raw.dPepMass;
   VarModSites pcVarModSites;

   if (modNumIdx >= 0)
   {
      const ModificationNumber& modNum = MOD_NUMBERS.at(modNumIdx);
      char* mods = modNum.modifications;
      int modSeqIdx = PEPTIDE_MOD_SEQ_IDXS[iWhichPeptide];
      const string& modSeq = MOD_SEQS.at(modSeqIdx);

      int j = 0;
      for (int i = 0; i < iLen; ++i)
      {
         if (raw.szPeptide[i] == modSeq[j])
         {
            if (mods[j] != -1)
            {
               int iSlot = vModSlotForAllModsIdx.at((size_t)mods[j]);
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


bool CometPeptideIndex::WritePeptideIndex(ThreadPool* tp)
{
   bool bSucceeded;
   FILE* fptr;

   const int iIndex_SIZE_FILE = SIZE_FILE + 4;
   char szIndexFile[iIndex_SIZE_FILE];
   sprintf(szIndexFile, "%s.idx", g_staticParams.databaseInfo.szDatabase);

   if ((fptr = fopen(szIndexFile, "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", szIndexFile);
      exit(1);
   }

   logout(" Creating peptide index file: ");
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
   {
      vector<pair<size_t,size_t>> slices;
      bSucceeded = CometFragmentIndex::GeneratePlainPeptideIndex(tp, slices);
   }

   if (!bSucceeded)
   {
      string strErrorMsg = " Error in GeneratePlainPeptideIndex() for peptide index creation.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // Phase B (docs/20260730_PI_reduction.md Phase 1): permute mods onto the deduplicated
   // unmodified list, but -- unlike the pre-reduction design -- do not materialize a full
   // DBIndex entry per (peptide, mod combination) pair here. Build a compact
   // FragmentPeptidesStruct reference {iWhichPeptide, modNumIdx, cNtermMod, cCtermMod,
   // dPepMass} per pair instead; g_vRawPeptides supplies the sequence/protein/flank data
   // shared by every variant of a given raw peptide, so it's kept alive (not cleared) for
   // the rest of this function -- it's written to the .idx file below and read back
   // alongside the compact variants at search time (CometPeptideIndex::ReadPeptideIndex()).
   g_vRawPeptides.clear();
   g_vRawPeptides.reserve(g_pvDBIndex.size());
   for (const auto& entry : g_pvDBIndex)
   {
      PlainPeptideIndexStruct sTmp;
      strcpy(sTmp.szPeptide, entry.sPeptide);
      sTmp.lIndexProteinFilePosition = entry.lIndexProteinFilePosition;
      sTmp.dPepMass = entry.dPepMass;
      sTmp.siVarModProteinFilter = entry.siVarModProteinFilter;
      sTmp.cPrevAA = entry.cPrevAA;
      sTmp.cNextAA = entry.cNextAA;
      g_vRawPeptides.push_back(sTmp);
   }

   // g_pvDBIndex (Phase A's per-unique-raw-peptide DBIndex entries) has now been fully
   // copied into g_vRawPeptides; nothing below needs the DBIndex-format copy any more.
   g_pvDBIndex.clear();
   vector<DBIndex>().swap(g_pvDBIndex);

   CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides);

   vector<FragmentPeptidesStruct> vVariants;
   vVariants.reserve(g_vRawPeptides.size());

   // require_variable_mod: every entry must carry a required mod, so the fully-unmodified
   // variant (modNumIdx == -1, no terminal mods) is only included when that's not required
   // (matching CometFragmentIndex::AddFragmentsThreadProc()'s equivalent check).
   if (!g_staticParams.variableModParameters.iRequireVarMod)
   {
      for (size_t i = 0; i < g_vRawPeptides.size(); ++i)
      {
         FragmentPeptidesStruct sVariant;
         sVariant.dPepMass = g_vRawPeptides[i].dPepMass;
         sVariant.iWhichPeptide = (unsigned int)i;
         sVariant.modNumIdx = -1;
         sVariant.cNtermMod = -1;
         sVariant.cCtermMod = -1;
         vVariants.push_back(sVariant);
      }
   }

   if (!EnumerateIndexPeptideMods(vVariants))
   {
      string strErrorMsg = " Error in EnumerateIndexPeptideMods() for peptide index creation.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // sanity check
   if (vVariants.size() == 0)
   {
      string strErrorMsg = " Error: no peptides in index; check the input database file or search parameters.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // No dedup pass: unlike the legacy RunSearch() build path (one raw entry per protein
   // OCCURRENCE, needing a grouping+dedup pass to consolidate into a per-unique-peptide
   // protein list), Phase A/B produces exactly one g_vRawPeptides row per unique peptide
   // and EnumerateIndexPeptideMods() enumerates each valid mod combination exactly once
   // per raw peptide -- there is no remaining source of duplication to defend against
   // (see docs/20260730_PI_reduction.md Section 8, Open Question 1).

   // sort by mass; FragmentPeptidesStruct::operator< compares dPepMass.
   sort(vVariants.begin(), vVariants.end());

   logout(" - writing file\n");
   fflush(stdout);

   // write out index header. Magic string bumped ("... v2") from the pre-reduction format
   // ("Comet peptide index database.") so old-format .idx files are rejected by
   // ReadPeptideIndex()'s header check with a clear error instead of being misread against
   // this file's new raw-peptide-table + compact-variant-array layout (see the layout
   // comment above ReadPeptideIndex() below).
   fprintf(fptr, "Comet peptide index database v2.  Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fptr, "InputDB:  %s\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fptr, "MassRange: %lf %lf\n", g_staticParams.options.dPeptideMassLow, g_staticParams.options.dPeptideMassHigh);
   fprintf(fptr, "LengthRange: %d %d\n", g_staticParams.options.peptideLengthRange.iStart, g_staticParams.options.peptideLengthRange.iEnd);
   fprintf(fptr, "MassType: %d %d\n", g_staticParams.massUtility.bMonoMassesParent, g_staticParams.massUtility.bMonoMassesFragment);
   fprintf(fptr, "DecoySearch: %d\n", g_staticParams.options.iDecoySearch);
   fprintf(fptr, "Enzyme: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzymeName,
      g_staticParams.enzymeInformation.iSearchEnzymeOffSet,
      g_staticParams.enzymeInformation.szSearchEnzymeBreakAA,
      g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
   fprintf(fptr, "Enzyme2: %s [%d %s %s]\n", g_staticParams.enzymeInformation.szSearchEnzyme2Name,
      g_staticParams.enzymeInformation.iSearchEnzyme2OffSet,
      g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA,
      g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
   fprintf(fptr, "NumPeptides: %ld\n", (long)vVariants.size());

   // write out static mod params A to Z is ascii 65 to 90 then terminal mods
   fprintf(fptr, "StaticMod:");
   for (int x = 65; x <= 90; x++)
      fprintf(fptr, " %lf", g_staticParams.staticModifications.pdStaticMods[x]);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddNterminusPeptide);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddCterminusPeptide);
   fprintf(fptr, " %lf", g_staticParams.staticModifications.dAddNterminusProtein);
   fprintf(fptr, " %lf\n", g_staticParams.staticModifications.dAddCterminusProtein);

   // write out variable mod params
   fprintf(fptr, "VariableMod:");
   for (int x = 0; x < VMODS; x++)
   {
      fprintf(fptr, " %s:%lf:%lf:%lf",
         g_staticParams.variableModParameters.varModList[x].szVarModChar,
         g_staticParams.variableModParameters.varModList[x].dVarModMass,
         g_staticParams.variableModParameters.varModList[x].dNeutralLoss,
         g_staticParams.variableModParameters.varModList[x].dNeutralLoss2);

   }
   fprintf(fptr, "\n\n");

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

   // Now write out g_pvProteinsList (ProteinsListCSR, already built by Phase A --
   // see the no-dedup-needed comment above for why no local rebuild is needed here).
   // clProteinsFilePos is written as a one-value footer at true EOF (below) so
   // ReadPeptideIndex() can jump straight past the protein-names region, which has no
   // stored count and is otherwise only navigable via the file offsets embedded in this
   // proteins list -- everything from here through the end of the variant array (further
   // below) is otherwise read purely sequentially, no other seeking needed.
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
            delete[] lProteinIndex;
            return false;
         }

         fwrite(&lProteinIndex[iWhichProtein], clSizeCometFileOffset, 1, fptr);
      }
   }

   delete[] lProteinIndex;

   // Write the raw-peptide table (one entry per unique unmodified peptide, sequence/protein/
   // flank data shared by every mod-variant of that peptide) followed by the compact variant
   // array (one entry per (peptide, mod combination) pair -- mass plus a reference back into
   // the raw-peptide table, no duplicated sequence or explicit mod-site data). Both blocks are
   // written and read purely sequentially -- no seek-driven footer/mass-index needed, since
   // ReadPeptideIndex() always loads both blocks fully into memory in one linear pass. See the
   // layout comment above ReadPeptideIndex() below for the complete on-disk format.
   uint64_t tNumRaw = (uint64_t)g_vRawPeptides.size();
   fwrite(&tNumRaw, sizeof(uint64_t), 1, fptr);
   for (const auto& raw : g_vRawPeptides)
   {
      int iLen = (int)strlen(raw.szPeptide);
      fwrite(&iLen, sizeof(int), 1, fptr);
      fwrite(raw.szPeptide, sizeof(char), iLen, fptr);
      fwrite(&raw.cPrevAA, sizeof(char), 1, fptr);
      fwrite(&raw.cNextAA, sizeof(char), 1, fptr);
      fwrite(&raw.dPepMass, sizeof(double), 1, fptr);
      fwrite(&raw.lIndexProteinFilePosition, sizeof(comet_fileoffset_t), 1, fptr);
   }

   uint64_t tNumVariants = (uint64_t)vVariants.size();
   fwrite(&tNumVariants, sizeof(uint64_t), 1, fptr);
   for (const auto& variant : vVariants)
   {
      fwrite(&variant.dPepMass, sizeof(double), 1, fptr);
      fwrite(&variant.iWhichPeptide, sizeof(unsigned int), 1, fptr);
      fwrite(&variant.modNumIdx, sizeof(int), 1, fptr);
      fwrite(&variant.cNtermMod, sizeof(char), 1, fptr);
      fwrite(&variant.cCtermMod, sizeof(char), 1, fptr);
   }

   // One-value footer: ReadPeptideIndex() seeks to EOF - sizeof(comet_fileoffset_t) to read
   // this, then seeks to it to jump past the protein-names region (see the comment above the
   // proteins-list write, above).
   fwrite(&clProteinsFilePos, clSizeCometFileOffset, 1, fptr);

   fclose(fptr);

   std::string strNumPeps;
   if (tNumVariants > 1e6)
   {
      std::ostringstream oss;
      oss << std::scientific << std::setprecision(3) << static_cast<double>(tNumVariants);
      strNumPeps = oss.str();
   }
   else
   {
      strNumPeps = std::to_string(tNumVariants);
   }

   string strOut = " - created: " + std::string(szIndexFile) + " (" + strNumPeps + " peptides)\n";
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


// Parses the .idx text header lines (MassType:, StaticMod:, DecoySearch:,
// Enzyme:, Enzyme2:, VariableMod:) from fp.  Reads until the VariableMod:
// line (inclusive), which is always the last header entry before the blank
// line separator.
//
// Both SearchPeptideIndex(ThreadPool*) and InitializeMassesFromPeptideIndex()
// call this helper so that any future .idx header format changes only need to
// be made in one place.
//
// IMPORTANT: resets pdAAMassFragment AND pdAAMassParent via AssignMass()
// before applying static mods, so this is safe to call whether or not
// InitializeStaticParams() has already applied static mods.
bool CometPeptideIndex::ParsePeptideIndexHeader(FILE* fp)
{
   char szBuf[SIZE_BUF];
   bool bFoundStatic = false;
   bool bFoundVariable = false;

   // Ignore any static masses from the params file; only the values baked
   // into the .idx header are authoritative for an index search.
   memset(g_staticParams.staticModifications.pdStaticMods, 0,
      sizeof(g_staticParams.staticModifications.pdStaticMods));

   rewind(fp);

   while (fgets(szBuf, SIZE_BUF, fp))
   {
      if (!strncmp(szBuf, "MassType:", 9))
      {
         sscanf(szBuf + 10, "%d %d",
            &g_staticParams.massUtility.bMonoMassesParent,
            &g_staticParams.massUtility.bMonoMassesFragment);
      }
      else if (!strncmp(szBuf, "StaticMod:", 10))
      {
         char* tok;
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
         tok = strtok(szBuf + 11, delims);
         while (tok != NULL)
         {
            sscanf(tok, "%lf", &(g_staticParams.staticModifications.pdStaticMods[x]));
            g_staticParams.massUtility.pdAAMassFragment[x] += g_staticParams.staticModifications.pdStaticMods[x];
            g_staticParams.massUtility.pdAAMassParent[x] += g_staticParams.staticModifications.pdStaticMods[x];
            tok = strtok(NULL, delims);
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
      else if (!strncmp(szBuf, "DecoySearch:", 12))
      {
         sscanf(szBuf, "DecoySearch: %d", &(g_staticParams.options.iDecoySearch));
      }
      else if (!strncmp(szBuf, "Enzyme:", 7))
      {
         sscanf(szBuf, "Enzyme: %s [%d %s %s]",
            g_staticParams.enzymeInformation.szSearchEnzymeName,
            &(g_staticParams.enzymeInformation.iSearchEnzymeOffSet),
            g_staticParams.enzymeInformation.szSearchEnzymeBreakAA,
            g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
      }
      else if (!strncmp(szBuf, "Enzyme2:", 8))
      {
         sscanf(szBuf, "Enzyme2: %s [%d %s %s]",
            g_staticParams.enzymeInformation.szSearchEnzyme2Name,
            &(g_staticParams.enzymeInformation.iSearchEnzyme2OffSet),
            g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA,
            g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
      }
      else if (!strncmp(szBuf, "VariableMod:", 12))
      {
         string strMods = szBuf + 13;
         istringstream iss(strMods);
         int iNumMods = 0;

         bFoundVariable = true;

         do
         {
            string subStr;
            iss >> subStr;
            std::replace(subStr.begin(), subStr.end(), ':', ' ');
            int iRet = sscanf(subStr.c_str(), "%s %lf %lf %lf",
               g_staticParams.variableModParameters.varModList[iNumMods].szVarModChar,
               &(g_staticParams.variableModParameters.varModList[iNumMods].dVarModMass),
               &(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss),
               &(g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss2));

            if (iRet != 4)
            {
               string strErrorMsg = " Error parsing mod entry: " + subStr + ".\n";
               logerr(strErrorMsg);
               return false;
            }

            if (g_staticParams.variableModParameters.varModList[iNumMods].dNeutralLoss != 0.0)
               g_staticParams.variableModParameters.bUseFragmentNeutralLoss = true;

            iNumMods++;
            if (iNumMods == VMODS)
               break;
         } while (iss);

         // VariableMod: is always the last relevant header line.
         break;
      }
   }

   if (!(bFoundStatic && bFoundVariable))
   {
      string strErrorMsg = " Error with index database format. Mods not parsed ("
         + std::to_string(bFoundStatic) + " " + std::to_string(bFoundVariable) + ").\n";
      logerr(strErrorMsg);
      return false;
   }

   // Peptide index searches always have variable mod search enabled because
   // mod sites are baked into every index entry.
   g_staticParams.variableModParameters.bVarModSearch = true;

   return true;
}
