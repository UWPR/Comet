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
   fprintf(fpout, "modified sequence\t");
   fprintf(fpout, "modifications\t");
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
   fprintf(fpout, "modifications\t");
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

         dDeltaCn = 1.0;

         if (pOutput[0].fXcorr > 0.0
               && iWhichResult+1 < (size_t)g_staticParams.options.iNumStored
               && pOutput[iWhichResult+1].fXcorr >= 0.0)
         {
            dDeltaCn = 1.0 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr;
         }

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

         // plain peptide
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

         // modified peptide
         fprintf(fpout, "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

         bool bNterm = false;
         bool bCterm = false;
         double dNterm = 0.0;
         double dCterm = 0.0;

         // See if n-term mod (static and/or variable) needs to be reported
         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0
               || !isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
               || (pOutput[iWhichResult].szPrevNextAA[0]=='-'
                  && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0)) )
         {
            bNterm = true;
            // static peptide n-term mod already accounted for in dNtermProton
            dNterm = g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

            if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
               dNterm += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass;

            if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
               dNterm += g_staticParams.staticModifications.dAddNterminusProtein;
         }

         // See if c-term mod (static and/or variable) needs to be reported
         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0
               || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0)
               || (pOutput[iWhichResult].szPrevNextAA[1]=='-'
                  && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)) )
         {
            bCterm = true;

            // static peptide c-term mod already accounted for in dCtermOH2Proton
            dCterm = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

            if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
               dCterm += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass;

            if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
               dCterm += g_staticParams.staticModifications.dAddCterminusProtein;
         }

         // generate modified_peptide string
         if (bNterm)
            fprintf(fpout, "n[%0.0f]", dNterm);
         for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
         {
            fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

            if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0)
                  || pOutput[iWhichResult].pcVarModSites[i] > 0)
            {
               fprintf(fpout, "[%0.0f]",
                     g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass
                     + g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]]);
            }
         }
         if (bCterm)
            fprintf(fpout, "c[%0.0f]", dCterm);

         fprintf(fpout, ".%c\t", pOutput[iWhichResult].szPrevNextAA[1]);

         // prints modification encoding
         PrintModifications(fpout, pOutput, iWhichResult);

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

      int iNumPrintLines = min((unsigned long)g_staticParams.options.iNumPeptideOutputLines, num_matches);

      int iMinLength = 999;
      for (int i=0; i<iNumPrintLines; i++)
      {
         int iLen = (int)strlen(pOutput[i].szPeptide);
         if (iLen == 0)
            break;
         if (iLen < iMinLength)
            iMinLength = iLen;
      }

      int iRankXcorr = 1;

      for (int iWhichResult=0; iWhichResult<iNumPrintLines; iWhichResult++)
      {
         int j;
         bool bNoDeltaCnYet = true;
         double dDeltaCn = 1.0;

         if (pOutput[iWhichResult].fXcorr <= XCORR_CUTOFF)
            continue;

         // go one past iNumPrintLines to calculate deltaCn value
         for (j=iWhichResult+1; j<iNumPrintLines+1; j++)
         {
            if (j<g_staticParams.options.iNumStored)
            {
               // very poor way of calculating peptide similarity but it's what we have for now
               int iDiffCt = 0;

               for (int k=0; k<iMinLength; k++)
               {
                  // I-L and Q-K are same for purposes here
                  if (pOutput[iWhichResult].szPeptide[k] != pOutput[j].szPeptide[k])
                  {

                     if (!((pOutput[0].szPeptide[k] == 'K' || pOutput[0].szPeptide[k] == 'Q')
                              && (pOutput[j].szPeptide[k] == 'K' || pOutput[j].szPeptide[k] == 'Q'))
                           && !((pOutput[0].szPeptide[k] == 'I' || pOutput[0].szPeptide[k] == 'L')
                              && (pOutput[j].szPeptide[k] == 'I' || pOutput[j].szPeptide[k] == 'L')))
                     {
                        iDiffCt++;
                     }
                  }
               }

               // calculate deltaCn only if sequences are less than 0.75 similar
               if ( ((double) (iMinLength - iDiffCt)/iMinLength) < 0.75)
               {
                  if (pOutput[0].fXcorr > 0.0 && pOutput[j].fXcorr >= 0.0)
                     dDeltaCn = 1.0 - pOutput[j].fXcorr/pOutput[0].fXcorr;
                  else if (pOutput[0].fXcorr > 0.0 && pOutput[j].fXcorr < 0.0)
                     dDeltaCn = 1.0;
                  else
                     dDeltaCn = 0.0;

                  bNoDeltaCnYet = 0;

                  break;
               }
            }
         }

         if (iWhichResult > 0 && !isEqual(pOutput[iWhichResult].fXcorr, pOutput[iWhichResult-1].fXcorr))
            iRankXcorr++;

         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);
         fprintf(fpout, "%d\t", iRankXcorr);
         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iChargeState);
         fprintf(fpout, "%0.6f\t", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
         fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass - PROTON_MASS);
         fprintf(fpout, "%0.2E\t", pOutput[iWhichResult].dExpect);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fXcorr);
         fprintf(fpout, "%0.4f\t", dDeltaCn);
         fprintf(fpout, "%0.1f\t", pOutput[iWhichResult].fScoreSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iMatchedIons);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iTotalIons);

         // plain peptide
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

         // modified peptide
         fprintf(fpout, "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

         bool bNterm = false;
         bool bCterm = false;
         double dNterm = 0.0;
         double dCterm = 0.0;

         // We're only reporting variable mod mass differences here
         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
         {
            bNterm = true;
            dNterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass;
         }

         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            bCterm = true;
            dCterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass;
         }

         // generate modified_peptide string
         if (bNterm)
            fprintf(fpout, "n[%0.1f]", dNterm);
         for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
         {
            fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

            if (pOutput[iWhichResult].pcVarModSites[i] > 0)
            {
               fprintf(fpout, "[%0.1f]",
                     g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass);
            }
         }
         if (bCterm)
            fprintf(fpout, "c[%0.1f]", dCterm);

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


// <offset> [S|V] <value> [N|C|n|c|]
void CometWriteTxt::PrintModifications(FILE *fpout,
                                       Results *pOutput,
                                       int iWhichResult)
{
   bool bFirst = true;

   // static N-terminus protein
   if (!isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0)
         && pOutput[iWhichResult].szPrevNextAA[0] == '-')
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_S_%0.6f_N", g_staticParams.staticModifications.dAddNterminusPeptide);
   }

   // static N-terminus peptide
   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0))
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_S_%0.6f_n", g_staticParams.staticModifications.dAddNterminusProtein);
   }

   // variable N-terminus peptide and protein
   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_V_%0.6f",
            g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass);

      if (g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].iVarModTermDistance == 0)
         fprintf(fpout, "_N");
      else
         fprintf(fpout, "_n");
   }

   for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      // static modification
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0))
      {
         if (!bFirst)
            fprintf(fpout, ", ");
         else
            bFirst=false;

         fprintf(fpout, "%d_S_%0.6f",
               i+1,
               g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]]);
      }

      // variable modification
      if (g_staticParams.variableModParameters.bVarModSearch
            && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass, 0.0))
      {
         if (!bFirst)
            fprintf(fpout, ", ");
         else
            bFirst=false;

         fprintf(fpout, "%d_V_%0.6f",
               i+1,
               g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass);
      }
   }

   // static C-terminus protein
   if (!isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)
         && pOutput[iWhichResult].szPrevNextAA[1] == '-')
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_S_%0.6f_C", g_staticParams.staticModifications.dAddCterminusProtein);
   }

   // static C-terminus peptide
   if (!isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_S_%0.6f_c", g_staticParams.staticModifications.dAddCterminusPeptide);
   }

   // variable C-terminus peptide and protein
   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "%d_V_%0.6f",
            pOutput[iWhichResult].iLenPeptide,
            g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass);

      if (g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].iVarModTermDistance == 0)
         fprintf(fpout, "_C");
      else
         fprintf(fpout, "_c");
   }

   fprintf(fpout, "\t");

}
