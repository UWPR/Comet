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
#include "CometWritePercolator.h"
#include "CometStatus.h"
#include <math.h>


CometWritePercolator::CometWritePercolator()
{
}


CometWritePercolator::~CometWritePercolator()
{
}


bool CometWritePercolator::WritePercolator(FILE *fpout,
                                           FILE *fpdb)
{
   int i;
   int iLenDecoyPrefix = strlen(g_staticParams.szDecoyPrefix);

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      if (g_pvQuery.at(i)->_pResults[0].fXcorr > g_staticParams.options.dMinimumXcorr)
      {
         PrintResults(i, fpout, fpdb, 0, iLenDecoyPrefix);  // print search hit (could be decoy if g_staticParams.options.iDecoySearch=1)
      }

      if (g_staticParams.options.iDecoySearch == 2 && g_pvQuery.at(i)->_pDecoys[0].fXcorr > g_staticParams.options.dMinimumXcorr)
      {
         PrintResults(i, fpout, fpdb, 2, iLenDecoyPrefix);  // print decoy hit
      }
   }

   fflush(fpout);

   return true;
}


void CometWritePercolator::WritePercolatorHeader(FILE *fpout)
{
   // Write header line
   fprintf(fpout, "SpecId\t");
   fprintf(fpout, "Label\t");
   fprintf(fpout, "ScanNr\t");
   fprintf(fpout, "ExpMass\t");
   fprintf(fpout, "CalcMass\t");
   fprintf(fpout, "lnrSp\t");
   fprintf(fpout, "deltLCn\t");
   fprintf(fpout, "deltCn\t");
   fprintf(fpout, "lnExpect\t");
   fprintf(fpout, "Xcorr\t");
   fprintf(fpout, "Sp\t");
   fprintf(fpout, "IonFrac\t");
   fprintf(fpout, "Mass\t");
   fprintf(fpout, "PepLen\t");
   for (int i=1 ; i<= g_staticParams.options.iMaxPrecursorCharge; i++)
      fprintf(fpout, "Charge%d\t", i);
   fprintf(fpout, "enzN\t");
   fprintf(fpout, "enzC\t");
   fprintf(fpout, "enzInt\t");
   fprintf(fpout, "lnNumSP\t");
   fprintf(fpout, "dM\t");
   fprintf(fpout, "absdM\t");
   fprintf(fpout, "Peptide\t");
   fprintf(fpout, "Proteins\n");

   fflush(fpout);
}


bool CometWritePercolator::PrintResults(int iWhichQuery,
                                        FILE *fpout,
                                        FILE *fpdb,
                                        int iPrintTargetDecoy,
                                        int iLenDecoyPrefix)
{
   int  i,
        iNumPrintLines,
        iMinLength;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   Results *pOutput;

   if (iPrintTargetDecoy == 2)  // decoys
   {
      pOutput = pQuery->_pDecoys;
      iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
   }
   else // combined or separate targets
   {
      pOutput = pQuery->_pResults;
      iNumPrintLines = pQuery->iMatchPeptideCount;
   }

   if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
      iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

   for (int iWhichResult=0; iWhichResult<iNumPrintLines; iWhichResult++)
   {
      if (pOutput[iWhichResult].fXcorr <= g_staticParams.options.dMinimumXcorr)
         continue;

      fprintf(fpout, "%s_%d_%d_%d\t",    // id
            g_staticParams.inputFile.szBaseName,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState,
            iWhichResult+1);

      std::vector<string> vProteinTargets;  // store vector of target protein names
      std::vector<string> vProteinDecoys;   // store vector of decoy protein names

      CometMassSpecUtils::GetProteinNameString(fpdb, iWhichQuery, iWhichResult, iPrintTargetDecoy, vProteinTargets, vProteinDecoys);

      if (g_staticParams.options.iDecoySearch) // using Comet's internal decoys
      {
         if (vProteinTargets.size() > 0)
            fprintf(fpout, "1\t");   // target label
         else
            fprintf(fpout, "-1\t");  // decoy label
      }
      else
      {
         // Standard database search with possible user supplied decoys in the fasta.
         // So compare the protein string to see if any match g_staticParams.szDecoyPrefix.
         // If so, annotate those with a decoy label.
         bool bTarget = false;
         std::vector<string>::iterator it;

         for (it = vProteinTargets.begin(); it != vProteinTargets.end(); it++)
         {
            if (strncmp((*it).c_str(), g_staticParams.szDecoyPrefix, iLenDecoyPrefix))
            {
               // if any protein string does not match the decoy prefix then it's a target
               bTarget = true;
               break;
            }
         }

         if (bTarget)
            fprintf(fpout, "1\t");   // target label
         else
            fprintf(fpout, "-1\t");  // decoy label
      }

      fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);
      fprintf(fpout, "%0.6f\t", pQuery->_pepMassInfo.dExpPepMass);  //ExpMass
      fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass);  //CalcMass

      iMinLength = 999;
      for (i=0; i<iNumPrintLines; i++)
      {
         int iLen = (int)strlen(pOutput[i].szPeptide);
         if (iLen == 0)
            break;
         if (iLen < iMinLength)
            iMinLength = iLen;
      }

      int j;
      bool bNoDeltaCnYet = true;
      double dDeltaCn=1.0;       // this is deltaCn between i and first dissimilar peptide
      double dLastDeltaCn=1.0;   // this is deltaCn between first and last peptide in output list

      // go one past iNumPrintLines to calculate deltaCn value
      for (j=iWhichResult+1; j<iNumPrintLines+1; j++)  // loop through remaining hits to calc dDeltaCn dLastDeltaCn
      {
         if (j<g_staticParams.options.iNumStored)
         {
            // very poor way of calculating peptide similarity but it's what we have for now
            int iDiffCt = 0;

            if (!g_staticParams.options.bExplicitDeltaCn)
            {
               for (int k=0; k<iMinLength; k++)
               {
                  // I-L and Q-K are same for purposes here
                  if (pOutput[iWhichResult].szPeptide[k] != pOutput[j].szPeptide[k])
                  {
                     if (!((pOutput[iWhichResult].szPeptide[k] == 'K' || pOutput[iWhichResult].szPeptide[k] == 'Q')
                             && (pOutput[j].szPeptide[k] == 'K' || pOutput[j].szPeptide[k] == 'Q'))
                           && !((pOutput[iWhichResult].szPeptide[k] == 'I' || pOutput[iWhichResult].szPeptide[k] == 'L')
                              && (pOutput[j].szPeptide[k] == 'I' || pOutput[j].szPeptide[k] == 'L')))
                     {
                        iDiffCt++;
                     }
                  }
               }
            }

            // calculate deltaCn only if sequences are less than 0.75 similar
            if (g_staticParams.options.bExplicitDeltaCn || ((double) (iMinLength - iDiffCt)/iMinLength) < 0.75)
            {
               if (pOutput[iWhichResult].fXcorr > 0.0 && pOutput[j].fXcorr >= 0.0)
                  dDeltaCn = 1.0 - pOutput[j].fXcorr/pOutput[iWhichResult].fXcorr;
               else if (pOutput[iWhichResult].fXcorr > 0.0 && pOutput[j].fXcorr < 0.0)
                  dDeltaCn = 1.0;
               else
                  dDeltaCn = 0.0;

               bNoDeltaCnYet = 0;

               break;
            }
         }
      }

      if (pOutput[iWhichResult].fXcorr > 0.0 && pOutput[iNumPrintLines-1].fXcorr >= 0.0)
         dLastDeltaCn = (pOutput[iWhichResult].fXcorr - pOutput[iNumPrintLines-1].fXcorr)/pOutput[iWhichResult].fXcorr;
      else if (pOutput[iWhichResult].fXcorr > 0.0 && pOutput[iNumPrintLines-1].fXcorr < 0.0)
         dLastDeltaCn = 1.0;
      else
         dLastDeltaCn = 0.0;

      if (bNoDeltaCnYet)
         dDeltaCn = 1.0;

      PrintPercolatorSearchHit(iWhichQuery, iWhichResult, iPrintTargetDecoy, pOutput, fpout, dDeltaCn, dLastDeltaCn, vProteinTargets, vProteinDecoys);
   }

   return true;
}


void CometWritePercolator::PrintPercolatorSearchHit(int iWhichQuery,
                                                    int iWhichResult,
                                                    int iPrintTargetDecoy,
                                                    Results *pOutput,
                                                    FILE *fpout,
                                                    double dDeltaCn,
                                                    double dLastDeltaCn,
                                                    vector<string> vProteinTargets,
                                                    vector<string> vProteinDecoys)
{
   int iNterm;
   int iCterm;
   int iNMC;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   CalcNTTNMC(pOutput, iWhichResult, &iNterm, &iCterm, &iNMC);

   if (pOutput[iWhichResult].iRankSp > 0)
      fprintf(fpout, "%0.6f\t", log((double)pOutput[iWhichResult].iRankSp) );  // lnrSp
   else
      fprintf(fpout, "%0.6f\t", -20.0);

   fprintf(fpout, "%0.6f\t", dLastDeltaCn); // deltLCn  last dCn in output list
   fprintf(fpout, "%0.6f\t", dDeltaCn); // deltCn

   if (pOutput[iWhichResult].dExpect > 0.0)
      fprintf(fpout, "%0.6f\t", log(pOutput[iWhichResult].dExpect)); // ln(Expect)
   else
      fprintf(fpout, "%0.6f\t", -20.0);

   fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].fXcorr); // xcorr
   fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].fScoreSp); // Sp

   if (pOutput[iWhichResult].iTotalIons > 0)
      fprintf(fpout, "%0.4f\t", (double)pOutput[iWhichResult].iMatchedIons / pOutput[iWhichResult].iTotalIons); // IonFrac
   else
      fprintf(fpout, "%0.4f\t", 0.0);

   fprintf(fpout, "%0.6f\t", pQuery->_pepMassInfo.dExpPepMass); // Mass is observed MH+
   fprintf(fpout, "%d\t", pOutput[iWhichResult].iLenPeptide); // PepLen

   for (int i=1 ; i<= g_staticParams.options.iMaxPrecursorCharge; i++)
      fprintf(fpout, "%d\t", (pQuery->_spectrumInfoInternal.iChargeState==i?1:0) );

   fprintf(fpout, "%d\t", iNterm); // enzN
   fprintf(fpout, "%d\t", iCterm); // enzC
   fprintf(fpout, "%d\t", iNMC); // enzInt

   unsigned long uliNumMatch;
   if (iPrintTargetDecoy == 2)
      uliNumMatch = pQuery->_uliNumMatchedDecoyPeptides;
   else
      uliNumMatch = pQuery->_uliNumMatchedPeptides;

   if (uliNumMatch > 0)
      fprintf(fpout, "%0.6f\t", log((double)(uliNumMatch))); // lnNumSP
   else
      fprintf(fpout, "%0.6f\t", -20.0);

   double dMassDiff = (pQuery->_pepMassInfo.dExpPepMass - pOutput[iWhichResult].dPepMass) / pOutput[iWhichResult].dPepMass;
   fprintf(fpout, "%0.6f\t", dMassDiff); // dM is normalized mass diff
   fprintf(fpout, "%0.6f\t", abs(dMassDiff)); // absdM

   bool bNterm = false;
   bool bCterm = false;
   double dNterm = 0.0;
   double dCterm = 0.0;

   // See if n-term variable mod needs to be reported
   if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      bNterm = true;
      dNterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass;
   }

   // See if c-term variable mod needs to be reported
   if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
   {
      bCterm = true;
      dCterm = g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass;
   }

   fprintf(fpout, "%c.", pOutput[iWhichResult].szPrevNextAA[0]);
   // generate modified_peptide string
   if (bNterm)
      fprintf(fpout, "n[%0.4f]", dNterm);
   // Print peptide sequence.
   for (int i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      fprintf(fpout, "%c", pOutput[iWhichResult].szPeptide[i]);

      if (pOutput[iWhichResult].piVarModSites[i] != 0)
         fprintf(fpout, "[%0.4f]", pOutput[iWhichResult].pdVarModSites[i]);
   }
   if (bCterm)
      fprintf(fpout, "c[%0.4f]", dCterm);
   fprintf(fpout, ".%c\t", pOutput[iWhichResult].szPrevNextAA[1]);

   std::vector<string>::iterator it;

   bool bPrintTab = false;
   if (iPrintTargetDecoy != 2)  // if not decoy only, print target proteins
   {
      for (it = vProteinTargets.begin(); it != vProteinTargets.end(); it++)
      {
         if (bPrintTab)
         {
            if (g_staticParams.options.bPinModProteinDelim)
               fprintf(fpout, ",");
            else
               fprintf(fpout, "\t");
         }

         fprintf(fpout, "%s", (*it).c_str());
         bPrintTab = true;
      }
   }

   if (iPrintTargetDecoy != 1)  // if not target only, print decoy proteins
   {
      for (it = vProteinDecoys.begin(); it != vProteinDecoys.end(); it++)
      {
         if (bPrintTab)
         {
            if (g_staticParams.options.bPinModProteinDelim)
               fprintf(fpout, ",");
            else
               fprintf(fpout, "\t");
         }

         fprintf(fpout, "%s", (*it).c_str());
         bPrintTab = true;
      }
   }

   fprintf(fpout,"\n");
}


void CometWritePercolator::CalcNTTNMC(Results *pOutput,
                                      int iWhichResult,
                                      int *iNterm,
                                      int *iCterm,
                                      int *iNMC)
{
   int i;
   *iNterm=0;
   *iCterm=0;
   *iNMC=0;

   // Calculate number of tolerable termini (NTT) based on sample_enzyme
   if (pOutput[iWhichResult].szPrevNextAA[0]=='-')
   {
      *iNterm = 1;
   }
   else if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[0])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[0]))
      {
         *iNterm = 1;
      }
   }
   else
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[0])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[0]))
      {
         *iNterm = 1;
      }
   }

   if (pOutput[iWhichResult].szPrevNextAA[1]=='-')
   {
      *iCterm = 1;
   }
   else if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[1]))
      {
         *iCterm = 1;
      }
   }
   else
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[1])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1]))
      {
         *iCterm = 1;
      }
   }

   // Calculate number of missed cleavage (NMC) sites based on sample_enzyme
   if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide-1; i++)
      {
         if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i+1]))
         {
            *iNMC += 1;
         }
      }
   }
   else
   {
      for (i=1; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i-1]))
         {
            *iNMC += 1;
         }
      }
   }

}
