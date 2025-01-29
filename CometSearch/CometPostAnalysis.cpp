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


void CometPostAnalysis::PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData, ThreadPool* tp)
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
