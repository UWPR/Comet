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
bool CometPeptideIndex::ReadPeptideIndex(void)
{
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
   comet_fseek(fp, -2 * (long)clSizeCometFileOffset, SEEK_END);

   comet_fileoffset_t lEndOfPeptides;
   comet_fileoffset_t clProteinsFilePos;
   size_t tTmpRead;
   tTmpRead = fread(&lEndOfPeptides, clSizeCometFileOffset, 1, fp);
   tTmpRead = fread(&clProteinsFilePos, clSizeCometFileOffset, 1, fp);

   // --- Read the mass index and peptide count from lEndOfPeptides position ---
   comet_fseek(fp, lEndOfPeptides, SEEK_SET);

   int iMinMass, iMaxMass;
   uint64_t tNumPeptides;
   tTmpRead = fread(&iMinMass, sizeof(int), 1, fp);
   tTmpRead = fread(&iMaxMass, sizeof(int), 1, fp);
   tTmpRead = fread(&tNumPeptides, sizeof(uint64_t), 1, fp);

   int iMaxPeptideMass10 = iMaxMass * 10;

   // Read the mass index array: lIndex[0..iMaxPeptideMass10-1]
   // Each entry is a file offset to the first peptide at that 0.1 Da mass bin
   comet_fileoffset_t* lIndex = new comet_fileoffset_t[iMaxPeptideMass10];
   tTmpRead = fread(lIndex, clSizeCometFileOffset, iMaxPeptideMass10, fp);

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

   // --- Read proteins list (vector of vectors) from clProteinsFilePos ---
   comet_fseek(fp, clProteinsFilePos, SEEK_SET);

   size_t tNumProteinEntries;
   tTmpRead = fread(&tNumProteinEntries, clSizeCometFileOffset, 1, fp);

   g_pvProteinsList.clear();
   g_pvProteinsList.resize(tNumProteinEntries);

   for (size_t i = 0; i < tNumProteinEntries; ++i)
   {
      size_t tNumProteins;
      tTmpRead = fread(&tNumProteins, sizeof(size_t), 1, fp);

      g_pvProteinsList[i].resize(tNumProteins);
      for (size_t j = 0; j < tNumProteins; ++j)
      {
         comet_fileoffset_t lProtFilePos;
         tTmpRead = fread(&lProtFilePos, clSizeCometFileOffset, 1, fp);
         g_pvProteinsList[i][j] = lProtFilePos;
      }
   }

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
      ReadPeptideIndexEntry(&sEntry, fp);
      g_pvDBIndex.push_back(sEntry);
   }

   // g_pvDBIndex is already sorted by mass from the .idx file

   delete[] lIndex;
   fclose(fp);

   logout(" Read peptide index: " + to_string(tNumPeptides) + " peptides, "
      + to_string(tNumProteinEntries) + " protein groups\n");

   g_staticParams.iDbType = DbType::PI_DB;
   g_bPeptideIndexRead = true;

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

   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);

   // these are used in call to RunSearch to generate peptides
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
      g_massRange.bNarrowMassRange = true;
   else
      g_massRange.bNarrowMassRange = false;

   if (bSucceeded)
   {
      bSucceeded = CometSearch::RunSearch(0, 0, tp);
   }

   if (!bSucceeded)
   {
      string strErrorMsg = " Error in RunSearch() for peptide index creation.\n";
      logerr(strErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

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

   // keep unique entries only; sort by peptide/modification state and protein
   // first sort by peptide, then mod state, then protein file position
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CometMassSpecUtils::DBICompareByPeptide);

   // At this point, need to create pvProteinsListLocal protein file position vector of vectors to map each peptide
   // to every protein. g_pvDBIndex.at().lProteinFilePosition is now reference to protein vector entry.
   // When reading the peptide index file as part of the first call to SearchPeptideIndex(), this vector of vectors
   // will be stored in g_pvProteinsList to be used in the search.
   vector<vector<comet_fileoffset_t>> pvProteinsListLocal;
   vector<comet_fileoffset_t> temp;  // stores list of duplicate proteins which gets pushed to pvProteinsListLocal

   // Create pvProteinsListLocal.  This is a vector of vectors.  Each element is vector list
   // of duplicate proteins (generated as "temp") ... these are generated by looping
   // through g_pvDBIndex and looking for consecutive, same peptides.  Once the "temp"
   // vector is assigned the lIndexProteinFilePosition offset, the g_pvDBIndex entry
   // sets lIndexProteinFilePosition to lProtCount which references which vector of
   // proteins the peptide is contained in.
   long lProtCount = 0;
   for (size_t i = 0; i < g_pvDBIndex.size(); i++)
   {
      if (i == 0)
      {
         temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
         g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
      }
      else
      {
         // each unique peptide, irregardless of mod state, will have the same list
         // of matched proteins
         if (!strcmp(g_pvDBIndex.at(i).szPeptide, g_pvDBIndex.at(i - 1).szPeptide))
         {
            temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
            g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
         }
         else
         {
            // different peptide + mod state so go ahead and push temp onto pvProteinsListLocal
            // and store current protein reference into new temp
            // temp can have duplicates due to mod forms of peptide so make unique here
            sort(temp.begin(), temp.end());
            temp.erase(unique(temp.begin(), temp.end()), temp.end());
            pvProteinsListLocal.push_back(temp);

            lProtCount++; // start new row in pvProteinsListLocal
            temp.clear();
            temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
            g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;

         }
      }
   }

   // now at end of loop, push last temp onto pvProteinsListLocal
   sort(temp.begin(), temp.end());
   temp.erase(unique(temp.begin(), temp.end()), temp.end() );
   pvProteinsListLocal.push_back(temp);

   // JKE FIX:  Currently g_vProteinsList has an entry for every peptide and there
   // can be duplicates set of file pointers in g_vProteinsList.  Ideally each entry
   // in g_vProteinsList is a unique set of file pointers which means a bit of
   // optimization needs to happen to here (granted resulting only in storage/ram
   // savings for the reduced size of g_vProteinsList).

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

   // Now write out: vector<vector<comet_fileoffset_t>> pvProteinsListLocal
   comet_fileoffset_t clProteinsFilePos = comet_ftell(fptr);
   size_t tTmp = pvProteinsListLocal.size();
   int iWhichProtein;
   fwrite(&tTmp, clSizeCometFileOffset, 1, fptr);
   for (auto it = pvProteinsListLocal.begin(); it != pvProteinsListLocal.end(); ++it)
   {
      tTmp = (*it).size();
      fwrite(&tTmp, sizeof(size_t), 1, fptr);

      for (size_t it2 = 0; it2 < tTmp; ++it2)
      {
         iWhichProtein = -1;

         // find protein by matching g_pvProteinNames.lProteinFilePosition to g_pvProteinNames.lProteinIndex;
         auto result = g_pvProteinNames.find((*it).at(it2));
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

      int iLen = (int)strlen((*it).szPeptide);
      fwrite(&iLen, sizeof(int), 1, fptr);
      fwrite((*it).szPeptide, sizeof(char), iLen, fptr);

      fwrite(&((*it).cPrevAA), sizeof(char), 1, fptr);
      fwrite(&((*it).cNextAA), sizeof(char), 1, fptr);

      // write out for char 0=no mod, N=mod.  If N, write out var mods as N pairs (pos,whichmod)
      int iLen2 = iLen + 2;
      unsigned char cNumMods = 0;
      for (unsigned char x = 0; x < iLen2; x++)
      {
         if ((*it).pcVarModSites[x] != 0)
            cNumMods++;
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

   string strOut = " - done. " + std::string(szIndexFile) + " (" + strNumPeps + " peptides)\n\n";
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
   tTmp = fread(sDBI->szPeptide, sizeof(char), iLen, fp);
   if (tTmp != (size_t)iLen) return false;
   sDBI->szPeptide[iLen] = '\0';

   tTmp = fread(&(sDBI->cPrevAA), sizeof(char), 1, fp);
   if (tTmp != 1) return false;
   tTmp = fread(&(sDBI->cNextAA), sizeof(char), 1, fp);
   if (tTmp != 1) return false;

   unsigned char cNumMods;  // number of var mods encoded as position:residue pairs
   tTmp = fread(&cNumMods, sizeof(unsigned char), 1, fp);  // read how many var mods are stored
   if (tTmp != 1) return false;

   // Issue 3: Add parentheses for correct precedence: sizeof(unsigned char) * (iLen + 2)
   memset(sDBI->pcVarModSites, 0, sizeof(unsigned char) * (iLen + 2));
   if (cNumMods > 0)
   {
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
