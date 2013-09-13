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
		
        void DoSearch();

    private:
        CometSearchManager *_pSearchMgr;
	};
}
