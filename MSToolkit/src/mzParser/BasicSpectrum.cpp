/*  
  BasicSpectrum.cpp
  Copyright (C) 2010-2012, Mike Hoopmann
  Institute for Systems Biology
  Version 1.1, Mar. 28, 2012
*/

#include "mzParser.h"
using namespace std;
using namespace mzParser;

//------------------------------------------
//  Constructors & Destructors
//------------------------------------------
BasicSpectrum::BasicSpectrum() {
  activation=none;
  basePeakIntensity=0.0;
  basePeakMZ=0.0;
  centroid=false;
  filterLine[0]='\0';
  highMZ=0.0;
  inverseReducedIonMobility=0;
  ionInjectionTime=0.0;
  ionMobilityDriftTime=0;
  ionMobilityScan=false;
  lowMZ=0.0;
  msLevel=1;
  peaksCount=0;
  positiveScan=true;
  compensationVoltage=0.0;
  precursorScanNum=-1;
  rTime=0.0f;
  scanIndex=0;
  scanNum=-1;
  totalIonCurrent=0.0;
  idString[0]='\0';
  vData=new vector<specDP>;
  vDataIonMob = new vector<specIonMobDP>;
  vPrecursor=new vector<sPrecursorIon>;
  vUserParam=new vector<sUParam>;
}
BasicSpectrum::BasicSpectrum(const BasicSpectrum& s){
  vData=new vector<specDP>(*s.vData);
  vDataIonMob = new vector<specIonMobDP>(*s.vDataIonMob);
  vPrecursor=new vector<sPrecursorIon>(*s.vPrecursor);
  vUserParam = new vector<sUParam>(*s.vUserParam);
  activation=s.activation;
  basePeakIntensity=s.basePeakIntensity;
  basePeakMZ=s.basePeakMZ;
  centroid=s.centroid;
  highMZ=s.highMZ;
  inverseReducedIonMobility=s.inverseReducedIonMobility;
  ionInjectionTime=s.ionInjectionTime;
  ionMobilityDriftTime=s.ionMobilityDriftTime;
  ionMobilityScan=s.ionMobilityScan;
  lowMZ=s.lowMZ;
  msLevel=s.msLevel;
  peaksCount=s.peaksCount;
  positiveScan=s.positiveScan;
  compensationVoltage=s.compensationVoltage;
  precursorScanNum=s.precursorScanNum;
  rTime=s.rTime;
  scanIndex=s.scanIndex;
  scanNum=s.scanNum;
  totalIonCurrent=s.totalIonCurrent;
  strcpy(idString,s.idString);
  strcpy(filterLine,s.filterLine);
}
BasicSpectrum::~BasicSpectrum() {
  delete vData;
  delete vDataIonMob;
  delete vPrecursor;
  delete vUserParam;
}

//------------------------------------------
//  Operator overloads
//------------------------------------------
BasicSpectrum& BasicSpectrum::operator=(const BasicSpectrum& s){
  if (this != &s) {
    delete vData;
    delete vDataIonMob;
    delete vPrecursor;
    delete vUserParam;
    vData=new vector<specDP>(*s.vData);
    vDataIonMob=new vector<specIonMobDP>(*s.vDataIonMob);
    vPrecursor=new vector<sPrecursorIon>(*s.vPrecursor);
    vUserParam = new vector<sUParam>(*s.vUserParam);
    activation=s.activation;
    basePeakIntensity=s.basePeakIntensity;
    basePeakMZ=s.basePeakMZ;
    centroid=s.centroid;
    highMZ=s.highMZ;
    inverseReducedIonMobility=s.inverseReducedIonMobility;
    ionInjectionTime=s.ionInjectionTime;
    ionMobilityDriftTime=s.ionMobilityDriftTime;
    ionMobilityScan=s.ionMobilityScan;
    lowMZ=s.lowMZ;
    msLevel=s.msLevel;
    peaksCount=s.peaksCount;
    positiveScan=s.positiveScan;
    compensationVoltage=s.compensationVoltage;
    precursorScanNum=s.precursorScanNum;
    rTime=s.rTime;
    scanIndex=s.scanIndex;
    scanNum=s.scanNum;
    totalIonCurrent=s.totalIonCurrent;
    strcpy(filterLine,s.filterLine);
    strcpy(idString,s.idString);
  }
  return *this;
}
specDP& BasicSpectrum::operator[ ](const size_t& index) {
  return vData->at(index);
}

//------------------------------------------
//  Modifiers
//------------------------------------------
void BasicSpectrum::addDP(specDP dp) { vData->push_back(dp);}
void BasicSpectrum::addDP(specIonMobDP dp) { vDataIonMob->push_back(dp); }
void BasicSpectrum::clear(){
  activation=none;
  basePeakIntensity=0.0;
  basePeakMZ=0.0;
  centroid=false;
  filterLine[0]='\0';
  highMZ=0.0;
  idString[0]='\0';
  inverseReducedIonMobility=0;
  ionInjectionTime=0.0;
  ionMobilityDriftTime=0;
  ionMobilityScan=false;
  lowMZ=0.0;
  msLevel=1;
  peaksCount=0;
  positiveScan=true;
  compensationVoltage=0.0;
  precursorScanNum=-1;
  rTime=0.0f;
  scanIndex=0;
  scanNum=-1;
  totalIonCurrent=0.0;
  vData->clear();
  vDataIonMob->clear();
  vPrecursor->clear();
  vUserParam->clear();
}
void BasicSpectrum::clearPrecursor(){ vPrecursor->clear();}
void BasicSpectrum::setActivation(int a){ activation=a;}
void BasicSpectrum::setBasePeakIntensity(double d){ basePeakIntensity=d;}
void BasicSpectrum::setBasePeakMZ(double d){ basePeakMZ=d;}
void BasicSpectrum::setCentroid(bool b){ centroid=b;}
void BasicSpectrum::setCollisionEnergy(double d){ collisionEnergy=d;}
void BasicSpectrum::setCompensationVoltage(double d){ compensationVoltage=d; }
void BasicSpectrum::setFilterLine(char* str) { 
  strncpy(filterLine,str,127);
  filterLine[127]='\0';
}
void BasicSpectrum::setHighMZ(double d){ highMZ=d;}
void BasicSpectrum::setIDString(const char* str) { 
  strncpy(idString,str,127); 
  idString[127]='\0';
}
void BasicSpectrum::setInverseReducedIonMobility(double d){ inverseReducedIonMobility=d;}
void BasicSpectrum::setIonInjectionTime(double d){ ionInjectionTime=d;}
void BasicSpectrum::setIonMobilityScan(bool b){ ionMobilityScan=b;}
void BasicSpectrum::setIonMobilityDriftTime(double d){ionMobilityDriftTime=d;}
void BasicSpectrum::setLowMZ(double d){ lowMZ=d;}
void BasicSpectrum::setMSLevel(int level){ msLevel=level;}
void BasicSpectrum::setPeaksCount(int i){ peaksCount=i;}
void BasicSpectrum::setPositiveScan(bool b){ positiveScan=b;}
void BasicSpectrum::setPrecursorCharge(int i){
  if(vPrecursor->size()==0) vPrecursor->emplace_back();
  vPrecursor->back().charge=i;
}
void BasicSpectrum::setPrecursorIntensity(double d) {
  if (vPrecursor->size() == 0) vPrecursor->emplace_back();
  vPrecursor->back().intensity = d;
}
void BasicSpectrum::setPrecursorIon(sPrecursorIon& p){ vPrecursor->push_back(p);}
void BasicSpectrum::setPrecursorMZ(double d) {
  if (vPrecursor->size() == 0) vPrecursor->emplace_back();
  vPrecursor->back().mz = d;
}
void BasicSpectrum::setPrecursorScanNum(int i){ 
  if (vPrecursor->size() == 0) vPrecursor->emplace_back();
  vPrecursor->back().scanNumber=i;
  precursorScanNum=i;
}
void BasicSpectrum::setRTime(float f){ rTime=f;}
void BasicSpectrum::setScanIndex(int num) { scanIndex=num;}
void BasicSpectrum::setScanNum(int num){scanNum=num;}
void BasicSpectrum::setTotalIonCurrent(double d){ totalIonCurrent=d;}
void BasicSpectrum::setUserParam(string name, string value, string type){
  vUserParam->emplace_back();
  vUserParam->back().name=name;
  vUserParam->back().value=value;
  vUserParam->back().type=type;
}

//------------------------------------------
//  Accessors
//------------------------------------------
int BasicSpectrum::getActivation(){ return activation;}
double BasicSpectrum::getBasePeakIntensity(){ return basePeakIntensity;}
double BasicSpectrum::getBasePeakMZ(){ return basePeakMZ;}
bool BasicSpectrum::getCentroid(){ return centroid;}
double BasicSpectrum::getCollisionEnergy(){ return collisionEnergy;}
double BasicSpectrum::getCompensationVoltage(){ return compensationVoltage;}
double BasicSpectrum::getInverseReducedIonMobility() { return inverseReducedIonMobility; }
specIonMobDP& BasicSpectrum::getIonMobDP(const size_t& index) { return vDataIonMob->at(index); }
bool BasicSpectrum::getIonMobilityScan() { return ionMobilityScan;}
void BasicSpectrum::getFilterLine(string& str) { str=filterLine;}
double BasicSpectrum::getHighMZ(){ return highMZ;}
void BasicSpectrum::getIDString(string& str) { str=idString; }
double BasicSpectrum::getIonInjectionTime(){return ionInjectionTime;}
double BasicSpectrum::getIonMobilityDriftTime(){return ionMobilityDriftTime;}
double BasicSpectrum::getLowMZ(){ return lowMZ;}
int BasicSpectrum::getMSLevel(){ return msLevel;}
int BasicSpectrum::getPeaksCount(){ return peaksCount;}
bool BasicSpectrum::getPositiveScan(){ return positiveScan;}
int BasicSpectrum::getPrecursorCharge(int i){ //legacy function. Always returns first charge of requested precursor, or first charge of first precursor
  if (vPrecursor->size()==0) return 0;
  if (i>=vPrecursor->size()) return 0;
  return vPrecursor->at(i).charge;
}
double BasicSpectrum::getPrecursorMZ(int i){ //legacy function. Always returns first precursor mz
  if (vPrecursor->size()==0) return 0;
  if (i >= vPrecursor->size()) return 0;
  return vPrecursor->at(i).mz;
}
sPrecursorIon BasicSpectrum::getPrecursorIon(int i){ return vPrecursor->at(i); }
int BasicSpectrum::getPrecursorIonCount() { return (int)vPrecursor->size(); }
int BasicSpectrum::getPrecursorScanNum(){ 
  if(vPrecursor->size()==0) return precursorScanNum;
  return vPrecursor->at(0).scanNumber;
}
float BasicSpectrum::getRTime(bool min){
  if(min) return rTime;
  else return rTime*60;
}
int BasicSpectrum::getScanIndex(){ return scanIndex;}
int BasicSpectrum::getScanNum(){ return scanNum;}
double BasicSpectrum::getTotalIonCurrent(){ return totalIonCurrent;}
sUParam BasicSpectrum::getUserParam(const size_t& index) {
  if(index>=vUserParam->size()) return sUParam();
  return vUserParam->at(index);
}
size_t BasicSpectrum::size(){ 
  if(vDataIonMob->size()>0) return vDataIonMob->size(); //ion mobility data and standard data should not exist together.
  return vData->size();
}

