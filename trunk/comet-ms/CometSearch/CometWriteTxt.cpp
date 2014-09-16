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
#include "CometWriteTxt.h"


CometWriteTxt::CometWriteTxt()
{
}


CometWriteTxt::~CometWriteTxt()
{
}


void CometWriteTxt::WriteTxt(FILE *fpout,
                             FILE *fpoutd)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
      PrintResults(i, 0, fpout);

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
         PrintResults(i, 1, fpoutd);
   }
}


void CometWriteTxt::PrintTxtHeader(FILE *fpout)
{
#ifdef CRUX
   fprintf(fpout, "scan\t");
   fprintf(fpout, "charge\t");
   fprintf(fpout, "spectrum precursor m/z\t");
   fprintf(fpout, "spectrum neutral mass\t");
   fprintf(fpout, "peptide mass\t");
   fprintf(fpout, "delta_cn\t");
   fprintf(fpout, "sp score\t");
   fprintf(fpout, "sp rank\t");
   fprintf(fpout, "xcorr score\t");
   fprintf(fpout, "xcorr rank\t");
   fprintf(fpout, "b/y ions matched\t");
   fprintf(fpout, "b/y ions total\t");
   fprintf(fpout, "total matches/spectrum\t");
   fprintf(fpout, "sequence\t");
   fprintf(fpout, "protein id\t");
   fprintf(fpout, "flanking aa\t");
   fprintf(fpout, "e-value\n");
#else
   fprintf(fpout, "CometVersion %s\t", comet_version);
   fprintf(fpout, "%s\t", g_staticParams.inputFile.szBaseName);
   fprintf(fpout, "%s\t", g_staticParams.szDate);
   fprintf(fpout, "%s\n", g_staticParams.databaseInfo.szDatabase);

   fprintf(fpout, "scan\t");
   fprintf(fpout, "charge\t");
   fprintf(fpout, "exp_neutral_mass\t");
   fprintf(fpout, "calc_neutral_mass\t");
   fprintf(fpout, "e-value\t");
   fprintf(fpout, "xcorr\t");
   fprintf(fpout, "delta_cn\t");
   fprintf(fpout, "sp_score\t");
   fprintf(fpout, "ions_matched\t");
   fprintf(fpout, "ions_total\t");
   fprintf(fpout, "plain_peptide\t");
   fprintf(fpout, "peptide\t");
   fprintf(fpout, "prev_aa\t");
   fprintf(fpout, "next_aa\t");
   fprintf(fpout, "protein\t");
   fprintf(fpout, "duplicate_protein_count\n");
#endif
}


#ifdef CRUX
void CometWriteTxt::PrintResults(int iWhichQuery,
                                 bool bDecoy,
                                 FILE *fpout)
{
   if ((!bDecoy && g_pvQuery.at(iWhichQuery)->_pResults[0].fXcorr > XCORR_CUTOFF)
         || (bDecoy && g_pvQuery.at(iWhichQuery)->_pDecoys[0].fXcorr > XCORR_CUTOFF))
   {
      char szBuf[SIZE_BUF];

      Query* pQuery = g_pvQuery.at(iWhichQuery);

      int charge = pQuery->_spectrumInfoInternal.iChargeState;
      double spectrum_neutral_mass = pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS;
      double spectrum_mz = (spectrum_neutral_mass + charge*PROTON_MASS) / (double)charge;

      sprintf(szBuf, "%d\t%d\t%0.6f\t%0.6f\t",
            pQuery->_spectrumInfoInternal.iScanNumber, 
            pQuery->_spectrumInfoInternal.iChargeState,
            spectrum_mz,
            spectrum_neutral_mass);

      Results *pOutput;
      unsigned long num_matches;
      if (bDecoy)
      {
         pOutput = pQuery->_pDecoys;
         num_matches = pQuery->_uliNumMatchedDecoyPeptides;
      }
      else
      {
         pOutput = pQuery->_pResults;
         num_matches = pQuery->_uliNumMatchedPeptides;
      }
      
      size_t iNumPrintLines = min((unsigned long)g_staticParams.options.iNumPeptideOutputLines, num_matches);

      for (size_t iWhichResult=0; iWhichResult<iNumPrintLines; iWhichResult++)
      {
         if (pOutput[iWhichResult].fXcorr <= XCORR_CUTOFF)
            continue;

         fprintf(fpout, "%s", szBuf);

         double dDeltaCn;

         if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr >= 0.0)
            dDeltaCn = 1.0 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr;
         else if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr < 0.0)
            dDeltaCn = 1.0;
         else
            dDeltaCn = 0.0;

         fprintf(fpout,
               "%0.4f\t"
               "%0.4f\t"
               "%0.4f\t"
               "%d\t"
               "%0.4f\t"
               "%lu\t"
               "%d\t"
               "%d\t"
               "%lu\t",
                  pOutput[iWhichResult].dPepMass - PROTON_MASS,
                  dDeltaCn,
                  pOutput[iWhichResult].fScoreSp,
                  pOutput[iWhichResult].iRankSp,
                  pOutput[iWhichResult].fXcorr,
                  iWhichResult + 1,                  // assuming want index starting at 1
                  pOutput[iWhichResult].iMatchedIons, 
                  pOutput[iWhichResult].iTotalIons,
                  num_matches);

         char szBuf2[SIZE_BUF];
         szBuf2[0] = '\0';
            
         //Print out peptide and give mass for variable mods.
         if (g_staticParams.variableModParameters.bVarModSearch
               && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
         {
            sprintf(szBuf2, "n[%0.4f]",
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass);
         }

         for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
         {
            sprintf(szBuf2+strlen(szBuf2), "%c", pOutput[iWhichResult].szPeptide[i]);

            if (g_staticParams.variableModParameters.bVarModSearch && pOutput[iWhichResult].pcVarModSites[i] > 0)
            {
               sprintf(szBuf2+strlen(szBuf2), "[%0.4f]",
                     g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass);
            }
         }

         if (g_staticParams.variableModParameters.bVarModSearch
               && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            sprintf(szBuf2+strlen(szBuf2), "c[0.4%f]", 
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass);
         }

         fprintf(fpout, "%s\t", szBuf2);
         // Print protein reference/accession.
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szProtein);
         // Cleavage type
         fprintf(fpout, "%c%c\t", pOutput[iWhichResult].szPrevNextAA[0], pOutput[iWhichResult].szPrevNextAA[1]);
         // e-value
         fprintf(fpout, "%0.2E\n", pOutput[iWhichResult].dExpect);
      }
   }
}

#else
void CometWriteTxt::PrintResults(int iWhichQuery,
                                 bool bDecoy,
                                 FILE *fpout)
{
   char szBuf[SIZE_BUF];

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   sprintf(szBuf, "%d\t%d\t%0.6f\t",
         pQuery->_spectrumInfoInternal.iScanNumber, 
         pQuery->_spectrumInfoInternal.iChargeState, 
         pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);

   fprintf(fpout, "%s", szBuf);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   if (pOutput[0].fXcorr > XCORR_CUTOFF)
      PrintTxtLine(0, pOutput, fpout);  // print top hit only right now
   else
      fprintf(fpout, "\n");
}
#endif

#ifdef CRUX
void CometWriteTxt::PrintTxtLine(int iWhichResult,
                                 Results *pOutput,
                                 FILE *fpout)
{
}
#else
void CometWriteTxt::PrintTxtLine(int iWhichResult,
                                 Results *pOutput,
                                 FILE *fpout)
{
   int  i;
   char szBuf[SIZE_BUF];

   double dDeltaCn;

   if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr >= 0.0)
      dDeltaCn = 1.0 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr;
   else if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr < 0.0)
      dDeltaCn = 1.0;
   else
      dDeltaCn = 0.0;

   sprintf(szBuf, "%0.6f\t%0.2E\t%0.4f\t%0.4f\t%0.1f\t%d\t%d",
         pOutput[iWhichResult].dPepMass - PROTON_MASS,
         pOutput[iWhichResult].dExpect,
         pOutput[iWhichResult].fXcorr,
         dDeltaCn,
         pOutput[iWhichResult].fScoreSp,
         pOutput[iWhichResult].iMatchedIons, 
         pOutput[iWhichResult].iTotalIons);

   fprintf(fpout, "%s\t", szBuf);

   // Print plain peptide
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

   // Print peptide sequence
   sprintf(szBuf, "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      sprintf(szBuf+strlen(szBuf), "n%c",
            g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1]);
   }

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
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
   {
      sprintf(szBuf+strlen(szBuf), "c%c",
            g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1]);
   }

   sprintf(szBuf+strlen(szBuf), ".%c", pOutput[iWhichResult].szPrevNextAA[1]);

   fprintf(fpout, "%s\t", szBuf);

   fprintf(fpout, "%c\t%c\t", pOutput[iWhichResult].szPrevNextAA[0], pOutput[iWhichResult].szPrevNextAA[1]);

   // Print protein reference/accession.
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szProtein);

   fprintf(fpout, "%d", pOutput[iWhichResult].iDuplicateCount); 

   fprintf(fpout, "\n");
}
#endif
