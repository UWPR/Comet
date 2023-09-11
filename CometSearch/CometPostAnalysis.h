/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
   static bool PostAnalysis(ThreadPool* tp);
   static void PostAnalysisThreadProc(PostAnalysisThreadData *pThreadData,
                                      ThreadPool* tp);
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
