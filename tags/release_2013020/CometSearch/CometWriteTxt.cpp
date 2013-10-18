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

bool CometWriteTxt::_bWroteHeader = false;

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

   if (!_bWroteHeader)
   {
      _bWroteHeader = true;
      PrintTxtHeader(fpout);

      if (g_staticParams.options.iDecoySearch == 2)
         PrintTxtHeader(fpoutd);
   }

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
   fprintf(fpout, "matches/spectrum\t");
   fprintf(fpout, "peptide mass\t");
   fprintf(fpout, "e-value\t");
   fprintf(fpout, "xcorr score\t");
   fprintf(fpout, "xcorr rank\t");
   fprintf(fpout, "delta_cn\t");
   fprintf(fpout, "sp score\t");
   fprintf(fpout, "sp rank\t");
   fprintf(fpout, "b/y ions matched\t");
   fprintf(fpout, "b/y ions total\t");
   fprintf(fpout, "sequence\t");
   fprintf(fpout, "flanking aa\t");
   fprintf(fpout, "protein id\n");
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
   char szBuf[SIZE_BUF];

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   int charge = pQuery->_spectrumInfoInternal.iChargeState;
   double spectrum_neutral_mass = pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS;
   double spectrum_mz = (spectrum_neutral_mass + charge*PROTON_MASS) / (double)charge;
   
   sprintf(szBuf, "%d\t%d\t%0.4f\t%0.4f\t%lu\t",
         pQuery->_spectrumInfoInternal.iScanNumber, 
         pQuery->_spectrumInfoInternal.iChargeState,
         spectrum_mz,
         spectrum_neutral_mass,
         pQuery->_uliNumMatchedPeptides);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   for (int i=1; i<=min((unsigned long)5, pQuery->_uliNumMatchedPeptides); i++)
   {
      fprintf(fpout, "%s", szBuf);      
      PrintTxtLine(i, pOutput, fpout);  // print top hit only right now
   }
}

#else
void CometWriteTxt::PrintResults(int iWhichQuery,
                                 bool bDecoy,
                                 FILE *fpout)
{
   char szBuf[SIZE_BUF];

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   sprintf(szBuf, "%d\t%d\t%0.4f\t",
         pQuery->_spectrumInfoInternal.iScanNumber, 
         pQuery->_spectrumInfoInternal.iChargeState, 
         pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);

   fprintf(fpout, "%s", szBuf);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   if (pOutput[0].fXcorr > 0.0)
      PrintTxtLine(1, pOutput, fpout);  // print top hit only right now
   else
      fprintf(fpout, "\n");
}
#endif

#ifdef CRUX
void CometWriteTxt::PrintTxtLine( int iWhichResult,
                                 Results *pOutput,
                                 FILE *fpout)
{
   int  i;
   char szBuf[SIZE_BUF];

   sprintf(szBuf, "%0.4f\t%0.2E\t%0.4f\t%d\t%0.4f\t%0.1f\t%d\t%d\t%d\t",
         pOutput[iWhichResult].dPepMass - PROTON_MASS,
         pOutput[iWhichResult].dExpect,
         pOutput[iWhichResult].fXcorr,
         iWhichResult,
         1.000000 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr,   // pOutput[0].fXcorr is >0 to enter this fn
         pOutput[iWhichResult].fScoreSp,
         pOutput[iWhichResult].iRankSp,
         pOutput[iWhichResult].iMatchedIons, 
         pOutput[iWhichResult].iTotalIons);

   fprintf(fpout, "%s", szBuf);
   
   szBuf[0] = '\0';
   
   //Print out peptide and give mass for variable mods.
   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
   {
      sprintf(szBuf, "[%0.4f]",  g_staticParams.variableModParameters.dVarModMassN);
   }

   for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
   {
      sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPeptide[i]);

      if (g_staticParams.variableModParameters.bVarModSearch
            && !isEqual(g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass, 0.0))
      {
         sprintf(szBuf+strlen(szBuf), "[%0.4f]",
            g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass);
      }
   }
   
   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
   {
      sprintf(szBuf+strlen(szBuf), "[0.4%f]", g_staticParams.variableModParameters.dVarModMassC);
   }

   fprintf(fpout, "%s\t", szBuf);

   fprintf(fpout, "%c%c\t", pOutput[iWhichResult].szPrevNextAA[0], pOutput[iWhichResult].szPrevNextAA[1]);

   // Print protein reference/accession.
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szProtein);
   fprintf(fpout, "\n");
}
#else
void CometWriteTxt::PrintTxtLine( int iWhichResult,
                                 Results *pOutput,
                                 FILE *fpout)
{
   int  i;
   char szBuf[SIZE_BUF];

   sprintf(szBuf, "%0.4f\t%0.2E\t%0.4f\t%0.4f\t%0.1f\t%d\t%d\t",
         pOutput[iWhichResult].dPepMass - PROTON_MASS,
         pOutput[iWhichResult].dExpect,
         pOutput[iWhichResult].fXcorr,
         1.000000 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr,   // pOutput[0].fXcorr is >0 to enter this fn
         pOutput[iWhichResult].fScoreSp,
         pOutput[iWhichResult].iMatchedIons, 
         pOutput[iWhichResult].iTotalIons);

   fprintf(fpout, "%s\t", szBuf);

   // Print plain peptide
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

   // Print peptide sequence
   sprintf(szBuf, "%c", pOutput[iWhichResult].szPrevNextAA[0]);

   if (g_staticParams.variableModParameters.bVarModSearch
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
   {
      sprintf(szBuf+strlen(szBuf), "]");
   }
   else
      sprintf(szBuf+strlen(szBuf), ".");


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
         && pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
   {
      sprintf(szBuf+strlen(szBuf), "[");
   }
   else
      sprintf(szBuf+strlen(szBuf), ".");

   sprintf(szBuf+strlen(szBuf), "%c", pOutput[iWhichResult].szPrevNextAA[1]);

   fprintf(fpout, "%s\t", szBuf);

   fprintf(fpout, "%c\t%c\t", pOutput[iWhichResult].szPrevNextAA[0], pOutput[iWhichResult].szPrevNextAA[1]);

   // Print protein reference/accession.
   fprintf(fpout, "%s\t", pOutput[iWhichResult].szProtein);

   fprintf(fpout, "%d", pOutput[iWhichResult].iDuplicateCount); 

   fprintf(fpout, "\n");
}
#endif
