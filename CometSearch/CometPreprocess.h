// Copyright 2012-2026 Jimmy Eng
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


#ifndef _COMETPREPROCESS_H_
#define _COMETPREPROCESS_H_

#include "ThreadPool.h"
#include "search/SearchSession.h"
#include <atomic>

// Defined file-local in CometPreprocess.cpp; only referenced here by
// reference, so a forward declaration is sufficient.
struct BoundedSpectrumQueue;

struct PreprocessThreadData
{
   Spectrum mstSpectrum;
   int iAnalysisType;
   int iFileLastScan;
   bool *pbMemoryPool;  //MH: Manages active memory pool
   SearchSession* pSession;

   PreprocessThreadData()
      : mstSpectrum(), iAnalysisType(0), iFileLastScan(0), pbMemoryPool(nullptr), pSession(nullptr)
   {
   }

   PreprocessThreadData(Spectrum& spec_in,
                        int iAnalysisType_in,
                        int iFileLastScan_in)
      : mstSpectrum(spec_in), iAnalysisType(iAnalysisType_in), iFileLastScan(iFileLastScan_in), pbMemoryPool(nullptr), pSession(nullptr)
   {
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
   static bool ReadPrecursors(MSReader &mstReader);
   static bool LoadAndPreprocessSpectra(MSReader &mstReader,
                                        int iFirstScan,
                                        int iLastScan,
                                        int iAnalysisType,
                                        ThreadPool* tp,
                                        SearchSession& session);
   static void PreprocessThreadProc(PreprocessThreadData *pPreprocessThreadData,
                                    ThreadPool* tp);
   static void PreprocessThreadProcMS1(PreprocessThreadData* pPreprocessThreadDataMS1,
                                       ThreadPool* tp,
                                       const double dMaxQueryRT,
                                       const double dMaxSpecLibRT);
   static bool DoneProcessingAllSpectra();
   static bool AllocateMemory(int maxNumThreads);
   static bool DeallocateMemory(int maxNumThreads);
   static bool PreprocessSingleSpectrum(int iPrecursorCharge,
                                        double dMZ,
                                        double *pdMass,
                                        double *pdInten,
                                        int iNumPeaks,
                                        double *pdTmpSpectrum,
                                        SearchSession& session);

   // Thread-local version: returns Query* without touching g_pvQuery.
   // Caller owns the returned Query* and must delete it when done.
   static Query* PreprocessSingleSpectrumThreadLocal(int iPrecursorCharge,
                                                     double dMZ,
                                                     double *pdMass,
                                                     double *pdInten,
                                                     int iNumPeaks,
                                                     double *pdTmpSpectrum);

   static bool PreprocessMS1SingleSpectrum(double* pdMass,
                                           double* pdInten,
                                           int iNumPeaks,
                                           SearchSession& session);
   // Thread-local version: returns QueryMS1* without touching g_pvQueryMS1.
   // Caller owns the returned QueryMS1* and must delete it when done.
   static QueryMS1* PreprocessMS1SingleSpectrumThreadLocal(double* pdMass,
                                                           double* pdInten,
                                                           int iNumPeaks);

   static double GetMassCushion(double dMass);

   // Fused FI_DB/PI_DB batch path: preprocess + search + post-analysis for one
   // spectrum in a single pass using thread-local scratch buffers.  iSlot is this
   // worker's pre-assigned _ppbDuplFragmentArr index.  outQueries is this worker's
   // own result vector (no lock needed); outCount is a shared running total bumped
   // with relaxed ordering for the CheckExit batch-size-cap read.  pArena is this
   // worker's per-slot sparse-XCorr-matrix bump arena (session.sparseArenas[iSlot]),
   // used instead of a per-spectrum heap allocation for each Query's sparse child
   // blocks -- see docs/20260723_ExtendFusedBatchPath.md.  The actual index search
   // (FI_DB vs. PI_DB) is dispatched by CometSearch::RunSearch(Query*, int).
   static void FusedSearchSpectrum(Spectrum spec,
                                   int iSlot,
                                   std::vector<Query*>& outQueries,
                                   std::atomic<size_t>& outCount,
                                   FusedSparseArena* pArena);

   // Fused FI_DB/PI_DB batch path: stream spectra through a bounded producer/
   // consumer queue into FusedSearchSpectrum workers.  Replaces
   // LoadAndPreprocessSpectra + AllocateResultsMem + RunSearch + PostAnalysis for
   // both index-based search types -- see FiStrategy::executeBatch() and
   // PiStrategy::executeBatch().
   static bool FusedLoadAndSearchSpectra(MSReader& mstReader,
                                          int iFirstScan,
                                          int iLastScan,
                                          int iAnalysisType,
                                          ThreadPool* tp,
                                          SearchSession& session);

   // Returns the thread-local raw-data buffer used by PreprocessSingleSpectrumThreadLocal.
   // The buffer is sized to g_staticParams.iArraySizeGlobal and its content after a
   // PreprocessSingleSpectrumThreadLocal call holds the binned sqrt-intensity spectrum
   // needed for fragment-ion matching in DoSingleSpectrumSearchMultiResults.
   // Calling this also initialises the thread-local RtsScratch pool if necessary.
   static double* GetRtsRawDataBuffer();

   static void PreloadIons(MSReader& mstReader,
                           Spectrum& spec,
                           bool bNext = false,
                           int scNum = 0);
   static bool CheckExit(int iAnalysisType,
                         int iScanNum,
                         int iTotalScans,
                         int iLastScan,
                         int iReaderLastScan,
                         int iNumSpectraLoaded,
                         bool bIgnoreSpectrumBatchSize);
   static bool IsValidInputType(int inputType);

private:

   // Private static methods
   static bool PreprocessSpectrum(Spectrum &spec,
                                  double *pdTmpRawData,
                                  double *pdTmpFastXcorrData,
                                  double *pdTmpCorrelationData,
                                  float *pfFastXcorrData,
                                  float *pfFastXcorrDataNL,
                                  float *pfSpScoreData,
                                  SearchSession* pSession);
   static bool AdjustMassTol(struct Query *pScoring);
   static bool CheckActivationMethodFilter(MSActivation act);

   // Shared by both the synchronous and readahead producer loops in
   // FusedLoadAndSearchSpectra so a future filter change (clearMzRange,
   // iMinPeaks, activation method) cannot land in only one of the two call
   // sites and silently diverge between them.  Mutates mstSpectrum in place
   // (clearMzRange zeroes cleared-range intensities) and moves it into queue
   // if it survives all three filters.  Returns true iff enqueued.
   static bool FilterAndEnqueueSpectrum(Spectrum& mstSpectrum,
                                        BoundedSpectrumQueue& queue);
   // pArena: when non-null, each sparse-matrix child block ([SPARSE_MATRIX_SIZE]
   // leaf array) is taken from this bump arena instead of individually heap-
   // allocated, and pScoring->bSparseFromPool is set so the Query destructor skips
   // delete[]-ing them (see docs/20260723_ExtendFusedBatchPath.md). Currently only
   // the fused batch path (FusedSearchSpectrum) passes non-null; nullptr preserves
   // the prior per-spectrum-heap-allocation behavior for any other caller.
   static bool Preprocess(struct Query *pScoring,
                          Spectrum mstSpectrum,
                          double *pdTmpRawData,
                          double *pdTmpFastXcorrData,
                          double *pdTmpCorrelationData,
                          float *pfFastXcorrData,
                          float *pfFastXcorrDataNL,
                          float *pfSpScoreData,
                          FusedSparseArena *pArena = nullptr);
   static bool LoadIons(struct Query *pScoring,
                        double *pdTmpRawData,
                        Spectrum mstSpectrum,
                        struct PreprocessStruct *pPre);
   static void MakeCorrData(double* pdTmpRawData,
                            double* pdTmpCorrelationData,
                            int iHighestIon,
                            double dHighestIntensity);

   // Shared core of PreprocessSingleSpectrum and PreprocessSingleSpectrumThreadLocal.
   // Builds a fully preprocessed Query* from the input spectrum data.
   // Does NOT push the Query* into g_pvQuery.
   // When bUseThreadLocalPool=true the five scratch buffers and sparse child arrays
   // are taken from the per-thread RtsScratch pool instead of being heap-allocated,
   // which eliminates ~6 large malloc/free pairs and ~100 small ones per spectrum.
   // Only PreprocessSingleSpectrumThreadLocal passes true here; the batch path must
   // use false because pooled Queries are destroyed before the pool is reset.
   static Query* PreprocessSingleSpectrumCore(int iPrecursorCharge,
                                              double dMZ,
                                              double *pdMass,
                                              double *pdInten,
                                              int iNumPeaks,
                                              double *pdTmpSpectrum,
                                              bool bUseThreadLocalPool = false);

   // Private member variables
   static Mutex _maxChargeMutex;
   static bool _bFirstScan;
   static bool _bDoneProcessingAllSpectra;

   //MH: Common memory to be shared by all threads during spectral processing
   static bool *pbMemoryPool;                 //MH: Regulator of memory use
   static double **ppdTmpRawDataArr;          //MH: Number of arrays equals threads
   static double **ppdTmpFastXcorrDataArr;    //MH: Ditto
   static double **ppdTmpCorrelationDataArr;  //MH: Ditto
   static float** ppfFastXcorrData;           //MH: Replacing temporary arrays using by Query
   static float** ppfFastXcorrDataNL;         //MH: Ditto
   static float** ppfSpScoreData;             //MH: Ditto

};

#endif // _COMETPREPROCESS_H_
