#ifndef _RAWREADER_H
#define _RAWREADER_H

#ifdef _MSC_VER

#include "MSToolkitTypes.h"
#include "Spectrum.h"
#include <algorithm>
#include <iostream>
#include <vector>
//#include <cstring>
//#include <cstdlib>
//#include <iomanip>

//Explicit path is needed on systems where Xcalibur installs with errors
//For example, Vista-64bit
//#import "C:\Xcalibur\system\programs\XRawfile2.dll" 
#import "MSFileReader.XRawfile2.dll" rename_namespace("XRawfile")
using namespace XRawfile;
using namespace std;

namespace MSToolkit {
class RAWReader {
public:
	//Constructors & Destructors
  RAWReader();
  ~RAWReader();

	//Public Functions
	long getLastScanNumber();
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

  char rawUserFilter[256];

	int rawAvgWidth;

	long rawAvgCutoff;
	long rawCurSpec;
	long rawTotSpec;
  
  IXRawfilePtr m_Raw;
	vector<MSSpectrumType>* msLevelFilter;

	//Private Functions
  int							calcChargeState(double precursormz, double highmass, VARIANT* varMassList, long nArraySize);
  double					calcPepMass(int chargestate, double precursormz);
  MSSpectrumType	evaluateFilter(long scan, char* chFilter, vector<double>& MZs, bool& bCentroid, double& cv);
	bool						initRaw();
  

};

}
#endif
#endif