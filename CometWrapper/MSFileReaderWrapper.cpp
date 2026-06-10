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

#pragma region Includes

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include "MSFileReaderWrapper.h"
#include <msclr/marshal_cppstd.h>
#include "CometWrapper.h"

using namespace CometWrapper;
using namespace CometWrapper;
using namespace System::Runtime::InteropServices;

#pragma endregion


MSFileReaderWrapper::MSFileReaderWrapper()
{
    // Instantiate the native C++ class
    _pMSReader = new MSReader();    
}

MSFileReaderWrapper::~MSFileReaderWrapper()
{
    if (NULL != _pMSReader)
    {
        delete _pMSReader;
        _pMSReader = NULL;
    }
}

bool MSFileReaderWrapper::ReadPeaks(String^ msFileName, int scanNum, MSSpectrumTypeWrapper msSpectrumType, List<Peak_T_Wrapper^> ^peaks)
{
    if (NULL == _pMSReader)
    {
        return false;
    }

    vector<MSSpectrumType> msLevel;
    msLevel.push_back((MSSpectrumType)msSpectrumType);
    _pMSReader->setFilter(msLevel);

    const char* pszMSFileName = _marshalContext.marshal_as<const char*>(msFileName);
    char szMSFileName[512];
    szMSFileName[0] = '\0';
    strcpy(szMSFileName, pszMSFileName);
    Spectrum spec;
    if (!_pMSReader->readFile(szMSFileName, spec, scanNum))
    {
        return false;
    }

    if (0 == spec.size())
    {
        return false;
    }

    for (int i = 0; i < spec.size(); i++)
    {
        peaks->Add(gcnew Peak_T_Wrapper(spec.at(i)));
    }

    return true;
}

bool MSFileReaderWrapper::ReadPrecursorPeaks(String^ msFileName, int fragmentScanNum, MSSpectrumTypeWrapper msFragmentSpectrumType, List<Peak_T_Wrapper^> ^precursorPeaks, int% ms1ScanNum)
{
    ms1ScanNum = 0;

    if (NULL == _pMSReader)
    {
        return false;
    }

    int scanNum = fragmentScanNum;
    if (scanNum <= 0)
    {
        return false;
    }

    int msLevelFragment = (int)msFragmentSpectrumType;
    if (msLevelFragment == 0)
    {
        return false;
    }

    const char* pszMSFileName = _marshalContext.marshal_as<const char*>(msFileName);
    char szMSFileName[512];
    szMSFileName[0] = '\0';
    strcpy(szMSFileName, pszMSFileName);

    // The MS level of the precursor is one less than that of the fragment
    vector<MSSpectrumType> msLevel;
    int msLevelPrecursor = msLevelFragment - 1;
    msLevel.push_back((MSSpectrumType)msLevelPrecursor);
    _pMSReader->setFilter(msLevel);

    // Loop and decrement the scanNum, trying each time to get the precursor 
    // peaks at that scan. When it finally succeeds, that is the closest 
    // precursor scan.
    Spectrum spec;
    scanNum--;
    while ((!_pMSReader->readFile(szMSFileName, spec, scanNum)) && (scanNum > 0))
    {
        scanNum--;

        if (fragmentScanNum - scanNum > 50)
            return false;
    }

    if (0 == spec.size())
    {
        return false;
    }

    ms1ScanNum = scanNum;

    for (int i = 0; i < spec.size(); i++)
    {
        precursorPeaks->Add(gcnew Peak_T_Wrapper(spec.at(i)));
    }

    return true;
}

