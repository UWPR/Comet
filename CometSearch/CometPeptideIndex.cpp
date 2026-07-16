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

// Read the full peptide index (.idx) file into global read-only structures:
//   g_pvDBIndex         - all peptide entries, sorted by mass
//   g_pvProteinsList    - vector-of-vectors mapping peptide to protein file positions
//   g_pvProteinNames    - map of file offset to protein name string
//   g_bPeptideIndexRead - guard flag
//
// The .idx binary layout (written by WritePeptideIndex):
//   [text header lines ending with blank line]
//   [protein names: each WIDTH_REFERENCE chars]
//   [proteins list: count then per-entry (size + file offsets)]
//   [peptide entries: each via ReadPeptideIndexEntry format]
//   [footer: iMinMass(int), iMaxMass(int), tNumPeptides(uint64_t),
//            lIndex[iMaxMass*10](comet_fileoffset_t), lEndOfPeptides, clProteinsFilePos]
//
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

   // Verify this is a peptide index file (not a fragment index)
   if (fgets(szBuf, SIZE_BUF, fp) == NULL)
   {
      fclose(fp);
      return false;
   }
   if (strncmp(szBuf, "Comet peptide index database", 28) != 0)
   {
      string strErrorMsg = " Error - \"" + string(g_staticParams.databaseInfo.szDatabase)
         + "\" is not a peptide index file.\n";
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

   // --- Read footer first to get layout positions ---
   // Footer is at the very end of the file:
   //   ... lEndOfPeptides(comet_fileoffset_t) clProteinsFilePos(comet_fileoffset_t)
   // Seek to end minus 2 * sizeof(comet_fileoffset_t) to read both values
   comet_fseek(fp, -2 * (comet_fileoffset_t)clSizeCometFileOffset, SEEK_END);

   comet_fileoffset_t lEndOfPeptides;
   comet_fileoffset_t clProteinsFilePos;
   (void)fread(&lEndOfPeptides, clSizeCometFileOffset, 1, fp);
   (void)fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);

   // --- Read the mass index and peptide count from lEndOfPeptides position ---
   comet_fseek(fp, lEndOfPeptides, SEEK_SET);

   int iMinMass, iMaxMass;
   uint64_t tNumPeptides;
   (void)fread(&iMinMass, sizeof(int), 1, fp);
   (void)fread(&iMaxMass, sizeof(int), 1, fp);
   (void)fread(&tNumPeptides, sizeof(uint64_t), 1, fp);

   int iMaxPeptideMass10 = iMaxMass * 10;

   // Read the mass index array: lIndex[0..iMaxPeptideMass10-1]
   // Each entry is a file offset to the first peptide at that 0.1 Da mass bin
   comet_fileoffset_t* lIndex = new comet_fileoffset_t[iMaxPeptideMass10];
   (void)fread(lIndex, clSizeCometFileOffset, iMaxPeptideMass10, fp);

   // --- Read protein names ---
   // Protein names are stored between end-of-header and clProteinsFilePos
   // Each protein name is WIDTH_REFERENCE chars.  We need to know how many
   // there are.  Read them by seeking to the position right after the header
   // and reading until clProteinsFilePos.

   // Actually, the protein names are written BEFORE clProteinsFilePos.
   // The structure is: [header][protein names][proteins list at clProteinsFilePos][peptides][footer]
   // We need the protein name positions for g_pvProteinNames mapping.
   // For the single-spectrum search path, protein names are resolved via
   // g_pvProteinsList file offsets that point into the .idx file protein name region.
   // We'll store them after reading the proteins list.

   // --- Read proteins list (ProteinsListCSR) from clProteinsFilePos ---
   comet_fseek(fp, clProteinsFilePos, SEEK_SET);

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

   // The file position after reading the proteins list is where the peptides start.
   comet_fileoffset_t lFirstPeptidePos = comet_ftell(fp);

   // --- Read all peptide entries into g_pvDBIndex ---
   g_pvDBIndex.clear();
   g_pvDBIndex.reserve((size_t)tNumPeptides);

   // Seek to first peptide (right after the proteins list)
   comet_fseek(fp, lFirstPeptidePos, SEEK_SET);

   for (uint64_t i = 0; i < tNumPeptides; ++i)
   {
      DBIndex sEntry;
      if (!ReadPeptideIndexEntry(&sEntry, fp))
      {
         g_pvDBIndex.clear();
         delete[] lIndex;
         fclose(fp);
         logout(" Error - failed to read peptide index entry " + to_string(i)
            + " from .idx file; file may be truncated or corrupt.\n");
         return false;
      }
      g_pvDBIndex.push_back(std::move(sEntry));
   }

   // g_pvDBIndex is already sorted by mass from the .idx file

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

   delete[] lIndex;
   fclose(fp);

   if (bIsRTS)
   {
      logout(" Read peptide index: " + to_string(tNumPeptides) + " peptides, "
         + to_string(tNumProteinEntries) + " protein groups\n");
   }
   else
   {
      logout("\n   - Read peptide index: " + to_string(tNumPeptides) + " peptides, "
         + to_string(tNumProteinEntries) + " protein groups\n");
   }

   g_staticParams.iDbType = DbType::PI_DB;
   g_bPeptideIndexRead = true;

   return true;
}


// See docs/20260713_PIidxformat.md section 8 (Phase B). Mirrors
// CometFragmentIndex::AddFragmentsThreadProc()'s enumeration structure (which
// mod/n-term/c-term combinations to try per peptide) and AddFragments()'s mass
// computation, but materializes full DBIndex entries (with explicit pcVarModSites)
// instead of fragment-ion bins, since PI_DB's .idx format requires every specific
// modified peptide fully enumerated and written, unlike FI_DB's deferred approach.
//
// Two deliberate deviations from AddFragments(), both scoped to this new function
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
// into that compacted list, not direct varModList slot indices. PI_DB's pcVarModSites
// encoding (WritePeptideIndex()/ReadPeptideIndexEntry()) requires a direct varModList
// slot index + 1. vModSlotForAllModsIdx rebuilds the exact same compaction order as
// CometFragmentIndex::PermuteIndexPeptideMods()'s ALL_MODS-building loop to translate
// between the two -- must stay in sync with that loop if it ever changes.
bool CometPeptideIndex::MaterializeIndexPeptideMods(vector<DBIndex>& vModifiedEntries)
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

   auto buildEntry = [&](size_t iWhichPeptide, int modNumIdx, char cNtermMod, char cCtermMod)
   {
      const PlainPeptideIndexStruct& raw = g_vRawPeptides.at(iWhichPeptide);
      const int iLen = (int)strlen(raw.szPeptide);

      double dCalcPepMass = raw.dPepMass;
      vector<char> pcVarModSites(iLen + 2, 0);

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
                  pcVarModSites[i] = (char)(iSlot + 1);
               }
               j++;
            }
         }
      }

      if (cNtermMod >= 0)
      {
         dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cNtermMod].dVarModMass;
         pcVarModSites[iLen] = (char)(cNtermMod + 1);
      }
      if (cCtermMod >= 0)
      {
         dCalcPepMass += g_staticParams.variableModParameters.varModList[(int)cCtermMod].dVarModMass;
         pcVarModSites[iLen + 1] = (char)(cCtermMod + 1);
      }

      if (dCalcPepMass > g_massRange.dMaxMass || dCalcPepMass < g_massRange.dMinMass)
         return;

      DBIndex sEntry;
      sEntry.pcVarModSites = std::move(pcVarModSites);
      sEntry.lIndexProteinFilePosition = raw.lIndexProteinFilePosition;
      sEntry.dPepMass = dCalcPepMass;
      sEntry.siVarModProteinFilter = raw.siVarModProteinFilter;
      sEntry.cPrevAA = raw.cPrevAA;
      sEntry.cNextAA = raw.cNextAA;
      strcpy(sEntry.sPeptide, raw.szPeptide);

      vModifiedEntries.push_back(std::move(sEntry));
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
               buildEntry(iWhichPeptide, -1, ctNtermMod, -1);
            }
         }

         for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
         {
            if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
               && (!g_staticParams.variableModParameters.bVarModProteinFilter
                  || cometbitcheck(raw.siVarModProteinFilter, ctCtermMod)))
            {
               buildEntry(iWhichPeptide, -1, -1, ctCtermMod);
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
                  buildEntry(iWhichPeptide, -1, ctNtermMod, ctCtermMod);
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

         buildEntry(iWhichPeptide, modNumIdx, -1, -1);

         if (g_staticParams.variableModParameters.bVarTermModSearch)
         {
            for (char ctNtermMod = 0; ctNtermMod < FRAGINDEX_VMODS; ++ctNtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[(int)ctNtermMod].bNtermMod
                  && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(raw.siVarModProteinFilter, ctNtermMod)))
               {
                  buildEntry(iWhichPeptide, modNumIdx, ctNtermMod, -1);
               }
            }

            for (char ctCtermMod = 0; ctCtermMod < FRAGINDEX_VMODS; ++ctCtermMod)
            {
               if (g_staticParams.variableModParameters.varModList[(int)ctCtermMod].bCtermMod
                  && (!g_staticParams.variableModParameters.bVarModProteinFilter || cometbitcheck(raw.siVarModProteinFilter, ctCtermMod)))
               {
                  buildEntry(iWhichPeptide, modNumIdx, -1, ctCtermMod);
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
                     buildEntry(iWhichPeptide, modNumIdx, ctNtermMod, ctCtermMod);
                  }
               }
            }
         }
      }
   }

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

   // Phase B: permute mods onto the deduplicated unmodified list and materialize a
   // full DBIndex entry (peptide, protein reference, mass, explicit pcVarModSites)
   // for every valid (peptide, mod combination) pair.
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

   CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides);

   vector<DBIndex> vModifiedEntries;
   if (!MaterializeIndexPeptideMods(vModifiedEntries))
   {
      string strErrorMsg = " Error in MaterializeIndexPeptideMods() for peptide index creation.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // require_variable_mod: every entry must carry a required mod, so the
   // unmodified variants Phase A generated unconditionally must be dropped here
   // (matching CometFragmentIndex::AddFragmentsThreadProc()'s equivalent check).
   if (g_staticParams.variableModParameters.iRequireVarMod)
      g_pvDBIndex.clear();

   g_pvDBIndex.insert(g_pvDBIndex.end(),
      make_move_iterator(vModifiedEntries.begin()), make_move_iterator(vModifiedEntries.end()));

   vector<PlainPeptideIndexStruct>().swap(g_vRawPeptides);

   // sanity check
   if (g_pvDBIndex.size() == 0)
   {
      string strErrorMsg = " Error: no peptides in index; check the input database file or search parameters.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // remove duplicates
   logout(" - removing duplicates\n");
   fflush(stdout);

   // Unlike the legacy RunSearch() path (one raw g_pvDBIndex entry per protein
   // OCCURRENCE, needing a grouping pass here to consolidate into a per-unique-peptide
   // protein list), Phase A/B already produced g_pvDBIndex with lIndexProteinFilePosition
   // as a ready-made index into g_pvProteinsList (a row per unique peptide, built once
   // during Phase A's digestion) -- both for the unmodified entries (set directly by
   // GeneratePlainPeptideIndex()) and the modified entries (MaterializeIndexPeptideMods()
   // copies the same index from each entry's parent unmodified peptide, since a modified
   // variant is found in exactly the same proteins as its unmodified form). No renumbering
   // or protein-list rebuilding is needed; g_pvProteinsList is written as-is further down.
   //
   // This sort+unique is a defensive no-op in the common case (Phase A guarantees unique
   // peptides; Phase B's enumeration is deterministic and non-duplicating per peptide) but
   // is cheap insurance against a peptide+mod-state collision.
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end());  // sort by peptide sequence, mod state, protein file position
   g_pvDBIndex.erase(unique(g_pvDBIndex.begin(), g_pvDBIndex.end()), g_pvDBIndex.end());

   // sort by mass;
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CometMassSpecUtils::DBICompareByMass);

/*
   printf("OK unique peptide index entries:\n");
   for (std::vector<DBIndex>::iterator it = g_pvDBIndex.begin(); it != g_pvDBIndex.end(); ++it)
   {
      if ((*it).pcVarModSites[strlen((*it).szPeptide)] != 0)
         printf("n%d", (*it).pcVarModSites[strlen((*it).szPeptide)] - 1);
      for (unsigned int x = 0; x < strlen((*it).szPeptide); x++)
      {
         printf("%c", (*it).szPeptide[x]);
         if ((*it).pcVarModSites[x] != 0)
            printf("%d", (*it).pcVarModSites[x] - 1);
      }
      if ((*it).pcVarModSites[strlen((*it).szPeptide) + 1] != 0)
         printf("c%d", (*it).pcVarModSites[strlen((*it).szPeptide)] - 1);
      printf("\t%f\t%lld\t%d\n", (*it).dPepMass, (*it).lIndexProteinFilePosition,(*it).siVarModProteinFilter);
   }
   printf("\n");
*/

   logout(" - writing file\n");
   fflush(stdout);

   // write out index header
   fprintf(fptr, "Comet peptide index database.  Comet version %s\n", g_sCometVersion.c_str());
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
   fprintf(fptr, "NumPeptides: %ld\n", (long)g_pvDBIndex.size());

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
   // see the comment above the dedup pass for why no local rebuild is needed here).
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

   // next write out the peptides and track peptide mass index
   int iMaxPeptideMass = (int)(g_staticParams.options.dPeptideMassHigh);
   int iMaxPeptideMass10 = iMaxPeptideMass * 10;  // make mass index at resolution of 0.1 Da
   comet_fileoffset_t* lIndex = new comet_fileoffset_t[iMaxPeptideMass10 + 1];
   for (int x = 0; x <= iMaxPeptideMass10; x++)
      lIndex[x] = -1;

   // write out peptide entry here
   int iPrevMass10 = 0;
   for (std::vector<DBIndex>::iterator it = g_pvDBIndex.begin(); it != g_pvDBIndex.end(); ++it)
   {
      if ((int)((*it).dPepMass * 10.0) > iPrevMass10)
      {
         iPrevMass10 = (int)((*it).dPepMass * 10.0);
         if (iPrevMass10 < iMaxPeptideMass10)
            lIndex[iPrevMass10] = comet_ftell(fptr);
      }

      int iLen = (int)strlen((*it).sPeptide);
      fwrite(&iLen, sizeof(int), 1, fptr);
      fwrite((*it).sPeptide, sizeof(char), iLen, fptr);

      fwrite(&((*it).cPrevAA), sizeof(char), 1, fptr);
      fwrite(&((*it).cNextAA), sizeof(char), 1, fptr);

      // write out for char 0=no mod, N=mod.  If N, write out var mods as N pairs (pos,whichmod)
      int iLen2 = iLen + 2;
      unsigned char cNumMods = 0;
      if (!(*it).pcVarModSites.empty())
      {
         for (unsigned char x = 0; x < iLen2; x++)
         {
            if ((*it).pcVarModSites[x] != 0)
               cNumMods++;
         }
      }
      fwrite(&cNumMods, sizeof(unsigned char), 1, fptr);

      if (cNumMods > 0)
      {
         for (unsigned char x = 0; x < iLen2; x++)
         {
            if ((*it).pcVarModSites[x] != 0)
            {
               char cWhichMod = (*it).pcVarModSites[x];
               fwrite(&x, sizeof(unsigned char), 1, fptr);
               fwrite(&cWhichMod, sizeof(char), 1, fptr);
            }
         }
      }
      // done writing out mod sites

      fwrite(&((*it).dPepMass), sizeof(double), 1, fptr);
      fwrite(&((*it).lIndexProteinFilePosition), sizeof(comet_fileoffset_t), 1, fptr);
   }

   comet_fileoffset_t lEndOfPeptides = comet_ftell(fptr);

   int iTmpCh = (int)(g_staticParams.options.dPeptideMassLow);
   fwrite(&iTmpCh, sizeof(int), 1, fptr);  // write min mass
   fwrite(&iMaxPeptideMass, sizeof(int), 1, fptr);  // write max mass
   uint64_t tNumPeptides = g_pvDBIndex.size();
   fwrite(&tNumPeptides, sizeof(uint64_t), 1, fptr);  // write # of peptides
   fwrite(lIndex, clSizeCometFileOffset, iMaxPeptideMass10, fptr); // write index
   fwrite(&lEndOfPeptides, clSizeCometFileOffset, 1, fptr);  // write ftell position of min/max mass, # peptides, peptide index
   fwrite(&clProteinsFilePos, clSizeCometFileOffset, 1, fptr);

   fclose(fptr);

   std::string strNumPeps;
   if (tNumPeptides > 1e6)
   {
      std::ostringstream oss;
      oss << std::scientific << std::setprecision(3) << static_cast<double>(tNumPeptides);
      strNumPeps = oss.str();
   }
   else
   {
      strNumPeps = std::to_string(tNumPeptides);
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

   g_pvDBIndex.clear();
   delete[] lIndex;

   return bSucceeded;
}


bool CometPeptideIndex::ReadPeptideIndexEntry(struct DBIndex* sDBI, FILE* fp)
{
   int iLen;
   size_t tTmp;

   tTmp = fread(&iLen, sizeof(int), 1, fp);
   if (tTmp != 1) return false;
   tTmp = fread(sDBI->sPeptide, sizeof(char), iLen, fp);
   if (tTmp != (size_t)iLen) return false;
   sDBI->sPeptide[iLen] = '\0';

   tTmp = fread(&(sDBI->cPrevAA), sizeof(char), 1, fp);
   if (tTmp != 1) return false;
   tTmp = fread(&(sDBI->cNextAA), sizeof(char), 1, fp);
   if (tTmp != 1) return false;

   unsigned char cNumMods;  // number of var mods encoded as position:residue pairs
   tTmp = fread(&cNumMods, sizeof(unsigned char), 1, fp);  // read how many var mods are stored
   if (tTmp != 1) return false;

   sDBI->pcVarModSites.clear();
   if (cNumMods > 0)
   {
      sDBI->pcVarModSites.assign(iLen + 2, 0);
      for (unsigned char x = 0; x < cNumMods; x++)
      {
         unsigned char cPosition;
         char cResidue;
         tTmp = fread(&cPosition, sizeof(unsigned char), 1, fp);
         if (tTmp != 1) return false;
         tTmp = fread(&cResidue, sizeof(char), 1, fp);
         if (tTmp != 1) return false;
         sDBI->pcVarModSites[(int)cPosition] = cResidue;
      }
   }
   // done reading mod sites

   tTmp = fread(&(sDBI->dPepMass), sizeof(double), 1, fp);
   if (tTmp != 1) return false;
   tTmp = fread(&(sDBI->lIndexProteinFilePosition), sizeof(comet_fileoffset_t), 1, fp);
   if (tTmp != 1) return false;

   return true;
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
