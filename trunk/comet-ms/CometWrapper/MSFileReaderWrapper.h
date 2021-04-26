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

// MSFileReaderWrapper.h

#pragma once

#include "MSReader.h"
#include "Spectrum.h"
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

using namespace MSToolkit;
using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;

namespace CometWrapper {

    public enum class MSSpectrumTypeWrapper 
    {
        MS1,
        MS2,
        MS3,
        ZS,
        UZS,
        IonSpec,
        SRM,
        REFERENCE,
        Unspecified,
        MSX
    };

    public ref class Peak_T_Wrapper
    {
    public:
        Peak_T_Wrapper() { _pPeak = new Peak_T(); }
        Peak_T_Wrapper(Peak_T &peak) { _pPeak = new Peak_T(peak);}
        virtual ~Peak_T_Wrapper() 
        { 
            if (NULL != _pPeak)
            {
                delete _pPeak;
                _pPeak = NULL;
            }
        }
        
        Peak_T* get_Peak_T_Ptr() {return _pPeak;}
        double get_mz() {return _pPeak->mz;}
        void set_mz(double mz) {_pPeak->mz = mz;}

        float get_intensity() {return _pPeak->intensity;}
        void set_intensity(float intensity) {_pPeak->intensity = intensity;}

    private:
        Peak_T* _pPeak;
    };

    public ref class MSFileReaderWrapper
    {
    public:
        MSFileReaderWrapper();
        virtual ~MSFileReaderWrapper();
        bool ReadPeaks(String^ msFileName, int scanNum, MSSpectrumTypeWrapper msSpectrumType, List<Peak_T_Wrapper^> ^peaks);
        bool ReadPrecursorPeaks(String^ msFileName, int fragmentScanNum, MSSpectrumTypeWrapper msFragmentSpectrumType, List<Peak_T_Wrapper^> ^precursorPeaks, int% ms1ScanNum);

    private:
        MSReader* _pMSReader;
        msclr::interop::marshal_context _marshalContext;
    };
}
