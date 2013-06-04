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
#include "CometWriteTxt.h"

bool CometWriteTxt::_bWroteHeader = false;

CometWriteTxt::CometWriteTxt()
{
}


CometWriteTxt::~CometWriteTxt()
{
}


void CometWriteTxt::WriteTxt(FILE *fpout,
                             FILE *fpoutd,
                             char *szOutput,
                             char *szOutputDecoy)
{
   int i;

   if (!_bWroteHeader)
   {
      _bWroteHeader = true;
      PrintTxtHeader(fpout);

      if (g_StaticParams.options.iDecoySearch == 2)
         PrintTxtHeader(fpoutd);
   }

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
      PrintResults(i, 0, fpout, szOutput);

   // Print out the separate decoy hits.
   if (g_StaticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
         PrintResults(i, 1, fpoutd, szOutputDecoy);
   }
}


void CometWriteTxt::PrintTxtHeader(FILE *fpout)
{
   char szEndDate[28];
   time_t tTime;

   fprintf(fpout, "##\t%s\t", g_StaticParams.inputFile.szBaseName);
   fprintf(fpout, "CometVersion %s\t", comet_version);
   fprintf(fpout, "%s\t", g_StaticParams._dtInfoStart.szDate);
   time(&tTime);
   strftime(szEndDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tTime));
   fprintf(fpout, "%s\t", szEndDate);
   fprintf(fpout, "%s\n", g_StaticParams.databaseInfo.szDatabase);
}


void CometWriteTxt::PrintResults(int iWhichQuery,
                                 bool bDecoy,
                                 FILE *fpout,
                                 char *szOutput)
{
   char szBuf[SIZE_BUF];

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   sprintf(szBuf, "%d\t%d\t%0.4f\t",
         pQuery->_spectrumInfoInternal.iScanNumber, 
         pQuery->_spectrumInfoInternal.iChargeState, 
         pQuery->_pepMassInfo.dExpPepMass);

   fprintf(fpout, "%s", szBuf);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   if (pOutput[0].fXcorr > 0.0)
      PrintTxtLine(iWhichQuery, 1, 0, bDecoy, pOutput, fpout);  // print top hit only right now
   else
      fprintf(fpout, "\n");
}


void CometWriteTxt::PrintTxtLine(int iWhichQuery,
                                 int iRankXcorr,
                                 int iWhichResult,
                                 bool bDecoy,
                                 Results *pOutput,
                                 FILE *fpout)
{
   int  i;
   char szBuf[SIZE_BUF];

   sprintf(szBuf, "%0.4f\t%0.2E\t%0.4f\t%0.4f\t%0.1f\t%d/%d\t",
         pOutput[iWhichResult].dPepMass,
         pOutput[iWhichResult].dExpect,
         pOutput[iWhichResult].fXcorr,
         1.000000 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr,   // pOutput[0].fXcorr is >0 to enter this fn
         pOutput[iWhichResult].fScoreSp,
         pOutput[iWhichResult].iMatchedIons, 
         pOutput[iWhichResult].iTotalIons);

   fprintf(fpout, "%s\t", szBuf);

   // Print plain peptide
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

   // Print peptide sequence
   sprintf(szBuf, "%c", pOutput[iWhichResult].szPrevNextAA[0]);

   if (g_StaticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
   {
      sprintf(szBuf+strlen(szBuf), "]");
   }
   else
      sprintf(szBuf+strlen(szBuf), ".");


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

   if (g_StaticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
   {
      sprintf(szBuf+strlen(szBuf), "[");
   }
   else
      sprintf(szBuf+strlen(szBuf), ".");

   sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPrevNextAA[1]);

   fprintf(fpout, "%s\t", szBuf);

   // Print protein reference/accession.
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szProtein);

   fprintf(fpout, "%+u", pOutput[iWhichResult].iDuplicateCount); 

   fprintf(fpout, "\n");
}
