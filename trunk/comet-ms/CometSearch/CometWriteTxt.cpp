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
   fprintf(fpout, "num\t");
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
      Query* pQuery = g_pvQuery.at(iWhichQuery);

      int charge = pQuery->_spectrumInfoInternal.iChargeState;
      double spectrum_neutral_mass = pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS;
      double spectrum_mz = (spectrum_neutral_mass + charge*PROTON_MASS) / (double)charge;

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

         double dDeltaCn;

         if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr >= 0.0)
            dDeltaCn = 1.0 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr;
         else if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr < 0.0)
            dDeltaCn = 1.0;
         else
            dDeltaCn = 0.0;

         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);
         fprintf(fpout, "%d\t",  pQuery->_spectrumInfoInternal.iChargeState);
         fprintf(fpout, "%0.6f\t",  spectrum_mz);
         fprintf(fpout, "%0.6f\t", spectrum_neutral_mass);
         fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass - PROTON_MASS);
         fprintf(fpout, "%0.4f\t", dDeltaCn);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fScoreSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iRankSp);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fXcorr);
         fprintf(fpout, "%lu\t", iWhichResult + 1);                 // assuming want index starting at 1
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iMatchedIons);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iTotalIons);
         fprintf(fpout, "%lu\t", num_matches);

         //Print out peptide and give mass for variable mods.
         if (g_staticParams.variableModParameters.bVarModSearch
               && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
         {
            fprintf(fpout, "n[%0.4f]",
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass);
         }

         for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
         {
            fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

            if (g_staticParams.variableModParameters.bVarModSearch && pOutput[iWhichResult].pcVarModSites[i] > 0)
            {
               fprintf(fpout, "[%0.4f]",
                     g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass);
            }
         }

         if (g_staticParams.variableModParameters.bVarModSearch
               && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            fprintf(fpout, "c[0.4%f]", 
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass);
         }

         // Print protein reference/accession.
         fprintf(fpout, "\t%s\t", pOutput[iWhichResult].szProtein);
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
   if ((!bDecoy && g_pvQuery.at(iWhichQuery)->_pResults[0].fXcorr > XCORR_CUTOFF)
         || (bDecoy && g_pvQuery.at(iWhichQuery)->_pDecoys[0].fXcorr > XCORR_CUTOFF))
   {
      Query* pQuery = g_pvQuery.at(iWhichQuery);

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

         double dDeltaCn;

         if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr >= 0.0)
            dDeltaCn = 1.0 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr;
         else if (pOutput[0].fXcorr > 0.0 && pOutput[iWhichResult+1].fXcorr < 0.0)
            dDeltaCn = 1.0;
         else
            dDeltaCn = 0.0;

         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);
         fprintf(fpout, "%lu\t", iWhichResult+1);
         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iChargeState);
         fprintf(fpout, "%0.6f\t", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
         fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass - PROTON_MASS);
         fprintf(fpout, "%0.2E\t", pOutput[iWhichResult].dExpect);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fXcorr);
         fprintf(fpout, "%0.4f\t", dDeltaCn);
         fprintf(fpout, "%0.1f\t", pOutput[iWhichResult].fScoreSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iMatchedIons);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iTotalIons);

         // Print plain peptide
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

         // Print peptide sequence
         fprintf(fpout, "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

         if (g_staticParams.variableModParameters.bVarModSearch
               && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
         {
            fprintf(fpout, "n[%0.1f]",
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass);
         }

         for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
         {
            fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

            if (g_staticParams.variableModParameters.bVarModSearch
                  && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass, 0.0))
            {
               fprintf(fpout, "[%0.1f]",
                     g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass);
            }
         }

         if (g_staticParams.variableModParameters.bVarModSearch
               && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            fprintf(fpout, "c[0.1%f]", 
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass);
         }

         fprintf(fpout, ".%c\t", pOutput[iWhichResult].szPrevNextAA[1]);

         fprintf(fpout, "%c\t", pOutput[iWhichResult].szPrevNextAA[0]);
         fprintf(fpout, "%c\t", pOutput[iWhichResult].szPrevNextAA[1]);

         // Print protein reference/accession.
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szProtein);

         fprintf(fpout, "%d", pOutput[iWhichResult].iDuplicateCount); 

         fprintf(fpout, "\n");
      }
   }
}
#endif
