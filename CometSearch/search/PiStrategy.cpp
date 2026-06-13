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
   (void)session;
   (void)tp;

   // The peptide index is loaded lazily on first access inside
   // CometSearch::RunSearch -> SearchPeptideIndex.  No explicit
   // ReadPeptideIndex() call is needed here.

   if (!CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads))
      return false;

   if (!CometSearch::AllocateMemory(g_staticParams.options.iNumThreads))
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
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
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
   g_cometStatus.SetStatusMsg(string("Loading and processing input spectra"));

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
      g_cometStatus.SetStatusMsg(strStatusMsg);
   }

   std::sort(session.queries.begin(), session.queries.end(), compareByPeptideMass);

   g_massRange.dMinMass = session.queries.at(0)->_pepMassInfo.dPeptideMassToleranceMinus;
   g_massRange.dMaxMass = session.queries.at(session.queries.size() - 1)->_pepMassInfo.dPeptideMassTolerancePlus;

   if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
      g_massRange.bNarrowMassRange = true;
   else
      g_massRange.bNarrowMassRange = false;

   bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
   if (!bSucceeded)
      return false;

   g_cometStatus.SetStatusMsg(string("Running search..."));

   if (session.bPerformDatabaseSearch)
      bSucceeded = CometSearch::RunSearch(iPercentStart, iPercentEnd, tp, session.queries);
   if (bSucceeded && session.bPerformSpecLibSearch)
      bSucceeded = CometSearch::RunSpecLibSearch(iPercentStart, iPercentEnd, tp, session.queries);

   if (!bSucceeded)
      return false;

   bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
   if (!bSucceeded)
      return false;

   if (session.bPerformDatabaseSearch)
   {
      g_cometStatus.SetStatusMsg(string("Performing post-search analysis ..."));
      bSucceeded = CometPostAnalysis::PostAnalysis(tp, session.queries);
   }

   return bSucceeded;
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
