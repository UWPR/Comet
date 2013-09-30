// CometWrapper.h

#pragma once

#include "CometData.h"
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace msclr::interop;
using namespace System::Runtime::InteropServices;

namespace CometWrapper {

    public ref class IntRangeWrapper
    {
    public:
        IntRangeWrapper() { _pIntRange = new IntRange(); }
        IntRangeWrapper(IntRange &intRangeParam) { _pIntRange = new IntRange(intRangeParam); }
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

    public ref class DoubleRangeWrapper
    {
    public:
        DoubleRangeWrapper() { _pDoubleRange = new DoubleRange(); }
        DoubleRangeWrapper(DoubleRange &doubleRangeParam) { _pDoubleRange = new DoubleRange(doubleRangeParam); }
        DoubleRangeWrapper(double dStart, double dEnd) { _pDoubleRange = new DoubleRange(dStart, dEnd); }
        virtual ~DoubleRangeWrapper() 
        { 
            if (NULL != _pDoubleRange)
            {
                delete _pDoubleRange;
                _pDoubleRange = NULL;
            }
        }
        
        int get_dStart() {return _pDoubleRange->dStart;}
        void set_dStart(double dStart) {_pDoubleRange->dStart = dStart;}

        int get_dEnd() {return _pDoubleRange->dEnd;}
        void set_dEnd(double dEnd) {_pDoubleRange->dEnd = dEnd;}

        DoubleRange* get_DoubleRangePtr() {return _pDoubleRange;}

    private:
        DoubleRange* _pDoubleRange;
    };

    public ref class VarModsWrapper
    {
    public:
        VarModsWrapper() { _pVarMods = new VarMods(); }
        VarModsWrapper(VarMods &varMods) { _pVarMods = new VarMods(varMods);}
        virtual ~VarModsWrapper() 
        { 
            if (NULL != _pVarMods)
            {
                delete _pVarMods;
                _pVarMods = NULL;
            }
        }

        int get_BinaryMod() {return _pVarMods->bBinaryMod;}
        void set_BinaryMod(int bBinaryMod) {_pVarMods->bBinaryMod = bBinaryMod;}

        int get_MaxNumVarModAAPerMod() {return _pVarMods->iMaxNumVarModAAPerMod;}
        void set_MaxNumVarModAAPerMod(int iMaxNumVarModAAPerMod) {_pVarMods->iMaxNumVarModAAPerMod = iMaxNumVarModAAPerMod;}

        int get_VarModMass() {return _pVarMods->dVarModMass;}
        void set_VarModMass(double dVarModMass) {_pVarMods->dVarModMass = dVarModMass;}

        System::String^% get_VarModChar() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pVarMods->szVarModChar))));}
        void set_VarModChar(System::String^ varModChar) 
        {
            std::string stdVarModChar = marshal_as<std::string>(varModChar);    
            strcpy(_pVarMods->szVarModChar, stdVarModChar.c_str());
        }

        VarMods* get_VarModsPtr() {return _pVarMods;}

    private:
        VarMods *_pVarMods;
    };
}
