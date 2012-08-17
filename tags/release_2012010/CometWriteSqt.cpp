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
#include "CometData.h"
#include "CometMassSpecUtils.h"
#include "CometWriteSqt.h"


CometWriteSqt::CometWriteSqt()
{
}


CometWriteSqt::~CometWriteSqt()
{
}


void CometWriteSqt::WriteSqt(FILE *fpout,
                             FILE *fpoutd,
                             char *szOutput,
                             char *szOutputDecoy,
                             char *szParamsFile)
{
   int i;

   if (g_StaticParams.options.bOutputSqtFile)
   {
      PrintSqtHeader(fpout, szParamsFile);

      if (g_StaticParams.options.iDecoySearch == 2)
      {
         // Print this header only if separate decoy search is also run.
         fprintf(fpout, "H\tTargetSearchResults\nH\n");

         PrintSqtHeader(fpoutd, szParamsFile);
         fprintf(fpoutd, "H\tDecoySearchResults\nH\n");
      }
   }

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      PrintResults(i, 0, fpout, szOutput);
   }

   // Print out the separate decoy hits.
   if (g_StaticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, 1, fpoutd, szOutputDecoy);
      }
   }
}


void CometWriteSqt::PrintSqtHeader(FILE *fpout,
                                   char *szParamsFile)
{
   char szParamBuf[SIZE_BUF];
   char szEndDate[28];
   time_t tTime;
   FILE *fp;

   fprintf(fpout, "H\tSQTGenerator Comet\n");
   fprintf(fpout, "H\tComment CometVersion\t%s\n", version);
   fprintf(fpout, "H\n");
   fprintf(fpout, "H\tStartTime %s\n", g_StaticParams._dtInfoStart.szDate);
   time(&tTime);
   strftime(szEndDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tTime));
   fprintf(fpout, "H\tEndTime %s\n", szEndDate);
   fprintf(fpout, "H\n");

   // write out entire parameters file to SQT header
   if ((fp=fopen(szParamsFile, "r")) == NULL)
   {
      fprintf(stderr, " Error - cannot open parameter file %s.\n\n", szParamsFile);
      exit(1);
   }

   fprintf(fpout, "H\tDBSeqLength\t%lu\n", g_StaticParams.databaseInfo.liTotAACount);
   fprintf(fpout, "H\tDBLocusCount\t%d\n", g_StaticParams.databaseInfo.iTotalNumProteins);

   fprintf(fpout, "H\n");
   while (fgets(szParamBuf, SIZE_BUF, fp))
      fprintf(fpout, "H\tCometParams\t%s", szParamBuf);
   fprintf(fpout, "H\n");

   fclose(fp);
}


void CometWriteSqt::PrintResults(int iWhichQuery,
                                 bool bDecoy,
                                 FILE *fpout,
                                 char *szOutput)
{
   int  i,
        iDoXcorrCount,
        iRankXcorr;
   char szBuf[SIZE_BUF],
        scan1[32],
        scan2[32];

   strcpy(scan1, "0");
   strcpy(scan2, "0");

   if (bDecoy)
   {
      sprintf(szBuf, "S\t%d\t%d\t%d\t%d\t%s\t%0.6f\t%0.2E\t%0.1f\t%ld\n",
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber, 
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState, 
            g_StaticParams.iElapseTime, 
            g_StaticParams.szHostName, 
            g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.dTotalIntensity, 
            g_pvQuery.at(iWhichQuery)->fLowestDecoySpScore, 
            g_pvQuery.at(iWhichQuery)->_liNumMatchedDecoyPeptides);
   }
   else
   {
      sprintf(szBuf, "S\t%d\t%d\t%d\t%d\t%s\t%0.6f\t%0.2E\t%0.1f\t%ld\n",
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber, 
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState, 
            g_StaticParams.iElapseTime, 
            g_StaticParams.szHostName, 
            g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.dTotalIntensity, 
            g_pvQuery.at(iWhichQuery)->fLowestSpScore, 
            g_pvQuery.at(iWhichQuery)->_liNumMatchedPeptides);
   }

   if (g_StaticParams.options.bOutputSqtStream)
      fprintf(stdout, "%s", szBuf); 
   else
      fprintf(fpout, "%s", szBuf);

   if (bDecoy)
      iDoXcorrCount = g_pvQuery.at(iWhichQuery)->iDoDecoyXcorrCount;
   else
      iDoXcorrCount = g_pvQuery.at(iWhichQuery)->iDoXcorrCount;

   // Print out each sequence line.
   if (iDoXcorrCount > (g_StaticParams.options.iNumPeptideOutputLines))
      iDoXcorrCount = (g_StaticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoy)
      pOutput = g_pvQuery.at(iWhichQuery)->_pDecoys;
   else
      pOutput = g_pvQuery.at(iWhichQuery)->_pResults;

   iRankXcorr = 1;

   for (i=0; i<iDoXcorrCount; i++)
   {
      if ((i > 0) && (pOutput[i].fXcorr != pOutput[i-1].fXcorr))
         iRankXcorr++;

      if (pOutput[i].fXcorr > 0)
         PrintSqtLine(iWhichQuery, iRankXcorr, i, bDecoy, pOutput, fpout);
   } 
}


void CometWriteSqt::PrintSqtLine(int iWhichQuery,
                                 int iRankXcorr,
                                 int iWhichResult,
                                 bool bDecoy,
                                 Results *pOutput,
                                 FILE *fpout)
{
   int  i;
   char szBuf[SIZE_BUF];

   sprintf(szBuf, "M\t%d\t%d\t%0.4f\t%0.4f\t%0.4f\t",
         iRankXcorr,
         pOutput[iWhichResult].iRankSp,
         pOutput[iWhichResult].dPepMass,
         1.000000 - pOutput[iWhichResult].fXcorr/pOutput[0].fXcorr,
         pOutput[iWhichResult].fXcorr);

   if (g_StaticParams.options.bPrintExpectScore)
      sprintf(szBuf+strlen(szBuf), "%0.2E", pOutput[iWhichResult].dExpect);
   else
      sprintf(szBuf+strlen(szBuf), "%0.1f", pOutput[iWhichResult].fScoreSp);

   sprintf(szBuf + strlen(szBuf), "\t%d\t%d\t",
         pOutput[iWhichResult].iMatchedIons, 
         pOutput[iWhichResult].iTotalIons);

   sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPrevNextAA[0]);

   if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
      sprintf(szBuf+strlen(szBuf), "]");
   else
      sprintf(szBuf+strlen(szBuf), ".");

   // Print peptide sequence.
   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPeptide[i]);

      if (g_StaticParams.variableModParameters.bVarModSearch
            && 0.0 != g_StaticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass)
      {
         sprintf(szBuf+strlen(szBuf), "%c",
               g_StaticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[i]-1]);
      }
   }

   if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
      sprintf(szBuf+strlen(szBuf), "[");
   else
      sprintf(szBuf+strlen(szBuf), ".");

   sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPrevNextAA[1]);

   if (g_StaticParams.options.bOutputSqtStream)
   {
      fprintf(stdout, "%s\tU\n", szBuf);

      // Print protein reference/accession.
      fprintf(stdout, "L\t%s", pOutput[iWhichResult].szProtein);

      if (pOutput[iWhichResult].iDuplicateCount > 0)
         fprintf(stdout, "\t%+d", pOutput[iWhichResult].iDuplicateCount); 

      fprintf(stdout, "\n");
   }
   else // OutputSqtFile
   {
      fprintf(fpout, "%s\tU\n", szBuf);

      // Print protein reference/accession.
      fprintf(fpout, "L\t%s", pOutput[iWhichResult].szProtein);

      if (pOutput[iWhichResult].iDuplicateCount > 0)
         fprintf(fpout, "\t%+d", pOutput[iWhichResult].iDuplicateCount); 

      fprintf(fpout, "\n");
   }
}
