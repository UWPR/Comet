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

bool MSFileReaderWrapper::ReadPrecursorPeaks(String^ msFileName, int fragmentScanNum, List<Peak_T_Wrapper^> ^precursorPeaks)
{
    if (NULL == _pMSReader)
    {
        return false;
    }

    const char* pszMSFileName = _marshalContext.marshal_as<const char*>(msFileName);
    char szMSFileName[512];
    szMSFileName[0] = '\0';
    strcpy(szMSFileName, pszMSFileName);

    RAMPFILE *fpRampFile = NULL;
    fpRampFile = rampOpenFile(szMSFileName);
    if (NULL == fpRampFile)
    {
        return false;
    }

    ramp_fileoffset_t indexOffset;
    indexOffset = getIndexOffset(fpRampFile);

    int rampLastScan;
    ramp_fileoffset_t  *pScanIndex = readIndex(fpRampFile, indexOffset, &rampLastScan);

    if (fragmentScanNum > rampLastScan || fragmentScanNum <= 0)
    {
        return false;
    }

    int iScanNum = fragmentScanNum;
    struct ScanHeaderStruct scanHeader;
    readHeader(fpRampFile, pScanIndex[iScanNum], &scanHeader);
    iScanNum--;

    // loop back through scans to find MS1 scan; break if greater than 60 seconds away
    struct ScanHeaderStruct scanHeaderMS;
    while (iScanNum > 0)
    {
        readHeader(fpRampFile, pScanIndex[iScanNum], &scanHeaderMS);
        if ((scanHeaderMS.msLevel == (scanHeader.msLevel - 1)) || 
            (fabs(scanHeaderMS.retentionTime - scanHeader.retentionTime) > 60.0))
        {
            break;
        }
        iScanNum--;
    }

    if (scanHeaderMS.msLevel != (scanHeader.msLevel - 1))
    {
        return false;
    }

    RAMPREAL *pPeaks;
    int n = 0;
    pPeaks = readPeaks(fpRampFile, pScanIndex[iScanNum]);
    while (pPeaks != NULL && pPeaks[n] != -1)
    {
        RAMPREAL fMass;
        RAMPREAL fInten;

        fMass = pPeaks[n];
        n++;
        fInten = pPeaks[n];
        n++;

        Peak_T peak;
        peak.mz = fMass;
        peak.intensity = fInten;
        precursorPeaks->Add(gcnew Peak_T_Wrapper(peak));
    }

    return true;
}
