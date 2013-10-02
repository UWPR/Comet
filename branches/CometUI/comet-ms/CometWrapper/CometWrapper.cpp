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
}

CometSearchManagerWrapper::~CometSearchManagerWrapper()
{
    ReleaseCometSearchManager();
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