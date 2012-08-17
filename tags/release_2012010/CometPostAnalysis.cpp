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
#include "ThreadPool.h"
#include "CometPostAnalysis.h"
#include "CometMassSpecUtils.h"

CometPostAnalysis::CometPostAnalysis()
{
}

CometPostAnalysis::~CometPostAnalysis()
{
}

void CometPostAnalysis::PostAnalysis(int minNumThreads, int maxNumThreads)
{
   if (!g_StaticParams.options.bOutputSqtStream)
      printf(" Perform post-search analysis\n");

   // Create the thread pool containing g_StaticParams.options.iNumThreads,
   // each hanging around and sleeping until asked to do a post analysis.
   ThreadPool<PostAnalysisThreadData *> postAnalysisThreadPool(PostAnalysisThreadProc,
         minNumThreads, maxNumThreads);

   for (int i=0; i<(int)g_pvQuery.size(); i++)
   {
      PostAnalysisThreadData *pThreadData = new PostAnalysisThreadData(i);
      postAnalysisThreadPool.Launch(pThreadData);
   }

   // Wait for active post analysis threads to complete processing.
   postAnalysisThreadPool.WaitForThreads();
}


void CometPostAnalysis::PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData)
{
   int iQueryIndex = pThreadData->iQueryIndex;

   AnalyzeSP(iQueryIndex);

   // Calculate E-values if necessary.
   if (g_StaticParams.options.bPrintExpectScore)
      AnalyzeEValue(iQueryIndex);

   delete pThreadData;
   pThreadData = NULL;
}


void CometPostAnalysis::AnalyzeSP(int i)
{
   int iSize = g_pvQuery.at(i)->iDoXcorrCount;

   if (iSize > g_StaticParams.options.iNumStored)
      iSize = g_StaticParams.options.iNumStored;

   // Target search.
   CalculateSP(g_pvQuery.at(i)->_pResults,
               i,
               g_pvQuery.at(i)->_spectrumInfoInternal.iArraySize,
               g_pvQuery.at(i)->_spectrumInfoInternal.iChargeState,
               iSize);

   qsort(g_pvQuery.at(i)->_pResults, iSize, sizeof(struct Results), SPQSortFn);
   g_pvQuery.at(i)->_pResults[0].iRankSp = 1;

   for (int ii=1; ii<iSize; ii++)
   {
      // Determine score rankings.
      if (g_pvQuery.at(i)->_pResults[ii].fScoreSp == g_pvQuery.at(i)->_pResults[ii-1].fScoreSp)
         g_pvQuery.at(i)->_pResults[ii].iRankSp = g_pvQuery.at(i)->_pResults[ii-1].iRankSp;
      else
         g_pvQuery.at(i)->_pResults[ii].iRankSp = g_pvQuery.at(i)->_pResults[ii-1].iRankSp + 1;
   }

   // Then sort each entry by xcorr.
   qsort(g_pvQuery.at(i)->_pResults, iSize, sizeof(struct Results), XcorrQSortFn);
 
   // Repeast for decoy search.
   if (g_StaticParams.options.iDecoySearch == 2)
   {

      CalculateSP(g_pvQuery.at(i)->_pDecoys,
                  i,
                  g_pvQuery.at(i)->_spectrumInfoInternal.iArraySize,
                  g_pvQuery.at(i)->_spectrumInfoInternal.iChargeState,
                  iSize);

      qsort(g_pvQuery.at(i)->_pDecoys, iSize, sizeof(struct Results), SPQSortFn);
      g_pvQuery.at(i)->_pDecoys[0].iRankSp = 1;

      for (int ii=1; ii<iSize; ii++)
      {
         // Determine score rankings.
         if (g_pvQuery.at(i)->_pDecoys[ii].fScoreSp == g_pvQuery.at(i)->_pDecoys[ii-1].fScoreSp)
            g_pvQuery.at(i)->_pDecoys[ii].iRankSp = g_pvQuery.at(i)->_pDecoys[ii-1].iRankSp;
         else
            g_pvQuery.at(i)->_pDecoys[ii].iRankSp = g_pvQuery.at(i)->_pDecoys[ii-1].iRankSp + 1;
      }

      // Then sort each entry by xcorr.
      qsort(g_pvQuery.at(i)->_pDecoys, iSize, sizeof(struct Results), XcorrQSortFn);
   }
}


void CometPostAnalysis::CalculateSP(Results *pOutput,
                                    int iWhichQuery,
                                    int iArraySize,
                                    int iChargeState,
                                    int iSize)
{
   int i;
   double pdAAforward[MAX_PEPTIDE_LEN];
   double pdAAreverse[MAX_PEPTIDE_LEN];
   IonSeriesStruct ionSeries[9];

   for (i=0; i<iSize; i++)
   {
      int  ii,
           ctCharge;
      double dFragmentIonMass = 0.0;
      double dConsec = 0.0;
      double dBion = g_StaticParams.precalcMasses.dNtermProton;
      double dYion = g_StaticParams.precalcMasses.dCtermOH2Proton;

      double dTmpIntenMatch = 0.0;

      int iMatchedFragmentIonCt = 0;
      int iMaxFragCharge;

      iMaxFragCharge = g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iMaxFragCharge;

      if (pOutput[i].szPrevNextAA[0] == '-')
         dBion += g_StaticParams.staticModifications.dAddNterminusProtein;
      if (pOutput[i].szPrevNextAA[1] == '-')
         dYion += g_StaticParams.staticModifications.dAddCterminusProtein;

      if (g_StaticParams.variableModParameters.bVarModSearch
            && (pOutput[i].pcVarModSites[pOutput[i].iLenPeptide] == 1))
      {
         dBion += g_StaticParams.variableModParameters.dVarModMassN;
      }

      if (g_StaticParams.variableModParameters.bVarModSearch
            && (pOutput[i].pcVarModSites[pOutput[i].iLenPeptide + 1] == 1))
      {
         dYion += g_StaticParams.variableModParameters.dVarModMassC;
      }

      for (ii=0; ii<g_StaticParams.ionInformation.iNumIonSeriesUsed; ii++)
      {
         int iii;

         for (iii=1; iii<=iMaxFragCharge; iii++)
            ionSeries[g_StaticParams.ionInformation.piSelectedIonSeries[ii]].bPreviousMatch[iii] = 0;
      }

      // Generate pdAAforward for _pResults[0].szPeptide.
      for (ii=0; ii<pOutput[i].iLenPeptide; ii++)
      {
         int iPos = pOutput[i].iLenPeptide - ii - 1;

         dBion += g_StaticParams.massUtility.pdAAMassFragment[(int)pOutput[i].szPeptide[ii]];
         dYion += g_StaticParams.massUtility.pdAAMassFragment[(int)pOutput[i].szPeptide[iPos]];

         if (g_StaticParams.variableModParameters.bVarModSearch && pOutput[i].pcVarModSites[ii]>0)
            dBion += g_StaticParams.variableModParameters.varModList[pOutput[i].pcVarModSites[ii]-1].dVarModMass;

         if (g_StaticParams.variableModParameters.bVarModSearch
               && (ii == pOutput[i].iLenPeptide -1)
               && (pOutput[i].pcVarModSites[pOutput[0].iLenPeptide + 1] == 1))
         {
            dBion += g_StaticParams.variableModParameters.dVarModMassC;
         }

         if (g_StaticParams.variableModParameters.bVarModSearch && pOutput[i].pcVarModSites[iPos]>0)
            dYion += g_StaticParams.variableModParameters.varModList[pOutput[i].pcVarModSites[iPos]-1].dVarModMass;

         pdAAforward[ii] = dBion;
         pdAAreverse[ii] = dYion;
      }

      for (ctCharge=1; ctCharge<=iMaxFragCharge; ctCharge++)
      {
         for (ii=0; ii<g_StaticParams.ionInformation.iNumIonSeriesUsed; ii++)
         {
            int iWhichIonSeries = g_StaticParams.ionInformation.piSelectedIonSeries[ii];

            // As both _pdAAforward and _pdAAreverse are increasing, loop through
            // iLenPeptide-1 to complete set of internal fragment ions.
            for (int iii=0; iii<pOutput[i].iLenPeptide-1; iii++)
            {
               // Gets fragment ion mass.
               dFragmentIonMass = CometMassSpecUtils::GetFragmentIonMass(iWhichIonSeries, iii, ctCharge, pdAAforward, pdAAreverse);

               if ( !(dFragmentIonMass <= FLOAT_ZERO))
               {
                  int iFragmentIonMass = BIN(dFragmentIonMass);

                  if (g_pvQuery.at(iWhichQuery)->pfSpScoreData[iFragmentIonMass] > FLOAT_ZERO)
                  {
                     iMatchedFragmentIonCt++;
                     
                     // Simple sum intensity.
                     dTmpIntenMatch += g_pvQuery.at(iWhichQuery)->pfSpScoreData[iFragmentIonMass];

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
            ((pOutput[i].iLenPeptide-1) * iMaxFragCharge * g_StaticParams.ionInformation.iNumIonSeriesUsed));

      pOutput[i].iMatchedIons = iMatchedFragmentIonCt;
   }
}


int CometPostAnalysis::SPQSortFn(const void *a, const void *b)
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


int CometPostAnalysis::XcorrQSortFn(const void *a, const void *b)
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


void CometPostAnalysis::AnalyzeEValue(int i)
{
   if (g_pvQuery.at(i)->iDoXcorrCount> 0)
   {
      CalculateEValue(i, 0);
      
      if (g_StaticParams.options.iDecoySearch == 2)
         CalculateEValue(i, 1);
   }
}


void CometPostAnalysis::CalculateEValue(int iWhichQuery, bool bDecoy)
{
   int i;
   int *piHistogram;
   int iHistogramCount;
   int iMaxCorr;
   int iStartCorr;
   int iNextCorr;
   int iXcorrCount;
   double dSlope;
   double dIntercept;

   if (bDecoy)
   {
      piHistogram = g_pvQuery.at(iWhichQuery)->iDecoyCorrelationHistogram;
      iHistogramCount = g_pvQuery.at(iWhichQuery)->iDoDecoyXcorrCount;
   }
   else
   {
      piHistogram = g_pvQuery.at(iWhichQuery)->iCorrelationHistogram;
      iHistogramCount = g_pvQuery.at(iWhichQuery)->iDoXcorrCount;
   }

   if (iHistogramCount < DECOY_SIZE)
      GenerateXcorrDecoys(iWhichQuery, bDecoy);

   LinearRegression(piHistogram, &dSlope, &dIntercept, &iMaxCorr, &iStartCorr, &iNextCorr);

   if (bDecoy)
   {
      iXcorrCount = g_pvQuery.at(iWhichQuery)->iDoDecoyXcorrCount;
      g_pvQuery.at(iWhichQuery)->fDecoyPar[0] = (float)dIntercept;  // b   y=mx+b
      g_pvQuery.at(iWhichQuery)->fDecoyPar[1] = (float)dSlope;      // m
      g_pvQuery.at(iWhichQuery)->fDecoyPar[2] = (float)iStartCorr;
      g_pvQuery.at(iWhichQuery)->fDecoyPar[3] = (float)iNextCorr;
      g_pvQuery.at(iWhichQuery)->siMaxDecoyXcorr = (short)iMaxCorr;
   }
   else
   {
      iXcorrCount = g_pvQuery.at(iWhichQuery)->iDoXcorrCount;
      g_pvQuery.at(iWhichQuery)->fPar[0] = (float)dIntercept;  // b
      g_pvQuery.at(iWhichQuery)->fPar[1] = (float)dSlope    ;  // m
      g_pvQuery.at(iWhichQuery)->fPar[2] = (float)iStartCorr;
      g_pvQuery.at(iWhichQuery)->fPar[3] = (float)iNextCorr;
      g_pvQuery.at(iWhichQuery)->siMaxXcorr = (short)iMaxCorr;
   }

   if (iXcorrCount > g_StaticParams.options.iNumPeptideOutputLines)
      iXcorrCount = g_StaticParams.options.iNumPeptideOutputLines;

   dSlope *= 10.0; // Used in pow() function so do multiply outside of for loop.

   for (i=0; i<iXcorrCount; i++)
   {
      double dExpect;

      if (dSlope >= 0.0)
      {
         g_pvQuery.at(iWhichQuery)->_pResults[i].dExpect = 999.0;
      }
      else
      {
         dExpect = pow(10.0, dSlope * g_pvQuery.at(iWhichQuery)->_pResults[i].fXcorr + dIntercept);

         if (dExpect > 999.0)
            dExpect = 999.0;

         // Sanity constraints - no low e-values allowed for xcorr < 1.2.
         // I'll admin xcorr < 1.2 is an arbitrary cutoff but something is needed.
         if (dExpect < 1.0)
         {
            if (bDecoy)
            {
               if (g_pvQuery.at(iWhichQuery)->_pDecoys[i].fXcorr < 1.2)
                  dExpect = 1.0;
            }
            else
            {
               if (g_pvQuery.at(iWhichQuery)->_pResults[i].fXcorr < 1.2)
                  dExpect = 1.0;
            }
         }

         if (bDecoy)
            g_pvQuery.at(iWhichQuery)->_pDecoys[i].dExpect = dExpect;
         else
            g_pvQuery.at(iWhichQuery)->_pResults[i].dExpect = dExpect;
      }
   }
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
void CometPostAnalysis::GenerateXcorrDecoys(int iWhichQuery,
                                            bool bDecoy)
{
   int i;
   int ii;
   int j;
   int k;
   int iMaxFragCharge;
   int ctCharge;
   double dBion;
   double dYion;
   double dTmp1 = g_StaticParams.precalcMasses.dNtermProton;
   double dTmp2 = g_StaticParams.precalcMasses.dCtermOH2Proton;
   double dShift = 5.432;  // a random number
   double dFastXcorr;
   double dFragmentIonMass = 0.0;

   int *piHistogram;
   int iHistogramCount;

   int iFragmentIonMass;

   if (bDecoy)
   {
      piHistogram = g_pvQuery.at(iWhichQuery)->iDecoyCorrelationHistogram;
      iHistogramCount = g_pvQuery.at(iWhichQuery)->iDoDecoyXcorrCount;
   }
   else
   {
      piHistogram = g_pvQuery.at(iWhichQuery)->iCorrelationHistogram;
      iHistogramCount = g_pvQuery.at(iWhichQuery)->iDoXcorrCount;
   }

   if (iHistogramCount > g_StaticParams.options.iNumStored)
      iHistogramCount = g_StaticParams.options.iNumStored;

   iMaxFragCharge = g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iMaxFragCharge;

   j=0;
 
   int iLoopMax = DECOY_SIZE - iHistogramCount;
   int iLenPeptide=0;
   char *szPeptide;
   double dPepMass;

   for (i=0; i<iLoopMax; i++)
   {
      if (j == iHistogramCount)  // j is rotating index through hit list.
      {
         j=0;
         dShift += 5.8524;      // Some random shift value.
      }

      if (bDecoy)
      {
         iLenPeptide = g_pvQuery.at(iWhichQuery)->_pDecoys[j].iLenPeptide;
         szPeptide = g_pvQuery.at(iWhichQuery)->_pDecoys[j].szPeptide;
         dPepMass = g_pvQuery.at(iWhichQuery)->_pDecoys[j].dPepMass;
      }
      else
      {
         iLenPeptide = g_pvQuery.at(iWhichQuery)->_pResults[j].iLenPeptide;
         szPeptide = g_pvQuery.at(iWhichQuery)->_pResults[j].szPeptide;
         dPepMass = g_pvQuery.at(iWhichQuery)->_pResults[j].dPepMass;
      }

      dFastXcorr = 0.0;

      for (ctCharge=1; ctCharge<=iMaxFragCharge; ctCharge++)
      {
         dBion = dTmp1 + dShift;  // Ignore variable mods and rotate plain fragment ions.
         dYion = dTmp2 + dShift;

         for (ii=0; ii<g_StaticParams.ionInformation.iNumIonSeriesUsed; ii++)
         {
            int iWhichIonSeries = g_StaticParams.ionInformation.piSelectedIonSeries[ii];

            for (k=0; k<iLenPeptide; k++)
            {
               int iPos = iLenPeptide - k - 1;

               dBion += g_StaticParams.massUtility.pdAAMassFragment[(int)szPeptide[k]];
               dYion += g_StaticParams.massUtility.pdAAMassFragment[(int)szPeptide[iPos]];

               dFragmentIonMass =  0.0;
               switch (iWhichIonSeries)
               {
                  case ION_SERIES_A:
                     dFragmentIonMass = dBion - g_StaticParams.massUtility.dCO;
                     break;
                  case ION_SERIES_B:
                     dFragmentIonMass = dBion;
                     break;
                  case ION_SERIES_C:
                     dFragmentIonMass = dBion + g_StaticParams.massUtility.dNH3;
                     break;
                  case ION_SERIES_X:
                     dFragmentIonMass = dYion + g_StaticParams.massUtility.dCOminusH2;
                     break;
                  case ION_SERIES_Y:
                     dFragmentIonMass = dYion;
                     break;
                  case ION_SERIES_Z:
                     dFragmentIonMass = dYion - g_StaticParams.massUtility.dNH2;
                     break;
               }

               while (dFragmentIonMass >= g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass)
                  dFragmentIonMass -= g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass;

               dFragmentIonMass = (dFragmentIonMass + (ctCharge-1)*PROTON_MASS)/ctCharge;
               iFragmentIonMass = BIN(dFragmentIonMass);

               if (iFragmentIonMass < g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iArraySize && iFragmentIonMass >= 0)
               {
                  dFastXcorr += g_pvQuery.at(iWhichQuery)->pfFastXcorrData[iFragmentIonMass];
                  dFastXcorr += 0.5 * g_pvQuery.at(iWhichQuery)->pfFastXcorrData[iFragmentIonMass-1];
               }
               else
               {
                  printf(" Error - XCORR DECOY: dFragMass %f, iFragMass %d, ArraySize %d, InputMass %f, scan %d, z %d\n",
                        dFragmentIonMass, 
                        iFragmentIonMass,
                        g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iArraySize, 
                        g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass,
                        g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
                        ctCharge);
                  exit(1);
               }

               if (dBion > dPepMass)
                  dBion -= dPepMass;
               if (dYion > dPepMass)
                  dYion -= dPepMass;
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
}
