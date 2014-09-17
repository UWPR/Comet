#include "RAWReader.h"

using namespace MSToolkit;

// ==========================
// Constructors & Destructors
// ==========================
RAWReader::RAWReader(){

  CoInitialize( NULL );
	bRaw = initRaw();
  rawCurSpec=0;
  rawTotSpec=0;
  rawAvg=false;
  rawAvgWidth=1;
  rawAvgCutoff=1000;
	rawFileOpen=false;
  rawLabel=false;
  rawUserFilterExact=true;
  strcpy(rawUserFilter,"");
	msLevelFilter=NULL;

}

RAWReader::~RAWReader(){

  if(bRaw){
    if(rawFileOpen) m_Raw->Close();
    m_Raw.Release();
    m_Raw=NULL;
  }
	msLevelFilter=NULL;

}

int RAWReader::calcChargeState(double precursormz, double highmass, VARIANT* varMassList, long nArraySize) {
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

	DataPeak* pDataPeaks = NULL;
	SAFEARRAY FAR* psa = varMassList->parray;
	SafeArrayAccessData( psa, (void**)(&pDataPeaks) );

//-------------
// calc charge
//-------------
	bFound=false;
	i=0;
	while(i<nArraySize && !bFound){
    if(pDataPeaks[i].dMass < precursormz - 20){
			//do nothing
		} else {
			bFound = true;
      iStart = i;
    }
    i++;
	}
	if(!bFound) iStart = nArraySize;

	for(i=0;i<iStart;i++)	dLeftSum = dLeftSum + pDataPeaks[i].dIntensity;

	bFound=false;
	i=0;
	while(i<nArraySize && !bFound){
    if(pDataPeaks[i].dMass < precursormz + 20){
			//do nothing
		} else {
      bFound = true;
      iStart = i;
    }
    i++;
	}

	if(!bFound) {
		SafeArrayUnaccessData( psa );
		psa = NULL;
		pDataPeaks = NULL;
		return 1;
	}
	if(iStart = 0) iStart++;

	for(i=iStart;i<nArraySize;i++) dRightSum = dRightSum + pDataPeaks[i].dIntensity;

	if(precursormz * 2 < highmass){
    CorrectionFactor = 1;
	} else {
    FractionWindow = (precursormz * 2) - highmass;
    CorrectionFactor = (precursormz - FractionWindow) / precursormz;
	}

	if(dLeftSum > 0 && (dRightSum / dLeftSum) < (0.2 * CorrectionFactor)){
		SafeArrayUnaccessData( psa );
		psa=NULL;
		pDataPeaks=NULL;
		return 1;
	} else {
		SafeArrayUnaccessData( psa );
		psa=NULL;
		pDataPeaks=NULL;
    return 0;  //Set charge to 0 to indicate that both +2 and +3 spectra should be created
	}

  //When all else fails, return 0
  return 0;
}

MSSpectrumType RAWReader::evaluateFilter(long scan, char* chFilter, vector<double>& MZs, bool& bCentroid, double& cv) {

  BSTR Filter = NULL;
	char cStr[256];
	string tStr;
	string mzVal;
	int stop;

	//For non-ATL and non-MFC conversions
	int sl;

  //Initialize raw values to default
	MZs.clear();
  cv=0;

  m_Raw->GetFilterForScanNum(scan, &Filter);
	sl = SysStringLen(Filter)+1;
	WideCharToMultiByte(CP_ACP,0,Filter,-1,chFilter,sl,NULL,NULL);
	SysFreeString(Filter);

	strcpy(cStr,chFilter);
	MSSpectrumType mst=Unspecified;
	char* tok;
	tok=strtok(cStr," \n");
	while(tok!=NULL){

		if(strcmp(tok,"c")==0){
      bCentroid=true;
		} else if(strlen(tok)>2 && tok[0]=='c' && tok[1]=='v'){
      cv=atof(tok+3);
		} else if(strcmp(tok,"d")==0){
		} else if(strcmp(tok,"ESI")==0){
		} else if(strcmp(tok,"FTMS")==0){
		} else if(strcmp(tok,"Full")==0){
		} else if(strcmp(tok,"ITMS")==0){
		} else if(strcmp(tok,"lock")==0){
		} else if(strcmp(tok,"ms")==0){
			mst=MS1;
		} else if(strcmp(tok,"msx")==0){
			mst=MSX;
		} else if(strcmp(tok,"ms2")==0){
			if(mst!=MSX) mst=MS2;
		} else if(strcmp(tok,"ms3")==0){
			if(mst!=MSX) mst=MS3;
		} else if(strcmp(tok,"NSI")==0){
		} else if(strcmp(tok,"p")==0){
      bCentroid=false;
		} else if(strncmp(tok,"sid",3)==0){
		} else if(strcmp(tok,"SRM")==0){
			mst=SRM;
		} else if(strcmp(tok,"u")==0){
			mst=UZS;
		} else if(strcmp(tok,"w")==0){ //wideband activation?
		} else if(strcmp(tok,"Z")==0){
			if(mst!=UZS) mst=ZS;
		} else if(strcmp(tok,"+")==0){
		} else if(strcmp(tok,"-")==0){
		} else if(strchr(tok,'@')!=NULL){
			tStr=tok;
			stop=tStr.find("@");
			mzVal=tStr.substr(0,stop);
			MZs.push_back(atof(&mzVal[0]));

		} else if(strchr(tok,'[')!=NULL){
		} else {
			cout << "Unknown token: " << tok << endl;
		}

		tok=strtok(NULL," \n");
	}

	return mst;

}

long RAWReader::getLastScanNumber(){
	return rawCurSpec;
}

long RAWReader::getScanCount(){
	return rawTotSpec;
}

bool RAWReader::getStatus(){
	return bRaw;
}

bool RAWReader::initRaw(){

	int raw=0;

	IXRawfile2Ptr m_Raw2;
	IXRawfile3Ptr m_Raw3;
	IXRawfile4Ptr m_Raw4;
	IXRawfile5Ptr m_Raw5;

	//Example of Xcalibur/Foundation first
	//if(FAILED(m_Raw5.CreateInstance("XRawfile.XRawfile.1"))){

	//Try MSFileReader - using ProteoWizard strategy
	if(FAILED(m_Raw5.CreateInstance("MSFileReader.XRawfile.1"))){
		if(FAILED(m_Raw4.CreateInstance("MSFileReader.XRawfile.1"))){
			if(FAILED(m_Raw3.CreateInstance("MSFileReader.XRawfile.1"))){
				if(FAILED(m_Raw2.CreateInstance("MSFileReader.XRawfile.1"))){
					if(FAILED(m_Raw.CreateInstance("MSFileReader.XRawfile.1"))){
// suppress warning
//						cout << "Cannot load Thermo MSFileReader. Cannot read .RAW files." << endl;
					} else {
						raw=1;
					}
				} else {
					m_Raw=m_Raw2;
					raw=2;
				}
			} else {
				m_Raw=m_Raw3;
				raw=3;
			}
		} else {
			m_Raw=m_Raw4;
			raw=4;
		}
	} else {
		m_Raw=m_Raw5;
		raw=5;
	}
	
	if(raw>0) return true;
	return false;
}

bool RAWReader::readRawFile(const char *c, Spectrum &s, int scNum){

	//General purpose function members
	bool bCheckNext;

	char chFilter[256];
  char curFilter[256];
	
	double dRTime;
	double highmass=0.0;
	double pm1;
	double pw;

	long i;
  long j;
	long lArraySize=0;

	vector<double> MZs;

	DataPeak* pDataPeaks = NULL;
  HRESULT lRet;
	MSSpectrumType MSn;
	SAFEARRAY FAR* psa;
  TCHAR pth[MAX_PATH];
  VARIANT varMassList;
	VARIANT varPeakFlags;

  //Members for gathering averaged scans
	int charge;
  int sl;
	int widthCount;
	
	long FirstBkg1=0;
	long FirstBkg2=0;
  long LastBkg1=0;
  long LastBkg2=0;
	long lowerBound;
  long upperBound;
    
  BSTR rawFilter=NULL;
  BSTR testStr;

	//Additional members for Scan Information
  bool bCentroid;

  double cv;    //Compensation Voltage
  double BPI;   //Base peak intensity
	double BPM;   //Base peak mass
	double td;    //temp double value
	double TIC;

	long tl;      //temp long value

	VARIANT Charge;
	VARIANT ConversionA;
  VARIANT ConversionB;
	VARIANT ConversionC;
  VARIANT ConversionD;
	VARIANT ConversionE;
  VARIANT ConversionI;
  VARIANT IIT;  //ion injection time
	VARIANT MonoMZ;

	//Clear spectrum object
  s.clear();

  if(c==NULL){
		//if file is closed and scan number requested, open file and grab scan number
    if(scNum>0) rawCurSpec=scNum;
    else rawCurSpec++;
    if(rawCurSpec>rawTotSpec) return false;

  } else {
		//close an open file, open the requested file.
    if(rawFileOpen) {
      lRet = m_Raw->Close();
      rawFileOpen=false;
    }
    MultiByteToWideChar(CP_ACP,0,c,-1,(LPWSTR)pth,MAX_PATH);
    lRet = m_Raw->Open((LPWSTR)pth);
		if(lRet != ERROR_SUCCESS) {
			cerr << "Cannot open " << c << endl;
			return false;
		}
	  else lRet = m_Raw->SetCurrentController(0,1);
    rawFileOpen=true;
    m_Raw->GetNumSpectra(&rawTotSpec);

		//if scan number is requested, grab it
    if(scNum>0) rawCurSpec=scNum;
    else rawCurSpec=1;
    if(rawCurSpec>rawTotSpec) return false;
  }

	//Initialize members
	strcpy(chFilter,"");
  strcpy(curFilter,"");
	VariantInit(&varMassList);
	VariantInit(&varPeakFlags);
	VariantInit(&IIT);
  VariantInit(&ConversionA);
  VariantInit(&ConversionB);
	VariantInit(&ConversionC);
  VariantInit(&ConversionD);
	VariantInit(&ConversionE);
  VariantInit(&ConversionI);
	VariantInit(&Charge);
	VariantInit(&MonoMZ);

	//Rather than grab the next scan number, get the next scan based on a user-filter (if supplied).
  //if the filter was set, make sure we pass the filter
  while(true){

	  MSn = evaluateFilter(rawCurSpec, curFilter, MZs, bCentroid,cv);

    //check for spectrum filter (string)
    if(strlen(rawUserFilter)>0){
      bCheckNext=false;
      if(rawUserFilterExact) {
        if(strcmp(curFilter,rawUserFilter)!=0) bCheckNext=true;
      } else {
        if(strstr(curFilter,rawUserFilter)==NULL) bCheckNext=true;
      }

      //if string doesn't match, get next scan until it does match or EOF
      if(bCheckNext){
        if(scNum>0) return false;
        rawCurSpec++;
        if(rawCurSpec>rawTotSpec) return false;
        continue;
      }
    }

    //check for msLevel filter
    if(msLevelFilter->size()>0 && find(msLevelFilter->begin(), msLevelFilter->end(), MSn) == msLevelFilter->end()) {
      if(scNum>0) return false;
      rawCurSpec++;
      if(rawCurSpec>rawTotSpec) return false;
    } else {
      break;
    }
  }

	//Get spectrum meta data  
	sl=lstrlenA("Monoisotopic M/Z:");
	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Monoisotopic M/Z:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &MonoMZ);
	SysFreeString(testStr);

	sl=lstrlenA("Charge State:");
	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Charge State:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &Charge);
	SysFreeString(testStr);

	sl=lstrlenA("Ion Injection Time (ms):");
  testStr = SysAllocStringLen(NULL,sl);
  MultiByteToWideChar(CP_ACP,0,"Ion Injection Time (ms):",sl,testStr,sl);
  m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &IIT);
  SysFreeString(testStr);

	sl=lstrlenA("Conversion Parameter A:");
	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Conversion Parameter A:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &ConversionA);
	SysFreeString(testStr);

	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Conversion Parameter B:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &ConversionB);
	SysFreeString(testStr);

	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Conversion Parameter C:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &ConversionC);
	SysFreeString(testStr);

	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Conversion Parameter D:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &ConversionD);
	SysFreeString(testStr);

	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Conversion Parameter E:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &ConversionE);
	SysFreeString(testStr);

	testStr = SysAllocStringLen(NULL,sl);
	MultiByteToWideChar(CP_ACP,0,"Conversion Parameter I:",sl,testStr,sl);
	m_Raw->GetTrailerExtraValueForScanNum(rawCurSpec, testStr, &ConversionI);
	SysFreeString(testStr);

  m_Raw->GetScanHeaderInfoForScanNum(rawCurSpec, &tl, &td, &td, &td, &TIC, &BPM, &BPI, &tl, &tl, &td);
  m_Raw->RTFromScanNum(rawCurSpec,&dRTime);

  //Get the peaks
	//Average raw files if requested by user
  if(rawAvg){
    widthCount=0;
    lowerBound=0;
    upperBound=0;
    for(i=rawCurSpec-1;i>0;i--){
      evaluateFilter(i, chFilter, MZs, bCentroid,cv);
      if(strcmp(curFilter,chFilter)==0){
        widthCount++;
        if(widthCount==rawAvgWidth) {
          lowerBound=i;
          break;
        }
      }
    }
    if(lowerBound==0) lowerBound=rawCurSpec; //this will have "edge" effects

    widthCount=0;
    for(i=rawCurSpec+1;i<rawTotSpec;i++){
      evaluateFilter(i, chFilter, MZs, bCentroid,cv);
      if(strcmp(curFilter,chFilter)==0){
        widthCount++;
        if(widthCount==rawAvgWidth) {
          upperBound=i;
          break;
        }
      }
    }
    if(upperBound==0) upperBound=rawCurSpec; //this will have "edge" effects

    m_Raw->GetFilterForScanNum(i, &rawFilter);
    j=m_Raw->GetAverageMassList(&lowerBound, &upperBound, &FirstBkg1, &LastBkg1, &FirstBkg2, &LastBkg2,
      rawFilter, 1, rawAvgCutoff, 0, FALSE, &pw, &varMassList, &varPeakFlags, &lArraySize );
    SysFreeString(rawFilter);
    rawFilter=NULL;

  } else {

		//Get regular spectrum data
		sl=lstrlenA("");
		testStr = SysAllocStringLen(NULL,sl);
		MultiByteToWideChar(CP_ACP,0,"",sl,testStr,sl);
		j=m_Raw->GetMassListFromScanNum(&rawCurSpec,testStr,0,0,0,FALSE,&pw,&varMassList,&varPeakFlags,&lArraySize);
		SysFreeString(testStr);
  }

	//Handle MS2 and MS3 files differently to create Z-lines
	if(MSn==MS2 || MSn==MS3){

		//if charge state is assigned to spectrum, add Z-lines.
		if(Charge.iVal>0){
			if(MonoMZ.dblVal>0.01) {
				pm1 = MonoMZ.dblVal * Charge.iVal - ((Charge.iVal-1)*1.007276466);
				s.setMZ(MZs[0],MonoMZ.dblVal);
			}	else {
				pm1 = MZs[0] * Charge.iVal - ((Charge.iVal-1)*1.007276466);
				s.setMZ(MZs[0]);
			}
			s.addZState(Charge.iVal,pm1);
			s.setCharge(Charge.iVal);
    } else {
			s.setMZ(MZs[0]);
      charge = calcChargeState(MZs[0], highmass, &varMassList, lArraySize);

      //Charge greater than 0 means the charge state is known
      if(charge>0){
        pm1 = MZs[0]*charge - ((charge-1)*1.007276466);
  	    s.addZState(charge,pm1);

      //Charge of 0 means unknown charge state, therefore, compute +2 and +3 states.
      } else {
        pm1 = MZs[0]*2 - 1.007276466;
        s.addZState(2,pm1);
        pm1 = MZs[0]*3 - 2*1.007276466;
        s.addZState(3,pm1);
      }

    }

  } //endif MS2 and MS3

	if(MSn==MSX){
		for(i=0;i<(int)MZs.size();i++){
			if(i==0) s.setMZ(MZs[i],0);
			else s.addMZ(MZs[i],0);
		}
		s.setCharge(0);
	}

	//Set basic scan info
  if(bCentroid) s.setCentroidStatus(1);
  else s.setCentroidStatus(0);
  s.setRawFilter(curFilter);
	s.setScanNumber((int)rawCurSpec);
  s.setScanNumber((int)rawCurSpec,true);
	s.setRTime((float)dRTime);
	s.setFileType(MSn);
  s.setBPI((float)BPI);
  s.setBPM(BPM);
  s.setCompensationVoltage(cv);
  s.setConversionA(ConversionA.dblVal);
  s.setConversionB(ConversionB.dblVal);
	s.setConversionC(ConversionC.dblVal);
  s.setConversionD(ConversionD.dblVal);
	s.setConversionE(ConversionE.dblVal);
  s.setConversionI(ConversionI.dblVal);
  s.setTIC(TIC);
  s.setIonInjectionTime(IIT.fltVal);
	if(MSn==SRM) s.setMZ(MZs[0]);
  switch(MSn){
    case MS1: s.setMsLevel(1); break;
    case MS2: s.setMsLevel(2); break;
    case MS3: s.setMsLevel(3); break;
		case MSX: s.setMsLevel(2); break;
    default: s.setMsLevel(0); break;
  }
	psa = varMassList.parray;
  SafeArrayAccessData( psa, (void**)(&pDataPeaks) );
  for(j=0;j<lArraySize;j++)	s.add(pDataPeaks[j].dMass,(float)pDataPeaks[j].dIntensity);
  SafeArrayUnaccessData( psa );

  //Clean up memory
	VariantClear(&Charge);
	VariantClear(&MonoMZ);
  VariantClear(&IIT);
  VariantClear(&ConversionA);
  VariantClear(&ConversionB);
	VariantClear(&ConversionC);
  VariantClear(&ConversionD);
	VariantClear(&ConversionE);
  VariantClear(&ConversionI);
	VariantClear(&varMassList);
	VariantClear(&varPeakFlags);

  return true;

}

void RAWReader::setMSLevelFilter(vector<MSSpectrumType>* v){
	msLevelFilter=v;
}

void RAWReader::setRawFilter(char *c){
  strcpy(rawUserFilter,c);
}
