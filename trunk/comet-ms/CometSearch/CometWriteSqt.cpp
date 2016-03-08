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
#include "CometDataInternal.h"
#include "CometMassSpecUtils.h"
#include "CometWriteSqt.h"
#include "CometSearchManager.h"


CometWriteSqt::CometWriteSqt()
{
}

CometWriteSqt::~CometWriteSqt()
{
}


void CometWriteSqt::WriteSqt(FILE *fpout,
                             FILE *fpoutd)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      PrintResults(i, 0, fpout);
   }

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, 1, fpoutd);
      }
   }
}


void CometWriteSqt::PrintSqtHeader(FILE *fpout,
                                   CometSearchManager &searchMgr)
{
   char szEndDate[28];
   time_t tTime;

   fprintf(fpout, "H\tSQTGenerator\tComet\n");
   fprintf(fpout, "H\tComment\tCometVersion %s\n", comet_version);
   fprintf(fpout, "H\n");
   fprintf(fpout, "H\tStartTime\t%s\n", g_staticParams.szDate);
   time(&tTime);
   strftime(szEndDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tTime));
   fprintf(fpout, "H\tEndTime\t%s\n", szEndDate);
   fprintf(fpout, "H\n");

   fprintf(fpout, "H\tDBSeqLength\t%lu\n", g_staticParams.databaseInfo.uliTotAACount);
   fprintf(fpout, "H\tDBLocusCount\t%d\n", g_staticParams.databaseInfo.iTotalNumProteins);

   fprintf(fpout, "H\tFragmentMassMode\tAMU\n");
   fprintf(fpout, "H\tPrecursorMassMode\t%s\n",
         (g_staticParams.tolerances.iMassToleranceUnits==0?"AMU":
          (g_staticParams.tolerances.iMassToleranceUnits==1?"MMU":"PPM")));
   fprintf(fpout, "H\tSQTGeneratorVersion\tN/A\n");
   fprintf(fpout, "H\tDatabase\t%s\n", g_staticParams.databaseInfo.szDatabase );
   fprintf(fpout, "H\tFragmentMasses\t%s\n", g_staticParams.massUtility.bMonoMassesFragment?"MONO":"AVG");
   fprintf(fpout, "H\tPrecursorMasses\t%s\n", g_staticParams.massUtility.bMonoMassesParent?"MONO":"AVG");
   fprintf(fpout, "H\tPrecursorMassTolerance\t%0.6f\n", g_staticParams.tolerances.dInputTolerance);
   fprintf(fpout, "H\tFragmentMassTolerance\t%0.6f\n", g_staticParams.tolerances.dFragmentBinSize);
   fprintf(fpout, "H\tEnzymeName\t%s\n", g_staticParams.enzymeInformation.szSearchEnzymeName);
   fprintf(fpout, "H\tAlgo-IonSeries\t 0 0 0 %0.1f %0.1f %0.1f 0.0 0.0 0.0 %0.1f %0.1f %0.1f\n",
         (double)g_staticParams.ionInformation.iIonVal[ION_SERIES_A],
         (double)g_staticParams.ionInformation.iIonVal[ION_SERIES_B],
         (double)g_staticParams.ionInformation.iIonVal[ION_SERIES_C],
         (double)g_staticParams.ionInformation.iIonVal[ION_SERIES_X],
         (double)g_staticParams.ionInformation.iIonVal[ION_SERIES_Y],
         (double)g_staticParams.ionInformation.iIonVal[ION_SERIES_Z]);

   for (int i=0; i<VMODS; i++)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='\0'))
      {
         int iLen = strlen(g_staticParams.variableModParameters.varModList[i].szVarModChar);

         for (int ii=0; ii<iLen; ii++)
         {
            fprintf(fpout, "H\tDiffMod\t%c%c=%+0.6f\n",
                  g_staticParams.variableModParameters.varModList[i].szVarModChar[ii],
                  g_staticParams.variableModParameters.cModCode[i],
                  g_staticParams.variableModParameters.varModList[i].dVarModMass);
         }
      }
   }

   char *pStr;

   // Since this process of parsing the static mods out is destructive, do it to copy
   // in case decoy search output is needed which uses same string
   char szMod[512];
   strcpy(szMod, g_staticParams.szMod);
   while ((pStr = strrchr(szMod, '='))!=NULL)
   {
      char szTmp[48];

      while (*pStr != ' ' && pStr!=szMod)
         *pStr--;

      if (pStr==szMod)
         sscanf(pStr, "%47s", szTmp);
      else
         sscanf(pStr+1, "%47s", szTmp); // skip the space

      fprintf(fpout, "H\tStaticMod\t%s\n", szTmp);

      *pStr = '\0';
   }

   fprintf(fpout, "H\n");

   std::map<std::string, CometParam*> mapParams = searchMgr.GetParamsMap();
   for (std::map<std::string, CometParam*>::iterator it=mapParams.begin(); it!=mapParams.end(); ++it)
   {
      if (it->first != "[COMET_ENZYME_INFO]")
      {
         fprintf(fpout, "H\tCometParams\t%s = %s\n", it->first.c_str(), it->second->GetStringValue().c_str());
      }
   }

   std::map<string, CometParam*>::iterator it;
   it = mapParams.find("[COMET_ENZYME_INFO]");
   if (it != mapParams.end())
   {
      fprintf(fpout, "H\tCometParams\n");
      fprintf(fpout, "H\tCometParams\t%s\n", it->first.c_str());
      string strEnzymeInfo = it->second->GetStringValue();
      std::size_t found = strEnzymeInfo.find_first_of("\n");
      while (found!=std::string::npos)
      {
         string strEnzymeInfoLine = strEnzymeInfo.substr(0, found);
         fprintf(fpout, "H\tCometParams\t%s\n", strEnzymeInfoLine.c_str());
         strEnzymeInfo =  strEnzymeInfo.substr(found+1);
         found = strEnzymeInfo.find_first_of("\n");
      }
   }

   fprintf(fpout, "H\n");
}


void CometWriteSqt::PrintResults(int iWhichQuery,
                                 bool bDecoy,
                                 FILE *fpout)
{
   int  i,
        iNumPrintLines,
        iRankXcorr;
   char szBuf[SIZE_BUF],
        scan1[32],
        scan2[32];

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   strcpy(scan1, "0");
   strcpy(scan2, "0");

   if (bDecoy)
   {
      sprintf(szBuf, "S\t%d\t%d\t%d\t%d\t%s\t%0.6f\t%0.2E\t%0.1f\t%lu\n",
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState,
            g_staticParams.iElapseTime,
            g_staticParams.szHostName,
            pQuery->_pepMassInfo.dExpPepMass,
            pQuery->_spectrumInfoInternal.dTotalIntensity,
            pQuery->fLowestDecoySpScore,
            pQuery->_uliNumMatchedDecoyPeptides);
   }
   else
   {
      sprintf(szBuf, "S\t%d\t%d\t%d\t%d\t%s\t%0.6f\t%0.2E\t%0.1f\t%lu\n",
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState,
            g_staticParams.iElapseTime,
            g_staticParams.szHostName,
            pQuery->_pepMassInfo.dExpPepMass,
            pQuery->_spectrumInfoInternal.dTotalIntensity,
            pQuery->fLowestSpScore,
            pQuery->_uliNumMatchedPeptides);
   }

   if (g_staticParams.options.bOutputSqtStream)
      fprintf(stdout, "%s", szBuf);
   else
      fprintf(fpout, "%s", szBuf);

   if (bDecoy)
      iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
   else
      iNumPrintLines = pQuery->iMatchPeptideCount;

   // Print out each sequence line.
   if (iNumPrintLines > (g_staticParams.options.iNumPeptideOutputLines))
      iNumPrintLines = (g_staticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   iRankXcorr = 1;

   for (i=0; i<iNumPrintLines; i++)
   {
      if ((i > 0) && !isEqual(pOutput[i].fXcorr, pOutput[i-1].fXcorr))
         iRankXcorr++;

      if (pOutput[i].fXcorr > XCORR_CUTOFF)
         PrintSqtLine(iRankXcorr, i, pOutput, fpout);
   }
}


void CometWriteSqt::PrintSqtLine(int iRankXcorr,
                                 int iWhichResult,
                                 Results *pOutput,
                                 FILE *fpout)
{
   int  i;
   char szBuf[SIZE_BUF];
   double dDeltaCn;

   if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult].fXcorr >= 0.0)
      dDeltaCn = 1.0 - pOutput[iWhichResult].fXcorr/pOutput[0].fXcorr;
   else if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult].fXcorr < 0.0)
      dDeltaCn = 1.0;
   else
      dDeltaCn = 0.0;

   sprintf(szBuf, "M\t%d\t%d\t%0.6f\t%0.4f\t%0.4f\t",
         iRankXcorr,
         pOutput[iWhichResult].iRankSp,
         pOutput[iWhichResult].dPepMass,
         dDeltaCn,
         pOutput[iWhichResult].fXcorr);

   if (g_staticParams.options.bPrintExpectScore)
      sprintf(szBuf+strlen(szBuf), "%0.2E", pOutput[iWhichResult].dExpect);
   else
      sprintf(szBuf+strlen(szBuf), "%0.1f", pOutput[iWhichResult].fScoreSp);

   sprintf(szBuf + strlen(szBuf), "\t%d\t%d\t",
         pOutput[iWhichResult].iMatchedIons,
         pOutput[iWhichResult].iTotalIons);

   sprintf(szBuf+strlen(szBuf), "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      sprintf(szBuf+strlen(szBuf), "n%c",
            g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1]);
   }

   // Print peptide sequence.
   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPeptide[i]);

      if (g_staticParams.variableModParameters.bVarModSearch
            && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass, 0.0))
      {
         sprintf(szBuf+strlen(szBuf), "%c",
               g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[i]-1]);
      }
   }

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 1)
   {
      sprintf(szBuf+strlen(szBuf), "c%c",
            g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1]);
   }

   sprintf(szBuf+strlen(szBuf), ".%c", pOutput[iWhichResult].szPrevNextAA[1]);

   if (g_staticParams.options.bOutputSqtStream)
   {
      fprintf(stdout, "%s\tU\n", szBuf);

      // Print protein reference/accession.
      fprintf(stdout, "L\t%s", pOutput[iWhichResult].szProtein);

      if (pOutput[iWhichResult].iDuplicateCount > 0)
         fprintf(stdout, "\t+%d", pOutput[iWhichResult].iDuplicateCount);

      fprintf(stdout, "\n");
   }
   else // OutputSqtFile
   {
      fprintf(fpout, "%s\tU\n", szBuf);

      // Print protein reference/accession.
      fprintf(fpout, "L\t%s", pOutput[iWhichResult].szProtein);

      if (pOutput[iWhichResult].iDuplicateCount > 0)
         fprintf(fpout, "\t+%d", pOutput[iWhichResult].iDuplicateCount);

      fprintf(fpout, "\n");
   }
}
