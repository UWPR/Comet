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
		// TODO: Add your methods for this class here.

    private:
        CometSearchManager *_pSearchMgr;
	};
}
