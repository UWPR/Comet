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
#include "CometData.h"
#include "CometMassSpecUtils.h"
#include "CometWritePinXML.h"


CometWritePinXML::CometWritePinXML()
{
}


CometWritePinXML::~CometWritePinXML()
{
}


void CometWritePinXML::WritePinXML(FILE *fpout)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      fprintf(fpout, "<fragSpectrumScan xmlns=\"http://per-colator.com/percolator_in/12\" scanNumber=\"%d\">\n",
            g_pvQuery[i]->_spectrumInfoInternal.iScanNumber);

      PrintResults(i, fpout, 0);  // print target hit
      PrintResults(i, fpout, 1);  // print decoy hit

      fprintf(fpout, "</fragSpectrumScan>\n");
   }

   fflush(fpout);
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
   
   for (i=0; i<strlen(g_StaticParams.enzymeInformation.szSampleEnzymeName); i++)
   {
      szEnzyme[i] = tolower(g_StaticParams.enzymeInformation.szSampleEnzymeName[i]);
      if (szEnzyme[i] == '_')
         szEnzyme[i] = '-';
   }
   szEnzyme[i]='\0';
   fprintf(fpout, "\n<enzyme>%s</enzyme>\n\n", szEnzyme);

   // These are the currently support enzyme names:
   //    no_enzyme, elastase, pepsin, proteinasek, thermolysin, chymotrypsin,
   //    lys-n, lys-c, arg-c, asp-n, glu-c, trypsin
   // The above transformation of szSampleEnzymeName will get a few of the default
   // Comet enzymes close to what Percolator expects.  Otherwise users can
   // change the enzyme names directly in the comet.params file.

   fprintf(fpout, "<process_info>\n");
   fprintf(fpout, " <command_line>comet</command_line>\n");
   fprintf(fpout, "</process_info>\n\n");

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

   for (i=1 ; i<= g_StaticParams.options.iMaxPrecursorCharge; i++)
      fprintf(fpout, " <featureDescription name=\"Charge%d\" />\n", i);

   fprintf(fpout, " <featureDescription name=\"enzN\" />\n");
   fprintf(fpout, " <featureDescription name=\"enzC\" />\n");
   fprintf(fpout, " <featureDescription name=\"enzInt\" />\n");
   fprintf(fpout, " <featureDescription name=\"lnNumSP\" />\n");
   fprintf(fpout, " <featureDescription name=\"dM\" />\n");
   fprintf(fpout, " <featureDescription name=\"absdM\" />\n");

   fprintf(fpout, "</featureDescriptions>\n\n");

   fflush(fpout);
}

void CometWritePinXML::WritePinXMLEndTags(FILE *fpout)
{
   fprintf(fpout, "</experiment>\n");
   fflush(fpout);
}

void CometWritePinXML::PrintResults(int iWhichQuery,
                                    FILE *fpout,
                                    bool bDecoy)
{
   int  i,
        iDoXcorrCount,
        iRankXcorr,
        iMinLength;
   double dMZ,
          dMZexp,
          dMZdiff;

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

   dMZ = (pQuery->_pepMassInfo.dExpPepMass + PROTON_MASS*(pQuery->_spectrumInfoInternal.iChargeState-1))
      / pQuery->_spectrumInfoInternal.iChargeState ;

   dMZexp = (pOutput[0].dPepMass + PROTON_MASS*(pQuery->_spectrumInfoInternal.iChargeState-1))
      / pQuery->_spectrumInfoInternal.iChargeState;

   dMZdiff = dMZ - dMZexp;

   fprintf(fpout, " <peptideSpectrumMatch calculatedMassToCharge=\"%0.6f\" ", dMZexp);
   fprintf(fpout, "chargeState=\"%d\" ", pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, "experimentalMassToCharge=\"%0.6f\" ", dMZ);
   fprintf(fpout, "id=\"%s_%d_%d_1\" ",
         g_StaticParams.inputFile.szBaseName,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iChargeState );   //basename_scannum_charge_1
   fprintf(fpout, "isDecoy=\"%s\">\n", bDecoy?"true":"false");

   if (iDoXcorrCount > (g_StaticParams.options.iNumPeptideOutputLines))
      iDoXcorrCount = (g_StaticParams.options.iNumPeptideOutputLines);

   iRankXcorr = 1;

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
         dDeltaCn = (pOutput[0].fXcorr - pOutput[j].fXcorr) / pOutput[0].fXcorr;
         bNoDeltaCnYet = 0;

         break;
      }
   }

   if (pOutput[0].fXcorr > 0.0)
      dLastDeltaCn = (pOutput[0].fXcorr - pOutput[iDoXcorrCount].fXcorr) / pOutput[0].fXcorr;
   else
      dLastDeltaCn = 1.0;

   if (bNoDeltaCnYet)
      dDeltaCn = 1.0;

   PrintPinXMLSearchHit(iWhichQuery, iRankXcorr, 0, iDoXcorrCount, bDecoy,
         pOutput, fpout, dDeltaCn, dLastDeltaCn, dMZ, dMZdiff);

   fprintf(fpout, " </peptideSpectrumMatch>\n");
}


void CometWritePinXML::PrintPinXMLSearchHit(int iWhichQuery,
                                            int iRankXcorr,
                                            int iWhichResult,
                                            int iDoXcorrCount,
                                            bool bDecoy,
                                            Results *pOutput,
                                            FILE *fpout,
                                            double dDeltaCn,
                                            double dLastDeltaCn,
                                            double dMZ,
                                            double dMZdiff)
{
   int iNterm;
   int iCterm;
   int iNMC;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   CalcNTTNMC(pOutput, iWhichResult, &iNterm, &iCterm, &iNMC);

   fprintf(fpout, "  <features>\n");
   fprintf(fpout, "   <feature>%0.6f</feature>\n", (double)log((double)pOutput[iWhichResult].iRankSp) );  // lnrSp
   fprintf(fpout, "   <feature>%0.6f</feature>\n", dLastDeltaCn); // deltLCn  last dCn in output list
   fprintf(fpout, "   <feature>%0.6f</feature>\n", dDeltaCn); // deltCn
   fprintf(fpout, "   <feature>%0.6f</feature>\n", (double)log(pOutput[iWhichResult].dExpect)); // ln(Expect)
   fprintf(fpout, "   <feature>%0.6f</feature>\n", pOutput[iWhichResult].fXcorr); // xcorr
   fprintf(fpout, "   <feature>%0.6f</feature>\n", pOutput[iWhichResult].fScoreSp); // Sp
   fprintf(fpout, "   <feature>%0.4f</feature>\n", (double)pOutput[iWhichResult].iMatchedIons / pOutput[iWhichResult].iTotalIons); // IonFrac
   fprintf(fpout, "   <feature>%0.6f</feature>\n",dMZ); // Mass is m/z
   fprintf(fpout, "   <feature>%d</feature>\n", pOutput[iWhichResult].iLenPeptide); // PepLen

   for (int i=1 ; i<= g_StaticParams.options.iMaxPrecursorCharge; i++)
      fprintf(fpout, "   <feature>%d</feature>\n", (pQuery->_spectrumInfoInternal.iChargeState==i?1:0) );

   fprintf(fpout, "   <feature>%d</feature>\n", iNterm); // enzN
   fprintf(fpout, "   <feature>%d</feature>\n", iCterm); // enzC
   fprintf(fpout, "   <feature>%d</feature>\n", iNMC); // enzInt
   fprintf(fpout, "   <feature>%0.6f</feature>\n", (double)log((double)(bDecoy?(pQuery->_liNumMatchedDecoyPeptides):(pQuery->_liNumMatchedPeptides))) ); // lnNumSP
   fprintf(fpout, "   <feature>%0.6f</feature>\n", dMZdiff); // dM  is m/z diff
   fprintf(fpout, "   <feature>%0.6f</feature>\n", abs(dMZdiff)); // absdM
   fprintf(fpout, "  </features>\n");
   fprintf(fpout, "  <peptide>\n");
   fprintf(fpout, "   <peptideSequence>%s</peptideSequence>\n", pOutput[iWhichResult].szPeptide);


   if (g_StaticParams.staticModifications.dAddNterminusPeptide != 0.0
         || (pOutput[iWhichResult].szPrevNextAA[0]=='-'
            && g_StaticParams.staticModifications.dAddNterminusProtein != 0.0))
   {
      fprintf(fpout, "   <modification location=\"%d\">\n", 0);
      fprintf(fpout, "    <uniMod accession=\"%d\" />\n", 10); // some random number for N-term mod
      fprintf(fpout, "   </modification>\n");
   }

   if (g_StaticParams.staticModifications.dAddCterminusPeptide != 0.0
         || (pOutput[iWhichResult].szPrevNextAA[1]=='-'
            && g_StaticParams.staticModifications.dAddCterminusProtein != 0.0))
   {
      fprintf(fpout, "   <modification location=\"%d\">\n", pOutput[iWhichResult].iLenPeptide + 1);
      fprintf(fpout, "    <uniMod accession=\"%d\" />\n", 11); // some random number for C-term mod
      fprintf(fpout, "   </modification>\n");
   }

   int i;

   // We don't supply any mechanism to provide real UniMod accession numbers here.
   // So, just like sqt2pin, we are supplying bogus ones.
   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      if (g_StaticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]] != 0.0)
      {
         fprintf(fpout, "   <modification location=\"%d\">\n", i+1);
         // using ascii value of residue for bogus unimod number here
         fprintf(fpout, "    <uniMod accession=\"%d\" />\n",
               (int)g_StaticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]]);
         fprintf(fpout, "   </modification>\n");
      }
      else if (pOutput[iWhichResult].pcVarModSites[i] > 0)
      {
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


void CometWritePinXML::GetVal(char *szElement,
                              char *szAttribute,
                              char *szAttributeVal)
{
   char *pStr;

   if ((pStr=strstr(szElement, szAttribute)))
   {
      strncpy(szAttributeVal, pStr+strlen(szAttribute)+2, SIZE_FILE);  // +2 to skip ="

      if ((pStr=strchr(szAttributeVal, '"')))
      {
         *pStr='\0';
         return;
      }
      else
      {
         strcpy(szAttributeVal, "unknown");  // Error - expecting an end quote in szAttributeVal.
         return;
      }
   }
   else
   {
      strcpy(szAttributeVal, "unknown"); // Attribute not found.
      return;
   }
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
   else if (g_StaticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[0])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[0]))
      {
         *iNterm = 1;
      }
   }
   else
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[0])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[0]))
      {
         *iNterm = 1;
      }
   }

   if (pOutput[iWhichResult].szPrevNextAA[1]=='-')
   {
      *iCterm = 1;
   }
   else if (g_StaticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[1]))
      {
         *iCterm = 1;
      }
   }
   else
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[1])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1]))
      {
         *iCterm = 1;
      }
   }

   // Calculate number of missed cleavage (NMC) sites based on sample_enzyme
   if (g_StaticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide-1; i++)
      {
         if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i+1]))
         {
            *iNMC += 1;
         }
      }
   }
   else
   {
      for (i=1; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i-1]))
         {
            *iNMC += 1;
         }
      }
   }

}
