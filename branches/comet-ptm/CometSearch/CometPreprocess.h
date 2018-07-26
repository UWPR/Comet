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
   bool *pbMemoryPool;  //MH: Manages active memory pool

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
      //MH: Mark that the memory is no longer in use.
      //DO NOT FREE MEMORY HERE. Just release pointer.
      Threading::LockMutex(g_preprocessMemoryPoolMutex);

      if(pbMemoryPool!=NULL)
         *pbMemoryPool=false;
      pbMemoryPool=NULL;

      Threading::UnlockMutex(g_preprocessMemoryPoolMutex);
   }

   void SetMemory(bool *pbMemoryPool_in)
   {
      pbMemoryPool = pbMemoryPool_in;
   }
};


// Comet-PTM
struct less_than_Peaks
{
   inline bool operator() (const Peak_T& left, const Peak_T& right)
   {
      return (left.mz < right.mz);
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
   static bool AllocateMemory(int maxNumThreads);
   static bool DeallocateMemory(int maxNumThreads);
   static bool PreprocessSingleSpectrum(int iPrecursorCharge,
                                        double dMZ,
                                        double *pdMass,
                                        double *pdInten,
                                        int iNumPeaks,
                                        double *pdTmpSpectrum);

private:

   // Private static methods
   static bool PreprocessSpectrum(Spectrum &spec,
                                  double *pdTmpRawData,
                                  double *pdQTmpRawData,
                                  double *pdTmpFastXcorrData,
                                  double *pdTmpCorrelationData,
                                  double *pdQTmpCorrelationData,
                                  double *pdTmpSmoothedSpectrum,
                                  double *pdTmpPeakExtracted);
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
                          Spectrum mstSpectrum,
                          double *pdTmpRawData,
                          double *pdQTmpRawData,
                          double *pdTmpFastXcorrData,
                          double *pdTmpCorrelationData,
                          double *pdQTmpCorrelationData,
                          double *pdSmoothedSpectrum,
                          double *pdTmpPeakExtracted);
   static bool LoadIons(struct Query *pScoring,
                        double *pdTmpRawData,
                        double *pdQTmpRawData,
                        Spectrum mstSpectrum,
                        struct PreprocessStruct *pPre);
   static void MakeCorrData(double *pdTmpRawData,
                            double *pdQTmpRawData,
                            double *pdTmpCorrelationData,
                            double *pdQTmpCorrelationData,
                            struct Query *pScoring,
                            struct PreprocessStruct *pPre);
   static bool Smooth(double *data,
                      int iArraySize,
                      double *pdSmoothedSpectrum);
   static bool PeakExtract(double *data,
                           int iArraySize,
                           double *pdTmpPeakExtracted);
   static void GetTopIons(double *pdTmpRawData,
                          double *pdQTmpRawData,
                          struct msdata *pTmpSpData,
                          struct msdata *pTmpQData,
                          int iArraySize);
   static int QsortByIon(const void *p0,
                         const void *p1);
   static void StairStep(struct msdata *pTmpSpData);
   static bool IsValidInputType(int inputType);


   // Private member variables
   static Mutex _maxChargeMutex;
   static bool _bFirstScan;
   static bool _bDoneProcessingAllSpectra;

   //MH: Common memory to be shared by all threads during spectral processing
   static bool *pbMemoryPool;                 //MH: Regulator of memory use
   static double **ppdTmpRawDataArr;          //MH: Number of arrays equals threads
   static double **ppdTmpFastXcorrDataArr;    //MH: Ditto
   static double **ppdTmpCorrelationDataArr;  //MH: Ditto
   static double **ppdTmpSmoothedSpectrumArr; //MH: Ditto
   static double **ppdTmpPeakExtractedArr;    //MH: Ditto
   static double **ppdQTmpRawDataArr;         // Comet-PTM
   static double **ppdQTmpCorrelationDataArr; // Comet-PTM

};

#endif // _COMETPREPROCESS_H_
