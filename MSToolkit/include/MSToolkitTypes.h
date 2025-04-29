/*
Copyright 2005-2016, Michael R. Hoopmann

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
#ifndef _MSTOOLKITTYPES_H
#define _MSTOOLKITTYPES_H

#include <stddef.h>
#include <string>

//Version information stored here, can be recalled at any point
#define MST_VER "2.0.0"
#define MST_DATE "2024 SEPTEMBER 13"

#ifdef _MSC_VER
#  define strtok_r strtok_s
#  ifdef MSTOOLKIT_DLL
#    ifdef MSTOOLKIT_EXPORTS
#      define MSTOOLKIT_API extern __declspec(dllexport)
#    else
#      define MSTOOLKIT_API extern __declspec(dllimport)
#    endif
#  endif
#endif

namespace MSToolkit {

enum MSSpectrumType {
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

enum MSFileFormat {
  bms1,
  bms2,
  cms1,
  cms2,
  mgf,
  ms1,
  ms2,
  msmat_ff,
  mzXML,
  mz5,
	mzML,
  mzMLb,
  raw,
  sqlite,
  psm,
  uzs,
  zs,
	mzXMLgz,
	mzMLgz,
  dunno
};

enum MSTag {
  no,
  D,
  H,
  I,
  S,
  Z
};

enum MSActivation {
  mstCID,
  mstECD,
  mstETD,
  mstETDSA,
  mstPQD,
  mstHCD,
	mstIRMPD,
  mstSID,
  mstNA
};

struct MSHeader {
	char header[16][128];
};

struct MSPrecursorInfo {
  double mz=0;
  double monoMz=0;
  double isoMz=0;
  int charge=0;
  MSActivation activation=mstNA;
  int precursorScanNumber=0;
  double isoOffsetLower=0;
  double isoOffsetUpper=0;
};


struct MSScanInfo {
	int scanNumber[2];
	int numDataPoints;
  int numEZStates;
	int numZStates;
  float rTime;
  float IIT;
  float BPI;
  double* mz;
	int mzCount;
  double convA;
  double convB;
	double convC;
	double convD;
	double convE;
	double convI;
  double TIC;
  double BPM;
	MSScanInfo(){
    mz=NULL;
		scanNumber[0]=scanNumber[1]=0;
		numDataPoints=numEZStates=numZStates=0;
		rTime=IIT=BPI=0.0f;
		TIC=BPM=0.0;
		convA=convB=convC=convD=convE=convI=0.0;
    mzCount=0;
	}
	~MSScanInfo(){
		if(mz!=NULL) delete [] mz;
	}
};

struct DataPeak {
	double dMass;
	double dIntensity;
}; //For RAW files

struct Peak_T {
  double mz;
  float intensity;
};

struct ZState {
  int z;
  double mh;   //M+H, not mz
};

struct EZState {
  int z;
  double mh;      //M+H
  float pRTime;   //precursor area
  float pArea;    //precursor retention time
};

//To accommodate user params from mzML files, which could be anything...
enum eDataType{
  dtInt =0,
  dtFloat,
  dtDouble,
  dtString
};
  
struct MSUserParam {
  std::string name;
  std::string value;
  eDataType type=dtString;
  int toInt(){return atoi(value.c_str());}
  float toFloat(){return (float)atof(value.c_str());}
  double toDouble(){return atof(value.c_str());}
};

}

#endif


