/************************************************************
 *              SAXMzmlHandler.cpp
 * Adapted from SAXMzdataHandler.cpp
 * August 2008
 * Ronald Beavis
 *
 * April 2009 - Support for referenceable param groups and mzML 1.1.0
 * Fredrik Levander
 *
 * December 2010 - Drastically modified and cannibalized to create
 * robust, yet generic, mzML parser
 * Mike Hoopmann, Institute for Systems Biology
 *
 * Premiere version janvier 2005
 * Patrick Lacasse
 * placasse@mat.ulaval.ca
 *
 * 3/11/2005 (Brendan MacLean): Use eXpat SAX parser, and create SAXSpectraHandler
 *
 * November 2005
 * Fredrik Levander 
 * A few changes to handle MzData 1.05.
 *
 * Updated to handle version 1.04 and 1.05. (Rob Craig)
 *
 *
 * See http://psidev.sourceforge.net/ms/#mzdata for
 * mzData schema information.
 *
 * Inspired by DtaSAX2Handler.cpp
 * copyright            : (C) 2002 by Pedrioli Patrick, ISB, Proteomics
 * email                : ppatrick@systemsbiology.org
 * Artistic License granted 3/11/2005
 *******************************************************/

#include "mzParser.h"
using namespace std;
using namespace mzParser;

mzpSAXMzmlHandler::mzpSAXMzmlHandler(BasicSpectrum* bs){
  m_bChromatogramIndex = false;
  m_bInmzArrayBinary = false;
  m_bInintenArrayBinary = false;
  m_bInionMobilityArrayBinary = false;
  m_bionMobility = false;
  m_bInRefGroup = false;
  m_bNetworkData = false; //always little-endian for mzML
  m_bNumpressLinear = false;
  m_bNumpressPic = false;
  m_bNumpressSlof = false;
  m_bLowPrecision = false;
  m_bInSpectrumList=false;
  m_bInChromatogramList=false;
  m_bInIndexedMzML=false;
  m_bInIndexList=false;
  m_bInProduct=false;
  m_bHeaderOnly=false;
  m_bSpectrumIndex=false;
  m_bNoIndex=true;
  m_bIndexSorted = true;
  m_bZlib=false;
  m_iDataType=0;
  spec=bs;
  indexOffset=-1;
  m_scanIDXCount = 0;

#ifdef MZP_HDF
  m_hdfFile=-1;
  m_hdfmzml = -1;
  m_hdfintData = -1;
  m_hdfmzData = -1;
  m_hdfmzSpace = -1;
  m_hdfintSpace = -1;
  m_hdfOffset=0;
  m_hdfArraySz=0;
#endif

  chromat=NULL;
}

mzpSAXMzmlHandler::mzpSAXMzmlHandler(BasicSpectrum* bs, BasicChromatogram* cs){
  m_bChromatogramIndex = false;
  m_bInmzArrayBinary = false;
  m_bInintenArrayBinary = false;
  m_bInionMobilityArrayBinary = false;
  m_bionMobility = false;
  m_bInRefGroup = false;
  m_bNetworkData = false; //always little-endian for mzML
  m_bNumpressLinear = false;
  m_bNumpressPic = false;
  m_bNumpressSlof = false;
  m_bLowPrecision = false;
  m_bInSpectrumList=false;
  m_bInChromatogramList=false;
  m_bInIndexedMzML=false;
  m_bInIndexList=false;
  m_bInProduct=false;
  m_bHeaderOnly=false;
  m_bSpectrumIndex=false;
  m_bNoIndex=true;
  m_bIndexSorted = true;
  m_bZlib=false;
  m_iDataType=0;
  spec=bs;
  chromat=cs;
  indexOffset=-1;
  m_scanIDXCount = 0;

#ifdef MZP_HDF
  m_hdfFile = -1;
  m_hdfmzml = -1;
  m_hdfintData = -1;
  m_hdfmzData = -1;
  m_hdfmzSpace = -1;
  m_hdfintSpace = -1;
  m_hdfOffset = 0;
  m_hdfArraySz = 0;
#endif
}

mzpSAXMzmlHandler::~mzpSAXMzmlHandler(){
  chromat=NULL;
  spec=NULL;
}

void mzpSAXMzmlHandler::startElement(const XML_Char *el, const XML_Char **attr){

  if (isElement("binaryDataArray",el)){
    m_bNumpressLinear=false;
    string s=getAttrValue("encodedLength", attr);
    m_encodedLen=atoi(&s[0]);

  } else if (isElement("binaryDataArrayList",el)) {
#ifdef MZP_HDF
    if(m_bHeaderOnly) stopParser();
#else
    if (m_bHeaderOnly) stopParser();
#endif
    m_bionMobility = false;

  } else if (isElement("chromatogram",el)) {
    string s=getAttrValue("id", attr);
    chromat->setIDString(&s[0]);
    m_peaksCount = atoi(getAttrValue("defaultArrayLength", attr));

  } else if (isElement("chromatogramList",el)) {
    m_bInChromatogramList=true;

  } else if(isElement("index",el) && m_bInIndexList){
    if(!strcmp(getAttrValue("name", attr),"spectrum")) m_bSpectrumIndex=true;
    if(!strcmp(getAttrValue("name", attr),"chromatogram")) m_bChromatogramIndex=true;

  } else if (isElement("indexedmzML",el)) {
    m_vIndex.clear();
    m_bInIndexedMzML=true;

  } else if(isElement("indexList",el)) {
    m_bInIndexList=true;

  } else if(isElement("offset",el) && m_bChromatogramIndex){
    m_strData.clear();
    curChromatIndex.idRef=string(getAttrValue("idRef", attr));

  } else if(isElement("offset",el) && m_bSpectrumIndex){
    m_strData.clear();
    curIndex.idRef=string(getAttrValue("idRef", attr));
    if(strstr(&curIndex.idRef[0],"scan=")!=NULL)  {
      curIndex.scanNum=atoi(strstr(&curIndex.idRef[0],"scan=")+5);
    } else if(strstr(&curIndex.idRef[0],"scanId=")!=NULL) {
      curIndex.scanNum=atoi(strstr(&curIndex.idRef[0],"scanId=")+7);
    } else if (strstr(&curIndex.idRef[0], "frame") != NULL) { //TIMSTOF is indexed
      curIndex.scanNum = ++m_scanIDXCount;
    } else if(strstr(&curIndex.idRef[0],"S")!=NULL) {
      curIndex.scanNum=atoi(strstr(&curIndex.idRef[0],"S")+1);
    } else if (strstr(&curIndex.idRef[0], "index=") != NULL) {
      curIndex.scanNum = atoi(strstr(&curIndex.idRef[0], "index=") + 6);
    } else {
      curIndex.scanNum=++m_scanIDXCount;
      //Suppressing warning.
      //cout << "WARNING: Cannot extract scan number in index offset line: " << &curIndex.idRef[0] << "\tDefaulting to " << m_scanIDXCount << endl;
    }

  } else if(isElement("precursor",el)) {
    m_vState.push_back(esPrecursor);
    string s=getAttrValue("spectrumRef", attr);

    //clear all precursor info, EXCEPT for the parent ion monoMz, which is captured in thermo trailer in the <scan> element.
    double parentMonoMz=m_precursorIon.monoMZ;
    m_precursorIon.clear();
    m_precursorIon.monoMZ=parentMonoMz;

    //if spectrumRef is not provided
    if (s.length() < 1) {
      m_precursorIon.scanNumber=0;
    } else {
      map<string, size_t>::iterator it = m_mIndex.find(s);
      if (it != m_mIndex.end()) {
        m_precursorIon.scanNumber=(int)it->second;
      } else {
        //silence error. A non-indexed precursor (e.g., if MS1 scans have been excised) leads to a scanNumber of 0.
        //cout << "ERROR: precursor:spectrumRef " << s << " is not indexed." << endl;
        m_precursorIon.scanNumber = 0;
      }
    }

  } else if (isElement("product", el)) {
    m_bInProduct=true;

  }  else if (isElement("referenceableParamGroup", el)) {
    const char* groupName = getAttrValue("id", attr);
    m_ccurrentRefGroupName = string(groupName);
    m_bInRefGroup = true;

  } else if (isElement("run", el)){
    stopParser();

  }  else if (isElement("softwareParam", el)) {
    const char* name = getAttrValue("name", attr);
    const char* accession = getAttrValue("accession", attr);
    const char* version = getAttrValue("version", attr);

  } else if(isElement("selectedIonList", el)){
    int count=atoi(getAttrValue("count",attr));
    if(count>1){
      cout << "WARNING: selectedIonList count >1. Please report." << endl;
    }

  }  else if (isElement("spectrum", el)) {
    m_precursorIon.clear(); //clear all precursor information here
    m_vState.push_back(esSpectrum);
    string s=getAttrValue("id", attr);
    spec->setIDString(&s[0]);
    map<string, size_t>::iterator it = m_mIndex.find(s);
    if (it != m_mIndex.end()) {
      spec->setScanNum((int)it->second);
    } else {
      cout << "ERROR: " << s << " is not indexed." << endl;
    }
    m_peaksCount = atoi(getAttrValue("defaultArrayLength", attr));
    spec->setPeaksCount(m_peaksCount);

  } else if (isElement("spectrumList",el)) {
    m_bInSpectrumList=true;

  }  else if (isElement("cvParam", el)) {
    const char* name = getAttrValue("name", attr);
    const char* accession = getAttrValue("accession", attr);
    const char* value = getAttrValue("value", attr);
    const char* unitName = getAttrValue("unitName", attr);
    const char* unitAccession = getAttrValue("unitAccession", attr);
    if (m_bInRefGroup) {
      cvParam m_cvParam;
      m_cvParam.refGroupName = string(m_ccurrentRefGroupName);
      m_cvParam.name = string(name);
      m_cvParam.accession = string(accession);
      m_cvParam.value = string(value);
      m_cvParam.unitName = string(unitName);
      m_cvParam.unitAccession = string(unitAccession);
      m_refGroupCvParams.push_back(m_cvParam);
    }  else {
      processCVParam(name,accession,value,unitName,unitAccession);
    }

  }  else if (isElement("referenceableParamGroupRef", el))  {
    const char* groupName = getAttrValue("ref", attr);
    for (unsigned int i=0;i<m_refGroupCvParams.size();i++)  {
      if( strcmp(groupName,&m_refGroupCvParams[i].refGroupName[0])==0 )  {
        processCVParam(&m_refGroupCvParams[i].name[0], &m_refGroupCvParams[i].accession[0], &m_refGroupCvParams[i].value[0], &m_refGroupCvParams[i].unitName[0], &m_refGroupCvParams[i].unitAccession[0]);
      }
    }
  } else if (isElement("userParam", el)) {
    const char* name = getAttrValue("name", attr);
    const char* dtype = getAttrValue("type", attr);
    const char* value = getAttrValue("value", attr);
    if(strcmp(name,"[Thermo Trailer Extra]Monoisotopic M/Z:")==0){
      m_precursorIon.monoMZ=atof(value);
    } else if (strcmp(name, "scan description") == 0) {
      //m_precursorIon.monoMZ = atof(value);
    } else if (strcmp(name, "ms level") == 0) {
      m_precursorIon.msLevel = atoi(value);
    }
    spec->setUserParam(name,value,dtype);
  }

  if(isElement("binary", el))  {
    m_strData.clear();
  }

}


void mzpSAXMzmlHandler::endElement(const XML_Char *el) {

  if(isElement("binary", el))  {
    processData();
    m_strData.clear();

  } else if(isElement("binaryDataArray", el))  {
    m_bZlib=false;
    m_bInintenArrayBinary = false;
    m_bInmzArrayBinary = false;
    m_bNumpressLinear=false;
    m_bNumpressSlof=false;
    m_bNumpressPic=false;
    m_iDataType=0;

  } else if(isElement("chromatogram",el)) {
    pushChromatogram();
    stopParser();

  } else if(isElement("chromatogramList",el)) {
    m_bInChromatogramList=false;

  } else if(isElement("componentList",el)) {
    m_vInstrument.push_back(m_instrument);

  } else if(isElement("index",el)){
    m_bSpectrumIndex=false;
    m_bChromatogramIndex=false;
    

  } else if(isElement("indexList",el)){
    m_bInIndexList=false;
    stopParser();
    if (!m_bIndexSorted) {
      sort(m_vIndex.begin(),m_vIndex.end(),cindex::compare);
      m_bIndexSorted=true;
    }

  } else if (isElement("isolationWindow",el)){
    if (chromat != NULL && m_bInProduct) chromat->setProduct(m_precursorIon.isoMZ, m_precursorIon.isoLowerMZ, m_precursorIon.isoUpperMZ);

  } else if (isElement("offset", el) && m_bChromatogramIndex) {
    curChromatIndex.offset = mzpatoi64(&m_strData[0]);
    m_vChromatIndex.push_back(curChromatIndex);
    m_mChromatIndex.insert(pair<string, size_t>(curChromatIndex.idRef, m_vChromatIndex.size()));

  } else if (isElement("offset", el) && m_bSpectrumIndex) {
    curIndex.offset = mzpatoi64(&m_strData[0]);
    m_vIndex.push_back(curIndex);
    m_mIndex.insert(pair<string, size_t>(curIndex.idRef, curIndex.scanNum));
    if (m_bIndexSorted && m_vIndex.size() > 1) {
      if (m_vIndex[m_vIndex.size() - 1].scanNum < m_vIndex[m_vIndex.size() - 2].scanNum) {
        m_bIndexSorted = false;
      }
    }

  } else if(isElement("precursor",el)){
    spec->setPrecursorIon(m_precursorIon);
    if(m_vState.back()!=esPrecursor){
      cout << "Error: expected state should be precursor." << endl;
    } else m_vState.pop_back();

  } else if(isElement("precursorList",el)){

  } else if (isElement("product", el)){
    m_bInProduct=false;
    
  } else if(isElement("referenceableParamGroup", el)) {
    m_bInRefGroup = false;

  } else if(isElement("selectedIon",el)) {

  }  else if(isElement("spectrum", el)) {
    pushSpectrum();
    stopParser();
    if (m_vState.back() != esSpectrum) {
      cout << "Error: expected state should be spectrum." << endl;
    } else m_vState.pop_back();
    
  } else if(isElement("spectrumList",el)) {
    m_bInSpectrumList = false;

  }
}

void mzpSAXMzmlHandler::characters(const XML_Char *s, int len) {
  m_strData.append(s, len);
}

void mzpSAXMzmlHandler::processCVParam(const char* name, const char* accession, const char* value, const char* unitName, const char* unitAccession)
{
  if(!strcmp(name, "32-bit float") || !strcmp(accession,"MS:1000521"))  {
    m_bLowPrecision = true;
    m_iDataType=1;

  } else if(!strcmp(name, "64-bit float") || !strcmp(accession,"MS:1000523"))  {
    m_bLowPrecision = false;
    m_iDataType=2;

  } else if(!strcmp(name, "base peak intensity") || !strcmp(accession,"MS:1000505"))  {
    spec->setBasePeakIntensity(atof(value));

  } else if(!strcmp(name, "base peak m/z") || !strcmp(accession,"MS:1000504"))  {
    spec->setBasePeakMZ(atof(value));
    
  } else if (!strcmp(name, "beam-type collision-induced dissociation") || !strcmp(accession, "MS:1000422")) {
    if(m_vState.back()==esPrecursor) m_precursorIon.activation=HCD;
    spec->setActivation(HCD);

  } else if(!strcmp(name, "centroid spectrum") || !strcmp(accession,"MS:1000127")) {
    spec->setCentroid(true);

  } else if(!strcmp(name, "charge state") || !strcmp(accession,"MS:1000041"))  {
    m_precursorIon.charge = atoi(value);

  } else if(!strcmp(name, "collision-induced dissociation") || !strcmp(accession,"MS:1000133"))  {
    if(spec->getActivation()==ETD) {
      if (m_vState.back() == esPrecursor) m_precursorIon.activation = ETDSA;
      spec->setActivation(ETDSA);
    } else {
      if (m_vState.back() == esPrecursor) m_precursorIon.activation = CID;
      spec->setActivation(CID);
    }

  } else if(!strcmp(name, "collision energy") || !strcmp(accession,"MS:1000045"))  {
    spec->setCollisionEnergy(atof(value));

  } else if(!strcmp(name,"electron multiplier") || !strcmp(accession,"MS:1000253")) {
    m_instrument.detector=name;

  } else if(!strcmp(name, "electron transfer dissociation") || !strcmp(accession,"MS:1000598"))  {
    if(spec->getActivation()==CID) {
      if (m_vState.back() == esPrecursor) m_precursorIon.activation = ETDSA;
      spec->setActivation(ETDSA);
    } else {
      if (m_vState.back() == esPrecursor) m_precursorIon.activation = ETD;
      spec->setActivation(ETD);
    }

#ifdef MZP_HDF
  } else if (!strcmp(name, "external HDF5 dataset") || !strcmp(accession, "MS:1002841")) {
    m_strHDFDatasetID = value;

  } else if (!strcmp(name, "external array length") || !strcmp(accession, "MS:1002843")) {
    m_hdfArraySz = (hsize_t)atoll(value);
    if (m_hdfFile > -1) {
      spec->setPeaksCount((int)m_hdfArraySz);
      if (m_bHeaderOnly) stopParser();
    }

  } else if (!strcmp(name, "external offset") || !strcmp(accession, "MS:1002842")) {
    m_hdfOffset = (hsize_t)atoll(value);
#endif

  } else if(!strcmp(name, "FAIMS compensation voltage") || !strcmp(accession,"MS:1001581"))  {
    spec->setCompensationVoltage(atof(value));
    
  } else if(!strcmp(name, "filter string") || !strcmp(accession,"MS:1000512"))  {
    char str[128];
    strncpy(str,value,127);
    str[127]='\0';
    spec->setFilterLine(str);

  } else if(!strcmp(name, "highest observed m/z") || !strcmp(accession,"MS:1000527"))  {
    spec->setHighMZ(atof(value));

  } else if(!strcmp(name,"inductive detector") || !strcmp(accession,"MS:1000624")) {
    m_instrument.detector=name;

  } else if(!strcmp(name, "intensity array") || !strcmp(accession,"MS:1000515"))  {
    m_bInintenArrayBinary = true;
    m_bInmzArrayBinary = false;
    m_bInionMobilityArrayBinary = false;

  } else if (!strcmp(name, "ion injection time") || !strcmp(accession, "MS:1000927"))  {
    spec->setIonInjectionTime(atof(value));

  } else if (!strcmp(name, "ion mobility drift time") || !strcmp(accession, "MS:1002476")) {
    spec->setIonMobilityDriftTime(atof(value));

  } else if(!strcmp(name,"isolation window target m/z") || !strcmp(accession,"MS:1000827")) {
    m_precursorIon.isoMZ=atof(value);

  } else if (!strcmp(name, "isolation window lower offset") || !strcmp(accession, "MS:1000828")) {
    m_precursorIon.isoLowerOffset= atof(value);

  } else if (!strcmp(name, "isolation window upper offset") || !strcmp(accession, "MS:1000829")) {
    m_precursorIon.isoUpperOffset = atof(value);

  } else if(!strcmp(name,"LTQ Velos") || !strcmp(accession,"MS:1000855")) {
    m_instrument.model=name;

  } else if(!strcmp(name, "lowest observed m/z") || !strcmp(accession,"MS:1000528"))  {
    spec->setLowMZ(atof(value));

  } else if( !strcmp(name, "MS1 spectrum") || !strcmp(accession,"MS:1000579") ){
    spec->setMSLevel(1);

  } else if( !strcmp(name, "ms level") || !strcmp(accession,"MS:1000511") ){
    spec->setMSLevel(atoi(value));

  } else if( !strcmp(name, "MS-Numpress linear prediction compression") || !strcmp(accession,"MS:1002312") ){
    m_bNumpressLinear = true;

  } else if( !strcmp(name, "MS-Numpress positive integer compression") || !strcmp(accession,"MS:1002313") ){
    m_bNumpressPic = true;

  } else if( !strcmp(name, "MS-Numpress short logged float compression") || !strcmp(accession,"MS:1002314") ){
    m_bNumpressSlof = true;

  } else if (!strcmp(name, "MS-Numpress linear prediction compression followed by zlib compression") || !strcmp(accession, "MS:1002746")){
    m_bZlib = true;
    m_bNumpressLinear = true;

  } else if (!strcmp(name, "MS-Numpress positive integer compression followed by zlib compression") || !strcmp(accession, "MS:1002747")) {
    m_bZlib = true;
    m_bNumpressPic = true;

  } else if (!strcmp(name, "MS-Numpress short logged float compression followed by zlib compression") || !strcmp(accession, "MS:1002748")){
    m_bZlib = true;
    m_bNumpressSlof = true;

  } else if(!strcmp(name, "m/z array") || !strcmp(accession,"MS:1000514"))  {
    m_bInmzArrayBinary = true;
    m_bInintenArrayBinary = false;
    m_bInionMobilityArrayBinary = false;

  } else if(!strcmp(name,"nanoelectrospray") || !strcmp(accession,"MS:1000398")) {
    m_instrument.ionization=name;

    //this looks like a mess. Fix it properly.
  } else if ((!strcmp(name, "non-standard data array") && !strcmp(value, "Ion Mobility") && !strcmp(accession, "MS:1000786")) ||
    (!strcmp(name, "mean inverse reduced ion mobility array") && !strcmp(accession, "MS:1003006")) ||
    (!strcmp(name, "raw ion mobility array") && !strcmp(accession, "MS:1003007"))) {
    m_bInmzArrayBinary = false;
    m_bInintenArrayBinary = false;
    m_bInionMobilityArrayBinary = true;
    m_bionMobility = true;
    spec->setIonMobilityScan(true);

  } else if(!strcmp(name,"orbitrap") || !strcmp(accession,"MS:1000484")) {
    m_instrument.analyzer=name;

  } else if(!strcmp(name,"peak intensity") || !strcmp(accession,"MS:1000042")) {
    m_precursorIon.intensity=atof(value);

  } else if(!strcmp(name,"positive scan") || !strcmp(accession,"MS:1000130")) {
    spec->setPositiveScan(true);

  } else if(!strcmp(name,"possible charge state") || !strcmp(accession,"MS:1000633")) {
    m_precursorIon.possibleCharges.push_back(atoi(value));

  } else if(!strcmp(name,"profile spectrum") || !strcmp(accession,"MS:1000128")) {
    spec->setCentroid(false);

  } else if(!strcmp(name,"radial ejection linear ion trap") || !strcmp(accession,"MS:1000083")) {
    m_instrument.analyzer=name;

  } else if (!strcmp(name, "inverse reduced ion mobility") || !strcmp(accession, "MS:1002815")) {
    spec->setInverseReducedIonMobility(atof(value));
    //spec->setIonMobilityScan(true);
    //m_bionMobility = true;

  } else if(!strcmp(name, "scan start time") || !strcmp(accession,"MS:1000016"))  {
    if(!strcmp(unitName, "minute") || !strcmp(unitAccession,"UO:0000031"))  {
      spec->setRTime((float)atof(value));
    } else {
      spec->setRTime((float)atof(value)/60.0f); //assume if not minutes, then seconds
    }

  } else if(!strcmp(name, "scan window lower limit") || !strcmp(accession,"MS:1000501"))    {
    //TODO: should we also check the units???
    spec->setLowMZ(atof(value));
    
  } else if(!strcmp(name, "scan window upper limit") || !strcmp(accession,"MS:1000500"))    {
    //TODO: should we also check the units???
    spec->setHighMZ(atof(value));
    
  } else if(!strcmp(name, "selected ion m/z") || !strcmp(accession,"MS:1000744"))  {
    //MH: Note the change here. From now on, selected ion m/z always goes in the m_precursorIon.mz variable.
    //Isolation mz (different cvParam) always goes in m_precursionIon.isoMZ, and the thermo trailer (userParam) goes in m_precursorIon.monoMZ.
    //However, the meaning of selected ion m/z can mean different things in ProteoWizard converted mzML files. If no thermo trailer is
    //provided, the selected ion m/z == isolated m/z. But if thermo trailer, then it selected ion m/z == monoisotopic m/z.
    //But if there are multiple precursors and thermo trailer, the same monoisotopic m/z will be applied to all instances of
    //m_precursorIon.monoMZ, regardless of whether or not it matches the selected ion m/z of that instance.
    m_precursorIon.mz=atof(value); //in Thermo instruments this is the monoisotopic peak (if known) or the selected ion peak.
    //if(m_precursorIon.monoMZ!=0) m_precursorIon.monoMZ=atof(value); //if the monoisotopic peak was specified in the thermo trailer, this is a better value to use.
    //else if (m_precursorIon.mz<m_precursorIon.isoMZ) m_precursorIon.monoMZ = atof(value); //failsafe? for when thermo trailer info is missing.
    //This still gets complicated when there are multiple precursors, but only one thermo trailer value...

  } else if(!strcmp(name, "time array") || !strcmp(accession,"MS:1000595"))  {
    m_bInmzArrayBinary = true; //note that this uses the m/z designation, although it is a time series
    m_bInintenArrayBinary = false;

  } else if(!strcmp(name, "total ion current") || !strcmp(accession,"MS:1000285"))  {
    spec->setTotalIonCurrent(atof(value));

  } else if(!strcmp(name,"Thermo RAW file") || !strcmp(accession,"MS:1000563")) {
    m_instrument.manufacturer="Thermo Scientific";

  } else if(!strcmp(name, "zlib compression") || !strcmp(accession,"MS:1000574"))  {
    m_bZlib=true;
  }
}

void mzpSAXMzmlHandler::processData(){
#ifdef MZP_HDF
  if(m_hdfFile>-1){

    if (m_bInintenArrayBinary) {
      if(m_hdfintData<0) {
        m_hdfintData = H5Dopen(m_hdfFile, m_strHDFDatasetID.c_str(), H5P_DEFAULT);
        m_hdfintSpace = H5Dget_space(m_hdfintData);
      }
    } else if (m_bInmzArrayBinary) {
      if(m_hdfmzData < 0) {
        m_hdfmzData = H5Dopen(m_hdfFile, m_strHDFDatasetID.c_str(), H5P_DEFAULT);
        m_hdfmzSpace = H5Dget_space(m_hdfmzData);
      }
    } else if (m_bInionMobilityArrayBinary) {
      cout << "Need to write code for IonMobilityArray in mzMLb" << endl;
      return;
    }

    m_peaksCount = (int)m_hdfArraySz;
    
    if(m_bLowPrecision) {
      float* tmp = new float[m_hdfArraySz];
      if(m_bInintenArrayBinary) {
        H5Sselect_hyperslab(m_hdfintSpace, H5S_SELECT_SET, &m_hdfOffset, NULL, &m_hdfArraySz, NULL);
        hid_t mspace = H5Screate_simple(1, &m_hdfArraySz, &m_hdfArraySz);
        herr_t status = H5Dread(m_hdfintData, H5T_NATIVE_FLOAT, mspace, m_hdfintSpace, H5P_DEFAULT, tmp);
        vdI.clear();
        for(hsize_t a=0;a<m_hdfArraySz;a++) vdI.push_back((double)tmp[a]);
        H5Sclose(mspace);
      } else if(m_bInmzArrayBinary){
        H5Sselect_hyperslab(m_hdfmzSpace, H5S_SELECT_SET, &m_hdfOffset, NULL, &m_hdfArraySz, NULL);
        hid_t mspace = H5Screate_simple(1, &m_hdfArraySz, &m_hdfArraySz);
        herr_t status = H5Dread(m_hdfmzData, H5T_NATIVE_FLOAT, mspace, m_hdfmzSpace, H5P_DEFAULT, tmp);
        vdM.clear();
        for (hsize_t a = 0; a < m_hdfArraySz; a++) vdM.push_back((double)tmp[a]);
        H5Sclose(mspace);
      }
      delete [] tmp;
    } else {
      double* tmp = new double[m_hdfArraySz];
      if (m_bInintenArrayBinary) {
        H5Sselect_hyperslab(m_hdfintSpace, H5S_SELECT_SET, &m_hdfOffset, NULL, &m_hdfArraySz, NULL);
        hid_t mspace = H5Screate_simple(1, &m_hdfArraySz, &m_hdfArraySz);
        herr_t status = H5Dread(m_hdfintData, H5T_NATIVE_DOUBLE, mspace, m_hdfintSpace, H5P_DEFAULT, tmp);
        vdI.clear();
        for (hsize_t a = 0; a < m_hdfArraySz; a++) vdI.push_back(tmp[a]);
        H5Sclose(mspace);
      } else if (m_bInmzArrayBinary) {
        H5Sselect_hyperslab(m_hdfmzSpace, H5S_SELECT_SET, &m_hdfOffset, NULL, &m_hdfArraySz, NULL);
        hid_t mspace = H5Screate_simple(1, &m_hdfArraySz, &m_hdfArraySz);
        herr_t status = H5Dread(m_hdfmzData, H5T_NATIVE_DOUBLE, mspace, m_hdfmzSpace, H5P_DEFAULT, tmp);
        vdM.clear();
        for (hsize_t a = 0; a < m_hdfArraySz; a++) vdM.push_back(tmp[a]);
        H5Sclose(mspace);
      }
      delete[] tmp;
    }
#else
  if(false){
#endif
  } else {
    if(m_bInmzArrayBinary) {
      decode(vdM);
    } else if(m_bInintenArrayBinary) {
      decode(vdI);
    } else if (m_bInionMobilityArrayBinary) {
      decode(vdIM);
    }
  }
}

bool mzpSAXMzmlHandler::readChromatogram(int num){
  if(chromat==NULL) return false;
  chromat->clear();

  if(m_bNoIndex){
    cout << "Currently only supporting indexed mzML" << endl;
    return false;
  }

  //if no scan was requested, grab the next one
  if(num<0) posChromatIndex++;
  else posChromatIndex=num;
  
  if(posChromatIndex>=(int)m_vChromatIndex.size()) return false;
  parseOffset(m_vChromatIndex[posChromatIndex].offset);
  return true;
}

bool mzpSAXMzmlHandler::readHeader(int num){
  spec->clear();

  if(m_bNoIndex){
    cout << "Currently only supporting indexed mzML" << endl;
    return false;
  }

  //if no scan was requested, grab the next one
  if(num<0){
    posIndex++;
    if(posIndex>=(int)m_vIndex.size()) return false;
    m_bHeaderOnly=true;
    parseOffset(m_vIndex[posIndex].offset);
    m_bHeaderOnly=false;
    return true;
  }

  //Assumes scan numbers are in order
  size_t mid=m_vIndex.size()/2;
  size_t upper=m_vIndex.size();
  size_t lower=0;
  while(m_vIndex[mid].scanNum!=num){
    if(lower==upper) break;
    if(m_vIndex[mid].scanNum>num){
      upper=mid-1;
      mid=(lower+upper)/2;
    } else {
      lower=mid+1;
      mid=(lower+upper)/2;
    }
  }

  if(m_vIndex[mid].scanNum==num) {
    m_bHeaderOnly=true;
    parseOffset(m_vIndex[mid].offset);
    //force scan number; this was done for files where scan events are not numbered
    if(spec->getScanNum()!=m_vIndex[mid].scanNum) spec->setScanNum((int)m_vIndex[mid].scanNum);
    spec->setScanIndex((int)mid+1); //set the index, which starts from 1, so offset by 1
    m_bHeaderOnly=false;
    posIndex=(int)mid;
    return true;
  }
  return false;

}

bool mzpSAXMzmlHandler::readHeaderFromOffset(f_off offset, int scNm){
  spec->clear();
  m_scanNumOverride=scNm;
  
  //index must be positive.
  if (offset<0) return false;

  //note that scan number will not be set if file doesn't use them.
  //also, no knowlege of current position in index is known or retained. Reading from
  //scan index will revert back to next scan from its current position.
  //m_bHeaderOnly = true;  //MH: Changed behavior to always process entire scan whether just the header information was needed or not.
#ifdef MZP_HDF
  if(m_hdfFile>-1) parseHDFOffset((int)offset);
  else parseOffset(offset);
#else
  parseOffset(offset);
#endif
  //m_bHeaderOnly = false;
  return true;

}

bool mzpSAXMzmlHandler::readSpectrum(int num){
  spec->clear();

  if(m_bNoIndex){
    cout << "Currently only supporting indexed mzML" << endl;
    return false;
  }

  //if no scan was requested, grab the next one
  if(num<0){
    posIndex++;
    if(posIndex>=(int)m_vIndex.size()) return false;
    parseOffset(m_vIndex[posIndex].offset);
    return true;
  }

  //Assumes scan numbers are in order
  size_t mid=m_vIndex.size()/2;
  size_t upper=m_vIndex.size();
  size_t lower=0;
  while(m_vIndex[mid].scanNum!=num){
    if(lower==upper) break;
    if(m_vIndex[mid].scanNum>num){
      upper=mid-1;
      mid=(lower+upper)/2;
    } else {
      lower=mid+1;
      mid=(lower+upper)/2;
    }
  }

  //need something faster than this perhaps
  //for(unsigned int i=0;i<m_vIndex.size();i++){
    if(m_vIndex[mid].scanNum==num) {
      parseOffset(m_vIndex[mid].offset);
      //force scan number; this was done for files where scan events are not numbered
      if(spec->getScanNum()!=m_vIndex[mid].scanNum) spec->setScanNum((int)m_vIndex[mid].scanNum);
      spec->setScanIndex((int)mid+1); //set the index, which starts from 1, so offset by 1
      posIndex=(int)mid;
      return true;
    }
  //}
  return false;
}

//somewhat dangerous as it allows reading anywhere in the file
bool mzpSAXMzmlHandler::readSpectrumFromOffset(f_off offset, int scNm){
  spec->clear();
  m_scanNumOverride=scNm;

  //index must be positive.
  if (offset<0) return false;

  //note that scan number will not be set if file doesn't use them.
  //also, no knowlege of current position in index is known or retained. Reading from
  //scan index will revert back to next scan from its current position.
#ifdef MZP_HDF
  if(m_hdfFile>-1) parseHDFOffset((int)offset);
  else parseOffset(offset);
#else 
  parseOffset(offset);
#endif
  return true;

}

void mzpSAXMzmlHandler::setMZMLB(bool b){
  m_bMZMLB=b;
}

void mzpSAXMzmlHandler::pushChromatogram(){
  TimeIntensityPair tip;
  for(size_t i=0;i<vdM.size();i++)  {
    tip.time = vdM[i];
    tip.intensity = vdI[i];
    chromat->addTIP(tip);
  }
}

void mzpSAXMzmlHandler::pushSpectrum(){
  specDP dp;
  specIonMobDP dp_im;
  if(vdIM.size()>0) spec->setIonMobilityScan(true);
  for(size_t i=0;i<vdM.size();i++)  {
    dp.mz = vdM[i];
    dp.intensity = vdI[i];

    if (i < vdIM.size()) {
      dp_im.mz = vdM[i];
      dp_im.intensity = vdI[i];
      dp_im.ionMobility = vdIM[i];
      spec->addDP(dp_im);
    } else {
      spec->addDP(dp);
    }
  }
}

void mzpSAXMzmlHandler::decode(vector<double>& d){

  //If there is no data, back out now
  d.clear();
  if(m_peaksCount < 1) return;

  //For byte order correction
  union udata32 {
    float d;
    uint32_t i;  
  } uData32; 

  union udata64 {
    double d;
    uint64_t i;  
  } uData64; 

  const char* pData = m_strData.data();
  size_t stringSize = m_strData.size();

  char* decoded = new char[m_encodedLen];  //array for decoded base64 string
  int decodeLen;
  Bytef* unzipped = NULL;
  uLong unzippedLen;

  int i;

  //Base64 decoding
  decodeLen = b64_decode_mio(decoded,(char*)pData,stringSize);

  //zlib decompression
  if(m_bZlib) {

    if(m_iDataType==1) {
      unzippedLen = m_peaksCount*sizeof(uint32_t);
    } else if(m_iDataType==2) {
      unzippedLen = m_peaksCount*sizeof(uint64_t);
    } else {
      if(!m_bNumpressLinear && !m_bNumpressSlof && !m_bNumpressPic){
        cout << "Unknown data format to unzip. Stopping file read." << endl;
        exit(EXIT_FAILURE);
      }
    //don't know the unzipped size of numpressed data, so assume it to be no larger than unpressed 64-bit data
    unzippedLen = m_peaksCount*sizeof(uint64_t);
    }

    unzipped = new Bytef[unzippedLen];
    uncompress((Bytef*)unzipped, &unzippedLen, (const Bytef*)decoded, (uLong)decodeLen);
    delete [] decoded;

  }

  //Numpress decompression
  if(m_bNumpressLinear || m_bNumpressSlof || m_bNumpressPic){
    double* unpressed=new double[m_peaksCount];
  
    try{
        if(m_bNumpressLinear){
          if(m_bZlib) ms::numpress::MSNumpress::decodeLinear((unsigned char*)unzipped,(const size_t)unzippedLen,unpressed);
          else ms::numpress::MSNumpress::decodeLinear((unsigned char*)decoded,decodeLen,unpressed);
        } else if(m_bNumpressSlof){
          if(m_bZlib) ms::numpress::MSNumpress::decodeSlof((unsigned char*)unzipped,(const size_t)unzippedLen,unpressed);
          else ms::numpress::MSNumpress::decodeSlof((unsigned char*)decoded,decodeLen,unpressed);
        } else if(m_bNumpressPic){
          if(m_bZlib) ms::numpress::MSNumpress::decodePic((unsigned char*)unzipped,(const size_t)unzippedLen,unpressed);
          else ms::numpress::MSNumpress::decodePic((unsigned char*)decoded,decodeLen,unpressed);
        }
    } catch (const char* ch){
      cout << "Exception: " << ch << endl;
      exit(EXIT_FAILURE);
    }

    if(m_bZlib) delete [] unzipped;
    else delete [] decoded;
    for(i=0;i<m_peaksCount;i++) d.push_back(unpressed[i]);
    delete [] unpressed;
    return;
  }

  //Byte order correction
  if(m_bZlib){
    if(m_iDataType==1){
      uint32_t* unzipped32 = (uint32_t*)unzipped;
      for(i=0;i<m_peaksCount;i++){
        uData32.i = dtohl(unzipped32[i], m_bNetworkData);
        d.push_back(uData32.d);
      }
    } else if(m_iDataType==2) {
      uint64_t* unzipped64 = (uint64_t*)unzipped;
      for(i=0;i<m_peaksCount;i++){
        uData64.i = dtohl(unzipped64[i], m_bNetworkData);
        d.push_back(uData64.d);
      }
    }
    delete [] unzipped;
  } else {
    if(m_iDataType==1){
      uint32_t* decoded32 = (uint32_t*)decoded;
      for(i=0;i<m_peaksCount;i++){
        uData32.i = dtohl(decoded32[i], m_bNetworkData);
        d.push_back(uData32.d);
      }
    } else if(m_iDataType==2) {
      uint64_t* decoded64 = (uint64_t*)decoded;
      for(i=0;i<m_peaksCount;i++){
        uData64.i = dtohl(decoded64[i], m_bNetworkData);
        d.push_back(uData64.d);
      }
    }
    delete [] decoded;
  }

}

unsigned long mzpSAXMzmlHandler::dtohl(uint32_t l, bool bNet) {

#ifdef OSX
  if (!bNet)
  {
    l = (l << 24) | ((l << 8) & 0xFF0000) |
      (l >> 24) | ((l >> 8) & 0x00FF00);
  }
#else
  if (bNet)
  {
    l = (l << 24) | ((l << 8) & 0xFF0000) |
      (l >> 24) | ((l >> 8) & 0x00FF00);
  }
#endif
  return l;
}

uint64_t mzpSAXMzmlHandler::dtohl(uint64_t l, bool bNet) {

#ifdef OSX
  if (!bNet)
  {
    l = (l << 56) | ((l << 40) & 0xFF000000000000LL) | ((l << 24) & 0x0000FF0000000000LL) | ((l << 8) & 0x000000FF00000000LL) |
      (l >> 56) | ((l >> 40) & 0x0000000000FF00LL) | ((l >> 24) & 0x0000000000FF0000LL) | ((l >> 8) & 0x00000000FF000000LL) ;
  }
#else
  if (bNet)
  {
    l = (l << 56) | ((l << 40) & 0x00FF000000000000LL) | ((l << 24) & 0x0000FF0000000000LL) | ((l << 8) & 0x000000FF00000000LL) |
      (l >> 56) | ((l >> 40) & 0x000000000000FF00LL) | ((l >> 24) & 0x0000000000FF0000LL) | ((l >> 8) & 0x00000000FF000000LL) ;
  }
#endif
  return l;
}

//Finding the index list offset is done without the xml parser
//to speed things along. This can be problematic if the <indexListOffset>
//tag is placed anywhere other than the end of the mzML file.
f_off mzpSAXMzmlHandler::readIndexOffset() {

  char buffer[200];
  char chunk[CHUNK];
  char* start;
  char* stop;
  int readBytes;
  size_t sz;

  if(!m_bGZCompression){
    FILE* f=fopen(&m_strFileName[0],"r");
    mzpfseek(f,-200,SEEK_END);
    sz = fread(buffer,1,200,f);
    fclose(f);
    start=strstr(buffer,"<indexListOffset>");
    stop=strstr(buffer,"</indexListOffset>");
  } else {
    readBytes = gzObj.extract(fptr, gzObj.getfilesize()-200, (unsigned char*)chunk, CHUNK);
    chunk[200]='\0';
    start=strstr(chunk,"<indexListOffset>");
    stop=strstr(chunk,"</indexListOffset>");
  }

  if(start==NULL || stop==NULL) {
//  cerr << "No index list offset found. File will not be read." << endl;
    return 0;
  }

  char offset[64];
  int len=(int)(stop-start-17);
  strncpy(offset,start+17,len);
  offset[len]='\0';
  return mzpatoi64(offset);

}

bool mzpSAXMzmlHandler::load(const char* fileName){
#ifdef MZP_HDF
  //open mzMLb files differently
  if(m_bMZMLB){
    if (fptr != NULL) fclose(fptr);
    if(!openHDF(fileName)) return false;
  } else{
    if (m_hdfFile > -1) closeHDF();
    if(!open(fileName)) return false;
  }
#else
  if (!open(fileName)) return false;
#endif

  m_vInstrument.clear();
  m_vIndex.clear();
  m_vChromatIndex.clear();
  m_mIndex.clear();
  m_mChromatIndex.clear();
  m_scanIDXCount = 0;

#ifdef MZP_HDF
  if(m_bMZMLB){
    readHDFIndex();
    readHDFHead();
    return true;
  }
#endif

  //parseOffset(0);
  indexOffset = readIndexOffset();
  if(indexOffset==0){
    m_bNoIndex=false;
    if (!generateIndexOffset()) {
       m_bNoIndex=true;
       return false;
    }

  } else {
    m_bNoIndex=false;
    if(!parseOffset(indexOffset)){ //Note: after reading index, should we check for order? assumes it is in file offset order.
       if (!generateIndexOffset()) {
          m_bNoIndex=true;
          cerr << "Cannot parse index. Make sure index offset is correct or rebuild index." << endl;
          return false;
       }
    }
    posIndex=-1;
    posChromatIndex=-1;
  }
  return true;
}

#ifdef MZP_HDF
void mzpSAXMzmlHandler::closeHDF(){
  if(m_hdfmzml>-1) H5Dclose(m_hdfmzml);
  if (m_hdfintData > -1) H5Dclose(m_hdfintData);
  if (m_hdfmzData > -1) H5Dclose(m_hdfmzData);
  if(m_hdfmzSpace>-1) H5Sclose(m_hdfmzSpace);
  if (m_hdfintSpace > -1) H5Sclose(m_hdfintSpace);
  H5Fclose(m_hdfFile);
  m_hdfFile=-1;
  m_hdfmzml=-1;
  m_hdfintData=-1;
  m_hdfmzData=-1;
  m_hdfmzSpace=-1;
  m_hdfintSpace=-1;
}

bool mzpSAXMzmlHandler::openHDF(const char* fileName) {
  if (m_hdfFile>-1) closeHDF();

  m_hdfFile=H5Fopen(fileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (m_hdfFile<0) {
    //cerr << "Failed to open input file '" << fileName << "'.\n";
    return false;
  }
  setFileName(fileName);
  return true;
}

bool mzpSAXMzmlHandler::parseHDFOffset(int index) {
  if (m_hdfFile<0) {
    cerr << "Error parseHDFOffset(): No open file." << endl;
    return false;
  }

  parserReset();
  m_bStopParse = false;

  hid_t mzml = H5Dopen(m_hdfFile, "mzML", H5P_DEFAULT);
  hsize_t pos = m_vIndex[index].offset;
  hsize_t len = m_vIndex[index].size;
  hid_t space = H5Dget_space(mzml);
  H5Sselect_hyperslab(space, H5S_SELECT_SET, &pos, NULL, &len, NULL);
  hid_t mspace = H5Screate_simple(1, &len, &len);
  char* tmp = new char[len];
  hid_t status = H5Dread(mzml, H5T_NATIVE_CHAR, mspace, space, H5P_DEFAULT, tmp);
  H5Sclose(mspace);
  H5Sclose(space);
  H5Dclose(mzml);
  bool success = (XML_Parse(m_parser, tmp, (int)len, false) != 0);
  if (!success && !m_bStopParse) {
    cout << "parseHDFOffset() failed." << endl;
    return false;
  };

  return true;
}

void mzpSAXMzmlHandler::readHDFHead() {
  if(m_hdfmzml<0) m_hdfmzml = H5Dopen(m_hdfFile, "mzML", H5P_DEFAULT);
  hsize_t pos = 0;
  hsize_t len = m_vIndex[0].offset;
  hid_t space = H5Dget_space(m_hdfmzml);
  H5Sselect_hyperslab(space, H5S_SELECT_SET, &pos, NULL, &len, NULL);
  hid_t mspace = H5Screate_simple(1, &len, &len);
  char* tmp = new char[len];
  hid_t status = H5Dread(m_hdfmzml, H5T_NATIVE_CHAR, mspace, space, H5P_DEFAULT, tmp);
  H5Sclose(mspace);
  H5Sclose(space);
  bool success = (XML_Parse(m_parser, tmp, (int)len, false) != 0);
  if(!success && !m_bStopParse){
    cout << "readHDFHead() failed." << endl;
  }
}

void mzpSAXMzmlHandler::readHDFIndex(){
  hid_t data = H5Dopen(m_hdfFile, "mzML_spectrumIndex", H5P_DEFAULT);
  hid_t space = H5Dget_space(data);

  hsize_t offCount,size,maxdims;
  H5Sget_simple_extent_dims(space, &offCount, &maxdims);
 
  int64_t* offset = new int64_t[offCount];
  hid_t status = H5Dread(data, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, offset);
  H5Sclose(space);

  hid_t index = H5Dopen(m_hdfFile, "mzML_spectrumIndex_idRef", H5P_DEFAULT);
  space = H5Dget_space(index);
  H5Sget_simple_extent_dims(space, &size, &maxdims);
  char* tmp = new char[size];
  status = H5Dread(index, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp);
  H5Sclose(space);

  hsize_t idPos = 0;
  for (hsize_t a = 0; a < offCount - 1; a++) {
    cindex i;
    i.offset = offset[a];
    i.size = offset[a + 1] - offset[a];
    i.idRef = &tmp[idPos];
    if (strstr(i.idRef.c_str(), "scan=") != NULL) {  //this code is now redundant in a few places. maybe give it a function soon.
      i.scanNum = atoi(strstr(i.idRef.c_str(), "scan=") + 5);
    } else if (strstr(i.idRef.c_str(), "scanId=") != NULL) {
      i.scanNum = atoi(strstr(i.idRef.c_str(), "scanId=") + 7);
    } else if (strstr(i.idRef.c_str(), "frame") != NULL) { //TIMSTOF is indexed
      i.scanNum = ++m_scanIDXCount;
    } else if (strstr(i.idRef.c_str(), "S") != NULL) {
      i.scanNum = atoi(strstr(i.idRef.c_str(), "S") + 1);
    } else if (strstr(i.idRef.c_str(), "index=") != NULL) {
      i.scanNum = atoi(strstr(i.idRef.c_str(), "index=") + 6);
    } else {
      i.scanNum = ++m_scanIDXCount;
      //Suppressing warning.
      //cout << "WARNING: Cannot extract scan number in index offset line: " << i.idRef << "\tDefaulting to " << m_scanIDXCount << endl;
    }
    m_vIndex.push_back(i);
    m_mIndex.insert(pair<string, size_t>(i.idRef, i.scanNum));
    idPos += strlen(&tmp[idPos]) + 1;
  }

  delete[] tmp;
  delete[] offset;
  H5Dclose(index);
  H5Dclose(data);
}
#endif

//Parse file from top to bottom to generate index offset if not present.
//If scan is present in native ID string, use it. Otherwise report spectrum index as scan number.
bool mzpSAXMzmlHandler::generateIndexOffset() {
  char chunk[CHUNK];
  int readBytes;
  long lOffset = 0;

  if(!m_bGZCompression){
    FILE* f=fopen(&m_strFileName[0],"r");
    char *pStr;

    if (f==NULL){
      cout << "Error cannot open file " << m_strFileName[0] << endl;
      exit(EXIT_FAILURE);
    }

    bool bReadingFirstSpectrum = true;
    bool bThermoFile = false;

    while (fgets(chunk, CHUNK, f)){

      // Treat thermo files differently. Always report scan 'index' value which start at 0
      // except for Thermo files where historically we're used to starting at scan 1.
      if (strstr(chunk, "MS:1000768"))
         bThermoFile = true;

      if (strstr(chunk, "<spectrum ")){
        long scanNum;
        bool bSuccessfullyReadScan = false;
        do{
          // now need to look for "index="
          if ((pStr = strstr(chunk, "index=\"")) != NULL){
            sscanf(pStr+7, "%ld", &scanNum);
            bSuccessfullyReadScan = true;
          }

          if ((pStr = strstr(chunk, "</spectrum>")) != NULL){
            if (bSuccessfullyReadScan){  // not that we've reached the next spectrum, store index offset
              if (bThermoFile)  // start scan count at 1 instead of 0
                 scanNum += 1;
              curIndex.scanNum = scanNum;
              curIndex.idRef = "";
              curIndex.offset = lOffset;
              m_vIndex.push_back(curIndex);
              break;
            }
            else{
              printf(" error, found \"<spectrum\" line before parsing index attribute of previous scan:  %s\n", pStr);
              return false;
            }
          }
          bReadingFirstSpectrum = false;
        } while (fgets(chunk, CHUNK, f));
      }
      lOffset = ftell(f);  // position of file pointer before fgets in loop
    }
  } else {
    readBytes = gzObj.extract(fptr, gzObj.getfilesize()-200, (unsigned char*)chunk, CHUNK);
  }

  return true;
}

void mzpSAXMzmlHandler::stopParser(){
  m_bStopParse=true;
  XML_StopParser(m_parser,false);

  //reset mzML flags
  m_bInmzArrayBinary = false;
  m_bInintenArrayBinary = false;
  m_bInRefGroup = false;
  m_bInSpectrumList=false;
  m_bInChromatogramList=false;
  m_bInIndexedMzML=false;
  m_bInIndexList=false;

  //reset other flags
  m_bSpectrumIndex=false;
}

int mzpSAXMzmlHandler::highChromat() {
  return (int)m_vChromatIndex.size();
}

int mzpSAXMzmlHandler::highScan() {
  if(m_vIndex.size()==0) return 0;
  return (int)m_vIndex[m_vIndex.size()-1].scanNum;
}

int mzpSAXMzmlHandler::lowScan() {
  if(m_vIndex.size()==0) return 0;
  return (int)m_vIndex[0].scanNum;
}

vector<cindex>* mzpSAXMzmlHandler::getChromatIndex(){
  return &m_vChromatIndex;
}

f_off mzpSAXMzmlHandler::getIndexOffset(){
  return indexOffset;
}

vector<instrumentInfo>* mzpSAXMzmlHandler::getInstrument(){
  return &m_vInstrument;
}

int mzpSAXMzmlHandler::getPeaksCount(){
  return m_peaksCount;
}

vector<cindex>* mzpSAXMzmlHandler::getSpecIndex(){
  return &m_vIndex;
}
