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

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      // print out entry only if there's a valid target or decoy search hit
      if (g_pvQuery.at(i)->_pResults[0].fXcorr > XCORR_CUTOFF
            || (g_staticParams.options.iDecoySearch == 2 && g_pvQuery.at(i)->_pDecoys[0].fXcorr > XCORR_CUTOFF))
      {
         if (g_pvQuery.at(i)->_pResults[0].fXcorr > XCORR_CUTOFF)
         {
            if (!PrintResults(i, fpout, fpdb, 0))  // print search hit (could be decoy if g_staticParams.options.iDecoySearch=1)
            {
               return false;
            }
         }

         if (g_staticParams.options.iDecoySearch == 2 && g_pvQuery.at(i)->_pDecoys[0].fXcorr > XCORR_CUTOFF)
         {
            if (!PrintResults(i, fpout, fpdb, 1))  // print decoy hit
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

   if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
      iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

   bool bLabelDecoy;
   char szProteinName[WIDTH_REFERENCE];
   std::vector<ProteinEntryStruct>::iterator it;

   for (int iWhichResult=0; iWhichResult<iNumPrintLines; iWhichResult++)
   {
      if (pOutput[iWhichResult].fXcorr <= XCORR_CUTOFF)
         continue;

      fprintf(fpout, "%s_%d_%d_%d\t",    // id
            g_staticParams.inputFile.szBaseName,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState,
            iWhichResult+1);

      if (bDecoy)
         bLabelDecoy = true;
      else
      {
         bLabelDecoy = true;

         it=pOutput[iWhichResult].pWhichProtein.begin();

         // If any protein in target protein list does not match decoy prefix then set target label
         for (; it!=pOutput[iWhichResult].pWhichProtein.end(); ++it)
         {
            CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);

            if (strncmp(szProteinName, g_staticParams.szDecoyPrefix, strlen(g_staticParams.szDecoyPrefix)))
            {
               bLabelDecoy = false;
               break;
            }
         }
      }

      if (bLabelDecoy)
         fprintf(fpout, "-1\t");  // decoy label
      else
         fprintf(fpout, "1\t");   // target label

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

            // calculate deltaCn only if sequences are less than 0.75 similar
            if ( ((double) (iMinLength - iDiffCt)/iMinLength) < 0.75)
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

      PrintPercolatorSearchHit(iWhichQuery, iWhichResult, bDecoy, pOutput, fpout, fpdb, dDeltaCn, dLastDeltaCn);
   }

   return true;
}


void CometWritePercolator::PrintPercolatorSearchHit(int iWhichQuery,
                                                    int iWhichResult,
                                                    bool bDecoy,
                                                    Results *pOutput,
                                                    FILE *fpout,
                                                    FILE *fpdb,
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

   char szProteinName[WIDTH_REFERENCE];
   bool bPrintDecoyPrefix = false;
   std::vector<ProteinEntryStruct>::iterator it;

   if (bDecoy)
   {  
      it=pOutput[iWhichResult].pWhichDecoyProtein.begin();
      bPrintDecoyPrefix = true;
   }
   else
   {  
      // if not reporting separate decoys, it's possible only matches
      // in combined search are decoy the entries
      if (pOutput[iWhichResult].pWhichProtein.size() > 0)
         it=pOutput[iWhichResult].pWhichProtein.begin();
      else
      {  
         it=pOutput[iWhichResult].pWhichDecoyProtein.begin();
         bPrintDecoyPrefix = true;
      }
   }

   CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);

   if (bPrintDecoyPrefix)
      fprintf(fpout, "%s%s", g_staticParams.szDecoyPrefix, szProteinName);
   else
      fprintf(fpout, "%s", szProteinName);
         
   ++it;
   int iPrintDuplicateProteinCt = 0;

   for (; it!=(bPrintDecoyPrefix?pOutput[iWhichResult].pWhichDecoyProtein.end():pOutput[iWhichResult].pWhichProtein.end()); ++it)
   {
      CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
      if (bPrintDecoyPrefix)
         fprintf(fpout, "\t%s%s", g_staticParams.szDecoyPrefix, szProteinName);
      else
         fprintf(fpout, "\t%s", szProteinName);

      iPrintDuplicateProteinCt++;
      if (iPrintDuplicateProteinCt == g_staticParams.options.iMaxDuplicateProteins)
         break;
   }

   // If combined search printed out target proteins above, now print out decoy proteins if necessary
   if (!bDecoy && pOutput[iWhichResult].pWhichProtein.size() > 0 && pOutput[iWhichResult].pWhichDecoyProtein.size() > 0
         && iPrintDuplicateProteinCt < g_staticParams.options.iMaxDuplicateProteins)
   {
      for (it=pOutput[iWhichResult].pWhichDecoyProtein.begin(); it!=pOutput[iWhichResult].pWhichDecoyProtein.end(); ++it)
      {
         CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
         fprintf(fpout, "\t%s%s", g_staticParams.szDecoyPrefix, szProteinName);

         iPrintDuplicateProteinCt++;
         if (iPrintDuplicateProteinCt == g_staticParams.options.iMaxDuplicateProteins)
            break;
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
