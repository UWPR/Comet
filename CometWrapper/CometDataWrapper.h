/*
   Copyright 2015 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
        
        int get_dStart() {return (int)_pDoubleRange->dStart;}
        void set_dStart(double dStart) {_pDoubleRange->dStart = dStart;}

        int get_dEnd() {return (int)_pDoubleRange->dEnd;}
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

        int get_BinaryMod() {return _pVarMods->iBinaryMod;}
        void set_BinaryMod(int iBinaryMod) {_pVarMods->iBinaryMod = iBinaryMod;}

        int get_RequireThisMod() {return _pVarMods->iRequireThisMod;}
        void set_RequireThisMod(int iRequireThisMod) {_pVarMods->iRequireThisMod = iRequireThisMod;}

        int get_MaxNumVarModAAPerMod() {return _pVarMods->iMaxNumVarModAAPerMod;}
        void set_MaxNumVarModAAPerMod(int iMaxNumVarModAAPerMod) {_pVarMods->iMaxNumVarModAAPerMod = iMaxNumVarModAAPerMod;}

        int get_VarModTermDistance() {return _pVarMods->iVarModTermDistance;}
        void set_VarModTermDistance(int iVarModTermDistance) {_pVarMods->iVarModTermDistance = iVarModTermDistance;}

        int get_WhichTerm() {return _pVarMods->iWhichTerm;}
        void set_WhichTerm(int iWhichTerm) {_pVarMods->iWhichTerm = iWhichTerm;}

        int get_VarModMass() {return (int)_pVarMods->dVarModMass;}
        void set_VarModMass(double dVarModMass) {_pVarMods->dVarModMass = dVarModMass;}

        int get_VarNeutralLoss() {return (int)_pVarMods->dNeutralLoss;}
        void set_VarNeutralLoss(double dNeutralLoss) {_pVarMods->dNeutralLoss = dNeutralLoss;}

        // Changed return type from System::String^% to System::String^ to avoid returning a tracking reference
        // to a temporary. Also removed unnecessary gcnew (PtrToStringAnsi already returns a managed String^).
        System::String^ get_VarModChar()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pVarMods->szVarModChar));
        }
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
        EnzymeInfoWrapper(EnzymeInfo &enzymeInfo) { _pEnzymeInfo = new EnzymeInfo(enzymeInfo); }
        virtual ~EnzymeInfoWrapper()
        {
            if (_pEnzymeInfo)
            {
                delete _pEnzymeInfo;
                _pEnzymeInfo = nullptr;
            }
        }

        EnzymeInfo* get_EnzymeInfoPtr() { return _pEnzymeInfo; }

        int get_AllowedMissedCleavge() { return _pEnzymeInfo->iAllowedMissedCleavage; }
        void set_AllowedMissedCleavge(int v) { _pEnzymeInfo->iAllowedMissedCleavage = v; }

        int get_SearchEnzymeOffSet() { return _pEnzymeInfo->iSearchEnzymeOffSet; }
        void set_SearchEnzymeOffSet(int v) { _pEnzymeInfo->iSearchEnzymeOffSet = v; }

        int get_SearchEnzyme2OffSet() { return _pEnzymeInfo->iSearchEnzyme2OffSet; }
        void set_SearchEnzyme2OffSet(int v) { _pEnzymeInfo->iSearchEnzyme2OffSet = v; }

        int get_SampleEnzymeOffSet() { return _pEnzymeInfo->iSampleEnzymeOffSet; }
        void set_SampleEnzymeOffSet(int v) { _pEnzymeInfo->iSampleEnzymeOffSet = v; }

        // Changed all System::String^% returns to System::String^ to avoid C4172 (tracking reference to temporary).
        System::String^ get_SearchEnzymeName()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSearchEnzymeName));
        }
        void set_SearchEnzymeName(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSearchEnzymeName, tmp.c_str());
        }

        System::String^ get_SearchEnzymeBreakAA()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSearchEnzymeBreakAA));
        }
        void set_SearchEnzymeBreakAA(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSearchEnzymeBreakAA, tmp.c_str());
        }

        System::String^ get_SearchEnzymeNoBreakAA()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSearchEnzymeNoBreakAA));
        }
        void set_SearchEnzymeNoBreakAA(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSearchEnzymeNoBreakAA, tmp.c_str());
        }

        System::String^ get_SearchEnzyme2Name()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSearchEnzyme2Name));
        }
        void set_SearchEnzyme2Name(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSearchEnzyme2Name, tmp.c_str());
        }

        System::String^ get_SearchEnzyme2BreakAA()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSearchEnzyme2BreakAA));
        }
        void set_SearchEnzyme2BreakAA(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSearchEnzyme2BreakAA, tmp.c_str());
        }

        System::String^ get_SearchEnzyme2NoBreakAA()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSearchEnzyme2NoBreakAA));
        }
        void set_SearchEnzyme2NoBreakAA(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSearchEnzyme2NoBreakAA, tmp.c_str());
        }

        System::String^ get_SampleEnzymeName()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSampleEnzymeName));
        }
        void set_SampleEnzymeName(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSampleEnzymeName, tmp.c_str());
        }

        System::String^ get_SampleEnzymeBreakAA()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSampleEnzymeBreakAA));
        }
        void set_SampleEnzymeBreakAA(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSampleEnzymeBreakAA, tmp.c_str());
        }

        System::String^ get_SampleEnzymeNoBreakAA()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pEnzymeInfo->szSampleEnzymeNoBreakAA));
        }
        void set_SampleEnzymeNoBreakAA(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pEnzymeInfo->szSampleEnzymeNoBreakAA, tmp.c_str());
        }

    private:
        EnzymeInfo* _pEnzymeInfo;
    };

    public ref class InputFileInfoWrapper
    {
    public:
        InputFileInfoWrapper() { _pInputFileInfo = new InputFileInfo(); }
        InputFileInfoWrapper(InputFileInfo &inputFileInfo) { _pInputFileInfo = new InputFileInfo(inputFileInfo); }
        virtual ~InputFileInfoWrapper()
        {
            if (_pInputFileInfo)
            {
                delete _pInputFileInfo;
                _pInputFileInfo = nullptr;
            }
        }

        InputFileInfo* get_InputFileInfoPtr() { return _pInputFileInfo; }

        InputType get_InputType() { return static_cast<InputType>(_pInputFileInfo->iInputType); }
        void set_InputType(InputType v) { _pInputFileInfo->iInputType = static_cast<int>(v); }

        AnalysisType get_AnalysisType() { return static_cast<AnalysisType>(_pInputFileInfo->iAnalysisType); }
        void set_AnalysisType(AnalysisType v) { _pInputFileInfo->iAnalysisType = static_cast<int>(v); }

        int get_FirstScan() { return _pInputFileInfo->iFirstScan; }
        void set_FirstScan(int v) { _pInputFileInfo->iFirstScan = v; }

        int get_LastScan() { return _pInputFileInfo->iLastScan; }
        void set_LastScan(int v) { _pInputFileInfo->iLastScan = v; }

        System::String^ get_FileName()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pInputFileInfo->szFileName));
        }
        void set_FileName(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pInputFileInfo->szFileName, tmp.c_str());
        }

        System::String^ get_BaseName()
        {
            return Marshal::PtrToStringAnsi(IntPtr(_pInputFileInfo->szBaseName));
        }
        void set_BaseName(System::String^ value)
        {
            std::string tmp = marshal_as<std::string>(value);
            strcpy(_pInputFileInfo->szBaseName, tmp.c_str());
        }

    private:
        InputFileInfo* _pInputFileInfo;
    };

    public ref class ScoreWrapper
    {
    public:
        ScoreWrapper(const Scores & score)
        {
            pScores = new Scores(score);
        }

        ~ScoreWrapper()
        {
            this->!ScoreWrapper();
        }

        !ScoreWrapper()
        {
            delete pScores;
        }

        property double xCorr
        { 
            double get() { return pScores->xCorr; }
        }

        property double dSp
        {
           double get() { return pScores->dSp; }
        }

        property double dCn
        {
           double get() { return pScores->dCn; }
        }

        property double dExpect
        {
            double get() { return pScores->dExpect; }
        }

        property double dAScoreScore
        {
           double get() { return pScores->dAScorePro; }
        }

        property System::String^ sAScoreProSiteScores
        {
           String^ get() { return gcnew String(pScores->sAScoreProSiteScores.c_str()); }
        }

        property double mass
        {
            double get() { return pScores->mass; }
        }

        property int MatchedIons
        {
            int get() { return (int)pScores->matchedIons; }
        }

        property int TotalIons
        {
            int get() { return (int)pScores->totalIons; }
        }

    private:
        Scores * pScores;
    };

    public ref class ScoreWrapperMS1
    {
    public:
       ScoreWrapperMS1(const ScoresMS1& score)
       {
          pScoresMS1 = new ScoresMS1(score);
       }

       ~ScoreWrapperMS1()
       {
          this->!ScoreWrapperMS1();
       }

       !ScoreWrapperMS1()
       {
          delete pScoresMS1;
       }

       property float fDotProduct
       {
          float get() { return pScoresMS1->fDotProduct; }
       }

       property float fRTime
       {
          float get() { return pScoresMS1->fRTime; }
       }

       property int iScanNumber
       {
          int get() { return pScoresMS1->iScanNumber; }
       }

    private:
       ScoresMS1* pScoresMS1;
    };

    public enum class IonSeries : int { a, b, c, x, y, z };

    public ref class FragmentWrapper
    {
    public:
        FragmentWrapper(const Fragment & fragment)
        {
            pFragment = new Fragment(fragment);
        }

        ~FragmentWrapper()
        {
            this->!FragmentWrapper();
        }
        
        !FragmentWrapper()
        {
            delete pFragment;
        }

        property double Mass
        {
            double get() { return pFragment->mass; }
        }

        property double MZ
        {
            double get() { return pFragment->ToMz(); }
        }

        property bool IsNeutralLossFragment
        {
            bool get() { return pFragment->neutralLoss; }
        }

        property double NeutralLossMass
        {
            double get() { return pFragment->neutralLossMass; }
        }

        property double Intensity
        {
            double get() { return pFragment->intensity; }
        }

        property int Number
        {
            int get() { return pFragment->number; }
        }

        property IonSeries Type
        {
            IonSeries get() { return (IonSeries)pFragment->type; }
        }

        property int Charge
        {
            int get() { return pFragment->charge; }
        }

    private:
        Fragment * pFragment;
    };
}
