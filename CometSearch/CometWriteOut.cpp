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
   for (i=0; i<(int)g_pvQuery.size(); ++i)
   {
      if (!PrintResults(i, false, fpdb))
      {
         return false;
      }
   }

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
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
        iMaxWidthReference;
   string sDBLine;
   FILE *fpout;
   char *pStr;
   std::ostringstream oss;
   std::string strOutput;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

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
      oss << g_staticParams.inputFile.szBaseName << "_decoy/"
          << pStr << "."
          << std::setfill('0') << std::setw(5) << pQuery->_spectrumInfoInternal.iScanNumber << "."
          << std::setfill('0') << std::setw(5) << pQuery->_spectrumInfoInternal.iScanNumber << "."
          << pQuery->_spectrumInfoInternal.usiChargeState << ".out";
      strOutput = oss.str();
   }
   else
   {
      oss << g_staticParams.inputFile.szBaseName << "/"
          << pStr << "."
          << std::setfill('0') << std::setw(5) << pQuery->_spectrumInfoInternal.iScanNumber << "."
          << std::setfill('0') << std::setw(5) << pQuery->_spectrumInfoInternal.iScanNumber << "."
          << pQuery->_spectrumInfoInternal.usiChargeState << ".out";
      strOutput = oss.str();
   }

   if (g_staticParams.options.iWhichReadingFrame)
   {
      sDBLine = "# bases = " + std::to_string(g_staticParams.databaseInfo.uliTotAACount)
         + " (frame=" + std::to_string(g_staticParams.options.iWhichReadingFrame) + "),"
         + " # proteins = " + std::to_string(g_staticParams.databaseInfo.iTotalNumProteins) + ", "
         + g_staticParams.databaseInfo.szDatabase;
   }
   else
   {
      sDBLine = "# amino acids = " + std::to_string(g_staticParams.databaseInfo.uliTotAACount)
         + ", # proteins = " + std::to_string(g_staticParams.databaseInfo.iTotalNumProteins) + ", "
         + g_staticParams.databaseInfo.szDatabase;
   }

   if ((fpout = fopen(strOutput.c_str(), "w")) == NULL)
   {
      string strErrorMsg = " Error - cannot write to file " + strOutput + "\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   fprintf(fpout, "\n");
   fprintf(fpout, " %s.%.5d.%.5d.%d.out\n",
      pStr,
      pQuery->_spectrumInfoInternal.iScanNumber,
      pQuery->_spectrumInfoInternal.iScanNumber,
      pQuery->_spectrumInfoInternal.usiChargeState);
   fprintf(fpout, " Comet version %s\n", g_sCometVersion.c_str());
   fprintf(fpout, " %s\n", copyright);
   fprintf(fpout, " %s\n", g_staticParams.strOutFileTimeString.c_str());

   fprintf(fpout, " (M+H)+ mass = %0.6f, tol %0.6f %0.6f, +%d, fragment tol = %0.4f, binoffset = %0.3f, %s%s\n",
      pQuery->_pepMassInfo.dExpPepMass,
      pQuery->_pepMassInfo.dPeptideMassToleranceLow,
      pQuery->_pepMassInfo.dPeptideMassToleranceHigh,
      pQuery->_spectrumInfoInternal.usiChargeState,
      g_staticParams.tolerances.dFragmentBinSize,
      g_staticParams.tolerances.dFragmentBinStartOffset,
      g_staticParams.massUtility.bMonoMassesParent   ? "MONO": "AVG",
      g_staticParams.massUtility.bMonoMassesFragment ? "/MONO" : "/AVG");

   if (bDecoySearch)
   {
      fprintf(fpout, " total inten = %0.2E, lowest Sp = 0 # matched peptides = %lu\n",
         pQuery->_spectrumInfoInternal.dTotalIntensity,
         pQuery->_uliNumMatchedDecoyPeptides);
   }
   else
   {
      fprintf(fpout, " total inten = %0.2E, lowest Sp = 0, # matched peptides = %lu\n",
         pQuery->_spectrumInfoInternal.dTotalIntensity,
         pQuery->_uliNumMatchedPeptides);
   }   fprintf(fpout, " %s\n", sDBLine.c_str());

   fprintf(fpout, " %s\n", g_staticParams.szIonSeries);
   fprintf(fpout, " %s\n", g_staticParams.szDisplayLine);

   if (g_staticParams.szMod[0]!='\0')
      fprintf(fpout, " %s\n", g_staticParams.szMod);

   fprintf(fpout, "\n");

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

   for (i=0; i<iNumPrintLines; ++i)
   {
      unsigned int uiNumTotProteins = 0;
      char szProteinName[WIDTH_REFERENCE];

      if (pOutput[i].usiLenPeptide > 0)
      {
         vector<ProteinEntryStruct>::iterator it;

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
            uiNumTotProteins = (int)pOutput[i].pWhichDecoyProtein.size();
         }
         else
         {
            if ((int)pOutput[i].pWhichProtein.size() > 0)
            {
               it = pOutput[i].pWhichProtein.begin();
               CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
               uiNumTotProteins = (int)(pOutput[i].pWhichProtein.size() + pOutput[i].pWhichDecoyProtein.size());
            }
            else //if ( (int)pOutput[i].pWhichDecoyProtein.size() > 0)
            {
               it = pOutput[i].pWhichDecoyProtein.begin();
               CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
               uiNumTotProteins = (int)pOutput[i].pWhichDecoyProtein.size();
               bPrintDecoyPrefix = true;
            }
         }
      }

      if (g_staticParams.iIndexDb)  //index database
      {
         uiNumTotProteins =  (unsigned int)g_pvProteinsList.at(pOutput[i].lProteinFilePosition).size();
      }

      // Use iLenMaxDuplicates here to store largest duplicate count;
      // This is then used after for loop.
      if ((int)uiNumTotProteins > iLenMaxDuplicates)
         iLenMaxDuplicates = uiNumTotProteins;

      if ((int)strlen(szProteinName) > iMaxWidthReference)
         iMaxWidthReference = (int)strlen(szProteinName);
   }
   if (bPrintDecoyPrefix)
      iMaxWidthReference += (int)strlen(g_staticParams.szDecoyPrefix);

   if (iLenMaxDuplicates > 0)
   {
      char szTempStr[13];

      sprintf(szTempStr, " %+d", iLenMaxDuplicates);
      iLenMaxDuplicates = (int)strlen(szTempStr);
   }

   if (g_staticParams.options.bPrintExpectScore)
      fprintf(fpout, "  #   Rank/Sp    (M+H)+   deltCn   Xcorr   Expect   Ions  Reference");
   else
      fprintf(fpout, "  #   Rank/Sp    (M+H)+   deltCn   Xcorr    Sp    Ions  Reference");

   for (i=0; i<iMaxWidthReference-9; ++i)
      fprintf(fpout, " ");

   for (i=0; i<iLenMaxDuplicates; ++i)
      fprintf(fpout, " ");

   fprintf(fpout, "  ");
   fprintf(fpout, "Peptide\n");

   if (g_staticParams.options.bPrintExpectScore)
      fprintf(fpout, " ---  -------  ---------  ------  ------   ------   ----  ---------");
   else
      fprintf(fpout, " ---  -------  ---------  ------  ------   ----   ----  ---------");

   for (i=0; i<iMaxWidthReference-9; ++i)
      fprintf(fpout, " ");

   for (i=0; i<iLenMaxDuplicates; ++i)
      fprintf(fpout, " ");

   fprintf(fpout, "  ");
   fprintf(fpout, "-------\n");

   for (i=0; i<iNumPrintLines; ++i)
   {
      if (pOutput[i].usiLenPeptide > 0 && pOutput[i].fXcorr > g_staticParams.options.dMinimumXcorr)
         PrintOutputLine(iLenMaxDuplicates, iMaxWidthReference, i, bDecoySearch, pOutput, fpout, fpdb);
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
      for (i=0; i<=pQuery->siMaxXcorr; ++i)
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


void CometWriteOut::PrintOutputLine(int iLenMaxDuplicates,
                                    int iMaxWidthReference,
                                    int iWhichResult,
                                    bool bDecoySearch,
                                    Results* pOutput,
                                    FILE* fpout,
                                    FILE* fpdb)
{
   int i, iWidthSize, iWidthPrintRef;
   std::ostringstream oss;

   oss << std::setw(3) << iWhichResult + 1 << ". "
      << std::setw(3) << pOutput[iWhichResult].usiRankXcorr << " /"
      << std::setw(3) << pOutput[iWhichResult].usiRankSp << "  "
      << std::setw(9) << std::fixed << std::setprecision(4) << pOutput[iWhichResult].dPepMass << "  "
      << std::setw(6) << std::setprecision(4) << pOutput[iWhichResult].fDeltaCn << " "
      << std::setw(7) << std::setprecision(4) << pOutput[iWhichResult].fXcorr << " ";

   if (g_staticParams.options.bPrintExpectScore)
   {
      if (pOutput[iWhichResult].dExpect < 0.0001)
         oss << std::setw(8) << std::scientific << std::setprecision(2) << pOutput[iWhichResult].dExpect << " ";
      else
         oss << std::setw(8) << std::fixed << std::setprecision(4) << pOutput[iWhichResult].dExpect << " ";
   }
   else
   {
      oss << std::setw(6) << std::fixed << std::setprecision(1) << pOutput[iWhichResult].fScoreSp << " ";
   }

   if (pOutput[iWhichResult].usiTotalIons < 10)
      oss << std::setw(3) << pOutput[iWhichResult].usiMatchedIons << "/" << pOutput[iWhichResult].usiTotalIons << "   ";
   else if (pOutput[iWhichResult].usiTotalIons < 100)
      oss << std::setw(3) << pOutput[iWhichResult].usiMatchedIons << "/" << std::setw(2) << pOutput[iWhichResult].usiTotalIons << "  ";
   else
      oss << std::setw(3) << pOutput[iWhichResult].usiMatchedIons << "/" << std::setw(3) << pOutput[iWhichResult].usiTotalIons << " ";

   // Print protein reference/accession.
   iWidthSize = 0;
   iWidthPrintRef = 0;

   char szProteinName[WIDTH_REFERENCE];
   vector<ProteinEntryStruct>::iterator it;
   bool bPrintDecoyPrefix = false;
   int uiNumTotProteins = 0;

   if (bDecoySearch)
   {
      it = pOutput[iWhichResult].pWhichDecoyProtein.begin();
      uiNumTotProteins = (int)pOutput[iWhichResult].pWhichDecoyProtein.size();
      bPrintDecoyPrefix = true;
   }
   else
   {
      if ((int)pOutput[iWhichResult].pWhichProtein.size() > 0)
      {
         it = pOutput[iWhichResult].pWhichProtein.begin();
         uiNumTotProteins = (int)(pOutput[iWhichResult].pWhichProtein.size() + pOutput[iWhichResult].pWhichDecoyProtein.size());
      }
      else
      {
         it = pOutput[iWhichResult].pWhichDecoyProtein.begin();
         uiNumTotProteins = (int)pOutput[iWhichResult].pWhichDecoyProtein.size();
         bPrintDecoyPrefix = true;
      }
   }

   CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);

   if (bPrintDecoyPrefix)
      oss << g_staticParams.szDecoyPrefix;

   while (iWidthSize < (int)strlen(szProteinName))
   {
      if (33 <= (szProteinName[iWidthSize]) && (szProteinName[iWidthSize]) <= 126)
      {
         oss << szProteinName[iWidthPrintRef];
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
      iWidthSize = iMaxWidthReference - iWidthPrintRef;
      for (i = 0; i < iWidthSize; ++i)
         oss << " ";
   }

   // Print out the number of duplicate proteins.
   if (uiNumTotProteins)
   {
      char szTemp[16];
      sprintf(szTemp, "+%u", uiNumTotProteins);
      oss << " +" << uiNumTotProteins;
      int iEnd = iLenMaxDuplicates - (int)strlen(szTemp) - 1;
      for (i = 0; i < iEnd; ++i)
         oss << " ";
   }
   else if (iLenMaxDuplicates > 0)
   {
      for (i = 0; i < iLenMaxDuplicates; ++i)
         oss << " ";
   }
   oss << "  ";

   oss << pOutput[iWhichResult].cPrevAA << ".";

   if (g_staticParams.variableModParameters.bVarModSearch
      && pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] > 0)
   {
      oss << "n" << g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] - 1];
   }

   // Print peptide sequence.
   for (i = 0; i < pOutput[iWhichResult].usiLenPeptide; ++i)
   {
      oss << pOutput[iWhichResult].szPeptide[i];

      if (g_staticParams.variableModParameters.bVarModSearch && pOutput[iWhichResult].piVarModSites[i] != 0)
      {
         if (pOutput[iWhichResult].piVarModSites[i] > 0
            && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].piVarModSites[i] - 1].dVarModMass, 0.0))
         {
            oss << g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].piVarModSites[i] - 1];
         }
         else
         {
            oss << "?";
         }
      }
   }

   if (g_staticParams.variableModParameters.bVarModSearch
      && pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide + 1] > 1)
   {
      oss << "c" << g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide + 1] - 1];
   }

   oss << "." << pOutput[iWhichResult].cNextAA;

   fprintf(fpout, "%s\n", oss.str().c_str());
}


// Print out fragment ions at end of .out files.
void CometWriteOut::PrintIons(int iWhichQuery,
                              FILE* fpout)
{
   int i, ii, ctCharge;
   double dFragmentIonMass = 0.0;
   double _pdAAforward[MAX_PEPTIDE_LEN];
   double _pdAAreverse[MAX_PEPTIDE_LEN];

   double dBion = g_staticParams.precalcMasses.dNtermProton;
   double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   if (pQuery->_pResults[0].cPrevAA == '-')
      dBion += g_staticParams.staticModifications.dAddNterminusProtein;
   if (pQuery->_pResults[0].cNextAA == '-')
      dYion += g_staticParams.staticModifications.dAddCterminusProtein;

   if (g_staticParams.variableModParameters.bVarModSearch
      && (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide] == 1))
   {
      dBion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide] - 1].dVarModMass;
   }

   if (g_staticParams.variableModParameters.bVarModSearch
      && (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide + 1] == 1))
   {
      dYion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide + 1] - 1].dVarModMass;
   }

   for (i = 0; i < pQuery->_pResults[0].usiLenPeptide; ++i)
   {
      int iPos = pQuery->_pResults[0].usiLenPeptide - i - 1;

      dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[0].szPeptide[i]];
      dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[0].szPeptide[iPos]];

      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         if (pQuery->_pResults[0].piVarModSites[i] != 0)
            dBion += pQuery->_pResults[0].pdVarModSites[i];

         if (pQuery->_pResults[0].piVarModSites[iPos] != 0)
            dYion += pQuery->_pResults[0].pdVarModSites[iPos];
      }

      _pdAAforward[i] = dBion;
      _pdAAreverse[iPos] = dYion;
   }

   for (ctCharge = 1; ctCharge <= pQuery->_spectrumInfoInternal.usiMaxFragCharge; ++ctCharge)
   {
      std::ostringstream oss;
      if (ctCharge > 1)
         oss << "\n";

      oss << "\n Seq  #  ";
      for (i = 0; i < g_staticParams.ionInformation.iNumIonSeriesUsed; ++i)
      {
         int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[i];
         if (iWhichIonSeries == 0) oss << "    a      ";
         else if (iWhichIonSeries == 1) oss << "    b      ";
         else if (iWhichIonSeries == 2) oss << "    c      ";
         else if (iWhichIonSeries == 3) oss << "    x      ";
         else if (iWhichIonSeries == 4) oss << "    y      ";
         else if (iWhichIonSeries == 5) oss << "    z      ";
         else                           oss << "    z1     ";
      }
      oss << "(" << ctCharge << "+)\n --- --  ";
      for (i = 0; i < g_staticParams.ionInformation.iNumIonSeriesUsed; ++i)
         oss << "---------  ";

      fprintf(fpout, "%s --", oss.str().c_str());

      for (i = 0; i < pQuery->_pResults[0].usiLenPeptide; ++i)
      {
         std::ostringstream line;
         line << "\n  " << pQuery->_pResults[0].szPeptide[i] << "  " << std::setw(2) << i + 1 << "  ";

         for (ii = 0; ii < g_staticParams.ionInformation.iNumIonSeriesUsed; ++ii)
         {
            int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ii];
            dFragmentIonMass = CometMassSpecUtils::GetFragmentIonMass(iWhichIonSeries, i, ctCharge, _pdAAforward, _pdAAreverse);

            if ((dFragmentIonMass <= FLOAT_ZERO)
               || ((i == pQuery->_pResults[0].usiLenPeptide - 1) && (iWhichIonSeries <= 3))
               || (i == 0 && (iWhichIonSeries >= 4)))
            {
               line << "    -      ";
            }
            else
            {
               line << std::setw(9) << std::fixed << std::setprecision(4) << dFragmentIonMass;
               if (FindSpScore(pQuery, BIN(dFragmentIonMass)) > FLOAT_ZERO)
                  line << "+ ";
               else
                  line << "  ";
            }
         }
         fprintf(fpout, "%s %2d", line.str().c_str(), pQuery->_pResults[0].usiLenPeptide - i);
      }
      fprintf(fpout, "\n");
   }
   fprintf(fpout, "\n");
}


float CometWriteOut::FindSpScore(Query *pQuery,
                                 int bin)
{
   int x = bin/SPARSE_MATRIX_SIZE;

   if (pQuery->ppfSparseSpScoreData[x] == NULL)
      return 0.0f;

   int y = bin - (x*SPARSE_MATRIX_SIZE);
   return pQuery->ppfSparseSpScoreData[x][y];
}
