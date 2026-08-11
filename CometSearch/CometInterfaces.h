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


#ifndef _COMETINTERFACES_H_
#define _COMETINTERFACES_H_

#include "Common.h"
#include "CometData.h"
#include "Threading.h"
#include "ThreadPool.h"

using namespace std;

namespace CometInterfaces
{
   class ICometSearchManager
   {
public:
      virtual ~ICometSearchManager() {}
      virtual bool CreateFragmentIndex() = 0;
      virtual bool CreatePeptideIndex() = 0;
      // Reads an existing PI_DB .idx (built earlier via CreatePeptideIndex()/-j) plus whatever
      // variable mods are live in comet.params right now, and dumps the canonical per-variant
      // (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, sequence, mod sites) enumeration to
      // strOutputFile -- docs/20260805_carafe.md Section 6.9/9: since Phase 0.5 stopped
      // persisting MOD_NUMBERS/MOD_SEQS/the variant array in the .idx file, this is the only
      // way for an external tool (tools/idx_to_carafe.py) to learn that enumeration without
      // re-implementing CometPeptideIndex::EnumerateIndexPeptideMods()'s combinatorics itself.
      // Forces PI_DB mode regardless of index_search_type (this export has no FI_DB analog).
      virtual bool ExportPeptideIndexVariants(const string &strOutputFile) = 0;
      virtual bool DoSearch() = 0;
      virtual bool InitializeSingleSpectrumSearch() = 0;
      virtual void FinalizeSingleSpectrumSearch() = 0;
      virtual bool InitializeSingleSpectrumMS1Search(const double dMaxQueryRT) = 0;
      virtual void FinalizeSingleSpectrumMS1Search() = 0;
      virtual bool DoSingleSpectrumSearchMultiResults(const int topN,
                                                      const int iPrecursorCharge,
                                                      const double dMZ,
                                                      double* dMass,
                                                      double* dInten,
                                                      const int iNumPeaks,
                                                      vector<string>& strReturnPeptide,
                                                      vector<string>& strReturnProtein,
                                                      vector<vector<Fragment>>& matchedFragments,
                                                      vector<CometScores>& scores) = 0;
      virtual bool DoMS1SearchMultiResults(const double dMaxMS1RTDiff,
                                           const double dMaxQueryRT,
                                           const int topN,
                                           const double dRT,
                                           double* dMass,
                                           double* dInten,
                                           const int iNumPeaks,
                                           vector<CometScoresMS1>& scores) = 0;
      virtual void AddInputFiles(vector<InputFileInfo*> &pvInputFiles) = 0;
      virtual void SetOutputFileBaseName(const char *pszBaseName) = 0;
      virtual void SetParam(const string &name, const string &strValue, const string &value) = 0;
      virtual bool GetParamValue(const string &name, string &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const int &value) = 0;
      virtual bool GetParamValue(const string &name, int &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const long &value) = 0;
      virtual bool GetParamValue(const string &name, long &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const double &value) = 0;
      virtual bool GetParamValue(const string &name, double &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const VarMods &value) = 0;
      virtual bool GetParamValue(const string &name, VarMods &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const DoubleRange &value) = 0;
      virtual bool GetParamValue(const string &name, DoubleRange &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const IntRange &value) = 0;
      virtual bool GetParamValue(const string &name, IntRange &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const EnzymeInfo &value) = 0;
      virtual bool GetParamValue(const string &name, EnzymeInfo &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const vector<double> &value) = 0;
      virtual bool GetParamValue(const string &name, vector<double> &value) = 0;
      virtual bool IsValidCometVersion(const string &version) = 0;
      virtual bool IsSearchError() = 0;
      virtual void GetStatusMessage(string &strStatusMsg) = 0;
      virtual void CancelSearch() = 0;
      virtual bool IsCancelSearch() = 0;
      virtual void ResetSearchStatus() = 0;
   };

   ICometSearchManager *GetCometSearchManager();
   void ReleaseCometSearchManager();

   [[maybe_unused]] static ThreadPool* _tp;
}

#endif // _COMETINTERFACES_H_
