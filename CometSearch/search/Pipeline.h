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
#include "../output/IResultWriter.h"
#include "../CometSearchManager.h"
#include <memory>
#include <vector>

// Pipeline drives the batch search for all input files.
// It owns the strategy (which provides the per-batch search implementation)
// and the result writers (which serialize results to disk).
//
// Typical call sequence from CometSearchManager::DoSearch():
//   Pipeline pipeline(std::move(strategy), std::move(writers), pMgr);
//   pipeline.run(session, g_pvInputFiles, *tp);
class Pipeline
{
public:
   Pipeline(std::unique_ptr<ISearchStrategy>            strategy,
            std::vector<std::unique_ptr<IResultWriter>> writers,
            CometSearchManager*                         pMgr);

   // Drives initialize -> per-file loop (open, batch-loop, close) -> finalize.
   // Returns false if any file fails or no spectra are found across all files.
   bool run(SearchSession&                      session,
            const std::vector<InputFileInfo*>&  files,
            ThreadPool&                         tp);

private:
   std::unique_ptr<ISearchStrategy>            _strategy;
   std::vector<std::unique_ptr<IResultWriter>> _writers;
   CometSearchManager*                         _pMgr;
};
