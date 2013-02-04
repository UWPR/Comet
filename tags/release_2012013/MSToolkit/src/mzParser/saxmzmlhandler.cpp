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
 * robust, yet generic, mzML pareser
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

//#include "stdafx.h"
//#include "saxmzmlhandler.h"
#include "mzParser.h"

SAXMzmlHandler::SAXMzmlHandler(BasicSpectrum* bs){
	m_bInmzArrayBinary = false;
	m_bInintenArrayBinary = false;
	m_bInRefGroup = false;
	m_bNetworkData = false; // always little-endian for mzML
	m_bLowPrecision = false;
	m_bInSpectrumList=false;
	m_bInChromatogramList=false;
	m_bInIndexedMzML=false;
	m_bInIndexList=false;
	m_bCompressedData=false;
	m_bHeaderOnly=false;
	m_bSpectrumIndex=false;
	m_bNoIndex=true;
	spec=bs;
	indexOffset=-1;
	m_scanPRECCount = 0;
	m_scanSPECCount = 0;
	m_scanIDXCount = 0;
}

SAXMzmlHandler::~SAXMzmlHandler(){
	spec=NULL;
}

void SAXMzmlHandler::startElement(const XML_Char *el, const XML_Char **attr){

	if (isElement("spectrumList",el)) {
		m_bInSpectrumList=true;

	} else if (isElement("binaryDataArrayList",el)) {
		if(m_bHeaderOnly) stopParser();

	} else if (isElement("chromatogramList",el)) {
		m_bInChromatogramList=true;

	} else if(isElement("index",el) && m_bInIndexList){
		if(!strcmp(getAttrValue("name", attr),"spectrum")) m_bSpectrumIndex=true;

	} else if (isElement("indexedmzML",el)) {
		m_vIndex.clear();
		m_bInIndexedMzML=true;

	} else if(isElement("indexList",el)) {
		m_bInIndexList=true;

	} else if(isElement("offset",el) && m_bSpectrumIndex){
		m_strData.clear();
		curIndex.idRef=string(getAttrValue("idRef", attr));
		if(strstr(&curIndex.idRef[0],"scan=")!=NULL)	{
			curIndex.scanNum=atoi(strstr(&curIndex.idRef[0],"scan=")+5);
		} else if(strstr(&curIndex.idRef[0],"scanId=")!=NULL) {
			curIndex.scanNum=atoi(strstr(&curIndex.idRef[0],"scanId=")+7);
		} else if(strstr(&curIndex.idRef[0],"S")!=NULL) {
			curIndex.scanNum=atoi(strstr(&curIndex.idRef[0],"S")+1);
		} else {
			curIndex.scanNum=++m_scanIDXCount;
			//Suppressing warning.
			//cout << "WARNING: Cannot extract scan number in index offset line: " << &curIndex.idRef[0] << "\tDefaulting to " << m_scanIDXCount << endl;
		}

	} else if(isElement("precursor",el)) {
		string s=getAttrValue("spectrumRef", attr);

		//if spectrumRef is not provided
		if(s.length()<1){
			spec->setPrecursorScanNum(0);
		} else {
			if(strstr(&s[0],"scan=")!=NULL)	{
				spec->setPrecursorScanNum(atoi(strstr(&s[0],"scan=")+5));
			} else if(strstr(&s[0],"scanId=")!=NULL) {
				spec->setPrecursorScanNum(atoi(strstr(&s[0],"scanId=")+7));
			} else if(strstr(&s[0],"S")!=NULL) {
				spec->setPrecursorScanNum(atoi(strstr(&s[0],"S")+1));
			} else {
				spec->setPrecursorScanNum(++m_scanPRECCount);
				//Suppressing warning.
				//cout << "WARNING: Cannot extract precursor scan number spectrum line: " << &s[0] << "\tDefaulting to " << m_scanPRECCount << endl;
			}
		}

	}	else if (isElement("referenceableParamGroup", el)) {
		const char* groupName = getAttrValue("id", attr);
		m_ccurrentRefGroupName = string(groupName);
		m_bInRefGroup = true;

	} else if (isElement("run", el)){
		stopParser();

	}	else if (isElement("softwareParam", el)) {
		const char* name = getAttrValue("name", attr);
		const char* accession = getAttrValue("accession", attr);
		const char* version = getAttrValue("version", attr);

	}	else if (isElement("spectrum", el)) {
		string s=getAttrValue("id", attr);
		if(strstr(&s[0],"scan=")!=NULL)	{
			spec->setScanNum(atoi(strstr(&s[0],"scan=")+5));
		} else if(strstr(&s[0],"scanId=")!=NULL) {
			spec->setScanNum(atoi(strstr(&s[0],"scanId=")+7));
		} else if(strstr(&s[0],"S")!=NULL) {
			spec->setScanNum(atoi(strstr(&s[0],"S")+1));
		} else {
			spec->setScanNum(++m_scanSPECCount);
			//Suppressing warning.
			//cout << "WARNING: Cannot extract scan number spectrum line: " << &s[0] << "\tDefaulting to " << m_scanSPECCount << endl;
		}
		m_peaksCount = atoi(getAttrValue("defaultArrayLength", attr));
		spec->setPeaksCount(m_peaksCount);

	} else if (isElement("spectrumList",el)) {
		m_bInSpectrumList=true;

	}	else if (isElement("cvParam", el)) {
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
		}	else {
			processCVParam(name,accession,value,unitName,unitAccession);
		}

	}	else if (isElement("referenceableParamGroupRef", el))	{
		const char* groupName = getAttrValue("ref", attr);
		for (unsigned int i=0;i<m_refGroupCvParams.size();i++)	{
			if( strcmp(groupName,&m_refGroupCvParams[i].refGroupName[0])==0 )	{
				processCVParam(&m_refGroupCvParams[i].name[0], &m_refGroupCvParams[i].accession[0], &m_refGroupCvParams[i].value[0], &m_refGroupCvParams[i].unitName[0], &m_refGroupCvParams[i].unitAccession[0]);
			}
		}
	}

	if(isElement("binary", el))	{
		m_strData.clear();
	}

}


void SAXMzmlHandler::endElement(const XML_Char *el) {

	if(isElement("binary", el))	{
		processData();
		m_bInintenArrayBinary = false;
		m_bInmzArrayBinary = false;
		m_strData.clear();

	} else if(isElement("chromatogramList",el)) {
		m_bInChromatogramList=false;

	} else if(isElement("componentList",el)) {
		m_vInstrument.push_back(m_instrument);

	} else if(isElement("index",el)){
		m_bSpectrumIndex=false;

	} else if(isElement("indexList",el)){
		m_bInIndexList=false;
		stopParser();

	} else if(isElement("offset",el) && m_bSpectrumIndex){
		curIndex.offset=mzpatoi64(&m_strData[0]);
		m_vIndex.push_back(curIndex);

	} else if(isElement("precursorList",el)){
		
	} else if (isElement("referenceableParamGroup", el)) {
		m_bInRefGroup = false;

	}	else if(isElement("spectrum", el)) {
		pushSpectrum();
		stopParser();
		
	} else if(isElement("spectrumList",el)) {
		m_bInSpectrumList = false;

	}
}

void SAXMzmlHandler::characters(const XML_Char *s, int len) {
	m_strData.append(s, len);
}

void SAXMzmlHandler::processCVParam(const char* name, const char* accession, const char* value, const char* unitName, const char* unitAccession)
{
	if(!strcmp(name, "32-bit float") || !strcmp(accession,"MS:1000521"))	{
		m_bLowPrecision = true;

	}	else if(!strcmp(name, "64-bit float") || !strcmp(accession,"MS:1000523"))	{
		m_bLowPrecision = false;

	}	else if(!strcmp(name, "base peak intensity") || !strcmp(accession,"MS:1000505"))	{
		spec->setBasePeakIntensity(atof(value));

	}	else if(!strcmp(name, "base peak m/z") || !strcmp(accession,"MS:1000504"))	{
		spec->setBasePeakMZ(atof(value));

	}	else if(!strcmp(name, "charge state") || !strcmp(accession,"MS:1000041"))	{
		spec->setPrecursorCharge(atoi(value));

	}	else if(!strcmp(name, "collision-induced dissociation") || !strcmp(accession,"MS:1000133"))	{
		if(spec->getActivation()==ETD) spec->setActivation(ETDSA);
		else spec->setActivation(CID);

	}	else if(!strcmp(name, "collision energy") || !strcmp(accession,"MS:1000045"))	{
		spec->setCollisionEnergy(atof(value));

	} else if(!strcmp(name,"electron multiplier") || !strcmp(accession,"MS:1000253")) {
		m_instrument.detector=name;

	}	else if(!strcmp(name, "electron transfer dissociation") || !strcmp(accession,"MS:1000133"))	{
		if(spec->getActivation()==CID) spec->setActivation(ETDSA);
		else spec->setActivation(ETDSA);

	}	else if(!strcmp(name, "FAIMS compensation voltage") || !strcmp(accession,"MS:1001581"))	{
		spec->setCompensationVoltage(atof(value));

	}	else if(!strcmp(name, "highest observed m/z") || !strcmp(accession,"MS:1000527"))	{
		spec->setHighMZ(atof(value));

	} else if(!strcmp(name,"inductive detector") || !strcmp(accession,"MS:1000624")) {
		m_instrument.detector=name;

	}	else if(!strcmp(name, "intensity array") || !strcmp(accession,"MS:1000515"))	{
		m_bInintenArrayBinary = true;
		m_bInmzArrayBinary = false;

	} else if(!strcmp(name,"LTQ Velos") || !strcmp(accession,"MS:1000855")) {
		m_instrument.model=name;

	}	else if(!strcmp(name, "lowest observed m/z") || !strcmp(accession,"MS:1000528"))	{
		spec->setLowMZ(atof(value));

	} else	if( !strcmp(name, "MS1 spectrum") || !strcmp(accession,"MS:1000579") ){
		spec->setMSLevel(1);

	} else	if( !strcmp(name, "ms level") || !strcmp(accession,"MS:1000511") ){
		spec->setMSLevel(atoi(value));

	}	else if(!strcmp(name, "m/z array") || !strcmp(accession,"MS:1000514"))	{
		m_bInmzArrayBinary = true;
		m_bInintenArrayBinary = false;

	} else if(!strcmp(name,"nanoelectrospray") || !strcmp(accession,"MS:1000398")) {
		m_instrument.ionization=name;

	} else if(!strcmp(name,"orbitrap") || !strcmp(accession,"MS:1000484")) {
		m_instrument.analyzer=name;

	} else if(!strcmp(name,"peak intensity") || !strcmp(accession,"MS:1000042")) {
		spec->setPrecursorIntensity(atof(value));

	} else if(!strcmp(name,"positive scan") || !strcmp(accession,"MS:1000130")) {
		spec->setPositiveScan(true);

	} else if(!strcmp(name,"profile spectrum") || !strcmp(accession,"MS:1000128")) {
		spec->setCentroid(false);

	} else if(!strcmp(name,"radial ejection linear ion trap") || !strcmp(accession,"MS:1000083")) {
		m_instrument.analyzer=name;

	}	else if(!strcmp(name, "scan start time") || !strcmp(accession,"MS:1000016"))	{
		if(!strcmp(unitName, "minute") || !strcmp(unitAccession,"UO:0000031"))	{
			spec->setRTime((float)atof(value));
		} else {
			spec->setRTime((float)atof(value)/60.0f); //assume if not minutes, then seconds
		}

	}	else if(!strcmp(name, "selected ion m/z") || !strcmp(accession,"MS:1000744"))	{
		spec->setPrecursorMZ(atof(value));

	}	else if(!strcmp(name, "total ion current") || !strcmp(accession,"MS:1000285"))	{
		spec->setTotalIonCurrent(atof(value));

	} else if(!strcmp(name,"Thermo RAW file") || !strcmp(accession,"MS:1000563")) {
		m_instrument.manufacturer="Thermo Scientific";

	}	else if(!strcmp(name, "zlib compression") || !strcmp(accession,"MS:1000574"))	{
		m_bCompressedData=true;
	}
}

void SAXMzmlHandler::processData()
{
	if(m_bInmzArrayBinary) {
		if(m_bLowPrecision && !m_bCompressedData) decode32(vdM);
		else if(m_bLowPrecision && m_bCompressedData) decompress32(vdM);
		else if(!m_bLowPrecision && !m_bCompressedData) decode64(vdM);
		else decompress64(vdM);
	} else if(m_bInintenArrayBinary) {
		if(m_bLowPrecision && !m_bCompressedData) decode32(vdI);
		else if(m_bLowPrecision && m_bCompressedData) decompress32(vdI);
		else if(!m_bLowPrecision && !m_bCompressedData) decode64(vdI);
		else decompress64(vdI);
	}
	m_bCompressedData=false;
}

bool SAXMzmlHandler::readHeader(int num){
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
	int mid=m_vIndex.size()/2;
	int upper=m_vIndex.size();
	int lower=0;
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
		if(spec->getScanNum()!=m_vIndex[mid].scanNum) spec->setScanNum(m_vIndex[mid].scanNum);
		spec->setScanIndex(mid+1); //set the index, which starts from 1, so offset by 1
		m_bHeaderOnly=false;
		posIndex=mid;
		return true;
	}
	return false;

}

bool SAXMzmlHandler::readSpectrum(int num){
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
	int mid=m_vIndex.size()/2;
	int upper=m_vIndex.size();
	int lower=0;
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
			if(spec->getScanNum()!=m_vIndex[mid].scanNum) spec->setScanNum(m_vIndex[mid].scanNum);
			spec->setScanIndex(mid+1); //set the index, which starts from 1, so offset by 1
			posIndex=mid;
			return true;
		}
	//}
	return false;
}

void SAXMzmlHandler::pushSpectrum(){

	specDP dp;
	for(unsigned int i=0;i<vdM.size();i++)	{
		dp.mz = vdM[i];
		dp.intensity = vdI[i];
		spec->addDP(dp);
	}
	
}

void SAXMzmlHandler::decompress32(vector<double>& d){

	d.clear();
	if(m_peaksCount < 1) return;
	
	union udata {
		float f;
		uint32_t i;
	} uData;

	uLong uncomprLen;
	uint32_t* data;
	int length;
	const char* pData = m_strData.data();
	size_t stringSize = m_strData.size();
	size_t size = m_peaksCount * 2 * sizeof(uint32_t);
	
	//Decode base64
	char* pDecoded = (char*) new char[size];
	memset(pDecoded, 0, size);
	length = b64_decode_mio( (char*) pDecoded , (char*) pData, stringSize );
	pData=NULL;

	//zLib decompression
	data = new uint32_t[m_peaksCount*2];
	uncomprLen = m_peaksCount * 2 * sizeof(uint32_t);
	uncompress((Bytef*)data, &uncomprLen, (const Bytef*)pDecoded, length);
	delete [] pDecoded;

	//write data to arrays
	for(int i=0;i<m_peaksCount;i++){
		uData.i = dtohl(data[i], m_bNetworkData);
		d.push_back((double)uData.f);
	}
	delete [] data;
}

void SAXMzmlHandler::decompress64(vector<double>& d){

	d.clear();
	if(m_peaksCount < 1) return;
	
	union udata {
		double d;
		uint64_t i;
	} uData;

	uLong uncomprLen;
	uint64_t* data;
	int length;
	const char* pData = m_strData.data();
	size_t stringSize = m_strData.size();
	size_t size = m_peaksCount * 2 * sizeof(uint64_t);
	
	//Decode base64
	char* pDecoded = (char*) new char[size];
	memset(pDecoded, 0, size);
	length = b64_decode_mio( (char*) pDecoded , (char*) pData, stringSize );
	pData=NULL;

	//zLib decompression
	data = new uint64_t[m_peaksCount*2];
	uncomprLen = m_peaksCount * 2 * sizeof(uint64_t);
	uncompress((Bytef*)data, &uncomprLen, (const Bytef*)pDecoded, length);
	delete [] pDecoded;

	//write data to arrays
	for(int i=0;i<m_peaksCount;i++){
		uData.i = dtohl(data[i], m_bNetworkData);
		d.push_back(uData.d);
	}
	delete [] data;

}

void SAXMzmlHandler::decode32(vector<double>& d){
// This code block was revised so that it packs floats correctly
// on both 64 and 32 bit machines, by making use of the uint32_t
// data type. -S. Wiley
	const char* pData = m_strData.data();
	size_t stringSize = m_strData.size();
	
	size_t size = m_peaksCount * sizeof(uint32_t);
	char* pDecoded = (char *) new char[size];
	memset(pDecoded, 0, size);

	if(m_peaksCount > 0) {
		// Base64 decoding
		// By comparing the size of the unpacked data and the expected size
		// an additional check of the data file integrity can be performed
		int length = b64_decode_mio( (char*) pDecoded , (char*) pData, stringSize );
		if(length != size) {
			cout << " decoded size " << length << " and required size " << (unsigned long)size << " dont match:\n";
			cout << " Cause: possible corrupted file.\n";
			exit(EXIT_FAILURE);
		}
	}

	// And byte order correction
	union udata {
		float fData;
		uint32_t iData;  
	} uData; 

	int n = 0;
	uint32_t* pDecodedInts = (uint32_t*)pDecoded; // cast to uint_32 for reading int sized chunks
	d.clear();
	for(int i = 0; i < m_peaksCount; i++) {
		uData.iData = dtohl(pDecodedInts[n++], m_bNetworkData);
		d.push_back((double)uData.fData);
	}

	// Free allocated memory
	delete[] pDecoded;
}

void SAXMzmlHandler::decode64(vector<double>& d){

// This code block was revised so that it packs floats correctly
// on both 64 and 32 bit machines, by making use of the uint32_t
// data type. -S. Wiley
	const char* pData = m_strData.data();
	size_t stringSize = m_strData.size();

	size_t size = m_peaksCount * sizeof(uint64_t);
	char* pDecoded = (char *) new char[size];
	memset(pDecoded, 0, size);

	if(m_peaksCount > 0) {
		// Base64 decoding
		// By comparing the size of the unpacked data and the expected size
		// an additional check of the data file integrity can be performed
		int length = b64_decode_mio( (char*) pDecoded , (char*) pData, stringSize );
		if(length != size) {
			cout << " decoded size " << length << " and required size " << (unsigned long)size << " dont match:\n";
			cout << " Cause: possible corrupted file.\n";
			exit(EXIT_FAILURE);
		}
	}

	// And byte order correction
	union udata {
		double fData;
		uint64_t iData;  
	} uData; 

	d.clear();
	int n = 0;
	uint64_t* pDecodedInts = (uint64_t*)pDecoded; // cast to uint_64 for reading int sized chunks
	for(int i = 0; i < m_peaksCount; i++) {
		uData.iData = dtohl(pDecodedInts[n++], m_bNetworkData);
		d.push_back(uData.fData);
	}

	// Free allocated memory
	delete[] pDecoded;
}

unsigned long SAXMzmlHandler::dtohl(uint32_t l, bool bNet) {

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

uint64_t SAXMzmlHandler::dtohl(uint64_t l, bool bNet) {

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
f_off SAXMzmlHandler::readIndexOffset() {

	char buffer[200];
	char chunk[CHUNK];
	char* start;
	char* stop;
	int readBytes;

	if(!m_bGZCompression){
		FILE* f=fopen(&m_strFileName[0],"r");
		mzpfseek(f,-200,SEEK_END);
		fread(buffer,1,200,f);
		fclose(f);
		start=strstr(buffer,"<indexListOffset>");
		stop=strstr(buffer,"</indexListOffset>");
	} else {
		readBytes = gzObj.extract(fptr, gzObj.getfilesize()-200, (unsigned char*)chunk, CHUNK);
		start=strstr(chunk,"<indexListOffset>");
		stop=strstr(chunk,"</indexListOffset>");
	}

	if(start==NULL || stop==NULL) {
		cout << "No index list offset found. Reading this file will be painfully slow." << endl;
		return 0;
	}

	char offset[64];
	int len=(int)(stop-start-17);
	strncpy(offset,start+17,len);
	offset[len]='\0';
	return mzpatoi64(offset);

}

bool SAXMzmlHandler::load(const char* fileName){
	if(!open(fileName)) return false;
	m_vInstrument.clear();
	parseOffset(0);
	indexOffset = readIndexOffset();
	if(indexOffset==0){
		m_bNoIndex=true;
	} else {
		m_bNoIndex=false;
		parseOffset(indexOffset);
		posIndex=-1;
	}
	return true;
}


void SAXMzmlHandler::stopParser(){
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

int SAXMzmlHandler::highScan() {
	if(m_vIndex.size()==0) return 0;
	return m_vIndex[m_vIndex.size()-1].scanNum;
}

int SAXMzmlHandler::lowScan() {
	if(m_vIndex.size()==0) return 0;
	return m_vIndex[0].scanNum;
}

f_off SAXMzmlHandler::getIndexOffset(){
	return indexOffset;
}

vector<cindex>* SAXMzmlHandler::getIndex(){
	return &m_vIndex;
}

vector<instrumentInfo>* SAXMzmlHandler::getInstrument(){
	return &m_vInstrument;
}

int SAXMzmlHandler::getPeaksCount(){
	return m_peaksCount;
}
