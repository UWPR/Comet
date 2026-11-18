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

#pragma once

#include "Common.h"
#include <string.h>
#include "CometDataInternal.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometPreprocess.h"
#include "MSReader.h"
#include "SearchSession.h"

// Shared utilities used by Pipeline and strategy classes.

bool UpdateInputFile(InputFileInfo* pFileInfo);
void SetMSLevelFilter(MSReader& mstReader);
bool AllocateResultsMem(std::vector<Query*>& queries);
bool RunSearchAndPostAnalysis(int iPercentStart, int iPercentEnd,
                              ThreadPool* tp, SearchSession& session,
                              bool bLogPrePostAnalysis = false);

// Legacy three-sweep batch body: LoadAndPreprocess -> AllocateResults ->
// RunSearchAndPostAnalysis.  Used by FiStrategy (non-fused fallback),
// FastaStrategy, and PiStrategy.  Pass bVerbose=true for FASTA-path
// console progress output.
bool executeBatchLegacy(MSToolkit::MSReader& mstReader,
                        int iFirstScan, int iLastScan, int iAnalysisType,
                        int& iPercentStart, int& iPercentEnd,
                        ThreadPool* tp, SearchSession& session,
                        bool bVerbose);

// -----------------------------------------------------------------------
// Query sort comparators -- kept inline; single-expression each.
// -----------------------------------------------------------------------
inline bool compareByPeptideMass(Query const* a, Query const* b)
{
   return (a->_pepMassInfo.dExpPepMass < b->_pepMassInfo.dExpPepMass);
}

// Scan, then charge, then the Mango A/B pair label. The label is the last tie-break so
// the two members of a Mango pair (same scan, usually the same charge) come out in a fixed
// order; without it their order depended on thread completion and the earlier mass sort,
// so output was not reproducible run to run. szMango is "" for every Query when
// mango_search = 0, so non-Mango ordering is unchanged.
inline bool compareByScanNumber(Query const* a, Query const* b)
{
   if (a->_spectrumInfoInternal.iScanNumber == b->_spectrumInfoInternal.iScanNumber)
   {
      if (a->_spectrumInfoInternal.usiChargeState == b->_spectrumInfoInternal.usiChargeState)
         return (strcmp(a->_spectrumInfoInternal.szMango, b->_spectrumInfoInternal.szMango) < 0);
      return (a->_spectrumInfoInternal.usiChargeState < b->_spectrumInfoInternal.usiChargeState);
   }
   return (a->_spectrumInfoInternal.iScanNumber < b->_spectrumInfoInternal.iScanNumber);
}
