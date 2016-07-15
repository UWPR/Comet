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
#include "CometSearch.h"
#include "ThreadPool.h"
#include "CometStatus.h"

bool *CometSearch::_pbSearchMemoryPool;
bool **CometSearch::_ppbDuplFragmentArr;

CometSearch::CometSearch()
{
   // Initialize the header modification string - won't change.

   // Allocate memory for protein sequence if necessary.

   _iSizepcVarModSites = sizeof(char)*MAX_PEPTIDE_LEN_P2;
}


CometSearch::~CometSearch()
{
}


bool CometSearch::AllocateMemory(int maxNumThreads)
{
   int i;

   // Must be equal to largest possible array
   int iArraySize = (int)((g_staticParams.options.dPeptideMassHigh + 100.0) * g_staticParams.dInverseBinWidth);

   // Initally mark all arrays as available (i.e. false == not in use)
   _pbSearchMemoryPool = new bool[maxNumThreads];
   for (i=0; i < maxNumThreads; i++)
   {
      _pbSearchMemoryPool[i] = false;
   }

   // Allocate array
   _ppbDuplFragmentArr = new bool*[maxNumThreads];
   for (i=0; i < maxNumThreads; i++)
   {
      try
      {
         _ppbDuplFragmentArr[i] = new bool[iArraySize];
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(_ppbDuplFragmentArr[%d]). bad_alloc: %s.\n", iArraySize, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }
   }

   return true;
}


bool CometSearch::DeallocateMemory(int maxNumThreads)
{
   int i;

   delete [] _pbSearchMemoryPool;

   for (i=0; i<maxNumThreads; i++)
   {
      delete [] _ppbDuplFragmentArr[i];
   }

   delete [] _ppbDuplFragmentArr;

   return true;
}


bool CometSearch::RunSearch(int minNumThreads,
                            int maxNumThreads,
                            int iPercentStart,
                            int iPercentEnd)
{
   bool bSucceeded = true;
   sDBEntry dbe;
   FILE *fptr;
   int iTmpCh;
   long lEndPos;
   long lCurrPos;
   bool bTrimDescr;
   string strPeffHeader;
   char *szMods = 0;             // will store ModRes and VariantSimple text for parsing for all entries; resize as needed
   int iLenAllocMods = 0;
   int iLen;

   vector<OBOStruct> vectorPeffOBO;


   // Create the thread pool containing g_staticParams.options.iNumThreads,
   // each hanging around and sleeping until asked to so a search.
   // NOTE: We don't want to read in ALL the database sequences at once or we
   // will run out of memory for large databases, so we specify a
   // "maxNumParamsToQueue" to indicate that, at most, we will only read in
   // and queue "maxNumParamsToQueue" additional parameters (1 in this case)
   ThreadPool<SearchThreadData *> *pSearchThreadPool = new ThreadPool<SearchThreadData *>(SearchThreadProc,
       minNumThreads, maxNumThreads, 1 /*maxNumParamsToQueue*/);

   g_staticParams.databaseInfo.uliTotAACount = 0;
   g_staticParams.databaseInfo.iTotalNumProteins = 0;

   if ((fptr=fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg, " Error - cannot read database file \"%s\".\n", g_staticParams.databaseInfo.szDatabase);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   fseek(fptr, 0, SEEK_END);
   lEndPos=ftell(fptr);
   rewind(fptr);

   // Load database entry header.
   lCurrPos = ftell(fptr);
   iTmpCh = getc(fptr);

   if (g_staticParams.peffInfo.bPEFF)
   {
      // allocate initial storage for mod strings that will be parsed from each def line
      iLenAllocMods = 100;
      szMods = (char*)malloc( iLenAllocMods * sizeof(char));
      if (szMods == NULL)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg, " Error - malloc szMods\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }

      // if PEFF database, make sure OBO file is specified
      if (strlen(g_staticParams.peffInfo.szPeffOBO)==0)
      {
         char szErrorMsg[512];
         sprintf(szErrorMsg,  " Error: \"peff_format = 1\" but \"peff_obo\" is not set\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         exit(1);
      }

      // read in PSI or UniMod file and get a map of all mod codes and mod masses
      CometSearch pOBO;
      if (strlen(g_staticParams.peffInfo.szPeffOBO) > 0)
      {
         pOBO.ReadOBO(g_staticParams.peffInfo.szPeffOBO, &vectorPeffOBO);
         sort(vectorPeffOBO.begin(), vectorPeffOBO.end());  // sort by strMod for efficient binary search
      }
   }

   if (!g_staticParams.options.bOutputSqtStream)
   {
      logout("     - Search progress: ");
      fflush(stdout);
   }

   char szBuf[8192];

   // Loop through entire database.
   while(!feof(fptr))
   {
      // When we created the thread pool above, we specified the max number of
      // additional params to queue. Here, we must call this method if we want
      // to wait for the queued params to be processed by the threads before we
      // load any more params.
      pSearchThreadPool->WaitForQueuedParams();

      dbe.strName = "";
      dbe.strSeq = "";
      dbe.vectorPeffMod.clear();
      dbe.vectorPeffVariantSimple.clear();

      // skip through whitespace at head of line
      while (isspace(iTmpCh))
         iTmpCh = getc(fptr);

      // skip comment lines
      if (iTmpCh == '#')
      {
         // skip to description line
         while ((iTmpCh != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF))
         {
            iTmpCh = getc(fptr);
         }
      }

      if (iTmpCh == '>') // Expect a '>' for sequence header line.
      {
         bTrimDescr = 0;
         while (((iTmpCh = getc(fptr)) != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF))
         {
            // Don't bother storing description text past first blank.
            if (isspace(iTmpCh) || iscntrl(iTmpCh))
               bTrimDescr = 1;

            if (!bTrimDescr && dbe.strName.size() < (WIDTH_REFERENCE-1))
               dbe.strName += iTmpCh;

            // load and parse PEFF header
            if (g_staticParams.peffInfo.bPEFF)
            {
               if (iTmpCh == '\\')
               {
                  char *szLine = NULL;
                  size_t len = 0;

                  ungetc(iTmpCh, fptr);
                  getline(&szLine, &len, fptr);

                  // grab from \ModRes and \Variant to end of line

                  char *pStr;

                  if ( (pStr = strstr(szLine, "\\ModRes=")) != NULL)
                  {
                     char *pStr2;
                     pStr += 8;

                     if ( (pStr2 = strchr(pStr, ' '))!=NULL)
                        iLen = (int)(strlen(pStr)-strlen(pStr2));
                     else
                        iLen = (int)(strlen(pStr));

                     if ( iLen > iLenAllocMods-1)
                     {
                        char *pTmp;

                        iLenAllocMods = iLen + 100;

                        pTmp=(char *)realloc(szMods, iLenAllocMods);
                        if (pTmp == NULL)
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  " Error realloc(szMods[%d])\n", iLenAllocMods);
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                           return false;
                        }
                        szMods = pTmp;
                     }

                     strncpy(szMods, pStr, iLen);
                     szMods[iLen]=0;

//printf("MODRES '%s'\n", szMods);

                     // parse ModRes entries
                     char *tok;
                     char delims[] = ")";  // tokenize by tab
                     int iPos;
                     char szTmp[80];
                     string strModCode;
                     tok = strtok(szMods, delims);
                     while (tok != NULL)
                     {
                        szTmp[0]='\0';
                        iPos = 0;

                        sscanf(tok, "(%d|%79s", &iPos, szTmp);  //tok+1 to skip first '(' char
                        strModCode = szTmp;

                        // sanity check: make sure position is positive
                        if (iPos < 0)
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  "Warning:  %s, ModRes=(%d|%s) ignored\n", dbe.strName.c_str(), iPos, strModCode.c_str());
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                        }
                        else if (iPos > 0)
                        {
                           struct PeffModStruct pData;
                           CometSearch pOBO;

                           pData.iPosition = iPos;

                           // find strModCode in vectorPeffOBO and get pData.dMassDiffAvg and pData.MassDiffMono
                           if (pOBO.MapOBO(strModCode, &vectorPeffOBO, &pData))
                              dbe.vectorPeffMod.push_back(pData);
                        }

                        tok = strtok(NULL, delims);
                     }
                  }

                  if ( (pStr = strstr(szLine, "\\VariantSimple=")) != NULL)
                  {
                     char *pStr2;
                     pStr += 15;

                     if ( (pStr2 = strchr(pStr, ' '))!=NULL)
                        iLen = (int)(strlen(pStr)-strlen(pStr2));
                     else
                        iLen = (int)(strlen(pStr));

                     if ( iLen > iLenAllocMods-1)
                     {
                        char *pTmp;

                        iLenAllocMods = iLen + 100;

                        pTmp=(char *)realloc(szMods, iLenAllocMods);
                        if (pTmp == NULL)
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  " Error realloc(szMods[%d])\n", iLenAllocMods);
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                           return false;
                        }
                        szMods = pTmp;
                     }

                     strncpy(szMods, pStr, iLen);
                     szMods[iLen]=0;

                     // parse ModRes entries
                     char *tok;
                     char delims[] = ")";  // tokenize by tab
                     int iPos;
                     char cVariant;
                     tok = strtok(szMods, delims);
                     while (tok != NULL && strlen(tok)>=3)
                     {
                        iPos = -1;
                        cVariant = 0;

                        sscanf(tok+1, "%d|%c", &iPos, &cVariant);  //tok+1 to skip first '(' char

                        // sanity check: make sure position is positive and residue is A-Z
                        if (iPos <0 || cVariant<65 || cVariant>90)
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  "Warning:  %s, VariantSimple=(%d|%c) ignored\n", dbe.strName.c_str(), iPos, cVariant);
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                        }
                        else
                        {
                           struct PeffVariantSimpleStruct pData;

                           pData.iPosition = iPos;
                           pData.cResidue = cVariant;
                           dbe.vectorPeffVariantSimple.push_back(pData);
                        }

                        tok = strtok(NULL, delims);
                     }
                  }

                  // exit out of this as end of line grabbed
                  break;
               }
            } // done with PEFF
         }

/*
for (int i=0; i<(int)dbe.vectorPeffMod.size(); i++)
   printf("OK modification  %d, %f\n", dbe.vectorPeffMod.at(i).iPosition, dbe.vectorPeffMod.at(i).dMassDiffMono);
for (int i=0; i<(int)dbe.vectorPeffVariantSimple.size(); i++)
   printf("OK variant %d, %c\n", dbe.vectorPeffVariantSimple.at(i).iPosition, dbe.vectorPeffVariantSimple.at(i).cResidue);
*/

         dbe.iSeqFilePosition = ftell(fptr);  // grab sequence file position here

         // Load sequence
         while (((iTmpCh=getc(fptr)) != '>') && (iTmpCh != EOF))
         {
            if (33<=iTmpCh && iTmpCh<=126) // ASCII physical character range.
            {
               // Convert all sequences to upper case.
               dbe.strSeq += toupper(iTmpCh);
               g_staticParams.databaseInfo.uliTotAACount++;
            }
         }

         g_staticParams.databaseInfo.iTotalNumProteins++;

         if (!g_staticParams.options.bOutputSqtStream
               && !(g_staticParams.databaseInfo.iTotalNumProteins%200))
         {
            char szTmp[128];
            lCurrPos = ftell(fptr);
            // go from iPercentStart to iPercentEnd, scaled by lCurrPos/iEndPos
            sprintf(szTmp, "%3d%%", (int)(((double)(iPercentStart + (iPercentEnd-iPercentStart)*(double)lCurrPos/(double)lEndPos) )));
            logout(szTmp);
            fflush(stdout);
            logout("\b\b\b\b");
         }

         // Now search sequence entry; add threading here so that
         // each protein sequence is passed to a separate thread.
         SearchThreadData *pSearchThreadData = new SearchThreadData(dbe);
         pSearchThreadPool->Launch(pSearchThreadData);

         bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
         if (!bSucceeded)
         {
            break;
         }
      }
      else
      {
         fgets(szBuf, sizeof(szBuf), fptr);
         iTmpCh = getc(fptr);
      }
   }

   // Wait for active search threads to complete processing.
   pSearchThreadPool->WaitForThreads();

   delete pSearchThreadPool;
   pSearchThreadPool = NULL;

   // Check for errors one more time since there might have been an error
   // while we were waiting for the threads.
   if (bSucceeded)
   {
      bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
   }

   fclose(fptr);

   if (!g_staticParams.options.bOutputSqtStream)
   {
      char szTmp[12];
      sprintf(szTmp, "%3d%%\n", iPercentEnd);
      logout(szTmp);
      fflush(stdout);
   }

   if (g_staticParams.peffInfo.bPEFF)
   {
      free(szMods);
   }

   return bSucceeded;
}


void CometSearch::ReadOBO(char *szOBO,
                          vector<OBOStruct> *vectorPeffOBO)
{
   FILE *fp;

   if ( (fp=fopen(szOBO, "r")) != NULL)
   {
      char *szLine = NULL;
      size_t len = 0;

      // store UniMod mod string "UNIMOD:1" and mass diffs 'delta_mono_mass "42.010565"' 'delta_avge_mass "42.0367"'
      getline(&szLine, &len, fp);
      while (!feof(fp))
      {
         if (!strncmp(szLine, "[Term]", 6))
         {
            OBOStruct pEntry;

            pEntry.dMassDiffAvg = 0.0;
            pEntry.dMassDiffMono = 0.0;

            // keep reading until next [Term] line or EOF
            // and store id: and mass diffs

            // for UniMod, parse these entries:
            //
            // id: UNIMOD:6
            // xref: delta_mono_mass "58.005479"
            // xref: delta_avge_mass "58.0361"
            //
            // for PSI-Mod, parse these entries:
            // id: MOD:00046
            // xref: DiffAvg: "79.98"
            // xref: DiffMono: "79.966331"

            while (getline(&szLine, &len, fp) != -1)
            {
               char szTmp[80];

               if (!strncmp(szLine, "[Term]", 6))
               {
                  if (pEntry.dMassDiffMono != 0.0)
                  {
                     (*vectorPeffOBO).push_back(pEntry);
//                   printf("OK  mod %s, %f, %f\n\n",  pEntry.strMod.c_str(), pEntry.dMassDiffMono, pEntry.dMassDiffAvg);
                  }

                  break;
               }
               else if (!strncmp(szLine, "id: ", 4))
               {
                  sscanf(szLine, "id: %79s", szTmp);
                  pEntry.strMod = szTmp;
               }
               else if (!strncmp(szLine, "xref: delta_mono_mass ", 22))
                  sscanf(szLine + 22, "\"%lf\"", &pEntry.dMassDiffMono);
               else if (!strncmp(szLine, "xref: delta_avge_mass ", 22))
                  sscanf(szLine + 22, "\"%lf\"", &pEntry.dMassDiffAvg);
               else if (!strncmp(szLine, "xref: DiffAvg: ", 15))
                  sscanf(szLine + 15, "\"%lf\"", &pEntry.dMassDiffAvg);
               else if (!strncmp(szLine, "xref: DiffMono: ", 16))
                  sscanf(szLine + 16, "\"%lf\"", &pEntry.dMassDiffMono);
            }
         }
         else
         {
            getline(&szLine, &len, fp);
         }
      }

      fclose(fp);
   }
   else
   {
      char szErrorMsg[512];
      sprintf(szErrorMsg,  " Warning: cannot read PEFF OBO file \"%s\"\n", g_staticParams.peffInfo.szPeffOBO);
      string strErrorMsg(szErrorMsg);
      logout(szErrorMsg);
   }

}


bool CometSearch::MapOBO(string strMod, vector<OBOStruct> *vectorPeffOBO, struct PeffModStruct *pData)
{
   int iPos;

   pData->dMassDiffAvg = 0;
   pData->dMassDiffMono = 0;

   // find match of strMod in vectorPeffOBO and store diff masses in pData

   iPos=BinarySearchPeffStrMod(0, (*vectorPeffOBO).size(), strMod, *vectorPeffOBO);

   if (iPos != -1 && iPos< (int)(*vectorPeffOBO).size() )
   {
      pData->dMassDiffAvg = (*vectorPeffOBO).at(iPos).dMassDiffAvg;
      pData->dMassDiffMono = (*vectorPeffOBO).at(iPos).dMassDiffMono;

      return true;
   }
   else
   {
      char szErrorMsg[512];
      sprintf(szErrorMsg,  " Warning: cannot find \"%s\" in OBO\n", strMod.c_str());
      string strErrorMsg(szErrorMsg);
      logout(szErrorMsg);

      return false;
   }
}


void CometSearch::SearchThreadProc(SearchThreadData *pSearchThreadData)
{
   // Grab available array from shared memory pool.
   int i;
   Threading::LockMutex(g_searchMemoryPoolMutex);
   for (i=0; i < g_staticParams.options.iNumThreads; i++)
   {
      if (!_pbSearchMemoryPool[i])
      {
         _pbSearchMemoryPool[i] = true;
         break;
      }
   }
   Threading::UnlockMutex(g_searchMemoryPoolMutex);

   // Fail-safe to stop if memory isn't available for the next thread.
   // Needs better capture and return?
   if (i == g_staticParams.options.iNumThreads)
   {
      printf("Error with memory pool.\n");
      exit(1);
   }

   // Give memory manager access to the thread.
   pSearchThreadData->pbSearchMemoryPool = &_pbSearchMemoryPool[i];

   CometSearch sqSearch;
   // DoSearch now returns true/false, but we already log errors and set
   // the global error variable before we get here, so no need to check
   // the return value here.
   sqSearch.DoSearch(pSearchThreadData->dbEntry, _ppbDuplFragmentArr[i]);
   delete pSearchThreadData;
   pSearchThreadData = NULL;
}


bool CometSearch::DoSearch(sDBEntry dbe, bool *pbDuplFragment)
{
   // Standard protein database search.
   if (g_staticParams.options.iWhichReadingFrame == 0)
   {
      _proteinInfo.iProteinSeqLength = dbe.strSeq.size();
      _proteinInfo.iSeqFilePosition = dbe.iSeqFilePosition;

      if (!SearchForPeptides(dbe,
                             (char *)dbe.strSeq.c_str(),  // have to pass sequence as it can be modified per below
                             0,
                             pbDuplFragment,
                             -1))
      {
         return false;
      }

      if (g_staticParams.options.bClipNtermMet && dbe.strSeq[0]=='M')
      {
         _proteinInfo.iProteinSeqLength -= 1;

         if (!SearchForPeptides(dbe,
                                (char *)dbe.strSeq.c_str()+1,
                                1,
                                pbDuplFragment,
                                -1))
         {
            return false;
         }
      }

      // Plug in an AA substitution and do a search, requiring that AA be present
      // in peptide or a flanking residue that causes a enzyme cut site
      if (dbe.vectorPeffVariantSimple.size() > 0)
      {
//       SearchForVariants(dbe, (char *)dbe.strSeq.c_str(), pbDuplFragment);
      }

   }
   else
   {
      int ii;

      // Nucleotide search; translate NA to AA.

      if ((g_staticParams.options.iWhichReadingFrame == 1) ||
          (g_staticParams.options.iWhichReadingFrame == 2) ||
          (g_staticParams.options.iWhichReadingFrame == 3))
      {
         // Specific forward reading frames.
         ii = g_staticParams.options.iWhichReadingFrame - 1;

         // Creates szProteinSeq[] for each reading frame.
         if (!TranslateNA2AA(&ii, 1,(char *)dbe.strSeq.c_str()))
         {
            return false;
         }

         if (!SearchForPeptides(dbe,
                                _proteinInfo.pszProteinSeq,
                                0,
                                pbDuplFragment,
                                -1))
         {
            return false;
         }
      }
      else if ((g_staticParams.options.iWhichReadingFrame == 7) ||
               (g_staticParams.options.iWhichReadingFrame == 9))
      {
         // All 3 forward reading frames.
         for (ii=0; ii<3; ii++)
         {
            if (!TranslateNA2AA(&ii, 1,(char *)dbe.strSeq.c_str()))
            {
               return false;
            }

            if (!SearchForPeptides(dbe,
                                   _proteinInfo.pszProteinSeq,
                                   0,
                                   pbDuplFragment,
                                   -1))
            {
               return false;
            }
         }
      }

      if ((g_staticParams.options.iWhichReadingFrame == 4) ||
          (g_staticParams.options.iWhichReadingFrame == 5) ||
          (g_staticParams.options.iWhichReadingFrame == 6) ||
          (g_staticParams.options.iWhichReadingFrame == 8) ||
          (g_staticParams.options.iWhichReadingFrame == 9))
      {
         char *pszTemp;
         int seqSize;

         // Generate complimentary strand.
         seqSize = dbe.strSeq.size()+1;
         try
         {
            pszTemp= new char[seqSize];
         }
         catch (std::bad_alloc& ba)
         {
            char szErrorMsg[256];
            sprintf(szErrorMsg, " Error - new(szTemp[%d]). bad_alloc: %s.\n", seqSize, ba.what());
            sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
            sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
            string strErrorMsg(szErrorMsg);
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(szErrorMsg);
            return false;
         }

         memcpy(pszTemp, (char *)dbe.strSeq.c_str(), seqSize);
         for (ii=0; ii<seqSize; ii++)
         {
            switch (dbe.strSeq[ii])
            {
               case 'G':
                  pszTemp[ii] = 'C';
                  break;
               case 'C':
                  pszTemp[ii] = 'G';
                  break;
               case 'T':
                  pszTemp[ii] = 'A';
                  break;
               case 'A':
                  pszTemp[ii] = 'T';
                  break;
               default:
                  pszTemp[ii] = dbe.strSeq[ii];
               break;
            }
         }

         if ((g_staticParams.options.iWhichReadingFrame == 8) ||
             (g_staticParams.options.iWhichReadingFrame == 9))
         {
            // 3 reading frames on complementary strand.
            for (ii=0; ii<3; ii++)
            {
               if (!TranslateNA2AA(&ii, -1, pszTemp))
               {
                  return false;
               }

               if (!SearchForPeptides(dbe,
                                      _proteinInfo.pszProteinSeq,
                                      0,
                                      pbDuplFragment,
                                      -1))
               {
                  return false;
               }
            }
         }
         else
         {
            // Specific reverse reading frame ... valid values are 4, 5 or 6.
            ii = 6 - g_staticParams.options.iWhichReadingFrame;

            if (ii == 0)
            {
               ii = 2;
            }
            else if (ii == 2)
            {
               ii=0;
            }

            if (!TranslateNA2AA(&ii, -1, pszTemp))
            {
               return false;
            }

            if (!SearchForPeptides(dbe,
                                   _proteinInfo.pszProteinSeq,
                                   0,
                                   pbDuplFragment,
                                   -1))
            {
               return false;
            }
         }

         delete[] pszTemp;
         pszTemp = NULL;
      }
   }

   return true;
}


// Compare MSMS data to peptide with szProteinSeq from the input database.
bool CometSearch::SearchForPeptides(struct sDBEntry dbe,
                                    char *szProteinSeq,
                                    bool bNtermPeptideOnly,
                                    bool *pbDuplFragment,
                                    bool bRequiredVariantPosition)
{
   int iLenPeptide = 0;
   int iStartPos = 0;
   int iEndPos = 0;
   int piVarModCounts[VMODS];
   int iProteinSeqLengthMinus1 = _proteinInfo.iProteinSeqLength-1;
   int iWhichIonSeries;
   int ctIonSeries;
   int ctLen;
   int ctCharge;
   double dCalcPepMass = 0.0;

   char* szProteinName = (char *)(dbe.strName.c_str());


/*
printf("\n\n");
printf("OK protein %s, length %d\n", szProteinName, (int)strlen(szProteinSeq));
for (int i=0; i<(int)dbe.vectorPeffMod.size(); i++)
   printf("OK *** modification  %d, %f\n", dbe.vectorPeffMod.at(i).iPosition, dbe.vectorPeffMod.at(i).dMassDiffMono); //, dbe.vectorPeffMod.at(i).strMod.c_str());
for (int i=0; i<(int)dbe.vectorPeffVariantSimple.size(); i++)
   printf("OK *** variant %d, %c\n", dbe.vectorPeffVariantSimple.at(i).iPosition, dbe.vectorPeffVariantSimple.at(i).cResidue);
*/

   if (dbe.vectorPeffMod.size() > 0)
      g_staticParams.variableModParameters.bVarModSearch = true;

   if (dbe.vectorPeffMod.size() > 0) // sort vectorPeffMod by iPosition
      sort(dbe.vectorPeffMod.begin(), dbe.vectorPeffMod.end());

   if (dbe.vectorPeffVariantSimple.size() > 0) // sort peffVariantSimpleStruct by iPosition
      sort(dbe.vectorPeffVariantSimple.begin(), dbe.vectorPeffVariantSimple.end());

   memset(piVarModCounts, 0, sizeof(piVarModCounts));

   if (_proteinInfo.iProteinSeqLength > 0)
   {
      dCalcPepMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
         + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[0]];

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         CountVarMods(piVarModCounts, szProteinSeq[iEndPos], iEndPos);
      }
   }

   if (iStartPos == 0)
      dCalcPepMass += g_staticParams.staticModifications.dAddNterminusProtein;
   if (iEndPos == iProteinSeqLengthMinus1)
      dCalcPepMass += g_staticParams.staticModifications.dAddCterminusProtein;

   // Search through entire protein.
   while (iStartPos < _proteinInfo.iProteinSeqLength)
   {
      // Check to see if peptide is within global min/mass range for all queries.
      iLenPeptide = iEndPos-iStartPos+1;

      if (iLenPeptide<MAX_PEPTIDE_LEN)
      {
         int iWhichQuery = WithinMassTolerance(dCalcPepMass, szProteinSeq, iStartPos, iEndPos);

         if (iWhichQuery != -1)
         {
            bool bFirstTimeThroughLoopForPeptide = true;

            // Compare calculated fragment ions against all matching query spectra.
            while (iWhichQuery < (int)g_pvQuery.size())
            {
               if (dCalcPepMass < g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassToleranceMinus)
               {
                  // If calculated mass is smaller than low mass range.
                  break;
               }

               // Mass tolerance check for particular query against this candidate peptide mass.
               if (CheckMassMatch(iWhichQuery, dCalcPepMass))
               {
                  char szDecoyProteinName[WIDTH_REFERENCE];
                  char szDecoyPeptide[MAX_PEPTIDE_LEN_P2];  // Allow for prev/next AA in string.

                  // Calculate ion series just once to compare against all relevant query spectra.
                  if (bFirstTimeThroughLoopForPeptide)
                  {
                     int iLenMinus1 = iEndPos - iStartPos; // Equals iLenPeptide minus 1.

                     bFirstTimeThroughLoopForPeptide = false;

                     int i;
                     double dBion = g_staticParams.precalcMasses.dNtermProton;
                     double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

                     if (iStartPos == 0)
                        dBion += g_staticParams.staticModifications.dAddNterminusProtein;
                     if (iEndPos == iProteinSeqLengthMinus1)
                        dYion += g_staticParams.staticModifications.dAddCterminusProtein;

                     int iPos;
                     for (i=iStartPos; i<iEndPos; i++)
                     {
                        iPos = i-iStartPos;

                        dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[i]];
                        _pdAAforward[iPos] = dBion;

                        dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[iEndPos-iPos]];
                        _pdAAreverse[iPos] = dYion;
                     }

                     // Now get the set of binned fragment ions once to compare this peptide against all matching spectra.
                     for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
                     {
                        for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
                        {
                           iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                           for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                              pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforward, _pdAAreverse))] = false;
                        }
                     }

                     for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
                     {
                        for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
                        {
                           iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                           // As both _pdAAforward and _pdAAreverse are increasing, loop through
                           // iLenPeptide-1 to complete set of internal fragment ions.
                           for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                           {
                              int iVal = BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforward, _pdAAreverse));

                              if (pbDuplFragment[iVal] == false)
                              {
                                 _uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen] = iVal;
                                 pbDuplFragment[iVal] = true;
                              }
                              else
                                 _uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen] = 0;
                           }
                        }
                     }

                     // Also take care of decoy here.
                     if (g_staticParams.options.iDecoySearch)
                     {
#ifdef _WIN32
                        _snprintf(szDecoyProteinName, WIDTH_REFERENCE, "%s%s", g_staticParams.szDecoyPrefix, szProteinName);
                        szDecoyProteinName[WIDTH_REFERENCE-1]=0;  // _snprintf does not guarantee null termination
#else
                        snprintf(szDecoyProteinName, WIDTH_REFERENCE, "%s%s", g_staticParams.szDecoyPrefix, szProteinName);
#endif
                        // Generate reverse peptide.  Keep prev and next AA in szDecoyPeptide string.
                        // So actual reverse peptide starts at position 1 and ends at len-2 (as len-1
                        // is next AA).

                        // Store flanking residues from original sequence.
                        if (iStartPos==0)
                           szDecoyPeptide[0]='-';
                        else
                           szDecoyPeptide[0]=szProteinSeq[iStartPos-1];

                        if (iEndPos == iProteinSeqLengthMinus1)
                           szDecoyPeptide[iLenPeptide+1]='-';
                        else
                           szDecoyPeptide[iLenPeptide+1]=szProteinSeq[iEndPos+1];
                        szDecoyPeptide[iLenPeptide+2]='\0';

                        if (g_staticParams.enzymeInformation.iSearchEnzymeOffSet==1)
                        {
                           // Last residue stays the same:  change ABCDEK to EDCBAK.
                           for (i=iEndPos-1; i>=iStartPos; i--)
                              szDecoyPeptide[iEndPos-i] = szProteinSeq[i];

                           szDecoyPeptide[iEndPos-iStartPos+1]=szProteinSeq[iEndPos];  // Last residue stays same.
                        }
                        else
                        {
                           // First residue stays the same:  change ABCDEK to AKEDCB.
                           for (i=iEndPos; i>=iStartPos+1; i--)
                              szDecoyPeptide[iEndPos-i+2] = szProteinSeq[i];

                           szDecoyPeptide[1]=szProteinSeq[iStartPos];  // First residue stays same.
                        }

                        // Now given szDecoyPeptide, calculate pdAAforwardDecoy and pdAAreverseDecoy.
                        dBion = g_staticParams.precalcMasses.dNtermProton;
                        dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

                        if (iStartPos == 0)
                           dBion += g_staticParams.staticModifications.dAddNterminusProtein;
                        if (iEndPos == iProteinSeqLengthMinus1)
                           dYion += g_staticParams.staticModifications.dAddCterminusProtein;

                        int iDecoyStartPos;       // This is start/end for newly created decoy peptide
                        int iDecoyEndPos;
                        int iTmp;

                        iDecoyStartPos = 1;
                        iDecoyEndPos = strlen(szDecoyPeptide)-2;

                        for (i=iDecoyStartPos; i<iDecoyEndPos; i++)
                        {
                           iTmp = i-iDecoyStartPos;

                           dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szDecoyPeptide[i]];
                           _pdAAforwardDecoy[iTmp] = dBion;

                           dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szDecoyPeptide[iDecoyEndPos - iTmp]];
                           _pdAAreverseDecoy[iTmp] = dYion;
                        }

                        for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
                        {
                           for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
                           {
                              iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                              for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                              {
                                 pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforwardDecoy, _pdAAreverseDecoy))] = false;
                              }
                           }
                        }

                        // Now get the set of binned fragment ions once to compare this peptide against all matching spectra.
                        for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
                        {
                           for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
                           {
                              iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                              // As both _pdAAforward and _pdAAreverse are increasing, loop through
                              // iLenPeptide-1 to complete set of internal fragment ions.
                              for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                              {
                                 int iVal = BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforwardDecoy, _pdAAreverseDecoy));

                                 if (pbDuplFragment[iVal] == false)
                                 {
                                    _uiBinnedIonMassesDecoy[ctCharge][ctIonSeries][ctLen] = iVal;
                                    pbDuplFragment[iVal] = true;
                                 }
                                 else
                                    _uiBinnedIonMassesDecoy[ctCharge][ctIonSeries][ctLen] = 0;
                              }
                           }
                        }
                     }
                  }

                  char pcVarModSites[4]; // This is unused variable mod placeholder to pass into XcorrScore.

                  if (!g_staticParams.variableModParameters.bRequireVarMod)
                  {
                     XcorrScore(szProteinSeq, szProteinName, iStartPos, iEndPos, false,
                           dCalcPepMass, false, iWhichQuery, iLenPeptide, pcVarModSites, &dbe);

                     if (g_staticParams.options.iDecoySearch)
                     {
                        XcorrScore(szDecoyPeptide, szDecoyProteinName, 1, iLenPeptide, false,
                              dCalcPepMass, true, iWhichQuery, iLenPeptide, pcVarModSites, &dbe);
                     }
                  }

               }
               iWhichQuery++;
            }
         }
      }

      // Increment end.
      if (dCalcPepMass <= g_massRange.dMaxMass && iEndPos < iProteinSeqLengthMinus1 && iLenPeptide<MAX_PEPTIDE_LEN)
      {
         iEndPos++;

         if (iEndPos < _proteinInfo.iProteinSeqLength)
         {
            dCalcPepMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndPos]];

            if (g_staticParams.variableModParameters.bVarModSearch)
               CountVarMods(piVarModCounts, szProteinSeq[iEndPos], iEndPos);

            if (iEndPos == iProteinSeqLengthMinus1)
               dCalcPepMass += g_staticParams.staticModifications.dAddCterminusProtein;
         }
      }

      // Increment start, reset end.
      else if (dCalcPepMass > g_massRange.dMaxMass || iEndPos==iProteinSeqLengthMinus1 || iLenPeptide == MAX_PEPTIDE_LEN)
      {
         // Run variable mod search before incrementing iStartPos.
         if (g_staticParams.variableModParameters.bVarModSearch)
         {
            // If any variable mod mass is negative, consider adding to iEndPos as long
            // as peptide minus all possible negative mods is less than the dMaxMass????
            //
            // Otherwise, at this point, peptide mass is too big which means should be ok for varmod search.

            if (HasVariableMod(piVarModCounts, iStartPos, iEndPos, &dbe))
            {
               // VariableModSearch also includes looking at PEFF mods
               if (!VariableModSearch(szProteinSeq, szProteinName, piVarModCounts, iStartPos, iEndPos, pbDuplFragment, &dbe))
                  return false;
            }
         
            SubtractVarMods(piVarModCounts, szProteinSeq[iStartPos], iStartPos);
         }

         if (bNtermPeptideOnly)
            return true;

         dCalcPepMass -= (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];
         if (iStartPos == 0)
            dCalcPepMass -= g_staticParams.staticModifications.dAddNterminusProtein;
         iStartPos++;          // Increment start of peptide.

         // Peptide is still potentially larger than input mass so need to delete AA from the end.
         while (dCalcPepMass >= g_massRange.dMinMass && iEndPos > iStartPos)
         {
            dCalcPepMass -= (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndPos]];

            if (g_staticParams.variableModParameters.bVarModSearch)
               SubtractVarMods(piVarModCounts, szProteinSeq[iEndPos], iEndPos);

            if (iEndPos == iProteinSeqLengthMinus1)
               dCalcPepMass -= g_staticParams.staticModifications.dAddCterminusProtein;
            iEndPos--;
         }
      }
   }

   return true;
}


// Analyze regions of the sequence that are affected by the variant
// Each analyzed peptide must either contain the variant or be flanked
// by the variant enabling new enzyme-digested peptide
void CometSearch::SearchForVariants(struct sDBEntry dbe,
                                    char *szProteinSeq,
                                    bool *pbDuplFragment)

{
   int iStartPos;
   int iEndPos;

   int iOneMinusEnzymeOffSet = 1 - g_staticParams.enzymeInformation.iSearchEnzymeOffSet;
   int iTwoMinusEnzymeOffSet = 2 - g_staticParams.enzymeInformation.iSearchEnzymeOffSet;

   // Walk through each variant
   for (int i=0; i<(int)dbe.vectorPeffVariantSimple.size(); i++)
   {
      if (dbe.vectorPeffVariantSimple.at(i).cResidue == szProteinSeq[dbe.vectorPeffVariantSimple.at(i).iPosition])
      {
         // Log a warning message here that the variant change didn't change the residue?
         char szErrorMsg[256];
         sprintf(szErrorMsg, " Warning: protein %s has variant '%c' at position %d with the same original AA residue.\n", 
               dbe.strName.c_str(),
               dbe.vectorPeffVariantSimple.at(i).cResidue,
               dbe.vectorPeffVariantSimple.at(i).iPosition);
         logout(szErrorMsg);
      }
      else
      {
         int iBeginRange;  // first position in sequence to find peptide that still contains variant
         int iEndRange;    // last position in sequence to find peptide that still contains variant
         int iVariantPos;
         bool bBeginCleavage = false;
         bool bEndCleavage = false;
         double dMass;

         iVariantPos = dbe.vectorPeffVariantSimple.at(i).iPosition;

         // Look at the new variant residue and see if it triggers an enzymatic cleavage
         // either at n-term or c-term of position
         bBeginCleavage = (iVariantPos==0
               || szProteinSeq[iVariantPos-1]=='*'
               || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iVariantPos -1 + iOneMinusEnzymeOffSet])
                  && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iVariantPos -1 + iTwoMinusEnzymeOffSet])));

         bEndCleavage = (iVariantPos==(int)(_proteinInfo.iProteinSeqLength-1)
               || szProteinSeq[iVariantPos+1]=='*'
               || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iVariantPos + iOneMinusEnzymeOffSet])
                  && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iVariantPos + iTwoMinusEnzymeOffSet])));

         if (bBeginCleavage) // N-terminal cleavage such as Asp-N:   xxxxx [LtoD xxxxxx 
         {
            // iVariantPos-1 should be end of a peptide
            // iVariantPos should be start of a peptide
            // could also be internal missed cleavage
 
            iBeginRange = iVariantPos;

            // Analyze all peptides that end in iVariantPos-1 or includes it as a missed cleavage
            dMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
               + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iBeginRange]];

            while (dMass < g_massRange.dMaxMass)
            {
               iBeginRange--;
               dMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iBeginRange]];
            }

            iEndRange = iVariantPos;

            dMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
               + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndRange]];

            while (dMass < g_massRange.dMaxMass)
            {
               iEndRange++;
               dMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndRange]];
            }

            // Now analyze all peptides that start at iVariantPos
         }
         else if (bEndCleavage) // C-terminal cleavage:   xxxx LtoK] xxxx
         {

            // Analyze all peptides that end in iVariantPos or includes it as a missed cleavage

            // Now analyze all peptides that begin at iVariantPos+1

         }
         else  // simple AA substitution or no-enzyme selected
         {
            double dMass;

            // Analyze all peptides that include iVariantPos.

            // Find furthest away iStartPos: start at iVariantPos and start adding mass in reverse position until > largest mass
            // Stop if # of missed cleavages exceeded.

            iBeginRange = iVariantPos;
      
            dMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
               + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iBeginRange]];

            while (dMass < g_massRange.dMaxMass && iBeginRange>0)
            {
               iBeginRange--;
               dMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iBeginRange]];
            }

            // Find first away iEndPos: start at iVariantPos and start adding mass forward until > largest mass
            // Stop at # of missed cleavages exceeded.

            iEndRange = iVariantPos;

            dMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
               + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndRange]];

            while (dMass < g_massRange.dMaxMass && iEndRange < (int)strlen(szProteinSeq))
            {
               iEndRange++;
               dMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iEndRange]];
            }

            // Now search from iStartPos to iEndPos with requirement that peptide contain new iVariantPos

            iStartPos = iBeginRange;
            iEndPos = iEndRange;
            
            // Create new protein string and run SearchForPeptides again but with variant position marked
 
            char pNewProteinSeq[128];

            if (iEndPos - iStartPos + 1 >= 128)
            {
               char szErrorMsg[256];
               sprintf(szErrorMsg, " Warning: in SearchForVariants, range too big.  iStartPos=%d, iEndPos=%d\n", iStartPos, iEndPos);
               logout(szErrorMsg);
            }
            strncpy(pNewProteinSeq, szProteinSeq + iStartPos, iEndPos - iStartPos + 1);
            pNewProteinSeq[iEndPos - iStartPos + 1] = '\0';

            int iVariantPosition = (dbe.vectorPeffVariantSimple.at(i).iPosition - 1) - iStartPos;

            pNewProteinSeq[iVariantPosition] = dbe.vectorPeffVariantSimple.at(i).cResidue;

            SearchForPeptides(dbe, pNewProteinSeq, 1, pbDuplFragment, iVariantPosition);
         }
      }

      if (g_staticParams.options.bClipNtermMet
            && dbe.vectorPeffVariantSimple.at(i).iPosition==0
            && dbe.vectorPeffVariantSimple.at(i).cResidue=='M'
            && szProteinSeq[0]!='M')
      {
//FIX
//       SearchForPeptides(dbe, szProteinSeq+1, 1, pbDuplFragment, 0);
      }
   }
}


int CometSearch::WithinMassTolerance(double dCalcPepMass,
                                     char* szProteinSeq,
                                     int iStartPos,
                                     int iEndPos)
{
   if (dCalcPepMass >= g_massRange.dMinMass
         && dCalcPepMass <= g_massRange.dMaxMass
         && CheckEnzymeTermini(szProteinSeq, iStartPos, iEndPos))
   {
      // Now that we know it's within the global mass range of our queries and has
      // proper enzyme termini, check if within mass tolerance of any given entry.

      int iPos;

      // Do a binary search on list of input queries to find matching mass.
      iPos=BinarySearchMass(0, g_pvQuery.size(), dCalcPepMass);

      // Seek back to first peptide entry that matches mass tolerance in case binary
      // search doesn't hit the first entry.
      while (iPos>0 && g_pvQuery.at(iPos)->_pepMassInfo.dPeptideMassTolerancePlus >= dCalcPepMass)
         iPos--;

      if (iPos != -1)
         return iPos;
      else
         return -1;
   }
   else
   {
      return -1;
   }
}


// This function will return true if unmodified peptide mass + any combination of
// PEFF mod is within mass tolerance of any peptide query
bool CometSearch::WithinMassTolerancePeff(double dCalcPepMass,
                                          vector<PeffPositionStruct>* vPeffArray)
{
   int i;

/* 
   // Print out list of PEFF mods
   int ii;
   for (i=0; i<(int) (*vPeffArray).size(); i++)
   {
      printf("*** OK  %d.  position %d\n", i, (*vPeffArray).at(i).iPosition);
      for (ii=0; ii<(int)(*vPeffArray).at(i).vectorWhichPeff.size(); ii++)
      {
         printf(" ... %f %f\n",
               (*vPeffArray).at(i).vectorMassDiffMono.at(ii),
               (*vPeffArray).at(i).vectorMassDiffAvg.at(ii));
      }
   }
*/

   // number of residues with a PEFF mod
   int n = (int)(*vPeffArray).size();

   int a[n],
       len[n],
       j;

   for (i = 0 ; i < n ; i++)
   {
      // number of mods at each residue position
      len[i] = (int)(*vPeffArray).at(i).vectorWhichPeff.size();

      a[i]=0;
   }

   double dMassAddition;
   bool bFirst=true;

   // permute through all combination of PEFF mods; return true if any are of right mass
   while(1)
   {
      if (!bFirst) // skip first iteration of this where there are no mods
      {
         dMassAddition = 0.0;
         for (i=0; i<n; i++)
         {
            if (a[i]>0)
               dMassAddition += (*vPeffArray).at(i).vectorMassDiffMono.at(a[i]-1);
         }

         // if dCalcPepMass + dMassAddition is within mass tol, add these mods

         // At this stage here, just need to see if any PEFF combination is within mass tolerance
         // of any entry.  If so, simply return true here and will repeat the PEFF permutations later.
         
         // Do a binary search on list of input queries to find matching mass.
         int iPos=BinarySearchMass(0, g_pvQuery.size(), dCalcPepMass + dMassAddition);

         // Seek back to first peptide entry that matches mass tolerance in case binary
         // search doesn't hit the first entry.
         while (iPos>0 && g_pvQuery.at(iPos)->_pepMassInfo.dPeptideMassTolerancePlus >= dCalcPepMass)
            iPos--;

         if (iPos != -1)
            return true;
      }
      else
         bFirst=false;

      for (j=n-1; j>=0; j--)
      {
         if(++a[j]<=len[j])
            break;
         else
            a[j]=0;
      }

      if (j<0)
         break;
   }    

   return false;
}


// Check enzyme termini.
bool CometSearch::CheckEnzymeTermini(char *szProteinSeq,
                                  int iStartPos,
                                  int iEndPos)
{
   if (!g_staticParams.options.bNoEnzymeSelected)
   {
      bool bBeginCleavage=0;
      bool bEndCleavage=0;
      bool bBreakPoint;
      int iOneMinusEnzymeOffSet = 1 - g_staticParams.enzymeInformation.iSearchEnzymeOffSet;
      int iTwoMinusEnzymeOffSet = 2 - g_staticParams.enzymeInformation.iSearchEnzymeOffSet;
      int iCountInternalCleavageSites=0;

      bBeginCleavage = (iStartPos==0
            || szProteinSeq[iStartPos-1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iStartPos -1 + iOneMinusEnzymeOffSet])
          && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iStartPos -1 + iTwoMinusEnzymeOffSet])));

      bEndCleavage = (iEndPos==(int)(_proteinInfo.iProteinSeqLength-1)
            || szProteinSeq[iEndPos+1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iEndPos + iOneMinusEnzymeOffSet])
          && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iEndPos + iTwoMinusEnzymeOffSet])));

      // Check full enzyme search.
      if ((g_staticParams.options.iEnzymeTermini == ENZYME_DOUBLE_TERMINI) && !(bBeginCleavage && bEndCleavage))
         return false;

      // Check semi enzyme search.
      if ((g_staticParams.options.iEnzymeTermini == ENZYME_SINGLE_TERMINI) && !(bBeginCleavage || bEndCleavage))
         return false;

      // Check single n-termini enzyme.
      if ((g_staticParams.options.iEnzymeTermini == ENZYME_N_TERMINI) && !bBeginCleavage)
         return false;

      // Check single c-termini enzyme.
      if ((g_staticParams.options.iEnzymeTermini == ENZYME_C_TERMINI) && !bEndCleavage)
         return false;

      // Check number of missed cleavages count.
      int i;
      for (i=iStartPos; i<=iEndPos; i++)
      {
         bBreakPoint = strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[i+iOneMinusEnzymeOffSet])
            && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[i+iTwoMinusEnzymeOffSet]);

         if (bBreakPoint)
         {
            if ((iOneMinusEnzymeOffSet == 0 && i!=iEndPos)  // Ignore last residue.
                  || (iOneMinusEnzymeOffSet == 1 && i!= iStartPos))  // Ignore first residue.
            {
               iCountInternalCleavageSites++;

               // Need to include -iOneMinusEnzymeOffSet in if statement below because for
               // AspN cleavage, the very last residue, if followed by a D, will be counted
               // as an internal cleavage site.
               if (iCountInternalCleavageSites-iOneMinusEnzymeOffSet > g_staticParams.enzymeInformation.iAllowedMissedCleavage)
                  return false;
            }
         }
      }
   }

   return true;
}


int CometSearch::BinarySearchPeffStrMod(int start,
                                    int end,
                                    string strMod,
                                    vector<OBOStruct>& vectorPeffOBO)
{

   // Termination condition: start index greater than end index.
   if (start > end || start == (int)vectorPeffOBO.size())
      return -1;

   // Find the middle element of the vector and use that for splitting
   // the array into two pieces.
   unsigned middle = start + ((end - start) / 2);

   if (vectorPeffOBO.at(middle).strMod <= strMod && strMod <= vectorPeffOBO.at(middle).strMod)
   {
      return middle;
   }
   else if (vectorPeffOBO.at(middle).strMod > strMod)
   {
      return BinarySearchPeffStrMod(start, middle - 1, strMod, vectorPeffOBO);
   }

   return BinarySearchPeffStrMod(middle + 1, end, strMod, vectorPeffOBO);
}


int CometSearch::BinarySearchMass(int start,
                                  int end,
                                  double dCalcPepMass)
{

   // Termination condition: start index greater than end index.
   if (start > end)
      return -1;

   // Find the middle element of the vector and use that for splitting
   // the array into two pieces.
   unsigned middle = start + ((end - start) / 2);

   if (g_pvQuery.at(middle)->_pepMassInfo.dPeptideMassToleranceMinus <= dCalcPepMass
         && dCalcPepMass <= g_pvQuery.at(middle)->_pepMassInfo.dPeptideMassTolerancePlus)
   {
      return middle;
   }
   else if (g_pvQuery.at(middle)->_pepMassInfo.dPeptideMassToleranceMinus > dCalcPepMass)
   {
      return BinarySearchMass(start, middle - 1, dCalcPepMass);
   }

   if ((int)middle+1 < end)
      return BinarySearchMass(middle + 1, end, dCalcPepMass);
   else
      return -1;
}


bool CometSearch::CheckMassMatch(int iWhichQuery,
                                 double dCalcPepMass)
{
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   int iMassOffsetsSize = g_staticParams.vectorMassOffsets.size();

   if ((dCalcPepMass >= pQuery->_pepMassInfo.dPeptideMassToleranceMinus)
         && (dCalcPepMass <= pQuery->_pepMassInfo.dPeptideMassTolerancePlus))
   {
      double dMassDiff = pQuery->_pepMassInfo.dExpPepMass - dCalcPepMass;

      if (g_staticParams.tolerances.iIsotopeError == 0 && iMassOffsetsSize == 0)
      {
         return true;
      }
      else if (iMassOffsetsSize > 0)
      {
         // need to account for both mass offsets and possible isotope offsets

         if (g_staticParams.tolerances.iIsotopeError == 0)
         {
            for (int i=0; i<iMassOffsetsSize; i++)
            {
               if (fabs(dMassDiff - g_staticParams.vectorMassOffsets[i]) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
               {
                  return true;
               }
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 1)
         {
            double dC13diff  = C13_DIFF;

            for (int i=0; i<iMassOffsetsSize; i++)
            {
               double dTmpDiff = dMassDiff - g_staticParams.vectorMassOffsets[i];

               if (     (fabs(dTmpDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance))
               {
                  return true;
               }
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 2)
         {
            double dC13diff  = C13_DIFF;
            double d2C13diff = C13_DIFF + C13_DIFF;

            for (int i=0; i<iMassOffsetsSize; i++)
            {
               double dTmpDiff = dMassDiff - g_staticParams.vectorMassOffsets[i];

               if (     (fabs(dTmpDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance))
               {
                  return true;
               }
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 3)
         {
            double dC13diff  = C13_DIFF;
            double d2C13diff = C13_DIFF + C13_DIFF;
            double d3C13diff = C13_DIFF + C13_DIFF + C13_DIFF;

            for (int i=0; i<iMassOffsetsSize; i++)
            {
               double dTmpDiff = dMassDiff - g_staticParams.vectorMassOffsets[i];

               if (     (fabs(dTmpDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - d3C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance))
               {
                  return true;
               }
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 4)
         {
            double dC13diff  = C13_DIFF;
            double d2C13diff = C13_DIFF + C13_DIFF;
            double d3C13diff = C13_DIFF + C13_DIFF + C13_DIFF;

            for (int i=0; i<iMassOffsetsSize; i++)
            {
               double dTmpDiff = dMassDiff - g_staticParams.vectorMassOffsets[i];

               if (     (fabs(dTmpDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - d3C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff + dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance))
               {
                  return true;
               }
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 5)
         {
            for (int i=0; i<iMassOffsetsSize; i++)
            {
               double dTmpDiff = dMassDiff - g_staticParams.vectorMassOffsets[i];

               if (     (fabs(dTmpDiff)             <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - 4.0070995) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff - 8.014199)  <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff + 4.0070995) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dTmpDiff + 8.014199)  <= pQuery->_pepMassInfo.dPeptideMassTolerance))
               {
                  return true;
               }
            }
            return false;
         }
         else
         {
            char szErrorMsg[256];
            sprintf(szErrorMsg, " Error - iIsotopeError=%d, should not be here!\n", g_staticParams.tolerances.iIsotopeError);
            string strErrorMsg(szErrorMsg);
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(szErrorMsg);
            return false;
         }
      }
      else
      {
         // only deal with isotope offsets; no mass offsets
         if (g_staticParams.tolerances.iIsotopeError == 1)
         {
            double dC13diff  = C13_DIFF;

            // Using C13 isotope mass difference here but likely should
            // be slightly bigger for other elemental contaminents.

            if (     (fabs(dMassDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance))
            {
               return true;
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 2)
         {
            double dC13diff  = C13_DIFF;
            double d2C13diff = C13_DIFF + C13_DIFF;

            // Using C13 isotope mass difference here but likely should
            // be slightly bigger for other elemental contaminents.

            if (     (fabs(dMassDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance))
            {
               return true;
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 3)
         {
            double dC13diff  = C13_DIFF;
            double d2C13diff = C13_DIFF + C13_DIFF;
            double d3C13diff = C13_DIFF + C13_DIFF + C13_DIFF;

            // Using C13 isotope mass difference here but likely should
            // be slightly bigger for other elemental contaminents.

            if (     (fabs(dMassDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - d3C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance))
            {
               return true;
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 4)
         {
            double dC13diff  = C13_DIFF;
            double d2C13diff = C13_DIFF + C13_DIFF;
            double d3C13diff = C13_DIFF + C13_DIFF + C13_DIFF;

            // Using C13 isotope mass difference here but likely should
            // be slightly bigger for other elemental contaminents.

            if (     (fabs(dMassDiff)            <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - d3C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff + dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance))
            {
               return true;
            }
            return false;
         }
         else if (g_staticParams.tolerances.iIsotopeError == 5)
         {
            if (     (fabs(dMassDiff)             <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - 4.0070995) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff - 8.014199)  <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff + 4.0070995) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                  || (fabs(dMassDiff + 8.014199)  <= pQuery->_pepMassInfo.dPeptideMassTolerance))
            {
               return true;
            }
            return false;
         }
         else
         {
            char szErrorMsg[256];
            sprintf(szErrorMsg, " Error - iIsotopeError=%d, should not be here!\n", g_staticParams.tolerances.iIsotopeError);
            string strErrorMsg(szErrorMsg);
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(szErrorMsg);
            return false;
         }
      }
   }

   return false;
}


// For nucleotide search, translate from DNA to amino acid.
bool CometSearch::TranslateNA2AA(int *frame,
                                 int iDirection,
                                 char *szDNASequence)
{
   int i, ii=0;
   int iSeqLength = strlen(szDNASequence);

   if (iDirection == 1)  // Forward reading frame.
   {
      i = (*frame);
      while ((i+2) < iSeqLength)
      {
         if (ii >= _proteinInfo.iAllocatedProtSeqLength)
         {
            char *pTmp;

            pTmp=(char *)realloc(_proteinInfo.pszProteinSeq, ii+100);
            if (pTmp == NULL)
            {
               char szErrorMsg[512];
               sprintf(szErrorMsg,  " Error realloc(szProteinSeq) ... size=%d\n\
 A sequence entry is larger than your system can handle.\n\
 Either add more memory or edit the database and divide\n\
 the sequence into multiple, overlapping, smaller entries.\n", ii);

               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }

            _proteinInfo.pszProteinSeq = pTmp;
            _proteinInfo.iAllocatedProtSeqLength=ii+99;
         }

         *(_proteinInfo.pszProteinSeq+ii) = GetAA(i, 1, szDNASequence);
         i += 3;
         ii++;
      }
      _proteinInfo.iProteinSeqLength = ii;
      _proteinInfo.pszProteinSeq[ii] = '\0';
   }
   else                 // Reverse reading frame.
   {
      i = iSeqLength - (*frame) - 1;
      while (i >= 2)    // positions 2,1,0 makes the last AA
      {
         if (ii >= _proteinInfo.iAllocatedProtSeqLength)
         {
            char *pTmp;

            pTmp=(char *)realloc(_proteinInfo.pszProteinSeq, ii+100);
            if (pTmp == NULL)
            {
               char szErrorMsg[512];
               sprintf(szErrorMsg,  " Error realloc(szProteinSeq) ... size=%d\n\
 A sequence entry is larger than your system can handle.\n\
 Either add more memory or edit the database and divide\n\
 the sequence into multiple, overlapping, smaller entries.\n", ii);

               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }

            _proteinInfo.pszProteinSeq = pTmp;
            _proteinInfo.iAllocatedProtSeqLength = ii+99;
         }

         *(_proteinInfo.pszProteinSeq + ii) = GetAA(i, -1, szDNASequence);
         i -= 3;
         ii++;
      }
      _proteinInfo.iProteinSeqLength = ii;
      _proteinInfo.pszProteinSeq[ii]='\0';
   }

   return true;
}


// GET amino acid from DNA triplets, direction=+/-1.
char CometSearch::GetAA(int i,
                        int iDirection,
                        char *szDNASequence)
{
   int iBase1 = i;
   int iBase2 = i + iDirection;
   int iBase3 = i + iDirection*2;

   if (szDNASequence[iBase1]=='G')
   {
      if (szDNASequence[iBase2]=='T')
         return ('V');
      else if (szDNASequence[iBase2]=='C')
         return ('A');
      else if (szDNASequence[iBase2]=='G')
         return ('G');
      else if (szDNASequence[iBase2]=='A')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('D');
         else if (szDNASequence[iBase3]=='A' || szDNASequence[iBase3]=='G')
            return ('E');
      }
   }

   else if (szDNASequence[iBase1]=='C')
   {
      if (szDNASequence[iBase2]=='T')
         return ('L');
      else if (szDNASequence[iBase2]=='C')
         return ('P');
      else if (szDNASequence[iBase2]=='G')
         return ('R');
      else if (szDNASequence[iBase2]=='A')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('H');
         else if (szDNASequence[iBase3]=='A' || szDNASequence[iBase3]=='G')
            return ('Q');
      }
   }

   else if (szDNASequence[iBase1]=='T')
   {
      if (szDNASequence[iBase2]=='C')
         return ('S');
      else if (szDNASequence[iBase2]=='T')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('F');
         else if (szDNASequence[iBase3]=='A' || szDNASequence[iBase3]=='G')
            return ('L');
      }
      else if (szDNASequence[iBase2]=='A')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('Y');
         else if (szDNASequence[iBase3]=='A' || szDNASequence[iBase3]=='G')
            return ('@');
      }
      else if (szDNASequence[iBase2]=='G')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('C');
         else if (szDNASequence[iBase3]=='A')
            return ('@');
         else if (szDNASequence[iBase3]=='G')
            return ('W');
      }
   }

   else if (szDNASequence[iBase1]=='A')
   {
      if (szDNASequence[iBase2]=='C')
         return ('T');
      else if (szDNASequence[iBase2]=='T')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C' || szDNASequence[iBase3]=='A')
            return ('I');
         else if (szDNASequence[iBase3]=='G')
            return ('M');
      }
      else if (szDNASequence[iBase2]=='A')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('N');
         else if (szDNASequence[iBase3]=='A' || szDNASequence[iBase3]=='G')
            return ('K');
      }
      else if (szDNASequence[iBase2]=='G')
      {
         if (szDNASequence[iBase3]=='T' || szDNASequence[iBase3]=='C')
            return ('S');
         else if (szDNASequence[iBase3]=='A' || szDNASequence[iBase3]=='G')
            return ('R');
      }
   }

   return ('*');

}


// Compares sequence to MSMS spectrum by matching ion intensities.
void CometSearch::XcorrScore(char *szProteinSeq,
                             char *szProteinName,
                             int iStartPos,
                             int iEndPos,
                             bool bFoundVariableMod,
                             double dCalcPepMass,
                             bool bDecoyPep,
                             int iWhichQuery,
                             int iLenPeptide,
                             char *pcVarModSites,
                             struct sDBEntry *dbe)
{
   int  ctLen,
        ctIonSeries,
        ctCharge;
   double dXcorr;
   int iLenPeptideMinus1 = iLenPeptide - 1;

   // Pointer to either regular or decoy uiBinnedIonMasses[][][].
   unsigned int (*p_uiBinnedIonMasses)[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN];

   // Point to right set of arrays depending on target or decoy search.
   if (bDecoyPep)
      p_uiBinnedIonMasses = &_uiBinnedIonMassesDecoy;
   else
      p_uiBinnedIonMasses = &_uiBinnedIonMasses;

   int iWhichIonSeries;
   bool bUseNLPeaks = false;
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   float **ppSparseFastXcorrData;              // use this if bSparseMatrix
   float *pFastXcorrData;                      // use this if not using SparseMatrix

   dXcorr = 0.0;

   int iMax = pQuery->_spectrumInfoInternal.iArraySize/SPARSE_MATRIX_SIZE + 1;
   for (ctCharge=1; ctCharge<=pQuery->_spectrumInfoInternal.iMaxFragCharge; ctCharge++)
   {
      for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
      {
         iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

         if (g_staticParams.ionInformation.bUseNeutralLoss
               && (iWhichIonSeries==ION_SERIES_A || iWhichIonSeries==ION_SERIES_B || iWhichIonSeries==ION_SERIES_Y))
         {
            bUseNLPeaks = true;
         }
         else
            bUseNLPeaks = false;

         if (ctCharge == 1 && bUseNLPeaks)
         {
            ppSparseFastXcorrData = pQuery->ppfSparseFastXcorrDataNL;
            pFastXcorrData = pQuery->pfFastXcorrDataNL;
         }
         else
         {
            ppSparseFastXcorrData = pQuery->ppfSparseFastXcorrData;
            pFastXcorrData = pQuery->pfFastXcorrData;
         }

         int bin,x,y;
         for (ctLen=0; ctLen<iLenPeptideMinus1; ctLen++)
         {
            //MH: newer sparse matrix converts bin to sparse matrix bin
            bin = *(*(*(*p_uiBinnedIonMasses + ctCharge)+ctIonSeries)+ctLen);
            x = bin / SPARSE_MATRIX_SIZE;
            if (ppSparseFastXcorrData[x]==NULL || x>iMax) // x should never be > iMax so this is just a safety check
               continue;
            y = bin - (x*SPARSE_MATRIX_SIZE);
            dXcorr += ppSparseFastXcorrData[x][y];
         }

         // *(*(*(*p_uiBinnedIonMasses + ctCharge)+ctIonSeries)+ctLen) gives uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen].
      }
   }

   if (dXcorr < XCORR_CUTOFF)
      dXcorr = XCORR_CUTOFF;
   else
      dXcorr *= 0.005;  // Scale intensities to 50 and divide score by 1E4.


   Threading::LockMutex(pQuery->accessMutex);

   // Increment matched peptide counts.
   if (bDecoyPep && g_staticParams.options.iDecoySearch == 2)
      pQuery->_uliNumMatchedDecoyPeptides++;
   else
      pQuery->_uliNumMatchedPeptides++;

   if (g_staticParams.options.bPrintExpectScore
         || g_staticParams.options.bOutputPepXMLFile
         || g_staticParams.options.bOutputPercolatorFile
         || g_staticParams.options.bOutputTxtFile)
   {
      int iTmp;

      iTmp = (int)(dXcorr * 10.0 + 0.5);

      if (iTmp < 0) // possible for CRUX compiled option to have a negative xcorr
         iTmp = 0;  // lump these all in the zero bin of the histogram

      if (iTmp >= HISTO_SIZE)
         iTmp = HISTO_SIZE - 1;

      pQuery->iXcorrHistogram[iTmp] += 1;
      pQuery->iHistogramCount += 1;
   }

   if (bDecoyPep && g_staticParams.options.iDecoySearch==2)
   {
      if (dXcorr > pQuery->fLowestDecoyCorrScore)
      {
         if (!CheckDuplicate(iWhichQuery, iStartPos, iEndPos, bFoundVariableMod, dCalcPepMass,
                  szProteinSeq, szProteinName, 1, pcVarModSites))
         {
            StorePeptide(iWhichQuery, iStartPos, iEndPos, bFoundVariableMod, szProteinSeq,
                  dCalcPepMass, dXcorr, szProteinName, 1,  pcVarModSites, dbe);
         }
      }
   }
   else
   {
      if (dXcorr > pQuery->fLowestCorrScore)
      {
         if (!CheckDuplicate(iWhichQuery, iStartPos, iEndPos, bFoundVariableMod, dCalcPepMass,
                  szProteinSeq, szProteinName, 0, pcVarModSites))
         {
            StorePeptide(iWhichQuery, iStartPos, iEndPos, bFoundVariableMod, szProteinSeq,
                  dCalcPepMass, dXcorr, szProteinName, 0, pcVarModSites, dbe);
         }
      }
   }

   Threading::UnlockMutex(pQuery->accessMutex);
}


double CometSearch::GetFragmentIonMass(int iWhichIonSeries,
                                       int i,
                                       int ctCharge,
                                       double *_pdAAforward,
                                       double *_pdAAreverse)
{
   double dFragmentIonMass = 0.0;

   switch (iWhichIonSeries)
   {
      case ION_SERIES_B:
         dFragmentIonMass = _pdAAforward[i];
         break;
      case ION_SERIES_Y:
         dFragmentIonMass = _pdAAreverse[i];
         break;
      case ION_SERIES_A:
         dFragmentIonMass = _pdAAforward[i] - g_staticParams.massUtility.dCO;
         break;
      case ION_SERIES_C:
         dFragmentIonMass = _pdAAforward[i] + g_staticParams.massUtility.dNH3;
         break;
      case ION_SERIES_Z:
         dFragmentIonMass = _pdAAreverse[i] - g_staticParams.massUtility.dNH2;
         break;
      case ION_SERIES_X:
         dFragmentIonMass = _pdAAreverse[i] + g_staticParams.massUtility.dCOminusH2;
         break;
   }

   return (dFragmentIonMass + (ctCharge-1)*PROTON_MASS)/ctCharge;
}


void CometSearch::StorePeptide(int iWhichQuery,
                               int iStartPos,
                               int iEndPos,
                               bool bFoundVariableMod,
                               char *szProteinSeq,
                               double dCalcPepMass,
                               double dXcorr,
                               char *szProteinName,
                               bool bStoreSeparateDecoy,
                               char *pcVarModSites,
                               struct sDBEntry *dbe)
{
   int i;
   int iLenPeptide;
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   iLenPeptide = iEndPos - iStartPos + 1;

   if (iLenPeptide >= MAX_PEPTIDE_LEN)
      return;

   if (bStoreSeparateDecoy)
   {
      short siLowestDecoySpScoreIndex;

      siLowestDecoySpScoreIndex = pQuery->siLowestDecoySpScoreIndex;

      pQuery->iDecoyMatchPeptideCount++;
      pQuery->_pDecoys[siLowestDecoySpScoreIndex].iLenPeptide = iLenPeptide;

      memcpy(pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPeptide, szProteinSeq+iStartPos, iLenPeptide);
      pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPeptide[iLenPeptide]='\0';

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].dPepMass = dCalcPepMass;

      if (pQuery->_spectrumInfoInternal.iChargeState > 2)
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].iTotalIons
            = (iLenPeptide-1)*(pQuery->_spectrumInfoInternal.iChargeState-1)
               * g_staticParams.ionInformation.iNumIonSeriesUsed;
      }
      else
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].iTotalIons
            = (iLenPeptide-1)*g_staticParams.ionInformation.iNumIonSeriesUsed;
      }

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].fXcorr = (float)dXcorr;

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].iDuplicateCount = 0;

      if (iStartPos == 0)
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0] = '-';
      else
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

      if (iEndPos == _proteinInfo.iProteinSeqLength-1)
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[1] = '-';
      else
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];

// FIX:  store szProteinName to set and add protein idx here
      strcpy(pQuery->_pDecoys[siLowestDecoySpScoreIndex].szProtein, szProteinName);

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].iSeqFilePosition = _proteinInfo. iSeqFilePosition;

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         if (!bFoundVariableMod)   // Normal peptide in variable mod search.
         {
            memset(pQuery->_pDecoys[siLowestDecoySpScoreIndex].pcVarModSites, 0, _iSizepcVarModSites);
         }
         else
         {
            memcpy(pQuery->_pDecoys[siLowestDecoySpScoreIndex].pcVarModSites, pcVarModSites, _iSizepcVarModSites);

            int iVal;
            for (i=0; i<iLenPeptide; i++)
            {
               iVal = pQuery->_pDecoys[siLowestDecoySpScoreIndex].pcVarModSites[i];

               if (iVal > 0)
               {
                  pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites[i]
                     = g_staticParams.variableModParameters.varModList[iVal-1].dVarModMass;
               }
               else if (iVal < 0)
               {
                  pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites[i] = dbe->vectorPeffMod.at(-iVal-1).dMassDiffMono;
               }
               else
               {
                  pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites[i] = 0.0;
               }
            }
         }
      }

      // Get new lowest score.
      pQuery->fLowestDecoyCorrScore = pQuery->_pDecoys[0].fXcorr;
      siLowestDecoySpScoreIndex=0;

      for (i=1; i<g_staticParams.options.iNumStored; i++)
      {
         if (pQuery->_pDecoys[i].fXcorr < pQuery->fLowestDecoyCorrScore)
         {
            pQuery->fLowestDecoyCorrScore = pQuery->_pDecoys[i].fXcorr;
            siLowestDecoySpScoreIndex = i;
         }
      }

      pQuery->siLowestDecoySpScoreIndex = siLowestDecoySpScoreIndex;
   }
   else
   {
      short siLowestSpScoreIndex;

      siLowestSpScoreIndex = pQuery->siLowestSpScoreIndex;

      pQuery->iMatchPeptideCount++;
      pQuery->_pResults[siLowestSpScoreIndex].iLenPeptide = iLenPeptide;

      memcpy(pQuery->_pResults[siLowestSpScoreIndex].szPeptide, szProteinSeq+iStartPos, iLenPeptide);
      pQuery->_pResults[siLowestSpScoreIndex].szPeptide[iLenPeptide]='\0';

      pQuery->_pResults[siLowestSpScoreIndex].dPepMass = dCalcPepMass;

      if (pQuery->_spectrumInfoInternal.iChargeState > 2)
      {
         pQuery->_pResults[siLowestSpScoreIndex].iTotalIons
            = (iLenPeptide-1)*(pQuery->_spectrumInfoInternal.iChargeState-1)
               * g_staticParams.ionInformation.iNumIonSeriesUsed;
      }
      else
      {
         pQuery->_pResults[siLowestSpScoreIndex].iTotalIons
            = (iLenPeptide-1)*g_staticParams.ionInformation.iNumIonSeriesUsed;
      }

      pQuery->_pResults[siLowestSpScoreIndex].fXcorr = (float)dXcorr;

      pQuery->_pResults[siLowestSpScoreIndex].iDuplicateCount = 0;

      if (iStartPos == 0)
         pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0] = '-';
      else
         pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

      if (iEndPos == _proteinInfo.iProteinSeqLength-1)
         pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1] = '-';
      else
         pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];

// FIX:  store szProteinName to set and add number here
      strcpy(pQuery->_pResults[siLowestSpScoreIndex].szProtein, szProteinName);

      pQuery->_pResults[siLowestSpScoreIndex].iSeqFilePosition = _proteinInfo.iSeqFilePosition;

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         if (!bFoundVariableMod)  // Normal peptide in variable mod search.
         {
            memset(pQuery->_pResults[siLowestSpScoreIndex].pcVarModSites, 0, _iSizepcVarModSites);
         }
         else
         {
            memcpy(pQuery->_pResults[siLowestSpScoreIndex].pcVarModSites, pcVarModSites, _iSizepcVarModSites);

            int iVal;
            for (i=0; i<iLenPeptide; i++)
            {
               iVal = pQuery->_pResults[siLowestSpScoreIndex].pcVarModSites[i];

               if (iVal > 0)
               {
                  pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites[i]
                     = g_staticParams.variableModParameters.varModList[iVal-1].dVarModMass;
               }
               else if (iVal < 0)
               {
                  pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites[i] = dbe->vectorPeffMod.at(-iVal-1).dMassDiffMono;
               }
               else
               {
                  pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites[i] = 0.0;
               }
            }
         }
      }

      // Get new lowest score.
      pQuery->fLowestCorrScore = pQuery->_pResults[0].fXcorr;
      siLowestSpScoreIndex=0;

      for (i=1; i<g_staticParams.options.iNumStored; i++)
      {
         if (pQuery->_pResults[i].fXcorr < pQuery->fLowestCorrScore)
         {
            pQuery->fLowestCorrScore = pQuery->_pResults[i].fXcorr;
            siLowestSpScoreIndex = i;
         }
      }

      pQuery->siLowestSpScoreIndex = siLowestSpScoreIndex;
   }
}


int CometSearch::CheckDuplicate(int iWhichQuery,
                                int iStartPos,
                                int iEndPos,
                                bool bFoundVariableMod,
                                double dCalcPepMass,
                                char *szProteinSeq,
                                char *szProteinName,
                                bool bDecoyResults,
                                char *pcVarModSites)
{
   int i,
       iLenMinus1,
       bIsDuplicate=0;
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   iLenMinus1 = iEndPos-iStartPos+1;

   if (bDecoyResults)
   {
      for (i=0; i<g_staticParams.options.iNumStored; i++)
      {
         // Quick check of peptide sequence length first.
         if (iLenMinus1 == pQuery->_pDecoys[i].iLenPeptide
               && isEqual(dCalcPepMass, pQuery->_pDecoys[i].dPepMass))
         {
            if (pQuery->_pDecoys[i].szPeptide[0] == szProteinSeq[iStartPos])
            {
               if (!memcmp(pQuery->_pDecoys[i].szPeptide, szProteinSeq + iStartPos,
                        pQuery->_pDecoys[i].iLenPeptide))
               {
                  bIsDuplicate=1;
               }
            }

            // If bIsDuplicate & variable mod search, check modification sites to see if peptide already stored.
            if (bIsDuplicate && g_staticParams.variableModParameters.bVarModSearch && bFoundVariableMod)
            {
               if (!memcmp(pcVarModSites, pQuery->_pDecoys[i].pcVarModSites, pQuery->_pDecoys[i].iLenPeptide + 2))
               {
                  bIsDuplicate=1;
               }
               else
               {
                  bIsDuplicate=0;
               }
            }

            if (bIsDuplicate)
            {
               // if duplicate, check to see if need to replace stored protein info
               // with protein that's earlier in database
               if (pQuery->_pDecoys[i].iSeqFilePosition > _proteinInfo.iSeqFilePosition)
               {
                  pQuery->_pDecoys[i].iSeqFilePosition = _proteinInfo.iSeqFilePosition;
                  strcpy(pQuery->_pDecoys[i].szProtein, szProteinName);  // szProteinName already has decoy prefix

                  if (iStartPos == 0)
                     pQuery->_pDecoys[i].szPrevNextAA[0] = '-';
                  else
                     pQuery->_pDecoys[i].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

                  if (iEndPos == _proteinInfo.iProteinSeqLength-1)
                     pQuery->_pDecoys[i].szPrevNextAA[1] = '-';
                  else
                     pQuery->_pDecoys[i].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];
               }

// FIX:  ignore if statement above, add _proteinInfo.szProteinName to protein set, and append protein idx here

               pQuery->_pDecoys[i].iDuplicateCount++;
               break;
            }
         }
      }
   }
   else
   {
      for (i=0; i<g_staticParams.options.iNumStored; i++)
      {
         // Quick check of peptide sequence length.
         if (iLenMinus1 == pQuery->_pResults[i].iLenPeptide
               && isEqual(dCalcPepMass, pQuery->_pResults[i].dPepMass))
         {
            if (pQuery->_pResults[i].szPeptide[0] == szProteinSeq[iStartPos])
            {
               if (!memcmp(pQuery->_pResults[i].szPeptide, szProteinSeq + iStartPos,
                        pQuery->_pResults[i].iLenPeptide))
               {
                  bIsDuplicate=1;
               }
            }

            // If bIsDuplicate & variable mod search, check modification sites to see if peptide already stored.
            if (bIsDuplicate && g_staticParams.variableModParameters.bVarModSearch && bFoundVariableMod)
            {
               if (!memcmp(pcVarModSites, pQuery->_pResults[i].pcVarModSites, pQuery->_pResults[i].iLenPeptide + 2))
               {
                  bIsDuplicate=1;
               }
               else
               {
                  bIsDuplicate=0;
               }
            }

            if (bIsDuplicate)
            {
               // if duplicate, check to see if need to replace stored protein info
               // with protein that's earlier in database
               if (pQuery->_pResults[i].iSeqFilePosition > _proteinInfo.iSeqFilePosition)
               {
                  pQuery->_pResults[i].iSeqFilePosition = _proteinInfo.iSeqFilePosition;

                  strcpy(pQuery->_pResults[i].szProtein, szProteinName);

                  if (iStartPos == 0)
                     pQuery->_pResults[i].szPrevNextAA[0] = '-';
                  else
                     pQuery->_pResults[i].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

                  if (iEndPos == _proteinInfo.iProteinSeqLength-1)
                     pQuery->_pResults[i].szPrevNextAA[1] = '-';
                  else
                     pQuery->_pResults[i].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];
               }

// FIX:  ignore if statement above, add _proteinInfo.szProteinName to protein set, and append protein idx here

               pQuery->_pResults[i].iDuplicateCount++;
               break;
            }
         }
      }
   }

   return (bIsDuplicate);
}


void CometSearch::SubtractVarMods(int *piVarModCounts,
                                  int cResidue,
                                  int iResiduePosition)
{
   int i;
   for (i=0; i<VMODS; i++)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, cResidue))
      {
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
         {
            piVarModCounts[i]--;
         }
         else
         {
            if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0)      // protein N
            {
               if (iResiduePosition <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                  piVarModCounts[i]--;
            }
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
            {
               if (iResiduePosition + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
                  piVarModCounts[i]--;
            }
            // Do we just let possible mod residue simply drop off here and
            // deal with peptide distance constraint later??  I think so.
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2) // peptide N
            {
               piVarModCounts[i]--;
            }
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3) // peptide C
            {
               piVarModCounts[i]--;
            }
         }
      }
   }
}


// track # of variable mod AA residues in peptide; note that n- and c-term mods are not tracked here
void CometSearch::CountVarMods(int *piVarModCounts,
                               int cResidue,
                               int iResiduePosition)
{
   int i;
   for (i=0; i<VMODS; i++)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, cResidue))
      {
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
         {
            piVarModCounts[i]++;
         }
         else
         {
            if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0)      // protein N
            {
               if (iResiduePosition <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                  piVarModCounts[i]++;
            }
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
            {
              if (iResiduePosition + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
                  piVarModCounts[i]++;
            }
            // deal with peptide terminal distance constraint elsewhere
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2) // peptide N
            {
               piVarModCounts[i]++;
            }
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3) // peptide C
            {
               piVarModCounts[i]++;
            }
         }
      }
   }
}


// return true if there are any possible variable mods (including PEFF mods)
bool CometSearch::HasVariableMod(int *pVarModCounts,
                                 int iStartPos,
                                 int iEndPos,
                                 struct sDBEntry *dbe)
{
   int i;

   // first check # of residues that could be modified
   for (i=0; i<VMODS; i++)
   {
      if (pVarModCounts[i] > 0)
         return true;
   }

   // next check n- and c-terminal residues
   for (i=0; i<VMODS; i++)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0))
      {
         // if there's no distance contraint and an n- or c-term mod is specified
         // then return true because every peptide will have an n- or c-term
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
         {
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                  || strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c'))
            {
               // there's a mod on either termini that can appear anywhere in sequence
               return true;
            }
         }
         else
         {
            // if n-term distance constraint is specified, make sure first residue for n-term
            // mod or last residue for c-term mod are within distance constraint
            if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0)       // protein N
            {
               // a distance contraint limiting terminal mod to n-terminus
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                     && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
               {
                  return true;
               }
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                     && iEndPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
               {
                  return true;
               }
            }
            // if c-cterm distance constraint specified, must make sure terminal mods are
            // at the end within the distance constraint
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1)  // protein C
            {
               // a distance contraint limiting terminal mod to c-terminus
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                     && iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
               {
                  return true;
               }
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                     && iEndPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
               {
                  return true;
               }
            }
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2)  // peptide N
            {
               // if distance contraint is from peptide n-term and n-term mod is specified
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n'))
               {
                  return true;
               }
               // if distance constraint is from peptide n-term, make sure c-term is within that distance from the n-term
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                     && iEndPos - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
               {
                  return true;
               }
            }
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3)  // peptide C
            {
               // if distance contraint is from peptide c-term and c-term mod is specified
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c'))
               {
                  return true;
               }
               // if distance constraint is from peptide c-term, make sure n-term is within that distance from the c-term
               if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                     && iEndPos - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
               {
                  return true;
               }

            }
         }
      }
   }

   // lastly check if any PEFF mod is present
   if ((int)dbe->vectorPeffMod.size() > 0)
   {
      // Check if there's a PEFF modification within iStartPos and iEndPos
      // Theoretically should check for modifications beyond iEndPos if
      // negative mods are used but will ignore that case until someone complains.
      for (i=0; i<(int)dbe->vectorPeffMod.size(); i++)
      {
         if (dbe->vectorPeffMod.at(i).iPosition >= iStartPos && dbe->vectorPeffMod.at(i).iPosition <=iEndPos)
            return true;
         if (dbe->vectorPeffMod.at(i).iPosition > iEndPos)
            break;
      }
   }

   return false;
}


bool CometSearch::VariableModSearch(char *szProteinSeq,
                                    char *szProteinName,
                                    int piVarModCounts[],
                                    int iStartPos,
                                    int iEndPos,
                                    bool *pbDuplFragment,
                                    struct sDBEntry *dbe)
{
   int i,
       ii,
       i1,
       i2,
       i3,
       i4,
       i5,
       i6,
       i7,
       i8,
       i9,
       piVarModCountsNC[VMODS],   // add n- and c-term mods to the counts here
       numVarModCounts[VMODS];
   double dTmpMass;

   int piTmpTotVarModCt[VMODS];
   int piTmpTotBinaryModCt[VMODS];

   bool bPeffMod = false;

   vector<PeffPositionStruct> vPeffArray;

   if ((int)dbe->vectorPeffMod.size() > 0)
   {
      bool bMatch;

      // vPeffArray will reduce peff mods by position on peptide

      // Check if there's a PEFF modification within iStartPos and iEndPos
      // Theoretically should check for modifications beyond iEndPos if
      // negative mods are used but will ignore that case until someone complains.
      for (i=0; i<(int)dbe->vectorPeffMod.size(); i++)
      {
         if (dbe->vectorPeffMod.at(i).iPosition >= iStartPos && dbe->vectorPeffMod.at(i).iPosition <=iEndPos)
         {
            bPeffMod = true;

            // add initial entry
            if (vPeffArray.size() == 0)
            {
               // add vector struct PeffPositionStruct
               struct PeffPositionStruct pTmp;

               pTmp.iPosition = dbe->vectorPeffMod.at(i).iPosition;
               pTmp.vectorWhichPeff.push_back(i);
               pTmp.vectorMassDiffAvg.push_back(dbe->vectorPeffMod.at(i).dMassDiffAvg);
               pTmp.vectorMassDiffMono.push_back(dbe->vectorPeffMod.at(i).dMassDiffMono);

               vPeffArray.push_back(pTmp);
            }
            else
            {
               // check if iPosition already stored; if so then add masses; if not then add new entry
               bMatch = false;

               for (ii=0; ii< (int)vPeffArray.size(); ii++)
               {
                  // check if iPosition is duplicate; if so just add new mass at that position
                  if (vPeffArray.at(ii).iPosition == dbe->vectorPeffMod.at(i).iPosition)
                  {
                     vPeffArray.at(ii).vectorWhichPeff.push_back(i);
                     vPeffArray.at(ii).vectorMassDiffAvg.push_back(dbe->vectorPeffMod.at(i).dMassDiffAvg);
                     vPeffArray.at(ii).vectorMassDiffMono.push_back(dbe->vectorPeffMod.at(i).dMassDiffMono);

                     bMatch = true;
                     break;
                  }
               }

               if (!bMatch)
               {
                  // add vector struct PeffPositionStruct
                  struct PeffPositionStruct pTmp;

                  pTmp.iPosition = dbe->vectorPeffMod.at(i).iPosition;
                  pTmp.vectorWhichPeff.push_back(i);
                  pTmp.vectorMassDiffAvg.push_back(dbe->vectorPeffMod.at(i).dMassDiffAvg);
                  pTmp.vectorMassDiffMono.push_back(dbe->vectorPeffMod.at(i).dMassDiffMono);

                  vPeffArray.push_back(pTmp);
               }
            }
         }
         if (dbe->vectorPeffMod.at(i).iPosition > iEndPos)
            break;
      }
   }

   strcpy(_proteinInfo.szProteinName, szProteinName);

   // consider possible n- and c-term mods; c-term position is not necessarily iEndPos
   // so need to add some buffer there
   for (i=0; i<VMODS; i++)
   {
      piTmpTotVarModCt[i] = piTmpTotBinaryModCt[i] = 0; // useless but supresses gcc 'may be used uninitialized in this function' warnings

      piVarModCountsNC[i] = piVarModCounts[i];

      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0))
      {
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
         {
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n'))
               piVarModCountsNC[i] += 1;
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c'))
               piVarModCountsNC[i] += 1;
         }
         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0)  // protein N
         {
            // a distance contraint limiting terminal mod to protein N-terminus
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                  && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
            {
               piVarModCountsNC[i] += 1;
            }
            // Since don't know if iEndPos is last residue in peptide (not necessarily),
            // have to be conservative here and count possible c-term mods if within iStartPos+3
            // Honestly not sure why I chose iStartPos+3 here.
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                  && iStartPos+3 <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
            {
               piVarModCountsNC[i] += 1;
            }
         }
         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1)  // protein C
         {
            // a distance contraint limiting terminal mod to protein C-terminus
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                  && iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
            {
               piVarModCountsNC[i] += 1;
            }
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                  && iEndPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
            {
               piVarModCountsNC[i] += 1;
            }
         }
         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2)  // peptide N
         {
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n'))
            {
               piVarModCountsNC[i] += 1;
            }
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                  && iEndPos - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
            {
               piVarModCountsNC[i] += 1;
            }
         }
         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3)  // peptide C
         {
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                  && iEndPos - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
            {
               piVarModCountsNC[i] += 1;
            }
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c'))
            {
               piVarModCountsNC[i] += 1;
            }
         }
      }
   }

   for (i=0; i<VMODS; i++)
   {
      numVarModCounts[i] = piVarModCountsNC[i] > g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod
         ? g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod : piVarModCountsNC[i];
   }

   dTmpMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm;

   if (iStartPos == 0)
      dTmpMass += g_staticParams.staticModifications.dAddNterminusProtein;

   for (i9=0; i9<=numVarModCounts[VMOD_9_INDEX]; i9++)
   {
      if (i9 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
         break;

      for (i8=0; i8<=numVarModCounts[VMOD_8_INDEX]; i8++)
      {
         int iSum8 = i9 + i8;

         if (iSum8 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
            break;

         for (i7=0; i7<=numVarModCounts[VMOD_7_INDEX]; i7++)
         {
            int iSum7 = iSum8 + i7;

            if (iSum7 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
               break;

            for (i6=0; i6<=numVarModCounts[VMOD_6_INDEX]; i6++)
            {
               int iSum6 = iSum7 + i6;

               if (iSum6 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
                  break;

               for (i5=0; i5<=numVarModCounts[VMOD_5_INDEX]; i5++)
               {
                  int iSum5 = iSum6 + i5;

                  if (iSum5 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
                     break;

                  for (i4=0; i4<=numVarModCounts[VMOD_4_INDEX]; i4++)
                  {
                     int iSum4 = iSum5 + i4;

                     if (iSum4 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
                        break;

                     for (i3=0; i3<=numVarModCounts[VMOD_3_INDEX]; i3++)
                     {
                        int iSum3 = iSum4 + i3;

                        if (iSum3 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
                           break;

                        for (i2=0; i2<=numVarModCounts[VMOD_2_INDEX]; i2++)
                        {
                           int iSum2 = iSum3 + i2;

                           if (iSum2 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
                              break;

                           for (i1=0; i1<=numVarModCounts[VMOD_1_INDEX]; i1++)
                           {
                              int iSum1 = iSum2 + i1;

                              if (iSum1 > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
                                 break;

                              int piTmpVarModCounts[] = {i1, i2, i3, i4, i5, i6, i7, i8, i9};

                              if (i1>0 || i2>0 || i3>0 || i4>0 || i5>0 || i6>0 || i7>0 || i8>0 || i9>0 || bPeffMod)
                              {
                                 double dCalcPepMass;
                                 int iTmpEnd;
                                 int iStartTmp = iStartPos+1;
                                 char cResidue;
 
                                 dCalcPepMass = dTmpMass + TotalVarModMass(piTmpVarModCounts);

                                 for (i=0; i<VMODS; i++)
                                 {
                                    // this variable tracks how many of each variable mod is in the peptide
                                    _varModInfo.varModStatList[i].iTotVarModCt = 0;
                                    _varModInfo.varModStatList[i].iTotBinaryModCt = 0;
                                 }

                                 // The start of the peptide is established; need to evaluate
                                 // where the end of the peptide is.
                                 for (iTmpEnd=iStartPos; iTmpEnd<=iEndPos; iTmpEnd++)
                                 {
                                    if (iTmpEnd-iStartTmp < MAX_PEPTIDE_LEN)
                                    {
                                       cResidue = szProteinSeq[iTmpEnd];

                                       dCalcPepMass += g_staticParams.massUtility.pdAAMassParent[(int)cResidue];

                                       for (i=0; i<VMODS; i++)
                                       {
                                          if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0))
                                          {

                                             // look at residues first
                                             if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, cResidue))
                                             {
                                                if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
                                                   _varModInfo.varModStatList[i].iTotVarModCt++;

                                                else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0) // protein N
                                                {
                                                   if (iTmpEnd <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                      _varModInfo.varModStatList[i].iTotVarModCt++;
                                                }
                                                else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
                                                {
                                                   if (iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                         >= _proteinInfo.iProteinSeqLength-1)
                                                   {
                                                      _varModInfo.varModStatList[i].iTotVarModCt++;
                                                   }
                                                }
                                                else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2) // peptide N
                                                {
                                                   if (iTmpEnd - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                      _varModInfo.varModStatList[i].iTotVarModCt++;
                                                }

                                                // analyse peptide C term mod later as iTmpEnd is variable

                                             }

                                             // consider n-term mods only for start residue
                                             if (iTmpEnd == iStartPos)
                                             {

// FIX:  without knowing iTmpEnd, how to consider 'n' mod with c-term peptide distance constraint??

                                                if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                                                      && ((g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
                                                         || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2)
                                                         || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0
                                                            && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                         || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1
                                                               &&  iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                               >= _proteinInfo.iProteinSeqLength-1)))
                                                {
                                                   _varModInfo.varModStatList[i].iTotVarModCt++;
                                                }
                                             }
                                          }
                                       }

                                       if (g_staticParams.variableModParameters.bBinaryModSearch)
                                       {
                                          // make iTotBinaryModCt similar to iTotVarModCt but count the
                                          // number of mod sites in peptide for that particular binary
                                          // mod group and store in first group entry
                                          for (i=0; i<VMODS; i++)
                                          {
                                             bool bMatched=false;

                                             if (g_staticParams.variableModParameters.varModList[i].iBinaryMod
                                                   && !isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
                                                   && !bMatched)
                                             {
                                                int ii;

                                                if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, cResidue))
                                                {
                                                   if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
                                                   {
                                                      _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                      bMatched = true;
                                                   }
                                                   else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0) // protein N
                                                   {
                                                      if (iTmpEnd <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                      {
                                                         _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                         bMatched = true;
                                                      }
                                                   }
                                                   else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
                                                   {
                                                      if (iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                            >= _proteinInfo.iProteinSeqLength-1)
                                                      {
                                                         _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                         bMatched = true;
                                                      }
                                                   }
                                                   else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2) // peptide N
                                                   {
                                                      if (iTmpEnd - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                      {
                                                         _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                         bMatched = true;
                                                      }
                                                   }

                                                   // analyse peptide C term mod later as iTmpEnd is variable

                                                }

                                                // if we didn't increment iTotBinaryModCt for base mod in group
                                                if (!bMatched)
                                                {
                                                   for (ii=i+1; ii<VMODS; ii++)
                                                   {
                                                      if (!isEqual(g_staticParams.variableModParameters.varModList[ii].dVarModMass, 0.0)
                                                            && (g_staticParams.variableModParameters.varModList[ii].iBinaryMod
                                                               == g_staticParams.variableModParameters.varModList[i].iBinaryMod)
                                                            && strchr(g_staticParams.variableModParameters.varModList[ii].szVarModChar, cResidue))
                                                      {
                                                         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
                                                         {
                                                            _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                            bMatched=true;
                                                         }
                                                         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0) // protein N
                                                         {
                                                            if (iTmpEnd <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            {
                                                               _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                               bMatched=true;
                                                            }
                                                         }
                                                         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
                                                         {
                                                            if (iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                                     >= _proteinInfo.iProteinSeqLength-1)
                                                            {
                                                                  _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                               bMatched=true;
                                                            }
                                                         }
                                                         else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2) // peptide N
                                                         {
                                                            if (iTmpEnd - iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            {
                                                               _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                               bMatched=true;
                                                            }
                                                         }
                                                      }

                                                      if (bMatched)
                                                         break;
                                                   }
                                                }

                                                // consider n-term mods only for start residue
                                                if (iTmpEnd == iStartPos)
                                                {
                                                   if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
                                                         && strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                                                         && ((g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0
                                                               && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1
                                                                  &&  iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                                  >= _proteinInfo.iProteinSeqLength-1)))
                                                   {
                                                      _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                      bMatched=true;
                                                   }

                                                   if (!bMatched)
                                                   {
                                                      for (ii=i+1; ii<VMODS; ii++)
                                                      {
                                                         if (!isEqual(g_staticParams.variableModParameters.varModList[ii].dVarModMass, 0.0)
                                                               && (g_staticParams.variableModParameters.varModList[ii].iBinaryMod
                                                                  == g_staticParams.variableModParameters.varModList[i].iBinaryMod)
                                                               && strchr(g_staticParams.variableModParameters.varModList[ii].szVarModChar, 'n'))
                                                         {
                                                            _varModInfo.varModStatList[i].iTotBinaryModCt++;
                                                            bMatched=true;
                                                         }

                                                         if (bMatched)
                                                            break;
                                                      }
                                                   }
                                                }
                                             }
                                          }
                                       }

                                       bool bValid = true;

                                       // since we're varying iEndPos, check enzyme consistency first
                                       if (!CheckEnzymeTermini(szProteinSeq, iStartPos, iTmpEnd))
                                          bValid = false;

                                       if (bValid)
                                       {
                                          // at this point, consider variable c-term mod at iTmpEnd position
                                          for (i=0; i<VMODS; i++)
                                          {
                                             // Store current number of iTotVarModCt because we're going to possibly
                                             // increment it for variable c-term mod.  But as we continue to extend iEndPos,
                                             // we need to temporarily save this value here and restore it later.
                                             piTmpTotVarModCt[i] = _varModInfo.varModStatList[i].iTotVarModCt;
                                             piTmpTotBinaryModCt[i] = _varModInfo.varModStatList[i].iTotBinaryModCt;

                                             // Add in possible c-term variable mods
                                             if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0))
                                             {
                                                if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c'))
                                                {
                                                   // valid if no distance contraint or if defined constraint is on peptide c-terminus
                                                   if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1
                                                         || g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3)
                                                   {
                                                      _varModInfo.varModStatList[i].iTotVarModCt++;
                                                   }
                                                   else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
                                                   {
                                                      if (iTmpEnd + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                            >= _proteinInfo.iProteinSeqLength-1)
                                                      {
                                                         _varModInfo.varModStatList[i].iTotVarModCt++;
                                                      }
                                                   }
                                                }
                                             }
                                          }

                                          // also need to consider all residue mods that have a c-term distance
                                          // constraint because these depend on iTmpEnd which was not defined until now
                                          int x;
                                          for (x=iStartPos; x<=iTmpEnd; x++)
                                          {
                                             cResidue = szProteinSeq[x];

                                             for (i=0; i<VMODS; i++)
                                             {
                                                if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0))
                                                {
                                                   if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, cResidue))
                                                   {
                                                      if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3)  //c-term pep
                                                      {
                                                         if (iTmpEnd - x <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            _varModInfo.varModStatList[i].iTotVarModCt++;
                                                      }
                                                      else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1)  //c-term prot
                                                      {
                                                         if (iTmpEnd + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                            >= _proteinInfo.iProteinSeqLength-1)
                                                         {
                                                            _varModInfo.varModStatList[i].iTotVarModCt++;
                                                         }
                                                      }
                                                   }
                                                }
                                             }
                                          }
                                       }

                                       if (bValid && !g_staticParams.variableModParameters.bBinaryModSearch)
                                       {

                                          // Check to make sure # required mod are actually present in
                                          // current peptide since the end position is variable.
                                          for (i=0; i<VMODS; i++)
                                          {
                                             // varModStatList[i].iTotVarModCt contains # of mod residues in current
                                             // peptide defined by iTmpEnd.  Since piTmpVarModCounts contains # of
                                             // each variable mod to match peptide mass, need to make sure that
                                             // piTmpVarModCounts is not greater than varModStatList[i].iTotVarModCt.

                                             // if number of expected modifications is greater than # of modifiable residues
                                             // within start/end then not possible
                                             if (piTmpVarModCounts[i] > _varModInfo.varModStatList[i].iTotVarModCt)
                                             {
                                                bValid = false;
                                                break;
                                             }
                                          }
                                       }

                                       if (bValid && g_staticParams.variableModParameters.bBinaryModSearch)
                                       {
                                          int ii;
                                          bool bUsed[VMODS];

                                          for (ii=0; ii<VMODS; ii++)
                                             bUsed[ii] = false;

                                          // walk through all list of mods, find those with the same iBinaryMod value,
                                          // and make sure all mods are accounted for
                                          for (i=0; i<VMODS; i++)
                                          {
                                             // check for binary mods; since multiple sets of binary mods can be
                                             // specified with logical OR, need to compare the sets
                                             int iSumTmpVarModCounts=0;

                                             if (!bUsed[i] && g_staticParams.variableModParameters.varModList[i].iBinaryMod)
                                             {
                                                iSumTmpVarModCounts += piTmpVarModCounts[i];

                                                bUsed[i]=true;

                                                for (ii=i+1; ii<VMODS; ii++)
                                                {
                                                   if ((g_staticParams.variableModParameters.varModList[ii].iBinaryMod
                                                            == g_staticParams.variableModParameters.varModList[i].iBinaryMod))
                                                   {
                                                      bUsed[ii]=true;
                                                      iSumTmpVarModCounts += piTmpVarModCounts[ii];
                                                   }
                                                }

                                                // the set sum counts must match total # of mods in peptide
                                                if (iSumTmpVarModCounts != 0
                                                      && iSumTmpVarModCounts != _varModInfo.varModStatList[i].iTotBinaryModCt)
                                                {
                                                   bValid = false;
                                                   break;
                                                }
                                             }

                                             if (piTmpVarModCounts[i] > _varModInfo.varModStatList[i].iTotVarModCt)
                                             {
                                                bValid = false;
                                                break;
                                             }
                                          }
                                       }

                                       if (bValid && g_staticParams.variableModParameters.bRequireVarMod)
                                       {
                                          // Check to see if required mods are satisfied; here, we're just making
                                          // sure the number of possible modified residues for each mod is non-zero
                                          // so don't worry about distance constraint issues yet.
                                          for (i=0; i<VMODS; i++)
                                          {
                                             if (g_staticParams.variableModParameters.varModList[i].bRequireThisMod
                                                   && piTmpVarModCounts[i] == 0)
                                             {
                                                bValid = false;
                                                break;
                                             }
                                          }
                                       }

                                       if (bValid && HasVariableMod(piTmpVarModCounts, iStartPos, iTmpEnd, dbe))   //FIX:  iTmpEnd here vs. iEndPos before??
                                       {
                                          // mass including terminal mods that need to be tracked separately here
                                          // because we are considering multiple terminating positions in peptide
                                          double dTmpCalcPepMass;

                                          dTmpCalcPepMass = dCalcPepMass;

                                          // static protein terminal mod
                                          if (iTmpEnd == _proteinInfo.iProteinSeqLength-1)
                                             dTmpCalcPepMass += g_staticParams.staticModifications.dAddCterminusProtein;

                                          int iWhichQuery = WithinMassTolerance(dTmpCalcPepMass, szProteinSeq, iStartPos, iTmpEnd);
                                          bool bDoPeffAnalysis = false;

                                          // Need to see if peptide + PEFF mod is within mass tolerance of any query.
                                          if (bPeffMod)
                                          {
                                             bool bPeff = false;

                                             // Only need to return true/false here to know whether or not to permute
                                             // through PEFF mods later.  So as long as just 1 combination of PEFF
                                             // mods work, that's great.

                                             // First see if PEFF mods are within iStartPos and iTmpEnd
                                             for (i=0; i<(int)dbe->vectorPeffMod.size(); i++)
                                             {
                                                if (dbe->vectorPeffMod.at(i).iPosition >= iStartPos && dbe->vectorPeffMod.at(i).iPosition <=iTmpEnd)
                                                {
                                                   bPeff = true;
                                                   break;
                                                }
                                             }

                                             if (bPeff)
                                                bDoPeffAnalysis = WithinMassTolerancePeff(dTmpCalcPepMass, &vPeffArray);
                                          }

                                          if (iWhichQuery != -1 || bDoPeffAnalysis)
                                          {
                                             // We know that mass is within some query's tolerance range so
                                             // now need to permute variable mods and at each permutation calculate
                                             // fragment ions once and loop through all matching spectra to score.
                                             for (i=0; i<VMODS; i++)
                                             {
                                                if (g_staticParams.variableModParameters.varModList[i].dVarModMass > 0.0  && piTmpVarModCounts[i] > 0)
                                                {
                                                   memset(_varModInfo.varModStatList[i].iVarModSites, 0, sizeof(_varModInfo.varModStatList[i].iVarModSites));
                                                }

                                                _varModInfo.varModStatList[i].iMatchVarModCt = piTmpVarModCounts[i];
                                             }

                                             _varModInfo.iStartPos = iStartPos;
                                             _varModInfo.iEndPos = iTmpEnd;

                                             _varModInfo.dCalcPepMass = dCalcPepMass;

                                             // iTmpEnd-iStartPos+3 = length of peptide +2 (for n/c-term)
                                             if (!PermuteMods(szProteinSeq, iWhichQuery, 1, pbDuplFragment, &bDoPeffAnalysis, &vPeffArray, dbe))
                                             {
                                                return false;
                                             }
                                          }
                                       }

                                       if (bValid)
                                       {
                                          for (i=0; i<VMODS; i++)
                                          {
                                             _varModInfo.varModStatList[i].iTotVarModCt = piTmpTotVarModCt[i];
                                             _varModInfo.varModStatList[i].iTotBinaryModCt = piTmpTotBinaryModCt[i];
                                          }
                                       }

                                    }
                                 } // loop through iStartPos to iEndPos
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   if ((int)dbe->vectorPeffMod.size() > 0)
      vPeffArray.clear();

   return true;
}


double CometSearch::TotalVarModMass(int *pVarModCounts)
{
   double dTotVarModMass = 0;

   int i;
   for (i=0; i<VMODS; i++)
      dTotVarModMass += g_staticParams.variableModParameters.varModList[i].dVarModMass * pVarModCounts[i];

   return dTotVarModMass;
}


bool CometSearch::PermuteMods(char *szProteinSeq,
                              int iWhichQuery,
                              int iWhichMod,
                              bool *pbDuplFragment,
                              bool *bDoPeffAnalysis,
                              vector <PeffPositionStruct>* vPeffArray,
                              struct sDBEntry *dbe)
{
   int iModIndex;

   switch (iWhichMod)
   {
      case 1:
         iModIndex = VMOD_1_INDEX;
         break;
      case 2:
         iModIndex = VMOD_2_INDEX;
         break;
      case 3:
         iModIndex = VMOD_3_INDEX;
         break;
      case 4:
         iModIndex = VMOD_4_INDEX;
         break;
      case 5:
         iModIndex = VMOD_5_INDEX;
         break;
      case 6:
         iModIndex = VMOD_6_INDEX;
         break;
      case 7:
         iModIndex = VMOD_7_INDEX;
         break;
      case 8:
         iModIndex = VMOD_8_INDEX;
         break;
      case 9:
         iModIndex = VMOD_9_INDEX;
         break;
      default:
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - in CometSearch::PermuteMods, iWhichIndex=%d (valid range 1 to 9)\n", iWhichMod);
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
   }

   if (_varModInfo.varModStatList[iModIndex].iMatchVarModCt > 0)
   {
      int b[MAX_PEPTIDE_LEN_P2];
      int p[MAX_PEPTIDE_LEN_P2 + 2];  // p array needs to be 2 larger than b

      int i, x, y, z;

      int N = _varModInfo.varModStatList[iModIndex].iTotVarModCt;
      int M = _varModInfo.varModStatList[iModIndex].iMatchVarModCt;

      inittwiddle(M, N, p);

      for (i=0; i != N-M; i++)
      {
         _varModInfo.varModStatList[iModIndex].iVarModSites[i] = 0;
         b[i] = 0;
      }

      while (i != N)
      {
         _varModInfo.varModStatList[iModIndex].iVarModSites[i] = iWhichMod;
         b[i] = 1;
         i++;
      }

      if (iWhichMod == 9)
      {
         if (!MergeVarMods(szProteinSeq, iWhichQuery, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe))
            return false;
      }
      else
      {
         if (!PermuteMods(szProteinSeq, iWhichQuery, iWhichMod+1, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe))
            return false;
      }

      while (!twiddle(&x, &y, &z, p))
      {
         b[x] = 1;
         b[y] = 0;

         for (i=0; i != N; i++)
            _varModInfo.varModStatList[iModIndex].iVarModSites[i] = (b[i] ? iWhichMod : 0);

         if (iWhichMod == 9)
         {
            if (!MergeVarMods(szProteinSeq, iWhichQuery, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe))
               return false;
         }
         else
         {
            if (!PermuteMods(szProteinSeq, iWhichQuery, iWhichMod+1, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe))
               return false;
         }
      }
   }
   else
   {
      if (iWhichMod == 9)
      {
         if (!MergeVarMods(szProteinSeq, iWhichQuery, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe))
            return false;
      }
      else
      {
         if (!PermuteMods(szProteinSeq, iWhichQuery, iWhichMod+1, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe))
            return false;
      }
   }

   return true;
}


/*
  twiddle.c - generate all combinations of M elements drawn without replacement
  from a set of N elements.  This routine may be used in two ways:
  (0) To generate all combinations of M out of N objects, let a[0..N-1]
      contain the objects, and let c[0..M-1] initially be the combination
      a[N-M..N-1].  While twiddle(&x, &y, &z, p) is false, set c[z] = a[x] to
      produce a new combination.
  (1) To generate all sequences of 0's and 1's containing M 1's, let
      b[0..N-M-1] = 0 and b[N-M..N-1] = 1.  While twiddle(&x, &y, &z, p) is
      false, set b[x] = 1 and b[y] = 0 to produce a new sequence.

  In either of these cases, the array p[0..N+1] should be initialised as
  follows:
    p[0] = N+1
    p[1..N-M] = 0
    p[N-M+1..N] = 1..M
    p[N+1] = -2
    if M=0 then p[1] = 1

  In this implementation, this initialisation is accomplished by calling
  inittwiddle(M, N, p), where p points to an array of N+2 ints.

  Coded by Matthew Belmonte <mkb4@Cornell.edu>, 23 March 1996.  This
  implementation Copyright (c) 1996 by Matthew Belmonte.  Permission for use and
  distribution is hereby granted, subject to the restrictions that this
  copyright notice and reference list be included in its entirety, and that any
  and all changes made to the program be clearly noted in the program text.

  This software is provided 'as is', with no warranty, express or implied,
  including but not limited to warranties of merchantability or fitness for a
  particular purpose.  The user of this software assumes liability for any and
  all damages, whether direct or consequential, arising from its use.  The
  author of this implementation will not be liable for any such damages.

  Reference:

  Phillip J Chase, `Algorithm 382: Combinations of M out of N Objects [G6]',
  Communications of the Association for Computing Machinery 13:6:368 (1970).

  The returned indices x, y, and z in this implementation are decremented by 1,
  in order to conform to the C language array reference convention.  Also, the
  parameter 'done' has been replaced with a Boolean return value.
*/

int CometSearch::twiddle(int *x, int *y, int *z, int *p)
{
   register int i, j, k;
   j = 1;

   while (p[j] <= 0)
      j++;

   if (p[j - 1] == 0)
   {
      for (i=j-1; i != 1; i--)
         p[i] = -1;
      p[j] = 0;
      *x = *z = 0;
      p[1] = 1;
      *y = j - 1;
   }
   else
   {
      if (j > 1)
         p[j - 1] = 0;
      do
         j++;

      while (p[j] > 0);

      k = j - 1;
      i = j;

      while (p[i] == 0)
         p[i++] = -1;

      if (p[i] == -1)
      {
         p[i] = p[k];
         *z = p[k] - 1;
         *x = i - 1;
         *y = k - 1;
         p[k] = -1;
      }
      else
      {
         if (i == p[0])
            return (1);
         else
         {
            p[j] = p[i];
            *z = p[i] - 1;
            p[i] = 0;
            *x = j - 1;
            *y = i - 1;
         }
      }
   }
   return (0);
}


void CometSearch::inittwiddle(int m, int n, int *p)
{
   int i;

   p[0] = n + 1;

   for (i=1; i != n-m+1; i++)
      p[i] = 0;

   while (i != n+1)
   {
      p[i] = i + m - n;
      i++;
   }

   p[n + 1] = -2;

   if (m == 0)
      p[1] = 1;
}


// FIX: 'false' is never returned by this function, why?
bool CometSearch::MergeVarMods(char *szProteinSeq,
                               int iWhichQuery,
                               bool *pbDuplFragment,
                               bool *bDoPeffAnalysis,
                               vector <PeffPositionStruct>* vPeffArray,
                               struct sDBEntry *dbe)
{
   char pcVarModSites[MAX_PEPTIDE_LEN_P2];
   int piVarModCharIdx[VMODS];
   int i;
   int j;

   // at this point, need to compare current modified peptide
   // against all relevant entries

   // but first, calculate modified peptide mass as it could've changed
   // by terminating earlier than start/end positions defined in VariableModSearch()
   double dCalcPepMass = g_staticParams.precalcMasses.dNtermProton + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS;

   int iLenMinus1 = _varModInfo.iEndPos - _varModInfo.iStartPos;     // equals iLenPeptide-1
   int iLenPeptide = iLenMinus1+1;

   // contains positional coding of a variable mod at each index which equals an AA residue
   memset(pcVarModSites, 0, _iSizepcVarModSites);
   memset(piVarModCharIdx, 0, sizeof(piVarModCharIdx));

   // deal with n-term mod
   for (j=0; j<VMODS; j++)
   {
      if ( strchr(g_staticParams.variableModParameters.varModList[j].szVarModChar, 'n')
            && !isEqual(g_staticParams.variableModParameters.varModList[j].dVarModMass, 0.0)
            && (_varModInfo.varModStatList[j].iMatchVarModCt > 0) )
      {
         if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
         {
            if (pcVarModSites[iLenPeptide] != 0)  // conflict in two variable mods on n-term
               return false;

            // store the modification number at modification position
            pcVarModSites[iLenPeptide] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
            dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;;
         }
         piVarModCharIdx[j]++;
      }
   }

   // deal with c-term mod
   for (j=0; j<VMODS; j++)
   {
      if ( strchr(g_staticParams.variableModParameters.varModList[j].szVarModChar, 'c')
            && !isEqual(g_staticParams.variableModParameters.varModList[j].dVarModMass, 0.0)
            && (_varModInfo.varModStatList[j].iMatchVarModCt > 0) )
      {
         if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
         {
            if (pcVarModSites[iLenPeptide+1] != 0)  // conflict in two variable mods on c-term
               return false;

            // store the modification number at modification position
            pcVarModSites[iLenPeptide+1] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
            dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
         }
         piVarModCharIdx[j]++;
      }
   }

   // Generate pdAAforward for _pResults[0].szPeptide
   for (i=_varModInfo.iStartPos; i<=_varModInfo.iEndPos; i++)
   {
      int iPos = i - _varModInfo.iStartPos;

      dCalcPepMass += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[i]];

      // This loop is where all individual variable mods are combined
      for (j=0; j<VMODS; j++)
      {
         if (!isEqual(g_staticParams.variableModParameters.varModList[j].dVarModMass, 0.0)
               && (_varModInfo.varModStatList[j].iMatchVarModCt > 0)
               && strchr(g_staticParams.variableModParameters.varModList[j].szVarModChar, szProteinSeq[i]))
         {
            if (g_staticParams.variableModParameters.varModList[j].iVarModTermDistance == -1)
            {
               if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
               {
                  if (pcVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                     return false;

                  // store the modification number at modification position
                  pcVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                  dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
               }
               piVarModCharIdx[j]++;
            }
            else  // terminal distance constraint specified
            {
               if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 0)      // protein N
               {
                  if (i <= g_staticParams.variableModParameters.varModList[j].iVarModTermDistance)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (pcVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return false;

                        // store the modification number at modification position
                        pcVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j]++;
                  }
               }
               else if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 1) // protein C
               {
                  if (i + g_staticParams.variableModParameters.varModList[j].iVarModTermDistance >= _proteinInfo.iProteinSeqLength-1)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (pcVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return false;

                        // store the modification number at modification position
                        pcVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j]++;
                  }
               }
               else if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 2) // peptide N
               {
                  if (iPos <= g_staticParams.variableModParameters.varModList[j].iVarModTermDistance)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (pcVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return false;

                        // store the modification number at modification position
                        pcVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j]++;
                  }
               }
               else if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 3) // peptide C
               {
                  if (iPos + g_staticParams.variableModParameters.varModList[j].iVarModTermDistance >= iLenMinus1)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (pcVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return false;

                        // store the modification number at modification position
                        pcVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j]++;
                  }
               }
            }
         }
      }
   }

   // Check to see if required mods are satisfied
   if (g_staticParams.variableModParameters.bRequireVarMod)
   {
      for (j=0; j<VMODS; j++)
      {
         if (g_staticParams.variableModParameters.varModList[j].bRequireThisMod
               && !isEqual(g_staticParams.variableModParameters.varModList[j].dVarModMass, 0.0))
         {
            bool bPresent = false;

            // if mod is required, see if it is present in the peptide
            for (i=_varModInfo.iStartPos; i<=_varModInfo.iEndPos; i++)
            {
               int iPos = i - _varModInfo.iStartPos;

//FIX:  add in logic to check distance constraints
               if (pcVarModSites[iPos] == _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
               {
                  bPresent = true;
                  break;
               }
            }

            if (!bPresent)
               return false;
         }
      }
   }

   // Now that normal variable mods are taken care of, add in PEFF mods if pertinent
   if (*bDoPeffAnalysis)
   {
      // permute through PEFF

      int n = (int)(*vPeffArray).size();  // number of residues with a PEFF mod
   
      int a[n],
          len[n],
          j;
   
      for (i = 0 ; i < n ; i++)
      {
         // number of mods at each residue position
         len[i] = (int)(*vPeffArray).at(i).vectorWhichPeff.size();
   
         a[i]=0;
      }
   
      double dMassAddition;
      bool bFirst=true;
      while(1)
      {
         if (!bFirst) // skip first iteration of this where there are no mods
         {
            dMassAddition = 0.0;
            memset(pcVarModSites, 0, _iSizepcVarModSites);

            // Now have permutations of PEFF. First, add PEFF masses and see if within mass tolerance
            for (i=0; i<n; i++)
            {
               if (a[i]>0)
                  dMassAddition += (*vPeffArray).at(i).vectorMassDiffMono.at(a[i]-1);
            }
   
            // if dCalcPepMass + dMassAddition is within mass tol, add these mods

            // Validate that total mass is within tolerance of some query entry
            double dTmpCalcPepMass = dCalcPepMass + dMassAddition;

            // With PEFF mods added in, find if new mass is within any query tolerance
            iWhichQuery = WithinMassTolerance(dTmpCalcPepMass, szProteinSeq, _varModInfo.iStartPos, _varModInfo.iEndPos);

            bool bValidPeffPosition = true;
            if (iWhichQuery != -1)
            {
               for (int ii=_varModInfo.iStartPos; ii<=_varModInfo.iEndPos; ii++)
               {
                  // See if there's a PEFF mod at this 'i' sequence location based on current permutation of PEFFs
                  for (i=0; i<n; i++)
                  {
                     // Check if sequence position matches PEFF position inside this if statement
                     if (a[i]>0)
                     {
                        if ((*vPeffArray).at(i).iPosition > _varModInfo.iEndPos)
                        {
                           bValidPeffPosition = false;
                           break;
                        }
                        else if (ii == (*vPeffArray).at(i).iPosition)
                        {
                           // make sure PEFF mod location doesn't conflict with existing variable mod
                           if (pcVarModSites[ii - _varModInfo.iStartPos] == 0)
                           {
                              // PEFF mods are encoded as negative values to reference appropriate PeffModStruct entry
                              // Sadly needs to be offset by -1 because first PEFF index is 0
                              pcVarModSites[ii - _varModInfo.iStartPos] = -1 -(*vPeffArray).at(i).vectorWhichPeff.at(a[i]-1); // use negative values for PEFF mods
                           }
                           else
                           {
                              bValidPeffPosition = false;
                              break;
                           }
                        }
                     }
                  }
               }

               if  (bValidPeffPosition)
               {
                  // Need to check if mass is OK
         
                  // Do a binary search on list of input queries to find matching mass.
                  iWhichQuery = BinarySearchMass(0, g_pvQuery.size(), dTmpCalcPepMass);

                  // Seek back to first peptide entry that matches mass tolerance in case binary
                  // search doesn't hit the first entry.
                  while (iWhichQuery>0 && g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassTolerancePlus >= dCalcPepMass)
                     iWhichQuery--;

                  // Only if this PEFF mod (plus possible variable mods) is within mass tolerance, continue
                  if (iWhichQuery != -1)
                  {
                     // FIX: add test here as pcVarModSites must contain a negative PEFF value
                     CalcVarModIons(szProteinSeq, iWhichQuery, pbDuplFragment, pcVarModSites, dTmpCalcPepMass, iLenPeptide, dbe);
                  }
               }
            }
            //else move onto next permutation of PEFF mods
   
         }
         else
            bFirst=false;
   
         for (j=n-1; j>=0; j--)
         {
            if(++a[j]<=len[j])
               break;
            else
               a[j]=0;
         }
   
         if(j<0)
            break;
      }    
   }
   else
   {
      // FIX: add test here as pcVarModSites should not contain any negative PEFF values
      CalcVarModIons(szProteinSeq, iWhichQuery, pbDuplFragment, pcVarModSites, dCalcPepMass, iLenPeptide,dbe);
   }

   return true;
}


bool CometSearch::CalcVarModIons(char *szProteinSeq,
                                 int iWhichQuery,
                                 bool *pbDuplFragment,
                                 char *pcVarModSites,
                                 double dCalcPepMass,
                                 int iLenPeptide,
                                 struct sDBEntry *dbe)
{
   char pcVarModSitesDecoy[MAX_PEPTIDE_LEN_P2];
   char szDecoyPeptide[MAX_PEPTIDE_LEN_P2];  // allow for prev/next AA in string
   char szDecoyProteinName[WIDTH_REFERENCE];
   int ctIonSeries;
   int ctLen;
   int ctCharge;
   int iWhichIonSeries;
   int i;
   int iLenMinus1 = iLenPeptide - 1;

   bool bFirstTimeThroughLoopForPeptide = true;

   // Compare calculated fragment ions against all matching query spectra

   while (iWhichQuery < (int)g_pvQuery.size())
   {
      if (dCalcPepMass < g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassToleranceMinus)
      {
         // if calculated mass is smaller than low mass range, it
         // means we reached candidate peptides that are too big
         break;
      }

      // check mass of peptide again; required for terminal mods that may or may not get applied??
      if (CheckMassMatch(iWhichQuery, dCalcPepMass))
      {

         // Calculate ion series just once to compare against all relevant query spectra
         if (bFirstTimeThroughLoopForPeptide)
         {
            bFirstTimeThroughLoopForPeptide = false;

            double dBion = g_staticParams.precalcMasses.dNtermProton;
            double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

            if (_varModInfo.iStartPos == 0)
               dBion += g_staticParams.staticModifications.dAddNterminusProtein;
            if (_varModInfo.iEndPos == _proteinInfo.iProteinSeqLength-1)
               dYion += g_staticParams.staticModifications.dAddCterminusProtein;

            // variable N-term
            if (pcVarModSites[iLenPeptide] > 0)
               dBion += g_staticParams.variableModParameters.varModList[pcVarModSites[iLenPeptide]-1].dVarModMass;

            // variable C-term
            if (pcVarModSites[iLenPeptide + 1] > 0)
               dYion += g_staticParams.variableModParameters.varModList[pcVarModSites[iLenPeptide+1]-1].dVarModMass;

            // Generate pdAAforward for _pResults[0].szPeptide
            for (i=_varModInfo.iStartPos; i<_varModInfo.iEndPos; i++)
            {
               int iPos = i - _varModInfo.iStartPos;

               dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[i]];

               if (pcVarModSites[iPos] > 0)
                  dBion += g_staticParams.variableModParameters.varModList[pcVarModSites[iPos]-1].dVarModMass;
               else if (pcVarModSites[iPos] < 0)
                  dBion += (dbe->vectorPeffMod.at(-pcVarModSites[iPos]-1)).dMassDiffMono;

               _pdAAforward[iPos] = dBion;

               dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[_varModInfo.iEndPos - i +_varModInfo.iStartPos]];

               iPos = _varModInfo.iEndPos - i;
               if (pcVarModSites[iPos] > 0)
                  dYion += g_staticParams.variableModParameters.varModList[pcVarModSites[iPos]-1].dVarModMass;
               else if (pcVarModSites[iPos] < 0)
                  dYion += (dbe->vectorPeffMod.at(-pcVarModSites[iPos]-1)).dMassDiffMono;

               _pdAAreverse[i - _varModInfo.iStartPos] = dYion;
            }

            // now get the set of binned fragment ions once for all matching peptides

            // initialize pbDuplFragment here
            for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
            {
               for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
               {
                  iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                  for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                     pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforward, _pdAAreverse))] = false;
               }
            }

            // set pbDuplFragment[bin] to true for each fragment ion bin
            for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
            {
               for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
               {
                  iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                  // as both _pdAAforward and _pdAAreverse are increasing, loop through
                  // iLenPeptide-1 to complete set of internal fragment ions
                  for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                  {
                     int iVal = BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforward, _pdAAreverse));

                     if (pbDuplFragment[iVal] == false)
                     {
                        _uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen] = iVal;
                        pbDuplFragment[iVal] = true;
                     }
                     else
                        _uiBinnedIonMasses[ctCharge][ctIonSeries][ctLen] = 0;
                  }
               }
            }

            // Also take care of decoy here
            if (g_staticParams.options.iDecoySearch)
            {
               char pcTmpVarModSearchSites[MAX_PEPTIDE_LEN_P2];  // placeholder to reverse variable mods

#ifdef _WIN32
               _snprintf(szDecoyProteinName, WIDTH_REFERENCE, "%s%s", g_staticParams.szDecoyPrefix, _proteinInfo.szProteinName);
               szDecoyProteinName[WIDTH_REFERENCE-1]=0;    // _snprintf does not guarantee null termination
#else
               snprintf(szDecoyProteinName, WIDTH_REFERENCE, "%s%s", g_staticParams.szDecoyPrefix, _proteinInfo.szProteinName);
#endif
               // Generate reverse peptide.  Keep prev and next AA in szDecoyPeptide string.
               // So actual reverse peptide starts at position 1 and ends at len-2 (as len-1
               // is next AA).

               // Store flanking residues from original sequence.
               if (_varModInfo.iStartPos==0)
                  szDecoyPeptide[0]='-';
               else
                  szDecoyPeptide[0]=szProteinSeq[_varModInfo.iStartPos-1];

               if (_varModInfo.iEndPos == _proteinInfo.iProteinSeqLength-1)
                  szDecoyPeptide[iLenPeptide+1]='-';
               else
                  szDecoyPeptide[iLenPeptide+1]=szProteinSeq[_varModInfo.iEndPos+1];

               szDecoyPeptide[iLenPeptide+2]='\0';

               // Now reverse the peptide and reverse the variable mod locations too
               if (g_staticParams.enzymeInformation.iSearchEnzymeOffSet==1)
               {
                  // last residue stays the same:  change ABCDEK to EDCBAK

                  for (i=_varModInfo.iEndPos-1; i>=_varModInfo.iStartPos; i--)
                  {
                     szDecoyPeptide[_varModInfo.iEndPos-i] = szProteinSeq[i];
                     pcTmpVarModSearchSites[_varModInfo.iEndPos-i-1] = pcVarModSites[i- _varModInfo.iStartPos];
                  }

                  szDecoyPeptide[_varModInfo.iEndPos - _varModInfo.iStartPos+1]=szProteinSeq[_varModInfo.iEndPos];  // last residue stays same
                  pcTmpVarModSearchSites[iLenPeptide-1] = pcVarModSites[iLenPeptide-1];
               }
               else
               {
                  // first residue stays the same:  change ABCDEK to AKEDCB

                  for (i=_varModInfo.iEndPos; i>=_varModInfo.iStartPos+1; i--)
                  {
                     szDecoyPeptide[_varModInfo.iEndPos-i+2] = szProteinSeq[i];
                     pcTmpVarModSearchSites[_varModInfo.iEndPos-i+1] = pcVarModSites[i- _varModInfo.iStartPos];
                  }

                  szDecoyPeptide[1]=szProteinSeq[_varModInfo.iStartPos];  // first residue stays same
                  pcTmpVarModSearchSites[0] = pcVarModSites[0];
               }

               pcTmpVarModSearchSites[iLenPeptide]   = pcVarModSites[iLenPeptide];    // N-term
               pcTmpVarModSearchSites[iLenPeptide+1] = pcVarModSites[iLenPeptide+1];  // C-term
               memcpy(pcVarModSitesDecoy, pcTmpVarModSearchSites, sizeof(char)*iLenPeptide+2);

               // Now need to recalculate _pdAAforward and _pdAAreverse for decoy entry
               double dBion = g_staticParams.precalcMasses.dNtermProton;
               double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

               if (_varModInfo.iStartPos == 0)
                  dBion += g_staticParams.staticModifications.dAddNterminusProtein;
               if (_varModInfo.iEndPos == _proteinInfo.iProteinSeqLength-1)
                  dYion += g_staticParams.staticModifications.dAddCterminusProtein;

               // variable N-term
               if (pcVarModSitesDecoy[iLenPeptide] > 0)
                  dBion += g_staticParams.variableModParameters.varModList[pcVarModSitesDecoy[iLenPeptide]-1].dVarModMass;

               // variable C-term
               if (pcVarModSitesDecoy[iLenPeptide + 1] > 0)
                  dYion += g_staticParams.variableModParameters.varModList[pcVarModSitesDecoy[iLenPeptide+1]-1].dVarModMass;

               int iDecoyStartPos = 1;  // This is start/end for newly created decoy peptide
               int iDecoyEndPos = strlen(szDecoyPeptide)-2;

               int iTmp1;
               int iTmp2;

               // Generate pdAAforward for szDecoyPeptide
               for (i=iDecoyStartPos; i<iDecoyEndPos; i++)
               {
                  iTmp1 = i-iDecoyStartPos;
                  iTmp2 = iDecoyEndPos - iTmp1;

                  dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szDecoyPeptide[i]];
                  if (pcVarModSitesDecoy[iTmp1] > 0)
                     dBion += g_staticParams.variableModParameters.varModList[pcVarModSitesDecoy[iTmp1]-1].dVarModMass;
                  else if (pcVarModSitesDecoy[iTmp1] < 0)
                     dBion += (dbe->vectorPeffMod.at(-pcVarModSites[iTmp1]-1)).dMassDiffMono;

                  dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szDecoyPeptide[iTmp2]];
//FIX: check if pcVarModSites index is correct
                  if (pcVarModSitesDecoy[iTmp2-iDecoyStartPos] > 0)
                     dYion += g_staticParams.variableModParameters.varModList[pcVarModSitesDecoy[iTmp2-iDecoyStartPos]-1].dVarModMass;
                  else if (pcVarModSitesDecoy[iTmp2-iDecoyStartPos] < 0)
                     dYion += (dbe->vectorPeffMod.at(-pcVarModSites[iTmp2-iDecoyStartPos]-1)).dMassDiffMono;

                  _pdAAforwardDecoy[iTmp1] = dBion;
                  _pdAAreverseDecoy[iTmp1] = dYion;
               }

               // now get the set of binned fragment ions once for all matching decoy peptides

               // initialize pbDuplFragment here
               for (ctCharge = 1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
               {
                  for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
                  {
                     iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                     for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                     {
                        pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforwardDecoy, _pdAAreverseDecoy))] = false;
                     }
                  }
               }

               for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
               {
                  for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
                  {
                     iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                     // as both _pdAAforward and _pdAAreverse are increasing, loop through
                     // iLenPeptide-1 to complete set of internal fragment ions
                     for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                     {
                        int iVal = BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforwardDecoy, _pdAAreverseDecoy));

                        if (pbDuplFragment[iVal] == false)
                        {
                           _uiBinnedIonMassesDecoy[ctCharge][ctIonSeries][ctLen] = iVal;
                           pbDuplFragment[iVal] = true;
                        }
                        else
                           _uiBinnedIonMassesDecoy[ctCharge][ctIonSeries][ctLen] = 0;
                     }
                  }
               }
            }
         }

         XcorrScore(szProteinSeq, _proteinInfo.szProteinName, _varModInfo.iStartPos, _varModInfo.iEndPos,
               true, dCalcPepMass, false, iWhichQuery, iLenPeptide, pcVarModSites, dbe);

         if (g_staticParams.options.iDecoySearch)
         {
            XcorrScore(szDecoyPeptide, szDecoyProteinName, 1, iLenPeptide, true, dCalcPepMass,
                  true, iWhichQuery, iLenPeptide, pcVarModSitesDecoy, dbe);
         }
      }

      iWhichQuery++;
   }

   return true;
}
