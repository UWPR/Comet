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
#ifndef _NO_THERMORAW
#include "RAWReader.h"
#include <msclr/marshal_cppstd.h>
#include <vcclr.h>

using namespace std;
using namespace MSToolkit;
using namespace System;
using namespace System::Globalization;
using namespace ThermoFisher::CommonCore::Data::Business;
using namespace ThermoFisher::CommonCore::Data::FilterEnums;
using namespace ThermoFisher::CommonCore::Data::Interfaces;
using namespace ThermoFisher::CommonCore::RawFileReader;

namespace MSToolkit {
  // The real managed handle. Only ever touched from this /clr-compiled file; RAWReader.h (seen
  // by plain-native translation units throughout MSToolkit/CometSearch) only ever sees the
  // opaque forward declaration. A plain native struct can't hold a '^' tracking handle directly
  // (C2365/C3265) -- gcroot<T> is vcclr.h's wrapper for exactly this: a managed handle stored as
  // a member of an unmanaged type.
  struct RAWReaderImpl {
    gcroot<IRawDataPlus^> rawFile;
    RAWReaderImpl() : rawFile(nullptr) {}
  };
}

namespace {

  int MsOrderToLevel(MSOrderType order){
    switch(order){
      case MSOrderType::Ms:  return 1;
      case MSOrderType::Ms2: return 2;
      case MSOrderType::Ms3: return 3;
      case MSOrderType::Ms4: return 4;
      case MSOrderType::Ms5: return 5;
      default: return 0;
    }
  }

  // RawFileReader has no structured per-scan equivalent of legacy's filter-derived activation
  // method, supplemental-activation flag, or compensation voltage -- IScanEventBase::Activation/
  // SupplementalActivation/CompensationVoltage are *filter-matching rule* settings (their type is
  // TriState/CompensationVoltageType, "is this filter criterion on/off/any"), not the actual
  // per-scan value. So, exactly as the COM-era evaluateFilter() did, get these (plus the same
  // handful of scan types MSOrderType alone can't distinguish: multiplexed MS2, zoom/ultra-zoom,
  // SRM) by tokenizing the human-readable scan event string -- its format is unchanged regardless
  // of which Thermo API reads it. Same left-to-right precedence as legacy (a "msx" token wins over
  // a later "ms2"/"ms3" token; "u" wins over a later "Z"; "sa" must appear before the "<mz>@<act>"
  // token it modifies). Also collects the precursor m/z value(s) embedded in "<mz>@<act>" tokens,
  // needed only for the MSX/SRM fallback paths (MS1/MS2/MS3 get precursor info from GetReaction()).
  MSSpectrumType EvaluateScanTokens(int msOrderLevel, String^ eventStr, vector<double>& atTokenMZs, double& cv, MSActivation& act){
    MSSpectrumType mst = Unspecified;
    switch(msOrderLevel){
      case 1: mst=MS1; break;
      case 2: mst=MS2; break;
      case 3: mst=MS3; break;
      default: break;
    }

    atTokenMZs.clear();
    cv=0;
    act=mstNA;
    bool bSA=false;

    string filterStr = msclr::interop::marshal_as<std::string>(eventStr);
    char cStr[1024];
    strncpy(cStr, filterStr.c_str(), sizeof(cStr)-1);
    cStr[sizeof(cStr)-1]='\0';

    char* nextTok;
    char* tok = strtok_r(cStr," \n",&nextTok);
    while(tok!=NULL){
      if(strlen(tok)>2 && tok[0]=='c' && tok[1]=='v'){
        cv=atof(tok+3);
      } else if(strcmp(tok,"ms")==0){
        mst=MS1;
      } else if(strcmp(tok,"msx")==0){
        mst=MSX;
      } else if(strcmp(tok,"ms2")==0){
        if(mst!=MSX) mst=MS2;
      } else if(strcmp(tok,"ms3")==0){
        if(mst!=MSX) mst=MS3;
      } else if(strcmp(tok,"sa")==0){
        bSA=true;
      } else if(strcmp(tok,"SRM")==0){
        mst=SRM;
      } else if(strcmp(tok,"u")==0){
        mst=UZS;
      } else if(strcmp(tok,"Z")==0){
        if(mst!=UZS) mst=ZS;
      } else if(strchr(tok,'@')!=NULL){
        string tStr=tok;
        size_t stop=tStr.find('@');
        atTokenMZs.push_back(atof(tStr.substr(0,stop).c_str()));
        string actStr=tStr.substr(stop+1,3);
        if(actStr=="cid") act=mstCID;
        else if(actStr=="etd") act = bSA ? mstETDSA : mstETD;
        else if(actStr=="hcd") act=mstHCD;
        else act=mstNA;
      }
      tok=strtok_r(NULL," \n",&nextTok);
    }
    return mst;
  }

  struct TrailerFields {
    int parScanNum;
    int charge;
    double monoMz;
    double ionInjectionTimeMs;
    string spsMasses;
    bool hasSpsMasses;
    string scanDescription;
    bool hasScanDescription;
    TrailerFields() : parScanNum(0), charge(0), monoMz(0.0), ionInjectionTimeMs(0.0), hasSpsMasses(false), hasScanDescription(false) {}
  };

  TrailerFields ReadTrailerFields(IRawDataPlus^ rawFile, int scanNum){
    TrailerFields f;
    LogEntry^ trailer = rawFile->GetTrailerExtraInformation(scanNum);
    for(int i=0; i<trailer->Length; i++){
      String^ label = trailer->Labels[i];
      String^ value = trailer->Values[i];
      double d; int n;
      if(label=="Master Scan Number:"){ if(Int32::TryParse(value, n)) f.parScanNum=n; }
      else if(label=="Charge State:"){ if(Int32::TryParse(value, n)) f.charge=n; }
      else if(label=="Monoisotopic M/Z:"){ if(Double::TryParse(value, NumberStyles::Float, CultureInfo::InvariantCulture, d)) f.monoMz=d; }
      else if(label=="Ion Injection Time (ms):"){ if(Double::TryParse(value, NumberStyles::Float, CultureInfo::InvariantCulture, d)) f.ionInjectionTimeMs=d; }
      else if(label=="SPS Masses:"){ f.spsMasses=msclr::interop::marshal_as<std::string>(value); f.hasSpsMasses=true; }
      else if(label=="Scan Description:"){ f.scanDescription=msclr::interop::marshal_as<std::string>(value); f.hasScanDescription=true; }
    }
    return f;
  }

} //anonymous namespace

// ==========================
// Constructors & Destructors
// ==========================
RAWReader::RAWReader(){

  pImpl = new RAWReaderImpl();
	bRaw = initRaw();
  rawCurSpec=0;
  rawTotSpec=0;
  rawAvg=false;
  rawAvgWidth=1;
  rawAvgCutoff=1000;
	rawFileOpen=false;
  rawLabel=false;
  rawUserFilterExact=true;
  strcpy(rawCurrentFile,".");
  strcpy(rawInstrument,"unknown");
  strcpy(rawManufacturer,"Thermo Scientific");
  strcpy(rawUserFilter,"");
	msLevelFilter=NULL;

}

RAWReader::~RAWReader(){

  if(rawFileOpen){
    IRawDataPlus^ rf = pImpl->rawFile;
    delete rf;
  }
  delete pImpl;
  pImpl=NULL;
	msLevelFilter=NULL;

}

int RAWReader::calcChargeState(double precursormz, double highmass, const double* masses, const double* intensities, long nArraySize) {
// Assumes spectrum is +1 or +2.  Figures out charge by
// seeing if signal is present above the parent mass
// indicating +2 (by taking ratio above/below precursor)

	bool bFound;
	long i, iStart;
	double dLeftSum,dRightSum;
	double FractionWindow;
	double CorrectionFactor;

	dLeftSum = 0.00001;
	dRightSum = 0.00001;

//-------------
// calc charge
//-------------
	bFound=false;
	i=0;
	while(i<nArraySize && !bFound){
    if(masses[i] < precursormz - 20){
			//do nothing
		} else {
			bFound = true;
      iStart = i;
    }
    i++;
	}
	if(!bFound) iStart = nArraySize;

	for(i=0;i<iStart;i++)	dLeftSum = dLeftSum + intensities[i];

	bFound=false;
	i=0;
	while(i<nArraySize && !bFound){
    if(masses[i] < precursormz + 20){
			//do nothing
		} else {
      bFound = true;
      iStart = i;
    }
    i++;
	}

	if(!bFound) {
		return 1;
	}
	if(iStart = 0) iStart++;

	for(i=iStart;i<nArraySize;i++) dRightSum = dRightSum + intensities[i];

	if(precursormz * 2 < highmass){
    CorrectionFactor = 1;
	} else {
    FractionWindow = (precursormz * 2) - highmass;
    CorrectionFactor = (precursormz - FractionWindow) / precursormz;
	}

	if(dLeftSum > 0 && (dRightSum / dLeftSum) < (0.2 * CorrectionFactor)){
		return 1;
	} else {
    return 0;  //Set charge to 0 to indicate that both +2 and +3 spectra should be created
	}

  //When all else fails, return 0
  return 0;
}

void RAWReader::getInstrument(string& str) {
  str=rawInstrument;
}

long RAWReader::getLastScanNumber(){
	return rawCurSpec;
}

void RAWReader::getManufacturer(string& str) {
  str=rawManufacturer;
}

long RAWReader::getScanCount(){
	return rawTotSpec;
}

bool RAWReader::getStatus(){
	return bRaw;
}

bool RAWReader::initRaw(){
  //RawFileReader .NET has no separate-install/COM-registration step (unlike the old MSFileReader
  //COM library this replaces) -- the managed assemblies are referenced directly by the build, so
  //there is nothing to probe for here. Kept returning true for interface compatibility with
  //getStatus(), which callers use to check "can this build read .raw files at all".
  return true;
}

bool RAWReader::readRawFile(const char *c, Spectrum &s, int scNum){

	bool bNewFile=false;

  if(!bRaw) return false;

	//Clear spectrum object
  s.clear();

  if(c==NULL){
		//continue reading current file
    if(!rawFileOpen) return false;
    if(scNum<0) rawCurSpec--;
    else if(scNum>0) rawCurSpec=scNum;
    else rawCurSpec++;
    if(rawCurSpec<1) return false;
    if(rawCurSpec>rawTotSpec) return false;
  } else {

    //check if requested file is already open
    if(rawFileOpen) {
      if(strcmp(c,rawCurrentFile)==0){
        if (scNum<0) rawCurSpec--;
        else if(scNum>0) rawCurSpec=scNum;
        else rawCurSpec++;
        if (rawCurSpec<1) return false;
        if(rawCurSpec>rawTotSpec) return false;
      } else {
        //new file requested, so close the existing one
        IRawDataPlus^ oldRf = pImpl->rawFile;
        delete oldRf;
        pImpl->rawFile=nullptr;
        rawFileOpen=false;
        bNewFile=true;
      }
    } else {
      bNewFile=true;
    }

    if(bNewFile){
      IRawDataPlus^ rf = RawFileReaderAdapter::FileFactory(gcnew String(c));
      if(!rf->IsOpen || rf->IsError){
        cerr << "Cannot open " << c << endl;
        return false;
      }
      rf->SelectInstrument(Device::MS, 1);
      pImpl->rawFile = rf;
      rawFileOpen=true;
      rawTotSpec = rf->RunHeaderEx->LastSpectrum;
      strcpy(rawInstrument, msclr::interop::marshal_as<std::string>(rf->GetInstrumentData()->Model).c_str());
      strcpy(rawCurrentFile,c);

			//if scan number is requested, grab it
      if(scNum<0) return false;
      if(scNum>0) rawCurSpec=scNum;
      else rawCurSpec=1;
      if(rawCurSpec>rawTotSpec) return false;
    }
  }

  IRawDataPlus^ rawFile = pImpl->rawFile;

  rawPrecursorInfo preInfo;
  vector<double> atTokenMZs;
  MSSpectrumType MSn;
  int msLevel;
  double cv;
  MSActivation act;

	//Rather than grab the next scan number, get the next scan based on a user-filter (if supplied).
  //if the filter was set, make sure we pass the filter
  while(true){

    IScanEvent^ peekEvent = rawFile->GetScanEventForScanNumber((int)rawCurSpec);
    msLevel = MsOrderToLevel(peekEvent->MSOrder);
    String^ eventStr = rawFile->GetScanEventStringForScanNumber((int)rawCurSpec);
    MSn = EvaluateScanTokens(msLevel, eventStr, atTokenMZs, cv, act);

    //check for spectrum filter (string)
    if(strlen(rawUserFilter)>0){
      bool bCheckNext=false;
      string curFilter = msclr::interop::marshal_as<std::string>(eventStr);
      if(rawUserFilterExact) {
        if(curFilter.compare(rawUserFilter)!=0) bCheckNext=true;
      } else {
        if(curFilter.find(rawUserFilter)==string::npos) bCheckNext=true;
      }

      //if string doesn't match, get next scan until it does match or EOF
      if(bCheckNext){
        if(scNum<0) rawCurSpec--;
        else if(scNum>0) return false;
        else rawCurSpec++;
        if(rawCurSpec<1) return false;
        if(rawCurSpec>rawTotSpec) return false;
        continue;
      }
    }

    //check for msLevel filter
    if(msLevelFilter->size()>0 && find(msLevelFilter->begin(), msLevelFilter->end(), MSn) == msLevelFilter->end()) {
      if (scNum<0) rawCurSpec--;
      else if(scNum>0) return false;
      else rawCurSpec++;
      if (rawCurSpec<1) return false;
      if(rawCurSpec>rawTotSpec) return false;
    } else {
      break;
    }
  }

  IScanEvent^ scanEvent = rawFile->GetScanEventForScanNumber((int)rawCurSpec);
  ScanStatistics^ stats = rawFile->GetScanStatsForScanNumber((int)rawCurSpec);
  double dRTime = rawFile->RetentionTimeFromScanNumber((int)rawCurSpec);

  //Get basic spectrum metadata. It will be replaced/supplemented later, if available
  preInfo.clear();
  if(msLevel>=2){
    TrailerFields trailer = ReadTrailerFields(rawFile, (int)rawCurSpec);
    preInfo.parScanNum = trailer.parScanNum;
    preInfo.charge = trailer.charge;
    preInfo.dMonoMZ = trailer.monoMz;
    float IIT = (float)trailer.ionInjectionTimeMs;

    if(trailer.hasSpsMasses){
      string SPS = trailer.spsMasses;
      size_t a=0;
      string tStr;
      while(a<SPS.size()){
        if(SPS[a]==','){
          if(!tStr.empty()) {
            double spsd=atof(tStr.c_str());
            if(spsd>0) s.addSPS(spsd);
            tStr.clear();
          }
          a++;
          continue;
        }
        tStr+=SPS[a++];
      }
      if (!tStr.empty()) {
        double spsd = atof(tStr.c_str());
        if (spsd > 0) s.addSPS(spsd);
      }
    }

    if(trailer.hasScanDescription)
      s.setScanDescription(trailer.scanDescription);

    s.setIonInjectionTime(IIT);

    //Immediate precursor transition: reaction(0) is the MS1->MS2 isolation, reaction(1) is the
    //MS2->MS3 isolation, etc. -- mirrors the legacy GetPrecursorMassForScanNum(scan, msLevel, ...)
    //call, which indexed by MS order the same way.
    IReaction^ reaction = scanEvent->GetReaction(msLevel-2);
    preInfo.dIsoMZ = reaction->PrecursorMass;

    //Correct precursor mono mass if it is more than 5 13C atoms away from isolation mass
    if (preInfo.dMonoMZ > 0 && preInfo.charge>0){
      double td = preInfo.dIsoMZ-preInfo.dMonoMZ;
      if (td>5.01675/preInfo.charge) preInfo.dMonoMZ=preInfo.dIsoMZ;
    }
  }

  //Get the peaks. setAverageRaw() (which would set rawAvg=true) is never called anywhere in
  //this codebase -- the averaged-scan path it would select here was already unreachable before
  //this rewrite, so it isn't reimplemented against RawFileReader's AverageScans() API; this just
  //falls through to the normal single-scan retrieval, same as rawAvg==false.
  // GetCentroidStream gives the instrument's saved centroid peaks (the representation Comet's
  // correlation scoring actually wants); this is available for FT/Orbitrap scans regardless of
  // whether the scan's *primary* stored format is profile (IsCentroidScan/SpectrumPacketType are
  // not reliable predictors of centroid-stream availability in practice -- confirmed against real
  // data during Phase 0/1 validation). Scans with no saved centroid stream at all (e.g. true
  // profile-only data from non-FT instruments) throw, and fall back to the profile SegmentedScan.
  cli::array<double>^ peakMasses;
  cli::array<double>^ peakIntensities;
  int numPeaks;
  bool gotCentroid=false;
  try{
    CentroidStream^ centroid = rawFile->GetCentroidStream((int)rawCurSpec, false);
    if(centroid->Length>0){
      peakMasses = centroid->Masses;
      peakIntensities = centroid->Intensities;
      numPeaks = centroid->Length;
      gotCentroid=true;
    }
  } catch(Exception^){
    //no centroid stream for this scan; fall through to the profile path below
  }
  if(!gotCentroid){
    SegmentedScan^ segScan = rawFile->GetSegmentedScanFromScanNumber((int)rawCurSpec, stats);
    peakMasses = segScan->Positions;
    peakIntensities = segScan->Intensities;
    numPeaks = segScan->PositionCount;
  }
  bool bCentroid = gotCentroid;

  //Marshal the managed peak arrays into native buffers once, up front (mirrors the SAFEARRAY
  //copy the COM path used to do), so the rest of this function works with plain native data.
  vector<double> masses(numPeaks), intensities(numPeaks);
  for(int p=0; p<numPeaks; p++){
    masses[p]=peakMasses[p];
    intensities[p]=peakIntensities[p];
  }

	//Handle MS2 and MS3 files differently to create Z-lines
	if(MSn==MS2 || MSn==MS3){

    MSPrecursorInfo pi;
    pi.charge = preInfo.charge;
    pi.monoMz = preInfo.dMonoMZ;
    pi.mz = preInfo.dIsoMZ;
    pi.isoMz = preInfo.dIsoMZ;
    pi.precursorScanNumber = preInfo.parScanNum;
    s.addPrecursor(pi);

		//if charge state is assigned to spectrum, add Z-lines.
    double isoMz = preInfo.dIsoMZ;
    double pm1;
		if(preInfo.charge>0){
			if(preInfo.dMonoMZ>0.01) {
        pm1 = preInfo.dMonoMZ * preInfo.charge - ((preInfo.charge - 1)*1.007276466);
        s.addMZ(isoMz, preInfo.dMonoMZ);
			}	else {
        pm1 = isoMz * preInfo.charge - ((preInfo.charge - 1)*1.007276466);
        s.addMZ(isoMz);
			}
      s.addZState(preInfo.charge, pm1);
      s.setCharge(preInfo.charge);
    } else {
      s.addMZ(isoMz);
      int charge = calcChargeState(isoMz, 0.0, masses.data(), intensities.data(), (long)numPeaks);

      //Charge greater than 0 means the charge state is known
      if(charge>0){
        pm1 = isoMz*charge - ((charge-1)*1.007276466);
  	    s.addZState(charge,pm1);

      //Charge of 0 means unknown charge state, therefore, compute +2 and +3 states.
      } else {
        pm1 = isoMz*2 - 1.007276466;
        s.addZState(2,pm1);
        pm1 = isoMz*3 - 2*1.007276466;
        s.addZState(3,pm1);
      }

    }

  } //endif MS2 and MS3

	if(MSn==MSX){
		for(size_t i=0;i<atTokenMZs.size();i++){
			if(i==0) s.setMZ(atTokenMZs[i],0);
			else s.addMZ(atTokenMZs[i],0);
		}
		s.setCharge(0);
	}

	//Set basic scan info
  if(bCentroid) s.setCentroidStatus(1);
  else s.setCentroidStatus(0);
  s.setActivationMethod(act);
	s.setScanNumber((int)rawCurSpec);
  s.setScanNumber((int)rawCurSpec,true);
	s.setRTime((float)dRTime);
	s.setFileType(MSn);
  s.setBPI((float)stats->BasePeakIntensity);
  s.setBPM(stats->BasePeakMass);
  s.setCompensationVoltage(cv);
  s.setTIC(stats->TIC);
  string ts="scan=";
  ts+=to_string(rawCurSpec);
  s.setNativeID(ts.c_str());
	if(MSn==SRM && atTokenMZs.size()>0) s.setMZ(atTokenMZs[0]);
  switch(MSn){
    case MS1: s.setMsLevel(1); break;
    case MS2: s.setMsLevel(2); break;
    case MS3: s.setMsLevel(3); break;
		case MSX: s.setMsLevel(2); break;
    default: s.setMsLevel(0); break;
  }

  for(int p=0; p<numPeaks; p++) s.add(masses[p],(float)intensities[p]);

  return true;

}

void RAWReader::setMSLevelFilter(vector<MSSpectrumType>* v){
	msLevelFilter=v;
}

void RAWReader::setRawFilter(char *c){
  strcpy(rawUserFilter,c);
}

#endif
