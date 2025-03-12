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

// This is the main DLL file.

#pragma region Includes

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <msclr/marshal_cppstd.h>

#include "CometWrapper.h"
using namespace CometWrapper;

using namespace System::Runtime::InteropServices;

#pragma endregion


CometSearchManagerWrapper::CometSearchManagerWrapper()
{
    // Instantiate the native C++ class
    _pSearchMgr = GetCometSearchManager();

    _pvInputFilesList = new vector<InputFileInfo*>();
}

CometSearchManagerWrapper::~CometSearchManagerWrapper()
{
    ReleaseCometSearchManager();

    // CometSearchManager releases all the objects stored in the vector, we just
    // need to release the vector itself here.
    if (NULL != _pvInputFilesList)
    {
        delete _pvInputFilesList;
        _pvInputFilesList = NULL;
    }
}

bool CometSearchManagerWrapper::CreatePeptideIndex()
{
   if (!_pSearchMgr)
   {
      return false;
   }
   return _pSearchMgr->CreatePeptideIndex();
}

bool CometSearchManagerWrapper::CreateFragmentIndex()
{
   if (!_pSearchMgr)
   {
      return false;
   }
   return _pSearchMgr->CreateFragmentIndex();
}

bool CometSearchManagerWrapper::InitializeSingleSpectrumSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }
    return _pSearchMgr->InitializeSingleSpectrumSearch();
}

void CometSearchManagerWrapper::FinalizeSingleSpectrumSearch()
{
    if (_pSearchMgr)
    {
        _pSearchMgr->FinalizeSingleSpectrumSearch();
    }
}

bool CometSearchManagerWrapper::DoSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    return _pSearchMgr->DoSearch();
}


bool CometSearchManagerWrapper::DoSingleSpectrumSearchMultiResults(int topN,
    int iPrecursorCharge,
    double dMZ,
    cli::array<double>^ pdMass,
    cli::array<double>^ pdInten,
    int iNumPeaks,
    [Out] List<String^>^% szPeptide,
    [Out] List<String^>^% szProtein,
    [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
    [Out] List<ScoreWrapper^>^% score)
{
    if (!_pSearchMgr)
    {
        return false;
    }
    pin_ptr<double> ptrMasses = &pdMass[0];
    pin_ptr<double> ptrInten = &pdInten[0];
    vector<std::string> stdStringszPeptide;
    vector<std::string> stdStringszProtein;

    vector<Scores> scores;
    vector<vector<Fragment>> matchedFragments;

    // perform the search
    bool isSuccess = _pSearchMgr->DoSingleSpectrumSearchMultiResults(topN, iPrecursorCharge, dMZ, ptrMasses, ptrInten, iNumPeaks,
        stdStringszPeptide, stdStringszProtein, matchedFragments, scores);

    szPeptide = gcnew List<String^>();
    for (auto eachszPeptide : stdStringszPeptide)
    {
        // Convert data back to the managed world
        szPeptide->Add(gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char*>(eachszPeptide.c_str())))));
    }

    szProtein = gcnew List<String^>();
    for (auto eachszProtein : stdStringszProtein)
    {
        // Convert data back to the managed world
        szProtein->Add(gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char*>(eachszProtein.c_str())))));
    }

    score = gcnew List<ScoreWrapper^>();
    for (auto eachScore : scores)
    {
        score->Add(gcnew ScoreWrapper(eachScore));
    }

    matchingFragments = gcnew List<List<FragmentWrapper^>^>();
    for (auto eachMatchedFragSet : matchedFragments)
    {
        auto eachMatchedFragments = gcnew List<FragmentWrapper^>();
        for (auto frag : eachMatchedFragSet)
        {
            eachMatchedFragments->Add(gcnew FragmentWrapper(frag));
        }
        matchingFragments->Add(eachMatchedFragments);
    }

    return isSuccess;
}

bool CometSearchManagerWrapper::AddInputFiles(List<InputFileInfoWrapper^> ^inputFilesList)
{
    if (!_pSearchMgr)
    {
        return false;
    }
    
    int numFiles = inputFilesList->Count;
    for (int i = 0; i < numFiles; i++)
    {
        InputFileInfoWrapper^ inputFile = inputFilesList[i];
        _pvInputFilesList->push_back(inputFile->get_InputFileInfoPtr());
    }

    _pSearchMgr->AddInputFiles(*_pvInputFilesList);

    return true;
}

bool CometSearchManagerWrapper::SetOutputFileBaseName(System::String^ baseName)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    const char* pszBaseName = _marshalContext.marshal_as<const char*>(baseName);
    _pSearchMgr->SetOutputFileBaseName(pszBaseName);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, System::String^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }
    
    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue); 
    std::string stdStringValue = marshal_as<std::string>(value);  

    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, stdStringValue);
    
    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, System::String^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    std::string stdStringValue;
    if (!_pSearchMgr->GetParamValue(stdStringName, stdStringValue))
    {
        return false;
    }
    
    value = gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(stdStringValue.c_str())))); 

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, int value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue); 
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, value);
    
    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, int %value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    int iValue;
    if (!_pSearchMgr->GetParamValue(stdStringName, iValue))
    {
        return false;
    }
    

    value = iValue; 

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, double value)
{
    if (!_pSearchMgr)
    {
        return false;
    }
    
    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue); 
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, value);
    
    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, double% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    double dValue;
    if (!_pSearchMgr->GetParamValue(stdStringName, dValue))
    {
        return false;
    }

    value = dValue; 

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, IntRangeWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    IntRange *pIntRange = value->get_IntRangePtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pIntRange);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, IntRangeWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    IntRange intRangeParam(0, 0);
    if (!_pSearchMgr->GetParamValue(stdStringName, intRangeParam))
    {
        return false;
    }

    value = gcnew IntRangeWrapper(intRangeParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, DoubleRangeWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    DoubleRange *pDoubleRange = value->get_DoubleRangePtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pDoubleRange);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, DoubleRangeWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    DoubleRange doubleRangeParam(0.0, 0.0);
    if (!_pSearchMgr->GetParamValue(stdStringName, doubleRangeParam))
    {
        return false;
    }

    value = gcnew DoubleRangeWrapper(doubleRangeParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, VarModsWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    VarMods *pVarMods = value->get_VarModsPtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pVarMods);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, VarModsWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    VarMods varModsParam;
    if (!_pSearchMgr->GetParamValue(stdStringName, varModsParam))
    {
        return false;
    }

    value = gcnew VarModsWrapper(varModsParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, EnzymeInfoWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    EnzymeInfo *pEnzymInfo = value->get_EnzymeInfoPtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pEnzymInfo);

    return true;
}


bool CometSearchManagerWrapper::GetParamValue(System::String^ name, EnzymeInfoWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    EnzymeInfo enzymeInfoParam;
    if (!_pSearchMgr->GetParamValue(stdStringName, enzymeInfoParam))
    {
        return false;
    }

    value = gcnew EnzymeInfoWrapper(enzymeInfoParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(String^ name, String^ strValue, List<double>^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    
    vector<double> vectorMassOffsets;
    int numItems = value->Count;
    for (int i = 0; i < numItems; i++)
    {
        vectorMassOffsets.push_back(value[i]);
    }
    sort(vectorMassOffsets.begin(), vectorMassOffsets.end());

    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, vectorMassOffsets);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(String^ name, List<double>^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    vector<double> vectorMassOffsets;
    if (!_pSearchMgr->GetParamValue(stdStringName, vectorMassOffsets))
    {
        return false;
    }

    int numItems = (int)vectorMassOffsets.size();
    for (int i = 0; i < numItems; i++)
    {
        value->Add(vectorMassOffsets[i]);
    }

    return true;
}

bool CometSearchManagerWrapper::ValidateCometVersion(String^ version, bool% isValid)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdVersion = marshal_as<std::string>(version);
    isValid = _pSearchMgr->IsValidCometVersion(stdVersion);
    return true;
}

bool CometSearchManagerWrapper::IsSearchError(bool% bError)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    bError = _pSearchMgr->IsSearchError();
    return true;
}

bool CometSearchManagerWrapper::GetStatusMessage(System::String^% strStatusMsg)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStrStatusMsg;
    _pSearchMgr->GetStatusMessage(stdStrStatusMsg);
    strStatusMsg = gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(stdStrStatusMsg.c_str())))); 

    return true;
}

bool CometSearchManagerWrapper::CancelSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    _pSearchMgr->CancelSearch();
    return true;
}

bool CometSearchManagerWrapper::IsCancelSearch(bool% bCancel)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    bCancel = _pSearchMgr->IsCancelSearch();
    return true;
}

bool CometSearchManagerWrapper::ResetSearchStatus()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    _pSearchMgr->ResetSearchStatus();
    return true;
}
