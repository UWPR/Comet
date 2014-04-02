// CometWrapper.h

#pragma once

#include "CometData.h"
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace msclr::interop;
using namespace System::Runtime::InteropServices;

namespace CometWrapper {

    public enum class InputType
    {
        Unknown = -1,
        MS2 = 0,
        MZXML,
        MZML,
        CMS2
    };

    public enum class AnalysisType
    {
        Unknown = 0,
        DTA,
        SpecificScan,
        SpecificScanRange,
        EntireFile
    };

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

    public ref class EnzymeInfoWrapper
    {
    public:
        EnzymeInfoWrapper() { _pEnzymeInfo = new EnzymeInfo(); }
        EnzymeInfoWrapper(EnzymeInfo &enzymeInfo) { _pEnzymeInfo = new EnzymeInfo(enzymeInfo);}
        virtual ~EnzymeInfoWrapper() 
        { 
            if (NULL != _pEnzymeInfo)
            {
                delete _pEnzymeInfo;
                _pEnzymeInfo = NULL;
            }
        }
        
        EnzymeInfo* get_EnzymeInfoPtr() {return _pEnzymeInfo;}

        int get_AllowedMissedCleavge() {return _pEnzymeInfo->iAllowedMissedCleavage;}
        void set_AllowedMissedCleavge(int iAllowedMissedCleavage) {_pEnzymeInfo->iAllowedMissedCleavage = iAllowedMissedCleavage;}

        int get_SearchEnzymeOffSet() {return _pEnzymeInfo->iSearchEnzymeOffSet;}
        void set_SearchEnzymeOffSet(int iSearchEnzymeOffSet) {_pEnzymeInfo->iSearchEnzymeOffSet = iSearchEnzymeOffSet;}

        int get_SampleEnzymeOffSet() {return _pEnzymeInfo->iSampleEnzymeOffSet;}
        void set_SampleEnzymeOffSet(int iSampleEnzymeOffSet) {_pEnzymeInfo->iSampleEnzymeOffSet = iSampleEnzymeOffSet;}

        System::String^% get_SearchEnzymeName() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pEnzymeInfo->szSearchEnzymeName))));}
        void set_SearchEnzymeName(System::String^ searchEnzymeName) 
        {
            std::string stdSearchEnzymeName = marshal_as<std::string>(searchEnzymeName);    
            strcpy(_pEnzymeInfo->szSearchEnzymeName, stdSearchEnzymeName.c_str());
        }

        System::String^% get_SearchEnzymeBreakAA() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pEnzymeInfo->szSearchEnzymeBreakAA))));}
        void set_SearchEnzymeBreakAA(System::String^ searchEnzymeBreakAA) 
        {
            std::string stdSearchEnzymeBreakAA = marshal_as<std::string>(searchEnzymeBreakAA);    
            strcpy(_pEnzymeInfo->szSearchEnzymeBreakAA, stdSearchEnzymeBreakAA.c_str());
        }

        System::String^% get_SearchEnzymeNoBreakAA() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pEnzymeInfo->szSearchEnzymeNoBreakAA))));}
        void set_SearchEnzymeNoBreakAA(System::String^ searchEnzymeNoBreakAA) 
        {
            std::string stdSearchEnzymeNoBreakAA = marshal_as<std::string>(searchEnzymeNoBreakAA);    
            strcpy(_pEnzymeInfo->szSearchEnzymeNoBreakAA, stdSearchEnzymeNoBreakAA.c_str());
        }

        System::String^% get_SampleEnzymeName() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pEnzymeInfo->szSampleEnzymeName))));}
        void set_SampleEnzymeName(System::String^ sampleEnzymeName) 
        {
            std::string stdSampleEnzymeName = marshal_as<std::string>(sampleEnzymeName);    
            strcpy(_pEnzymeInfo->szSampleEnzymeName, stdSampleEnzymeName.c_str());
        }

        System::String^% get_SampleEnzymeBreakAA() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pEnzymeInfo->szSampleEnzymeBreakAA))));}
        void set_SampleEnzymeBreakAA(System::String^ sampleEnzymeBreakAA) 
        {
            std::string stdSampleEnzymeBreakAA = marshal_as<std::string>(sampleEnzymeBreakAA);    
            strcpy(_pEnzymeInfo->szSampleEnzymeBreakAA, stdSampleEnzymeBreakAA.c_str());
        }

        System::String^% get_SampleEnzymeNoBreakAA() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pEnzymeInfo->szSampleEnzymeNoBreakAA))));}
        void set_SampleEnzymeNoBreakAA(System::String^ sampleEnzymeNoBreakAA) 
        {
            std::string stdSampleEnzymeNoBreakAA = marshal_as<std::string>(sampleEnzymeNoBreakAA);    
            strcpy(_pEnzymeInfo->szSampleEnzymeNoBreakAA, stdSampleEnzymeNoBreakAA.c_str());
        }

    private:
        EnzymeInfo *_pEnzymeInfo;
    };

    public ref class InputFileInfoWrapper
    {
    public:
        InputFileInfoWrapper() { _pInputFileInfo = new InputFileInfo(); }
        InputFileInfoWrapper(InputFileInfo &inputFileInfo) { _pInputFileInfo = new InputFileInfo(inputFileInfo);}
        virtual ~InputFileInfoWrapper() 
        { 
            if (NULL != _pInputFileInfo)
            {
                delete _pInputFileInfo;
                _pInputFileInfo = NULL;
            }
        }
        
        InputFileInfo* get_InputFileInfoPtr() {return _pInputFileInfo;}

        InputType get_InputType() {return static_cast<InputType>(_pInputFileInfo->iInputType);}
        void set_InputType(InputType inputType) {_pInputFileInfo->iInputType = static_cast<int>(inputType);}

        AnalysisType get_AnalysisType() {return static_cast<AnalysisType>(_pInputFileInfo->iAnalysisType);}
        void set_AnalysisType(AnalysisType analysisType) {_pInputFileInfo->iAnalysisType = static_cast<int>(analysisType);}

        int get_FirstScan() {return _pInputFileInfo->iFirstScan;}
        void set_FirstScan(int iFirstScan) {_pInputFileInfo->iFirstScan = iFirstScan;}

        int get_LastScan() {return _pInputFileInfo->iLastScan;}
        void set_LastScan(int iLastScan) {_pInputFileInfo->iLastScan = iLastScan;}

        System::String^% get_FileName() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pInputFileInfo->szFileName))));}
        void set_FileName(System::String^ fileName) 
        {
            std::string stdFileName = marshal_as<std::string>(fileName);    
            strcpy(_pInputFileInfo->szFileName, stdFileName.c_str());
        }

        System::String^% get_BaseName() { return gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(_pInputFileInfo->szBaseName))));}
        void set_BaseName(System::String^ baseName) 
        {
            std::string stdBaseName = marshal_as<std::string>(baseName);
            strcpy(_pInputFileInfo->szBaseName, stdBaseName.c_str());
        }

    private:
        InputFileInfo *_pInputFileInfo;
    };
}
