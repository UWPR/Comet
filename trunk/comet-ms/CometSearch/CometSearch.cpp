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
#include "CometPostAnalysis.h"
#include "CometMassSpecUtils.h"

#include <stdio.h>
#include <sstream>

bool *CometSearch::_pbSearchMemoryPool;
bool **CometSearch::_ppbDuplFragmentArr;

CometSearch::CometSearch()
{
   // Initialize the header modification string - won't change.

   // Allocate memory for protein sequence if necessary.

   _iSizepiVarModSites = sizeof(int)*MAX_PEPTIDE_LEN_P2;
   _iSizepdVarModSites = sizeof(double)*MAX_PEPTIDE_LEN_P2;
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
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to mitigate memory use.\n");
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


// See if any peptides in indexed database matches dMZ/iPrecursorCharge.
// Return iPrecursorMatch=1 if there's a match, iPrecursorMatch=0 for no match
void CometSearch::CheckIdxPrecursorMatch(int iPrecursorCharge,
                                         double dMZ,
                                         int* iPrecursorMatch)
{
   double dMH;

   Query *pScoring = new Query();


   dMH = (dMZ * iPrecursorCharge) - (iPrecursorCharge - 1)*PROTON_MASS;

   pScoring->_pepMassInfo.dExpPepMass = dMH;
   pScoring->_spectrumInfoInternal.iChargeState = iPrecursorCharge;
   pScoring->iSpScoreData = 0;
   pScoring->iFastXcorrData = 0;
   pScoring->iFastXcorrDataNL = 0;
   g_staticParams.options.iDecoySearch = 0;

   if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
   {
      pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
      {
         pScoring->_pepMassInfo.dPeptideMassTolerance *= pScoring->_spectrumInfoInternal.iChargeState;
      }
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
   {
      pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance * 0.001;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
      {
         pScoring->_pepMassInfo.dPeptideMassTolerance *= pScoring->_spectrumInfoInternal.iChargeState;
      }
   }
   else // ppm
   {
      pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance
         * pScoring->_pepMassInfo.dExpPepMass / 1000000.0;
   }

   if (g_staticParams.tolerances.iIsotopeError == 0)
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 1) // search 0, +1 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - C13_DIFF * PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 2) // search 0, +1, +2 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 2.0 * C13_DIFF * PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 3) // search 0, +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0 * C13_DIFF * PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 4) // search -8, -4, 0, 4, 8 windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 8.1;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance + 8.1;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 5) // search -1, 0, +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0 * C13_DIFF * PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance + 1.0 * C13_DIFF * PROTON_MASS;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 6) // search -3, -2, -1, 0, +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0 * C13_DIFF * PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance + 3.0 * C13_DIFF * PROTON_MASS;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 7) // search -1, 0, +1 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - C13_DIFF * PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance + C13_DIFF * PROTON_MASS;
   }
   else  // Should not get here.
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg, " Error - iIsotopeError=%d\n", g_staticParams.tolerances.iIsotopeError);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return;
   }
   
   pScoring->_pResults = new Results[0];

   g_pvQuery.push_back(pScoring);

   g_massRange.dMinMass = pScoring->_pepMassInfo.dPeptideMassToleranceMinus;
   g_massRange.dMaxMass = pScoring->_pepMassInfo.dPeptideMassTolerancePlus;

   *iPrecursorMatch = 0;

   CometSearch sqSearch;
   sqSearch.IndexSearch(iPrecursorMatch);

   // Deleting each Query object in the vector calls its destructor, which
   // frees the spectral memory (see definition for Query in CometData.h)
   for (std::vector<Query*>::iterator it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
      delete *it;

   g_pvQuery.clear();
}

bool CometSearch::RunSearch(int minNumThreads,
                            int maxNumThreads,
                            int iPercentStart,
                            int iPercentEnd)
{
   bool bSucceeded = true;

   if (g_staticParams.bIndexDb)
   {
      int iPrecursorMatch = -1;
      CometSearch sqSearch;
      sqSearch.IndexSearch(&iPrecursorMatch);
   }
   else
   {
      sDBEntry dbe;
      FILE *fp;
      int iTmpCh = 0;
      comet_fileoffset_t lEndPos = 0;
      comet_fileoffset_t lCurrPos = 0;
      bool bTrimDescr = false;
      string strPeffHeader;
      char *szMods = 0;             // will store ModResPsi (or ModResUnimod) and VariantSimple text for parsing for all entries; resize as needed
      char *szPeffLine = 0;         // store description line starting with first \ to parse above
      comet_fileoffset_t iLenAllocMods = 0;
      int iLenSzLine = 0;
      comet_fileoffset_t iLen = 0;

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

      if ((fp=fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
      {
         char szErrorMsg[1024];
         sprintf(szErrorMsg, " Error - cannot read database file \"%s\".\n", g_staticParams.databaseInfo.szDatabase);
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }

      fseek(fp, 0, SEEK_END);
      lEndPos=ftell(fp);
      rewind(fp);

      // Load database entry header.
      lCurrPos = ftell(fp);
      iTmpCh = getc(fp);

      if (g_staticParams.peffInfo.iPeffSearch)
      {
         iLenSzLine = 2048;
         szPeffLine = (char*)malloc( iLenSzLine* sizeof(char));
         if (szPeffLine == NULL)
         {
            char szErrorMsg[256];
            sprintf(szErrorMsg, " Error - malloc szPeffLine\n");
            string strErrorMsg(szErrorMsg);
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(szErrorMsg);
            return false;
         }

         // allocate initial storage for mod strings that will be parsed from each def line
         iLenAllocMods = 5000;
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
            sprintf(szErrorMsg,  " Error: \"peff_format\" is specified but \"peff_obo\" is not set\n");
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

      if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.options.bCreateIndex)
      {
         logout("     - Search progress: ");
         fflush(stdout);
      }

      char szBuf[8192];
      char szPeffAttributeMod[16];                                // from ModRes
      char szPeffAttributeVariant[16];
      char szPeffAttributeProcessed[16];

      if (g_staticParams.peffInfo.iPeffSearch == 1 || g_staticParams.peffInfo.iPeffSearch == 3)
         strcpy(szPeffAttributeMod, "\\ModResPsi=");
      else if (g_staticParams.peffInfo.iPeffSearch == 2 || g_staticParams.peffInfo.iPeffSearch == 4)
         strcpy(szPeffAttributeMod, "\\ModResUnimod=");
      else
         strcpy(szPeffAttributeMod, "");

      if (g_staticParams.peffInfo.iPeffSearch == 1 || g_staticParams.peffInfo.iPeffSearch == 2
            || g_staticParams.peffInfo.iPeffSearch == 5)
         strcpy(szPeffAttributeVariant, "\\VariantSimple=");
      else
         strcpy(szPeffAttributeVariant, "");

      if (g_staticParams.peffInfo.iPeffSearch > 1)
         strcpy(szPeffAttributeProcessed, "\\Processed=");
      else
         strcpy(szPeffAttributeProcessed, "");

      int  iLenAttributeVariant = (int)strlen(szPeffAttributeVariant);
      int  iLenAttributeMod = (int)strlen(szPeffAttributeMod);
      int  iNumBadChars = 0; // count # of bad (non-printing) characters in header 

      bool bHeadOfFasta = true;
      // Loop through entire database.
      while(!feof(fp))
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
         dbe.vectorPeffProcessed.clear();

         if (bHeadOfFasta)
         {
            // skip through whitespace at head of line
            while (isspace(iTmpCh))
               iTmpCh = getc(fp);

            // skip comment lines
            if (iTmpCh == '#')
            {
               // skip to description line
               while ((iTmpCh != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF))
                  iTmpCh = getc(fp);
            }

            bHeadOfFasta = false;
         }

         if (iTmpCh == '>') // Expect a '>' for sequence header line.
         {
            // grab file pointer here for this sequence entry
            // this will be stored for protein references for each matched entry and
            // will be used to retrieve actual protein references when printing output
            dbe.lProteinFilePosition = ftell(fp);

            bTrimDescr = false;
            while (((iTmpCh = getc(fp)) != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF))
            {
               if (!bTrimDescr && iscntrl(iTmpCh))
                  bTrimDescr = true;

               if (!bTrimDescr && dbe.strName.size() < (WIDTH_REFERENCE-1))
               {
                  if (iTmpCh < 32 || iTmpCh>126)  // sanity check for reading binary (index) file
                  {
                     iNumBadChars++;
                     if (iNumBadChars > 20)
                     {
                        logerr(" Too many non-printing characters in database header lines; wrong file type/format?\n");
                        fclose(fp);
                        return false;
                     }
                  }
                  else
                     dbe.strName += iTmpCh;
               }

               // load and parse PEFF header
               if (g_staticParams.peffInfo.iPeffSearch)
               {
                  if (iTmpCh == '\\')
                  {
                     ungetc(iTmpCh, fp);

                     // grab rest of description line here
                     szPeffLine[0]='\0';
                     fgets(szPeffLine, iLenSzLine, fp);
                     while (!feof(fp) && szPeffLine[strlen(szPeffLine)-1]!='\n')
                     {
                        char *pTmp;
                        iLenSzLine += 512;
                        pTmp = (char *)realloc(szPeffLine, iLenSzLine);
                        if (pTmp == NULL)
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  " Error realloc(szPeffLine[%d])\n", iLenSzLine);
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                           fclose(fp);
                           return false;
                        }
                        szPeffLine = pTmp;
                        fgets(szPeffLine+(int)strlen(szPeffLine), iLenSzLine - (int)strlen(szPeffLine), fp);
                     }

                     // grab from \ModResPsi or \ModResUnimod and \VariantSimple to end of line
                     char *pStr;
                     if (iLenAttributeMod>0 && (pStr = strstr(szPeffLine, szPeffAttributeMod)) != NULL)
                     {
                        char *pStr2;
                        pStr += iLenAttributeMod;

                        pStr2 = pStr;

                        // need to find closing parenthesis
                        int iTmp=0;  // count of number of open parenthesis
                        while (1)
                        {
                           if ((iTmp == 0 && *pStr2 == ' ') || *pStr2 == '\r' || *pStr2=='\n')
                              break;
                           else if (*pStr2 == '(')
                              iTmp++;
                           else if (*pStr2 == ')')
                              iTmp--;

                           pStr2++;
                        }

                        iLen = pStr2 - pStr;

                        if ( iLen > iLenAllocMods)
                        {
                           char *pTmp;

                           iLenAllocMods = iLen + 1000;
                           pTmp=(char *)realloc(szMods, iLenAllocMods);
                           if (pTmp == NULL)
                           {
                              char szErrorMsg[512];
#ifdef WIN32
                              sprintf(szErrorMsg,  " Error realloc(szMods[%lld])\n", iLenAllocMods);
#else
                              sprintf(szErrorMsg,  " Error realloc(szMods[%ld])\n", iLenAllocMods);
#endif
                              string strErrorMsg(szErrorMsg);
                              g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                              logerr(szErrorMsg);
                              fclose(fp);
                              return false;
                           }
                           szMods = pTmp;
                        }

                        strncpy(szMods, pStr, iLen);
                        szMods[iLen]='\0';

                        if ( (pStr2 = strrchr(szMods, ')'))!=NULL)
                        {
                           pStr2++;
                           *pStr2 = '\0';
                        }
                        else
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  " Error: PEFF entry '%s' missing mod closing parenthesis\n", dbe.strName.c_str()); 
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                           fclose(fp);
                           return false;
                        }

                        int iPos;
                        string strModID;

                        // now tokenize/split szMods on ')' character
                        string strModRes(szMods);
                        istringstream ssMods(strModRes);
                        while (!ssMods.eof())
                        {
                           string strModEntry;
                           getline(ssMods, strModEntry, ')');

                           iPos = 0;

                           if (strModEntry.length() < 8)   // strModEntry should look like "(1|XXX:1|name"
                              break;

                           // at this point, strModEntry should look like (118,121|MOD:00000 
                           if (strModEntry[0]=='(' && isdigit(strModEntry[1]))  //handle possible '?' in the position field ; need to check that strModEntry looks like "(number"
                           {
                              // turn '|' to space
                              std::string::iterator it;
                              for (it = strModEntry.begin(); it != strModEntry.end(); ++it)
                              {
                                 if (*it == '|' || *it == '(')
                                    *it = ' ';
                              }

                              // split "118,121 MOD:00000" into "118,121" and "MOD:00000"
                              std::stringstream converter(strModEntry);
                              string strPos;
                              string strModID;
                              converter >> strPos >> strModID;

                              // now tokenize on comma separated szPos
                              istringstream ss(strPos);
                              while (!ss.eof())
                              {
                                 string x;               // here's a nice, empty string
                                 getline( ss, x, ',' );  // try to read the next field into it
                                 iPos = atoi(x.c_str());
                                 if (iPos <= 0)
                                 {
                                    if (g_staticParams.options.bVerboseOutput)
                                    {
                                       char szErrorMsg[512];
                                       sprintf(szErrorMsg,  "Warning:  %s, %s=(%d|%s) ignored; modentry: %s\n",
                                             dbe.strName.c_str(), szPeffAttributeMod, iPos, strModID.c_str(), strModEntry.c_str());
                                       string strErrorMsg(szErrorMsg);
                                       g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                                       logerr(szErrorMsg);
                                    }
                                 }
                                 else
                                 {
                                    struct PeffModStruct pData;
                                    CometSearch pOBO;

                                    pData.iPosition = iPos - 1;   // represent PEFF mod position in 0 array index coordinates

                                    // find strModID in vectorPeffOBO and get pData.dMassDiffAvg and pData.MassDiffMono
                                    if (pOBO.MapOBO(strModID, &vectorPeffOBO, &pData))
                                    {
                                       dbe.vectorPeffMod.push_back(pData);
                                    }
                                 }
                              }
                           }
                           else
                           {
                              if (g_staticParams.options.bVerboseOutput)
                              {
                                 char szErrorMsg[512];
                                 sprintf(szErrorMsg,  "Warning:  %s, %s=(%d|%s) ignored; modentry: %s\n",
                                       dbe.strName.c_str(), szPeffAttributeMod, iPos, strModID.c_str(), strModEntry.c_str());
                                 string strErrorMsg(szErrorMsg);
                                 g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                                 logerr(szErrorMsg);
                              }
                           }
                        }
                     }

                     if (iLenAttributeVariant>0 && (pStr = strstr(szPeffLine, szPeffAttributeVariant)) != NULL)
                     {
                        char *pStr2;
                        pStr += iLenAttributeVariant;

                        pStr2 = pStr;

                        // need to find closing parenthesis
                        int iTmp=0;  // count of number of open parenthesis
                        while (1)
                        {
                           if ((iTmp == 0 && *pStr2 == ' ') || *pStr2 == '\r' || *pStr2=='\n')
                              break;
                           else if (*pStr2 == '(')
                              iTmp++;
                           else if (*pStr2 == ')')
                              iTmp--;

                           pStr2++;
                        }

                        iLen = pStr2 - pStr;

                        if ( iLen > iLenAllocMods)
                        {
                           char *pTmp;
                           iLenAllocMods = iLen + 1000;
                           pTmp=(char *)realloc(szMods, iLenAllocMods);
                           if (pTmp == NULL)
                           {
                              char szErrorMsg[512];
#ifdef WIN32
                              sprintf(szErrorMsg,  " Error realloc(szMods[%lld])\n", iLenAllocMods);
#else
                              sprintf(szErrorMsg,  " Error realloc(szMods[%ld])\n", iLenAllocMods);
#endif
                              string strErrorMsg(szErrorMsg);
                              g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                              logerr(szErrorMsg);
                              fclose(fp);
                              return false;
                           }
                           szMods = pTmp;
                        }

                        strncpy(szMods, pStr, iLen);
                        szMods[iLen]='\0';

                        if ( (pStr2 = strrchr(szMods, ')'))!=NULL)
                        {
                           pStr2++;
                           *pStr2 = '\0';
                        }
                        else
                        {
                           char szErrorMsg[512];
                           sprintf(szErrorMsg,  " Error: PEFF entry '%s' missing variant closing parenthesis\n", dbe.strName.c_str()); 
                           string strErrorMsg(szErrorMsg);
                           g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                           logerr(szErrorMsg);
                           fclose(fp);
                           return false;
                        }

                        // parse VariantSimple entries
                        string strMods(szMods);
                        istringstream ssVariants(strMods);
                        string strVariant;
                        char cVariant;
                        int iPos;

                        while (!ssVariants.eof())
                        {
                           string strVariantEntry;
                           getline(ssVariants, strVariantEntry, ')');

                           //handle possible '?' in the position field; need to check that strVariantEntry looks like "(number"
                           if (strVariantEntry[0]=='(' && isdigit(strVariantEntry[1]))
                           {
                              // turn '|' to space
                              std::string::iterator it;
                              for (it = strVariantEntry.begin(); it != strVariantEntry.end(); ++it)
                              {
                                 if (*it == '|' || *it == '(')
                                    *it = ' ';
                              }

                              // split "8 C" into "8" and "C"
                              iPos = -1;
                              std::stringstream converter(strVariantEntry);
                              converter >> iPos >> strVariant;

                              // make sure variant residue is just a single residue in VariantSimple entry
                              cVariant = '\0';
                              if (strVariant.length() == 1)
                                 cVariant = strVariant[0];

                              // sanity check: make sure position is positive and residue is A-Z or *
                              if (iPos<0 || ((cVariant<65 || cVariant>90) && cVariant!=42))  // char can be AA or *
                              {
                                 if (g_staticParams.options.bVerboseOutput)
                                 {
                                    char szErrorMsg[512];
                                    sprintf(szErrorMsg,  "Warning:  %s, VariantSimple=(%d|%c) ignored\n", dbe.strName.c_str(), iPos, cVariant);
                                    string strErrorMsg(szErrorMsg);
                                    g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                                    logerr(szErrorMsg);
                                 }
                              }
                              else
                              {
                                 struct PeffVariantSimpleStruct pData;

                                 pData.iPosition = iPos - 1;   // represent PEFF variant position in 0 array index coordinates
                                 pData.cResidue = cVariant;
                                 dbe.vectorPeffVariantSimple.push_back(pData);
                              }
                           }
                        }
                     }

                     // exit out of this as end of line grabbed
                     break;
                  }
               } // done with PEFF
            }

            if (dbe.strName.length() <= 0)
            {
               char szErrorMsg[512];
               sprintf(szErrorMsg,  "\n Error - zero length sequence description; wrong database file/format?\n");
               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }

            if (g_staticParams.options.bCreateIndex)
            {
               struct IndexProteinStruct sEntry;

               // store protein name
               strcpy(sEntry.szProt, dbe.strName.c_str());
               sEntry.lProteinFilePosition = dbe.lProteinFilePosition;
               g_pvProteinNames.insert({ sEntry.lProteinFilePosition, sEntry });
            }

            // Load sequence
            while (((iTmpCh=getc(fp)) != '>') && (iTmpCh != EOF))
            {
               if ('a'<=iTmpCh && iTmpCh<='z')
               {
                  dbe.strSeq += iTmpCh - 32;  // convert toupper case so subtract 32 (i.e. 'A'-'a')
                  g_staticParams.databaseInfo.uliTotAACount++;
               }
               else if ('A'<=iTmpCh && iTmpCh<='Z')
               {
                  dbe.strSeq += iTmpCh;
                  g_staticParams.databaseInfo.uliTotAACount++;
               }
            }

            g_staticParams.databaseInfo.iTotalNumProteins++;

            if (!g_staticParams.options.bOutputSqtStream && !(g_staticParams.databaseInfo.iTotalNumProteins%500))
            {
               char szTmp[128];
               lCurrPos = ftell(fp);
               if (g_staticParams.options.bCreateIndex)
                  sprintf(szTmp, "%3d%%", (int)(100.0*(0.005 + (double)lCurrPos/(double)lEndPos)));
               else // go from iPercentStart to iPercentEnd, scaled by lCurrPos/iEndPos
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
               break;
         }
         else
         {
            fgets(szBuf, sizeof(szBuf), fp);
            iTmpCh = getc(fp);
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

      fclose(fp);

      if (!g_staticParams.options.bOutputSqtStream)
      {
         char szTmp[128];
         if (g_staticParams.options.bCreateIndex)
            sprintf(szTmp, "100%%\n");
         else
            sprintf(szTmp, "%3d%%\n", iPercentEnd);
         logout(szTmp);
         fflush(stdout);
      }

      if (g_staticParams.peffInfo.iPeffSearch)
      {
         free(szMods);
         free(szPeffLine);
      }
   }

   return bSucceeded;
}


void CometSearch::ReadOBO(char *szOBO,
                          vector<OBOStruct> *vectorPeffOBO)
{
   FILE *fp;

   if ( (fp=fopen(szOBO, "r")) != NULL)
   {
      char szLineOBO[SIZE_BUF];

      // store UniMod mod string "UNIMOD:1" and mass diffs 'delta_mono_mass "42.010565"' 'delta_avge_mass "42.0367"'
      fgets(szLineOBO, SIZE_BUF, fp);
      while (!feof(fp))
      {
         if (!strncmp(szLineOBO, "[Term]", 6))
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

            while (fgets(szLineOBO, SIZE_BUF, fp))
            {
               char szTmp[MAX_PEFFMOD_LEN];

               if (!strncmp(szLineOBO, "[Term]", 6))
               {
                  if (pEntry.dMassDiffMono != 0.0)
                     (*vectorPeffOBO).push_back(pEntry);

                  break;
               }
               else if (!strncmp(szLineOBO, "id: ", 4))
               {
                  sscanf(szLineOBO, "id: %16s", szTmp);
                  pEntry.strMod = string(szTmp);
               }
               else if (!strncmp(szLineOBO, "xref: delta_mono_mass ", 22))
                  sscanf(szLineOBO + 22, "\"%lf\"", &pEntry.dMassDiffMono);
               else if (!strncmp(szLineOBO, "xref: delta_avge_mass ", 22))
                  sscanf(szLineOBO + 22, "\"%lf\"", &pEntry.dMassDiffAvg);
               else if (!strncmp(szLineOBO, "xref: DiffAvg: ", 15))
                  sscanf(szLineOBO + 15, "\"%lf\"", &pEntry.dMassDiffAvg);
               else if (!strncmp(szLineOBO, "xref: DiffMono: ", 16))
                  sscanf(szLineOBO + 16, "\"%lf\"", &pEntry.dMassDiffMono);
            }
         }
         else
         {
            fgets(szLineOBO, SIZE_BUF, fp);
         }
      }

      fclose(fp);
   }
   else
   {
      char szErrorMsg[1024];
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

   iPos=BinarySearchPeffStrMod(0, (int)(*vectorPeffOBO).size(), strMod, *vectorPeffOBO);

   if (iPos != -1 && iPos< (int)(*vectorPeffOBO).size() )
   {
      pData->dMassDiffAvg = (*vectorPeffOBO).at(iPos).dMassDiffAvg;
      pData->dMassDiffMono = (*vectorPeffOBO).at(iPos).dMassDiffMono;

      if (!strMod.compare(0,7, "UNIMOD:"))
         strncpy(pData->szMod, strMod.c_str(), MAX_PEFFMOD_LEN-1);  // UNIMOD:XXXXX
      else if (!strMod.compare(0, 4, "MOD:"))
         strncpy(pData->szMod, strMod.c_str(), MAX_PEFFMOD_LEN-1);  // MOD:XXXXX
      else
         strcpy(pData->szMod, "ERROR");

      pData->szMod[MAX_PEFFMOD_LEN-1]='\0';
      return true;
   }
   else
   {
      if (g_staticParams.options.bVerboseOutput)
      {
         char szErrorMsg[1024];
         sprintf(szErrorMsg,  " Warning: cannot find \"%s\" in OBO\n", strMod.c_str());
         string strErrorMsg(szErrorMsg);
         logerr(szErrorMsg);
      }

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
      _proteinInfo.iProteinSeqLength = _proteinInfo.iTmpProteinSeqLength = (int)dbe.strSeq.size();
      _proteinInfo.lProteinFilePosition = dbe.lProteinFilePosition;
      _proteinInfo.cPeffOrigResidue = '\0';
      _proteinInfo.iPeffOrigResiduePosition = -9;

      // have to pass sequence as it can be modified per below
      if (!SearchForPeptides(dbe, (char *)dbe.strSeq.c_str(), false, pbDuplFragment))
         return false;

      if (g_staticParams.options.bClipNtermMet && dbe.strSeq[0]=='M')
      {
         _proteinInfo.iTmpProteinSeqLength -= 1;   // remove 1 for M, used in checking termini

         if (!SearchForPeptides(dbe, (char *)dbe.strSeq.c_str()+1, true, pbDuplFragment))
            return false;

         _proteinInfo.iTmpProteinSeqLength += 1;
      }

      // Plug in an AA substitution and do a search, requiring that AA be present
      // in peptide or a flanking residue that causes a enzyme cut site
      if (dbe.vectorPeffVariantSimple.size() > 0)
         SearchForVariants(dbe, (char *)dbe.strSeq.c_str(), pbDuplFragment);

      // FIX: how to incorporate Variant search with clipped N-term Met protein??

   }
   else
   {
      int ii;

      _proteinInfo.iProteinSeqLength = _proteinInfo.iTmpProteinSeqLength = (int)dbe.strSeq.size();
      _proteinInfo.lProteinFilePosition = dbe.lProteinFilePosition;
      _proteinInfo.cPeffOrigResidue = '\0';
      _proteinInfo.iPeffOrigResiduePosition = -9;

      // Nucleotide search; translate NA to AA.

      if ((g_staticParams.options.iWhichReadingFrame == 1) ||
          (g_staticParams.options.iWhichReadingFrame == 2) ||
          (g_staticParams.options.iWhichReadingFrame == 3))
      {
         // Specific forward reading frames.
         ii = g_staticParams.options.iWhichReadingFrame - 1;

         // Creates szProteinSeq[] for each reading frame.
         if (!TranslateNA2AA(&ii, 1,(char *)dbe.strSeq.c_str()))
            return false;

         if (!SearchForPeptides(dbe, _proteinInfo.pszProteinSeq, false, pbDuplFragment))
            return false;
      }
      else if ((g_staticParams.options.iWhichReadingFrame == 7) ||
               (g_staticParams.options.iWhichReadingFrame == 9))
      {
         // All 3 forward reading frames.
         for (ii=0; ii<3; ii++)
         {
            if (!TranslateNA2AA(&ii, 1,(char *)dbe.strSeq.c_str()))
               return false;

            if (!SearchForPeptides(dbe, _proteinInfo.pszProteinSeq, false, pbDuplFragment))
               return false;
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
         seqSize = (int)dbe.strSeq.size()+1;
         try
         {
            pszTemp = new char[seqSize];
         }
         catch (std::bad_alloc& ba)
         {
            char szErrorMsg[1024];
            sprintf(szErrorMsg, " Error - new(szTemp[%d]). bad_alloc: %s.\n", seqSize, ba.what());
            sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
            sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to mitigate memory use.\n");
            string strErrorMsg(szErrorMsg);
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(szErrorMsg);
            return false;
         }

         memcpy(pszTemp, (char *)dbe.strSeq.c_str(), seqSize*sizeof(char));
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
                  return false;

               if (!SearchForPeptides(dbe, _proteinInfo.pszProteinSeq, false, pbDuplFragment))
                  return false;
            }
         }
         else
         {
            // Specific reverse reading frame ... valid values are 4, 5 or 6.
            ii = 6 - g_staticParams.options.iWhichReadingFrame;

            if (ii == 0)
               ii = 2;
            else if (ii == 2)
               ii=0;

            if (!TranslateNA2AA(&ii, -1, pszTemp))
               return false;

            if (!SearchForPeptides(dbe, _proteinInfo.pszProteinSeq, false, pbDuplFragment))
               return false;
         }

         delete[] pszTemp;
         pszTemp = NULL;
      }
   }

   return true;
}


// iPrecursorMatch=0 will do a simple mass lookup, called from CheckIdxPrecursorMatch
// iPrecursorMatch=-1 will perform index search
bool CometSearch::IndexSearch(int *iPrecursorMatch)
{
   comet_fileoffset_t lEndOfStruct;
   char szBuf[SIZE_BUF];
   FILE *fp;

   if ((fp = fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
   {
      char szErrorMsg[1024];
      sprintf(szErrorMsg, " Error - cannot read indexed database file \"%s\" %s.\n", g_staticParams.databaseInfo.szDatabase, strerror(errno));
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   // ignore any static masses in params file; only valid ones
   // are those in database index
   memset(g_staticParams.staticModifications.pdStaticMods, 0, sizeof(g_staticParams.staticModifications.pdStaticMods));

   bool bFoundStatic = false;
   bool bFoundVariable = false;

   // read in static and variable mods
   while (fgets(szBuf, sizeof(szBuf), fp))
   {
      if (!strncmp(szBuf, "MassType:", 9))
      {
         sscanf(szBuf, "%d %d", &g_staticParams.massUtility.bMonoMassesParent, &g_staticParams.massUtility.bMonoMassesFragment);
      }

      if (!strncmp(szBuf, "StaticMod:", 10))
      {
         char *tok;
         char delims[] = " ";
         int x=65;

         // FIX:  hack here for setting static mods; need to reset masses ... fix later
         CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassFragment,
            g_staticParams.massUtility.bMonoMassesFragment,
            &g_staticParams.massUtility.dOH2fragment);

         bFoundStatic = true;
         tok=strtok(szBuf+11, delims);
         while (tok != NULL)
         {
            sscanf(tok, "%lf", &(g_staticParams.staticModifications.pdStaticMods[x]));
            g_staticParams.massUtility.pdAAMassFragment[x] += g_staticParams.staticModifications.pdStaticMods[x];
            tok = strtok (NULL, delims);
            x++;
            if (x==95)  // 65-90 stores A-Z then next 4 (ascii 91-94) are n/c term peptide, n/c term protein
               break;
         }

         g_staticParams.staticModifications.dAddNterminusPeptide = g_staticParams.staticModifications.pdStaticMods[91];
         g_staticParams.staticModifications.dAddCterminusPeptide = g_staticParams.staticModifications.pdStaticMods[92];
         g_staticParams.staticModifications.dAddNterminusProtein = g_staticParams.staticModifications.pdStaticMods[93];
         g_staticParams.staticModifications.dAddCterminusProtein = g_staticParams.staticModifications.pdStaticMods[94];

         // have to set these here again once static mods are read
         g_staticParams.precalcMasses.dNtermProton = g_staticParams.staticModifications.dAddNterminusPeptide
            + PROTON_MASS;

         g_staticParams.precalcMasses.dCtermOH2Proton = g_staticParams.staticModifications.dAddCterminusPeptide
            + g_staticParams.massUtility.dOH2fragment
            + PROTON_MASS;

         g_staticParams.precalcMasses.dOH2ProtonCtermNterm = g_staticParams.massUtility.dOH2parent
            + PROTON_MASS
            + g_staticParams.staticModifications.dAddCterminusPeptide
            + g_staticParams.staticModifications.dAddNterminusPeptide;
      }

      if (!strncmp(szBuf, "VariableMod:", 12))
      {
         char *tok;
         char delims[] = " ";
         int x=0;

         bFoundVariable = true;

         tok=strtok(szBuf+13, delims);
         while (tok != NULL)
         {
            tok = strtok (NULL, delims); // skip list of var mod residues
            // for index search, storing variable mods 0-9 in pdStaticMods array 0-9
            sscanf(tok, "%lf", &(g_staticParams.variableModParameters.varModList[x].dVarModMass));

            tok = strtok (NULL, delims);
            x++;
            if (x == VMODS)
               break;
         }
         break;
      }
   }

   if (!(bFoundStatic && bFoundVariable))
   {
      char szErr[256];
      sprintf(szErr, " Error with index database format. Mods not parsed (%d %d).", bFoundStatic, bFoundVariable);
      logerr(szErr);
      fclose(fp);
      return false;
   }

   // indexed searches will always set this to true
   g_staticParams.variableModParameters.bVarModSearch = true;
 
   // read fp of index
   comet_fileoffset_t clTmp;
#ifdef _WIN32
#ifdef _WIN64
   clTmp = sizeof(comet_fileoffset_t);              //win64
#else
   clTmp = (long long)sizeof(comet_fileoffset_t);   //win32
#endif
#else
   clTmp = sizeof(comet_fileoffset_t);              //linux
#endif

   comet_fseek(fp, -clTmp, SEEK_END);
   fread(&lEndOfStruct, sizeof(comet_fileoffset_t), 1, fp);

   // read index
   int iMinMass=0;
   int iMaxMass=0;
   uint64_t tNumPeptides=0;
   comet_fseek(fp, lEndOfStruct, SEEK_SET);
   fread(&iMinMass, sizeof(int), 1, fp);
   fread(&iMaxMass, sizeof(int), 1, fp);
   fread(&tNumPeptides, sizeof(uint64_t), 1, fp);

   // sanity checks
   if (iMinMass < 0 || iMinMass > 20000 || iMaxMass < 0 || iMaxMass > 20000)
   {
      char szErr[256];
      sprintf(szErr, " Error reading .idx database:  min mass %d, max mass %d, num peptides %zu\n", iMinMass, iMaxMass, tNumPeptides);
      logerr(szErr);
      fclose(fp);
      return false;
   }

   comet_fileoffset_t *lReadIndex = new comet_fileoffset_t[iMaxMass+1];
   for (int i=0; i<iMaxMass+1; i++)
      lReadIndex[i] = -1;

   fread(lReadIndex, sizeof(comet_fileoffset_t), iMaxMass+1, fp);

   int iStart = (int)(g_massRange.dMinMass - 0.5);  // smallest mass/index start
   int iEnd = (int)(g_massRange.dMaxMass + 0.5);  // largest mass/index end

   if (iStart > iMaxMass)  // smallest input mass is greater than what's stored in index
   {
      delete[] lReadIndex;
      fclose(fp);
      return true;
   }

   if (iStart < iMinMass)
      iStart = iMinMass;
   if (iEnd > iMaxMass)
      iEnd = iMaxMass;

   struct DBIndex sDBI;
   sDBEntry dbe;

   while (lReadIndex[iStart] == -1 && iStart<iEnd)
      iStart++;
   comet_fseek(fp, lReadIndex[iStart], SEEK_SET);
   fread(&sDBI, sizeof(struct DBIndex), 1, fp);

   // Now read in the protein file positions here; not used yet
   _proteinInfo.lProteinFilePosition = dbe.lProteinFilePosition = comet_ftell(fp);
   long lSize;
   fread(&lSize, sizeof(long), 1, fp);
   for (long x = 0; x < lSize; x++)
   {
      comet_fileoffset_t tmpoffset;
      fread(&tmpoffset, sizeof(comet_fileoffset_t), 1, fp);
   }

   _proteinInfo.cPrevAA = sDBI.szPrevNextAA[0];
   _proteinInfo.cNextAA = sDBI.szPrevNextAA[1];
   dbe.strSeq = sDBI.szPrevNextAA[0] + sDBI.szPeptide + sDBI.szPrevNextAA[1];  // make string including prev/next AA

   while ((int)sDBI.dPepMass <= iEnd)
   {
/*
      printf("OK  index pep ");
      for (unsigned int x=0; x<strlen(sDBI.szPeptide); x++)
      {
         printf("%c", sDBI.szPeptide[x]);
         if (sDBI.pcVarModSites[x] != 0)
            printf("[%0.3f]", g_staticParams.variableModParameters.varModList[sDBI.pcVarModSites[x]-1].dVarModMass);
      }
      printf(", mass %f\n", sDBI.dPepMass); fflush(stdout);
*/
      if (sDBI.dPepMass > g_massRange.dMaxMass)
      {
         if (*iPrecursorMatch == 0)
         {
            delete[] lReadIndex;
            fclose(fp);
            return true;
         }
         else
            break;
      }

      if (*iPrecursorMatch == 0)  //CheckIdxPrecursorMatch
      {
         if (CheckMassMatch(0, sDBI.dPepMass))
         {
            *iPrecursorMatch = 1;   // found mass match index
            delete[] lReadIndex;
            fclose(fp);
            return true;
         }
      }
      else
      {
         int iWhichQuery = BinarySearchMass(0, (int)g_pvQuery.size(), sDBI.dPepMass);

         while (iWhichQuery > 0 && g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassTolerancePlus >= sDBI.dPepMass)
            iWhichQuery--;

         if (iWhichQuery != -1)
            AnalyzeIndexPep(iWhichQuery, sDBI, _ppbDuplFragmentArr[0], &dbe);
      }

      if (comet_ftell(fp)>=lEndOfStruct || sDBI.dPepMass>g_massRange.dMaxMass)
         break;

      fread(&sDBI, sizeof(struct DBIndex), 1, fp);
      // Now read in the protein file positions here; not used yet
      _proteinInfo.lProteinFilePosition = dbe.lProteinFilePosition = comet_ftell(fp);
      fread(&lSize, sizeof(long), 1, fp);
      for (long x = 0; x < lSize; x++)
      {
         comet_fileoffset_t tmpoffset;
         fread(&tmpoffset, sizeof(comet_fileoffset_t), 1, fp);
      }

      // read past last entry in indexed db, need to break out of loop
      if (feof(fp))
         break;

      _proteinInfo.cPrevAA = sDBI.szPrevNextAA[0];
      _proteinInfo.cNextAA = sDBI.szPrevNextAA[1];
      dbe.strSeq = sDBI.szPrevNextAA[0] + sDBI.szPeptide + sDBI.szPrevNextAA[1]; 
   }

   if (*iPrecursorMatch != -1)  // if here, no mass match for CheckIdxPrecursorMatch
   {
      delete[] lReadIndex;
      fclose(fp);
      return true;
   }

   for (vector<Query*>::iterator it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
   {
      int iSize;

      iSize  = (*it)->iMatchPeptideCount;
      if (iSize > g_staticParams.options.iNumStored)
         iSize = g_staticParams.options.iNumStored;

      if ((*it)->iMatchPeptideCount > 0)
      {
         // simply take top xcorr peptide as E-value calculation too expensive
         std::sort((*it)->_pResults, (*it)->_pResults + iSize, CometPostAnalysis::SortFnXcorr);

         // Retrieve protein name
         if ((*it)->_pResults[0].pWhichProtein.at(0).lWhichProtein > -1)
         {
            comet_fseek(fp, (*it)->_pResults[0].pWhichProtein.at(0).lWhichProtein, SEEK_SET);
            fread(&lSize, sizeof(long), 1, fp);
            vector<comet_fileoffset_t> vOffsets;
            for (long x = 0; x < lSize; x++)
            {
               comet_fileoffset_t tmpoffset;
               fread(&tmpoffset, sizeof(comet_fileoffset_t), 1, fp);
               vOffsets.push_back(tmpoffset);
            }
            for (long x = 0; x < lSize; x++)
            {
               char szTmp[WIDTH_REFERENCE];
               comet_fseek(fp, vOffsets.at(x), SEEK_SET);
               fread(szTmp, sizeof(char)*WIDTH_REFERENCE, 1, fp);
               (*it)->_pResults[0].strSingleSearchProtein += szTmp;
               if (x < lSize - 1)
                  (*it)->_pResults[0].strSingleSearchProtein += " : ";
            }
         }
/*
         printf("OK  scan %d, pep %s, prot %s, xcorr %f, matchcount %d\n",
            (*it)->_spectrumInfoInternal.iScanNumber,
            (*it)->_pResults[0].szPeptide,
            (*it)->_pResults[0].strSingleSearchProtein.c_str(),
            (*it)->_pResults[0].fXcorr,
            (*it)->iMatchPeptideCount); fflush(stdout);
*/
      }
   }

   delete [] lReadIndex;
   fclose(fp);
   return true;
}


// Compare MSMS data to peptide with szProteinSeq from the input database.
bool CometSearch::SearchForPeptides(struct sDBEntry dbe,
                                    char *szProteinSeq,
                                    bool bNtermPeptideOnly,  // skip M in very first residue
                                    bool *pbDuplFragment)
{
   int iLenPeptide = 0;
   int iLenProtein;
   int iProteinSeqLengthMinus1;
   int iStartPos = 0;
   int iEndPos = 0;
   int piVarModCounts[VMODS];
   int iWhichIonSeries;
   int ctIonSeries;
   int ctLen;
   int ctCharge;
   double dCalcPepMass = 0.0;

   int iPeffRequiredVariantPosition = _proteinInfo.iPeffOrigResiduePosition;

   iLenProtein = _proteinInfo.iProteinSeqLength;

   int iFirstResiduePosition = 0;
   if (bNtermPeptideOnly)  // we're skipping the leading M
   {
      iLenProtein -= 1;
 //     iStartPos = 0;
 //     iFirstResiduePosition = 0;
   }

   if (dbe.vectorPeffMod.size() > 0) // sort vectorPeffMod by iPosition
      sort(dbe.vectorPeffMod.begin(), dbe.vectorPeffMod.end());

   if (dbe.vectorPeffVariantSimple.size() > 0) // sort peffVariantSimpleStruct by iPosition
      sort(dbe.vectorPeffVariantSimple.begin(), dbe.vectorPeffVariantSimple.end());

   memset(piVarModCounts, 0, sizeof(piVarModCounts));

   if (iPeffRequiredVariantPosition >= 0)
   {
      double dMass;

      // So find iStartPos that has to be near iPeffRequiredVariantPosition
      iStartPos = iPeffRequiredVariantPosition;
      dMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
         + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];

      while (dMass < g_massRange.dMaxMass)
      {
         if ((iStartPos == iFirstResiduePosition) || (iPeffRequiredVariantPosition - iStartPos >= MAX_PEPTIDE_LEN))
            break;

         iStartPos--;
         dMass += (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];
      }

      if (bNtermPeptideOnly && iStartPos != 0)  // Variant outside of first peptide caused by clipping Met
         return true;
   }

   iProteinSeqLengthMinus1 = iLenProtein-1;

   iEndPos = iStartPos;

   if (iLenProtein > 0)
   {
      dCalcPepMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
         + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];

      if (g_staticParams.variableModParameters.bVarModSearch)
         CountVarMods(piVarModCounts, szProteinSeq[iEndPos], iEndPos);
   }
   else
      return true;

   if (iStartPos == iFirstResiduePosition)
      dCalcPepMass += g_staticParams.staticModifications.dAddNterminusProtein;
   if (iEndPos == iProteinSeqLengthMinus1)
      dCalcPepMass += g_staticParams.staticModifications.dAddCterminusProtein;

   // Search through entire protein.
   while (iStartPos < iLenProtein)
   {
      // Check to see if peptide is within global min/mass range for all queries.
      iLenPeptide = iEndPos-iStartPos+1;

      if (iLenPeptide<MAX_PEPTIDE_LEN-1)  // account for terminating char
      {
         if (g_staticParams.options.bCreateIndex)
         {
            if (WithinMassTolerance(dCalcPepMass, szProteinSeq, iStartPos, iEndPos) == 1)
            {
               Threading::LockMutex(g_pvQueryMutex);

               // add to DBIndex vector
               DBIndex sEntry;
               sEntry.dPepMass = dCalcPepMass;  //MH+ mass
               strncpy(sEntry.szPeptide, szProteinSeq + iStartPos, iLenPeptide);
               sEntry.szPeptide[iLenPeptide]='\0';

               if (iStartPos == 0)
                  sEntry.szPrevNextAA[0] = '-';
               else
                  sEntry.szPrevNextAA[0] = szProteinSeq[iStartPos - 1];
               if (iEndPos == iProteinSeqLengthMinus1)
                  sEntry.szPrevNextAA[1] = '-';
               else
                  sEntry.szPrevNextAA[1] = szProteinSeq[iEndPos + 1];
         
               sEntry.lIndexProteinFilePosition = _proteinInfo.lProteinFilePosition;
               memset(sEntry.pcVarModSites, 0, sizeof(sEntry.pcVarModSites));

               g_pvDBIndex.push_back(sEntry);

               Threading::UnlockMutex(g_pvQueryMutex);
            }
         }
         else
         {
            int iWhichQuery = WithinMassTolerance(dCalcPepMass, szProteinSeq, iStartPos, iEndPos);

            // If PEFF variant analysis, see if peptide is results of amino acid swap
            if (iPeffRequiredVariantPosition >= 0 && iWhichQuery != -1)
            {
               bool bPass = false;;

               if ((iStartPos <= iPeffRequiredVariantPosition && iPeffRequiredVariantPosition <= iEndPos))
               {
                  // all is good here, continue to next "if" loop below
                  bPass = true;
               }
               else
               {
                  // iSearchEnZymeOffset == 1
                  // K.DLRST  where K is iPeffRequiredVariantPosition and D is iStart ... must check for this
                  //
                  // iSearchEnZymeOffset == 0
                  // S.DLRST  where D is iPeffRequiredVariantPosition and D is iStart; already accounted for in if() above
                  //
                  // iSearchEnZymeOffset == 1
                  // SESTEQR.S   where R is iPeffRequiredVariantPositon and R is iEnd; already accounted for in if() above
                  //
                  // iSearchEnZymeOffset == 0
                  // SESTEQL.D   where D is iPeffRequiredVariantPosition and L is iEnd ... must check for this


                  // At this point, only case need to check for is if variant is position before iStartPos
                  // and causes enzyme digest.  Or if variant is position after iEndPos and causes enzyme
                  // digest. All other cases are ok as variant is in peptide.
                  if (iPeffRequiredVariantPosition == iStartPos - 1)
                  {
                     if (g_staticParams.enzymeInformation.iSearchEnzymeOffSet == 1 && CheckEnzymeStartTermini(szProteinSeq, iStartPos))
                        bPass = true;
                     else
                        bPass = false;
                  }

                  else if (iPeffRequiredVariantPosition == iEndPos + 1)
                  {
                     if (g_staticParams.enzymeInformation.iSearchEnzymeOffSet == 0 && CheckEnzymeEndTermini(szProteinSeq, iEndPos))
                        bPass = true;
                     else
                        bPass = false;
                  }
               }

               if (bPass == false)
                  iWhichQuery = -1;
            }

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
                     char szDecoyPeptide[MAX_PEPTIDE_LEN_P2];  // Allow for prev/next AA in string.

                     // Calculate ion series just once to compare against all relevant query spectra.
                     if (bFirstTimeThroughLoopForPeptide && !g_staticParams.options.bCreateIndex)
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
                              {
                                 pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforward, _pdAAreverse))] = false;
                              }
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
                           iDecoyEndPos = (int)strlen(szDecoyPeptide)-2;

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
                                    pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforwardDecoy, _pdAAreverseDecoy))] = false;
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

                     int piVarModSites[4]; // This is unused variable mod placeholder to pass into XcorrScore.

                     if (!g_staticParams.variableModParameters.bRequireVarMod)
                     {
                        XcorrScore(szProteinSeq, iStartPos, iEndPos, iStartPos, iEndPos, false,
                              dCalcPepMass, false, iWhichQuery, iLenPeptide, piVarModSites, &dbe);

                        if (g_staticParams.options.iDecoySearch)
                        {
                           XcorrScore(szDecoyPeptide, iStartPos, iEndPos, 1, iLenPeptide, false,
                                 dCalcPepMass, true, iWhichQuery, iLenPeptide, piVarModSites, &dbe);
                        }
                     }
                  }
                  iWhichQuery++;
               }
            }
         }
      }

      // Increment end.
      if (dCalcPepMass <= g_massRange.dMaxMass && iEndPos < iProteinSeqLengthMinus1 && iLenPeptide<MAX_PEPTIDE_LEN)
      {
         iEndPos++;

         if (iEndPos < iLenProtein)
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
               VariableModSearch(szProteinSeq, piVarModCounts, iStartPos, iEndPos, pbDuplFragment, &dbe);
            }

            if (g_massRange.bNarrowMassRange)
               SubtractVarMods(piVarModCounts, szProteinSeq[iStartPos], iStartPos);
         }

         if (bNtermPeptideOnly)
            return true;

         if (g_massRange.bNarrowMassRange)
         {
            dCalcPepMass -= (double)g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];
            if (iStartPos == iFirstResiduePosition)
               dCalcPepMass -= g_staticParams.staticModifications.dAddNterminusProtein;
         }
         iStartPos++;          // Increment start of peptide.

         // Skip any more processing because outside of range of variant
         if (iPeffRequiredVariantPosition>=0 && iStartPos > iPeffRequiredVariantPosition+1)
            return true;

         if (g_massRange.bNarrowMassRange)
         {  
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
         else
         {
            iEndPos = iStartPos;

            dCalcPepMass = g_staticParams.precalcMasses.dOH2ProtonCtermNterm
               + g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[iStartPos]];

            if (g_staticParams.variableModParameters.bVarModSearch)
            {
               for (int x = 0; x < VMODS; x++)  //reset variable mod counts
                  piVarModCounts[x] = 0;
               if (g_staticParams.variableModParameters.bVarModSearch)
                  CountVarMods(piVarModCounts, szProteinSeq[iEndPos], iEndPos);
            }
         }
      }
   }

   return true;
}


void CometSearch::AnalyzeIndexPep(int iWhichQuery,
                                  DBIndex sDBI,
                                  bool *pbDuplFragment,
                                  struct sDBEntry *dbe)
{
   int iWhichIonSeries;
   int ctIonSeries;
   int ctLen;
   int ctCharge;
   int iStartPos = 0;
   int iEndPos = (int)strlen(sDBI.szPeptide)-1;
   int iLenPeptide = (int)strlen(sDBI.szPeptide);
   bool bFirstTimeThroughLoopForPeptide = true;

   // Compare calculated fragment ions against all matching query spectra.
   while (iWhichQuery < (int)g_pvQuery.size())
   {
      if (sDBI.dPepMass < g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassToleranceMinus)
      {
         // If calculated mass is smaller than low mass range.
         break;
      }

      // Mass tolerance check for particular query against this candidate peptide mass.
      if (CheckMassMatch(iWhichQuery, sDBI.dPepMass))
      {
         bool bIsVarModPep = false;

         // Calculate ion series just once to compare against all relevant query spectra.
         if (bFirstTimeThroughLoopForPeptide)
         {
            int iLenMinus1 = iEndPos - iStartPos; // Equals iLenPeptide minus 1.

            bFirstTimeThroughLoopForPeptide = false;

            double dBion = g_staticParams.precalcMasses.dNtermProton;
            double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

/*
n/c-term protein mods not supported yet
            if (iStartPos == 0)
               dBion += g_staticParams.staticModifications.dAddNterminusProtein;
            if (iEndPos == iLenProteinMinus1)
               dYion += g_staticParams.staticModifications.dAddCterminusProtein;
*/

            // variable N-term peptide mod
            if (sDBI.pcVarModSites[iLenPeptide] > 0)
               dBion += g_staticParams.variableModParameters.varModList[sDBI.pcVarModSites[iLenPeptide]-1].dVarModMass;

            // variable C-term peptide mod
            if (sDBI.pcVarModSites[iLenPeptide + 1] > 0)
               dYion += g_staticParams.variableModParameters.varModList[sDBI.pcVarModSites[iLenPeptide+1]-1].dVarModMass;

            // Generate pdAAforward for _pResults[0].szPeptide
            for (int i=0; i<iEndPos; i++)
            {
               int iPos;
              
               iPos = i;
               dBion += g_staticParams.massUtility.pdAAMassFragment[(int)sDBI.szPeptide[i]];

               if (sDBI.pcVarModSites[iPos] > 0)
               {
                  dBion += g_staticParams.variableModParameters.varModList[sDBI.pcVarModSites[iPos]-1].dVarModMass;
                  bIsVarModPep = true;
               }

               _pdAAforward[iPos] = dBion;

               dYion += g_staticParams.massUtility.pdAAMassFragment[(int)sDBI.szPeptide[iEndPos - i]];

               iPos = iEndPos - i;
               if (sDBI.pcVarModSites[iPos] > 0)
               {
                  dYion += g_staticParams.variableModParameters.varModList[sDBI.pcVarModSites[iPos]-1].dVarModMass;
                  bIsVarModPep = true;
               }

               _pdAAreverse[i] = dYion;
            }

            // Now get the set of binned fragment ions once to compare this peptide against all matching spectra.
            for (ctCharge=1; ctCharge<=g_massRange.iMaxFragmentCharge; ctCharge++)
            {
               for (ctIonSeries=0; ctIonSeries<g_staticParams.ionInformation.iNumIonSeriesUsed; ctIonSeries++)
               {
                  iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ctIonSeries];

                  for (ctLen=0; ctLen<iLenMinus1; ctLen++)
                  {
                     pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforward, _pdAAreverse))] = false;
                  }
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
         }

         if (!g_staticParams.variableModParameters.bRequireVarMod || g_staticParams.bIndexDb)
         {
            int piVarModSites[MAX_PEPTIDE_LEN_P2];
            for (int x = 0; x < MAX_PEPTIDE_LEN_P2; x++)
               piVarModSites[x] = (int)sDBI.pcVarModSites[x];  // translate char to int
            XcorrScore(sDBI.szPeptide, iStartPos+1, iEndPos+1, iStartPos, iEndPos, bIsVarModPep,
                  sDBI.dPepMass, false, iWhichQuery, iLenPeptide, piVarModSites, dbe);
         }
      }
      iWhichQuery++;
   }
}


// Analyze regions of the sequence that are affected by the variant
// Each analyzed peptide must either contain the variant or be flanked
// by the variant enabling new enzyme-digested peptide
void CometSearch::SearchForVariants(struct sDBEntry dbe,
                                    char *szProteinSeq,
                                    bool *pbDuplFragment)

{
   int iLen = (int)strlen(szProteinSeq);

   // Walk through each variant
   for (int i=0; i<(int)dbe.vectorPeffVariantSimple.size(); i++)
   {
      if (dbe.vectorPeffVariantSimple.at(i).iPosition < iLen)
      {
         int iPosition = dbe.vectorPeffVariantSimple.at(i).iPosition;
         char cResidue = dbe.vectorPeffVariantSimple.at(i).cResidue;

         // make sure there's an actual AA change
         if (cResidue == szProteinSeq[iPosition])
         {
            if (g_staticParams.options.bVerboseOutput)
            {
               // Log a warning message here that the variant change didn't change the residue?
               char szErrorMsg[256];
               sprintf(szErrorMsg, " Warning: protein %s has variant '%c' at position %d with the same original AA residue.\n", 
                     dbe.strName.c_str(), cResidue, iPosition);
               logout(szErrorMsg);
            }
         }
         else
         {
            char cOrigResidue;

            cOrigResidue  = szProteinSeq[iPosition];

            // Place variant in protein
            szProteinSeq[iPosition] = cResidue;

            _proteinInfo.iPeffOrigResiduePosition = iPosition;
            _proteinInfo.cPeffOrigResidue = cOrigResidue;

            SearchForPeptides(dbe, szProteinSeq, false, pbDuplFragment);

            // FIX: with change to how bClibNtermMet is implemented in SearchForPeptides, validate
            // that it's still functional with PEFF variant.
            if (g_staticParams.options.bClipNtermMet && iPosition == 0 && cResidue == 'M')
               SearchForPeptides(dbe, szProteinSeq+1, true, pbDuplFragment);

            // Return protein sequence to back to normal
            szProteinSeq[iPosition] = cOrigResidue;
         }
      }
   }
}


int CometSearch::WithinMassTolerance(double dCalcPepMass,
                                     char* szProteinSeq,
                                     int iStartPos,
                                     int iEndPos)
{
   int iPepLen = iEndPos - iStartPos + 1;

   if (dCalcPepMass >= g_massRange.dMinMass
         && dCalcPepMass <= g_massRange.dMaxMass
         && iPepLen >= g_staticParams.options.peptideLengthRange.iStart
         && iPepLen <= g_staticParams.options.peptideLengthRange.iEnd
         && CheckEnzymeTermini(szProteinSeq, iStartPos, iEndPos))
   {
      // if creating indexed database, only care of peptide is within global mass range
      if (g_staticParams.options.bCreateIndex)
      {
         return 1;
      }

      // Now that we know it's within the global mass range of our queries and has
      // proper enzyme termini, check if within mass tolerance of any given entry.

      int iPos;

      // Do a binary search on list of input queries to find matching mass.
      iPos=BinarySearchMass(0, (int)g_pvQuery.size(), dCalcPepMass);

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
      return -1;
}


// This function will return true if unmodified peptide mass + any combination of
// PEFF mod is within mass tolerance of any peptide query
bool CometSearch::WithinMassTolerancePeff(double dCalcPepMass,
                                          vector<PeffPositionStruct>* vPeffArray,
                                          int iStartPos,
                                          int iEndPos)
{
   int i;


/*
   // Print out list of PEFF mods
   for (i=0; i<(int) (*vPeffArray).size(); i++)
   {
      printf("*** OK  %d.  position %d\n", i, (*vPeffArray).at(i).iPosition);
      for (int ii=0; ii<(int)(*vPeffArray).at(i).vectorWhichPeff.size(); ii++)
      {
         printf(" ... %f %f\n",
               (*vPeffArray).at(i).vectorMassDiffMono.at(ii),
               (*vPeffArray).at(i).vectorMassDiffAvg.at(ii));
      }
   }
*/

   // number of residues with a PEFF mod
   int n = (int)(*vPeffArray).size();
   double dMassAddition;

   // permute through each PEFF mod; return true if any are of right mass; only 1 PEFF mod at a time

   for (i=0 ; i<n ; i++)
   {
      // only consider those PEFF mods that are within the peptide
      if ((*vPeffArray).at(i).iPosition >= iStartPos && (*vPeffArray).at(i).iPosition <=iEndPos)
      {
         for (int ii=0; ii<(int)(*vPeffArray).at(i).vectorWhichPeff.size(); ii++)
         {
            dMassAddition = (*vPeffArray).at(i).vectorMassDiffMono.at(ii);

            // if dCalcPepMass + dMassAddition is within mass tol, add these mods

            // At this stage here, just need to see if any PEFF mod addition is within mass tolerance
            // of any entry.  If so, simply return true here and will repeat the PEFF permutations later.

            // Do a binary search on list of input queries to find matching mass.
            int iPos=BinarySearchMass(0, (int)g_pvQuery.size(), dCalcPepMass + dMassAddition);

            // Seek back to first peptide entry that matches mass tolerance in case binary
            // search doesn't hit the first entry.
            while (iPos>0 && g_pvQuery.at(iPos)->_pepMassInfo.dPeptideMassTolerancePlus >= dCalcPepMass)
               iPos--;

            if (iPos != -1)
               return true;
         }
      }
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
      int iCountInternalCleavageSites=0;

      bBeginCleavage = (iStartPos==0
            || szProteinSeq[iStartPos-1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iStartPos -1 + g_staticParams.enzymeInformation.iOneMinusOffset])
               && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iStartPos -1 + g_staticParams.enzymeInformation.iTwoMinusOffset])));

      bEndCleavage = (iEndPos==(int)(_proteinInfo.iTmpProteinSeqLength - 1)    // either _proteinInfo.iProteinSeqLength or 1 less for clip N-term M
            || szProteinSeq[iEndPos+1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iEndPos + g_staticParams.enzymeInformation.iOneMinusOffset])
               && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iEndPos + g_staticParams.enzymeInformation.iTwoMinusOffset])));

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
         bBreakPoint = strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[i+ g_staticParams.enzymeInformation.iOneMinusOffset])
            && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[i+ g_staticParams.enzymeInformation.iTwoMinusOffset]);

         if (bBreakPoint)
         {
            if ((g_staticParams.enzymeInformation.iOneMinusOffset == 0 && i != iEndPos)  // Ignore last residue.
                  || (g_staticParams.enzymeInformation.iOneMinusOffset == 1 && i != iStartPos))  // Ignore first residue.
            {
               iCountInternalCleavageSites++;

               // Need to include -iOneMinusEnzymeOffSet in if statement below because for
               // AspN cleavage, the very last residue, if followed by a D, will be counted
               // as an internal cleavage site.
               if (iCountInternalCleavageSites - g_staticParams.enzymeInformation.iOneMinusOffset > g_staticParams.enzymeInformation.iAllowedMissedCleavage)
                  return false;
            }
         }
      }
   }

   return true;
}


bool CometSearch::CheckEnzymeStartTermini(char *szProteinSeq,
                                          int iStartPos)
{
   if (!g_staticParams.options.bNoEnzymeSelected)
   {
      bool bBeginCleavage=0;

      bBeginCleavage = (iStartPos==0
            || szProteinSeq[iStartPos-1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iStartPos -1 + g_staticParams.enzymeInformation.iOneMinusOffset])
          && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iStartPos -1 + g_staticParams.enzymeInformation.iTwoMinusOffset])));
            

      return bBeginCleavage;
   }

   return true;
}


bool CometSearch::CheckEnzymeEndTermini(char *szProteinSeq,
                                        int iEndPos)
{
   if (!g_staticParams.options.bNoEnzymeSelected)
   {
      bool bEndCleavage=0;

      bEndCleavage = (iEndPos==(int)(_proteinInfo.iTmpProteinSeqLength - 1)    // either _proteinInfo.iProteinSeqLength or 1 less for clip N-term M
            || szProteinSeq[iEndPos+1]=='*'
            || (strchr(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, szProteinSeq[iEndPos + g_staticParams.enzymeInformation.iOneMinusOffset])
          && !strchr(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, szProteinSeq[iEndPos + g_staticParams.enzymeInformation.iTwoMinusOffset])));

      return bEndCleavage;
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
      return middle;
   else if (vectorPeffOBO.at(middle).strMod > strMod)
      return BinarySearchPeffStrMod(start, middle - 1, strMod, vectorPeffOBO);

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
      return BinarySearchMass(start, middle - 1, dCalcPepMass);

   if ((int)middle+1 < end)
      return BinarySearchMass(middle + 1, end, dCalcPepMass);
   else
   {
      if ((int)(middle+1) == end
            && end < (int)g_pvQuery.size()
            && g_pvQuery.at(end)->_pepMassInfo.dPeptideMassToleranceMinus <= dCalcPepMass
            && dCalcPepMass <= g_pvQuery.at(end)->_pepMassInfo.dPeptideMassTolerancePlus)
      {
         return end;
      }
      else
         return -1;
   }
}


bool CometSearch::CheckMassMatch(int iWhichQuery,
                                 double dCalcPepMass)
{
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   if (g_pvDIAWindows.size() == 0)
   {
      int iMassOffsetsSize = (int)g_staticParams.vectorMassOffsets.size();

      if ((dCalcPepMass >= pQuery->_pepMassInfo.dPeptideMassToleranceMinus)
            && (dCalcPepMass <= pQuery->_pepMassInfo.dPeptideMassTolerancePlus))
      {
         double dMassDiff = pQuery->_pepMassInfo.dExpPepMass - dCalcPepMass;

         if (g_staticParams.tolerances.iIsotopeError == 0 && iMassOffsetsSize == 0)
            return true;
         else if (iMassOffsetsSize > 0)
         {
            // need to account for both mass offsets and possible isotope offsets

            if (g_staticParams.tolerances.iIsotopeError == 0)
            {
               for (int i=0; i<iMassOffsetsSize; i++)
               {
                  if (fabs(dMassDiff - g_staticParams.vectorMassOffsets[i]) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     return true;
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
            else if (g_staticParams.tolerances.iIsotopeError == 5)  // -1/0/1/2/3
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
            else if (g_staticParams.tolerances.iIsotopeError == 6)  //  -3/-2/-1/0/1/2/3
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
                        || (fabs(dTmpDiff + dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                        || (fabs(dTmpDiff + d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                        || (fabs(dTmpDiff + d3C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance))
                  {
                     return true;
                  }
               }
               return false;
            }
			else if (g_staticParams.tolerances.iIsotopeError == 7)
			{
				double dC13diff = C13_DIFF;

				for (int i = 0; i < iMassOffsetsSize; i++)
				{
					double dTmpDiff = dMassDiff - g_staticParams.vectorMassOffsets[i];

					if ((fabs(dTmpDiff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
						|| (fabs(dTmpDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance) 
						|| (fabs(dTmpDiff + dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance))
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
            else if (g_staticParams.tolerances.iIsotopeError == 5)
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
            else if (g_staticParams.tolerances.iIsotopeError == 6)
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
                     || (fabs(dMassDiff + dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dMassDiff + d2C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance)
                     || (fabs(dMassDiff + d3C13diff)<= pQuery->_pepMassInfo.dPeptideMassTolerance))
               {
                  return true;
               }
               return false;
            }
			else if (g_staticParams.tolerances.iIsotopeError == 7)
			{
				double dC13diff = C13_DIFF;

				// Using C13 isotope mass difference here but likely should
				// be slightly bigger for other elemental contaminents.

				if ((fabs(dMassDiff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
					|| (fabs(dMassDiff - dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance)
					|| (fabs(dMassDiff + dC13diff) <= pQuery->_pepMassInfo.dPeptideMassTolerance))
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
   }
   else
   {
      // use DIA windows
      if (pQuery->_pepMassInfo.dPeptideMassToleranceMinus <= dCalcPepMass
            && dCalcPepMass <= pQuery->_pepMassInfo.dPeptideMassTolerancePlus)
      {
         return true;
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
   int iSeqLength = (int)strlen(szDNASequence);

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

   _proteinInfo.cPeffOrigResidue = '\0';
   _proteinInfo.iPeffOrigResiduePosition = -9;
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
                             int iStartResidue,        // needed for decoy peptide; otherwise just duplicate of iStartPos
                             int iEndResidue,
                             int iStartPos,
                             int iEndPos,
                             bool bFoundVariableMod,
                             double dCalcPepMass,
                             bool bDecoyPep,
                             int iWhichQuery,
                             int iLenPeptide,
                             int *piVarModSites,
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
         }
         else
         {
            ppSparseFastXcorrData = pQuery->ppfSparseFastXcorrData;
         }

         int bin,x,y;
         for (ctLen=0; ctLen<iLenPeptideMinus1; ctLen++)
         {
            //MH: newer sparse matrix converts bin to sparse matrix bin
            bin = *(*(*(*p_uiBinnedIonMasses + ctCharge)+ctIonSeries)+ctLen);
            x = bin / SPARSE_MATRIX_SIZE;
            if (x>iMax || ppSparseFastXcorrData[x]==NULL) // x should never be > iMax so this is just a safety check
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
      if (pQuery->iHistogramCount < DECOY_SIZE)
         pQuery->iHistogramCount += 1;
   }

   if (bDecoyPep && g_staticParams.options.iDecoySearch==2)
   {
      if (dXcorr > pQuery->fLowestDecoyXcorrScore)
      {
         if (!CheckDuplicate(iWhichQuery, iStartResidue, iEndResidue, iStartPos, iEndPos, bFoundVariableMod, dCalcPepMass,
                  szProteinSeq, bDecoyPep, piVarModSites, dbe))
         {
            StorePeptide(iWhichQuery, iStartResidue, iStartPos, iEndPos, bFoundVariableMod, szProteinSeq,
                  dCalcPepMass, dXcorr, bDecoyPep,  piVarModSites, dbe);
         }
      }
   }
   else
   {
      if (dXcorr > pQuery->fLowestXcorrScore)
      {
         // no need to check duplicates if indexed database search
         if (g_staticParams.bIndexDb
            || !CheckDuplicate(iWhichQuery, iStartResidue, iEndResidue, iStartPos, iEndPos, bFoundVariableMod, dCalcPepMass,
                  szProteinSeq, bDecoyPep, piVarModSites, dbe))
         {
            StorePeptide(iWhichQuery, iStartResidue, iStartPos, iEndPos, bFoundVariableMod, szProteinSeq,
                  dCalcPepMass, dXcorr, bDecoyPep, piVarModSites, dbe);
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
                               int iStartResidue,
                               int iStartPos,
                               int iEndPos,
                               bool bFoundVariableMod,
                               char *szProteinSeq,
                               double dCalcPepMass,
                               double dXcorr,
                               bool bDecoyPep,
                               int *piVarModSites,
                               struct sDBEntry *dbe)
{
   int i;
   int iLenPeptide;
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   iLenPeptide = iEndPos - iStartPos + 1;

   if (iLenPeptide >= MAX_PEPTIDE_LEN)
      return;

   if (g_staticParams.options.iDecoySearch==2 && bDecoyPep)  // store separate decoys
   {
      short siLowestDecoySpScoreIndex;

      siLowestDecoySpScoreIndex = pQuery->siLowestDecoySpScoreIndex;

      pQuery->iDecoyMatchPeptideCount++;
      pQuery->_pDecoys[siLowestDecoySpScoreIndex].iLenPeptide = iLenPeptide;

      memcpy(pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPeptide, szProteinSeq+iStartPos, iLenPeptide*sizeof(char));
      pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPeptide[iLenPeptide]='\0';

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].dPepMass = dCalcPepMass;

      if (pQuery->_spectrumInfoInternal.iChargeState > 2)
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].iTotalIons
            = (iLenPeptide-1)*(pQuery->_spectrumInfoInternal.iChargeState-1) * g_staticParams.ionInformation.iNumIonSeriesUsed;
      }
      else
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].iTotalIons
            = (iLenPeptide-1)*g_staticParams.ionInformation.iNumIonSeriesUsed;
      }

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].fXcorr = (float)dXcorr;

      if (g_staticParams.bIndexDb)
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0] = _proteinInfo.cPrevAA;
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[1] = _proteinInfo.cNextAA;
      }
      else
      {
         if (iStartPos == 0)
         {
            // check if clip n-term met
            if (g_staticParams.options.bClipNtermMet && dbe->strSeq.c_str()[0] == 'M' && !strcmp(dbe->strSeq.c_str() + 1, szProteinSeq))
               pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0] = 'M';
            else
               pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0] = '-';
         }
         else
            pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

         if (iEndPos == _proteinInfo.iProteinSeqLength-1)
            pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[1] = '-';
         else
            pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];
      }

      // store PEFF info; +1 and -1 to account for PEFF in flanking positions
      if (_proteinInfo.iPeffOrigResiduePosition != -9 && (iStartPos-1 <= _proteinInfo.iPeffOrigResiduePosition) && (_proteinInfo.iPeffOrigResiduePosition <= iEndPos+1))
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].iPeffOrigResiduePosition = _proteinInfo.iPeffOrigResiduePosition - iStartPos;
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].cPeffOrigResidue = _proteinInfo.cPeffOrigResidue;
      }
      else
      {
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].iPeffOrigResiduePosition = -9;
         pQuery->_pDecoys[siLowestDecoySpScoreIndex].cPeffOrigResidue = '\0';
      }

      // store protein
      struct ProteinEntryStruct pTmp;

      pTmp.lWhichProtein = dbe->lProteinFilePosition;
      pTmp.iStartResidue = iStartResidue + 1;  // 1 based position
      pTmp.cPrevAA = pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[0];
      pTmp.cNextAA = pQuery->_pDecoys[siLowestDecoySpScoreIndex].szPrevNextAA[1];

      pQuery->_pDecoys[siLowestDecoySpScoreIndex].pWhichDecoyProtein.clear();
      pQuery->_pDecoys[siLowestDecoySpScoreIndex].pWhichDecoyProtein.push_back(pTmp);
      pQuery->_pDecoys[siLowestDecoySpScoreIndex].lProteinFilePosition = dbe->lProteinFilePosition;

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         if (!bFoundVariableMod)   // Normal peptide in variable mod search.
         {
            memset(pQuery->_pDecoys[siLowestDecoySpScoreIndex].piVarModSites, 0, _iSizepiVarModSites);
            memset(pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites, 0, _iSizepdVarModSites);
         }
         else
         {
            memcpy(pQuery->_pDecoys[siLowestDecoySpScoreIndex].piVarModSites, piVarModSites, _iSizepiVarModSites);

            int iVal;
            for (i=0; i<iLenPeptide; i++)
            {
               iVal = pQuery->_pDecoys[siLowestDecoySpScoreIndex].piVarModSites[i];

               if (iVal > 0)
               {
                  pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites[i]
                     = g_staticParams.variableModParameters.varModList[iVal-1].dVarModMass;
               }
               else if (iVal < 0)
               {
                  pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites[i] = dbe->vectorPeffMod.at(-iVal-1).dMassDiffMono;
                  strcpy(pQuery->_pDecoys[siLowestDecoySpScoreIndex].pszMod[i], dbe->vectorPeffMod.at(-iVal-1).szMod);
               }
               else
                  pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites[i] = 0.0;
            }
         }
      }
      else
      {
         memset(pQuery->_pDecoys[siLowestDecoySpScoreIndex].piVarModSites, 0, _iSizepiVarModSites);
         memset(pQuery->_pDecoys[siLowestDecoySpScoreIndex].pdVarModSites, 0, _iSizepdVarModSites);
      }

      // Get new lowest score.
      pQuery->fLowestDecoyXcorrScore = pQuery->_pDecoys[0].fXcorr;
      siLowestDecoySpScoreIndex=0;

      for (i=g_staticParams.options.iNumStored-1; i>0; i--)
      {
         if (pQuery->_pDecoys[i].fXcorr < pQuery->fLowestDecoyXcorrScore || pQuery->_pDecoys[i].iLenPeptide == 0)
         {
            pQuery->fLowestDecoyXcorrScore = pQuery->_pDecoys[i].fXcorr;
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

      memcpy(pQuery->_pResults[siLowestSpScoreIndex].szPeptide, szProteinSeq+iStartPos, iLenPeptide*sizeof(char));
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

      if (g_staticParams.bIndexDb)
      {
         pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0] = _proteinInfo.cPrevAA;
         pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1] = _proteinInfo.cNextAA;
      }
      else
      {
         if (iStartPos == 0)
         {
            // check if clip n-term met
            if (g_staticParams.options.bClipNtermMet && dbe->strSeq.c_str()[0] == 'M' && !strcmp(dbe->strSeq.c_str() + 1, szProteinSeq))
               pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0] = 'M';
            else
               pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0] = '-';
         }
         else
            pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

         if (iEndPos == _proteinInfo.iProteinSeqLength-1)
            pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1] = '-';
         else
            pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];
      }

      // store PEFF info; +1 and -1 to account for PEFF in flanking positions
      if (_proteinInfo.iPeffOrigResiduePosition != -9
            && (iStartPos-1 <= _proteinInfo.iPeffOrigResiduePosition)
            && (_proteinInfo.iPeffOrigResiduePosition <= iEndPos+1))
      {
         pQuery->_pResults[siLowestSpScoreIndex].iPeffOrigResiduePosition = _proteinInfo.iPeffOrigResiduePosition - iStartPos;
         pQuery->_pResults[siLowestSpScoreIndex].cPeffOrigResidue = _proteinInfo.cPeffOrigResidue;
      }
      else
      {
         pQuery->_pResults[siLowestSpScoreIndex].iPeffOrigResiduePosition = -9;
         pQuery->_pResults[siLowestSpScoreIndex].cPeffOrigResidue = '\0';
      }

      // store protein
      if (bDecoyPep)
      {
         struct ProteinEntryStruct pTmp;

         pTmp.lWhichProtein = dbe->lProteinFilePosition;
         pTmp.iStartResidue = iStartResidue + 1;  // 1 based position
         pTmp.cPrevAA = pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0];
         pTmp.cNextAA = pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1];

         pQuery->_pResults[siLowestSpScoreIndex].pWhichProtein.clear();
         pQuery->_pResults[siLowestSpScoreIndex].pWhichDecoyProtein.clear();
         pQuery->_pResults[siLowestSpScoreIndex].pWhichDecoyProtein.push_back(pTmp);
         pQuery->_pResults[siLowestSpScoreIndex].lProteinFilePosition = dbe->lProteinFilePosition;
      }
      else
      {
         struct ProteinEntryStruct pTmp;

         pTmp.lWhichProtein = dbe->lProteinFilePosition;
         pTmp.iStartResidue = iStartResidue + 1;  // 1 based position
         pTmp.cPrevAA = pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[0];
         pTmp.cNextAA = pQuery->_pResults[siLowestSpScoreIndex].szPrevNextAA[1];

         pQuery->_pResults[siLowestSpScoreIndex].pWhichProtein.clear();
         pQuery->_pResults[siLowestSpScoreIndex].pWhichDecoyProtein.clear();
         pQuery->_pResults[siLowestSpScoreIndex].pWhichProtein.push_back(pTmp);
         pQuery->_pResults[siLowestSpScoreIndex].lProteinFilePosition = dbe->lProteinFilePosition;
      }

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         if (!bFoundVariableMod)  // Normal peptide in variable mod search.
         {
            memset(pQuery->_pResults[siLowestSpScoreIndex].piVarModSites, 0, _iSizepiVarModSites);
            memset(pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites, 0, _iSizepdVarModSites);
         }
         else
         {
            memcpy(pQuery->_pResults[siLowestSpScoreIndex].piVarModSites, piVarModSites, _iSizepiVarModSites);

            int iVal;
            for (i=0; i<iLenPeptide; i++)
            {
               iVal = pQuery->_pResults[siLowestSpScoreIndex].piVarModSites[i];

               if (iVal > 0)
               {
                  pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites[i]
                     = g_staticParams.variableModParameters.varModList[iVal-1].dVarModMass;
               }
               else if (iVal < 0)
               {
                  pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites[i] = dbe->vectorPeffMod.at(-iVal-1).dMassDiffMono;
                  strcpy(pQuery->_pResults[siLowestSpScoreIndex].pszMod[i], dbe->vectorPeffMod.at(-iVal-1).szMod);
               }
               else
                  pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites[i] = 0.0;
            }
         }
      }
      else
      {
         memset(pQuery->_pResults[siLowestSpScoreIndex].piVarModSites, 0, _iSizepiVarModSites);
         memset(pQuery->_pResults[siLowestSpScoreIndex].pdVarModSites, 0, _iSizepdVarModSites);
      }

      // Get new lowest score.
      pQuery->fLowestXcorrScore = pQuery->_pResults[0].fXcorr;
      siLowestSpScoreIndex=0;

      for (i=g_staticParams.options.iNumStored-1; i>0; i--)
      {
         if (pQuery->_pResults[i].fXcorr < pQuery->fLowestXcorrScore || pQuery->_pResults[i].iLenPeptide == 0)
         {
            pQuery->fLowestXcorrScore = pQuery->_pResults[i].fXcorr;
            siLowestSpScoreIndex = i;
         }
      }

      pQuery->siLowestSpScoreIndex = siLowestSpScoreIndex;
   }
}


int CometSearch::CheckDuplicate(int iWhichQuery,
                                int iStartResidue,
                                int iEndResidue,
                                int iStartPos,
                                int iEndPos,
                                bool bFoundVariableMod,
                                double dCalcPepMass,
                                char *szProteinSeq,
                                bool bDecoyPep,
                                int *piVarModSites,
                                struct sDBEntry *dbe)
{
   int i,
       iLenMinus1,
       bIsDuplicate=0;
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   if (g_staticParams.bIndexDb)  // assume duplicates not possible with indexed db
      return 0;

   iLenMinus1 = iEndPos-iStartPos+1;

   if (g_staticParams.options.iDecoySearch == 2 && bDecoyPep)
   {
      for (i=0; i<g_staticParams.options.iNumStored; i++)
      {
         // Quick check of peptide sequence length first.
         if (iLenMinus1 == pQuery->_pDecoys[i].iLenPeptide && isEqual(dCalcPepMass, pQuery->_pDecoys[i].dPepMass))
         {
            if (!memcmp(pQuery->_pDecoys[i].szPeptide, szProteinSeq + iStartPos, sizeof(char)*(pQuery->_pDecoys[i].iLenPeptide)))
            {
               bIsDuplicate = 1;
            }
            else if (g_staticParams.options.bTreatSameIL)
            {
               bIsDuplicate = 1;

               for (int ii=iStartPos; ii<=iEndPos; ii++ )
               {
                  if (pQuery->_pDecoys[i].szPeptide[ii-iStartPos] != szProteinSeq[ii])
                  {
                     if ((pQuery->_pDecoys[i].szPeptide[ii-iStartPos]!='I' && pQuery->_pDecoys[i].szPeptide[ii-iStartPos]!='L')
                           || (szProteinSeq[ii] != 'I' && szProteinSeq[ii] != 'L'))
                     {
                        bIsDuplicate = 0;
                        break;
                     }
                  }
               }
            }

            // If bIsDuplicate & variable mod search, check modification sites to see if peptide already stored.
            if (bIsDuplicate && g_staticParams.variableModParameters.bVarModSearch && bFoundVariableMod)
            {
               if (g_staticParams.peffInfo.iPeffSearch)
               {
                  int iVal;
                  for (int ii=0; ii<=pQuery->_pDecoys[i].iLenPeptide; ii++)
                  {
                     iVal = pQuery->_pDecoys[i].piVarModSites[ii];
      
                     if ( (iVal>0 && piVarModSites[ii]<=0) || (iVal<0 && piVarModSites[ii]>=0) )
                     {
                        bIsDuplicate = 0;
                        break;
                     }
                     else if (iVal > 0)
                     {
                        if (piVarModSites[ii] != pQuery->_pDecoys[i].piVarModSites[ii])
                        {
                           bIsDuplicate = 0;
                           break;
                        }
                     }
                     else if (iVal < 0)
                     {
                        // must loop through each modsite and see if OBO string is same
                        if (strcmp(dbe->vectorPeffMod.at(-(piVarModSites[ii])-1).szMod, pQuery->_pDecoys[i].pszMod[ii]))
                        {
                           bIsDuplicate = 0;
                           break;
                        }
                     }
                     else
                        pQuery->_pDecoys[i].pdVarModSites[i] = 0.0;
                  }
               }
               else
               {
                  if (!memcmp(piVarModSites, pQuery->_pDecoys[i].piVarModSites, sizeof(int)*(pQuery->_pDecoys[i].iLenPeptide + 2)))
                     bIsDuplicate = 1;
                  else
                    bIsDuplicate = 0;
               }
            }

            if (bIsDuplicate)
            {
               struct ProteinEntryStruct pTmp;

               pTmp.lWhichProtein = dbe->lProteinFilePosition;
               pTmp.iStartResidue = iStartResidue + 1;  // 1 based position

               if (iStartResidue == 0)
                  pTmp.cPrevAA = '-';
               else
                  pTmp.cPrevAA = dbe->strSeq[iStartResidue - 1];  //must be dbe->strSeq here instead of szProteinSeq because latter could be short decoy

               if (iEndResidue == (int)(dbe->strSeq.length() -1))
                  pTmp.cNextAA = '-';
               else
                  pTmp.cNextAA = dbe->strSeq[iEndResidue + 1];  //must be dbe->strSeq here instead of szProteinSeq because latter could be short decoy

               pQuery->_pDecoys[i].pWhichDecoyProtein.push_back(pTmp);

               // if duplicate, check to see if need to replace stored protein info 
               // with protein that's earlier in database
               if (pQuery->_pDecoys[i].lProteinFilePosition > dbe->lProteinFilePosition)
               {     
                  pQuery->_pDecoys[i].lProteinFilePosition = dbe->lProteinFilePosition;
                  if (iStartPos == 0)
                     pQuery->_pDecoys[i].szPrevNextAA[0] = '-';
                  else
                     pQuery->_pDecoys[i].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];
               
                  if (iEndPos == (int)strlen(szProteinSeq) - 1)
                     pQuery->_pDecoys[i].szPrevNextAA[1] = '-';
                  else
                     pQuery->_pDecoys[i].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];

                  // also if IL equivalence set, go ahead and copy peptide from first sequence
                  memcpy(pQuery->_pDecoys[i].szPeptide, szProteinSeq+iStartPos, pQuery->_pDecoys[i].iLenPeptide*sizeof(char));
                  pQuery->_pDecoys[i].szPeptide[pQuery->_pDecoys[i].iLenPeptide]='\0';
               }

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
         if (iLenMinus1 == pQuery->_pResults[i].iLenPeptide && isEqual(dCalcPepMass, pQuery->_pResults[i].dPepMass))
         {
            if (!memcmp(pQuery->_pResults[i].szPeptide, szProteinSeq + iStartPos, sizeof(char)*pQuery->_pResults[i].iLenPeptide))
            {
               bIsDuplicate = 1;
            }
            else if (g_staticParams.options.bTreatSameIL)
            {
               bIsDuplicate = 1;

               for (int ii=iStartPos; ii<=iEndPos; ii++ )
               {
                  if (pQuery->_pResults[i].szPeptide[ii-iStartPos] != szProteinSeq[ii])
                  {
                     if ((pQuery->_pResults[i].szPeptide[ii-iStartPos]!='I' && pQuery->_pResults[i].szPeptide[ii-iStartPos]!='L')
                           || (szProteinSeq[ii] != 'I' && szProteinSeq[ii] != 'L'))
                     {
                        bIsDuplicate = 0;
                        break;
                     }
                  }
               }
            }

            // If bIsDuplicate & variable mod search, check modification sites to see if peptide already stored.
            if (bIsDuplicate && g_staticParams.variableModParameters.bVarModSearch && bFoundVariableMod)
            {
               if (g_staticParams.peffInfo.iPeffSearch)
               {
                  int iVal;
                  for (int ii=0; ii<=pQuery->_pResults[i].iLenPeptide; ii++)
                  {
                     iVal = pQuery->_pResults[i].piVarModSites[ii];
      
                     if ((iVal > 0 && piVarModSites[ii] <= 0) || (iVal < 0 && piVarModSites[ii] >= 0))
                     {
                        bIsDuplicate = 0;
                        break;
                     }
                     else if (iVal >= 0)
                     {
                        if (piVarModSites[ii] != pQuery->_pResults[i].piVarModSites[ii])
                        {
                           bIsDuplicate = 0;
                           break;
                        }
                     }
                     else // iVal < 0
                     {
                        // must loop through each modsite and see if OBO string is same
                        if (strcmp(dbe->vectorPeffMod.at(-(piVarModSites[ii])-1).szMod, pQuery->_pResults[i].pszMod[ii]))
                        {
                           bIsDuplicate = 0;
                           break;
                        }
                     }
                  }
               }
               else
               {
                  if (!memcmp(piVarModSites, pQuery->_pResults[i].piVarModSites, sizeof(int)*(pQuery->_pResults[i].iLenPeptide + 2)))
                     bIsDuplicate = 1;
                  else
                    bIsDuplicate = 0;
               }
            }

            if (bIsDuplicate)
            {
               struct ProteinEntryStruct pTmp;

               pTmp.lWhichProtein = dbe->lProteinFilePosition;
               pTmp.iStartResidue = iStartResidue + 1;  // 1 based position

               if (iStartResidue == 0)
                  pTmp.cPrevAA = '-';
               else
                  pTmp.cPrevAA = dbe->strSeq[iStartResidue - 1];  //must be dbe->strSeq here instead of szProteinSeq because latter could be short decoy

               if (iEndResidue == (int)(dbe->strSeq.length() -1))
                  pTmp.cNextAA = '-';
               else
                  pTmp.cNextAA = dbe->strSeq[iEndResidue + 1];  //must be dbe->strSeq here instead of szProteinSeq because latter could be short decoy

               if (bDecoyPep)
                  pQuery->_pResults[i].pWhichDecoyProtein.push_back(pTmp);
               else
                  pQuery->_pResults[i].pWhichProtein.push_back(pTmp);

               // FIX:  figure out what to do with lProteinFilePosition and bDecoyPep
 
               // if duplicate, check to see if need to replace stored protein info
               // with protein that's earlier in database
               if (pQuery->_pResults[i].lProteinFilePosition > dbe->lProteinFilePosition)
               {
                  pQuery->_pResults[i].lProteinFilePosition = dbe->lProteinFilePosition;

                  if (iStartPos == 0)
                     pQuery->_pResults[i].szPrevNextAA[0] = '-';
                  else
                     pQuery->_pResults[i].szPrevNextAA[0] = szProteinSeq[iStartPos - 1];

                  if (iEndPos == (int)strlen(szProteinSeq)-1)
                     pQuery->_pResults[i].szPrevNextAA[1] = '-';
                  else
                     pQuery->_pResults[i].szPrevNextAA[1] = szProteinSeq[iEndPos + 1];

                  // also if IL equivalence set, go ahead and copy peptide from first sequence
                  memcpy(pQuery->_pResults[i].szPeptide, szProteinSeq+iStartPos, pQuery->_pResults[i].iLenPeptide*sizeof(char));
                  pQuery->_pResults[i].szPeptide[pQuery->_pResults[i].iLenPeptide]='\0';
               }

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
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
            piVarModCounts[i]--;
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
               piVarModCounts[i]--;
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3) // peptide C
               piVarModCounts[i]--;
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
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
            piVarModCounts[i]++;
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
               piVarModCounts[i]++;
            else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3) // peptide C
               piVarModCounts[i]++;
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
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
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
                  return true;
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
                  return true;
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


void CometSearch::VariableModSearch(char *szProteinSeq,
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
   int iLenProteinMinus1;

   bool bPeffMod = false;

   vector<PeffPositionStruct> vPeffArray;

   if (_proteinInfo.iPeffOrigResiduePosition >=0)
      iLenProteinMinus1 = (int)strlen(szProteinSeq) - 1;
   else
      iLenProteinMinus1 = _proteinInfo.iProteinSeqLength - 1;

   long lNumIterations = 0;

   if ((int)dbe->vectorPeffMod.size() > 0)

   // do not apply PEFF mods to a PEFF variant peptide
   if (_proteinInfo.iPeffOrigResiduePosition < 0 && (int)dbe->vectorPeffMod.size() > 0)
   {
      bool bMatch;

      // vPeffArray will reduce representing peff mods by position on peptide

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

   // consider possible n- and c-term mods; c-term position is not necessarily iEndPos
   // so need to add some buffer there
   for (i=0; i<VMODS; i++)
   {
      piTmpTotVarModCt[i] = piTmpTotBinaryModCt[i] = 0; // useless but supresses gcc 'may be used uninitialized in this function' warnings

      piVarModCountsNC[i] = piVarModCounts[i];

      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0))
      {
         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
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
                  && iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= iLenProteinMinus1)
            {
               piVarModCountsNC[i] += 1;
            }
            if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                  && iEndPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= iLenProteinMinus1)
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
               piVarModCountsNC[i] += 1;
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


//                            if (g_staticParams.options.lMaxIterations > 0 && lNumIterations >= g_staticParams.options.lMaxIterations)
//                               break;

                              if (i1>0 || i2>0 || i3>0 || i4>0 || i5>0 || i6>0 || i7>0 || i8>0 || i9>0 || bPeffMod)
                              {
                                 double dCalcPepMass;
                                 int iTmpEnd;
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
                                    if (iTmpEnd-iStartPos+1 < MAX_PEPTIDE_LEN-1)
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
                                                if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
                                                   _varModInfo.varModStatList[i].iTotVarModCt++;

                                                else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0) // protein N
                                                {
                                                   if (iTmpEnd <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                      _varModInfo.varModStatList[i].iTotVarModCt++;
                                                }
                                                else if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1) // protein C
                                                {
                                                   if (iTmpEnd + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                         >= iLenProteinMinus1)
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
                                                if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n')
                                                      && ((g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
                                                         || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0
                                                            && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                         || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1
                                                               &&  iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                               >= iLenProteinMinus1)
                                                         || g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2))
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
                                                   if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
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
                                                            >= iLenProteinMinus1)
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
                                                         if (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
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
                                                            if (iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= iLenProteinMinus1)
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
                                                         && ((g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0
                                                               && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1
                                                                  &&  iStartPos + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance
                                                                  >= _proteinInfo.iProteinSeqLength-1)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2)))
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
                                                if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c')
                                                      && ((g_staticParams.variableModParameters.varModList[i].iVarModTermDistance < 0
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0
                                                               && iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1
                                                               &&  iTmpEnd + g_staticParams.variableModParameters.varModList[i].iVarModTermDistance >= iLenProteinMinus1)
                                                            || (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 2
                                                               && iTmpEnd-iStartPos <= g_staticParams.variableModParameters.varModList[i].iVarModTermDistance)
                                                            || g_staticParams.variableModParameters.varModList[i].iWhichTerm == 3)))
                                                {
                                                   _varModInfo.varModStatList[i].iTotVarModCt++;
                                                }
                                             }
                                          }

                                          // also need to consider all residue mods that have a peptide c-term distance
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

                                       if (bValid && HasVariableMod(piTmpVarModCounts, iStartPos, iTmpEnd, dbe))
                                       {
                                          // mass including terminal mods that need to be tracked separately here
                                          // because we are considering multiple terminating positions in peptide
                                          double dTmpCalcPepMass;

                                          dTmpCalcPepMass = dCalcPepMass;

                                          // static protein terminal mod
                                          if (iTmpEnd == iLenProteinMinus1)
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
                                                bDoPeffAnalysis = WithinMassTolerancePeff(dTmpCalcPepMass, &vPeffArray, iStartPos, iTmpEnd);
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
                                             PermuteMods(szProteinSeq, iWhichQuery, 1, pbDuplFragment, &bDoPeffAnalysis, &vPeffArray, dbe, &lNumIterations);

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
}


double CometSearch::TotalVarModMass(int *pVarModCounts)
{
   double dTotVarModMass = 0;

   int i;
   for (i=0; i<VMODS; i++)
      dTotVarModMass += g_staticParams.variableModParameters.varModList[i].dVarModMass * pVarModCounts[i];

   return dTotVarModMass;
}


// false=exit; true=continue
bool CometSearch::PermuteMods(char *szProteinSeq,
                              int iWhichQuery,
                              int iWhichMod,
                              bool *pbDuplFragment,
                              bool *bDoPeffAnalysis,
                              vector <PeffPositionStruct>* vPeffArray,
                              struct sDBEntry *dbe,
                              long *lNumIterations)
{
   int iModIndex;

// if (g_staticParams.options.lMaxIterations > 0 && *lNumIterations >= g_staticParams.options.lMaxIterations)
//    return false;

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
         if (!MergeVarMods(szProteinSeq, iWhichQuery, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe, lNumIterations))
            return false;
      }
      else
      {
         if (!PermuteMods(szProteinSeq, iWhichQuery, iWhichMod+1, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe, lNumIterations))
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
            if (!MergeVarMods(szProteinSeq, iWhichQuery, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe, lNumIterations))
               return false;
         }
         else
         {
            if (!PermuteMods(szProteinSeq, iWhichQuery, iWhichMod+1, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe, lNumIterations))
               return false;
         }
      }
   }
   else
   {
      if (iWhichMod == 9)
      {
         if (!MergeVarMods(szProteinSeq, iWhichQuery, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe, lNumIterations))
            return false;
      }
      else
      {
         if (!PermuteMods(szProteinSeq, iWhichQuery, iWhichMod+1, pbDuplFragment, bDoPeffAnalysis, vPeffArray, dbe, lNumIterations))
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


// always need to return true so permutations of variable mods continues
// except when lMaxIterations is hit
bool CometSearch::MergeVarMods(char *szProteinSeq,
                               int iWhichQuery,
                               bool *pbDuplFragment,
                               bool *bDoPeffAnalysis,
                               vector <PeffPositionStruct>* vPeffArray,
                               struct sDBEntry *dbe,
                               long *lNumIterations)
{
   int piVarModSites[MAX_PEPTIDE_LEN_P2];
   int piVarModCharIdx[VMODS];
   int i;
   int j;

// if (g_staticParams.options.lMaxIterations > 0 && *lNumIterations >= g_staticParams.options.lMaxIterations)
//    return false;  // this will stop all further permutations of peptide

   // at this point, need to compare current modified peptide
   // against all relevant entries

   // but first, calculate modified peptide mass as it could've changed
   // by terminating earlier than start/end positions defined in VariableModSearch()
   double dCalcPepMass = g_staticParams.precalcMasses.dNtermProton + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS;

   int iLenMinus1 = _varModInfo.iEndPos - _varModInfo.iStartPos;     // equals iLenPeptide-1
   int iLenPeptide = iLenMinus1+1;
   int iLenProteinMinus1;

   if (_proteinInfo.iPeffOrigResiduePosition>=0)
      iLenProteinMinus1 = (int)strlen(szProteinSeq) - 1;
   else
      iLenProteinMinus1 = _proteinInfo.iProteinSeqLength - 1;

   // contains positional coding of a variable mod at each index which equals an AA residue
   memset(piVarModSites, 0, _iSizepiVarModSites);
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
            if (piVarModSites[iLenPeptide] != 0)  // conflict in two variable mods on n-term
               return true;

            // store the modification number at modification position
            piVarModSites[iLenPeptide] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];   //FIX: check this logic
            dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;;
         }
         piVarModCharIdx[j] += 1;     //FIX: check this logic
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
            if (piVarModSites[iLenPeptide+1] != 0)  // conflict in two variable mods on c-term
               return true;

            // store the modification number at modification position
            piVarModSites[iLenPeptide+1] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];   //FIX: check this logic
            dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
         }
         piVarModCharIdx[j] += 1;     //FIX: check this logic
      }
   }

   // Generate pdAAforward for _pResults[0].szPeptide
   for (i=_varModInfo.iStartPos; i<=_varModInfo.iEndPos; i++)
   {
      int iPos = i - _varModInfo.iStartPos;

      dCalcPepMass += g_staticParams.massUtility.pdAAMassParent[(int)szProteinSeq[i]];

      // This loop is where all individual variable mods are combined
      for (j=0; j<VMODS; j++)
      {
         if (!isEqual(g_staticParams.variableModParameters.varModList[j].dVarModMass, 0.0)
               && (_varModInfo.varModStatList[j].iMatchVarModCt > 0)
               && strchr(g_staticParams.variableModParameters.varModList[j].szVarModChar, szProteinSeq[i]))
         {
            if (g_staticParams.variableModParameters.varModList[j].iVarModTermDistance < 0)
            {
               if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
               {
                  if (piVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                     return true;

                  // store the modification number at modification position
                  piVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                  dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
               }
               piVarModCharIdx[j] += 1;
            }
            else  // terminal distance constraint specified
            {
               if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 0)      // protein N
               {
                  if (i <= g_staticParams.variableModParameters.varModList[j].iVarModTermDistance)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (piVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return true;

                        // store the modification number at modification position
                        piVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j] += 1;
                  }
               }
               else if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 1) // protein C
               {
                  if (i + g_staticParams.variableModParameters.varModList[j].iVarModTermDistance >= iLenProteinMinus1)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (piVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return true;

                        // store the modification number at modification position
                        piVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j] += 1;
                  }
               }
               else if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 2) // peptide N
               {
                  if (iPos <= g_staticParams.variableModParameters.varModList[j].iVarModTermDistance)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (piVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return true;

                        // store the modification number at modification position
                        piVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j] += 1;
                  }
               }
               else if (g_staticParams.variableModParameters.varModList[j].iWhichTerm == 3) // peptide C
               {
                  if (iPos + g_staticParams.variableModParameters.varModList[j].iVarModTermDistance >= iLenMinus1)
                  {
                     if (_varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]])
                     {
                        if (piVarModSites[iPos] != 0)  // conflict in two variable mods on same residue
                           return true;

                        // store the modification number at modification position
                        piVarModSites[iPos] = _varModInfo.varModStatList[j].iVarModSites[piVarModCharIdx[j]];
                        dCalcPepMass += g_staticParams.variableModParameters.varModList[j].dVarModMass;
                     }
                     piVarModCharIdx[j] += 1;
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

            // if mod is required, see if it is present in the peptide; also consider n- and c-term positions
            for (i=_varModInfo.iStartPos; i<=_varModInfo.iEndPos+2; i++)
            {
               int iPos = i - _varModInfo.iStartPos;

               // variable mode code is j+1. Check if that required j mod is present in peptide
               if (piVarModSites[iPos] == j+1)
               {
                  bPresent = true;
                  break;
               }
            }

            if (!bPresent)
               return true;
         }
      }
   }

   // Check to see if variable mod cannot occur on c-term residue
   for (j=0; j<VMODS; j++)
   {
      if (g_staticParams.variableModParameters.varModList[j].iVarModTermDistance == -2
            && !isEqual(g_staticParams.variableModParameters.varModList[j].dVarModMass, 0.0))
      {
         // not allowed for terminal residue to have this mod
         if (piVarModSites[_varModInfo.iEndPos - _varModInfo.iStartPos] == j+1)
            return true;
      }
   }

   // At this point, piVarModSites[] values should only range of 0 to 9.  Now possibly
   // add PEFF mods which are encoded as negative values.  Must copy current state of
   // piVarModSites as the loop below will go through PEFF mods and we need to
   // add each single PEFF mod to these existing variable mods.

   int piTmpVarModSites[MAX_PEPTIDE_LEN_P2];
   memcpy(piTmpVarModSites, piVarModSites, _iSizepiVarModSites);

   // Now that normal variable mods are taken care of, add in PEFF mods if pertinent
   if (*bDoPeffAnalysis && !g_staticParams.options.bCreateIndex)
   {
      // permute through PEFF

      int n = (int)(*vPeffArray).size();  // number of residues with a PEFF mod

      double dMassAddition;

      for (i=0 ; i<n ; i++)
      {
         // only consider those PEFF mods that are within the peptide
         if ((*vPeffArray).at(i).iPosition >= _varModInfo.iStartPos && (*vPeffArray).at(i).iPosition <= _varModInfo.iEndPos)
         {
            for (int ii=0; ii<(int)(*vPeffArray).at(i).vectorWhichPeff.size(); ii++)
            {  
               dMassAddition = (*vPeffArray).at(i).vectorMassDiffMono.at(ii);

               // For each iteration of PEFF mods, start with fresh state of variable mods
               memcpy(piVarModSites, piTmpVarModSites, _iSizepiVarModSites);

               // if dCalcPepMass + dMassAddition is within mass tol, add these mods

               // Validate that total mass is within tolerance of some query entry
               double dTmpCalcPepMass = dCalcPepMass + dMassAddition;

               // With PEFF mods added in, find if new mass is within any query tolerance
               iWhichQuery = WithinMassTolerance(dTmpCalcPepMass, szProteinSeq, _varModInfo.iStartPos, _varModInfo.iEndPos);

               if (iWhichQuery != -1)
               {
                  bool bValidPeffPosition = true;
                  int iTmpModPosition  = (*vPeffArray).at(i).iPosition - _varModInfo.iStartPos;

                  // make sure PEFF mod location doesn't conflict with existing variable mod
                  if (piVarModSites[iTmpModPosition] == 0)
                  {
                     // PEFF mods are encoded as negative values to reference appropriate PeffModStruct entry
                     // Sadly needs to be offset by -1 because first PEFF index is 0
                     piVarModSites[iTmpModPosition] = -1 -(*vPeffArray).at(i).vectorWhichPeff.at(ii); // use negative values for PEFF mods
                  }
                  else
                  {
                     bValidPeffPosition = false;
                     break;
                  }

                  if  (bValidPeffPosition)
                  {
                     // Need to check if mass is ok
         
                     // Do a binary search on list of input queries to find matching mass.
                     iWhichQuery = BinarySearchMass(0, (int)g_pvQuery.size(), dTmpCalcPepMass);

                     // Seek back to first peptide entry that matches mass tolerance in case binary
                     // search doesn't hit the first entry.
                     while (iWhichQuery>0 && g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassTolerancePlus >= dCalcPepMass)
                        iWhichQuery--;

                     // Only if this PEFF mod (plus possible variable mods) is within mass tolerance, continue
                     if (iWhichQuery != -1)
                     {
                        // FIX: add test here as piVarModSites must contain a negative PEFF value
                        CalcVarModIons(szProteinSeq, iWhichQuery, pbDuplFragment, piVarModSites, dTmpCalcPepMass, iLenPeptide, dbe, lNumIterations);
                     }
                  }
               }
            }
         }
      }
   }
   else
   {
      bool bHasVarMod = false;

      // first make sure no negative piVarModSites entries as no PEFF here
      for (int x=0; x<iLenPeptide+2; x++)
      {
         if  (piVarModSites[x] < 0)
         {
            char szErrorMsg[256];
            sprintf(szErrorMsg, " Error, piVarModSites[%d]=%d; should not be less than zeros since no PEFF.\n", x, piVarModSites[x]);
            string strErrorMsg(szErrorMsg);
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(szErrorMsg);
            return true;
         }
      }

      // now check to make sure there is a variable mod
      for (int x=0; x<iLenPeptide+2; x++)
      {
         if  (piVarModSites[x] > 0)
         {
            bHasVarMod = true;
            break;
         }
      }

      if (bHasVarMod)
      {
         if (g_staticParams.options.bCreateIndex)
         {
            Threading::LockMutex(g_pvQueryMutex);

            // add to DBIndex vector
            DBIndex sDBTmp;
            sDBTmp.dPepMass = dCalcPepMass;  //MH+ mass
            strncpy(sDBTmp.szPeptide, szProteinSeq + _varModInfo.iStartPos, _varModInfo.iEndPos - _varModInfo.iStartPos + 1);
            sDBTmp.szPeptide[_varModInfo.iEndPos - _varModInfo.iStartPos + 1]='\0';
         
            if (_varModInfo.iStartPos == 0)
               sDBTmp.szPrevNextAA[0] = '-';
            else
               sDBTmp.szPrevNextAA[0] = szProteinSeq[_varModInfo.iStartPos - 1];
            if (_varModInfo.iEndPos == _proteinInfo.iProteinSeqLength - 1)         //FIX why this vs. dbe.strSeq.length()?
               sDBTmp.szPrevNextAA[1] = '-';
            else
               sDBTmp.szPrevNextAA[1] = szProteinSeq[_varModInfo.iEndPos + 1];
         
            sDBTmp.lIndexProteinFilePosition = _proteinInfo.lProteinFilePosition;

            memset(sDBTmp.pcVarModSites, 0, sizeof(sDBTmp.pcVarModSites));

            for (int x=0; x<iLenPeptide+2; x++)  // +2 for n/c term mods
               sDBTmp.pcVarModSites[x] = (char)piVarModSites[x];

            g_pvDBIndex.push_back(sDBTmp);

            Threading::UnlockMutex(g_pvQueryMutex);
         }
         else
         {
            CalcVarModIons(szProteinSeq, iWhichQuery, pbDuplFragment, piVarModSites, dCalcPepMass, iLenPeptide, dbe, lNumIterations);
         }
      }
   }

   return true;
}


bool CometSearch::CalcVarModIons(char *szProteinSeq,
                                 int iWhichQuery,
                                 bool *pbDuplFragment,
                                 int *piVarModSites,
                                 double dCalcPepMass,
                                 int iLenPeptide,
                                 struct sDBEntry *dbe,
                                 long *lNumIterations)
{
   int piVarModSitesDecoy[MAX_PEPTIDE_LEN_P2];
   char szDecoyPeptide[MAX_PEPTIDE_LEN_P2];  // allow for prev/next AA in string
   int ctIonSeries;
   int ctLen;
   int ctCharge;
   int iWhichIonSeries;
   int i;
   int iLenMinus1 = iLenPeptide - 1;

   bool bFirstTimeThroughLoopForPeptide = true;

   int iLenProteinMinus1;

// *lNumIterations += 1;

   iLenProteinMinus1 = _proteinInfo.iProteinSeqLength - 1;

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
            if (_varModInfo.iEndPos == iLenProteinMinus1)
               dYion += g_staticParams.staticModifications.dAddCterminusProtein;

            // variable N-term
            if (piVarModSites[iLenPeptide] > 0)
               dBion += g_staticParams.variableModParameters.varModList[piVarModSites[iLenPeptide]-1].dVarModMass;

            // variable C-term
            if (piVarModSites[iLenPeptide + 1] > 0)
               dYion += g_staticParams.variableModParameters.varModList[piVarModSites[iLenPeptide+1]-1].dVarModMass;

            // Generate pdAAforward for _pResults[0].szPeptide
            for (i=_varModInfo.iStartPos; i<_varModInfo.iEndPos; i++)
            {
               int iPos = i - _varModInfo.iStartPos;

               dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[i]];

               if (piVarModSites[iPos] > 0)
                  dBion += g_staticParams.variableModParameters.varModList[piVarModSites[iPos]-1].dVarModMass;
               else if (piVarModSites[iPos] < 0)
                  dBion += (dbe->vectorPeffMod.at(-piVarModSites[iPos]-1)).dMassDiffMono;

               _pdAAforward[iPos] = dBion;

               dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szProteinSeq[_varModInfo.iEndPos - i +_varModInfo.iStartPos]];

               iPos = _varModInfo.iEndPos - i;
               if (piVarModSites[iPos] > 0)
                  dYion += g_staticParams.variableModParameters.varModList[piVarModSites[iPos]-1].dVarModMass;
               else if (piVarModSites[iPos] < 0)
                  dYion += (dbe->vectorPeffMod.at(-piVarModSites[iPos]-1)).dMassDiffMono;

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
               int piTmpVarModSearchSites[MAX_PEPTIDE_LEN_P2];  // placeholder to reverse variable mods

               // Generate reverse peptide.  Keep prev and next AA in szDecoyPeptide string.
               // So actual reverse peptide starts at position 1 and ends at len-2 (as len-1
               // is next AA).

               // Store flanking residues from original sequence.
               if (_varModInfo.iStartPos==0)
                  szDecoyPeptide[0]='-';
               else
                  szDecoyPeptide[0]=szProteinSeq[_varModInfo.iStartPos-1];

               if (_varModInfo.iEndPos == iLenProteinMinus1)
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
                     piTmpVarModSearchSites[_varModInfo.iEndPos-i-1] = piVarModSites[i- _varModInfo.iStartPos];
                  }

                  szDecoyPeptide[_varModInfo.iEndPos - _varModInfo.iStartPos+1]=szProteinSeq[_varModInfo.iEndPos];  // last residue stays same
                  piTmpVarModSearchSites[iLenPeptide-1] = piVarModSites[iLenPeptide-1];
               }
               else
               {
                  // first residue stays the same:  change ABCDEK to AKEDCB

                  for (i=_varModInfo.iEndPos; i>=_varModInfo.iStartPos+1; i--)
                  {
                     szDecoyPeptide[_varModInfo.iEndPos-i+2] = szProteinSeq[i];
                     piTmpVarModSearchSites[_varModInfo.iEndPos-i+1] = piVarModSites[i- _varModInfo.iStartPos];
                  }

                  szDecoyPeptide[1]=szProteinSeq[_varModInfo.iStartPos];  // first residue stays same
                  piTmpVarModSearchSites[0] = piVarModSites[0];
               }

               piTmpVarModSearchSites[iLenPeptide]   = piVarModSites[iLenPeptide];    // N-term
               piTmpVarModSearchSites[iLenPeptide+1] = piVarModSites[iLenPeptide+1];  // C-term
               memcpy(piVarModSitesDecoy, piTmpVarModSearchSites, (iLenPeptide+2)*sizeof(int));

               // Now need to recalculate _pdAAforward and _pdAAreverse for decoy entry
               double dBion = g_staticParams.precalcMasses.dNtermProton;
               double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

               // use same protein terminal static mods as target peptide
               if (_varModInfo.iStartPos == 0)
                  dBion += g_staticParams.staticModifications.dAddNterminusProtein;
               if (_varModInfo.iEndPos == iLenProteinMinus1)
                  dYion += g_staticParams.staticModifications.dAddCterminusProtein;

               // variable N-term
               if (piVarModSitesDecoy[iLenPeptide] > 0)
                  dBion += g_staticParams.variableModParameters.varModList[piVarModSitesDecoy[iLenPeptide]-1].dVarModMass;

               // variable C-term
               if (piVarModSitesDecoy[iLenPeptide + 1] > 0)
                  dYion += g_staticParams.variableModParameters.varModList[piVarModSitesDecoy[iLenPeptide+1]-1].dVarModMass;

               int iDecoyStartPos = 1;  // This is start/end for newly created decoy peptide
               int iDecoyEndPos = (int)strlen(szDecoyPeptide)-2;

               int iTmp1;
               int iTmp2;

               // Generate pdAAforward for szDecoyPeptide
               for (i=iDecoyStartPos; i<iDecoyEndPos; i++)
               {
                  iTmp1 = i-iDecoyStartPos;
                  iTmp2 = iDecoyEndPos - iTmp1;

                  dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szDecoyPeptide[i]];
                  if (piVarModSitesDecoy[iTmp1] > 0)
                     dBion += g_staticParams.variableModParameters.varModList[piVarModSitesDecoy[iTmp1]-1].dVarModMass;
                  else if (piVarModSitesDecoy[iTmp1] < 0)
                     dBion += (dbe->vectorPeffMod.at(-piVarModSitesDecoy[iTmp1]-1)).dMassDiffMono;

                  dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szDecoyPeptide[iTmp2]];
//FIX: check if piVarModSites index is correct
                  if (piVarModSitesDecoy[iTmp2-iDecoyStartPos] > 0)
                     dYion += g_staticParams.variableModParameters.varModList[piVarModSitesDecoy[iTmp2-iDecoyStartPos]-1].dVarModMass;
                  else if (piVarModSitesDecoy[iTmp2-iDecoyStartPos] < 0)
                     dYion += (dbe->vectorPeffMod.at(-piVarModSitesDecoy[iTmp2-iDecoyStartPos]-1)).dMassDiffMono;

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
                        pbDuplFragment[BIN(GetFragmentIonMass(iWhichIonSeries, ctLen, ctCharge, _pdAAforwardDecoy, _pdAAreverseDecoy))] = false;
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

         XcorrScore(szProteinSeq, _varModInfo.iStartPos, _varModInfo.iEndPos, _varModInfo.iStartPos, _varModInfo.iEndPos,
               true, dCalcPepMass, false, iWhichQuery, iLenPeptide, piVarModSites, dbe);

         if (g_staticParams.options.iDecoySearch)
         {
            XcorrScore(szDecoyPeptide, _varModInfo.iStartPos, _varModInfo.iEndPos, 1, iLenPeptide, true, 
                  dCalcPepMass, true, iWhichQuery, iLenPeptide, piVarModSitesDecoy, dbe);
         }
      }

      iWhichQuery++;
   }

   return true;
}
