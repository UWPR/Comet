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


bool CometWriteOut::WriteOut(FILE *fpdb)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      if (!PrintResults(i, false, fpdb))
      {
         return false;
      }
   }

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         if (!PrintResults(i, true, fpdb))
         {
            return false;
         }
      }
   }

   return true;
}


bool CometWriteOut::PrintResults(int iWhichQuery,
                                 bool bDecoySearch,
                                 FILE *fpdb)
{
   int  i,
        iNumPrintLines,
        iLenMaxDuplicates,
        iMaxWidthReference,
        iRankXcorr;
   char szDbLine[1024],
        szBuf[SIZE_BUF],
        szStatsBuf[512],
        szMassLine[200],
        szOutput[1024],
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

   bool bPrintDecoyPrefix = false;

   for (i=0; i<iNumPrintLines; i++)
   {
      char szProteinName[100];
      vector<ProteinEntryStruct>::iterator it;

      int iNumTotProteins = 0;

      // Now get max protein length of first protein; take target entry if available
      // otherwise take decoy entry
      if (bDecoySearch)
      {
         if ((int)pOutput[i].pWhichDecoyProtein.size() > 0)
         {
            it = pOutput[i].pWhichDecoyProtein.begin();
            CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
            bPrintDecoyPrefix = true;
         }
         iNumTotProteins = (int)pOutput[i].pWhichDecoyProtein.size();
      }
      else
      {
         if ((int)pOutput[i].pWhichProtein.size() > 0)
         {
            it = pOutput[i].pWhichProtein.begin();
            CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
            iNumTotProteins = (int)(pOutput[i].pWhichProtein.size() + pOutput[i].pWhichDecoyProtein.size());
         }
         else //if ( (int)pOutput[i].pWhichDecoyProtein.size() > 0)
         {
            it = pOutput[i].pWhichDecoyProtein.begin();
            CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
            iNumTotProteins = (int)pOutput[i].pWhichDecoyProtein.size();
            bPrintDecoyPrefix = true;

         }
      }

      // Use iLenMaxDuplicates here to store largest duplicate count;
      // This is then used after for loop.
      if (iNumTotProteins > iLenMaxDuplicates)
         iLenMaxDuplicates = iNumTotProteins;

      if ((int)strlen(szProteinName) > iMaxWidthReference)
         iMaxWidthReference = (int)strlen(szProteinName);
   }
   if (bPrintDecoyPrefix)
      iMaxWidthReference += (int)strlen(g_staticParams.szDecoyPrefix);

   if (iLenMaxDuplicates > 0)
   {
      char szTempStr[10];

      sprintf(szTempStr, " %+d", iLenMaxDuplicates);
      iLenMaxDuplicates = (int)strlen(szTempStr);
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
         PrintOutputLine(iRankXcorr, iLenMaxDuplicates, iMaxWidthReference, i, bDecoySearch, pOutput, fpout, fpdb);
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
   fclose(fpout);

   return true;
}


void CometWriteOut::PrintOutputLine(int iRankXcorr,
                                    int iLenMaxDuplicates,
                                    int iMaxWidthReference,
                                    int iWhichResult,
                                    bool bDecoySearch,
                                    Results *pOutput,
                                    FILE *fpout,
                                    FILE *fpdb)
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

   char szProteinName[100];
   vector<ProteinEntryStruct>::iterator it;
   bool bPrintDecoyPrefix = false;

   int iNumTotProteins = 0;

   if (bDecoySearch)
   {
      it = pOutput[iWhichResult].pWhichDecoyProtein.begin();
      iNumTotProteins = (int)pOutput[iWhichResult].pWhichDecoyProtein.size();
      bPrintDecoyPrefix = true;
   }
   else
   {
      if ((int)pOutput[iWhichResult].pWhichProtein.size() > 0)
      {
         it = pOutput[iWhichResult].pWhichProtein.begin();
         iNumTotProteins = (int)(pOutput[iWhichResult].pWhichProtein.size() + pOutput[iWhichResult].pWhichDecoyProtein.size());
      }
      else
      {
         it = pOutput[iWhichResult].pWhichDecoyProtein.begin();
         iNumTotProteins = (int)pOutput[iWhichResult].pWhichDecoyProtein.size();
         bPrintDecoyPrefix = true;
      }
   }

   CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);

   if (bPrintDecoyPrefix)
      strcat(szBuf, g_staticParams.szDecoyPrefix);
   while (iWidthSize < (int)strlen(szProteinName))
   {
      if (33<=(szProteinName[iWidthSize]) && (szProteinName[iWidthSize])<=126) // Ascii physical character range.
      {
         sprintf(szBuf+strlen(szBuf), "%c", szProteinName[iWidthPrintRef]);

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
   if (iNumTotProteins)
   {
      char szTemp[10];
      int iEnd;

      sprintf(szTemp, "+%d", iNumTotProteins);
      sprintf(szBuf+strlen(szBuf), " +%d", iNumTotProteins);

      iEnd = iLenMaxDuplicates - (int)strlen(szTemp) - 1;

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
         && pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      sprintf(szBuf+strlen(szBuf),
            "n%c", g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide]-1]);
   }

   // Print peptide sequence.
   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      sprintf(szBuf+strlen(szBuf), "%c", (int)pOutput[iWhichResult].szPeptide[i]);

      if (g_staticParams.variableModParameters.bVarModSearch && pOutput[iWhichResult].piVarModSites[i] != 0)
      {
         if (pOutput[iWhichResult].piVarModSites[i] > 0
               && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].piVarModSites[i]-1].dVarModMass, 0.0))
         {
            sprintf(szBuf+strlen(szBuf), "%c",
                  (int)g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].piVarModSites[i]-1]);
         }
         else
         {
            sprintf(szBuf+strlen(szBuf), "?");  // PEFF:  no clue how to specify mod encoding
         }
      }
   }

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 1)
   {
      sprintf(szBuf+strlen(szBuf),
            "c%c", g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1]);
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
         && (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].iLenPeptide] == 1))
   {
      dBion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].iLenPeptide]-1].dVarModMass;
   }

   if (g_staticParams.variableModParameters.bVarModSearch
         && (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].iLenPeptide + 1] == 1))
   {
      dYion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].iLenPeptide+1]-1].dVarModMass;
   }

   // Generate pdAAforward for pQuery->_pResults[0].szPeptide.
   for (i=0; i<pQuery->_pResults[0].iLenPeptide; i++)
   {
      int iPos = pQuery->_pResults[0].iLenPeptide - i - 1;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[0].szPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[0].szPeptide[iPos]];

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         if (pQuery->_pResults[0].piVarModSites[i] != 0)
            dBion += pQuery->_pResults[0].pdVarModSites[i];   // PEFF need to validate this change
//          dBion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].piVarModSites[i]-1].dVarModMass;


         if (pQuery->_pResults[0].piVarModSites[iPos] != 0)
            dYion += pQuery->_pResults[0].pdVarModSites[iPos];
//          dYion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].piVarModSites[iPos]-1].dVarModMass;
      }

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
