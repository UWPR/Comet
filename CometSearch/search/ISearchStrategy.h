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

#pragma once

#include "SearchSession.h"
#include "ThreadPool.h"
#include <cstdio>
#include <string>

namespace MSToolkit { class MSReader; }

// Abstract search strategy. One concrete implementation per database type:
//   FiStrategy  -- FI_DB (fragment ion index, fused + fallback legacy path)
//   FastaStrategy -- FASTA_DB (classic three-sweep path)
//   PiStrategy  -- PI_DB (plain peptide index)
//
// Pipeline selects the correct one at startup and holds it for the entire run.
class ISearchStrategy
{
public:
   virtual ~ISearchStrategy() = default;

   // Called once before the per-file loop.
   // Allocates search/preprocess memory pools, loads/builds the index,
   // pre-reads precursors (FI_DB), reads var-mod filter file (FASTA_DB).
   // Returns false on error.
   virtual bool initialize(SearchSession& session, ThreadPool* tp) = 0;

   // Called once per input file.
   // Opens database file handles (fpfasta, fpidx) and sets fpdb to whichever
   // handle writers use for sequence retrieval.
   // Sets session.bIdxNoFasta = true when an .idx search has no companion .fasta.
   // Returns false on error.
   virtual bool openFiles(const std::string& szDatabase,
                          FILE*& fpfasta, FILE*& fpidx, FILE*& fpdb,
                          SearchSession& session) = 0;

   // Called once per batch within a file.
   // Fills session.queries with fully scored Query* results (preprocess + search
   // + post-analysis, all done here). May return with session.queries empty
   // if no spectra passed the filters in this batch.
   // Updates iPercentStart/iPercentEnd after loading (before RunSearch) so that
   // RunSearch receives the file-position range for this batch.
   // Returns false on error or cancel.
   virtual bool executeBatch(MSToolkit::MSReader& mstReader,
                             int iFirstScan, int iLastScan, int iAnalysisType,
                             int& iPercentStart, int& iPercentEnd,
                             ThreadPool* tp, SearchSession& session) = 0;

   // Called once per input file after all batches.
   // Closes the file handles opened by openFiles().
   virtual void closeFiles(FILE* fpfasta, FILE* fpidx) = 0;

   // Called once after all files.
   // Frees memory pools and (for FI_DB) the fragment index arrays.
   virtual void finalize() = 0;

   // Returns true for index-based searches (FI_DB, PI_DB).
   // Pipeline uses this to select progress-message style.
   virtual bool isIndexBased() const = 0;
};
