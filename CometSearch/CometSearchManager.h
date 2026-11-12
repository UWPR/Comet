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


#ifndef _COMETSEARCHMANAGER_H_
#define _COMETSEARCHMANAGER_H_

#include "CometData.h"
#include "CometDataInternal.h"
#include "CometInterfaces.h"
#include "githubsha.h"           // blank hash that github workflows should populate

#include <atomic>
#include <chrono>
#include <errno.h>
#include <string.h>
#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>
#ifdef _WIN32
#include <io.h>
#include <cstdio>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

using namespace CometInterfaces;

class CometSearchManager : public ICometSearchManager
{
public:
   CometSearchManager();
   ~CometSearchManager();

   std::map<std::string, CometParam*>& GetParamsMap();

   // Methods inherited from ICometSearchManager
   virtual bool CreateFragmentIndex();
   virtual bool CreatePeptideIndex();
   virtual bool DoSearch();
   virtual bool InitializeSingleSpectrumSearch();
   virtual void FinalizeSingleSpectrumSearch();
   virtual bool InitializeSingleSpectrumMS1Search(const double dMaxQueryRT);
   virtual void FinalizeSingleSpectrumMS1Search();
   virtual bool DoSingleSpectrumSearchMultiResults(const int topN,
                                                   int iPrecursorCharge,
                                                   double dMZ,
                                                   double* pdMass,
                                                   double* pdInten,
                                                   int iNumPeaks,
                                                   vector<string>& strReturnPeptide,
                                                   vector<string>& strReturnProtein,
                                                   vector<vector<Fragment>>& matchedFragments,
                                                   vector<CometScores>& scores);
   virtual bool DoSingleSpectrumSearchChimeric(const std::vector<PrecursorCandidate>& vPrecursors,
                                               const double dPrecursorToleranceMZ,
                                               const double dMaxExpect,
                                               const int iMaxPeptidesPerSpectrum,
                                               const int iTopN,
                                               double* pdMass,
                                               double* pdInten,
                                               const int iNumPeaks,
                                               vector<string>& strReturnPeptide,
                                               vector<string>& strReturnProtein,
                                               vector<vector<Fragment>>& matchedFragments,
                                               vector<CometScores>& scores,
                                               bool& bIncomplete);
   virtual bool DoSingleSpectrumSearchChimeric(const double dMZ,
                                               const int iMinPrecursorCharge,
                                               const int iMaxPrecursorCharge,
                                               const double dPrecursorToleranceMZ,
                                               const double dMaxExpect,
                                               const int iMaxPeptidesPerSpectrum,
                                               const int iTopN,
                                               double* pdMass,
                                               double* pdInten,
                                               const int iNumPeaks,
                                               vector<string>& strReturnPeptide,
                                               vector<string>& strReturnProtein,
                                               vector<vector<Fragment>>& matchedFragments,
                                               vector<CometScores>& scores,
                                               bool& bIncomplete);
   virtual bool DoMS1SearchMultiResults(const double dMaxMS1RTDiff,
                                        const double dMaxQueryRT,
                                        const int topN,
                                        const double dRT,
                                        double* pdMass,
                                        double* pdInten,
                                        int iNumPeaks,
                                        vector<CometScoresMS1>& scores);
   virtual void AddInputFiles(vector<InputFileInfo*> &pvInputFiles);
   virtual void SetOutputFileBaseName(const char *pszBaseName);
   virtual void SetParam(const string &name, const string &strValue, const string &value);
   virtual bool GetParamValue(const string &name, string &value);
   virtual void SetParam(const string &name, const string &strValue, const bool &value);
   virtual bool GetParamValue(const string &name, bool &value);
   virtual void SetParam(const string &name, const string &strValue, const int &value);
   virtual bool GetParamValue(const string &name, int &value);
   virtual void SetParam(const string &name, const string &strValue, const long &value);
   virtual bool GetParamValue(const string &name, long &value);
   virtual void SetParam(const string &name, const string &strValue, const double &value);
   virtual bool GetParamValue(const string &name, double &value);
   virtual void SetParam(const string &name, const string &strValue, const VarMods &value);
   virtual bool GetParamValue(const string &name, VarMods &value);
   virtual void SetParam(const string &name, const string &strValue, const DoubleRange &value);
   virtual bool GetParamValue(const string &name, DoubleRange &value);
   virtual void SetParam(const string &name, const string &strValue, const IntRange &value);
   virtual bool GetParamValue(const string &name, IntRange &value);
   virtual void SetParam(const string &name, const string &strValue, const EnzymeInfo &value);
   virtual bool GetParamValue(const string &name, EnzymeInfo &value);
   virtual void SetParam(const string &name, const string &strValue, const vector<double> &value);
   virtual bool GetParamValue(const string &name, vector<double> &value);
   virtual bool IsValidCometVersion(const string &version);
   virtual bool IsSearchError();
   virtual void GetStatusMessage(string &strStatusMsg);
   virtual void CancelSearch();
   virtual bool IsCancelSearch();
   virtual void ResetSearchStatus();
   virtual void SetAScoreOptions(AScoreProCpp::AScoreOptions& g_vAScoreOptions);
   virtual void PrintAScoreOptions(const AScoreProCpp::AScoreOptions& g_vAScoreOptions);


private:
   bool InitializeStaticParams();
   bool ReadProteinVarModFilterFile();

   // One (spectrum, precursor m/z, charge) RTS search: preprocess, index search,
   // post-analysis and result extraction. The body of DoSingleSpectrumSearchMultiResults(),
   // shared with the chimeric multi-precursor search (docs/20261105_RTSchimeric.md).
   // Appends up to takeN rows to the four output vectors; returns the number appended,
   // -1 when preprocessing produced no Query (input rejected or too few peaks), -2 when
   // the index search failed or was cancelled. dToleranceMZ > 0 overrides the configured
   // precursor tolerance for this search; tStart is the
   // max_index_runtime origin (the caller's call entry). pvRemovalBins, when given, receives the sorted, unique fragment-ion bins
   // of the rank-1 PSM for the chimeric loop to clear (section 4.5 of the plan).
   int SearchSingleSpectrumCore(const int takeN,
                                const int iPrecursorCharge,
                                const double dMZ,
                                const double dToleranceMZ,
                                double* pdMass,
                                double* pdInten,
                                const int iNumPeaks,
                                const std::chrono::high_resolution_clock::time_point tStart,
                                vector<string>& strReturnPeptide,
                                vector<string>& strReturnProtein,
                                vector<vector<Fragment>>& matchedFragments,
                                vector<CometScores>& scores,
                                std::vector<int>* pvRemovalBins = nullptr);
   // The iteration loop behind both DoSingleSpectrumSearchChimeric() (validated candidate list) and
   // DoSingleSpectrumSearchMultiResults() (one candidate, one iteration, dMaxExpect +inf). Returns the
   // chimeric return value; bAllRejected: iteration 0 searched nothing because every candidate was
   // rejected (the plain search returns false for that, the chimeric one true).
   bool RunChimericLoop(const PrecursorCandidate* pCand,
                        const size_t uiNumCand,
                        const double dMaxExpect,
                        const int iMaxPeptidesPerSpectrum,
                        const int iTopN,
                        double* pdMass,
                        double* pdInten,
                        const int iNumPeaks,
                        const std::chrono::high_resolution_clock::time_point tStart,
                        vector<string>& strReturnPeptide,
                        vector<string>& strReturnProtein,
                        vector<vector<Fragment>>& matchedFragments,
                        vector<CometScores>& scores,
                        bool& bIncomplete,
                        bool& bAllRejected);
   std::atomic<bool> singleSearchInitializationComplete;
   std::atomic<bool> singleSearchMS1InitializationComplete;
   bool staticParamsInitializationComplete;
   bool m_bRTSIndexBuild;    // true when DoSearch() is called via CreateFragmentIndex/CreatePeptideIndex
                              // from InitializeSingleSpectrumSearch(); suppresses _exit(0) in DoSearch()
   std::map<std::string, CometParam*> _mapStaticParams;
};

#endif
