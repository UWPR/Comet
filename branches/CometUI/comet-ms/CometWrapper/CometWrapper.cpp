// This is the main DLL file.

#pragma region Includes

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <msclr/marshal_cppstd.h>

#include "CometWrapper.h"
using namespace CometWrapper;

#include <msclr/marshal.h>
using namespace msclr::interop;

#pragma endregion


CometSearchManagerWrapper::CometSearchManagerWrapper()
{
    // Instantiate the native C++ class CSimpleObject.
    _pSearchMgr = new CometSearchManager();
}

CometSearchManagerWrapper::~CometSearchManagerWrapper()
{
    if (_pSearchMgr)
    {
        delete _pSearchMgr;
        _pSearchMgr = NULL;
    }
}

bool CometSearchManagerWrapper::DoSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    _pSearchMgr->DoSearch();
    
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
