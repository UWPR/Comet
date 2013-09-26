// CometWrapper.h

#pragma once

#include "CometData.h"

using namespace System;

namespace CometWrapper {

    public ref class IntRangeWrapper
    {
    public:
        IntRangeWrapper() { _pIntRange = new IntRange(); }
        IntRangeWrapper(int iStart, int iEnd) { _pIntRange = new IntRange(iStart, iEnd); }
        virtual ~IntRangeWrapper();
        
        int get_iStart() {return _pIntRange->iStart;}
        void set_iStart(int iStart) {_pIntRange->iStart = iStart;}

        int get_iEnd() {return _pIntRange->iEnd;}
        void set_iEnd(int iEnd) {_pIntRange->iEnd = iEnd;}

    private:
        IntRange* _pIntRange;
    };
}
