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

#include "SearchUtils.h"

static bool HasSuffixIgnoreCase(const char* pszFileName, int iLen, const char* pszSuffix)
{
   int iSuffixLen = (int)strlen(pszSuffix);

   return iLen >= iSuffixLen && !STRCMP_IGNORE_CASE(pszFileName + iLen - iSuffixLen, pszSuffix);
}


static InputType GetInputType(const char* pszFileName)
{
   int iLen = (int)strlen(pszFileName);

   if (HasSuffixIgnoreCase(pszFileName, iLen, ".mzXML")
         || HasSuffixIgnoreCase(pszFileName, iLen, ".mzML")
         || HasSuffixIgnoreCase(pszFileName, iLen, ".mzXML.gz")
         || HasSuffixIgnoreCase(pszFileName, iLen, ".mzML.gz"))
   {
      return InputType_MZXML;
   }
   else if (HasSuffixIgnoreCase(pszFileName, iLen, ".raw"))
   {
      return InputType_RAW;
   }
   else if (HasSuffixIgnoreCase(pszFileName, iLen, ".ms2")
         || HasSuffixIgnoreCase(pszFileName, iLen, ".cms2"))
   {
      return InputType_MS2;
   }
   else if (HasSuffixIgnoreCase(pszFileName, iLen, ".mgf"))
   {
      return InputType_MGF;
   }

   return InputType_UNKNOWN;
}


bool UpdateInputFile(InputFileInfo* pFileInfo)
{
   bool bUpdateBaseName = false;
   char szTmpBaseName[SIZE_FILE];

   if (g_staticParams.inputFile.szBaseName[0] == '\0' || g_pvInputFiles.size() > 1)
      bUpdateBaseName = true;
   else
      strcpy(szTmpBaseName, g_staticParams.inputFile.szBaseName);

   g_staticParams.inputFile = *pFileInfo;
   g_staticParams.inputFile.iInputType = GetInputType(g_staticParams.inputFile.szFileName);

   if (InputType_UNKNOWN == g_staticParams.inputFile.iInputType)
      return false;

   FILE* fp;
   if ((fp = fopen(g_staticParams.inputFile.szFileName, "r")) == NULL)
   {
      string strErrorMsg = " Error - cannot read input file \"" + string(g_staticParams.inputFile.szFileName) + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
   else
   {
      fclose(fp);
   }

#ifndef CRUX
   if (bUpdateBaseName)
   {
      char* pStr;
      int iLen = (int)strlen(g_staticParams.inputFile.szFileName);

      strcpy(g_staticParams.inputFile.szBaseName, g_staticParams.inputFile.szFileName);

      if ((pStr = strrchr(g_staticParams.inputFile.szBaseName, '.')))
         *pStr = '\0';

      if (HasSuffixIgnoreCase(g_staticParams.inputFile.szFileName, iLen, ".mzXML.gz")
            || HasSuffixIgnoreCase(g_staticParams.inputFile.szFileName, iLen, ".mzML.gz"))
      {
         if ((pStr = strrchr(g_staticParams.inputFile.szBaseName, '.')))
            *pStr = '\0';
      }
   }
   else
   {
      strcpy(g_staticParams.inputFile.szBaseName, szTmpBaseName);
   }
#endif

   return true;
}


void SetMSLevelFilter(MSReader& mstReader)
{
   vector<MSSpectrumType> msLevel;

   if (g_staticParams.options.iMSLevel == 3)
      msLevel.push_back(MS3);
   else if (g_staticParams.options.iMSLevel == 2)
      msLevel.push_back(MS2);
   else if (g_staticParams.options.iMSLevel == 1)
      msLevel.push_back(MS1);

   mstReader.setFilter(msLevel);
}


bool AllocateResultsMem(std::vector<Query*>& queries)
{
   for (std::vector<Query*>::iterator it = queries.begin(); it != queries.end(); ++it)
   {
      Query* pQuery = *it;

      try
      {
         pQuery->_pResults = new Results[g_staticParams.options.iNumStored];
      }
      catch (std::bad_alloc& ba)
      {
         string strErrorMsg = " Error - new(_pResults[]). bad_alloc: \"" + std::string(ba.what()) + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }

      if (g_staticParams.options.iDecoySearch == 2)
      {
         try
         {
            pQuery->_pDecoys = new Results[g_staticParams.options.iNumStored];
         }
         catch (std::bad_alloc& ba)
         {
            string strErrorMsg = " Error - new(_pDecoys[]). bad_alloc: " + std::string(ba.what()) + "\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }

      pQuery->iMatchPeptideCount = 0;
      pQuery->iDecoyMatchPeptideCount = 0;
      memset(pQuery->iXcorrHistogram, 0, sizeof(pQuery->iXcorrHistogram));

      for (int j = 0; j < g_staticParams.options.iNumStored; ++j)
      {
         pQuery->_pResults[j].dPepMass = 0.0;
         pQuery->_pResults[j].dExpect = 999;
         pQuery->_pResults[j].fScoreSp = 0.0;
         pQuery->_pResults[j].fXcorr = (float)g_staticParams.options.dMinimumXcorr;
         pQuery->_pResults[j].fAScorePro = 0.0;
         pQuery->_pResults[j].usiLenPeptide = 0;
         pQuery->_pResults[j].usiRankSp = 0;
         pQuery->_pResults[j].usiMatchedIons = 0;
         pQuery->_pResults[j].usiTotalIons = 0;
         pQuery->_pResults[j].szPeptide[0] = '\0';
         pQuery->_pResults[j].sAScoreProSiteScores.clear();
         pQuery->_pResults[j].pWhichProtein.clear();
         pQuery->_pResults[j].sPeffOrigResidues.clear();
         pQuery->_pResults[j].iPeffOrigResiduePosition = -9;

         if (g_staticParams.options.iDecoySearch)
            pQuery->_pResults[j].pWhichDecoyProtein.clear();

         if (g_staticParams.options.iDecoySearch == 2)
         {
            pQuery->_pDecoys[j].dPepMass = 0.0;
            pQuery->_pDecoys[j].dExpect = 999;
            pQuery->_pDecoys[j].fScoreSp = 0.0;
            pQuery->_pDecoys[j].fXcorr = (float)g_staticParams.options.dMinimumXcorr;
            pQuery->_pDecoys[j].fAScorePro = 0.0;
            pQuery->_pDecoys[j].usiLenPeptide = 0;
            pQuery->_pDecoys[j].usiRankSp = 0;
            pQuery->_pDecoys[j].usiMatchedIons = 0;
            pQuery->_pDecoys[j].usiTotalIons = 0;
            pQuery->_pDecoys[j].szPeptide[0] = '\0';
            pQuery->_pDecoys[j].sAScoreProSiteScores.clear();
            pQuery->_pDecoys[j].pWhichProtein.clear();
            pQuery->_pDecoys[j].sPeffOrigResidues.clear();
            pQuery->_pDecoys[j].iPeffOrigResiduePosition = -9;
         }
      }
   }

   return true;
}


bool RunSearchAndPostAnalysis(int iPercentStart, int iPercentEnd,
                              ThreadPool* tp, SearchSession& session,
                              bool bLogPrePostAnalysis)
{
   if (g_staticParams.options.bMango)
   {
      int iCurrentScanNumber = 0;
      int iMangoIndex = 0;

      std::sort(session.queries.begin(), session.queries.end(), compareByMangoIndex);

      for (std::vector<Query*>::iterator it = session.queries.begin(); it != session.queries.end(); ++it)
      {
         if ((*it)->_spectrumInfoInternal.iScanNumber != iCurrentScanNumber)
         {
            iCurrentScanNumber = (*it)->_spectrumInfoInternal.iScanNumber;
            iMangoIndex = 0;
         }
         else
         {
            iMangoIndex++;
         }
         sprintf((*it)->_spectrumInfoInternal.szMango, "%03d_%c",
                 (int)iMangoIndex / 2, (iMangoIndex % 2) ? 'B' : 'A');
      }
   }

   std::sort(session.queries.begin(), session.queries.end(), compareByPeptideMass);

   g_massRange.dMinMass = session.queries.at(0)->_pepMassInfo.dPeptideMassToleranceMinus;
   g_massRange.dMaxMass = session.queries.at(session.queries.size() - 1)->_pepMassInfo.dPeptideMassTolerancePlus;
   g_massRange.bNarrowMassRange = (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass);

   bool bSucceeded = !session.statusRef.IsError() && !session.statusRef.IsCancel();
   if (!bSucceeded)
      return false;

   session.statusRef.SetStatusMsg(string("Running search..."));

   if (session.bPerformDatabaseSearch)
      bSucceeded = CometSearch::RunSearch(iPercentStart, iPercentEnd, tp, session.queries);
   if (bSucceeded && session.bPerformSpecLibSearch)
      bSucceeded = CometSearch::RunSpecLibSearch(iPercentStart, iPercentEnd, tp, session.queries);
   // TODO(batch-MS1): CometSearch::RunMS1Search(tp, dRT, dMaxMS1RTDiff, dMaxSpecLibRT,
   // dMaxQueryRT, session.ms1Queries) must be called here when the batch MS1 speclib
   // path is implemented.  It requires a second reader pass over the file at
   // iSpecLibMSLevel to populate session.ms1Queries, plus per-file RT range values
   // from CometSpecLib::LoadSpecLibMS1Raw.  Neither exists in the batch pipeline yet.

   if (!bSucceeded)
      return false;

   bSucceeded = !session.statusRef.IsError() && !session.statusRef.IsCancel();
   if (!bSucceeded)
      return false;

   if (bLogPrePostAnalysis && !g_staticParams.options.bOutputSqtStream)
   {
      logout("     - Post analysis:");
      fflush(stdout);
   }

   if (session.bPerformDatabaseSearch)
   {
      session.statusRef.SetStatusMsg(string("Performing post-search analysis ..."));
      bSucceeded = CometPostAnalysis::PostAnalysis(tp, session.queries);
   }

   return bSucceeded;
}


bool executeBatchLegacy(MSToolkit::MSReader& mstReader,
                        int iFirstScan, int iLastScan, int iAnalysisType,
                        int& iPercentStart, int& iPercentEnd,
                        ThreadPool* tp, SearchSession& session,
                        bool bVerbose)
{
   if (bVerbose && !g_staticParams.options.bOutputSqtStream)
   {
      logout("   - Load spectra:");
      fflush(stdout);
   }

   session.statusRef.SetStatusMsg(string("Loading and processing input spectra"));

   bool bSucceeded = CometPreprocess::LoadAndPreprocessSpectra(
         mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session);

   iPercentStart = iPercentEnd;
   iPercentEnd   = mstReader.getPercent();

   if (!bSucceeded)
      return false;

   if (session.queries.empty())
      return true;

   bSucceeded = AllocateResultsMem(session.queries);
   if (!bSucceeded)
      return false;

   {
      string strStatusMsg = " " + std::to_string(session.queries.size()) + string("\n");
      if (bVerbose && !g_staticParams.options.bOutputSqtStream)
         logout(strStatusMsg);
      session.statusRef.SetStatusMsg(strStatusMsg);
   }

   return RunSearchAndPostAnalysis(iPercentStart, iPercentEnd, tp, session, bVerbose);
}
