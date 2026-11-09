/*
   Copyright 2015 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

// CometWrapper.h

#pragma once

#include "Common.h"
#include "CometDataWrapper.h"
#include "CometInterfaces.h"
#include <msclr/marshal.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;
using namespace CometInterfaces;

namespace CometWrapper {
    public ref class CometSearchManagerWrapper
    {
    public:
        CometSearchManagerWrapper();
        virtual ~CometSearchManagerWrapper();
        // Deliberately NO finalizer (!CometSearchManagerWrapper): the native
        // CometSearchManager is a process-wide singleton shared by every wrapper
        // instance (GetCometSearchManager()/ReleaseCometSearchManager() do no
        // refcounting), so releasing it from a GC finalizer destroys it at a
        // nondeterministic time -- under any other live wrapper, or even mid-native-call
        // on this one (nothing keeps the managed wrapper alive during a long native
        // call). The host must Dispose()/`using` the wrapper; an undisposed wrapper
        // leaks the singleton for the life of the process, which is the safe failure
        // mode.

        bool CreateFragmentIndex();
        bool CreatePeptideIndex();
        bool DoSearch();
        bool InitializeSingleSpectrumSearch();
        void FinalizeSingleSpectrumSearch();
        bool InitializeSingleSpectrumMS1Search(double dMaxQueryRT);
        void FinalizeSingleSpectrumMS1Search();
        bool DoSingleSpectrumSearchMultiResults(int intValue1,                   // top N results
                                                int intValue2,                   // precursor charge
                                                double value,                    // dMZ
                                                cli::array<double>^ dVal1,       // dMass
                                                cli::array<double>^ dVal2,       // dInten
                                                const int iVal1,                 // iNumPeaks
                                                [Out] List<String^>^% szPeptide,
                                                [Out] List<String^>^% szProtein,
                                                [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
                                                [Out] List<ScoreWrapper^>^% score);
        // Chimeric-spectrum search (docs/20261105_RTSchimeric.md; ICometSearchManager for the
        // contract). Explicit-list form: the candidates are three parallel arrays -- m/z, charge
        // and per-candidate +/- tolerance in m/z (0 = inherit dPrecursorToleranceMZ; the array
        // may be nullptr) -- rather than a managed candidate type, so a call allocates nothing
        // per candidate. One row per reported peptide; ScoreWrapper::Charge / Iteration say
        // where it was found. bIncomplete: the call stopped before its normal end -- true with
        // the rows found so far when max_index_runtime ran out; false with the complete earlier
        // iterations' rows when the search was cancelled or failed (CometInterfaces.h).
        bool DoSingleSpectrumSearchChimeric(int topN,
                                            cli::array<double>^ pdCandidateMZ,
                                            cli::array<int>^ piCandidateCharge,
                                            cli::array<double>^ pdCandidateToleranceMZ,
                                            double dPrecursorToleranceMZ,
                                            double dMaxExpect,
                                            int iMaxPeptidesPerSpectrum,
                                            cli::array<double>^ pdMass,
                                            cli::array<double>^ pdInten,
                                            const int iNumPeaks,
                                            [Out] List<String^>^% szPeptide,
                                            [Out] List<String^>^% szProtein,
                                            [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
                                            [Out] List<ScoreWrapper^>^% score,
                                            [Out] bool% bIncomplete);
        // Charge-range form: one m/z across [iMinPrecursorCharge, iMaxPrecursorCharge].
        bool DoSingleSpectrumSearchChimeric(int topN,
                                            double dMZ,
                                            int iMinPrecursorCharge,
                                            int iMaxPrecursorCharge,
                                            double dPrecursorToleranceMZ,
                                            double dMaxExpect,
                                            int iMaxPeptidesPerSpectrum,
                                            cli::array<double>^ pdMass,
                                            cli::array<double>^ pdInten,
                                            const int iNumPeaks,
                                            [Out] List<String^>^% szPeptide,
                                            [Out] List<String^>^% szProtein,
                                            [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
                                            [Out] List<ScoreWrapper^>^% score,
                                            [Out] bool% bIncomplete);
        // Need to convert vector to List and back
        bool DoMS1SearchMultiResults(double dVal3,
                                     double dVal4,
                                     int intValue1,
                                     double value,
                                     cli::array<double>^ dVal1,
                                     cli::array<double>^ dVal2,
                                     const int iVal1,
                                     [Out] List<ScoreWrapperMS1^>^% score);
        bool AddInputFiles(List<InputFileInfoWrapper^> ^inputFilesList);
        bool SetOutputFileBaseName(String^ baseName);
        bool SetParam(String^ name, String^ strValue, String^ value);
        bool GetParamValue(String^ name, String^% value);
        bool SetParam(String^ name, String^ strValue, int value);
        bool GetParamValue(String^ name, int %value);
        bool SetParam(String^ name, String^ strValue, double value);
        bool GetParamValue(String^ name, double% value);
        bool SetParam(String^ name, String^ strValue, IntRangeWrapper^ value);
        bool GetParamValue(String^ name, IntRangeWrapper^% value);
        bool SetParam(String^ name, String^ strValue, DoubleRangeWrapper^ value);
        bool GetParamValue(String^ name, DoubleRangeWrapper^% value);
        bool SetParam(String^ name, String^ strValue, VarModsWrapper^ value);
        bool GetParamValue(String^ name, VarModsWrapper^% value);
        bool SetParam(String^ name, String^ strValue, EnzymeInfoWrapper^ value);
        bool GetParamValue(String^ name, EnzymeInfoWrapper^% value);
        bool SetParam(String^ name, String^ strValue, List<double>^ value);
        bool GetParamValue(String^ name, List<double>^% value);
        bool ValidateCometVersion(String^ version, bool% isValid);
        bool IsSearchError(bool% bError);
        bool GetStatusMessage(String^% strStatusMsg);
        bool CancelSearch();
        bool IsCancelSearch(bool% bCancel);
        bool ResetSearchStatus();
        String^ GetPeakMemory();

        // Why the last RTS search call (DoSingleSpectrumSearchMultiResults,
        // DoSingleSpectrumSearchChimeric, DoMS1SearchMultiResults) on the calling thread returned
        // false for a reason the wrapper detected -- invalid peak arrays, or a native C++ exception
        // it caught instead of letting it escape into the host -- or nullptr when it returned
        // normally. Per thread, because RTS hosts call these concurrently.
        String^ GetLastWrapperError();

    private:
        ICometSearchManager *_pSearchMgr;
        msclr::interop::marshal_context _marshalContext;

        [System::ThreadStaticAttribute]
        static String^ s_strLastWrapperError;
    };
}
