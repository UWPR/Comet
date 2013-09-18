// CometWrapper.h

#pragma once

#include "Common.h"
#include "CometSearchManager.h"

using namespace System;

namespace CometWrapper {

    public ref class CometSearchManagerWrapper
	{
    public:
        CometSearchManagerWrapper();
        virtual ~CometSearchManagerWrapper();
		
        bool DoSearch();
        bool SetParam(System::String^ name, System::String^ strValue, System::String^ value);

    private:
        CometSearchManager *_pSearchMgr;
	};
}
