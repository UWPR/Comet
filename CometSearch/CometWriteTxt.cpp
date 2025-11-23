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
#include "CometWriteTxt.h"


CometWriteTxt::CometWriteTxt()
{
}


CometWriteTxt::~CometWriteTxt()
{
}


void CometWriteTxt::WriteTxt(FILE *fpout,
                             FILE *fpoutd,
                             FILE *fpdb)
{
   int i;

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintResults(i, 1, fpout, fpdb);
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintResults(i, 2, fpoutd, fpdb);
   }
   else
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintResults(i, 0, fpout, fpdb);
   }
}


void CometWriteTxt::PrintTxtHeader(FILE *fpout)
{
#ifdef CRUX
   fprintf(fpout, "scan");
   fprintf(fpout, "\tcharge");
   fprintf(fpout, "\tspectrum precursor m/z");
   fprintf(fpout, "\tspectrum neutral mass");
   fprintf(fpout, "\tpeptide mass");
   fprintf(fpout, "\tdelta_cn");
   fprintf(fpout, "\tsp score");
   fprintf(fpout, "\tsp rank");
   fprintf(fpout, "\txcorr score");
   fprintf(fpout, "\txcorr rank");
   fprintf(fpout, "\tb/y ions matched");
   fprintf(fpout, "\tb/y ions total");
   fprintf(fpout, "\ttotal matches/spectrum");
   fprintf(fpout, "\tsequence");
   fprintf(fpout, "\tmodified sequence");
   fprintf(fpout, "\tmodifications");
   fprintf(fpout, "\tprotein id");
   fprintf(fpout, "\tflanking aa");
   fprintf(fpout, "\te-value");
   if (g_staticParams.options.iPrintAScoreProScore)
      fprintf(fpout, "\tascorepro");
   fprintf(fpout, "\n");
#else
   fprintf(fpout, "CometVersion %s\t", g_sCometVersion.c_str());
   fprintf(fpout, "%s\t", g_staticParams.inputFile.szBaseName);
   fprintf(fpout, "%s\t", g_staticParams.szDate);
   fprintf(fpout, "%s\n", g_staticParams.databaseInfo.szDatabase);

   fprintf(fpout, "scan");
   fprintf(fpout, "\tnum");
   fprintf(fpout, "\tcharge");
   fprintf(fpout, "\texp_neutral_mass");
   fprintf(fpout, "\tcalc_neutral_mass");
   fprintf(fpout, "\te-value");
   fprintf(fpout, "\txcorr");
   fprintf(fpout, "\tdelta_cn");
   fprintf(fpout, "\tsp_score");
   fprintf(fpout, "\tions_matched");
   fprintf(fpout, "\tions_total");
   fprintf(fpout, "\tplain_peptide");
   fprintf(fpout, "\tmodified_peptide");
   if (g_staticParams.peffInfo.iPeffSearch)
      fprintf(fpout, "\tpeff_modified_peptide");
   fprintf(fpout, "\tprev_aa");
   fprintf(fpout, "\tnext_aa");
   fprintf(fpout, "\tprotein");
   fprintf(fpout, "\tprotein_count");
   fprintf(fpout, "\tmodifications");
   fprintf(fpout, "\tretention_time_sec");
   fprintf(fpout, "\tsp_rank");
   if (g_staticParams.options.iPrintAScoreProScore)
      fprintf(fpout, "\tascorepro\tascore_sitescores");
// fprintf(fpout, "\tnum_matched_peptides");
   fprintf(fpout, "\n");
#endif
}


void CometWriteTxt::PrintResults(int iWhichQuery,
                                 int iPrintTargetDecoy,
                                 FILE *fpout,
                                 FILE *fpdb)  //fpdb is file pointer for either FASTA or .idx file
{
#ifdef CRUX
   if ((iPrintTargetDecoy != 2 && g_pvQuery.at(iWhichQuery)->_pResults[0].fXcorr > g_staticParams.options.dMinimumXcorr)
         || (iPrintTargetDecoy == 2 && g_pvQuery.at(iWhichQuery)->_pDecoys[0].fXcorr > g_staticParams.options.dMinimumXcorr))
   {
      Query* pQuery = g_pvQuery.at(iWhichQuery);

      int charge = pQuery->_spectrumInfoInternal.usiChargeState;
      double spectrum_neutral_mass = pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS;
      double spectrum_mz = (spectrum_neutral_mass + charge*PROTON_MASS) / (double)charge;

      Results *pOutput;
      int iNumPrintLines;
      unsigned long iNumMatches;

      if (iPrintTargetDecoy == 2)  // decoys
      {
         pOutput = pQuery->_pDecoys;
         iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
         iNumMatches =  pQuery->_uliNumMatchedDecoyPeptides;
      }
      else  // combined or separate targets
      {
         pOutput = pQuery->_pResults;
         iNumPrintLines = pQuery->iMatchPeptideCount;
         iNumMatches =  pQuery->_uliNumMatchedPeptides;
      }

      if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
         iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

      for (int iWhichResult=0; iWhichResult<iNumPrintLines; ++iWhichResult)
      {
         if (pOutput[iWhichResult].fXcorr <= g_staticParams.options.dMinimumXcorr)
            continue;

         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);
         fprintf(fpout, "%d\t",  pQuery->_spectrumInfoInternal.usiChargeState);
         fprintf(fpout, "%0.6f\t",  spectrum_mz);
         fprintf(fpout, "%0.6f\t", spectrum_neutral_mass);
         fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass - PROTON_MASS);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fDeltaCn);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fScoreSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].usiRankSp);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fXcorr);
         fprintf(fpout, "%d\t", iWhichResult + 1);                 // assuming want index starting at 1
         fprintf(fpout, "%d\t", pOutput[iWhichResult].usiMatchedIons);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].usiTotalIons);
         fprintf(fpout, "%lu\t", iNumMatches);

         // plain peptide
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

         // modified peptide
         fprintf(fpout, "%c.", pOutput[iWhichResult].cPrevAA);

         bool bNterm = false;
         bool bCterm = false;
         double dNterm = 0.0;
         double dCterm = 0.0;

         // See if n-term variable mod needs to be reported
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] > 0)
         {
            bNterm = true;
            dNterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide]-1].dVarModMass;
         }

         // See if c-term variable mod needs to be reported
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1] > 0)
         {
            bCterm = true;
            dCterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1]-1].dVarModMass;
         }

         // generate modified_peptide string
         if (bNterm)
            fprintf(fpout, "n[%0.4f]", dNterm);
         for (int i=0; i<pOutput[iWhichResult].usiLenPeptide; ++i)
         {
            fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

            if (pOutput[iWhichResult].piVarModSites[i] != 0)
               fprintf(fpout, "[%0.4f]", pOutput[iWhichResult].pdVarModSites[i]);
         }
         if (bCterm)
            fprintf(fpout, "c[%0.4f]", dCterm);

         fprintf(fpout, ".%c\t", pOutput[iWhichResult].cNextAA);

         // prints modification encoding
         PrintModifications(fpout, pOutput, iWhichResult);

         unsigned int uiNumTotProteins = 0;
         // print protein list
         PrintProteins(fpout, fpdb, iWhichQuery, iWhichResult, iPrintTargetDecoy, &uiNumTotProteins);

         // Cleavage type
         fprintf(fpout, "\t%c%c\t", pOutput[iWhichResult].cPrevAA, pOutput[iWhichResult].cNextAA);

         // e-value
         fprintf(fpout, "%0.2E", pOutput[iWhichResult].dExpect);

         if (g_staticParams.options.iPrintAScoreProScore)
            fprintf(fpout, "\t%0.2f", pOutput[iWhichResult].fAScorePro);

         fprintf(fpout, "\n");
      }
   }

#else
   if ((iPrintTargetDecoy != 2 && g_pvQuery.at(iWhichQuery)->_pResults[0].fXcorr > g_staticParams.options.dMinimumXcorr)
         || (iPrintTargetDecoy == 2 && g_pvQuery.at(iWhichQuery)->_pDecoys[0].fXcorr > g_staticParams.options.dMinimumXcorr))
   {
      Query* pQuery = g_pvQuery.at(iWhichQuery);

      Results *pOutput;
      int iNumPrintLines;

      if (iPrintTargetDecoy == 2)  // decoys
      {
         pOutput = pQuery->_pDecoys;
         iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
      }
      else  // combined or separate targets
      {
         pOutput = pQuery->_pResults;
         iNumPrintLines = pQuery->iMatchPeptideCount;
      }

      if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
         iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

      int iMinLength = 999;
      for (int i=0; i<iNumPrintLines; ++i)
      {
         int iLen = (int)strlen(pOutput[i].szPeptide);
         if (iLen == 0)
            break;
         if (iLen < iMinLength)
            iMinLength = iLen;
      }

      int iLineCount = 1;

      for (int iWhichResult=0; iWhichResult<iNumPrintLines; ++iWhichResult)
      {
         if (pOutput[iWhichResult].fXcorr <= g_staticParams.options.dMinimumXcorr)
            continue;

         iLineCount++;

         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);

         // Print spectrum_query element.
         if (g_staticParams.options.bMango)   // Mango specific
         {
            char *pStr;

            // look for either \ or / separator so valid for Windows or Linux
            if ((pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL
               && (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
            {
               pStr = g_staticParams.inputFile.szBaseName;

            }
            else
               pStr++;  // skip separation character

            fprintf(fpout, "%s_%s.%05d.%05d.%d\t",
                  pStr,
                  pQuery->_spectrumInfoInternal.szMango,
                  pQuery->_spectrumInfoInternal.iScanNumber,
                  pQuery->_spectrumInfoInternal.iScanNumber,
                  pQuery->_spectrumInfoInternal.usiChargeState);
         }

         fprintf(fpout, "%d\t", pOutput[iWhichResult].usiRankXcorr);
         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.usiChargeState);
         fprintf(fpout, "%0.6f\t", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
         fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass - PROTON_MASS);
         fprintf(fpout, "%0.2E\t", pOutput[iWhichResult].dExpect);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fXcorr);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fDeltaCn);
         fprintf(fpout, "%0.1f\t", pOutput[iWhichResult].fScoreSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].usiMatchedIons);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].usiTotalIons);

         // plain peptide
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

         // modified peptide

         bool bNterm = false;
         bool bCterm = false;
         double dNterm = 0.0;
         double dCterm = 0.0;

         // See if n-term mod (static and/or variable) needs to be reported
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] > 0)
         {
            bNterm = true;
            dNterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide]-1].dVarModMass;
         }

         // See if c-term mod (static and/or variable) needs to be reported
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1] > 0)
         {
            bCterm = true;
            dCterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1]-1].dVarModMass;
         }

         // generate modified_peptide string
         if (pOutput[iWhichResult].cHasVariableMod)
         {
            fprintf(fpout, "%c.", pOutput[iWhichResult].cPrevAA);
            if (bNterm)
               fprintf(fpout, "n[%0.4f]", dNterm);
            for (int i = 0; i < pOutput[iWhichResult].usiLenPeptide; ++i)
            {
               fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

               if (pOutput[iWhichResult].piVarModSites[i] != 0)
                  fprintf(fpout, "[%0.4f]", pOutput[iWhichResult].pdVarModSites[i]);
            }
            if (bCterm)
               fprintf(fpout, "c[%0.4f]", dCterm);
            fprintf(fpout, ".%c\t", pOutput[iWhichResult].cNextAA);
         }
         else
         {
            fprintf(fpout, "%c.%s.%c\t", pOutput[iWhichResult].cPrevAA, pOutput[iWhichResult].szPeptide, pOutput[iWhichResult].cNextAA);
         }

         // mod string with PEFF
         if (g_staticParams.peffInfo.iPeffSearch)
         {
            fprintf(fpout, "%c.", pOutput[iWhichResult].cPrevAA);
            if (bNterm)
               fprintf(fpout, "n[%0.4f]", dNterm);
            for (int i=0; i<pOutput[iWhichResult].usiLenPeptide; ++i)
            {  
               fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);
            
               if (pOutput[iWhichResult].piVarModSites[i] < 0)
                  fprintf(fpout, "[%s]", pOutput[iWhichResult].pszMod[i]);
               else if (pOutput[iWhichResult].piVarModSites[i] > 0)
                  fprintf(fpout, "[%0.4f]", pOutput[iWhichResult].pdVarModSites[i]);
            }
            if (bCterm)
               fprintf(fpout, "c[%0.4f]", dCterm);

            fprintf(fpout, ".%c\t", pOutput[iWhichResult].cNextAA);
         }

         fprintf(fpout, "%c\t", pOutput[iWhichResult].cPrevAA);
         fprintf(fpout, "%c\t", pOutput[iWhichResult].cNextAA);

         unsigned int uiNumTotProteins = 0;

         // print protein list
         PrintProteins(fpout, fpdb, iWhichQuery, iWhichResult, iPrintTargetDecoy, &uiNumTotProteins);

         fprintf(fpout, "\t%u\t", uiNumTotProteins);

         // encoded modifications
         PrintModifications(fpout, pOutput, iWhichResult);

         // retention time seconds
         fprintf(fpout, "%0.1f\t", pQuery->_spectrumInfoInternal.fRTime);

         // Sp rank
         fprintf(fpout, "%d", pOutput[iWhichResult].usiRankSp);

         if (g_staticParams.options.iPrintAScoreProScore)
            fprintf(fpout, "\t%0.4f\t'%s'", pOutput[iWhichResult].fAScorePro, pOutput[iWhichResult].sAScoreProSiteScores.c_str());

//       // number of scored peptides
//       fprintf(fpout, "\t%u", pQuery->uiHistogramCount);

         fprintf(fpout, "\n");
      }
   }
#endif
}


// print out a comma separate list of protein refereces/accessions
void CometWriteTxt::PrintProteins(FILE *fpout,
                                  FILE *fpdb,
                                  int iWhichQuery,
                                  int iWhichResult,
                                  int iPrintTargetDecoy,
                                  unsigned int *uiNumTotProteins)
{
   std::vector<std::string> vProteinTargets;  // store vector of target protein names
   std::vector<std::string> vProteinDecoys;   // store vector of decoy protein names
   std::vector<std::string>::iterator it;

   bool bReturnFulProteinString = false;

   CometMassSpecUtils::GetProteinNameString(fpdb, iWhichQuery, iWhichResult, iPrintTargetDecoy, bReturnFulProteinString, uiNumTotProteins, vProteinTargets, vProteinDecoys);

   bool bPrintComma = false;

   if (iPrintTargetDecoy != 2)  // if not decoy only, print target proteins
   {
      for (it = vProteinTargets.begin(); it != vProteinTargets.end(); ++it)
      {
         if (bPrintComma)
            fprintf(fpout, ",");
         bPrintComma = true;
         fprintf(fpout, "%s", (*it).c_str());
      }
   }

   if (iPrintTargetDecoy != 1)  // if not target only, print decoy proteins
   {
      for (it = vProteinDecoys.begin(); it != vProteinDecoys.end(); ++it)
      {
         if (bPrintComma)
            fprintf(fpout, ",");
         bPrintComma = true;
         fprintf(fpout, "%s%s", g_staticParams.szDecoyPrefix, (*it).c_str());
      }
   }
}


// <offset> [S|V] <value> [N|C|n|c|]
// <offset> P <value>   for PEFF modificaiton
// <offset> p <residue> for PEFF substitution
void CometWriteTxt::PrintModifications(FILE *fpout,
                                       Results *pOutput,
                                       int iWhichResult)
{
   bool bFirst = true;
   bool bPrintMod = false;

   // static N-terminus protein
   if (!isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0)
         && pOutput[iWhichResult].cPrevAA == '-')
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_S_%0.6f_N", g_staticParams.staticModifications.dAddNterminusProtein);
      bPrintMod = true;
   }

   // static N-terminus peptide
   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0))
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_S_%0.6f_n", g_staticParams.staticModifications.dAddNterminusPeptide);
      bPrintMod = true;
   }

   // variable N-terminus peptide and protein
   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] > 0)
   {
      if (!bFirst)
         fprintf(fpout, ", ");
      else
         bFirst=false;

      fprintf(fpout, "1_V_%0.6f",
            g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide]-1].dVarModMass);

      if (g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide]-1].iVarModTermDistance == 0)
         fprintf(fpout, "_N");
      else
         fprintf(fpout, "_n");
      bPrintMod = true;
   }

   for (int i=0; i<pOutput[iWhichResult].usiLenPeptide; ++i)
   {
      // static modification
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0))
      {
         if (!bFirst)
            fprintf(fpout, ",");
         else
            bFirst=false;

         fprintf(fpout, "%d_S_%0.6f",
               i+1,
               g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]]);
         bPrintMod = true;
      }

      // variable modification
      if (g_staticParams.variableModParameters.bVarModSearch && pOutput[iWhichResult].piVarModSites[i] != 0)
      {
         if (!bFirst)
            fprintf(fpout, ",");
         else
            bFirst=false;

         if (g_staticParams.variableModParameters.bVarModSearch && pOutput[iWhichResult].piVarModSites[i] > 0)
            fprintf(fpout, "%d_V_%0.6f", i+1, pOutput[iWhichResult].pdVarModSites[i]);  // variable mod
         else
            fprintf(fpout, "%d_P_%0.6f", i+1, pOutput[iWhichResult].pdVarModSites[i]);  // PEFF mod
         bPrintMod = true;
      }
   }

   // static C-terminus protein
   if (!isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)
         && pOutput[iWhichResult].cNextAA == '-')
   {
      if (!bFirst)
         fprintf(fpout, ",");
      else
         bFirst=false;

      fprintf(fpout, "%d_S_%0.6f_C", pOutput[iWhichResult].usiLenPeptide, g_staticParams.staticModifications.dAddCterminusProtein);
      bPrintMod = true;
   }

   // static C-terminus peptide
   if (!isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
   {
      if (!bFirst)
         fprintf(fpout, ",");
      else
         bFirst=false;

      fprintf(fpout, "%d_S_%0.6f_c", pOutput[iWhichResult].usiLenPeptide, g_staticParams.staticModifications.dAddCterminusPeptide);
      bPrintMod = true;
   }

   // variable C-terminus peptide and protein
   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1] > 0)
   {
      if (!bFirst)
         fprintf(fpout, ",");
      else
         bFirst=false;

      fprintf(fpout, "%d_V_%0.6f",
            pOutput[iWhichResult].usiLenPeptide,
            g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1]-1].dVarModMass);

      if (g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide+1]-1].iVarModTermDistance == 0)
         fprintf(fpout, "_C");
      else
         fprintf(fpout, "_c");
      bPrintMod = true;
   }

   // PEFF amino acid substitution
   //if (pOutput[iWhichResult].cPeffOrigResidue != '\0' && pOutput[iWhichResult].iPeffOrigResiduePosition != -9)
   if (!pOutput[iWhichResult].sPeffOrigResidues.empty() && pOutput[iWhichResult].iPeffOrigResiduePosition != NO_PEFF_VARIANT)
   {
      if (!bFirst)
         fprintf(fpout, ",");
      else
         bFirst=false;

      //fprintf(fpout, "%d_p_%c", pOutput[iWhichResult].iPeffOrigResiduePosition+1, pOutput[iWhichResult].cPeffOrigResidue);
      if(pOutput[iWhichResult].sPeffOrigResidues.size()>1)
        fprintf(fpout, "%d-%d_p_%s", pOutput[iWhichResult].iPeffOrigResiduePosition + 1, pOutput[iWhichResult].iPeffOrigResiduePosition + (int)pOutput[iWhichResult].sPeffOrigResidues.size(), pOutput[iWhichResult].sPeffOrigResidues.c_str());
      else
        fprintf(fpout, "%d_p_%s", pOutput[iWhichResult].iPeffOrigResiduePosition + 1, pOutput[iWhichResult].sPeffOrigResidues.c_str());
      bPrintMod = true;
   }

   if (bPrintMod)
      fprintf(fpout, "\t");
   else
      fprintf(fpout, "-\t");
}
