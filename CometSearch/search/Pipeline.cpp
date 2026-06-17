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
#include "Pipeline.h"
#include "SearchUtils.h"
#include "CometPreprocess.h"
#include "CometMassSpecUtils.h"
#include "MSReader.h"

Pipeline::Pipeline(std::unique_ptr<ISearchStrategy>            strategy,
                   std::vector<std::unique_ptr<IResultWriter>> writers,
                   CometSearchManager*                         pMgr)
   : _strategy(std::move(strategy))
   , _writers(std::move(writers))
   , _pMgr(pMgr)
{
}

bool Pipeline::run(SearchSession&                     session,
                   const std::vector<InputFileInfo*>& files,
                   ThreadPool&                        tp)
{
   auto tGlobalStart = chrono::steady_clock::now();

   if (!_strategy->initialize(session, &tp))
   {
      _strategy->finalize();
      return false;
   }

   bool bSucceeded      = true;
   int  iTotalAllFiles  = 0;   // spectra searched across all files (for blank-file check)

   for (auto pFile : files)
   {
      if (!UpdateInputFile(pFile))
      {
         bSucceeded = false;
         break;
      }

      int iFirstScan    = g_staticParams.inputFile.iFirstScan;
      int iLastScan     = g_staticParams.inputFile.iLastScan;
      int iPercentStart = 0;
      int iPercentEnd   = 0;
      int iAnalysisType = g_staticParams.inputFile.iAnalysisType;

      // Print search-start banner for FASTA searches.
      if (!g_staticParams.options.bOutputSqtStream && !_strategy->isIndexBased())
      {
         time_t tStartTime;
         time(&tStartTime);
         strftime(g_staticParams.szDate, 26, "%Y/%m/%d, %I:%M:%S %p", localtime(&tStartTime));

         string strOut  = " Search start:  " + string(g_staticParams.szDate) + "\n";
         strOut += " - Input file: " + string(g_staticParams.inputFile.szFileName) + "\n";
         logout(strOut);
         fflush(stdout);
      }

      // Open database file handles (strategy-specific: .idx or .fasta).
      FILE* fpfasta = nullptr;
      FILE* fpidx   = nullptr;
      FILE* fpdb    = nullptr;

      if (!_strategy->openFiles(g_staticParams.databaseInfo.szDatabase,
                                fpfasta, fpidx, fpdb, session))
      {
         bSucceeded = false;
         break;
      }

      if (g_staticParams.options.iSpectrumBatchSize == 0 && !_strategy->isIndexBased())
      {
         logout("   - Reading all spectra into memory; set \"spectrum_batch_size\" if search terminates here.\n");
         fflush(stdout);
      }

      // Open writers (after openFiles so session.bIdxNoFasta is correctly set).
      WriterOpenCtx woctx;
      woctx.szBaseName     = g_staticParams.inputFile.szBaseName;
      woctx.szOutputSuffix = g_staticParams.szOutputSuffix;
      woctx.szTxtFileExt   = g_staticParams.szTxtFileExt;
      woctx.bEntireFile    = (iAnalysisType == AnalysisType_EntireFile);
      woctx.iFirstScan     = iFirstScan;
      woctx.iLastScan      = iLastScan;
      woctx.iDecoySearch   = g_staticParams.options.iDecoySearch;
      woctx.bIdxNoFasta    = session.bIdxNoFasta;
      woctx.pMgr           = _pMgr;
      woctx.pStatus        = &session.statusRef;

      for (auto& pw : _writers)
      {
         if (!pw->open(woctx))
         {
            bSucceeded = false;
            break;
         }
      }

      if (!bSucceeded)
      {
         for (auto& pw : _writers) pw->close(false, false);
         _strategy->closeFiles(fpfasta, fpidx);
         break;
      }

      // MSReader setup.
      MSReader mstReader;
      SetMSLevelFilter(mstReader);
      CometPreprocess::Reset();

      // Print "searching..." message for index-based searches.
      auto tBeginTime = chrono::steady_clock::now();
      if (_strategy->isIndexBased())
      {
         printf(" - searching \"%s\" ... ", g_staticParams.inputFile.szBaseName);
         fflush(stdout);
      }

      int  iTotalSpectraSearched = 0;
      int  iBatchNum             = 0;

      auto cleanupBatch = [&]()
      {
         for (auto* q : session.queries) delete q;
         session.queries.clear();
         for (auto* q : session.ms1Queries) delete q;
         session.ms1Queries.clear();
      };

      while (!CometPreprocess::DoneProcessingAllSpectra())
      {
         iBatchNum++;

         bSucceeded = _strategy->executeBatch(mstReader,
                                              iFirstScan, iLastScan, iAnalysisType,
                                              iPercentStart, iPercentEnd,
                                              &tp, session);

         if (!bSucceeded)
         {
            cleanupBatch();
            break;
         }

         if (session.queries.empty())
            continue;

         iTotalSpectraSearched += (int)session.queries.size();

         // Sort by scan number (shared by all paths; SQT writes last, which modifies szMod).
         std::sort(session.queries.begin(), session.queries.end(), compareByScanNumber);

         if (!g_staticParams.options.bOutputSqtStream && !_strategy->isIndexBased())
         {
            logout("  done\n");
            fflush(stdout);
         }

         // Per-batch write.
         {
            WriterWriteCtx wwctx;
            wwctx.fpdb        = fpdb;
            wwctx.iScanOffset = iTotalSpectraSearched - (int)session.queries.size();
            wwctx.iBatchNum   = iBatchNum;
            wwctx.pQueries    = &session.queries;

            for (auto& pw : _writers)
            {
               if (!pw->write(wwctx))
               {
                  bSucceeded = false;
                  break;
               }
            }
         }

         cleanupBatch();

         if (!bSucceeded)
            break;
      }

      // Per-file timing and run-stats message.
      if (bSucceeded)
      {
         if (iTotalSpectraSearched == 0)
            logout(" Warning - no spectra searched.\n");

         if (!g_staticParams.options.bOutputSqtStream)
         {
            const auto duration = chrono::duration_cast<chrono::milliseconds>(
                  chrono::steady_clock::now() - tBeginTime);
            double dTimePerSpectra = (iTotalSpectraSearched > 0)
                  ? (double)duration.count() / (double)iTotalSpectraSearched
                  : 0.0;

            string strOut;
            char buf[128];

            if (!_strategy->isIndexBased())
               strOut = "     - Run stats: ";
            else
               strOut = "";

            std::snprintf(buf, sizeof(buf), "%.2f", dTimePerSpectra);
            strOut += CometMassSpecUtils::ElapsedTime(tBeginTime)
                      + " (" + std::to_string(iTotalSpectraSearched) + " spectra, "
                      + std::string(buf) + "ms/spec, ";

            std::snprintf(buf, sizeof(buf), "%.0f", (dTimePerSpectra > 0.0) ? 1000.0 / dTimePerSpectra : 0.0);
            strOut += std::string(buf) + "Hz";

            if (!_strategy->isIndexBased())
               strOut += ", " + CometMassSpecUtils::GetPeakMemory();

            strOut += ")\n";
            logout(strOut);
         }

         if (!g_staticParams.options.bOutputSqtStream && !_strategy->isIndexBased())
         {
            time_t tEndTime;
            time(&tEndTime);
            strftime(g_staticParams.szDate, 26, "%Y/%m/%d, %I:%M:%S %p", localtime(&tEndTime));
            string strOut = " Search end:    " + string(g_staticParams.szDate)
                            + " (" + CometMassSpecUtils::ElapsedTime(tGlobalStart)
                            + ", " + CometMassSpecUtils::GetPeakMemory() + ")\n\n";
            logout(strOut);
         }
      }

      _strategy->closeFiles(fpfasta, fpidx);

      // Finalize and close writers.
      {
         bool bEmpty = (iTotalSpectraSearched == 0);
         for (auto& pw : _writers)
            pw->close(bSucceeded, bEmpty);
      }

      iTotalAllFiles += iTotalSpectraSearched;
      g_staticParams.inputFile.szBaseName[0] = '\0';

      if (!bSucceeded)
         break;
   }

   _strategy->finalize();

   // Print overall "done" banner for index-based searches.
   if (_strategy->isIndexBased())
   {
      string strOut = " - done. (" + CometMassSpecUtils::ElapsedTime(tGlobalStart);
      string strMemUse = CometMassSpecUtils::GetPeakMemory();
      if (!strMemUse.empty())
         strOut += ", " + strMemUse + ")";
      else
         strOut += ")";
      strOut += "\n\n";
      logout(strOut);
   }

   // Return false if no spectra were searched across all files (blank-file sentinel).
   if (iTotalAllFiles == 0)
      return false;

   return bSucceeded;
}
