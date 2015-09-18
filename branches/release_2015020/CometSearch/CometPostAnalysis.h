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

#ifndef _COMETPOSTANALYSIS_H_
#define _COMETPOSTANALYSIS_H_


struct PostAnalysisThreadData
{
   int iQueryIndex;

   PostAnalysisThreadData()
   {
      iQueryIndex = -1;
   }

   PostAnalysisThreadData(int iQueryIndex_in)
   {
      iQueryIndex = iQueryIndex_in;
   }
};

class CometPostAnalysis
{
public:
   CometPostAnalysis();
   ~CometPostAnalysis();
   static bool PostAnalysis(int minNumThreads, int maxNumThreads);
   static void PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData);

private:
   static void AnalyzeSP(int i);
   static void CalculateSP(Results *pOutput,
                           int iWhichQuery,
                           int iSize);
   static int QSortFnSp(const void *a,
                        const void *b);
   static int QSortFnXcorr(const void *a,
                           const void *b);
   static int QSortFnPep(const void *a,
                         const void *b);
   static int QSortFnMod(const void *a,
                         const void *b);
   static bool CalculateEValue(int iWhichQuery);
   static bool GenerateXcorrDecoys(int iWhichQuery);
   static void LinearRegression(int *pHistogram,
                                double *dSlope,
                                double *dIntercept,
                                int *iMaxCorr,
                                int *iStartCorr,
                                int *iNextCorr);
   static float FindSpScore(Query *pQuery,
                            int bin);
};


#endif // _COMETPOSTANALYSIS_H_
