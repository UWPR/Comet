/*
PWIZface - The code is
open source under the FreeBSD License, please see LICENSE file
for detailed information.

Copyright (C) 2011, Mike Hoopmann, Institute for Systems Biology
Version 1.0, January 4, 2011.
Version 1.1, March 14, 2012.
*/
#include "mzParser.h"
using namespace std;
using namespace mzParser;

Chromatogram::Chromatogram(){
  bc = NULL;
}

Chromatogram::~Chromatogram(){
  bc = NULL;
}

void Chromatogram::getTimeIntensityPairs(vector<TimeIntensityPair>& v){
  if(bc==NULL) cerr << "Null chromatogram" << endl;
  else v=bc->getData();
}

ChromatogramList::ChromatogramList(){
}

ChromatogramList::ChromatogramList(mzpSAXMzmlHandler* ml, void* m5, BasicChromatogram* bc){
  mzML=ml;
  #ifdef MZP_HDF
    mz5=(mzpMz5Handler*)m5;
  #endif
  chromat=new Chromatogram();
  chromat->bc=bc;
}

ChromatogramList::~ChromatogramList(){
  mzML=NULL;
  vChromatIndex=NULL;
  #ifdef MZP_HDF
  mz5=NULL;
  vMz5Index=NULL;
  #endif
  delete chromat;
}

ChromatogramPtr ChromatogramList::chromatogram(int index, bool binaryData) {
  string str;
  if(mzML!=NULL) {
    mzML->readChromatogram(index);
    chromat->bc->getIDString(str);
    chromat->id=str;
    return chromat;
  #ifdef MZP_HDF
  }  else if(mz5!=NULL) {
    mz5->readChromatogram(index);
    chromat->bc->getIDString(str);
    chromat->id=str;
    return chromat;
  #endif
  }
  return NULL;
}

bool ChromatogramList::get() {
  if(mzML!=NULL) vChromatIndex = mzML->getChromatIndex();
  #ifdef MZP_HDF
  else if(mz5!=NULL) vMz5Index = mz5->getChromatIndex();
  #endif
  else return false;
  return true;
}

size_t ChromatogramList::size() {
  if(vChromatIndex==NULL) {
    cerr << "Get chromatogram list first." << endl;
    return 0;
  }
  if(mzML!=NULL) return vChromatIndex->size();
  #ifdef MZP_HDF
  else if(mz5!=NULL) return vMz5Index->size();
  #endif
  else return 0;
}

PwizRun::PwizRun(){
  chromatogramListPtr = new ChromatogramList();
  spectrumListPtr = new SpectrumList();
}

PwizRun::PwizRun(mzpSAXMzmlHandler* ml, void* m5, BasicChromatogram* b){
  mzML=ml;
#ifdef MZP_HDF
  mz5=(mzpMz5Handler*)m5;
#endif
  bc = NULL;
  bs = NULL;
  delete chromatogramListPtr;
  delete spectrumListPtr;
}

PwizRun::~PwizRun(){
  mzML=NULL;
#ifdef MZP_HDF
  mz5=NULL;
#endif
  bc=NULL;
  bs=NULL;
  delete chromatogramListPtr;
  delete spectrumListPtr;
}

bool PwizRun::get(){
  if (mzML != NULL) return chromatogramListPtr->get();
#ifdef MZP_HDF
  else if (mz5 != NULL) chromatogramListPtr->vMz5Index = mz5->getChromatIndex();
#endif
  else return false;
}

void PwizRun::set(mzpSAXMzmlHandler* ml, void* m5, BasicChromatogram* b, BasicSpectrum* s){
  mzML=ml;
#ifdef MZP_HDF
  mz5=(mzpMz5Handler*)m5;
#endif
  bc=b;
  bs=s;
  delete chromatogramListPtr;
  chromatogramListPtr = new ChromatogramList(ml, m5, b);
  delete spectrumListPtr;
  spectrumListPtr = new SpectrumList(ml, m5, s);
}

MSDataFile::MSDataFile(string s){
  int i=checkFileType(&s[0]);
  if(i==0){
    cerr << "Cannot identify file type." << endl;
  } else {
    bs = new BasicSpectrum();
    bc = new BasicChromatogram();
    switch(i){
      case 1: //mzML
      case 3:
        mzML=new mzpSAXMzmlHandler(bs,bc);
        if(i==3) mzML->setGZCompression(true);
        else mzML->setGZCompression(false);
        if(!mzML->load(&s[0])){
          cerr << "Failed to load file." << endl;
          delete mzML;
          delete bs;
          delete bc;
        }
        run.chromatogramListPtr->vChromatIndex=mzML->getChromatIndex();
        run.spectrumListPtr->vSpecIndex = mzML->getSpecIndex();
        break;
      case 2: //mzXML
      case 4:
        cerr << "mzXML not supported in this interface." << endl;
        delete bs;
        delete bc;
        break;
#ifdef MZP_HDF
      case 5: //mz5
        mz5Config = new mzpMz5Config();
        mz5=new mzpMz5Handler(mz5Config, bs, bc);
        if(!mz5->readFile(&s[0])){
          cerr << "Failed to load file." << endl;
          delete mz5;
          delete mz5Config;
          delete bs;
          delete bc;
        }
        break;
#endif
      default:
        break;
    }
#ifdef MZP_HDF
    run.set(mzML,mz5,bc,bs);
#else
    run.set(mzML,NULL,bc,bs);
#endif
  }
}

MSDataFile::~MSDataFile(){
  if(mzML!=NULL) delete mzML;
#ifdef MZP_HDF
  if(mz5!=NULL){
    delete mz5;
    delete mz5Config;
  }
#endif
  delete bs;
  delete bc;
}

SpectrumInfo::SpectrumInfo(){
}

void SpectrumInfo::update(const Spectrum& spectrum, bool getBinaryData){

  scanNumber = spectrum.bs->getScanNum();
  msLevel = spectrum.bs->getMSLevel();
  retentionTime = spectrum.bs->getRTime(false); // seconds

  mzLow = spectrum.bs->getLowMZ();
  mzHigh = spectrum.bs->getHighMZ();
  basePeakMZ = spectrum.bs->getBasePeakMZ();
  basePeakIntensity = spectrum.bs->getBasePeakIntensity();
  totalIonCurrent = spectrum.bs->getTotalIonCurrent();
  //thermoMonoisotopicMZ=spectrum.bs->getPrecursorMonoMZ();
  ionInjectionTime = spectrum.bs->getIonInjectionTime();

  //slow and always data
  MZIntensityPair p;
  data.clear();
  for(size_t i=0;i<spectrum.bs->size();i++){
    p.mz=spectrum.bs->operator[](i).mz;
    p.intensity=spectrum.bs->operator[](i).intensity;
    data.push_back(p);
  }
}

Spectrum::Spectrum() {
  bs = NULL;
}

Spectrum::~Spectrum() {
  bs = NULL;
}

SpectrumList::SpectrumList() {
}

SpectrumList::SpectrumList(mzpSAXMzmlHandler * ml, void* m5, BasicSpectrum * bs) {
  mzML = ml;
#ifdef MZP_HDF
  mz5 = (mzpMz5Handler*)m5;
#endif
  spec = new Spectrum();
  spec->bs = bs;
}

SpectrumList::~SpectrumList() {
  mzML = NULL;
  vSpecIndex = NULL;
#ifdef MZP_MZ5
  mz5 = NULL;
  vMz5Index = NULL;
#endif
  delete spec;
}

bool SpectrumList::get() {
  if (mzML != NULL) vSpecIndex = mzML->getSpecIndex();
#ifdef MZP_HDF
  else if (mz5 != NULL) vMz5Index = mz5->getSpecIndex();
#endif
  else return false;
  return true;
}

size_t SpectrumList::size() {
  if (vSpecIndex == NULL) {
    cerr << "Get spectrum list first." << endl;
    return 0;
  }
  if (mzML != NULL) return vSpecIndex->size();
#ifdef MZP_HDF
  else if (mz5 != NULL) return vMz5Index->size();
#endif
  else return 0;
}

SpectrumPtr SpectrumList::spectrum(int index, bool binaryData) {
  string str;
  if (mzML != NULL) {
    mzML->readSpectrum(index);
    spec->bs->getIDString(str);
    spec->id = str;
    return spec;
#ifdef MZP_HDF
  } else if (mz5 != NULL) {
    mz5->readSpectrum(index);
    spec->bs->getIDString(str);
    spec->id = str;
    return spec;
#endif
  }
  return NULL;
}
