#ifndef _RAWREADER_H
#define _RAWREADER_H

#ifdef _MSC_VER

#include "MSToolkitTypes.h"
#include "Spectrum.h"
#include <algorithm>
#include <iostream>
#include <vector>
#import "libid:F0C5F3E3-4F2A-443E-A74D-0AABE3237494" rename_namespace("XRawfile")
//#import "libid:5FE970A2-29C3-11D3-811D-00104B304896" rename_namespace("XRawfile")
using namespace XRawfile;
using namespace std;

namespace MSToolkit {

typedef struct rawPrecursorInfo{
  double  dIsoMZ;
  double  dMonoMZ;
  long    charge;
  long    parScanNum;
  rawPrecursorInfo(){
    dIsoMZ=0;
    dMonoMZ=0;
    charge=0;
    parScanNum=0;
  }
} rawPrecursorInfo;

class RAWReader {
public:
	//Constructors & Destructors
  RAWReader();
  ~RAWReader();

	//Public Functions
  void getInstrument(char* str);
	long getLastScanNumber();
  void getManufacturer(char* str);
	long getScanCount();
	bool getStatus();

	bool lookupRT(char* c, int scanNum, float& rt);
	bool readRawFile(const char* c, Spectrum& s, int scNum=0);
  void setAverageRaw(bool b, int width=1, long cutoff=1000);
  void setLabel(bool b);  //label data contains all centroids (including noise and excluded peaks)
	void setMSLevelFilter(vector<MSSpectrumType>* v);
  void setRawFilter(char* c);
  void setRawFilterExact(bool b);
	

private:

	//Data Members
  bool bRaw;
	bool rawAvg;
	bool rawFileOpen;
	bool rawLabel;
	bool rawUserFilterExact;

  char rawCurrentFile[256];
  char rawInstrument[256];
  char rawManufacturer[256];
  char rawUserFilter[256];

	int rawAvgWidth;

	long rawAvgCutoff;
	long rawCurSpec;
	long rawTotSpec;
  
#ifdef _WIN64
  IXRawfile3Ptr m_Raw;  //Note: minimum support is now IXRawfile3 interface on 64-bit installations.
#else
  IXRawfilePtr m_Raw;
#endif

	vector<MSSpectrumType>* msLevelFilter;

	//Private Functions
  int							calcChargeState(double precursormz, double highmass, VARIANT* varMassList, long nArraySize);
  double					calcPepMass(int chargestate, double precursormz);
  MSSpectrumType	evaluateFilter(long scan, char* chFilter, vector<double>& MZs, bool& bCentroid, double& cv, MSActivation& act);
	bool						initRaw();
  

};

}
#endif
#endif