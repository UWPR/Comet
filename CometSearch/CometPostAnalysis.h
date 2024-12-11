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


#ifndef _COMETPOSTANALYSIS_H_
#define _COMETPOSTANALYSIS_H_

#include "CometDataInternal.h"

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
   static bool PostAnalysis(ThreadPool* tp);
   static void PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData,
                                      ThreadPool* tp);
   static void CalculateDeltaCn(int i);
   static void AnalyzeSP(int i);
   static void CalculateSP(Results *pOutput,
                           int iWhichQuery,
                           int iSize);
   static bool CalculateEValue(int iWhichQuery,
                               bool bTopHitOnly);
   static bool SortFnXcorr(const Results &a,
                           const Results &b);
private:

   static bool SortFnSp(const Results &a,
                        const Results &b);

   static bool SortFnMod(const Results &a,
                         const Results &b);
   static bool GenerateXcorrDecoys(int iWhichQuery);
   static void LinearRegression(int *pHistogram,
                                double *dSlope,
                                double *dIntercept,
                                int *iMaxCorr,
                                int *iStartCorr,
                                int *iNextCorr);
   static float FindSpScore(Query *pQuery,
                            int bin,
                            int iMax);
   static bool ProteinEntryCmp(const struct ProteinEntryStruct &a,
                               const struct ProteinEntryStruct &b);
};


#endif // _COMETPOSTANALYSIS_H_
