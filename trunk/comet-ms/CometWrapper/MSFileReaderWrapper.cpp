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

