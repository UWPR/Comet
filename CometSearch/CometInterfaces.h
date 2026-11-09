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

      // Chimeric-spectrum search (docs/20261105_RTSchimeric.md): search the spectrum once
      // per precursor candidate, keep the best PSM (lowest E-value, ties by xcorr), and if
      // its E-value is <= dMaxExpect report it, remove the fragment peaks it explains and
      // repeat on the reduced spectrum, up to iMaxPeptidesPerSpectrum rows. One row per
      // reported peptide (its iteration's rank 1, with that iteration's delta-Cn); the
      // charge it was found at and its iteration are in CometScores::iCharge / iIteration.
      // dPrecursorToleranceMZ > 0 replaces the configured precursor tolerance (+/- m/z) for
      // every candidate whose own dToleranceMZ is 0; 0 = peptide_mass_tolerance as configured.
      // Return value and bIncomplete (2026-11-08):
      //   true,  bIncomplete false: a normal end -- no PSM left, the best failed dMaxExpect,
      //                             iMaxPeptidesPerSpectrum rows, or too few peaks remain
      //                             (zero rows included);
      //   true,  bIncomplete true:  the max_index_runtime budget ran out; the rows are the
      //                             PSMs found so far (the spectrum may hold more);
      //   false, bIncomplete true:  the call stopped early because the search was cancelled
      //                             (IsCancelSearch()) or an index search failed; the rows are
      //                             the complete iterations before that and are valid;
      //   false, bIncomplete false: invalid arguments; no rows.
      // Rows are appended to the four output vectors, one complete row per kept iteration.
      virtual bool DoSingleSpectrumSearchChimeric(const int topN,
                                                  const std::vector<PrecursorCandidate>& vPrecursors,
                                                  const double dPrecursorToleranceMZ,
                                                  const double dMaxExpect,
                                                  const int iMaxPeptidesPerSpectrum,
                                                  double* dMass,
                                                  double* dInten,
                                                  const int iNumPeaks,
                                                  vector<string>& strReturnPeptide,
                                                  vector<string>& strReturnProtein,
                                                  vector<vector<Fragment>>& matchedFragments,
                                                  vector<CometScores>& scores,
                                                  bool& bIncomplete) = 0;
      // Same, with one precursor m/z expanded across [iMinPrecursorCharge, iMaxPrecursorCharge].
      virtual bool DoSingleSpectrumSearchChimeric(const int topN,
                                                  const double dMZ,
                                                  const int iMinPrecursorCharge,
                                                  const int iMaxPrecursorCharge,
                                                  const double dPrecursorToleranceMZ,
                                                  const double dMaxExpect,
                                                  const int iMaxPeptidesPerSpectrum,
                                                  double* dMass,
                                                  double* dInten,
                                                  const int iNumPeaks,
                                                  vector<string>& strReturnPeptide,
                                                  vector<string>& strReturnProtein,
                                                  vector<vector<Fragment>>& matchedFragments,
                                                  vector<CometScores>& scores,
                                                  bool& bIncomplete) = 0;
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
