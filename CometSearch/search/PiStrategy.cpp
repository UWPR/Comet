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
#include "PiStrategy.h"
#include "SearchUtils.h"
#include "CometPreprocess.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "MSReader.h"

bool PiStrategy::initialize(SearchSession& session, ThreadPool* tp)
{
   (void)tp;

   if (!CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   if (!CometSearch::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   // Load the peptide index up front (mirrors FiStrategy::initialize()) so the
   // fused per-spectrum search below never has to lazily trigger it mid-stream.
   // EnsurePeptideIndexLoaded() itself logs the "Read peptide index: ..." status
   // line and is a no-op if already loaded.
   if (session.bPerformDatabaseSearch && !CometSearch::EnsurePeptideIndexLoaded(false))
      return false;

   return true;
}

bool PiStrategy::openFiles(const std::string& szDatabase,
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

   // Try to open the companion .fasta (not required for PI_DB search).
   sTmpDB = sTmpDB.erase(sTmpDB.size() - 4);   // strip .idx
   if ((fpfasta = fopen(sTmpDB.c_str(), "r")) == nullptr)
   {
      session.bIdxNoFasta = true;
      fpfasta = nullptr;
   }

   fpdb = fpidx;

   return true;
}

bool PiStrategy::executeBatch(MSToolkit::MSReader& mstReader,
                              int iFirstScan, int iLastScan, int iAnalysisType,
                              int& iPercentStart, int& iPercentEnd,
                              ThreadPool* tp, SearchSession& session)
{
   // Fused path: per-spectrum read+preprocess+search+post-analysis in one pass,
   // same as FiStrategy. Disabled for Mango or speclib runs (those require the
   // legacy ordering), matching FiStrategy's own restriction.
   bool bFused = session.bPerformDatabaseSearch
                 && !g_staticParams.options.bMango
                 && !session.bPerformSpecLibSearch;

   if (bFused)
   {
      session.statusRef.SetStatusMsg(string("Running fused PI_DB search..."));

      bool bSucceeded = CometPreprocess::FusedLoadAndSearchSpectra(
            mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session);

      iPercentStart = iPercentEnd;
      iPercentEnd   = mstReader.getPercent();

      return bSucceeded;
   }

   return executeBatchLegacy(mstReader, iFirstScan, iLastScan, iAnalysisType,
                             iPercentStart, iPercentEnd, tp, session, false);
}

void PiStrategy::closeFiles(FILE* fpfasta, FILE* fpidx)
{
   if (fpidx   != nullptr) fclose(fpidx);
   if (fpfasta != nullptr) fclose(fpfasta);
}

void PiStrategy::finalize()
{
   CometPreprocess::DeallocateMemory(g_staticParams.options.iNumThreads);
   CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
}
