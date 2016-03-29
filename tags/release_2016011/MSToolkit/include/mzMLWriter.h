#ifndef _MZMLWRITER_H
#define _MZMLWRITER_H

#include <string>
#include <iostream>
#include "MSObject.h"
#include "MSReader.h"
#include "Spectrum.h"
#include "mzParser.h"

using namespace std;

namespace MSToolkit {

typedef struct sMzMLIndex{
  string id;
  f_off offset;
} sMzMLIndex;

class MzMLWriter {
public:

  MzMLWriter();
  ~MzMLWriter();

  bool  closeMzML();
  bool  createMzML(char* fn);
  int   checkState();
  void  setNumpress(bool b);
  void  setTabs(bool b);
  void  setZlib(bool b);
  bool  writeRunInformation();
  bool  writeSpectra(MSObject& o);
  bool  writeSpectra(Spectrum& s);
  bool  writeChromatograms();
  bool  writeIndex();

private:
  bool exportActivation(Spectrum& s, int tabs=0);
  bool exportAnalyzer();
  bool exportBinary(char* str, int len, int tabs=0);
  bool exportBinaryDataArrayList(Spectrum& s, int tabs=0);
  bool exportBinaryDataArray(Spectrum& s, bool bMZ, int tabs=0);
  bool exportChromatogram();
  bool exportChromatogramList();
  bool exportComponentList();
  bool exportContact();
  bool exportCv();
  bool exportCvList();
  bool exportCvParam(string ac, string ref, string name, string unitAc="", string unitRef="", string unitName="", string value="", int tabs=0);
  bool exportDataProcessing();
  bool exportDataProcessingList();
  bool exportFileContent();
  bool exportFileDescription();
  bool exportInstrumentConfiguration();
  bool exportInstrumentConfigurationList();
  bool exportIsolationWindow(Spectrum& s, int tabs=0);
  bool exportMzML();
  bool exportOffset(string idRef, f_off offset, int tabs=0);
  bool exportPrecursor(Spectrum& s, int tabs=0);
  bool exportPrecursorList(Spectrum& s, int tabs=0);
  bool exportProcessingMethod();
  bool exportProduct();
  bool exportProductList();
  bool exportReferencableParamGroup();
  bool exportReferenceableParamGroupList();
  bool exportReferenceableParamGroupRef();
  bool exportRun();
  bool exportSample();
  bool exportSampleList();
  bool exportScan(Spectrum& s, int tabs=0);
  bool exportScanList(Spectrum& s, int tabs=0);
  bool exportScanSettings();
  bool exportScanSettingsList();
  bool exportScanWindowList();
  bool exportSelectedIon(Spectrum& s, int tabs=0);
  bool exportSelectedIonList(Spectrum& s, int tabs=0);
  bool exportSoftware();
  bool exportSoftwareList();
  bool exportSoftwareRef();
  bool exportSource();
  bool exportSouceFile();
  bool exportSourceFileList();
  bool exportSourceFileRef();
  bool exportSourceFileRefList();
  bool exportSpectrum(Spectrum& s, int tabs=0);
  bool exportSpectrumList();
  bool exportTarget();
  bool exportTargetList();
  bool exportDetector();
  bool exportUserParam();

  int index;
  FILE* fptr;
  f_off fSpecList;

  bool bTabs;
  bool bFileOpen;
  bool bZlib;
  bool bNumpress;

  vector<sMzMLIndex> vIndex;

};
}

#endif
