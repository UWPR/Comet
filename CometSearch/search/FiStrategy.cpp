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
extern bool g_bPlainPeptideIndexRead;
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

         MSReader mstReader;
         SetMSLevelFilter(mstReader);
         CometPreprocess::Reset();

         if (!CometPreprocess::ReadPrecursors(mstReader))
            return false;
      }

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
      sqSearch.ReadPlainPeptideIndex();

      if (!g_staticParams.options.bOutputSqtStream)
         cout << CometMassSpecUtils::ElapsedTime(tStartTime) << endl;

      sqSearch.CreateFragmentIndex(tp);
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

      bool bSucceeded = CometPreprocess::FusedLoadAndSearchSpectra(
            mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session);

      iPercentStart = iPercentEnd;
      iPercentEnd   = mstReader.getPercent();

      return bSucceeded;
   }

   // Legacy three-sweep path: LoadAndPreprocess -> AllocateResults ->
   // sort-by-mass -> RunSearch -> PostAnalysis.
   session.statusRef.SetStatusMsg(string("Loading and processing input spectra"));

   bool bSucceeded = CometPreprocess::LoadAndPreprocessSpectra(
         mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session);

   iPercentStart = iPercentEnd;
   iPercentEnd   = mstReader.getPercent();

   if (!bSucceeded)
      return false;

   if (session.queries.empty())
      return true;   // no spectra in this batch; caller will continue to next

   bSucceeded = AllocateResultsMem(session.queries);
   if (!bSucceeded)
      return false;

   {
      string strStatusMsg = " " + std::to_string(session.queries.size()) + string("\n");
      session.statusRef.SetStatusMsg(strStatusMsg);
   }

   return RunSearchAndPostAnalysis(iPercentStart, iPercentEnd, tp, session);
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
   }

   CometPreprocess::DeallocateMemory(g_staticParams.options.iNumThreads);
   CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
}
