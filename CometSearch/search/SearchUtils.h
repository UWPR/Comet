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

#pragma once

#include "Common.h"
#include "CometDataInternal.h"

// Shared inline utilities used by Pipeline and strategy classes.
// All functions operate on globals (g_staticParams, g_cometStatus, etc.)
// which are declared in CometDataInternal.h / Common.h.

// -----------------------------------------------------------------------
// Input type detection (file extension -> InputType enum)
// -----------------------------------------------------------------------
inline static InputType GetInputType(const char* pszFileName)
{
   int iLen = (int)strlen(pszFileName);

   if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 6, ".mzXML")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 5, ".mzML")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 9, ".mzXML.gz")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 8, ".mzML.gz"))
   {
      return InputType_MZXML;
   }
   else if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 4, ".raw"))
   {
      return InputType_RAW;
   }
   else if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 4, ".ms2")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 5, ".cms2"))
   {
      return InputType_MS2;
   }
   else if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 4, ".mgf"))
   {
      return InputType_MGF;
   }

   return InputType_UNKNOWN;
}

// -----------------------------------------------------------------------
// UpdateInputFile: sets g_staticParams.inputFile from pFileInfo.
// Returns false on unknown type or if file cannot be opened.
// -----------------------------------------------------------------------
inline static bool UpdateInputFile(InputFileInfo* pFileInfo)
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

      if (!STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 9, ".mzXML.gz")
            || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 8, ".mzML.gz"))
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

// -----------------------------------------------------------------------
// SetMSLevelFilter: configure MSReader to read the right MS level.
// -----------------------------------------------------------------------
inline static void SetMSLevelFilter(MSReader& mstReader)
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

// -----------------------------------------------------------------------
// AllocateResultsMem: allocate _pResults (and optionally _pDecoys) for
// every Query* in the batch, and zero-initialize scoring fields.
// -----------------------------------------------------------------------
inline static bool AllocateResultsMem(std::vector<Query*>& queries)
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
         memset(pQuery->iXcorrHistogram, 0, sizeof(pQuery->iXcorrHistogram));

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

// -----------------------------------------------------------------------
// Query sort comparators
// -----------------------------------------------------------------------
inline static bool compareByPeptideMass(Query const* a, Query const* b)
{
   return (a->_pepMassInfo.dExpPepMass < b->_pepMassInfo.dExpPepMass);
}

inline static bool compareByMangoIndex(Query const* a, Query const* b)
{
   return (a->dMangoIndex < b->dMangoIndex);
}

inline static bool compareByScanNumber(Query const* a, Query const* b)
{
   if (a->_spectrumInfoInternal.iScanNumber == b->_spectrumInfoInternal.iScanNumber)
      return (a->_spectrumInfoInternal.usiChargeState < b->_spectrumInfoInternal.usiChargeState);
   return (a->_spectrumInfoInternal.iScanNumber < b->_spectrumInfoInternal.iScanNumber);
}
