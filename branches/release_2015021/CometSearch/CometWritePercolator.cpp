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


bool CometWritePercolator::WritePercolator(FILE *fpout)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      // print out entry only if there's a valid target or decoy search hit
      if (g_pvQuery.at(i)->_pResults[0].fXcorr > XCORR_CUTOFF
            || (g_staticParams.options.iDecoySearch == 2 && g_pvQuery.at(i)->_pDecoys[0].fXcorr > XCORR_CUTOFF))
      {
         if (g_pvQuery.at(i)->_pResults[0].fXcorr > XCORR_CUTOFF)
         {
            if (!PrintResults(i, fpout, 0))  // print search hit (could be decoy if g_staticParams.options.iDecoySearch=1)
            {
               return false;
            }
         }

         if (g_staticParams.options.iDecoySearch == 2 && g_pvQuery.at(i)->_pDecoys[0].fXcorr > XCORR_CUTOFF)
         {
            if (!PrintResults(i, fpout, 1))  // print decoy hit
            {
               return false;
            }
         }
      }
   }

   fflush(fpout);

   return true;
}


void CometWritePercolator::WritePercolatorHeader(FILE *fpout)
{
   // Write header line
   fprintf(fpout, "id\t");
   fprintf(fpout, "label\t");
   fprintf(fpout, "ScanNr\t");
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
   fprintf(fpout, "peptide\t");
   fprintf(fpout, "proteinId1\n");

   fflush(fpout);
}


bool CometWritePercolator::PrintResults(int iWhichQuery,
                                        FILE *fpout,
                                        bool bDecoy)
{
   int  i,
        iNumPrintLines,
        iMinLength;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   Results *pOutput;

   if (bDecoy)
   {
      pOutput = pQuery->_pDecoys;
      iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
   }
   else
   {
      pOutput = pQuery->_pResults;
      iNumPrintLines = pQuery->iMatchPeptideCount;
   }

   fprintf(fpout, "%s_%d_%d_1\t",    // id
         g_staticParams.inputFile.szBaseName,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iChargeState );

   if (bDecoy || !strncmp(pOutput[0].szProtein, g_staticParams.szDecoyPrefix, strlen(g_staticParams.szDecoyPrefix)))
      fprintf(fpout, "-1\t");  // label
   else
      fprintf(fpout, "1\t");

   fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);

   if (iNumPrintLines > (g_staticParams.options.iNumPeptideOutputLines))
      iNumPrintLines = (g_staticParams.options.iNumPeptideOutputLines);

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
   for (j=1; j<iNumPrintLines+1; j++)  // loop through remaining hits to calc dDeltaCn dLastDeltaCn
   {
      if (j<g_staticParams.options.iNumStored)
      {
         // very poor way of calculating peptide similarity but it's what we have for now
         int iDiffCt = 0;

         for (int k=0; k<iMinLength; k++)
         {
            // I-L and Q-K are same for purposes here
            if (pOutput[0].szPeptide[k] != pOutput[j].szPeptide[k])
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

   if (pOutput[0].fXcorr > 0.0 && pOutput[iNumPrintLines].fXcorr >= 0.0)
      dLastDeltaCn = 1.0 - pOutput[iNumPrintLines].fXcorr/pOutput[0].fXcorr;
   else if (pOutput[0].fXcorr > 0.0 && pOutput[iNumPrintLines].fXcorr < 0.0)
      dLastDeltaCn = 1.0;
   else
      dLastDeltaCn = 0.0;

   if (bNoDeltaCnYet)
      dDeltaCn = 1.0;

   PrintPercolatorSearchHit(iWhichQuery, 0, bDecoy, pOutput, fpout, dDeltaCn, dLastDeltaCn);

   return true;
}


void CometWritePercolator::PrintPercolatorSearchHit(int iWhichQuery,
                                                    int iWhichResult,
                                                    bool bDecoy,
                                                    Results *pOutput,
                                                    FILE *fpout,
                                                    double dDeltaCn,
                                                    double dLastDeltaCn)
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

   if ((bDecoy?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides)) > 0)
      fprintf(fpout, "%0.6f\t", log((double)(bDecoy?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides))) ); // lnNumSP
   else
      fprintf(fpout, "%0.6f\t", -20.0);

   double dMassDiff = (pQuery->_pepMassInfo.dExpPepMass - pOutput[0].dPepMass) / pOutput[0].dPepMass;
   fprintf(fpout, "%0.6f\t", dMassDiff); // dM is normalized mass diff
   fprintf(fpout, "%0.6f\t", abs(dMassDiff)); // absdM

   // construct modified peptide string
   char szModPep[512];
   szModPep[0]='\0';

   // Write the peptide sequence in the same format as an sqt file.
   sprintf(szModPep+strlen(szModPep), "%c.", pOutput[iWhichResult].szPrevNextAA[0]);

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
   {
      sprintf(szModPep+strlen(szModPep), "n%c",
            g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1]);
   }

   for (int i = 0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      sprintf(szModPep+strlen(szModPep), "%c", pOutput[iWhichResult].szPeptide[i]);

      if (g_staticParams.variableModParameters.bVarModSearch
            && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass, 0.0))
      {
         sprintf(szModPep+strlen(szModPep), "%c",
               g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[i]-1]);
      }
   }

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 1)
   {
      sprintf(szModPep+strlen(szModPep), "c%c",
            g_staticParams.variableModParameters.cModCode[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1]);
   }

   sprintf(szModPep+strlen(szModPep), ".%c", pOutput[iWhichResult].szPrevNextAA[1]);

   fprintf(fpout, "%s\t", szModPep);

   fprintf(fpout, "%s\n", pOutput[iWhichResult].szProtein);
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
