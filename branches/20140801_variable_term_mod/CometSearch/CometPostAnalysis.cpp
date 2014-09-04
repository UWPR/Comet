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
#include "ThreadPool.h"
#include "CometPostAnalysis.h"
#include "CometMassSpecUtils.h"
#include "CometStatus.h"

CometPostAnalysis::CometPostAnalysis()
{
}


CometPostAnalysis::~CometPostAnalysis()
{
}


bool CometPostAnalysis::PostAnalysis(int minNumThreads,
                                     int maxNumThreads)
{
   bool bSucceeded = true;

   if (!g_staticParams.options.bOutputSqtStream)
      logout(" - Perform post-search analysis\n");

   // Create the thread pool containing g_staticParams.options.iNumThreads,
   // each hanging around and sleeping until asked to do a post analysis.
   ThreadPool<PostAnalysisThreadData *> *pPostAnalysisThreadPool = new ThreadPool<PostAnalysisThreadData*>(PostAnalysisThreadProc,
         minNumThreads, maxNumThreads);

   for (int i=0; i<(int)g_pvQuery.size(); i++)
   {
      PostAnalysisThreadData *pThreadData = new PostAnalysisThreadData(i);
      pPostAnalysisThreadPool->Launch(pThreadData);

      bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
      if (!bSucceeded)
      {
         break;
      }
   }

   // Wait for active post analysis threads to complete processing.
   pPostAnalysisThreadPool->WaitForThreads();

   delete pPostAnalysisThreadPool;
   pPostAnalysisThreadPool = NULL;

   // Check for errors one more time since there might have been an error 
   // while we were waiting for the threads.
   if (bSucceeded)
   {
      bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
   }

   return bSucceeded;
}


void CometPostAnalysis::PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData)
{
   int iQueryIndex = pThreadData->iQueryIndex;
   bool bSucceeded = true;

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
         bSucceeded = CalculateEValue(iQueryIndex);
      }
   }

   // FIX ... need to do something with bSucceeded otherwise no point in having it
 
   delete pThreadData;
   pThreadData = NULL;
}


void CometPostAnalysis::AnalyzeSP(int i)
{
   Query* pQuery = g_pvQuery.at(i);

   int iSize = pQuery->iMatchPeptideCount;

   if (iSize > g_staticParams.options.iNumStored)
      iSize = g_staticParams.options.iNumStored;

   // Target search
   CalculateSP(pQuery->_pResults, i, iSize);

   qsort(pQuery->_pResults, iSize, sizeof(struct Results), SPQSortFn);
   pQuery->_pResults[0].iRankSp = 1;

   for (int ii=1; ii<iSize; ii++)
   {
      // Determine score rankings
      if (isEqual(pQuery->_pResults[ii].fScoreSp, pQuery->_pResults[ii-1].fScoreSp))
         pQuery->_pResults[ii].iRankSp = pQuery->_pResults[ii-1].iRankSp;
      else
         pQuery->_pResults[ii].iRankSp = pQuery->_pResults[ii-1].iRankSp + 1;
   }

   // Then sort each entry by xcorr
   qsort(pQuery->_pResults, iSize, sizeof(struct Results), XcorrQSortFn);
 
   // Repeat for decoy search
   if (g_staticParams.options.iDecoySearch == 2)
   {
      iSize = pQuery->iDecoyMatchPeptideCount;

      if (iSize > g_staticParams.options.iNumStored)
         iSize = g_staticParams.options.iNumStored;

      CalculateSP(pQuery->_pDecoys, i, iSize);

      qsort(pQuery->_pDecoys, iSize, sizeof(struct Results), SPQSortFn);
      pQuery->_pDecoys[0].iRankSp = 1;

      for (int ii=1; ii<iSize; ii++)
      {
         // Determine score rankings
         if (isEqual(pQuery->_pDecoys[ii].fScoreSp, pQuery->_pDecoys[ii-1].fScoreSp))
            pQuery->_pDecoys[ii].iRankSp = pQuery->_pDecoys[ii-1].iRankSp;
         else
            pQuery->_pDecoys[ii].iRankSp = pQuery->_pDecoys[ii-1].iRankSp + 1;
      }

      // Then sort each entry by xcorr
      qsort(pQuery->_pDecoys, iSize, sizeof(struct Results), XcorrQSortFn);
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

   for (i=0; i<iSize; i++)
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

      // if no variable mods are used in search, clear pcVarModSites here
      if (!g_staticParams.variableModParameters.bVarModSearch)
         memset(pOutput[i].pcVarModSites, 0, sizeof(char)*MAX_PEPTIDE_LEN_P2);

      iMaxFragCharge = g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iMaxFragCharge;

      if (pOutput[i].szPrevNextAA[0] == '-')
         dBion += g_staticParams.staticModifications.dAddNterminusProtein;
      if (pOutput[i].szPrevNextAA[1] == '-')
         dYion += g_staticParams.staticModifications.dAddCterminusProtein;

      if (g_staticParams.variableModParameters.bVarModSearch
            && (pOutput[i].pcVarModSites[pOutput[i].iLenPeptide] > 0))
      {
         dBion += g_staticParams.variableModParameters.varModList[pOutput[i].pcVarModSites[pOutput[i].iLenPeptide]-1].dVarModMass;
      }

      if (g_staticParams.variableModParameters.bVarModSearch
            && (pOutput[i].pcVarModSites[pOutput[i].iLenPeptide + 1] > 0))
      {
         dYion += g_staticParams.variableModParameters.varModList[pOutput[i].pcVarModSites[pOutput[i].iLenPeptide+1]-1].dVarModMass;
      }

      for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ii++)
      {
         int iii;

         for (iii=1; iii<=iMaxFragCharge; iii++)
            ionSeries[g_staticParams.ionInformation.piSelectedIonSeries[ii]].bPreviousMatch[iii] = 0;
      }

      // Generate pdAAforward for _pResults[0].szPeptide.
      for (ii=0; ii<pOutput[i].iLenPeptide; ii++)
      {
         int iPos = pOutput[i].iLenPeptide - ii - 1;

         dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[i].szPeptide[ii]];
         dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[i].szPeptide[iPos]];

         if (g_staticParams.variableModParameters.bVarModSearch && pOutput[i].pcVarModSites[ii]>0)
            dBion += g_staticParams.variableModParameters.varModList[pOutput[i].pcVarModSites[ii]-1].dVarModMass;

/*
FIX:  what is this code??
         if (g_staticParams.variableModParameters.bVarModSearch
               && (ii == pOutput[i].iLenPeptide -1)
               && (pOutput[i].pcVarModSites[pOutput[0].iLenPeptide + 1] == 1))
         {
            dBion += g_staticParams.variableModParameters.dVarModMassC;
         }
*/

         if (g_staticParams.variableModParameters.bVarModSearch && pOutput[i].pcVarModSites[iPos]>0)
            dYion += g_staticParams.variableModParameters.varModList[pOutput[i].pcVarModSites[iPos]-1].dVarModMass;

         pdAAforward[ii] = dBion;
         pdAAreverse[ii] = dYion;
      }

      for (ctCharge=1; ctCharge<=iMaxFragCharge; ctCharge++)
      {
         for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ii++)
         {
            int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ii];

            // As both _pdAAforward and _pdAAreverse are increasing, loop through
            // iLenPeptide-1 to complete set of internal fragment ions.
            for (int iii=0; iii<pOutput[i].iLenPeptide-1; iii++)
            {
               // Gets fragment ion mass.
               dFragmentIonMass = CometMassSpecUtils::GetFragmentIonMass(iWhichIonSeries, iii, ctCharge, pdAAforward, pdAAreverse);

               if ( !(dFragmentIonMass <= FLOAT_ZERO))
               {
                  int iFragmentIonMass = BIN(dFragmentIonMass);
                  float fSpScore;

                  if (g_staticParams.options.bSparseMatrix)
                     fSpScore = FindSpScore(g_pvQuery.at(iWhichQuery),iFragmentIonMass);
                  else
                     fSpScore = g_pvQuery.at(iWhichQuery)->pfSpScoreData[iFragmentIonMass];

                  if (fSpScore > FLOAT_ZERO)
                  {
                     iMatchedFragmentIonCt++;
                     
                     // Simple sum intensity.
                     dTmpIntenMatch += fSpScore;

                     // Increase score for consecutive fragment ion series matches.
                     if (ionSeries[iWhichIonSeries].bPreviousMatch[ctCharge])
                        dConsec += 0.075;

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

      pOutput[i].fScoreSp = (float) ((dTmpIntenMatch * iMatchedFragmentIonCt*(1.0+dConsec)) / 
            ((pOutput[i].iLenPeptide-1) * iMaxFragCharge * g_staticParams.ionInformation.iNumIonSeriesUsed));

      pOutput[i].iMatchedIons = iMatchedFragmentIonCt;
   }
}


int CometPostAnalysis::SPQSortFn(const void *a,
                                 const void *b)
{
   struct Results *ia = (struct Results *)a;
   struct Results *ib = (struct Results *)b;

   if (ia->fScoreSp < ib->fScoreSp)
      return 1;
   else if (ia->fScoreSp > ib->fScoreSp)
      return -1;
   else
      return 0;
}


int CometPostAnalysis::XcorrQSortFn(const void *a,
                                    const void *b)
{
   struct Results *ia = (struct Results *)a;
   struct Results *ib = (struct Results *)b;

   if (ia->fXcorr < ib->fXcorr)
      return 1;
   else if (ia->fXcorr > ib->fXcorr)
      return -1;
   else
      return 0;
}


bool CometPostAnalysis::CalculateEValue(int iWhichQuery)
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

   if (pQuery->iHistogramCount < DECOY_SIZE)
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

   for (i=0; i<iLoopCount; i++)
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

            // Sanity constraints - no low e-values allowed for xcorr < 1.0.
            // I'll admit xcorr < 1.0 is an arbitrary cutoff but something is needed.
            if (dExpect < 1.0)
            {
               if (pQuery->_pResults[i].fXcorr < 1.0)
                  dExpect = 1.0;
            }

            pQuery->_pResults[i].dExpect = dExpect;
         }

         if (i<pQuery->iDecoyMatchPeptideCount)
         {
            dExpect = pow(10.0, dSlope * pQuery->_pDecoys[i].fXcorr + dIntercept);
            if (dExpect > 999.0)
               dExpect = 999.0;

            if (dExpect < 1.0)
            {
               if (pQuery->_pDecoys[i].fXcorr < 1.0)
                  dExpect = 1.0;
            }

            pQuery->_pDecoys[i].dExpect = dExpect;
         }
      }
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

   double dCummulative[HISTO_SIZE];  // Cummulative frequency at each xcorr value.

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

   iNextCorr =0;
   for (i=0; i<iMaxCorr; i++)
   {
      if (piHistogram[i]==0)
      {
         if (i>0)
            iNextCorr = i-1;
         break;
      }
   }

   if (i==iMaxCorr)
   {
      iNextCorr = iMaxCorr;
      if (iMaxCorr>12)
         iNextCorr = iMaxCorr-2;
   }

   // Create cummulative distribution function from iNextCorr down, skipping the outliers.
   dCummulative[iNextCorr] = piHistogram[iNextCorr];
   for (i=iNextCorr-1; i>=0; i--)
   {
      dCummulative[i] = dCummulative[i+1] + piHistogram[i];
      if (piHistogram[i+1] == 0)
         dCummulative[i+1] = 0.0;
   }

   // log10
   for (i=iNextCorr; i>=0; i--)
   {
      piHistogram[i] = (int)dCummulative[i];  // First store cummulative in histogram.
      dCummulative[i] = log10(dCummulative[i]);
   }

   iStartCorr = 0;
   if (iNextCorr > 12)
      iStartCorr = iNextCorr - 12 ;

   Mx=My=a=b=0.0;

   while (iStartCorr >= 0)
   {
      Sx=Sxy=SumX=SumY=0.0;
      iNumPoints=0;

      // Calculate means.
      for (i=iStartCorr; i<=iNextCorr; i++)
      {
         if (piHistogram[i] > 0)
         {
            SumY += (float)dCummulative[i];
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
      for (i=iStartCorr; i<=iNextCorr; i++)
      {
         if (dCummulative[i] > 0)
         {
            double dX;
            double dY;

            dX = i - Mx;
            dY = dCummulative[i] - My;

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
   int iFastXcorrIndex;
   int iLastFastXcorrIndex;
   int iMaxFragCharge;
   int ctCharge;
   double dBion;
   double dYion;
   double dTmp1 = g_staticParams.precalcMasses.dNtermProton;
   double dTmp2 = g_staticParams.precalcMasses.dCtermOH2Proton;
   double dShift = 5.432;  // a random number
   double dFastXcorr;
   double dFragmentIonMass = 0.0;

   int *piHistogram;

   int iFragmentIonMass;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   piHistogram = pQuery->iXcorrHistogram;

   iMaxFragCharge = pQuery->_spectrumInfoInternal.iMaxFragCharge;

 
   // DECOY_SIZE is the minimum # of decoys required or else this function is
   // called.  So need generate iLoopMax more xcorr scores for the histogram.
   int iLoopMax = DECOY_SIZE - pQuery->iHistogramCount;
   int iLenPeptide=0;
   char *szPeptide;
   double dPepMass;
   bool bDecoy;
   int iLastEntry;

   // Determine if using target or decoy peptides to rotate to fill out histogram.
   if (pQuery->iMatchPeptideCount >= pQuery->iDecoyMatchPeptideCount)
   {
      iLastEntry = pQuery->iMatchPeptideCount;
      bDecoy = false;
   }
   else
   {
      iLastEntry = pQuery->iDecoyMatchPeptideCount;
      bDecoy = true;
   }

   if (iLastEntry > g_staticParams.options.iNumStored)
      iLastEntry = g_staticParams.options.iNumStored;

   j=0;
   for (i=0; i<iLoopMax; i++)
   {
      if (j == iLastEntry)      // j is rotating index through stored peptides
      {
         j=0;
         dShift += 5.8524;      // Some random shift value.
      }

      if (bDecoy)
      {
         iLenPeptide = pQuery->_pDecoys[j].iLenPeptide;
         szPeptide = pQuery->_pDecoys[j].szPeptide;
         dPepMass = pQuery->_pDecoys[j].dPepMass;
      }
      else
      {
         iLenPeptide = pQuery->_pResults[j].iLenPeptide;
         szPeptide = pQuery->_pResults[j].szPeptide;
         dPepMass = pQuery->_pResults[j].dPepMass;
      }

      dFastXcorr = 0.0;

      for (ctCharge=1; ctCharge<=iMaxFragCharge; ctCharge++)
      {
         dBion = dTmp1 + dShift;  // Ignore variable mods and rotate plain fragment ions.
         dYion = dTmp2 + dShift;

         for (ii=0; ii<g_staticParams.ionInformation.iNumIonSeriesUsed; ii++)
         {
            int iWhichIonSeries = g_staticParams.ionInformation.piSelectedIonSeries[ii];
            iLastFastXcorrIndex=0;
            for (k=0; k<iLenPeptide; k++)
            {
               int iPos = iLenPeptide - k - 1;

               dBion += g_staticParams.massUtility.pdAAMassFragment[(int)szPeptide[k]];
               dYion += g_staticParams.massUtility.pdAAMassFragment[(int)szPeptide[iPos]];

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
               }

               while (dFragmentIonMass >= pQuery->_pepMassInfo.dExpPepMass)
               {
                  iLastFastXcorrIndex=0;
                  dFragmentIonMass -= pQuery->_pepMassInfo.dExpPepMass;
               }

               dFragmentIonMass = (dFragmentIonMass + (ctCharge-1)*PROTON_MASS)/ctCharge;
               iFragmentIonMass = BIN(dFragmentIonMass);

               if (g_staticParams.options.bSparseMatrix)
               {
                  if (iFragmentIonMass < pQuery->pSparseFastXcorrData[pQuery->iFastXcorrData-1].bin && iFragmentIonMass >= 0)
                  {
                     iFastXcorrIndex = FindFastXcorrIndex(pQuery, iFragmentIonMass, iLastFastXcorrIndex);
                     dFastXcorr += pQuery->pSparseFastXcorrData[iFastXcorrIndex].fIntensity;

                     iLastFastXcorrIndex=iFastXcorrIndex;
                  }
                  else
                  {
                     char szErrorMsg[256];
                     sprintf(szErrorMsg,  " Error - XCORR DECOY: dFragMass %f, iFragMass %d, ArraySize %d, InputMass %f, scan %d, z %d",
                           dFragmentIonMass, 
                           iFragmentIonMass,
                           pQuery->_spectrumInfoInternal.iArraySize, 
                           pQuery->_pepMassInfo.dExpPepMass,
                           pQuery->_spectrumInfoInternal.iScanNumber,
                           ctCharge);
                  
                     string strErrorMsg(szErrorMsg);
                     g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                     logerr("\n Comet version \"%s\"\n\n", comet_version);
                     logerr("%s\n\n", szErrorMsg);
                     return false;
                  }
               }
               else
               {
                  if (iFragmentIonMass < pQuery->_spectrumInfoInternal.iArraySize && iFragmentIonMass >= 0)
                  {
                     dFastXcorr += pQuery->pfFastXcorrData[iFragmentIonMass];
                  }
                  else
                  {
                     char szErrorMsg[256];
                     sprintf(szErrorMsg,  " Error - XCORR DECOY: dFragMass %f, iFragMass %d, ArraySize %d, InputMass %f, scan %d, z %d",
                           dFragmentIonMass,
                           iFragmentIonMass,
                           pQuery->_spectrumInfoInternal.iArraySize,
                           pQuery->_pepMassInfo.dExpPepMass,
                           pQuery->_spectrumInfoInternal.iScanNumber,
                           ctCharge);
                  
                     string strErrorMsg(szErrorMsg);
                     g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                     logerr("\n Comet version \"%s\"\n\n", comet_version);
                     logerr("%s\n\n", szErrorMsg);
                     return false;
                  }
               }

               if (dBion > dPepMass)
               {
                  dBion -= dPepMass;
                  iLastFastXcorrIndex=0;
               }
               if (dYion > dPepMass)
               {
                  dYion -= dPepMass;
                  iLastFastXcorrIndex=0;
               }
            }
         }
      }

      k = (int)(dFastXcorr*10.0*0.005 + 0.5);  // 10 for histogram, 0.005=50/10000.

      if (k < 0)
         k = 0;
      else if (k >= HISTO_SIZE)
         k = HISTO_SIZE-1;

      piHistogram[k] += 1;

      j++;  // Go to next candidate peptide.
   }

   return true;
}


float CometPostAnalysis::FindSpScore(Query *pQuery,
                                     int bin)
{
   int lower;
   int mid;
   int upper;
   int sz=pQuery->iSpScoreData;

   mid=sz/2;
   lower=0;
   upper=sz;

   while(pQuery->pSparseSpScoreData[mid].bin!=bin)
   {
      if(lower>=upper) 
         return 0.0f;

      if(bin<pQuery->pSparseSpScoreData[mid].bin)
      {
         upper=mid-1;
         mid=(lower+upper)/2;
      } 
      else 
      {
         lower=mid+1;
         mid=(lower+upper)/2;
      }

      if(mid==sz) 
         return 0.0f;
   }
   return pQuery->pSparseSpScoreData[mid].fIntensity;
}


int CometPostAnalysis::FindFastXcorrIndex(Query *pQuery,
                                          int bin,
                                          int low)
{
   int lower=low;
   int upper=pQuery->iFastXcorrData;
   int sz=upper-low;
   int mid=sz/2+low;
   
   while(pQuery->pSparseFastXcorrData[mid].bin!=bin)
   {
      if(lower>=upper) 
      {
         if(pQuery->pSparseFastXcorrData[mid].bin<bin)
            return mid;
         else
            return mid-1;
      }

      if(bin<pQuery->pSparseFastXcorrData[mid].bin)
      {
         upper=mid-1;
         mid=(lower+upper)/2;
      } 
      else 
      {
         lower=mid+1;
         mid=(lower+upper)/2;
      }

      if(mid==pQuery->iFastXcorrData)
         return mid-1;
   }
   return mid;
}
