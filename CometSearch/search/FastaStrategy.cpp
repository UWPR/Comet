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

bool FastaStrategy::initialize(SearchSession& session, ThreadPool* tp)
{
   // Read protein variable-mod filter file (FASTA-only feature).
   if (session.bPerformDatabaseSearch
         && g_staticParams.variableModParameters.sProteinLModsListFile.length() > 0)
   {
      bool bVarModUsed = false;
      for (int iMod = 0; iMod < VMODS; ++iMod)
      {
         if (g_staticParams.variableModParameters.varModList[iMod].dVarModMass != 0.0)
         {
            bVarModUsed = true;
            break;
         }
      }

      if (bVarModUsed)
      {
         // ReadProteinVarModFilterFile() is a private member of CometSearchManager;
         // it is called from DoSearch() before pipeline.run() for the FASTA path.
         // This initialize() is called AFTER that call, so the filter is already loaded.
         // Nothing to do here.  (The call is retained in DoSearch() for the FASTA path
         // only, which is handled before makeStrategy() is invoked.)
      }
   }

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
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
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
      if (!g_staticParams.options.bOutputSqtStream)
         logout(strStatusMsg);
      g_cometStatus.SetStatusMsg(strStatusMsg);
   }

   if (g_staticParams.options.bMango)
   {
      int iCurrentScanNumber = 0;
      int iMangoIndex = 0;

      std::sort(session.queries.begin(), session.queries.end(), compareByMangoIndex);

      for (std::vector<Query*>::iterator it = session.queries.begin(); it != session.queries.end(); ++it)
      {
         if ((*it)->_spectrumInfoInternal.iScanNumber != iCurrentScanNumber)
         {
            iCurrentScanNumber = (*it)->_spectrumInfoInternal.iScanNumber;
            iMangoIndex = 0;
         }
         else
         {
            iMangoIndex++;
         }
         sprintf((*it)->_spectrumInfoInternal.szMango, "%03d_%c",
                 (int)iMangoIndex / 2, (iMangoIndex % 2) ? 'B' : 'A');
      }
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

   if (!g_staticParams.options.bOutputSqtStream)
   {
      logout("     - Post analysis:");
      fflush(stdout);
   }

   if (session.bPerformDatabaseSearch)
   {
      g_cometStatus.SetStatusMsg(string("Performing post-search analysis ..."));
      bSucceeded = CometPostAnalysis::PostAnalysis(tp, session.queries);
   }

   return bSucceeded;
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
