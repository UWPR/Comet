/*
   Copyright 2012 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Common.h"
#include "CometIndexDb.h"
#include "CometStatus.h"

#include <stdio.h>
#include <sstream>


CometIndexDb::CometIndexDb()
{
}


CometIndexDb::~CometIndexDb()
{
}



bool CometIndexDb::SortByPeptide(const DBIndex &lhs,
                                 const DBIndex &rhs)
{
   if (!strcmp(lhs.szPeptide, rhs.szPeptide))  // if same peptide, sort by first file positon
      return (lhs.lFP < rhs.lFP);
   else
      return(strcmp(lhs.szPeptide, rhs.szPeptide) < 0);
};


bool CometIndexDb::SortByMass(const DBIndex &lhs,
                              const DBIndex &rhs)
{
   return( lhs.dPepMass < rhs.dPepMass);
}

bool CometIndexDb::SortByFP(const DBIndex &lhs,
                            const DBIndex &rhs)
{
   return( lhs.lFP < rhs.lFP);
}


bool CometIndexDb::CreateIndex(void)

{
   bool bSucceeded = true;
   sDBEntry dbe;
   FILE *fptr;
   int iTmpCh;
   bool bTrimDescr = false;
   char szOut[1024];

   vector<struct DBIndex> vIndex;

   // If creating .idx file, make sure input database does not have .idx extension.
   // If it does, remove the .idx extension before trying to parse it as a fasta file.
   if (!strcmp(g_staticParams.databaseInfo.szDatabase+strlen(g_staticParams.databaseInfo.szDatabase)-4, ".idx"))
   {
      sprintf(szOut, " Error - input database has .idx extension: %s\n", g_staticParams.databaseInfo.szDatabase);
      logout(szOut);
      return false;
   }

   if ((fptr=fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
   {
      char szErrorMsg[1024];
      sprintf(szErrorMsg, " Error - cannot read database file \"%s\".\n", g_staticParams.databaseInfo.szDatabase);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   fseek(fptr, 0, SEEK_END);
   rewind(fptr);

   // Load database entry header.

   char szBuf[8192];
   int iProtNum = 0;

   sprintf(szOut, " - generating peptides\n");
   logout(szOut);
   fflush(stdout);

   vector<struct IndexProteinStruct> vProtein;  // just store a list of protein names

   // Loop through entire database.
   iTmpCh = getc(fptr);
   while (!feof(fptr))
   {
      dbe.strName = "";
      dbe.strSeq = "";

      // skip through whitespace at head of line
      while (isspace(iTmpCh))
         iTmpCh = getc(fptr);

      // skip comment lines
      if (iTmpCh == '#')
      {
         // skip to description line
         while ((iTmpCh != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF))
            iTmpCh = getc(fptr);
      }

      if (iTmpCh == '>') // Expect a '>' for sequence header line.
      {
         // grab file pointer here for this sequence entry
         // this will be stored for protein references for each matched entry and
         // will be used to retrieve actual protein references when printing output
         dbe.lProteinFilePosition = ftell(fptr);

         bTrimDescr = false;
         while (((iTmpCh = getc(fptr)) != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF))
         {
            // Don't bother storing description text past first blank.
            if (!bTrimDescr && (isspace(iTmpCh) || iscntrl(iTmpCh)))
               bTrimDescr = true;

            if (!bTrimDescr && dbe.strName.size() < (WIDTH_REFERENCE-1))
               dbe.strName += iTmpCh;
         }

         // Load sequence
         while (((iTmpCh=getc(fptr)) != '>') && (iTmpCh != EOF))
         {
            if ('a'<=iTmpCh && iTmpCh<='z')
            {
               dbe.strSeq += iTmpCh - 27;  // convert toupper case so subtract 27 (i.e. 'A'-'a')
            }
            else if (33<=iTmpCh && iTmpCh<=126)
            {
               dbe.strSeq += iTmpCh;
            }
         }

         CometIndexDb sqIndex;
         sqIndex.DoIndex(dbe, vIndex, iProtNum);

         struct IndexProteinStruct sEntry;
         strcpy(sEntry.szProt, dbe.strName.c_str());

         // store protein name
         vProtein.push_back(sEntry);

         iProtNum++;

         bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
         if (!bSucceeded)
            break;
      }
      else
      {
         fgets(szBuf, sizeof(szBuf), fptr);
         iTmpCh = getc(fptr);
      }
   }

   // Check for errors one more time since there might have been an error
   // while we were waiting for the threads.
   if (bSucceeded)
      bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();

   fclose(fptr);

   // sanity check
   if (vIndex.size() == 0)
   {
      sprintf(szOut, " Error - no peptides; check the input database file.\n");
      logout(szOut);
      return false;
   }

   // remove duplicates
   sprintf(szOut, " - removing duplicates\n");
   logout(szOut);
   fflush(stdout);
   sort(vIndex.begin(), vIndex.end(), SortByPeptide); // sort by peptide; also sorted by protein ftell position
   vIndex.erase(unique(vIndex.begin(), vIndex.end()), vIndex.end());

   // sort by mass;
   sort(vIndex.begin(), vIndex.end(), SortByMass);

   // write output file

   char szIndexFile[SIZE_FILE];
   sprintf(szIndexFile, "%s.idx", g_staticParams.databaseInfo.szDatabase);
   sprintf(szOut, " - writing index file:  %s\n", szIndexFile);
   logout(szOut);
   fflush(stdout);
   if ( (fptr=fopen(szIndexFile, "wb"))==NULL)
   {
      printf(" Error - cannot open file %s to write\n", szIndexFile);
      exit(1);
   }

   long *lProteinIndex = new long[iProtNum];
   for (int i=0; i<iProtNum; i++)
      lProteinIndex[i] = -1;

   // first just write out protein names. Track file position of each protein name
   int i=0;
   for (std::vector<IndexProteinStruct>::iterator it=vProtein.begin(); it != vProtein.end(); ++it)
   {
      lProteinIndex[i] = ftell(fptr);
      fwrite((*it).szProt, sizeof(char)*WIDTH_REFERENCE, 1, fptr);
      i++;
   }

   // next write out the peptides and track peptide mass index
   int iMaxPeptideMass = (int)(g_staticParams.options.dPeptideMassHigh);

   long *lIndex = new long[iMaxPeptideMass];
   for (int i=0; i<iMaxPeptideMass; i++)
      lIndex[i] = -1;

   // write out struct data
   int iPrevMass=0;
   for (std::vector<DBIndex>::iterator it=vIndex.begin(); it != vIndex.end(); ++it)
   {
      if ( (int)((*it).dPepMass)> iPrevMass)
      {
         iPrevMass = (int)((*it).dPepMass);
         if (iPrevMass < iMaxPeptideMass)
            lIndex[iPrevMass] = ftell(fptr);
      }
      // add composition
      for (int i=0; i<26; i++)
         (*it).iAAComposition[i]=0;
      for (int i=0; i<(int)strlen((*it).szPeptide); i++)
         (*it).iAAComposition[(*it).szPeptide[i] - 65] += 1;

      (*it).lFP = lProteinIndex[(int) (*it).lFP];
      fwrite(&(*it), sizeof(struct DBIndex), 1, fptr);
   }


   long lEndOfPeptides = ftell(fptr); 

   iTmpCh = (int)(g_staticParams.options.dPeptideMassLow);
   fwrite(&iTmpCh, sizeof(int), 1, fptr);  // write min mass
   fwrite(&iMaxPeptideMass, sizeof(int), 1, fptr);  // write max mass
   iTmpCh = vIndex.size();
   fwrite(&iTmpCh, sizeof(int), 1, fptr);  // write # of peptides
   fwrite(lIndex, sizeof(long), iMaxPeptideMass, fptr); // write index
   fwrite(&lEndOfPeptides, sizeof(long), 1, fptr);  // write ftell position of min/max mass, # peptides, peptide index

   fclose(fptr);
   delete [] lIndex;
   delete [] lProteinIndex;

   sprintf(szOut, " Done.\n\n");
   logout(szOut);
   return bSucceeded;
}


bool CometIndexDb::DoIndex(sDBEntry dbe, vector<struct DBIndex> &vIndex, int iProtNum)
{
   // Standard protein database search.
   if (g_staticParams.options.iWhichReadingFrame == 0)
   {
      _proteinInfo.iProteinSeqLength = dbe.strSeq.size();
      _proteinInfo.lProteinFilePosition = dbe.lProteinFilePosition;

      // have to pass sequence as it can be modified per below
      if (!DigestPeptides((char *)dbe.strSeq.c_str(), vIndex, iProtNum))
         return false;
   }
   else
   {
      char szErrorMsg[1024];
      sprintf(szErrorMsg, " Error - indexing of nucleotide database currently not supported\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   return true;
}


// Compare MSMS data to peptide with szProteinSeq from the input database.
bool CometIndexDb::DigestPeptides(char *szProteinSeq,
                                  vector<struct DBIndex> &vIndex,
                                  int iProtNum)
{
   int iLenPeptide = 0;
   int iLenProtein;
   int iProteinSeqLengthMinus1;
   int iStartPos = 0;
   int iEndPos = 0;
   double dCalcPepMass = 0.0;

   Threading::LockMutex(g_dbIndexMutex);

   iLenProtein = _proteinInfo.iProteinSeqLength;

   iProteinSeqLengthMinus1 = iLenProtein-1;

   iEndPos = iStartPos;

   if (iLenProtein > 0)
   {
      dCalcPepMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
         + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];
   }
   else
      return true;

   while (iStartPos < iLenProtein)
   {
      iLenPeptide = iEndPos-iStartPos+1;

      if (iLenPeptide<MAX_PEPTIDE_LEN && WithinDigestRange(dCalcPepMass, szProteinSeq, iStartPos, iEndPos))
      {
         // add to vector
         DBIndex sEntry;
         sEntry.dPepMass = dCalcPepMass;  //MH+ mass
         strncpy(sEntry.szPeptide, szProteinSeq+iStartPos, iEndPos-iStartPos+1);
         sEntry.szPeptide[iEndPos-iStartPos+1]='\0';

         if (iStartPos == 0)
            sEntry.szPrevNextAA[0] = '-';
         else
            sEntry.szPrevNextAA[0] = szProteinSeq[iStartPos-1];
         if (iEndPos == iLenProtein-1)
            sEntry.szPrevNextAA[1] = '-';
         else
            sEntry.szPrevNextAA[1] = szProteinSeq[iEndPos+1];

         sEntry.lFP = iProtNum;
         vIndex.push_back(sEntry);
      }

      // increment end
      if (dCalcPepMass <= g_staticParams.options.dPeptideMassHigh && iEndPos < iProteinSeqLengthMinus1 && iLenPeptide<MAX_PEPTIDE_LEN)
      {
         iEndPos++;

         if (iEndPos < iLenProtein)
            dCalcPepMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndPos]];
      }
      // increment start, reset end
      else if (dCalcPepMass > g_staticParams.options.dPeptideMassHigh || iEndPos==iProteinSeqLengthMinus1 || iLenPeptide == MAX_PEPTIDE_LEN)
      {
         dCalcPepMass -= (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];
         iStartPos++;          // Increment start of peptide.

         // peptide is still potentially larger than input mass so need to delete AA from the end.
         while (dCalcPepMass >= g_staticParams.options.dPeptideMassLow && iEndPos > iStartPos)
         {
            dCalcPepMass -= (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndPos]];
            iEndPos--;
         }
      }
   }

   Threading::UnlockMutex(g_dbIndexMutex);

   return true;
}


bool CometIndexDb::WithinDigestRange(double dCalcPepMass,
                                     char *szProteinSeq,
                                     int iStartPos,
                                     int iEndPos)
{
   if (dCalcPepMass >= g_staticParams.options.dPeptideMassLow
         && dCalcPepMass <= g_staticParams.options.dPeptideMassHigh
         && CheckEnzymeTermini(szProteinSeq, iStartPos, iEndPos))
   {
      return true;
   }
   else
      return false;
}


// Check enzyme termini.
bool CometIndexDb::CheckEnzymeTermini(char *szProteinSeq,
                                      int iStartPos,
                                      int iEndPos)
{
   if (!g_staticParams.options.bNoEnzymeSelected)
   {
      bool bBeginCleavage=0;
      bool bEndCleavage=0;
      bool bBreakPoint;
      int iCountInternalCleavageSites=0;

      bBeginCleavage = (iStartPos==0
            || szProteinSeq[iStartPos-1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iStartPos - g_staticParams.enzymeInformation.iSearchEnzymeOffSet])
               && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iStartPos - g_staticParams.enzymeInformation.iSearchEnzymeOffSet + 1])));

      bEndCleavage = (iEndPos==(int)(_proteinInfo.iProteinSeqLength-1)
            || szProteinSeq[iEndPos+1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iEndPos - g_staticParams.enzymeInformation.iSearchEnzymeOffSet + 1])
               && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iEndPos - g_staticParams.enzymeInformation.iSearchEnzymeOffSet + 2])));

      if (g_staticParams.enzymeInformation.szSearchEnzyme2Name[0] != '\0')
      {
         if (!bBeginCleavage)
         {
            bBeginCleavage = strchr(g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA, szProteinSeq[iStartPos - g_staticParams.enzymeInformation.iSearchEnzyme2OffSet])
               && !strchr(g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA, szProteinSeq[iStartPos - g_staticParams.enzymeInformation.iSearchEnzyme2OffSet + 1]);
         }
         if (!bEndCleavage)
         {
            bEndCleavage = strchr(g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA, szProteinSeq[iEndPos - g_staticParams.enzymeInformation.iSearchEnzyme2OffSet + 1])
               && !strchr(g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA, szProteinSeq[iEndPos - g_staticParams.enzymeInformation.iSearchEnzyme2OffSet + 2]);
         }
      }

      if (g_staticParams.options.iEnzymeTermini == ENZYME_DOUBLE_TERMINI)      // Check full enzyme search.
      {
        if (!(bBeginCleavage && bEndCleavage))
           return false;
      }
      else if (g_staticParams.options.iEnzymeTermini == ENZYME_SINGLE_TERMINI) // Check semi enzyme search.
      {
         if (!(bBeginCleavage || bEndCleavage))
            return false;
      }
      else if (g_staticParams.options.iEnzymeTermini == ENZYME_N_TERMINI)      // Check single n-termini enzyme.
      {
         if (!bBeginCleavage)
            return false;
      }
      else if (g_staticParams.options.iEnzymeTermini == ENZYME_C_TERMINI)      // Check single c-termini enzyme.
      {
         if (!bEndCleavage)
            return false;
      }

      // Check number of missed cleavages count.
      int i;
      for (i=iStartPos; i<=iEndPos; i++)
      {
         bBreakPoint = strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[i+g_staticParams.enzymeInformation.iSearchEnzymeOffSet-1])
            && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[i+g_staticParams.enzymeInformation.iSearchEnzymeOffSet-2]);

         if (bBreakPoint)
         {
            if ((g_staticParams.enzymeInformation.iSearchEnzymeOffSet == 1 && i!=iEndPos)  // Ignore last residue.
                  || (g_staticParams.enzymeInformation.iSearchEnzymeOffSet == 0 && i!= iStartPos))  // Ignore first residue.
            {
               iCountInternalCleavageSites++;

               // Account for iSearchEnzymeOffset -1 below as for AspN cleavage,the very last
               // residue, if followed by a D, will be counted as an internal cleavage site.
               if (iCountInternalCleavageSites + g_staticParams.enzymeInformation.iSearchEnzymeOffSet -1 > g_staticParams.enzymeInformation.iAllowedMissedCleavage)
                  return false;
            }
         }
      }
   }

   return true;
}

