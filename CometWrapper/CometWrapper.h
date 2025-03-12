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

        bool CreateFragmentIndex();
        bool CreatePeptideIndex();
        bool DoSearch();
        bool InitializeSingleSpectrumSearch();
        void FinalizeSingleSpectrumSearch();
        bool DoSingleSpectrumSearchMultiResults(int intValue1,
            int intValue2,
            double value,
            cli::array<double>^ dVal1,
            cli::array<double>^ dVal2,
            const int iVal1,
            [Out] List<String^>^% szPeptide,
            [Out] List<String^>^% szProtein,
            [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
            [Out] List<ScoreWrapper^>^% score);
        // Need to convert vector to List and back
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

    private:
        ICometSearchManager *_pSearchMgr;
        msclr::interop::marshal_context _marshalContext;
        vector<InputFileInfo*>* _pvInputFilesList;
    };
}
