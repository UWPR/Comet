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
                                        ThreadPool* tp);
   static void PreprocessThreadProc(PreprocessThreadData *pPreprocessThreadData,
                                    ThreadPool* tp);
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
                                  double *pdTmpFastXcorrData,
                                  double *pdTmpCorrelationData);
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
                          double *pdTmpFastXcorrData,
                          double *pdTmpCorrelationData);
   static bool LoadIons(struct Query *pScoring,
                        double *pdTmpRawData,
                        Spectrum mstSpectrum,
                        struct PreprocessStruct *pPre);
   static void MakeCorrData(double *pdTmpRawData,
                            double *pdTmpCorrelationData,
                            struct Query *pScoring,
                            struct PreprocessStruct *pPre);
   static bool SortByIon(const struct msdata &a,
                         const struct msdata &b);
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
};

#endif // _COMETPREPROCESS_H_
