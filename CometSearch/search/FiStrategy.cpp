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

#include <cstdlib>
#include <memory>
#include <mutex>
#include "Common.h"
#include "FiStrategy.h"
#include "SearchUtils.h"
#include "CometFragmentIndex.h"
#include "CometPreprocess.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometMassSpecUtils.h"
#include "MSReader.h"

extern std::vector<InputFileInfo*> g_pvInputFiles;
extern std::atomic<bool> g_bPlainPeptideIndexRead;
extern unsigned int*  g_iFragmentIndex;
extern uint64_t*      g_iFragmentIndexOffset;
extern bool*          g_bIndexPrecursors;

bool FiStrategy::initialize(SearchSession& session, ThreadPool* tp)
{
   if (!CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   if (!CometSearch::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   // Pre-read precursors across all input files before building the index.
   if (session.bPerformDatabaseSearch && !g_staticParams.options.iFragIndexSkipReadPrecursors)
   {
      auto tTime1 = chrono::steady_clock::now();
      if (!g_staticParams.options.bOutputSqtStream)
      {
         cout << " - read precursors ... ";
         fflush(stdout);
      }

      for (int i = 0; i < (int)g_pvInputFiles.size(); ++i)
      {
         if (!UpdateInputFile(g_pvInputFiles.at(i)))
            return false;
      }

      // Each file is read by one pool task into a bin array of its own (reads are header-only and
      // independent) that the task OR-s into g_bIndexPrecursors when it finishes and then frees, so
      // mask memory scales with the number of running tasks, not with the number of input files
      // (one array per file would be ~0.5 MB each for 0.01 Da bins -- over 600 MB for 1288 files).
      const size_t iNumBins = (size_t)BIN(g_staticParams.options.dPeptideMassHigh) + 1;
      mutex mutexMergeBins;
      atomic<bool> bReadOK(true);

      // The tasks capture iNumBins, mutexMergeBins and bReadOK by reference.  That is safe because every
      // path from here to tp->wait_on_threads() below reaches it (doJob() throws only when the pool was
      // never filled, which DoSearch() always does before a strategy runs), so these locals outlive all tasks.
      for (size_t i = 0; i < g_pvInputFiles.size(); ++i)
      {
         tp->doJob([&, i]()
         {
            unique_ptr<bool[]> pBins(new bool[iNumBins]());   // value-initialized: all false
            MSReader mstReader;
            SetMSLevelFilter(mstReader);

            if (!CometPreprocess::ReadPrecursors(mstReader, g_pvInputFiles.at(i)->szFileName, pBins.get()))
            {
               bReadOK = false;
               return;
            }

            lock_guard<mutex> lock(mutexMergeBins);
            for (size_t x = 0; x < iNumBins; ++x)
            {
               if (pBins[x])
                  g_bIndexPrecursors[x] = true;
            }
         });
      }

      tp->wait_on_threads();

      if (!bReadOK)
         return false;

      if (!g_staticParams.options.bOutputSqtStream)
         cout << CometMassSpecUtils::ElapsedTime(tTime1) << endl;
   }

   // Load plain peptide index (.idx) and build the in-memory fragment index.
   if (session.bPerformDatabaseSearch && !g_bPlainPeptideIndexRead)
   {
      auto tStartTime = chrono::steady_clock::now();
      if (!g_staticParams.options.bOutputSqtStream)
      {
         cout << " - read .idx ... ";
         fflush(stdout);
      }

      CometFragmentIndex sqSearch;
      if (!CometPeptideIndex::ReadPeptideIndex(false))   // batch path
         return false;

      if (!g_staticParams.options.bOutputSqtStream)
         cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

      if (!sqSearch.CreateFragmentIndex(tp, false))
         return false;
   }

   return true;
}

bool FiStrategy::openFiles(const std::string& szDatabase,
                           FILE*& fpfasta, FILE*& fpidx, FILE*& fpdb,
                           SearchSession& session)
{
   fpfasta = nullptr;
   fpidx   = nullptr;
   fpdb    = nullptr;

   if (!session.bPerformDatabaseSearch)
      return true;

   string sTmpDB = szDatabase;

   if ((fpidx = fopen(sTmpDB.c_str(), "r")) == nullptr)
   {
      string strErrorMsg = " Error (1a) - cannot read .idx file \"" + sTmpDB + "\".\n";
      session.statusRef.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Try to open the companion .fasta (not required for FI_DB search).
   sTmpDB = sTmpDB.erase(sTmpDB.size() - 4);   // strip .idx
   if ((fpfasta = fopen(sTmpDB.c_str(), "r")) == nullptr)
   {
      session.bIdxNoFasta = true;
      fpfasta = nullptr;
   }

   fpdb = fpidx;

   return true;
}

bool FiStrategy::executeBatch(MSToolkit::MSReader& mstReader,
                              int iFirstScan, int iLastScan, int iAnalysisType,
                              int& iPercentStart, int& iPercentEnd,
                              ThreadPool* tp, SearchSession& session)
{
   // Fused path: per-spectrum read+preprocess+search+post-analysis in one pass.
   // Disabled for Mango or speclib runs (those require the legacy ordering).
   bool bFused = session.bPerformDatabaseSearch
                 && !g_staticParams.options.bMango
                 && !session.bPerformSpecLibSearch;

   if (bFused)
   {
      session.statusRef.SetStatusMsg(string("Running fused FI_DB search..."));

      // DIAGNOSTIC / BENCHMARK ONLY: COMET_PRELOAD_BENCHMARK opts into
      // FusedPreloadThenSearch (read the whole file into memory first, then
      // search, separately timed) instead of the normal streaming fused path.
      // See CometPreprocess.cpp's FusedPreloadThenSearch comment and
      // docs/20260724_PreloadBenchmark.md. Default behavior (env var unset) is
      // completely unchanged.
      bool bSucceeded = getenv("COMET_PRELOAD_BENCHMARK") != nullptr
            ? CometPreprocess::FusedPreloadThenSearch(
                  mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session)
            : CometPreprocess::FusedLoadAndSearchSpectra(
                  mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session);

      iPercentStart = iPercentEnd;
      iPercentEnd   = mstReader.getPercent();

      return bSucceeded;
   }

   // Legacy three-sweep path: LoadAndPreprocess -> AllocateResults ->
   // sort-by-mass -> RunSearch -> PostAnalysis.
   return executeBatchLegacy(mstReader, iFirstScan, iLastScan, iAnalysisType,
                             iPercentStart, iPercentEnd, tp, session, false);
}

void FiStrategy::closeFiles(FILE* fpfasta, FILE* fpidx)
{
   if (fpidx   != nullptr) fclose(fpidx);
   if (fpfasta != nullptr) fclose(fpfasta);
}

void FiStrategy::finalize()
{
   if (g_staticParams.iDbType == DbType::FI_DB)
   {
      free(g_bIndexPrecursors);
      delete[] g_iFragmentIndex;
      delete[] g_iFragmentIndexOffset;

      // Reset so a subsequent DoSearch() in the same process (batch run after an
      // RTS session, or a second batch run) rebuilds the index instead of reusing
      // these now-freed pointers; g_bPlainPeptideIndexRead gates that rebuild in
      // FiStrategy::initialize() and is otherwise never reset to false.
      g_bIndexPrecursors       = nullptr;
      g_iFragmentIndex         = nullptr;
      g_iFragmentIndexOffset   = nullptr;
      g_bPlainPeptideIndexRead = false;
   }

   CometPreprocess::DeallocateMemory(g_staticParams.options.iNumThreads);
   CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
}
