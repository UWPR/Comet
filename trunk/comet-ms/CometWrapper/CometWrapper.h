// CometWrapper.h

#pragma once

#include "Common.h"
#include "CometDataWrapper.h"
#include "CometInterfaces.h"
#include <msclr/marshal.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;
using namespace CometInterfaces;

namespace CometWrapper {
    public ref class CometSearchManagerWrapper
	{
    public:
        CometSearchManagerWrapper();
        virtual ~CometSearchManagerWrapper();
		
        bool DoSearch();
        // Need to convert vector to List and back
        bool AddInputFiles(List<InputFileInfoWrapper^> ^inputFilesList);
        bool SetOutputFileBaseName(String^ baseName);
        bool SetParam(String^ name, String^ strValue, String^ value);
        bool GetParamValue(String^ name, String^% value);
        bool SetParam(String^ name, String^ strValue, int value);
        bool GetParamValue(String^ name, int %value);
        bool SetParam(String^ name, String^ strValue, double value);
        bool GetParamValue(String^ name, double% value);
        bool SetParam(String^ name, String^ strValue, IntRangeWrapper^ value);
        bool GetParamValue(String^ name, IntRangeWrapper^% value);
        bool SetParam(String^ name, String^ strValue, DoubleRangeWrapper^ value);
        bool GetParamValue(String^ name, DoubleRangeWrapper^% value);
        bool SetParam(String^ name, String^ strValue, VarModsWrapper^ value);
        bool GetParamValue(String^ name, VarModsWrapper^% value);
        bool SetParam(String^ name, String^ strValue, EnzymeInfoWrapper^ value);
        bool GetParamValue(String^ name, EnzymeInfoWrapper^% value);
        bool GetErrorMessage(System::String^% strErrorMsg);

    private:
        ICometSearchManager *_pSearchMgr;
        msclr::interop::marshal_context _marshalContext;
        vector<InputFileInfo*>* _pvInputFilesList;
	};
}
