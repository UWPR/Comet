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

#ifndef _COMETPREPROCESS_H_
#define _COMETPREPROCESS_H_

#include "Common.h"
#include "ThreadPool.h"

struct PreprocessThreadData
{
   Spectrum mstSpectrum;
   int iAnalysisType;
   int iFileLastScan;
// bool *pbMemoryPool;  //MH: Manages active memory pool

   PreprocessThreadData()
   {
   }

   PreprocessThreadData(Spectrum &spec_in,
                        int iAnalysisType_in,
                        int iFileLastScan_in)
   {
      mstSpectrum = spec_in;
      iAnalysisType = iAnalysisType_in;
      iFileLastScan = iFileLastScan_in;
   }

   ~PreprocessThreadData()
   {
   }

};


class CometPreprocess
{
public:
   CometPreprocess();
   ~CometPreprocess();

   static void Reset();
   static bool LoadAndPreprocessSpectra(MSReader &mstReader,
                                        int iFirstScan,
                                        int iLastScan,
                                        int iAnalysisType,
                                        int minNumThreads,
                                        int maxNumThreads);
   static void PreprocessThreadProc(PreprocessThreadData *pPreprocessThreadData);
   static bool DoneProcessingAllSpectra();
   static bool PreprocessSingleSpectrum(int iPrecursorCharge,
                                        double dMZ,
                                        double *pdMass,
                                        double *pdInten,
                                        int iNumPeaks,
                                        map<int, double> &mapRawSpectrum);

private:

   // Private static methods
   static bool PreprocessSpectrum(Spectrum &spec);
   static bool CheckExistOutFile(int iCharge,
                                 int iScanNum);
   static bool AdjustMassTol(struct Query *pScoring);
   static void PreloadIons(MSReader &mstReader,
                           Spectrum &spec,
                           bool bNext=false,
                           int scNum=0);
   static bool CheckActivationMethodFilter(MSActivation act);
   static bool CheckExit(int iAnalysisType,
                         int iScanNum,
                         int iTotalScans,
                         int iLastScan,
                         int iReaderLastScan,
                         int iNumSpectraLoaded);
   static bool Preprocess(struct Query *pScoring,
                          Spectrum mstSpectrum);
   static bool LoadIons(struct Query *pScoring,
                        map<int, double> *mapSpectrum,
                        Spectrum mstSpectrum,
                        struct PreprocessStruct *pPre);
   static void NormalizeIntensities(map<int, double> *mapSpectrum,
                                    vector<std::pair<int, double> > *vBinnedSpectrumSP, 
                                    struct PreprocessStruct *pPre);
   static bool FillSparseMatrixMap(struct Query *pScoring,
                                   map<int, double> *mapSpectrum,
                                   struct PreprocessStruct *pPre);
   static bool SortVectorByInverseIntensity(const pair<int,double> &a,
                                    const pair<int,double> &b);
   static bool IsValidInputType(int inputType);


   // Private member variables
   static Mutex _maxChargeMutex;
   static bool _bFirstScan;
   static bool _bDoneProcessingAllSpectra;

   //MH: Common memory to be shared by all threads during spectral processing
// static bool *pbMemoryPool;                 //MH: Regulator of memory use
/*
   static double **ppdTmpRawDataArr;          //MH: Number of arrays equals threads
   static double **ppdTmpFastXcorrDataArr;    //MH: Ditto
   static double **ppdTmpCorrelationDataArr;  //MH: Ditto
   static double **ppdTmpSmoothedSpectrumArr; //MH: Ditto
   static double **ppdTmpPeakExtractedArr;    //MH: Ditto
*/
};

#endif // _COMETPREPROCESS_H_