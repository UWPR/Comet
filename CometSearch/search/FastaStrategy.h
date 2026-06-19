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

#include "ISearchStrategy.h"

// Search strategy for FASTA_DB (classic three-sweep) batch searches.
//
// initialize(): reads the protein variable-mod filter file if configured;
//               allocates search and preprocess memory pools.
// executeBatch(): LoadAndPreprocessSpectra -> RunSearch -> PostAnalysis.
// finalize(): frees memory pools.
class FastaStrategy : public ISearchStrategy
{
public:
   bool initialize(SearchSession& session, ThreadPool* tp) override;
   bool openFiles(const std::string& szDatabase,
                  FILE*& fpfasta, FILE*& fpidx, FILE*& fpdb,
                  SearchSession& session) override;
   bool executeBatch(MSToolkit::MSReader& mstReader,
                     int iFirstScan, int iLastScan, int iAnalysisType,
                     int& iPercentStart, int& iPercentEnd,
                     ThreadPool* tp, SearchSession& session) override;
   void closeFiles(FILE* fpfasta, FILE* fpidx) override;
   void finalize() override;
   bool isIndexBased() const override { return false; }
};
