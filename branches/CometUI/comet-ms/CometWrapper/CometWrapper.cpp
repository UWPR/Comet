// This is the main DLL file.

#include "stdafx.h"

#include "CometWrapper.h"

#pragma region Includes
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

void CometSearchManagerWrapper::DoSearch()
{
    if (_pSearchMgr)
    {
        _pSearchMgr->DoSearch();
    }
}
