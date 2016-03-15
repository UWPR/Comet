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
#include "CometWriteOut.h"
#include "CometStatus.h"


CometWriteOut::CometWriteOut()
{
}


CometWriteOut::~CometWriteOut()
{
}


bool CometWriteOut::WriteOut(void)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      if (!PrintResults(i, false))
      {
         return false;
      }
   }

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         if (!PrintResults(i, true))
         {
            return false;
         }
      }
   }

   return true;
}


bool CometWriteOut::PrintResults(int iWhichQuery,
                                 bool bDecoySearch)
{
   int  i,
        ii,
        iNumPrintLines,
        iLenMaxDuplicates,
        iMaxWidthReference,
        iRankXcorr;
   char szDbLine[512],
        szBuf[SIZE_BUF],
        szStatsBuf[512],
        szMassLine[200],
        szOutput[SIZE_FILE],
        scan1[32],
        scan2[32];
   FILE *fpout;
   char *pStr;

   strcpy(scan1, "0");
   strcpy(scan2, "0");

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   sprintf(szMassLine, "(M+H)+ mass = %0.6f ~ %0.6f (+%d), fragment tol = %0.4f, binoffset = %0.3f",
         pQuery->_pepMassInfo.dExpPepMass,
         pQuery->_pepMassInfo.dPeptideMassTolerance,
         pQuery->_spectrumInfoInternal.iChargeState,
         g_staticParams.tolerances.dFragmentBinSize,
         g_staticParams.tolerances.dFragmentBinStartOffset);

   if (g_staticParams.massUtility.bMonoMassesParent)
      sprintf(szMassLine + strlen(szMassLine), ", MONO");
   else
      sprintf(szMassLine + strlen(szMassLine), ", AVG");

   if (g_staticParams.massUtility.bMonoMassesFragment)
      sprintf(szMassLine + strlen(szMassLine), "/MONO");
   else
      sprintf(szMassLine + strlen(szMassLine), "/AVG");

#ifdef _WIN32
    if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL)
#else
    if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
#endif
       pStr = g_staticParams.inputFile.szBaseName;
    else
       pStr++;  // skip separation character

   if (bDecoySearch)
   {
      sprintf(szOutput, "%s_decoy/%s.%.5d.%.5d.%d.out",
            g_staticParams.inputFile.szBaseName,
            pStr,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState);
   }
   else
   {
      sprintf(szOutput, "%s/%s.%.5d.%.5d.%d.out",
            g_staticParams.inputFile.szBaseName,
            pStr,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState);
   }

   if (g_staticParams.options.iWhichReadingFrame)
   {
      sprintf(szDbLine, "# bases = %lu (frame=%d), # proteins = %d, %s",
            g_staticParams.databaseInfo.uliTotAACount,
            g_staticParams.options.iWhichReadingFrame,
            g_staticParams.databaseInfo.iTotalNumProteins,
            g_staticParams.databaseInfo.szDatabase);
   }
   else
   {
      sprintf(szDbLine, "# amino acids = %lu, # proteins = %d, %s",
            g_staticParams.databaseInfo.uliTotAACount,
            g_staticParams.databaseInfo.iTotalNumProteins,
            g_staticParams.databaseInfo.szDatabase);
   }

   if ((fpout = fopen(szOutput, "w")) == NULL)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - cannot write to file %s.\n", szOutput);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   if (bDecoySearch)
   {
      sprintf(szStatsBuf, "total inten = %0.2E, lowest Sp = %0.1f, # matched peptides = %lu",
            pQuery->_spectrumInfoInternal.dTotalIntensity,
            pQuery->fLowestDecoySpScore,
            pQuery->_uliNumMatchedDecoyPeptides);
   }
   else
   {
      sprintf(szStatsBuf, "total inten = %0.2E, lowest Sp = %0.1f, # matched peptides = %lu",
            pQuery->_spectrumInfoInternal.dTotalIntensity,
            pQuery->fLowestSpScore,
            pQuery->_uliNumMatchedPeptides);
   }

   sprintf(szBuf, "\n");
   sprintf(szBuf+strlen(szBuf), " %s.%.5d.%.5d.%d.out\n",
         pStr,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iChargeState);
   sprintf(szBuf+strlen(szBuf), " Comet version %s\n", comet_version);
   sprintf(szBuf+strlen(szBuf), " %s\n", copyright);
   sprintf(szBuf+strlen(szBuf), " %s\n", g_staticParams.szOutFileTimeString);
   sprintf(szBuf+strlen(szBuf), " %s\n", szMassLine);
   sprintf(szBuf+strlen(szBuf), " %s\n", szStatsBuf);
   sprintf(szBuf+strlen(szBuf), " %s\n", szDbLine);
   sprintf(szBuf+strlen(szBuf), " %s\n", g_staticParams.szIonSeries);
   sprintf(szBuf+strlen(szBuf), " %s\n", g_staticParams.szDisplayLine);

   fprintf(fpout, "%s", szBuf);

   szBuf[0] = '\0';

   if (g_staticParams.szMod[0]!='\0')
   {
      sprintf(szBuf+strlen(szBuf), " %s", g_staticParams.szMod);
      sprintf(szBuf+strlen(szBuf), "\n");
   }

   fprintf(fpout, "%s\n", szBuf);

   if (bDecoySearch)
      iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
   else
      iNumPrintLines = pQuery->iMatchPeptideCount;

   // Print out each sequence line.
   if (iNumPrintLines > (g_staticParams.options.iNumPeptideOutputLines))
      iNumPrintLines = (g_staticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoySearch)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   iMaxWidthReference = 9;
   iLenMaxDuplicates = 0;
   for (i=0; i<iNumPrintLines; i++)
   {
      if (pOutput[i].iDuplicateCount > iLenMaxDuplicates)
         iLenMaxDuplicates = pOutput[i].iDuplicateCount;

      for (ii=0; ii<WIDTH_REFERENCE; ii++)
      {
         if (   (pOutput[i].szProtein[ii] == ' ')
             || (pOutput[i].szProtein[ii] == '\n')
             || (pOutput[i].szProtein[ii] == '\r')
             || (pOutput[i].szProtein[ii] == '\t')
             || (pOutput[i].szProtein[ii] == '\0'))
         {
            break;
         }
      }

      if (ii > iMaxWidthReference)
         iMaxWidthReference = ii;
   }

   if (iLenMaxDuplicates > 0)
   {
      char szTempStr[10];

      sprintf(szTempStr, " %+d", iLenMaxDuplicates);
      iLenMaxDuplicates = strlen(szTempStr);
   }

   if (g_staticParams.options.bPrintExpectScore)
      sprintf(szBuf, "  #   Rank/Sp    (M+H)+   deltCn   Xcorr   Expect   Ions  Reference");
   else
      sprintf(szBuf, "  #   Rank/Sp    (M+H)+   deltCn   Xcorr    Sp    Ions  Reference");

   for (i=0; i<iMaxWidthReference-9; i++)
      sprintf(szBuf+strlen(szBuf), " ");

   for (i=0; i<iLenMaxDuplicates; i++)
      sprintf(szBuf+strlen(szBuf), " ");

   sprintf(szBuf+strlen(szBuf), "  ");
   sprintf(szBuf+strlen(szBuf), "Peptide\n");

   if (g_staticParams.options.bPrintExpectScore)
      sprintf(szBuf+strlen(szBuf), " ---  -------  ---------  ------  ------   ------   ----  ---------");
   else
      sprintf(szBuf+strlen(szBuf), " ---  -------  ---------  ------  ------   ----   ----  ---------");

   for (i=0; i<iMaxWidthReference-9; i++)
      sprintf(szBuf+strlen(szBuf), " ");

   for (i=0; i<iLenMaxDuplicates; i++)
      sprintf(szBuf+strlen(szBuf), " ");

   sprintf(szBuf+strlen(szBuf), "  ");
   sprintf(szBuf+strlen(szBuf), "-------\n");

   fprintf(fpout, "%s", szBuf);
   szBuf[0]='\0';

   iRankXcorr = 1;

   for (i=0; i<iNumPrintLines; i++)
   {
      if ((i > 0) && !isEqual(pOutput[i].fXcorr, pOutput[i-1].fXcorr))
         iRankXcorr++;

      if (pOutput[i].fXcorr > XCORR_CUTOFF)
         PrintOutputLine(iRankXcorr, iLenMaxDuplicates, iMaxWidthReference, i, pOutput, fpout);
   }

   fprintf(fpout, "\n");

   // Print out the fragment ions for the selected ion series
   // and mark matched ions in the sp scoring routine.
   if (g_staticParams.options.bShowFragmentIons && iNumPrintLines > 0)
   {
      PrintIons(iWhichQuery, fpout);
   }

   // Print out expect score histogram.
   if (g_staticParams.options.bPrintExpectScore)
   {
      double dVal;
      double dExpect;

      if (bDecoySearch)
      {
         fprintf(fpout, "a=%f b=%f\n", pQuery->fPar[1], pQuery->fPar[0]);

         // iXcorrHistogram is already cummulative here.
         for (i=0; i<=pQuery->siMaxXcorr; i++)
         {
            if (pQuery->iXcorrHistogram[i]> 0)
            {
               dVal = pQuery->fPar[0] + pQuery->fPar[1] * i;
               dExpect = pow(10.0, dVal);

               if (dExpect > 999.0)
                  dExpect = 999.0;

               fprintf(fpout, "HIST:\t%0.1f\t%d\t%0.3f\t%0.3f\t%0.3f\n",
                     i*0.1,
                     pQuery->iXcorrHistogram[i],
                     log10((float)pQuery->iXcorrHistogram[i]),
                     dVal,
                     dExpect);
            }
         }
         fprintf(fpout, "\n");
      }
      else
      {
         fprintf(fpout, "a=%f b=%f %d-%d\n",
               pQuery->fPar[1],
               pQuery->fPar[0],
               (int)pQuery->fPar[2],
               (int)pQuery->fPar[3]);

         // iXcorrHistogram is already cummulative here.
         for (i=0; i<=pQuery->siMaxXcorr; i++)
         {
            if (pQuery->iXcorrHistogram[i]> 0)
            {
               dVal = pQuery->fPar[0] + pQuery->fPar[1] * i;
               dExpect = pow(10.0, dVal);

               if (dExpect > 999.0)
                  dExpect = 999.0;

               fprintf(fpout, "HIST:\t%0.1f\t%d\t%0.3f\t%0.3f\t%0.3f\n",
                     i*0.1,
                     pQuery->iXcorrHistogram[i],
                     log10((float)pQuery->iXcorrHistogram[i]),
                     dVal,
                     dExpect);
            }
         }
         fprintf(fpout, "\n");
      }
   }
   fclose(fpout);

   return true;
}


void CometWriteOut::PrintOutputLine(int iRankXcorr,
                                    int iLenMaxDuplicates,
                                    int iMaxWidthReference,
                                    int iWhichResult,
                                    Results *pOutput,
                                    FILE *fpout)
{
   int  i,
        iWidthSize,
        iWidthPrintRef;
   char szBuf[SIZE_BUF];

   double dDeltaCn;

   if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult].fXcorr >= 0.0)
      dDeltaCn = 1.0 - pOutput[iWhichResult].fXcorr/pOutput[0].fXcorr;
   else if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult].fXcorr < 0.0)
      dDeltaCn = 1.0;
   else
      dDeltaCn = 0.0;

   sprintf(szBuf, "%3d. %3d /%3d  %9.4f  %6.4f %7.4f ",
         iWhichResult+1,
         iRankXcorr,
         pOutput[iWhichResult].iRankSp,
         pOutput[iWhichResult].dPepMass,
         dDeltaCn,
         pOutput[iWhichResult].fXcorr);

   if (g_staticParams.options.bPrintExpectScore)
   {
      if (pOutput[iWhichResult].dExpect < 0.0001)
         sprintf(szBuf+strlen(szBuf), "%8.2e ", pOutput[iWhichResult].dExpect);
      else
         sprintf(szBuf+strlen(szBuf), "%8.4f ", pOutput[iWhichResult].dExpect);
   }
   else
   {
      sprintf(szBuf+strlen(szBuf), "%6.1f ", pOutput[iWhichResult].fScoreSp);
   }

   if (pOutput[iWhichResult].iTotalIons < 10)
   {
      sprintf(szBuf+strlen(szBuf), "%3d/%d   ", pOutput[iWhichResult].iMatchedIons, pOutput[iWhichResult].iTotalIons);
   }
   else if (pOutput[iWhichResult].iTotalIons < 100)
   {
      sprintf(szBuf+strlen(szBuf), "%3d/%2d  ", pOutput[iWhichResult].iMatchedIons, pOutput[iWhichResult].iTotalIons);
   }
   else
   {
      sprintf(szBuf+strlen(szBuf), "%3d/%3d ", pOutput[iWhichResult].iMatchedIons, pOutput[iWhichResult].iTotalIons);
   }

   // Print protein reference/accession.
   iWidthSize=0;
   iWidthPrintRef=0;
   while (    (pOutput[iWhichResult].szProtein[iWidthSize] != ' ')
           && (pOutput[iWhichResult].szProtein[iWidthSize] != '\n')
           && (pOutput[iWhichResult].szProtein[iWidthSize] != '\r')
           && (pOutput[iWhichResult].szProtein[iWidthSize] != '\t')
           && (pOutput[iWhichResult].szProtein[iWidthSize] != '\0'))
   {
      if (33<=(pOutput[iWhichResult].szProtein[iWidthSize]) // Ascii physical character range.
            && (pOutput[iWhichResult].szProtein[iWidthSize])<=126)
      {
         sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szProtein[iWidthPrintRef]);

         iWidthPrintRef++;

         if (iWidthPrintRef == iMaxWidthReference)
            break;
      }
      else
         break;
      iWidthSize++;
   }

   if (iWidthSize < iMaxWidthReference)
   {
      iWidthSize = iMaxWidthReference-iWidthPrintRef;

      for (i=0; i<iWidthSize; i++)
         sprintf(szBuf+strlen(szBuf), " ");
   }

   // Print out the number of duplicate proteins.
   if (pOutput[iWhichResult].iDuplicateCount)
   {
      char szTemp[10];
      int iEnd;

      sprintf(szTemp, "+%d", pOutput[iWhichResult].iDuplicateCount);
      sprintf(szBuf+strlen(szBuf), " +%d", pOutput[iWhichResult].iDuplicateCount);

      iEnd = iLenMaxDuplicates - strlen(szTemp) - 1;

      for (i=0; i<iEnd; i++)
         sprintf(szBuf+strlen(szBuf), " ");
   }
   else if (iLenMaxDuplicates > 0)
   {
      for (i=0; i<iLenMaxDuplicates; i++)
         sprintf(szBuf+strlen(szBuf), " ");
   }
   sprintf(szBuf+strlen(szBuf), "  ");

   sprintf(szBuf+strlen(szBuf), "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      sprintf(szBuf+strlen(szBuf),
            "n%c", g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1]);
   }

   // Print peptide sequence.
   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      sprintf(szBuf+strlen(szBuf), "%c", (int)pOutput[iWhichResult].szPeptide[i]);

      if (g_staticParams.variableModParameters.bVarModSearch
            && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass, 0.0))
      {
         sprintf(szBuf+strlen(szBuf), "%c",
               (int)g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[i]-1]);
      }
   }

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 1)
   {
      sprintf(szBuf+strlen(szBuf),
            "c%c", g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1]);
   }

   sprintf(szBuf+strlen(szBuf), ".%c", pOutput[iWhichResult].szPrevNextAA[1]);

   fprintf(fpout, "%s\n", szBuf);
}


// Print out fragment ions at end of .out files.
void CometWriteOut::PrintIons(int iWhichQuery,
                              FILE *fpout)
{
   int  i,
        ii,
        ctCharge;
   char szBuf[SIZE_BUF];
   double dFragmentIonMass = 0.0;
   double _pdAAforward[MAX_PEPTIDE_LEN];
   double _pdAAreverse[MAX_PEPTIDE_LEN];

   double dBion = g_staticParams.precalcMasses.dNtermProton;
   double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   if (pQuery->_pResults[0].szPrevNextAA[0] == '-')
      dBion += g_staticParams.staticModifications.dAddNterminusProtein;
   if (pQuery->_pResults[0].szPrevNextAA[1] == '-')
      dYion += g_staticParams.staticModifications.dAddCterminusProtein;

   if (g_staticParams.variableModParameters.bVarModSearch
         && (pQuery->_pResults[0].pcVarModSites[pQuery->_pResults[0].iLenPeptide] == 1))
   {
      dBion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].pcVarModSites[pQuery->_pResults[0].iLenPeptide]-1].dVarModMass;
   }

   if (g_staticParams.variableModParameters.bVarModSearch
         && (pQuery->_pResults[0].pcVarModSites[pQuery->_pResults[0].iLenPeptide + 1] == 1))
   {
      dYion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].pcVarModSites[pQuery->_pResults[0].iLenPeptide+1]-1].dVarModMass;
   }

   // Generate pdAAforward for pQuery->_pResults[0].szPeptide.
   for (i=0; i<pQuery->_pResults[0].iLenPeptide; i++)
   {
      int iPos = pQuery->_pResults[0].iLenPeptide - i - 1;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[0].szPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[0].szPeptide[iPos]];

      if (g_staticParams.variableModParameters.bVarModSearch)
         dBion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].pcVarModSites[i]-1].dVarModMass;

      if (g_staticParams.variableModParameters.bVarModSearch)
         dYion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].pcVarModSites[iPos]-1].dVarModMass;

      _pdAAforward[i] = dBion;
      _pdAAreverse[iPos] = dYion;
   }

   for (ctCharge=1; ctCharge<=pQuery->_spectrumInfoInternal.iMaxFragCharge; ctCharge++)
   {
      if (ctCharge > 1)
         sprintf(szBuf, "\n");

      sprintf(szBuf, "\n Seq  #  ");

      for (i=0; i<g_staticParams.ionInformation.iNumIonSeriesUsed; i++)
      {
         int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[i];

         char cAA=(iWhichIonSeries==0?'a':
                     (iWhichIonSeries==1?'b':
                        (iWhichIonSeries==2?'c':
                           (iWhichIonSeries==3?'d':
                              (iWhichIonSeries==4?'v':
                                 (iWhichIonSeries==5?'w':
                                    (iWhichIonSeries==6?'x':
                                       (iWhichIonSeries==7?'y':'z'))))))));
         sprintf(szBuf+strlen(szBuf), "    %c      ", cAA);
      }
      sprintf(szBuf+strlen(szBuf), "(+%d)\n --- --  ", ctCharge-1);

      for (i=0; i<g_staticParams.ionInformation.iNumIonSeriesUsed; i++)
      {
         sprintf(szBuf+strlen(szBuf), "---------  ");
      }

      fprintf(fpout, "%s --", szBuf);

      for (i=0; i<pQuery->_pResults[0].iLenPeptide; i++)
      {
         sprintf(szBuf, "\n  %c  %2d  ", pQuery->_pResults[0].szPeptide[i], i+1);

         for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ii++)
         {
            int iWhichIonSeries;

            iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ii];

            dFragmentIonMass = CometMassSpecUtils::GetFragmentIonMass(iWhichIonSeries, i, ctCharge, _pdAAforward, _pdAAreverse);

            if ((dFragmentIonMass <= FLOAT_ZERO)
                  || ((i == pQuery->_pResults[0].iLenPeptide - 1) && (iWhichIonSeries <= 3))
                  || (i == 0 && (iWhichIonSeries >= 4)))
            {
               sprintf(szBuf+strlen(szBuf), "    -      ");
            }
            else
            {
               sprintf(szBuf+strlen(szBuf), "%9.4f", dFragmentIonMass);

               if (FindSpScore(pQuery, BIN(dFragmentIonMass)) > FLOAT_ZERO)
                  sprintf(szBuf+strlen(szBuf), "+ ");
               else
                  sprintf(szBuf+strlen(szBuf), "  ");
            }
         }
         fprintf(fpout, "%s %2d", szBuf, pQuery->_pResults[0].iLenPeptide-i);

      }
      fprintf(fpout, "\n");

   }
   fprintf(fpout, "\n");
}


float CometWriteOut::FindSpScore(Query *pQuery,
                                 int bin)
{
   int x = bin / 10;

   if (pQuery->ppfSparseSpScoreData[x] == NULL)
      return 0.0f;

   int y = bin - (x*10);
   return pQuery->ppfSparseSpScoreData[x][y];
}
