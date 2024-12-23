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


bool CometPeptideIndex::WritePeptideIndex(ThreadPool *tp)
{
   bool bSucceeded;
   char szOut[256];
   FILE *fptr;

   const int iIndex_SIZE_FILE=SIZE_FILE+4;
   char szIndexFile[iIndex_SIZE_FILE];
   sprintf(szIndexFile, "%s.idx", g_staticParams.databaseInfo.szDatabase);

   if ((fptr = fopen(szIndexFile, "wb")) == NULL)
   {
      printf(" Error - cannot open index file %s to write\n", szIndexFile);
      exit(1);
   }

   sprintf(szOut, " Creating peptide index file: ");
   logout(szOut);
   fflush(stdout);

   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);

   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   tp->fillPool( g_staticParams.options.iNumThreads < 0 ? 0 : g_staticParams.options.iNumThreads-1);  
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
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error in RunSearch() for peptide index creation.\n");
      logerr(szErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // sanity check
   if (g_pvDBIndex.size() == 0)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg, " Error: no peptides in index; check the input database file or search parameters.\n");
      logerr(szErrorMsg);
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
      return false;
   }

   // remove duplicates
   sprintf(szOut, " - removing duplicates\n");
   logout(szOut);
   fflush(stdout);

   // keep unique entries only; sort by peptide/modification state and protein
   // first sort by peptide, then mod state, then protein file position
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CometFragmentIndex::CompareByPeptide);

   // At this point, need to create g_pvProteinsList protein file position vector of vectors to map each peptide
   // to every protein. g_pvDBIndex.at().lProteinFilePosition is now reference to protein vector entry
   vector<vector<comet_fileoffset_t>> g_pvProteinsList;
   vector<comet_fileoffset_t> temp;  // stores list of duplicate proteins which gets pushed to g_pvProteinsList

   // Create g_pvProteinsList.  This is a vector of vectors.  Each element is vector list
   // of duplicate proteins (generated as "temp") ... these are generated by looping
   // through g_pvDBIndex and looking for consecutive, same peptides.  Once the "temp"
   // vector is assigned the lIndexProteinFilePosition offset, the g_pvDBIndex entry is
   // is assigned lProtCount to lIndexProteinFilePosition.  This is used later to look up
   // the right vector element of duplicate proteins later.
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
         if (!strcmp(g_pvDBIndex.at(i).szPeptide, g_pvDBIndex.at(i-1).szPeptide))
         {
            temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
            g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
         }
         else
         {
            // different peptide + mod state so go ahead and push temp onto g_pvProteinsList
            // and store current protein reference into new temp
            // temp can have duplicates due to mod forms of peptide so make unique here
            sort(temp.begin(), temp.end());
            temp.erase(unique(temp.begin(), temp.end()), temp.end() );
            g_pvProteinsList.push_back(temp);

            lProtCount++; // start new row in g_pvProteinsList
            temp.clear();
            temp.push_back(g_pvDBIndex.at(i).lIndexProteinFilePosition);
            g_pvDBIndex.at(i).lIndexProteinFilePosition = lProtCount;
         }
      }
   }
   // now at end of loop, push last temp onto g_pvProteinsList
   sort(temp.begin(), temp.end());
   temp.erase(unique(temp.begin(), temp.end()), temp.end() );
   g_pvProteinsList.push_back(temp);

   g_pvDBIndex.erase(unique(g_pvDBIndex.begin(), g_pvDBIndex.end()), g_pvDBIndex.end());

   // sort by mass;
   sort(g_pvDBIndex.begin(), g_pvDBIndex.end(), CometFragmentIndex::CompareByMass);

/*
   for (std::vector<DBIndex>::iterator it = g_pvDBIndex.begin(); it != g_pvDBIndex.end(); ++it)
   {
      printf("OK after unique ");
      if ((*it).pcVarModSites[strlen((*it).szPeptide)] != 0)
         printf("n*");
      for (unsigned int x = 0; x < strlen((*it).szPeptide); x++)
      {
         printf("%c", (*it).szPeptide[x]);
         if ((*it).pcVarModSites[x] != 0)
            printf("*");
      }
      if ((*it).pcVarModSites[strlen((*it).szPeptide) + 1] != 0)
         printf("c*");
      printf("   %f   %lld\n", (*it).dPepMass, (*it).lIndexProteinFilePosition);
   }
   printf("\n");
*/

   sprintf(szOut, " - writing file\n");
   logout(szOut);
   fflush(stdout);

   // write out index header
   fprintf(fptr, "Comet peptide index database.  Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fptr, "InputDB:  %s\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fptr, "MassRange: %lf %lf\n", g_staticParams.options.dPeptideMassLow, g_staticParams.options.dPeptideMassHigh);
   fprintf(fptr, "LengthRange: %d %d\n", g_staticParams.options.peptideLengthRange.iStart, g_staticParams.options.peptideLengthRange.iEnd);
   fprintf(fptr, "MassType: %d %d\n", g_staticParams.massUtility.bMonoMassesParent, g_staticParams.massUtility.bMonoMassesFragment);
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
      fprintf(fptr, " %s %lf:%lf", g_staticParams.variableModParameters.varModList[x].szVarModChar,
            g_staticParams.variableModParameters.varModList[x].dVarModMass,
            g_staticParams.variableModParameters.varModList[x].dNeutralLoss);
   }
   fprintf(fptr, "\n\n");

   // Now write out: vector<vector<comet_fileoffset_t>> g_pvProteinsList
   comet_fileoffset_t clProteinsFilePos = comet_ftell(fptr);
   size_t tTmp = g_pvProteinsList.size();
   fwrite(&tTmp, clSizeCometFileOffset, 1, fptr);
   for (auto it = g_pvProteinsList.begin(); it != g_pvProteinsList.end(); ++it)
   {
      tTmp = (*it).size();
      fwrite(&tTmp, sizeof(size_t), 1, fptr);
      for (size_t it2 = 0; it2 < tTmp; ++it2)
      {
         fwrite(&((*it).at(it2)), clSizeCometFileOffset, 1, fptr);
      }
   }

   // next write out the peptides and track peptide mass index
   int iMaxPeptideMass = (int)(g_staticParams.options.dPeptideMassHigh);
   int iMaxPeptideMass10 = iMaxPeptideMass * 10;  // make mass index at resolution of 0.1 Da
   comet_fileoffset_t *lIndex = new comet_fileoffset_t[iMaxPeptideMass10 + 1];
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
//    fwrite((*it).szPrevNextAA, sizeof(char), 2, fptr);

      // write out for char 0=no mod, N=mod.  If N, write out var mods as N pairs (pos,whichmod)
      int iLen2 = iLen + 2;
      unsigned char cNumMods = 0; 
      for (unsigned char x=0; x<iLen2; x++)
      {
         if ((*it).pcVarModSites[x] != 0)
            cNumMods++;
      }
      fwrite(&cNumMods, sizeof(unsigned char), 1, fptr);

      if (cNumMods > 0)
      {
         for (unsigned char x=0; x<iLen2; x++)
         {
            if ((*it).pcVarModSites[x] != 0)
            {
               char cWhichMod = (*it).pcVarModSites[x];
               fwrite(&x, sizeof(unsigned char), 1, fptr);
               fwrite(&cWhichMod , sizeof(char), 1, fptr);
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

   sprintf(szOut, " - done\n");
   logout(" - done\n");
   fflush(stdout);

   CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

   g_pvDBIndex.clear();
   delete[] lIndex;

   return bSucceeded;
}


void CometPeptideIndex::ReadPeptideIndexEntry(struct DBIndex *sDBI, FILE *fp)
{
   int iLen;
   size_t tTmp;

   tTmp = fread(&iLen, sizeof(int), 1, fp);
   tTmp = fread(sDBI->szPeptide, sizeof(char), iLen, fp);
   sDBI->szPeptide[iLen] = '\0';

   unsigned char cNumMods;  // number of var mods encoded as position:residue pairs
   tTmp = fread(&cNumMods, sizeof(unsigned char), 1, fp);  // read how many var mods are stored

   memset(sDBI->pcVarModSites, 0, sizeof(unsigned char)*iLen+2);
   if (cNumMods > 0)
   {
      for (unsigned char x=0; x<cNumMods; x++)
      {
         unsigned char cPosition;
         char cResidue;
         tTmp = fread(&cPosition, sizeof(unsigned char), 1, fp);
         tTmp = fread(&cResidue, sizeof(char), 1, fp);
         sDBI->pcVarModSites[(int)cPosition] = cResidue;
      }
   }
   // done reading mod sites

   tTmp = fread(&(sDBI->dPepMass), sizeof(double), 1, fp);
   tTmp = fread(&(sDBI->lIndexProteinFilePosition), sizeof(comet_fileoffset_t), 1, fp);
}
