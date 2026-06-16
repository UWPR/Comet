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
#include "FastaStrategy.h"
#include "SearchUtils.h"
#include "CometPreprocess.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometSearchManager.h"
#include "MSReader.h"

bool FastaStrategy::initialize(SearchSession& /*session*/, ThreadPool* /*tp*/)
{
   if (!CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   if (!CometSearch::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   return true;
}

bool FastaStrategy::openFiles(const std::string& szDatabase,
                              FILE*& fpfasta, FILE*& fpidx, FILE*& fpdb,
                              SearchSession& session)
{
   fpfasta = nullptr;
   fpidx   = nullptr;
   fpdb    = nullptr;

   if (!session.bPerformDatabaseSearch)
      return true;

   if ((fpfasta = fopen(szDatabase.c_str(), "r")) == nullptr)
   {
      string strErrorMsg = " Error (1b) - cannot read sequence database file \"" + szDatabase + "\".\n";
      session.statusRef.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   fpdb = fpfasta;
   (void)session;   // session.bIdxNoFasta stays false for FASTA searches

   return true;
}

bool FastaStrategy::executeBatch(MSToolkit::MSReader& mstReader,
                                 int iFirstScan, int iLastScan, int iAnalysisType,
                                 int& iPercentStart, int& iPercentEnd,
                                 ThreadPool* tp, SearchSession& session)
{
   if (!g_staticParams.options.bOutputSqtStream)
   {
      logout("   - Load spectra:");
      fflush(stdout);
   }

   session.statusRef.SetStatusMsg(string("Loading and processing input spectra"));

   bool bSucceeded = CometPreprocess::LoadAndPreprocessSpectra(
         mstReader, iFirstScan, iLastScan, iAnalysisType, tp, session);

   iPercentStart = iPercentEnd;
   iPercentEnd   = mstReader.getPercent();

   if (!bSucceeded)
      return false;

   if (session.queries.empty())
      return true;

   bSucceeded = AllocateResultsMem(session.queries);
   if (!bSucceeded)
      return false;

   {
      string strStatusMsg = " " + std::to_string(session.queries.size()) + string("\n");
      if (!g_staticParams.options.bOutputSqtStream)
         logout(strStatusMsg);
      session.statusRef.SetStatusMsg(strStatusMsg);
   }

   return RunSearchAndPostAnalysis(iPercentStart, iPercentEnd, tp, session, true);
}

void FastaStrategy::closeFiles(FILE* fpfasta, FILE* fpidx)
{
   (void)fpidx;   // always nullptr for FASTA searches
   if (fpfasta != nullptr) fclose(fpfasta);
}

void FastaStrategy::finalize()
{
   CometPreprocess::DeallocateMemory(g_staticParams.options.iNumThreads);
   CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);
}
