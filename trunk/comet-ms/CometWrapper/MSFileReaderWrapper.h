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
		
    private:
        MSReader* _pMSReader;
        msclr::interop::marshal_context _marshalContext;
    };
}