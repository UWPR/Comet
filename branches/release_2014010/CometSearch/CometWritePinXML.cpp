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
#include "CometWritePinXML.h"
#include "CometStatus.h"
#include <math.h>


CometWritePinXML::CometWritePinXML()
{
}


CometWritePinXML::~CometWritePinXML()
{
}


bool CometWritePinXML::WritePinXML(FILE *fpout)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      // print out entry only if there's a valid target or decoy search hit
      if (g_pvQuery.at(i)->_pResults[0].fXcorr > XCORR_CUTOFF
            || (g_staticParams.options.iDecoySearch == 2 && g_pvQuery.at(i)->_pDecoys[0].fXcorr > XCORR_CUTOFF))
      {
         fprintf(fpout, "<fragSpectrumScan xmlns=\"http://per-colator.com/percolator_in/12\" scanNumber=\"%d\">\n",
               g_pvQuery[i]->_spectrumInfoInternal.iScanNumber);

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

         fprintf(fpout, "</fragSpectrumScan>\n");
      }
   }

   fflush(fpout);

   return true;
}

void CometWritePinXML::WritePinXMLHeader(FILE *fpout)
{
   char szEnzyme[48];
   int i;

   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   fprintf(fpout, "<experiment xmlns=\"http://per-colator.com/percolator_in/12\" ");
   fprintf(fpout, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
   fprintf(fpout, "xsi:schemaLocation=\"http://per-colator.com/percolator_in/12 ");
   fprintf(fpout, "https://github.com/percolator/percolator/raw/pin-1-2/src/xml/percolator_in.xsd\"> \n");
   
   int iLen = strlen(g_staticParams.enzymeInformation.szSampleEnzymeName);
   for (i=0; i<iLen ; i++)
   {
      szEnzyme[i] = tolower(g_staticParams.enzymeInformation.szSampleEnzymeName[i]);
      if (szEnzyme[i] == '_')
         szEnzyme[i] = '-';
   }
   szEnzyme[i]='\0';
   fprintf(fpout, "<enzyme>%s</enzyme>\n", szEnzyme);

   // These are the currently support enzyme names:
   //    no_enzyme, elastase, pepsin, proteinasek, thermolysin, chymotrypsin,
   //    lys-n, lys-c, arg-c, asp-n, glu-c, trypsin
   // The above transformation of szSampleEnzymeName will get a few of the default
   // Comet enzymes close to what Percolator expects.  Otherwise users can
   // change the enzyme names directly in the comet.params file.

   fprintf(fpout, "<process_info>\n");
   fprintf(fpout, " <command_line>comet</command_line>\n");
   fprintf(fpout, "</process_info>\n");

   fprintf(fpout, "<featureDescriptions xmlns=\"http://per-colator.com/percolator_in/12\">\n");
   fprintf(fpout, " <featureDescription name=\"lnrSp\" />\n");
   fprintf(fpout, " <featureDescription name=\"deltLCn\" />\n");
   fprintf(fpout, " <featureDescription name=\"deltCn\" />\n");
   fprintf(fpout, " <featureDescription name=\"lnExpect\" />\n");
   fprintf(fpout, " <featureDescription name=\"Xcorr\" />\n");
   fprintf(fpout, " <featureDescription name=\"Sp\" />\n");
   fprintf(fpout, " <featureDescription name=\"IonFrac\" />\n");
   fprintf(fpout, " <featureDescription name=\"Mass\" />\n");
   fprintf(fpout, " <featureDescription name=\"PepLen\" />\n");

   for (i=1 ; i<= g_staticParams.options.iMaxPrecursorCharge; i++)
      fprintf(fpout, " <featureDescription name=\"Charge%d\" />\n", i);

   fprintf(fpout, " <featureDescription name=\"enzN\" />\n");
   fprintf(fpout, " <featureDescription name=\"enzC\" />\n");
   fprintf(fpout, " <featureDescription name=\"enzInt\" />\n");
   fprintf(fpout, " <featureDescription name=\"lnNumSP\" />\n");
   fprintf(fpout, " <featureDescription name=\"dM\" />\n");
   fprintf(fpout, " <featureDescription name=\"absdM\" />\n");

   fprintf(fpout, "</featureDescriptions>\n");

   fflush(fpout);
}

void CometWritePinXML::WritePinXMLEndTags(FILE *fpout)
{
   fprintf(fpout, "</experiment>\n");
   fflush(fpout);
}

bool CometWritePinXML::PrintResults(int iWhichQuery,
                                    FILE *fpout,
                                    bool bDecoy)
{
   int  i,
        iDoXcorrCount,
        iMinLength;
   double dMZcalc,
          dMZexp;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   Results *pOutput;

   if (bDecoy)
   {
      pOutput = pQuery->_pDecoys;
      iDoXcorrCount = pQuery->iDoDecoyXcorrCount;
   }
   else
   {
      pOutput = pQuery->_pResults;
      iDoXcorrCount = pQuery->iDoXcorrCount;
   }

   dMZexp = (pQuery->_pepMassInfo.dExpPepMass + PROTON_MASS*(pQuery->_spectrumInfoInternal.iChargeState-1))
      / pQuery->_spectrumInfoInternal.iChargeState ;

   dMZcalc = (pOutput[0].dPepMass + PROTON_MASS*(pQuery->_spectrumInfoInternal.iChargeState-1))
      / pQuery->_spectrumInfoInternal.iChargeState;

   fprintf(fpout, " <peptideSpectrumMatch calculatedMassToCharge=\"%0.6f\" ", dMZcalc);
   fprintf(fpout, "chargeState=\"%d\" ", pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, "experimentalMassToCharge=\"%0.6f\" ", dMZexp);
   fprintf(fpout, "id=\"%s_%d_%d_1\" ",
         g_staticParams.inputFile.szBaseName,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iChargeState );   //basename_scannum_charge_1

   if (bDecoy || !strncmp(pOutput[0].szProtein, g_staticParams.szDecoyPrefix, strlen(g_staticParams.szDecoyPrefix)))
      fprintf(fpout, "isDecoy=\"true\">\n");
   else
      fprintf(fpout, "isDecoy=\"false\">\n");

   if (iDoXcorrCount > (g_staticParams.options.iNumPeptideOutputLines))
      iDoXcorrCount = (g_staticParams.options.iNumPeptideOutputLines);

   iMinLength = 999;
   for (i=0; i<iDoXcorrCount; i++)
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

   for (j=1; j<iDoXcorrCount; j++)  // loop through remaining hits to calc dDeltaCn dLastDeltaCn
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

   if (pOutput[0].fXcorr > 0.0 && pOutput[iDoXcorrCount].fXcorr >= 0.0)
      dLastDeltaCn = 1.0 - pOutput[iDoXcorrCount].fXcorr/pOutput[0].fXcorr;
   else if (pOutput[0].fXcorr > 0.0 && pOutput[iDoXcorrCount].fXcorr < 0.0)
      dLastDeltaCn = 1.0;
   else
      dLastDeltaCn = 0.0;

   if (bNoDeltaCnYet)
      dDeltaCn = 1.0;

   PrintPinXMLSearchHit(iWhichQuery, 0, bDecoy, pOutput, fpout, dDeltaCn, dLastDeltaCn);

   fprintf(fpout, " </peptideSpectrumMatch>\n");

   return true;
}


void CometWritePinXML::PrintPinXMLSearchHit(int iWhichQuery,
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

   fprintf(fpout, "  <features>\n");

   if (pOutput[iWhichResult].iRankSp > 0)
      fprintf(fpout, "   <feature>%0.6f</feature>\n", log((double)pOutput[iWhichResult].iRankSp) );  // lnrSp
   else
      fprintf(fpout, "   <feature>%0.6f</feature>\n", -20.0);

   fprintf(fpout, "   <feature>%0.6f</feature>\n", dLastDeltaCn); // deltLCn  last dCn in output list
   fprintf(fpout, "   <feature>%0.6f</feature>\n", dDeltaCn); // deltCn

   if (pOutput[iWhichResult].dExpect > 0.0)
      fprintf(fpout, "   <feature>%0.6f</feature>\n", log(pOutput[iWhichResult].dExpect)); // ln(Expect)
   else
      fprintf(fpout, "   <feature>%0.6f</feature>\n", -20.0);

   fprintf(fpout, "   <feature>%0.6f</feature>\n", pOutput[iWhichResult].fXcorr); // xcorr
   fprintf(fpout, "   <feature>%0.6f</feature>\n", pOutput[iWhichResult].fScoreSp); // Sp

   if (pOutput[iWhichResult].iTotalIons > 0)
      fprintf(fpout, "   <feature>%0.4f</feature>\n", (double)pOutput[iWhichResult].iMatchedIons / pOutput[iWhichResult].iTotalIons); // IonFrac
   else
      fprintf(fpout, "   <feature>%0.4f</feature>\n", 0.0);

   fprintf(fpout, "   <feature>%0.6f</feature>\n", pQuery->_pepMassInfo.dExpPepMass); // Mass is observed MH+
   fprintf(fpout, "   <feature>%d</feature>\n", pOutput[iWhichResult].iLenPeptide); // PepLen

   for (int i=1 ; i<= g_staticParams.options.iMaxPrecursorCharge; i++)
      fprintf(fpout, "   <feature>%d</feature>\n", (pQuery->_spectrumInfoInternal.iChargeState==i?1:0) );

   fprintf(fpout, "   <feature>%d</feature>\n", iNterm); // enzN
   fprintf(fpout, "   <feature>%d</feature>\n", iCterm); // enzC
   fprintf(fpout, "   <feature>%d</feature>\n", iNMC); // enzInt

   if ((bDecoy?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides)) > 0)
      fprintf(fpout, "   <feature>%0.6f</feature>\n", log((double)(bDecoy?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides))) ); // lnNumSP
   else
      fprintf(fpout, "   <feature>%0.6f</feature>\n", -20.0);

   double dMassDiff = (pQuery->_pepMassInfo.dExpPepMass - pOutput[0].dPepMass) / pOutput[0].dPepMass;
   fprintf(fpout, "   <feature>%0.6f</feature>\n", dMassDiff); // dM is normalized mass diff
   fprintf(fpout, "   <feature>%0.6f</feature>\n", abs(dMassDiff)); // absdM

   fprintf(fpout, "  </features>\n");
   fprintf(fpout, "  <peptide>\n");
   fprintf(fpout, "   <peptideSequence>%s</peptideSequence>\n", pOutput[iWhichResult].szPeptide);

   // terminal static mods
   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
         || (pOutput[iWhichResult].szPrevNextAA[0]=='-'
            && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0)) )
   {
      fprintf(fpout, "   <modification location=\"%d\">\n", 0);
      fprintf(fpout, "    <uniMod accession=\"%d\" />\n", 10); // some random number for N-term static mod
      fprintf(fpout, "   </modification>\n");
   }

   if (!isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0)
         || (pOutput[iWhichResult].szPrevNextAA[1]=='-'
            && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)) )
   {
      fprintf(fpout, "   <modification location=\"%d\">\n", pOutput[iWhichResult].iLenPeptide + 1);
      fprintf(fpout, "    <uniMod accession=\"%d\" />\n", 11); // some random number for C-term static mod
      fprintf(fpout, "   </modification>\n");
   }

   // terminal variable mods
   if (!isEqual(g_staticParams.variableModParameters.dVarModMassN, 0.0)
         && (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] != 0))  // nterm
   {
      fprintf(fpout, "   <modification location=\"%d\">\n", 0);
      fprintf(fpout, "    <uniMod accession=\"%d\" />\n", 12); // some random number for N-term variable mod
      fprintf(fpout, "   </modification>\n");
   }

   if (!isEqual(g_staticParams.variableModParameters.dVarModMassC, 0.0)
         && (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide + 1] != 0))  // cterm
   {
      fprintf(fpout, "   <modification location=\"%d\">\n", pOutput[iWhichResult].iLenPeptide + 1);
      fprintf(fpout, "    <uniMod accession=\"%d\" />\n", 13); // some random number for N-term variable mod
      fprintf(fpout, "   </modification>\n");
   }

   int i;

   // We don't have any mechanism to provide real UniMod accession numbers here.
   // So, just like sqt2pin, we are supplying bogus ones.
   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0))
      {
         // static mods - use ascii value of residue for bogus unimod number here (A=65 thru Z=90)

         fprintf(fpout, "   <modification location=\"%d\">\n", i+1);
         fprintf(fpout, "    <uniMod accession=\"%d\" />\n", (int)pOutput[iWhichResult].szPeptide[i]);
         fprintf(fpout, "   </modification>\n");
      }

      if (pOutput[iWhichResult].pcVarModSites[i] > 0)
      {
         // variable mods - use modification number i.e. 1 for variable_mod1, 2 for variable_mod2, etc.

         fprintf(fpout, "   <modification location=\"%d\">\n", i+1);
         fprintf(fpout, "    <uniMod accession=\"%d\" />\n", pOutput[iWhichResult].pcVarModSites[i]);
         fprintf(fpout, "   </modification>\n");
      }
   }

   fprintf(fpout, "  </peptide>\n");

   fprintf(fpout, "  <occurence flankC=\"%c\" flankN=\"%c\" proteinId=\"%s\" />\n", 
         pOutput[iWhichResult].szPrevNextAA[1],
         pOutput[iWhichResult].szPrevNextAA[0],
         pOutput[iWhichResult].szProtein); // wrong spelling of "occurrence" in schema
}


void CometWritePinXML::CalcNTTNMC(Results *pOutput,
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
