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
   int iZLine;
   int iAnalysisType;
   int iFileLastScan;

   PreprocessThreadData()
   {
   }

   PreprocessThreadData(Spectrum &spec_in,
                        int iZLine_in,
                        int iAnalysisType_in,
                        int iFileLastScan_in)
   {
      mstSpectrum = spec_in;
      iZLine = iZLine_in;
      iAnalysisType = iAnalysisType_in;
      iFileLastScan = iFileLastScan_in;
   }
};


class CometPreprocess
{
public:
   CometPreprocess();
   ~CometPreprocess();

   static void LoadAndPreprocessSpectra(int iZLine, 
                                        int iFirstScan, 
                                        int iLastScan, 
                                        int iScanCount, 
                                        int iAnalysisType,
                                        int minNumThreads,
                                        int maxNumThreads);
   static void PreprocessThreadProc(PreprocessThreadData *pPreprocessThreadData);
   
private:
   
   // Private static methods
   static void SetMSLevelFilter(MSReader &mstReader);
   static void PreprocessSpectrum(Spectrum &spec, 
                                  int iZLine, 
                                  int iAnalysisType, 
                                  int iFileLastScan);
   static bool CheckExistOutFile(int iCharge,
                                 int iScanNum);
   static void AdjustMassTol(struct Query *pScoring);
   static void PreloadIons(MSReader &mstReader,
                           Spectrum &spec,
                           bool bNext=false,
                           int scNum=0);
   static bool CheckActivationMethodFilter(MSActivation act);
   static bool CheckExit(int iAnalysisType,
                         int iScanNum, 
                         int iScanCount, 
                         int iTotalScans, 
                         int iLastScan,
                         int iReaderLastScan);
   static void Preprocess(struct Query *pScoring,
                          Spectrum mstSpectrum);
   static void LoadIons(struct Query *pScoring,
                        Spectrum mstSpectrum,
                        struct PreprocessStruct *pPre);
   static void Initialize_P(struct msdata *pTempSpData);
   static void MakeCorrData(double *pdTempRawData,
                            struct Query *pScoring,
                            struct PreprocessStruct *pPre);
   static void Smooth(double *data,
                      int iArraySize);
   static void PeakExtract(double *data,
                           int iArraySize);
   static void GetTopIons(double *pdTempRawData,
                          struct msdata *pTempSpData,
                          int iArraySize);
   static int QsortByIon(const void *p0,
                         const void *p1);
   static void StairStep(struct msdata *pTempSpData);


   // Private member variables
   static Mutex _maxChargeMutex;
};

#endif // _COMETPREPROCESS_H_
