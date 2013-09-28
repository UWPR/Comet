// CometWrapper.h

#pragma once

#include "CometData.h"

using namespace System;

namespace CometWrapper {

    public ref class IntRangeWrapper
    {
    public:
        IntRangeWrapper() { _pIntRange = new IntRange(); }
        IntRangeWrapper(IntRange &intRangeParam) { _pIntRange = new IntRange(intRangeParam.iStart, intRangeParam.iEnd); }
        IntRangeWrapper(int iStart, int iEnd) { _pIntRange = new IntRange(iStart, iEnd); }
        virtual ~IntRangeWrapper() 
        { 
            if (NULL != _pIntRange)
            {
                delete _pIntRange;
                _pIntRange = NULL;
            }
        }
        
        int get_iStart() {return _pIntRange->iStart;}
        void set_iStart(int iStart) {_pIntRange->iStart = iStart;}

        int get_iEnd() {return _pIntRange->iEnd;}
        void set_iEnd(int iEnd) {_pIntRange->iEnd = iEnd;}

        IntRange* get_IntRangePtr() {return _pIntRange;}

    private:
        IntRange* _pIntRange;
    };
}
