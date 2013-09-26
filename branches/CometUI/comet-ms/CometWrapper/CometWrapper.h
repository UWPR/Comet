// CometWrapper.h

#pragma once

#include "Common.h"
#include "CometInterfaces.h"
#include <msclr/marshal.h>

using namespace System;
using namespace msclr::interop;
using namespace CometInterfaces;

namespace CometWrapper {
    public ref class CometSearchManagerWrapper
	{
    public:
        CometSearchManagerWrapper();
        virtual ~CometSearchManagerWrapper();
		
        bool DoSearch();
        bool SetOutputFileBaseName(System::String^ baseName);
        bool SetParam(System::String^ name, System::String^ strValue, System::String^ value);
        bool GetParamValue(System::String^ name, System::String^% value);
        bool SetParam(System::String^ name, System::String^ strValue, int value);
        bool GetParamValue(System::String^ name, int %value);
        bool SetParam(System::String^ name, System::String^ strValue, double value);
        bool GetParamValue(System::String^ name, double% value);

    private:
        ICometSearchManager *_pSearchMgr;
        msclr::interop::marshal_context _marshalContext;
	};
}
