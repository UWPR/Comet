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
#include "ThreadPool.h"
#include "CometPostAnalysis.h"
#include "CometMassSpecUtils.h"
#include "CometStatus.h"

#include "AScoreFactory.h"
#include "AScoreOptions.h"
#include "AScoreOutput.h"
#include "AScoreDllInterface.h"
#include "Centroid.h"
#include "PeptideBuilder.h"
#include "Mass.h"


#include "CometDecoys.h"  // this is where decoyIons[EXPECT_DECOY_SIZE] is initialized


CometPostAnalysis::CometPostAnalysis()
{
}


CometPostAnalysis::~CometPostAnalysis()
{
}


bool CometPostAnalysis::PostAnalysis(ThreadPool* tp)
{
   bool bSucceeded = true;

   //Reuse existing ThreadPool
   ThreadPool *pPostAnalysisThreadPool = tp;

   for (int i=0; i<(int)g_pvQuery.size(); ++i)
   {
      if (g_pvQuery.at(i)->iMatchPeptideCount > 0 || g_pvQuery.at(i)->iDecoyMatchPeptideCount > 0)
      {
         PostAnalysisThreadData* pThreadData = new PostAnalysisThreadData(i);

         pPostAnalysisThreadPool->doJob(std::bind(PostAnalysisThreadProc, pThreadData, pPostAnalysisThreadPool));

         pThreadData = NULL;
         bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
         if (!bSucceeded)
         {
            break;
         }
      }
   }

   // Wait for active post analysis threads to complete processing.

   pPostAnalysisThreadPool->wait_on_threads();
   
   pPostAnalysisThreadPool = NULL;

   // Check for errors one more time since there might have been an error
   // while we were waiting for the threads.
   if (bSucceeded)
   {
      bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
   }

   return bSucceeded;
}


void CometPostAnalysis::PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData,
                                               ThreadPool* tp)
{
   int iQueryIndex = pThreadData->iQueryIndex;

   AnalyzeSP(iQueryIndex);

   // Calculate E-values if necessary.
   // Only time to not calculate E-values is for .out/.sqt output only and
   // user decides to not replace Sp score with E-values
   if (g_staticParams.options.bPrintExpectScore
         || g_staticParams.options.bOutputPepXMLFile
         || g_staticParams.options.bOutputPercolatorFile
         || g_staticParams.options.bOutputTxtFile)
   {
      if (g_pvQuery.at(iQueryIndex)->iMatchPeptideCount > 0
            || g_pvQuery.at(iQueryIndex)->iDecoyMatchPeptideCount > 0)
      {
         CalculateEValue(iQueryIndex, 0);
      }
   }

   // this has to happen after AnalyzeSP as results are sorted in that fn
   CalculateDeltaCn(iQueryIndex);

   if (g_staticParams.options.bPrintAScoreProScore)
   {
      using namespace AScoreProCpp;

      // Create the AScoreDllInterface using the factory function
      AScoreDllInterface* ascoreInterface = CreateAScoreDllInterface();
      if (!ascoreInterface) {
         std::cerr << "Failed to create AScore interface." << std::endl;
         exit(1);
      }

      // Set up AScore options; move this to somewhere earlier at head of search
//      g_AScoreOptions = ascoreInterface->GetDefaultOptions();
//      g_AScoreOptions.setSymbol('#'); // Phosphorylation symbol
//      g_AScoreOptions.setResidues("STY"); // Phosphorylation residues

      CalculateAScorePro(iQueryIndex, ascoreInterface);

      // Clean up
      DeleteAScoreDllInterface(ascoreInterface);
   }

   delete pThreadData;
   pThreadData = NULL;
}


void CometPostAnalysis::CalculateDeltaCn(int iWhichQuery)
{
   int iNumPrintLines;
   int iMinLength;

   Results* pOutput;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   pOutput = pQuery->_pResults;

   iNumPrintLines = pQuery->iMatchPeptideCount;

   // extend 1 past iNumPeptideOutputLines need for deltaCn calculation of last entry
   if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines + 1)
      iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines + 1;

   iMinLength = 999;
   for (int i = 0; i < iNumPrintLines; ++i)
   {
      int iLen = (int)strlen(pOutput[i].szPeptide);
      if (iLen == 0)
         break;
      if (iLen < iMinLength)
         iMinLength = iLen;
   }

   int iRankXcorr = 1;

   int iLastEntry = 0;
   for (int iWhichResult = 0; iWhichResult < iNumPrintLines; ++iWhichResult)
      if (pOutput[iWhichResult].fXcorr > FLOAT_ZERO)
         iLastEntry = iWhichResult;

   for (int iWhichResult = 0; iWhichResult < iNumPrintLines; ++iWhichResult)
   {
      int j;
      bool bNoDeltaCnYet = true;
      double dDeltaCn = 0.0;       // this is deltaCn between top hit and peptide in list (or next dissimilar peptide)

      for (j = iWhichResult + 1; j < iNumPrintLines + 1; ++j)
      {
         if (j < g_staticParams.options.iNumStored)
         {
            // very poor way of calculating peptide similarity but it's what we have for now
            int iDiffCt = 0;

            if (!g_staticParams.options.bExplicitDeltaCn)
            {
               for (int k = 0; k < iMinLength; ++k)
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
            }

            // calculate deltaCn only if sequences are less than 0.75 similar
            if (g_staticParams.options.bExplicitDeltaCn || ((double)(iMinLength - iDiffCt) / iMinLength) < 0.75)
            {
               if (pOutput[iWhichResult].fXcorr > FLOAT_ZERO && pOutput[j].fXcorr > FLOAT_ZERO)
                  dDeltaCn = 1.0 - (pOutput[j].fXcorr / pOutput[iWhichResult].fXcorr);
               else if (pOutput[iWhichResult].fXcorr > 0.0 && pOutput[j].fXcorr < 0.0)
                  dDeltaCn = 0.0;
               else
                  dDeltaCn = 0.0;

               bNoDeltaCnYet = false;

               break;
            }
         }
      }

      if (bNoDeltaCnYet || iNumPrintLines == 1)
         dDeltaCn = 0.0;

      if (iWhichResult > 0 && !isEqual(pOutput[iWhichResult].fXcorr, pOutput[iWhichResult - 1].fXcorr))
         iRankXcorr++;

      double dLastDeltaCn = 1.0;  // this is deltaCn between first and last peptide in output list

      if (g_staticParams.options.bExportAdditionalScoresPepXML)
      {
         if (pOutput[iWhichResult].fXcorr > FLOAT_ZERO && pOutput[iLastEntry].fXcorr > FLOAT_ZERO)
            dLastDeltaCn = 1.0 - (pOutput[iLastEntry].fXcorr / pOutput[iWhichResult].fXcorr);
         else if (pOutput[iWhichResult].fXcorr > 0.0 && pOutput[iLastEntry].fXcorr < 0.0)
            dLastDeltaCn = 0.0;
         else
            dLastDeltaCn = 0.0;
      }

      pOutput[iWhichResult].fDeltaCn = (float)dDeltaCn;
      pOutput[iWhichResult].fLastDeltaCn = (float)dLastDeltaCn;
      pOutput[iWhichResult].iRankXcorr = iRankXcorr;
   }
}


void CometPostAnalysis::AnalyzeSP(int iWhichQuery)
{
   Query* pQuery = g_pvQuery.at(iWhichQuery);

   // need this sort first for all iNumStored hits
   std::sort(pQuery->_pResults, pQuery->_pResults + g_staticParams.options.iNumStored, SortFnXcorr);

   int iSize = pQuery->iMatchPeptideCount;

   // Need to analyze up to iNumStored here so that Sp rank can range to this
   if (iSize > g_staticParams.options.iNumStored)
      iSize = g_staticParams.options.iNumStored;

   // Target search
   CalculateSP(pQuery->_pResults, iWhichQuery, iSize);

   std::sort(pQuery->_pResults, pQuery->_pResults + iSize, SortFnSp);

   pQuery->_pResults[0].iRankSp = 1;

   for (int ii=1; ii<iSize; ++ii)
   {
      // Determine score rankings
      if (isEqual(pQuery->_pResults[ii].fScoreSp, pQuery->_pResults[ii-1].fScoreSp))
         pQuery->_pResults[ii].iRankSp = pQuery->_pResults[ii-1].iRankSp;
      else
         pQuery->_pResults[ii].iRankSp = pQuery->_pResults[ii-1].iRankSp + 1;
   }

   // Then sort each entry in descending order by xcorr
   std::sort(pQuery->_pResults, pQuery->_pResults + iSize, SortFnXcorr);

   // if mod search, now sort peptides with same score but different mod locations
   if (g_staticParams.variableModParameters.bVarModSearch)
   {
      for (int ii=0; ii<iSize; ++ii)
      {
         int j=ii+1;

         // increment j if fXcorr is same and peptide is the same; this implies multiple
         // different mod forms of this peptide
         while (j<iSize && (pQuery->_pResults[j].fXcorr == pQuery->_pResults[ii].fXcorr)
               && !strcmp(pQuery->_pResults[j].szPeptide,  pQuery->_pResults[ii].szPeptide))
         {
            j++;
         }

         if (j>ii+1)
         {
            std::sort(pQuery->_pResults + ii, pQuery->_pResults + j, SortFnMod);
         }

         ii=j-1;
      }
   }

   // Repeat for decoy search
   if (g_staticParams.options.iDecoySearch == 2)
   {
      // need this sort first for all iNumStored hits
      std::sort(pQuery->_pDecoys, pQuery->_pDecoys + g_staticParams.options.iNumStored, SortFnXcorr);

      iSize = pQuery->iDecoyMatchPeptideCount;

      if (iSize > g_staticParams.options.iNumPeptideOutputLines)
         iSize = g_staticParams.options.iNumPeptideOutputLines;

      CalculateSP(pQuery->_pDecoys, iWhichQuery, iSize);

      std::sort(pQuery->_pDecoys, pQuery->_pDecoys + iSize, SortFnSp);
      pQuery->_pDecoys[0].iRankSp = 1;

      for (int ii=1; ii<iSize; ++ii)
      {
         // Determine score rankings
         if (isEqual(pQuery->_pDecoys[ii].fScoreSp, pQuery->_pDecoys[ii-1].fScoreSp))
            pQuery->_pDecoys[ii].iRankSp = pQuery->_pDecoys[ii-1].iRankSp;
         else
            pQuery->_pDecoys[ii].iRankSp = pQuery->_pDecoys[ii-1].iRankSp + 1;
      }

      // Then sort each entry by xcorr
      std::sort(pQuery->_pDecoys, pQuery->_pDecoys + iSize, SortFnXcorr);

      // if mod search, now sort peptides with same score but different mod locations
      if (g_staticParams.variableModParameters.bVarModSearch)
      {
         for (int ii=0; ii<iSize; ++ii)
         {
            int j=ii+1;

            while (j<iSize && (pQuery->_pDecoys[j].fXcorr == pQuery->_pDecoys[ii].fXcorr)
                  && !strcmp(pQuery->_pDecoys[j].szPeptide, pQuery->_pDecoys[ii].szPeptide))
            {
               j++;
            }

            if (j>ii+1)
            {
               std::sort(pQuery->_pDecoys + ii, pQuery->_pDecoys + j, SortFnMod);
            }

            ii=j-1;
         }
      }
   }
}


void CometPostAnalysis::CalculateSP(Results *pOutput,
                                    int iWhichQuery,
                                    int iSize)
{
   int i;
   double pdAAforward[MAX_PEPTIDE_LEN];
   double pdAAreverse[MAX_PEPTIDE_LEN];
   IonSeriesStruct ionSeries[9];

   int  _iSizepiVarModSites = sizeof(int)*MAX_PEPTIDE_LEN_P2;

   for (i = 0; i < iSize; ++i)
   {
      if (!g_staticParams.iIndexDb)
      {
         // hijack here to make protein vector unique
         if (pOutput[i].pWhichProtein.size() > 1)
         {
            sort(pOutput[i].pWhichProtein.begin(), pOutput[i].pWhichProtein.end(), ProteinEntryCmp);

            // erase duplicate proteins in the list
            comet_fileoffset_t lPrev = 0;
            for (std::vector<ProteinEntryStruct>::iterator it = pOutput[i].pWhichProtein.begin(); it != pOutput[i].pWhichProtein.end(); )
            {
               if ((*it).lWhichProtein == lPrev)
                  it = pOutput[i].pWhichProtein.erase(it);
               else
               {
                  lPrev = (*it).lWhichProtein;
                  ++it;
               }
            }
         }

         if (g_staticParams.options.iDecoySearch && pOutput[i].pWhichDecoyProtein.size() > 1)
         {
            sort(pOutput[i].pWhichDecoyProtein.begin(), pOutput[i].pWhichDecoyProtein.end(), ProteinEntryCmp);

            comet_fileoffset_t lPrev = 0;
            for (std::vector<ProteinEntryStruct>::iterator it = pOutput[i].pWhichDecoyProtein.begin(); it != pOutput[i].pWhichDecoyProtein.end(); )
            {
               if ((*it).lWhichProtein == lPrev)
                  it = pOutput[i].pWhichDecoyProtein.erase(it);
               else
               {
                  lPrev = (*it).lWhichProtein;
                  ++it;
               }
            }
         }
      }

      if (pOutput[i].iLenPeptide > 0 && pOutput[i].fXcorr > g_staticParams.options.dMinimumXcorr) // take care of possible edge case
      {
         int  ii;
         int  ctCharge;
         double dFragmentIonMass = 0.0;
         double dConsec = 0.0;
         double dBion = g_staticParams.precalcMasses.dNtermProton;
         double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

         double dTmpIntenMatch = 0.0;

         int iMatchedFragmentIonCt = 0;
         int iMaxFragCharge;

         // if no variable mods are used in search, clear piVarModSites here
         if (!g_staticParams.variableModParameters.bVarModSearch)
            memset(pOutput[i].piVarModSites, 0, _iSizepiVarModSites);

         iMaxFragCharge = g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iMaxFragCharge;

         if (pOutput[i].cPrevAA == '-' || pOutput[i].bClippedM)
            dBion += g_staticParams.staticModifications.dAddNterminusProtein;
         if (pOutput[i].cNextAA == '-')
            dYion += g_staticParams.staticModifications.dAddCterminusProtein;

         if (g_staticParams.variableModParameters.bVarModSearch
               && (pOutput[i].piVarModSites[pOutput[i].iLenPeptide] > 0))
         {
            dBion += g_staticParams.variableModParameters.varModList[pOutput[i].piVarModSites[pOutput[i].iLenPeptide]-1].dVarModMass;
         }

         if (g_staticParams.variableModParameters.bVarModSearch
               && (pOutput[i].piVarModSites[pOutput[i].iLenPeptide + 1] > 0))
         {
            dYion += g_staticParams.variableModParameters.varModList[pOutput[i].piVarModSites[pOutput[i].iLenPeptide+1]-1].dVarModMass;
         }

         for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ++ii)
         {
            int iii;

            for (iii=1; iii<=iMaxFragCharge; ++iii)
               ionSeries[g_staticParams.ionInformation.piSelectedIonSeries[ii]].bPreviousMatch[iii] = 0;
         }

         int iCountNLB[VMODS][MAX_PEPTIDE_LEN];  // sum/count of # of varmods counting from n-term at each residue position
         int iCountNLY[VMODS][MAX_PEPTIDE_LEN];  // sum/count of # of varmods counting from c-term at each position

         if (g_staticParams.variableModParameters.bUseFragmentNeutralLoss)
         {
            for (int x=0; x<VMODS; x++)
            {
               memset(iCountNLB[x], 0, sizeof(int)*MAX_PEPTIDE_LEN);
               memset(iCountNLY[x], 0, sizeof(int)*MAX_PEPTIDE_LEN);
            }
         }

         // Generate pdAAforward for _pResults[0].szPeptide.
         int iLenMinus1 = pOutput[i].iLenPeptide - 1;
         for (ii=0; ii<iLenMinus1; ++ii)
         {
            int iPos = iLenMinus1 - ii;

            if (g_staticParams.variableModParameters.bUseFragmentNeutralLoss)
            {
               if (ii > 0)
               {
                  for (int x = 0 ; x < VMODS; x++)
                  {
                     iCountNLB[x][ii] = iCountNLB[x][ii-1]; // running sum/count of # of var mods contained at position i
                     iCountNLY[x][ii] = iCountNLY[x][ii-1]; // running sum/count of # of var mods contained at position i (R to L in sequence)
                  }
               }
            }

            dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[i].szPeptide[ii]];
            dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[i].szPeptide[iPos]];

            if (g_staticParams.variableModParameters.bVarModSearch)
            {
               if (pOutput[i].piVarModSites[ii] != 0)
               {
                  dBion += pOutput[i].pdVarModSites[ii];

                  int iMod = pOutput[i].piVarModSites[ii];

                  if (iMod > 0)
                  {
                     if (g_staticParams.options.bScaleFragmentNL)
                        iCountNLB[iMod-1][ii] += 1;
                     else
                        iCountNLB[iMod-1][ii] = 1;
                  }
               }

               if (pOutput[i].piVarModSites[iPos] != 0)
               {
                  dYion += pOutput[i].pdVarModSites[iPos];

                  int iMod = pOutput[i].piVarModSites[iPos];

                  if (iMod > 0)
                  {
                     if (g_staticParams.options.bScaleFragmentNL)
                        iCountNLY[iMod-1][ii] += 1;
                     else
                        iCountNLY[iMod-1][ii] = 1;
                  }
               }
            }

            pdAAforward[ii] = dBion;
            pdAAreverse[ii] = dYion;
         }

         int iMax = g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE;

         for (ctCharge=1; ctCharge<=iMaxFragCharge; ++ctCharge)
         {
            for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ++ii)
            {
               int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ii];

               // As both _pdAAforward and _pdAAreverse are increasing, loop through
               // iLenPeptide-1 to complete set of internal fragment ions.
               for (int iii=0; iii<pOutput[i].iLenPeptide-1; ++iii)
               {
                  // Gets fragment ion mass.
                  dFragmentIonMass = CometMassSpecUtils::GetFragmentIonMass(iWhichIonSeries, iii, ctCharge, pdAAforward, pdAAreverse);

                  if (dFragmentIonMass > FLOAT_ZERO)
                  {
                     int iFragmentIonMass = BIN(dFragmentIonMass);
                     float fSpScore;

                     double dAddConsecutive = 0.0;
                     int iAddMatchedFragment = 0;

                     fSpScore = FindSpScore(g_pvQuery.at(iWhichQuery), iFragmentIonMass, iMax);

                     if (fSpScore > FLOAT_ZERO)
                     {
                        iAddMatchedFragment = 1;

                        // Simple sum intensity.
                        dTmpIntenMatch += fSpScore;

                        // Increase score for consecutive fragment ion series matches.
                        if (ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge])
                           dAddConsecutive = 0.075;

                        ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge] = 1;
                     }
                     else
                     {
                        ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge] = 0;
                     }

                     if (g_staticParams.variableModParameters.bUseFragmentNeutralLoss)
                     {
                        for (int iMod = 0; iMod < VMODS; iMod++)
                        {
                           for (int iWhichNL = 0; iWhichNL < 2; ++iWhichNL)
                           {
                              if (iWhichIonSeries <= 2)  // abc ions
                              {
                                 if (iCountNLB[iMod][iii] > 0)
                                 {
                                    int iScaleFactor = iCountNLB[iMod][iii];
                                    double dNewMass;
 
                                    if (iWhichNL == 0)
                                    {
                                       if (g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss == 0.0)
                                          continue;
                                       dNewMass = dFragmentIonMass - (iScaleFactor * g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss / ctCharge);
                                    }
                                    else
                                    {
                                       if (g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss2 == 0.0)
                                          continue;
                                       dNewMass = dFragmentIonMass - (iScaleFactor * g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss2 / ctCharge);
                                    }

                                    if (dNewMass >= 0)
                                    {
                                       int iFragmentIonMass = BIN(dNewMass);

                                       fSpScore = FindSpScore(g_pvQuery.at(iWhichQuery), iFragmentIonMass, iMax);

                                       if (fSpScore > FLOAT_ZERO)
                                       {
                                          iAddMatchedFragment = 1;

                                          // Simple sum intensity.
                                          dTmpIntenMatch += fSpScore;

                                          // Increase score for consecutive fragment ion series matches.
                                          if (ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge])
                                             dAddConsecutive = 0.075;

                                          ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge] = 1;
                                       }
                                       else
                                       {
                                          ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge] = 0;
                                       }
                                    }
                                 }
                              }
                              else // xyz ions
                              {
                                 if (iCountNLY[iMod][iii] > 0)
                                 {
                                    int iScaleFactor = iCountNLY[iMod][iii];
                                    double dNewMass;

                                    if (iWhichNL == 0)
                                    {
                                       if (g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss == 0.0)
                                          continue;
                                       dNewMass = dFragmentIonMass - (iScaleFactor * g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss / ctCharge);
                                    }
                                    else
                                    {
                                       if (g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss2 == 0.0)
                                          continue;
                                       dNewMass = dFragmentIonMass - (iScaleFactor * g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss2 / ctCharge);
                                    }

                                    if (dNewMass >= 0)
                                    {
                                       int iFragmentIonMass = BIN(dNewMass);

                                       fSpScore = FindSpScore(g_pvQuery.at(iWhichQuery), iFragmentIonMass, iMax);

                                       if (fSpScore > FLOAT_ZERO)
                                       {
                                          iAddMatchedFragment = 1;

                                          // Simple sum intensity.
                                          dTmpIntenMatch += fSpScore;

                                          // Increase score for consecutive fragment ion series matches.
                                          if (ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge])
                                             dAddConsecutive = 0.075;

                                          ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge] = 1;
                                       }
                                       else
                                       {
                                          ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge] = 0;
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }

                     dConsec += dAddConsecutive;  // move this here after fragment neutral loss analysis
                     iMatchedFragmentIonCt += iAddMatchedFragment;
                  }
               }
            }
         }

         pOutput[i].fScoreSp = (float)((dTmpIntenMatch * iMatchedFragmentIonCt * (1.0 + dConsec)) /
            ((pOutput[i].iLenPeptide - 1.0) * iMaxFragCharge * g_staticParams.ionInformation.iNumIonSeriesUsed));
         // round Sp to 3 significant digits
         pOutput[i].fScoreSp =  (float)(( ((int)pOutput[i].fScoreSp)  * 100)  / 100.0);

         pOutput[i].iMatchedIons = iMatchedFragmentIonCt;
      }
   }
}

using namespace AScoreProCpp;

void CometPostAnalysis::CalculateAScorePro(int iWhichQuery,
                                           AScoreDllInterface* ascoreInterface)
{
   std::string sequence;
   double precursorMz;
   int precursorCharge;

   precursorCharge = g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState;
   precursorMz = (g_pvQuery.at(iWhichQuery)->_pResults[0].dPepMass + (precursorCharge -1) * PROTON_MASS) / precursorCharge;

   // Generate peptide sequence of format "K.M0LAES1DDSGDEES1VSQTDK.T" where the mod char is the mod 1
   sequence = g_pvQuery.at(iWhichQuery)->_pResults[0].cPrevAA + std::string(".");
   for (int i = 0; i < g_pvQuery.at(iWhichQuery)->_pResults[0].iLenPeptide; ++i)
   {
      sequence += g_pvQuery.at(iWhichQuery)->_pResults[0].szPeptide[i];

      if (g_staticParams.variableModParameters.bVarModSearch && g_pvQuery.at(iWhichQuery)->_pResults[0].piVarModSites[i] != 0)
      {
         if (g_pvQuery.at(iWhichQuery)->_pResults[0].piVarModSites[i] > 0)
         {
            sequence += std::to_string(g_pvQuery.at(iWhichQuery)->_pResults[0].piVarModSites[i] - 1);
         }
         else
         {
            sequence += "?";    // PEFF:  no clue how to specify mod encoding
         }
      }
   }
   sequence += std::string(".") + g_pvQuery.at(iWhichQuery)->_pResults[0].cNextAA;

   sequence = "R.MQNNSSPS1ISPNTSFTSDGSPSPLGGIK.-"; // test sequence

   // construct sequence with preceeding/following residues and old mod chars

   std::vector<AScoreProCpp::Centroid> peaks = {
{201.0987, 10813},
{202.0827, 2069},
{208.1086, 13174},
{209.7756, 8544},
{209.7820, 10454},
{211.1446, 3672},
{212.0671, 7997},
{212.1035, 14268},
{216.0992, 2143},
{219.0773, 2671},
{220.6469, 2066},
{220.6525, 1940},
{226.0827, 4637},
{226.1195, 2190},
{229.0938, 27821},
{236.1034, 16668},
{237.0880, 1771},
{242.1867, 2295},
{243.0803, 17092},
{243.1098, 16197},
{252.1713, 2411},
{254.0775, 2720},
{254.1141, 7730},
{260.1070, 89275},
{260.1974, 11834},
{261.1108, 2402},
{264.1341, 1865},
{268.1660, 7753},
{270.1455, 2069},
{270.1817, 3691},
{271.1042, 2229},
{272.1727, 2047},
{277.6593, 2077},
{280.1042, 1911},
{280.1659, 11298},
{284.1065, 7587},
{285.1562, 2017},
{288.1565, 2647},
{295.1039, 1922},
{295.1407, 4314},
{297.1927, 2418},
{298.1151, 9424},
{298.1766, 9345},
{299.0988, 2608},
{304.2712, 3853},
{304.2807, 3487},
{305.1249, 4378},
{312.1018, 21037},
{312.1307, 2298},
{313.1510, 14402},
{316.1257, 11265},
{317.2190, 28953},
{323.0989, 6636},
{323.1355, 17447},
{325.1879, 20258},
{329.1282, 3695},
{329.1573, 2824},
{333.0835, 2434},
{334.0805, 2597},
{339.1131, 2420},
{339.1419, 4585},
{339.2032, 4230},
{340.1259, 23970},
{349.1261, 2604},
{349.1884, 3945},
{350.1100, 8875},
{350.1481, 2698},
{357.1236, 7106},
{357.1512, 13166},
{367.1364, 30765},
{367.1982, 21047},
{367.2342, 3681},
{368.1210, 7737},
{373.1656, 2783},
{374.1496, 41176},
{374.2403, 112622},
{375.2435, 9856},
{377.1823, 2442},
{382.1726, 7944},
{385.1470, 18735},
{385.2087, 13394},
{390.2132, 4605},
{392.1207, 3065},
{395.2295, 9916},
{400.1832, 12507},
{401.1582, 3407},
{408.1636, 2229},
{408.2245, 12127},
{409.1469, 7302},
{410.1315, 1988},
{410.2766, 27567},
{412.2195, 4170},
{418.2094, 14597},
{419.1696, 3056},
{419.9804, 2168},
{421.1125, 3132},
{426.1447, 10375},
{426.1742, 4517},
{426.2344, 2248},
{427.1582, 3780},
{436.2200, 25795},
{437.1776, 2502},
{437.2039, 2660},
{438.2718, 26039},
{439.2747, 2209},
{443.1713, 25936},
{444.1154, 2404},
{444.1858, 3001},
{447.1635, 3173},
{448.1475, 2135},
{452.2511, 8620},
{453.1552, 8508},
{454.1400, 4117},
{454.2299, 3763},
{455.1527, 2436},
{456.2819, 10010},
{460.1981, 24294},
{460.2145, 2400},
{461.1419, 8246},
{465.1736, 4119},
{469.2034, 2603},
{470.1825, 4082},
{471.1659, 28143},
{477.1839, 2622},
{478.1689, 13861},
{479.1533, 2546},
{481.1501, 2890},
{482.2612, 2580},
{483.1237, 3080},
{487.2133, 2490},
{487.2315, 3255},
{487.3250, 15596},
{488.1929, 82274},
{489.1959, 11669},
{491.2621, 8830},
{495.1662, 4034},
{495.1957, 3828},
{496.1790, 3545},
{497.3083, 7502},
{505.2417, 21634},
{506.1649, 1938},
{509.2736, 12773},
{511.2106, 2814},
{511.2305, 2256},
{512.1927, 12290},
{512.2130, 2229},
{513.2084, 2219},
{516.1862, 8097},
{523.1603, 2698},
{523.2510, 29788},
{524.1749, 2659},
{524.2540, 3036},
{525.3033, 2790},
{529.2193, 14369},
{529.2411, 9609},
{530.2024, 7242},
{533.2119, 3844},
{533.2351, 2236},
{534.1956, 11675},
{535.1555, 7697},
{539.2632, 2614},
{540.1872, 10258},
{541.1725, 3560},
{547.2516, 8024},
{550.2625, 3376},
{551.2217, 2818},
{556.2560, 7871},
{557.2135, 15258},
{557.2769, 2668},
{558.1979, 10425},
{564.1860, 2229},
{566.3662, 12378},
{567.2597, 3170},
{575.1562, 3900},
{575.2249, 27125},
{576.2272, 2909},
{577.2258, 2246},
{578.2940, 3627},
{582.1965, 2404},
{584.3771, 266398},
{585.3802, 45247},
{588.2772, 2238},
{591.2001, 2389},
{592.1826, 19325},
{594.3611, 12155},
{596.3044, 10883},
{601.1058, 2071},
{601.2739, 3206},
{602.2565, 2569},
{604.3458, 4051},
{608.2253, 6625},
{609.2087, 41236},
{610.1900, 13320},
{616.2733, 7324},
{618.2997, 2389},
{619.2832, 2545},
{620.2444, 3449},
{622.3563, 33364},
{623.3597, 2576},
{626.2357, 66850},
{626.2960, 3782},
{627.2162, 30521},
{628.2231, 2281},
{629.2686, 2244},
{630.2891, 3096},
{632.1708, 3446},
{634.2826, 3126},
{635.3145, 2063},
{637.2457, 3064},
{637.2929, 6734},
{640.3661, 9311},
{644.2458, 50983},
{645.2496, 8076},
{646.2947, 4395},
{647.2780, 7833},
{648.2394, 2787},
{648.2986, 7761},
{649.1973, 7391},
{653.3992, 2768},
{662.2532, 2944},
{664.2570, 2228},
{664.3043, 10092},
{671.4094, 28438},
{672.4120, 8931},
{681.3929, 12065},
{690.2466, 3860},
{695.2853, 2565},
{695.3181, 3266},
{697.3521, 2718},
{699.3119, 3768},
{706.2613, 7241},
{716.3002, 3509},
{717.3200, 8693},
{722.3299, 2147},
{723.2861, 4140},
{724.2739, 4679},
{731.3373, 2508},
{733.3273, 8609},
{734.3096, 4147},
{734.3437, 3016},
{735.3303, 3472},
{741.2971, 4155},
{741.3421, 1951},
{748.3979, 7867},
{750.4506, 27711},
{751.4525, 3638},
{752.2951, 2253},
{757.2730, 2274},
{757.3273, 3850},
{758.2574, 4520},
{760.2294, 2641},
{762.2817, 3506},
{764.4180, 11803},
{764.9172, 3454},
{765.2986, 3024},
{765.4213, 4457},
{766.4100, 6916},
{768.4614, 366860},
{769.3475, 2592},
{769.4642, 91932},
{770.4680, 3000},
{774.3530, 2920},
{775.2829, 19140},
{775.3364, 3089},
{776.2685, 3897},
{777.2684, 2591},
{792.3081, 13667},
{793.2914, 20194},
{794.2959, 2782},
{804.3537, 2481},
{809.3654, 2871},
{810.3201, 12533},
{811.3224, 3934},
{822.3674, 2739},
{824.3777, 3311},
{832.3478, 9046},
{837.4831, 3087},
{838.4872, 2993},
{843.3475, 2377},
{844.3558, 2742},
{845.3403, 2788},
{849.3152, 2772},
{850.3569, 16929},
{855.4937, 66389},
{856.4961, 17869},
{860.3714, 9621},
{862.3139, 2720},
{862.3636, 3010},
{863.4268, 3016},
{870.3555, 4211},
{871.3410, 3949},
{872.4312, 3409},
{872.9244, 3871},
{873.2599, 3711},
{877.4009, 4431},
{878.3831, 12727},
{879.3864, 3681},
{881.4386, 13508},
{881.9377, 12527},
{888.3658, 30516},
{889.3599, 14348},
{890.3641, 3714},
{891.2707, 9546},
{894.5047, 30285},
{895.4094, 9391},
{895.5078, 3305},
{896.3981, 3472},
{905.3936, 26451},
{906.3765, 38974},
{907.3810, 17105},
{908.2980, 9864},
{912.5152, 312746},
{913.3639, 2034},
{913.4423, 2957},
{913.5183, 91308},
{914.5193, 4437},
{919.3766, 2892},
{923.4051, 30694},
{924.4008, 20450},
{924.9537, 2940},
{937.3887, 11075},
{941.4126, 12860},
{947.4061, 4198},
{948.3990, 2058},
{953.3896, 2296},
{957.3871, 7480},
{958.3867, 12814},
{959.3757, 8390},
{965.4119, 3334},
{966.4152, 2967},
{968.4713, 3790},
{974.4125, 3794},
{975.4013, 20308},
{976.4044, 22063},
{977.3855, 3642},
{986.3431, 3807},
{987.3314, 2432},
{992.4272, 28424},
{993.4119, 24151},
{994.4143, 20789},
{995.4132, 7818},
{1003.3732, 2052},
{811.3224, 3934},
{822.3674, 2739},
{824.3777, 3311},
{832.3478, 9046},
{837.4831, 3087},
{838.4872, 2993},
{843.3475, 2377},
{844.3558, 2742},
{845.3403, 2788},
{849.3152, 2772},
{850.3569, 16929},
{855.4937, 66389},
{856.4961, 17869},
{860.3714, 9621},
{862.3139, 2720},
{862.3636, 3010},
{863.4268, 3016},
{870.3555, 4211},
{871.3410, 3949},
{872.4312, 3409},
{872.9244, 3871},
{873.2599, 3711},
{877.4009, 4431},
{878.3831, 12727},
{879.3864, 3681},
{881.4386, 13508},
{881.9377, 12527},
{888.3658, 30516},
{889.3599, 14348},
{890.3641, 3714},
{891.2707, 9546},
{894.5047, 30285},
{895.4094, 9391},
{895.5078, 3305},
{896.3981, 3472},
{905.3936, 26451},
{906.3765, 38974},
{907.3810, 17105},
{908.2980, 9864},
{912.5152, 312746},
{913.3639, 2034},
{913.4423, 2957},
{913.5183, 91308},
{914.5193, 4437},
{919.3766, 2892},
{923.4051, 30694},
{924.4008, 20450},
{924.9537, 2940},
{937.3887, 11075},
{941.4126, 12860},
{947.4061, 4198},
{948.3990, 2058},
{953.3896, 2296},
{957.3871, 7480},
{958.3867, 12814},
{959.3757, 8390},
{965.4119, 3334},
{966.4152, 2967},
{968.4713, 3790},
{974.4125, 3794},
{975.4013, 20308},
{976.4044, 22063},
{977.3855, 3642},
{986.3431, 3807},
{987.3314, 2432},
{992.4272, 28424},
{993.4119, 24151},
{994.4143, 20789},
{995.4132, 7818},
{1003.3732, 2052},
{1004.3555, 11203},
{1009.5325, 9308},
{1010.4380, 28054},
{1010.5302, 3925},
{1011.3956, 8114},
{1021.3828, 11861},
{1022.3640, 6322},
{1027.5421, 81969},
{1028.4457, 8864},
{1028.5459, 31614},
{1035.4391, 2799},
{1039.3921, 8834},
{1045.4237, 3577},
{1053.4487, 2436},
{1055.5229, 2325},
{1063.4320, 11938},
{1064.5358, 12116},
{1065.0295, 3968},
{1069.5104, 2969},
{1072.4751, 2177},
{1073.3752, 3523},
{1073.5365, 11168},
{1074.0388, 11328},
{1081.4434, 11271},
{1082.4393, 2478},
{1091.3866, 11767},
{1096.5653, 14767},
{1097.5668, 8477},
{1099.0442, 2879},
{1108.0496, 4642},
{1108.4116, 12659},
{1108.5500, 4246},
{1109.3999, 2464},
{1114.5745, 122328},
{1115.5082, 3462},
{1115.5782, 50997},
{1125.4545, 2095},
{1126.4208, 3621},
{1142.4750, 2691},
{1160.4856, 7289},
{1161.4833, 2194},
{1178.4973, 8546},
{1179.6030, 4000},
{1197.6101, 14629},
{1198.6156, 7542},
{1215.6222, 132930},
{1216.5521, 3901},
{1216.6260, 60381},
{1234.5605, 9506},
{1235.5608, 2516},
{1246.5536, 2406},
{1247.5181, 9434},
{1248.5111, 2549},
{1265.5284, 11346},
{1266.5315, 2191},
{1303.6104, 3948},
{1342.6047, 2629},
{1344.6780, 10639},
{1345.6873, 4023},
{1360.6125, 12139},
{1361.6079, 2729},
{1362.6918, 86807},
{1363.6962, 40162},
{1364.6963, 7629},
{1378.6180, 3882},
{1386.6537, 2801},
{1388.6322, 8050},
{1409.6227, 2306},
{1431.7147, 13515},
{1432.7145, 4133},
{1449.7236, 100341},
{1450.7267, 53598},
{1451.7286, 8292},
{1457.6556, 8247},
{1467.6912, 3272},
{1476.6594, 2595},
{1502.6899, 2871},
{1514.7627, 2666},
{1518.9027, 2206},
{1532.7712, 14059},
{1533.7629, 3281},
{1544.6930, 3374},
{1545.6761, 2354},
{1550.7709, 65101},
{1551.7732, 34356},
{1562.6965, 2975},
{1589.7043, 2533},
{1597.7495, 4156},
{1602.7616, 2336},
{1615.7623, 8076},
{1616.7646, 3350},
{1646.7955, 9123},
{1647.7922, 8090},
{1648.7822, 7942},
{1664.8163, 31460},
{1665.8134, 16819},
{1666.8074, 2902},
{1684.7836, 2807},
{1702.7897, 4280},
{1703.7883, 2999},
{1708.8381, 2413},
{1718.8474, 2692},
{1725.8387, 15425},
{1726.8403, 10285},
{1727.8420, 2771},
{1743.8585, 64135},
{1744.8544, 60962},
{1745.8541, 17505},
{1761.8684, 319543},
{1762.8708, 248537},
{1763.8751, 38943},
{1794.8710, 2607},
{1812.8837, 14523},
{1813.8717, 11393},
{1814.8638, 8202},
{1819.9034, 2666},
{1830.8900, 66846},
{1831.8890, 62029},
{1832.8885, 16726},
{1848.9005, 304569},
{1849.9041, 234572},
{1850.9053, 41939},
{1910.8893, 2754},
{1943.9723, 4300},
{1944.9656, 8047},
{1961.9855, 36116},
{1962.9884, 32477},
{1963.9974, 4052},
{2003.8525, 2279},
{2030.9941, 6940},
{2032.0039, 3036},
{2049.0144, 19301},
{2050.0254, 12309},
{2128.0637, 19406},
{2129.0671, 14352},
{2130.0537, 3260},
{2146.0732, 80887},
{2147.0769, 66718},
{2148.0879, 13078},
{2198.0850, 2701},
{2215.0864, 12536},
{2216.0977, 11431},
{2266.1323, 2677},
{2267.0984, 8308},
{2268.0886, 11434},
{2284.1121, 12881},
{2285.1150, 14820},
{2286.1353, 2548},
{2302.1182, 3038},
{2303.1426, 3706},
{2400.1082, 2242} };


   using namespace AScoreProCpp;

   // Calculate AScore using the DLL interface
   AScoreOutput result = ascoreInterface->CalculateScoreWithOptions(sequence,
      peaks, precursorMz, precursorCharge, g_AScoreOptions);

   // CalculateScoreWithOptions:
   //   Scan scan =  CreateScanFromCentroids(peaks, precursorMz, precursorCharge);
   //   Peptide peptide = ParsePeptideString(peptideSequence, g_AScoreOptions);
   //   peptide.setPrecursorMz(precursorMz);
   //   return CalculateScore(peptide, scan, g_AScoreOptions);
   //             AScoreCalculator calculator(g_AScoreOptions);
   //             return calculator.calculateScore(peptide, scan);

   // Print results
   std::cout << endl << "Original sequence: " << sequence << "\n";
   std::cout << "Peptides scored: " << result.peptides.size() << "\n";
   std::cout << "Sites scored: " << result.sites.size() << "\n";
   std::cout << "Best peptide: " << result.peptides[0].toString() << "\n";
   std::cout << "Score: " << result.peptides[0].getScore() << "\n";

   // Output site positions and scores (up to 6 sites)
   std::cout << "Site scores: ";
   for (size_t i = 0; i < 6; ++i)
   {
      if (i < result.sites.size())
         std::cout << result.sites[i].getScore() << " (" << result.sites[i].getPosition() << +") \t";
      else
         break;
   }
   std::cout << "\n";

   exit(0);
}


bool CometPostAnalysis::ProteinEntryCmp(const struct ProteinEntryStruct &a,
                                        const struct ProteinEntryStruct &b)
{
   if (a.lWhichProtein == b.lWhichProtein)
      return a.iStartResidue < b.iStartResidue;
   else
      return a.lWhichProtein < b.lWhichProtein;
}


bool CometPostAnalysis::SortFnSp(const Results &a,
                                 const Results &b)
{
   if (a.fScoreSp > b.fScoreSp)
      return true;
   else if (a.fScoreSp == b.fScoreSp)
   {
      int iCmp = strcmp(a.szPeptide, b.szPeptide);

      if (iCmp < 0)
         return true;
      else if (iCmp > 0)
         return false;
      else  // same peptide, check mod state
      {
         for (int i = 0; i < g_staticParams.options.peptideLengthRange.iEnd; ++i)
         {
            if (a.piVarModSites[i] < b.piVarModSites[i])
               return true;
            else if (a.piVarModSites[i] > b.piVarModSites[i])
               return false;
         }
      }
   }

   return false;
}


bool CometPostAnalysis::SortFnXcorr(const Results &a,
                                    const Results &b)
{
   if (a.fXcorr > b.fXcorr)
      return true;
   else if (a.fXcorr == b.fXcorr)
   {
      int iCmp = strcmp(a.szPeptide, b.szPeptide);

      if (iCmp < 0)
         return true;
      else if (iCmp > 0)
         return false;
      else  // same peptide, check mod state
      {
         for (int i = 0; i < a.iLenPeptide + 2; ++i)
         {
            if (a.piVarModSites[i] < b.piVarModSites[i])
               return true;
            else if (a.piVarModSites[i] > b.piVarModSites[i])
               return false;
         }
      }
   }

   return false;
}


bool CometPostAnalysis::SortSpecLibFnXcorrMS1(const SpecLibResultsMS1& a,
                                              const SpecLibResultsMS1& b)
{
   if (a.fXcorr > b.fXcorr)
      return true;
   else
      return false;
}


bool CometPostAnalysis::SortFnMod(const Results &a,
                                  const Results &b)
{
   // must compare character at a time
   // actually not sure why strcmp doesn't work
   // as piVarModSites is a char array
   for (int i = 0; i < a.iLenPeptide + 2; ++i)
   {
      if (a.piVarModSites[i] < b.piVarModSites[i])
         return true;
      else if (a.piVarModSites[i] > b.piVarModSites[i])
         return false;
   }

   return false;
}


bool CometPostAnalysis::CalculateEValue(int iWhichQuery,
                                        bool bTopHitOnly)
{
   int i;
   int *piHistogram;
   int iMaxCorr;
   int iStartCorr;
   int iNextCorr;
   double dSlope;
   double dIntercept;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   piHistogram = pQuery->iXcorrHistogram;

   if (pQuery->uiHistogramCount < EXPECT_DECOY_SIZE)
   {
      if (!GenerateXcorrDecoys(iWhichQuery))
      {
         return false;
      }
   }

   LinearRegression(piHistogram, &dSlope, &dIntercept, &iMaxCorr, &iStartCorr, &iNextCorr);

   pQuery->fPar[0] = (float)dIntercept;  // b
   pQuery->fPar[1] = (float)dSlope    ;  // m
   pQuery->fPar[2] = (float)iStartCorr;
   pQuery->fPar[3] = (float)iNextCorr;
   pQuery->siMaxXcorr = (short)iMaxCorr;

   dSlope *= 10.0; // Used in pow() function so do multiply outside of for loop.

   int iLoopCount;

   iLoopCount = max(pQuery->iMatchPeptideCount, pQuery->iDecoyMatchPeptideCount);

   if (iLoopCount > g_staticParams.options.iNumPeptideOutputLines)
      iLoopCount = g_staticParams.options.iNumPeptideOutputLines;

   for (i=0; i<iLoopCount; ++i)
   {
      if (dSlope >= 0.0)
      {
         if (i<pQuery->iMatchPeptideCount)
            pQuery->_pResults[i].dExpect = 999.0;
         if (i<pQuery->iDecoyMatchPeptideCount)
            pQuery->_pDecoys[i].dExpect = 999.0;
      }
      else
      {
         double dExpect;
         if (i<pQuery->iMatchPeptideCount)
         {
            dExpect = pow(10.0, dSlope * pQuery->_pResults[i].fXcorr + dIntercept);
            if (dExpect > 999.0)
               dExpect = 999.0;
            pQuery->_pResults[i].dExpect = dExpect;
         }

         if (i<pQuery->iDecoyMatchPeptideCount)
         {
            dExpect = pow(10.0, dSlope * pQuery->_pDecoys[i].fXcorr + dIntercept);
            if (dExpect > 999.0)
               dExpect = 999.0;
            pQuery->_pDecoys[i].dExpect = dExpect;
         }
      }

      if (bTopHitOnly)
         break;
   }

   return true;
}


void CometPostAnalysis::LinearRegression(int *piHistogram,
                                         double *slope,
                                         double *intercept,
                                         int *iMaxXcorr,
                                         int *iStartXcorr,
                                         int *iNextXcorr)
{
   double Sx, Sxy;      // Sum of square distances.
   double Mx, My;       // means
   double b, a;
   double SumX, SumY;   // Sum of X and Y values to calculate mean.

   double pdCumulative[HISTO_SIZE];  // Cummulative frequency at each xcorr value.
   memset(pdCumulative, 0, sizeof(pdCumulative));

   int i;
   int iNextCorr;    // 2nd best xcorr index
   int iMaxCorr=0;   // max xcorr index
   int iStartCorr;
   int iNumPoints;

   // Find maximum correlation score index.
   for (i=HISTO_SIZE-2; i>=0; i--)
   {
      if (piHistogram[i] > 0)
         break;
   }
   iMaxCorr = i;

   iNextCorr = 0;
   bool bFoundFirstNonZeroEntry = false;

   for (i=0; i<iMaxCorr; ++i)
   {
      if (piHistogram[i] == 0 && bFoundFirstNonZeroEntry && i >= 10)
      {
         // register iNextCorr if there's a histo value of 0 consecutively
         if (piHistogram[i+1] == 0 || i+1 == iMaxCorr)
         {
            if (i>0)
               iNextCorr = i-1;
            break;
         }
      }
      if (piHistogram[i] != 0)
         bFoundFirstNonZeroEntry = true;
   }

   if (i==iMaxCorr)
   {
      iNextCorr = iMaxCorr;

      if (iMaxCorr >= 10)
      {
         for (i = iMaxCorr; i >= iMaxCorr - 5; --i)
         {
            if (piHistogram[i] == 0)
            {
               iNextCorr = i;

               if (iMaxCorr <= 20)
                  break;
            }
         }
         if (iNextCorr == iMaxCorr)
            iNextCorr = iMaxCorr - 1;
      }
   }

   // Create cumulative distribution function from iNextCorr down, skipping the outliers.
   pdCumulative[iNextCorr] = piHistogram[iNextCorr];
   for (i=iNextCorr-1; i>=0; i--)
   {
      pdCumulative[i] = pdCumulative[i+1] + piHistogram[i];
      if (piHistogram[i+1] == 0)
         pdCumulative[i+1] = 0.0;
   }

   // log10
   for (i=iNextCorr; i>=0; i--)
   {
      piHistogram[i] = (int)pdCumulative[i];  // First store cumulative in histogram.
      if (pdCumulative[i] > 0.0)
         pdCumulative[i] = log10(pdCumulative[i]);
      else
      {
         if (pdCumulative[i+1] > 0.0)
            pdCumulative[i] = log10(pdCumulative[i+1]);
         else
            pdCumulative[i] = 0.0;
      }
   }

   iStartCorr = iNextCorr - 5;
   int iNumZeroes = 0;
   for (i=iStartCorr; i<=iNextCorr; ++i)
      if (pdCumulative[i] == 0)
         iNumZeroes++;

   iStartCorr -= iNumZeroes;

   if (iStartCorr < 0)
      iStartCorr = 0;

   Mx=My=a=b=0.0;

   while (iStartCorr >= 0 && iNextCorr > iStartCorr + 2)
   {
      Sx=Sxy=SumX=SumY=0.0;
      iNumPoints=0;

      // Calculate means.
      for (i=iStartCorr; i<=iNextCorr; ++i)
      {
         if (piHistogram[i] > 0)
         {
            SumY += (float)pdCumulative[i];
            SumX += i;
            iNumPoints++;
         }
      }

      if (iNumPoints > 0)
      {
         Mx = SumX / iNumPoints;
         My = SumY / iNumPoints;
      }
      else
         Mx = My = 0.0;

      // Calculate sum of squares.
      for (i=iStartCorr; i<=iNextCorr; ++i)
      {
         if (pdCumulative[i] > 0)
         {
            double dX;
            double dY;

            dX = i - Mx;
            dY = pdCumulative[i] - My;

            Sx  += dX*dX;
            Sxy += dX*dY;
         }
      }

      if (Sx > 0)
         b = Sxy / Sx;   // slope
      else
         b = 0;

      if (b < 0.0)
         break;
      else
         iStartCorr--;
   }

   a = My - b*Mx;  // y-intercept

   *slope = b;
   *intercept = a;
   *iMaxXcorr = iMaxCorr;
   *iStartXcorr = iStartCorr;
   *iNextXcorr = iNextCorr;
}


// Make synthetic decoy spectra to fill out correlation histogram by going
// through each candidate peptide and rotating spectra in m/z space.
bool CometPostAnalysis::GenerateXcorrDecoys(int iWhichQuery)
{
   int i;
   int ii;
   int j;
   int k;
   int iMaxFragCharge;
   int ctCharge;
   double dBion;
   double dYion;
   double dFastXcorr;
   double dFragmentIonMass = 0.0;

   int *piHistogram;

   int iFragmentIonMass;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   piHistogram = pQuery->iXcorrHistogram;

   iMaxFragCharge = pQuery->_spectrumInfoInternal.iMaxFragCharge;

   // EXPECT_DECOY_SIZE is the minimum # of decoys required or else this function isn't
   // called.  So need to generate iLoopMax more xcorr scores for the histogram.
   int iLoopMax = EXPECT_DECOY_SIZE - pQuery->uiHistogramCount;
   int iLastEntry;

   // Determine if using target or decoy peptides to rotate to fill out histogram.
   if (pQuery->iMatchPeptideCount >= pQuery->iDecoyMatchPeptideCount)
      iLastEntry = pQuery->iMatchPeptideCount;
   else
      iLastEntry = pQuery->iDecoyMatchPeptideCount;

   if (iLastEntry > g_staticParams.options.iNumStored)
      iLastEntry = g_staticParams.options.iNumStored;

   j=0;
   for (i=0; i<iLoopMax; ++i)  // iterate through required # decoys
   {
      dFastXcorr = 0.0;

      for (j=0; j<MAX_DECOY_PEP_LEN; ++j)  // iterate through decoy fragment ions
      {
         dBion = decoyIons[i].pdIonsN[j];
         dYion = decoyIons[i].pdIonsC[j];

         for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ++ii)
         {
            int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ii];

            // skip any padded 0.0 masses in decoy ions
            if (dBion == 0.0 && (iWhichIonSeries == ION_SERIES_A || iWhichIonSeries == ION_SERIES_B || iWhichIonSeries == ION_SERIES_C))
               continue;
            else if (dYion == 0.0 && (iWhichIonSeries == ION_SERIES_X || iWhichIonSeries == ION_SERIES_Y || iWhichIonSeries == ION_SERIES_Z || iWhichIonSeries == ION_SERIES_Z1))
               continue;

            dFragmentIonMass =  0.0;

            switch (iWhichIonSeries)
            {
               case ION_SERIES_A:
                  dFragmentIonMass = dBion - g_staticParams.massUtility.dCO;
                  break;
               case ION_SERIES_B:
                  dFragmentIonMass = dBion;
                  break;
               case ION_SERIES_C:
                  dFragmentIonMass = dBion + g_staticParams.massUtility.dNH3;
                  break;
               case ION_SERIES_X:
                  dFragmentIonMass = dYion + g_staticParams.massUtility.dCOminusH2;
                  break;
               case ION_SERIES_Y:
                  dFragmentIonMass = dYion;
                  break;
               case ION_SERIES_Z:
                  dFragmentIonMass = dYion - g_staticParams.massUtility.dNH2;
                  break;
               case ION_SERIES_Z1:
                  dFragmentIonMass = dYion - g_staticParams.massUtility.dNH2 + Hydrogen_Mono;
                  break;
            }

            for (ctCharge=1; ctCharge<=iMaxFragCharge; ++ctCharge)
            {
               dFragmentIonMass = (dFragmentIonMass + (ctCharge - 1.0) * PROTON_MASS) / ctCharge;

               if (dFragmentIonMass < pQuery->_pepMassInfo.dExpPepMass)
               {
                  iFragmentIonMass = BIN(dFragmentIonMass);

                  if (iFragmentIonMass < pQuery->_spectrumInfoInternal.iArraySize && iFragmentIonMass >= 0)
                  {
                     int x = iFragmentIonMass / SPARSE_MATRIX_SIZE;
                     if (pQuery->ppfSparseFastXcorrData[x] != NULL)
                     {
                        int y = iFragmentIonMass - (x*SPARSE_MATRIX_SIZE);
                        dFastXcorr += pQuery->ppfSparseFastXcorrData[x][y];
                     }
                  }
                  else if (iFragmentIonMass > pQuery->_spectrumInfoInternal.iArraySize && iFragmentIonMass >= 0)
                  {
                     char szErrorMsg[SIZE_ERROR];
                     sprintf(szErrorMsg,  " Error - XCORR DECOY: dFragMass %f, iFragMass %d, ArraySize %d, InputMass %f, scan %d, z %d",
                           dFragmentIonMass,
                           iFragmentIonMass,
                           pQuery->_spectrumInfoInternal.iArraySize,
                           pQuery->_pepMassInfo.dExpPepMass,
                           pQuery->_spectrumInfoInternal.iScanNumber,
                           ctCharge);

                     string strErrorMsg(szErrorMsg);
                     g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                     logerr(szErrorMsg);
                     return false;
                  }
               }
            }
         }
      }

      k = (int)(dFastXcorr*10.0*0.005 + 0.5);  // 10 for histogram, 0.005=50/10000.

      if (k < 0)
         k = 0;

      if (!(i%2) && k < pQuery->iMinXcorrHisto)  // lump some zero decoys into iMinXcorrHisto bin
         k = pQuery->iMinXcorrHisto;

      if (k >= HISTO_SIZE)
         k = HISTO_SIZE-1;

      piHistogram[k] += 1;
   }

   return true;
}


float CometPostAnalysis::FindSpScore(Query *pQuery,
                                     int bin,
                                     int iMax)
{
   int x = bin / SPARSE_MATRIX_SIZE;

   if (x > iMax || pQuery->ppfSparseSpScoreData[x] == NULL || bin == 0) // x should never be > iMax so this is just a safety check
      return 0.0f;

   int y = bin - (x*SPARSE_MATRIX_SIZE);

   return pQuery->ppfSparseSpScoreData[x][y];
}
