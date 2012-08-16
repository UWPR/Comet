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
#include "CometWriteOut.h"



CometWriteOut::CometWriteOut()
{
}


CometWriteOut::~CometWriteOut()
{
}


void CometWriteOut::WriteOut(void)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      PrintResults(i, false);
   }

   // Print out the separate decoy hits.
   if (g_StaticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, true);
      }
   }
}


void CometWriteOut::PrintResults(int iWhichQuery,
                                 bool bDecoySearch)
{
   int  i,
        ii,
        iDoXcorrCount,
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

   strcpy(scan1, "0");
   strcpy(scan2, "0");

    sprintf(szMassLine, "(M+H)+ mass = %0.6f ~ %0.6f (%+d), fragment tol = %0.4f, binoffset = %0.3f",
             g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass,
             g_pvQuery.at(iWhichQuery)->_pepMassInfo.dPeptideMassTolerance,
             g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState,
             g_StaticParams.tolerances.dFragmentBinSize,
             g_StaticParams.tolerances.dFragmentBinStartOffset); 

    if (g_StaticParams.massUtility.bMonoMassesParent)
       sprintf(szMassLine + strlen(szMassLine), ", MONO");
    else
       sprintf(szMassLine + strlen(szMassLine), ", AVG");

    if (g_StaticParams.massUtility.bMonoMassesFragment)
       sprintf(szMassLine + strlen(szMassLine), "/MONO");
    else
       sprintf(szMassLine + strlen(szMassLine), "/AVG");

   if (bDecoySearch)
   {
      sprintf(szOutput, "%s_decoy/%s.%.5d.%.5d.%d.out",
            g_StaticParams.inputFile.szBaseName, 
            g_StaticParams.inputFile.szBaseName,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState);
   }
   else
   {
      sprintf(szOutput, "%s/%s.%.5d.%.5d.%d.out",
            g_StaticParams.inputFile.szBaseName, 
            g_StaticParams.inputFile.szBaseName,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState);
   }

   if (g_StaticParams.options.iWhichReadingFrame)
   {
      sprintf(szDbLine, "# bases = %ld (frame=%d), # proteins = %d, %s",
            g_StaticParams.databaseInfo.liTotAACount, 
            g_StaticParams.options.iWhichReadingFrame, 
            g_StaticParams.databaseInfo.iTotalNumProteins, 
            g_StaticParams.databaseInfo.szDatabase);
   }
   else
   {
      sprintf(szDbLine, "# amino acids = %ld, # proteins = %d, %s",
            g_StaticParams.databaseInfo.liTotAACount, 
            g_StaticParams.databaseInfo.iTotalNumProteins, 
            g_StaticParams.databaseInfo.szDatabase);
   }

   if ((fpout = fopen(szOutput, "w")) == NULL)
   {
      fprintf(stderr, "Error - cannot write to file %s.\n\n", szOutput);
      exit(1);
   }

   if (bDecoySearch)
   {
      sprintf(szStatsBuf, "total inten = %0.2E, lowest Sp = %0.1f, # matched peptides = %ld",
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.dTotalIntensity, 
            g_pvQuery.at(iWhichQuery)->fLowestDecoySpScore, 
            g_pvQuery.at(iWhichQuery)->_liNumMatchedDecoyPeptides);
   }
   else
   {
      sprintf(szStatsBuf, "total inten = %0.2E, lowest Sp = %0.1f, # matched peptides = %ld",
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.dTotalIntensity, 
            g_pvQuery.at(iWhichQuery)->fLowestSpScore, 
            g_pvQuery.at(iWhichQuery)->_liNumMatchedPeptides);
   }

   sprintf(szBuf, "\n");
   sprintf(szBuf+strlen(szBuf), " %s\n", szOutput);
   sprintf(szBuf+strlen(szBuf), " Comet version %s\n", version);
   sprintf(szBuf+strlen(szBuf), " %s\n", copyright);
   sprintf(szBuf+strlen(szBuf), " %s\n", g_StaticParams.szTimeBuf);
   sprintf(szBuf+strlen(szBuf), " %s\n", szMassLine);
   sprintf(szBuf+strlen(szBuf), " %s\n", szStatsBuf);
   sprintf(szBuf+strlen(szBuf), " %s\n", szDbLine);
   sprintf(szBuf+strlen(szBuf), " %s\n", g_StaticParams.szIonSeries);
   sprintf(szBuf+strlen(szBuf), " %s\n", g_StaticParams.szDisplayLine); 

   fprintf(fpout, "%s", szBuf);

   szBuf[0] = '\0';

   if (strlen(g_StaticParams.szMod) > 0)
   {
      sprintf(szBuf+strlen(szBuf), " %s", g_StaticParams.szMod);
      sprintf(szBuf+strlen(szBuf), "\n");
   }

   fprintf(fpout, "%s\n", szBuf);

   szBuf[0]='\0';

   if (bDecoySearch)
      iDoXcorrCount = g_pvQuery.at(iWhichQuery)->iDoDecoyXcorrCount;
   else
      iDoXcorrCount = g_pvQuery.at(iWhichQuery)->iDoXcorrCount;

   // Print out each sequence line.
   if (iDoXcorrCount > (g_StaticParams.options.iNumPeptideOutputLines))
      iDoXcorrCount = (g_StaticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoySearch)
      pOutput = g_pvQuery.at(iWhichQuery)->_pDecoys;
   else
      pOutput = g_pvQuery.at(iWhichQuery)->_pResults;

   iMaxWidthReference = 9;
   iLenMaxDuplicates = 0;
   for (i=0; i<iDoXcorrCount; i++)
   {
      if (pOutput[i].iDuplicateCount>(unsigned int)iLenMaxDuplicates)
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

   if (g_StaticParams.options.bPrintExpectScore)
      sprintf(szBuf, "  #   Rank/Sp    (M+H)+   deltCn   Xcorr   Expect   Ions  Reference");
   else
      sprintf(szBuf, "  #   Rank/Sp    (M+H)+   deltCn   Xcorr    Sp    Ions  Reference");

   for (i=0; i<iMaxWidthReference-9; i++)
      sprintf(szBuf+strlen(szBuf), " ");

   for (i=0; i<iLenMaxDuplicates; i++)
      sprintf(szBuf+strlen(szBuf), " ");

   sprintf(szBuf+strlen(szBuf), "  ");
   sprintf(szBuf+strlen(szBuf), "Peptide\n");

   if (g_StaticParams.options.bPrintExpectScore)
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

   for (i=0; i<iDoXcorrCount; i++)
   {
      if ((i > 0) && (pOutput[i].fXcorr != pOutput[i-1].fXcorr))
         iRankXcorr++;

      if (pOutput[i].fXcorr > 0)
         PrintOutputLine(iWhichQuery, iRankXcorr, iLenMaxDuplicates, iMaxWidthReference, i, bDecoySearch, pOutput, fpout);
   } 

   fprintf(fpout, "\n");

   // Print out the fragment ions for the selected ion series
   // and mark matched ions in the sp scoring routine.
   if (g_StaticParams.options.bPrintFragIons && iDoXcorrCount > 0)
   {
      PrintIons(iWhichQuery,
            g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState,
            fpout);
   }

   // Print out expect score histogram.
   if (g_StaticParams.options.bPrintExpectScore)
   {
      double dVal;
      double dExpect;

      if (bDecoySearch)
      {
         fprintf(fpout, "a=%f b=%f\n",
               g_pvQuery.at(iWhichQuery)->fDecoyPar[1],
               g_pvQuery.at(iWhichQuery)->fDecoyPar[0]);

         // iDecoyCorrelationHistogram is already cummulative here.
         for (i=0; i<=g_pvQuery.at(iWhichQuery)->siMaxDecoyXcorr; i++)
         {
            if (g_pvQuery.at(iWhichQuery)->iDecoyCorrelationHistogram[i]> 0)
            {
               dVal = g_pvQuery.at(iWhichQuery)->fDecoyPar[0] + g_pvQuery.at(iWhichQuery)->fDecoyPar[1] * i;
               dExpect = pow(10.0, dVal);

               if (dExpect>999.0)
                  dExpect = 999.0;

               fprintf(fpout, "HIST:\t%0.1f\t%d\t%0.3f\t%0.3f\t%0.3f\n",
                     i*0.1,
                     g_pvQuery.at(iWhichQuery)->iDecoyCorrelationHistogram[i],
                     log10((float)g_pvQuery.at(iWhichQuery)->iDecoyCorrelationHistogram[i]),
                     dVal,
                     dExpect);
            }
         }
         fprintf(fpout, "\n");
      }
      else
      {
         fprintf(fpout, "a=%f b=%f %d-%d\n",
               g_pvQuery.at(iWhichQuery)->fPar[1],
               g_pvQuery.at(iWhichQuery)->fPar[0],
               (int)g_pvQuery.at(iWhichQuery)->fPar[2],
               (int)g_pvQuery.at(iWhichQuery)->fPar[3]);

         // iCorrelationHistogram is already cummulative here.
         for (i=0; i<=g_pvQuery.at(iWhichQuery)->siMaxXcorr; i++)
         {
            if (g_pvQuery.at(iWhichQuery)->iCorrelationHistogram[i]> 0)
            {
               dVal = g_pvQuery.at(iWhichQuery)->fPar[0] + g_pvQuery.at(iWhichQuery)->fPar[1] * i;
               dExpect = pow(10.0, dVal);

               if (dExpect>999.0)
                  dExpect = 999.0;

               fprintf(fpout, "HIST:\t%0.1f\t%d\t%0.3f\t%0.3f\t%0.3f\n",
                     i*0.1,
                     g_pvQuery.at(iWhichQuery)->iCorrelationHistogram[i],
                     log10((float)g_pvQuery.at(iWhichQuery)->iCorrelationHistogram[i]),
                     dVal,
                     dExpect);
            }
         }
         fprintf(fpout, "\n");
      }
   }
   fclose(fpout);
}


void CometWriteOut::PrintOutputLine(int iWhichQuery,
                                    int iRankXcorr,
                                    int iLenMaxDuplicates,
                                    int iMaxWidthReference,
                                    int iWhichResult,
                                    bool bDecoySearch,
                                    Results *pOutput,
                                    FILE *fpout)
{
   int  i,
        iWidthSize,
        iWidthPrintRef;
   char szBuf[SIZE_BUF];

   sprintf(szBuf, "%3d. %3d /%3d  %9.4f  %6.4f %7.4f ",
         iWhichResult+1,
         iRankXcorr,
         pOutput[iWhichResult].iRankSp,
         pOutput[iWhichResult].dPepMass,
         1.000 - pOutput[iWhichResult].fXcorr/pOutput[0].fXcorr,
         pOutput[iWhichResult].fXcorr);

   if (g_StaticParams.options.bPrintExpectScore)
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

   fprintf(fpout, "%s\n", szBuf);
}


// Print out fragment ions at end of .out files.
void CometWriteOut::PrintIons(int iWhichQuery,
                              int iChargeState,
                              FILE *fpout)
{
   int  i,
        ii,
        ctCharge;
   char szBuf[SIZE_BUF];
   double dFragmentIonMass = 0.0;
   double _pdAAforward[MAX_PEPTIDE_LEN];
   double _pdAAreverse[MAX_PEPTIDE_LEN];

   double dBion = g_StaticParams.precalcMasses.dNtermProton;
   double dYion = g_StaticParams.precalcMasses.dCtermOH2Proton;

   if (g_pvQuery.at(iWhichQuery)->_pResults[0].szPrevNextAA[0] == '-')
      dBion += g_StaticParams.staticModifications.dAddNterminusProtein;
   if (g_pvQuery.at(iWhichQuery)->_pResults[0].szPrevNextAA[1] == '-')
      dYion += g_StaticParams.staticModifications.dAddCterminusProtein;

   if (g_StaticParams.variableModParameters.bVarModSearch
         && (g_pvQuery.at(iWhichQuery)->_pResults[0].pcVarModSites[g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide] == 1))
   {
      dBion += g_StaticParams.variableModParameters.dVarModMassN;
   }

   if (g_StaticParams.variableModParameters.bVarModSearch
         && (g_pvQuery.at(iWhichQuery)->_pResults[0].pcVarModSites[g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide + 1] == 1))
   {
      dYion += g_StaticParams.variableModParameters.dVarModMassC;
   }

   // Generate pdAAforward for g_pvQuery.at(iWhichQuery)->_pResults[0].szPeptide.
   for (i=0; i<g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide; i++)
   {
      int iPos = g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide - i - 1;

      dBion += g_StaticParams.massUtility.pdAAMassFragment[(int)g_pvQuery.at(iWhichQuery)->_pResults[0].szPeptide[i]];
      dYion += g_StaticParams.massUtility.pdAAMassFragment[(int)g_pvQuery.at(iWhichQuery)->_pResults[0].szPeptide[iPos]];

      if (g_StaticParams.variableModParameters.bVarModSearch)
         dBion += g_StaticParams.variableModParameters.varModList[g_pvQuery.at(iWhichQuery)->_pResults[0].pcVarModSites[i]-1].dVarModMass;

      if (g_StaticParams.variableModParameters.bVarModSearch
            && (i == g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide -1)
            && (g_pvQuery.at(iWhichQuery)->_pResults[0].pcVarModSites[g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide + 1] == 1))
      {
         dBion += g_StaticParams.variableModParameters.dVarModMassC;
      }

      if (g_StaticParams.variableModParameters.bVarModSearch)
         dYion += g_StaticParams.variableModParameters.varModList[g_pvQuery.at(iWhichQuery)->_pResults[0].pcVarModSites[iPos]-1].dVarModMass;

      _pdAAforward[i] = dBion;
      _pdAAreverse[iPos] = dYion;
   }

   for (ctCharge=1; ctCharge<=g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iMaxFragCharge; ctCharge++)
   {
      if (ctCharge > 1)
         sprintf(szBuf, "\n");

      sprintf(szBuf, "\n Seq  #  ");

      for (i=0; i<g_StaticParams.ionInformation.iNumIonSeriesUsed; i++)
      {
         int iWhichIonSeries = g_StaticParams.ionInformation.piSelectedIonSeries[i];

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

      for (i=0; i<g_StaticParams.ionInformation.iNumIonSeriesUsed; i++)
      {
         sprintf(szBuf+strlen(szBuf), "---------  ");
      }

      fprintf(fpout, "%s --", szBuf);

      for (i=0; i<g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide; i++)
      {
         sprintf(szBuf, "\n  %c  %2d  ", g_pvQuery.at(iWhichQuery)->_pResults[0].szPeptide[i], i+1);

         for (ii=0; ii<g_StaticParams.ionInformation.iNumIonSeriesUsed; ii++)
         {
            int iWhichIonSeries;

            iWhichIonSeries = g_StaticParams.ionInformation.piSelectedIonSeries[ii];

            dFragmentIonMass = CometMassSpecUtils::GetFragmentIonMass(iWhichIonSeries, i, ctCharge, _pdAAforward, _pdAAreverse);

            if ((dFragmentIonMass <= FLOAT_ZERO)
                  || ((i == g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide - 1) && (iWhichIonSeries <= 3))
                  || (i == 0 && (iWhichIonSeries >= 4)))
            {
               sprintf(szBuf+strlen(szBuf), "    -      ");
            }
            else
            {
               sprintf(szBuf+strlen(szBuf), "%9.4f", dFragmentIonMass);

               if (g_pvQuery.at(iWhichQuery)->pfSpScoreData[BIN(dFragmentIonMass)] > FLOAT_ZERO)
                  sprintf(szBuf+strlen(szBuf), "+ ");
               else
                  sprintf(szBuf+strlen(szBuf), "  ");
            }
         }
         fprintf(fpout, "%s %2d", szBuf, g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide-i); 

      }
      fprintf(fpout, "\n");

   }
   fprintf(fpout, "\n");
}
