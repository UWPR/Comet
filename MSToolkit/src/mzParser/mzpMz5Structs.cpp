/*
mzpMz5Structs - The code is
open source under the FreeBSD License, please see LICENSE file
for detailed information.

Copyright (C) 2011, Mike Hoopmann, Institute for Systems Biology
Version 1.0, January 4, 2011.
Version 1.1, March 14, 2012.
*/

#include "mzParser.h"
#ifdef MZP_HDF

using namespace mzParser;
using namespace H5;
using namespace std;

StrType getStringType() {
	StrType stringtype(PredType::C_S1, H5T_VARIABLE);
	return stringtype;
}

StrType getFStringType(const size_t len) {
	StrType stringtype(PredType::C_S1, len);
	return stringtype;
}

CompType BinaryDataMZ5::getType() {
	CompType ret(sizeof(BinaryDataMZ5));
	size_t offset = 0;
	ret.insertMember("xParams", offset, ParamListMZ5::getType());
	offset += ParamListMZ5::getType().getSize();
	ret.insertMember("yParams", offset, ParamListMZ5::getType());
	offset += ParamListMZ5::getType().getSize();
	ret.insertMember("xrefDataProcessing", offset, RefMZ5::getType());
	offset += RefMZ5::getType().getSize();
	ret.insertMember("yrefDataProcessing", offset, RefMZ5::getType());
	return ret;
}

CompType ChromatogramMZ5::getType() {
	CompType ret(sizeof(ChromatogramMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("precursor", offset, PrecursorMZ5::getType());
	offset += sizeof(PrecursorMZ5);
	ret.insertMember("productIsolationWindow", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("refDataProcessing", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("index", offset, PredType::NATIVE_ULONG);
	offset += sizeof(unsigned long);
	return ret;
}

VarLenType ComponentListMZ5::getType() {
	CompType c = ComponentMZ5::getType();
	VarLenType ret(&c);
	return ret;
}

CompType ComponentMZ5::getType() {
	CompType ret(sizeof(ComponentMZ5));
	size_t offset = 0;
	ret.insertMember("paramList", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("order", offset, PredType::NATIVE_ULONG);
	return ret;
}

CompType ComponentsMZ5::getType() {
	CompType ret(sizeof(ComponentsMZ5));
	size_t offset = 0;
	ret.insertMember("sources", offset, ComponentListMZ5::getType());
	offset += sizeof(ComponentListMZ5);
	ret.insertMember("analyzers", offset, ComponentListMZ5::getType());
	offset += sizeof(ComponentListMZ5);
	ret.insertMember("detectors", offset, ComponentListMZ5::getType());
	return ret;
}

CompType ContVocabMZ5::getType() {
	CompType cvtype(sizeof(ContVocabMZ5Data));
	StrType stringtype = getStringType();
	cvtype.insertMember("uri", HOFFSET(ContVocabMZ5Data, uri), stringtype);
	cvtype.insertMember("fullname", HOFFSET(ContVocabMZ5Data, fullname), stringtype);
	cvtype.insertMember("id", HOFFSET(ContVocabMZ5Data, id), stringtype);
	cvtype.insertMember("version", HOFFSET(ContVocabMZ5Data, version), stringtype);
	return cvtype;
}

CVParamMZ5::CVParamMZ5() {
	init(0, ULONG_MAX, ULONG_MAX);
}

CVParamMZ5::CVParamMZ5(const CVParamMZ5& cvparam) {
	init(cvparam.value, cvparam.typeCVRefID, cvparam.unitCVRefID);
}

CVParamMZ5& CVParamMZ5::operator=(const CVParamMZ5& rhs) {
	if (this != &rhs) init(rhs.value, rhs.typeCVRefID, rhs.unitCVRefID);
	return *this;
}

CVParamMZ5::~CVParamMZ5(){}

CompType CVParamMZ5::getType() {
	CompType ret(sizeof(CVParamMZ5Data));
	StrType stringtype = getFStringType(CVL);
	ret.insertMember("value", HOFFSET(CVParamMZ5Data, value), stringtype);
	ret.insertMember("cvRefID", HOFFSET(CVParamMZ5Data, typeCVRefID), PredType::NATIVE_ULONG);
	ret.insertMember("uRefID", HOFFSET(CVParamMZ5Data, unitCVRefID), PredType::NATIVE_ULONG);
	return ret;
}

void CVParamMZ5::init(const char* value, const unsigned long& cvrefid, const unsigned long& urefid) {
	if (value) strcpy(this->value, value);
	else this->value[0] = '\0';
	this->value[CVL - 1] = '\0';
	this->typeCVRefID = cvrefid;
	this->unitCVRefID = urefid;
}

CVRefMZ5::CVRefMZ5() {
	init("","",ULONG_MAX);
}

CVRefMZ5::CVRefMZ5(const CVRefMZ5& cvref) {
	init(cvref.name, cvref.prefix, cvref.accession);
}

CVRefMZ5::~CVRefMZ5() {
	delete [] name;
	delete [] prefix;
}

CVRefMZ5& CVRefMZ5::operator=(const CVRefMZ5& cvp) {
	if (this != &cvp){
		delete [] name;
		delete [] prefix;
		init(cvp.name, cvp.prefix, cvp.accession);
	}
	return *this;
}

void CVRefMZ5::init(const char* name, const char* prefix,	const unsigned long accession){
	if(name) {
		this->name = new char[strlen(name)+1];
		strcpy(this->name,name);
	}
	if(prefix){
		this->prefix = new char[strlen(prefix) + 1];
		strcpy(this->prefix,prefix);
	}
	this->accession=accession;
}

CompType CVRefMZ5::getType() {
	CompType ret(sizeof(CVRefMZ5Data));
	StrType stringtype = getStringType();
	ret.insertMember("name", HOFFSET(CVRefMZ5Data, name), stringtype);
	ret.insertMember("prefix", HOFFSET(CVRefMZ5Data, prefix), stringtype);
	ret.insertMember("accession", HOFFSET(CVRefMZ5Data, accession),	PredType::NATIVE_ULONG);
	return ret;
}

CompType DataProcessingMZ5::getType() {
	CompType ret(sizeof(DataProcessingMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("method", offset, ProcessingMethodListMZ5::getType());
	offset += sizeof(ProcessingMethodListMZ5);
	return ret;
}

FileInformationMZ5::FileInformationMZ5() {
	this->majorVersion = MZ5_FILE_MAJOR_VERSION;
	this->minorVersion = MZ5_FILE_MINOR_VERSION;
	this->didFiltering = 1;
	this->deltaMZ = 1;
	this->translateInten = 1;
}

FileInformationMZ5::FileInformationMZ5(const FileInformationMZ5& rhs) {
	init(rhs.majorVersion, rhs.minorVersion, rhs.didFiltering, rhs.deltaMZ, rhs.translateInten);
}

FileInformationMZ5::FileInformationMZ5(const mzpMz5Config& c) {
	unsigned short didfiltering = c.doFiltering() ? 1 : 0;
	unsigned short deltamz = c.doTranslating() ? 1 : 0;
	unsigned short translateinten = c.doTranslating() ? 1 : 0;
	init(MZ5_FILE_MAJOR_VERSION, MZ5_FILE_MINOR_VERSION, didfiltering, deltamz,	translateinten);
}

FileInformationMZ5::~FileInformationMZ5(){}

FileInformationMZ5& FileInformationMZ5::operator=(const FileInformationMZ5& rhs) {
	if (this != &rhs){
		init(rhs.majorVersion, rhs.minorVersion, rhs.didFiltering, rhs.deltaMZ, rhs.translateInten);
	}
	return *this;
}

void FileInformationMZ5::init(const unsigned short majorVersion, const unsigned short minorVersion, const unsigned didFiltering, const unsigned deltaMZ, const unsigned translateInten) {
	this->majorVersion = majorVersion;
	this->minorVersion = minorVersion;
	this->didFiltering = didFiltering;
	this->deltaMZ = deltaMZ;
	this->translateInten = translateInten;
}

CompType FileInformationMZ5::getType() {
	CompType ret(sizeof(FileInformationMZ5Data));
	ret.insertMember("majorVersion", HOFFSET(FileInformationMZ5Data, majorVersion), PredType::NATIVE_USHORT);
	ret.insertMember("minorVersion", HOFFSET(FileInformationMZ5Data, minorVersion), PredType::NATIVE_USHORT);
	ret.insertMember("didFiltering", HOFFSET(FileInformationMZ5Data, didFiltering), PredType::NATIVE_USHORT);
	ret.insertMember("deltaMZ", HOFFSET(FileInformationMZ5Data, deltaMZ), PredType::NATIVE_USHORT);
	ret.insertMember("translateInten", HOFFSET(FileInformationMZ5Data, translateInten), PredType::NATIVE_USHORT);
	return ret;
}

CompType InstrumentConfigurationMZ5::getType() {
	CompType ret(sizeof(InstrumentConfigurationMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("components", offset, ComponentsMZ5::getType());
	offset += sizeof(ComponentsMZ5);
	ret.insertMember("refScanSetting", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("refSoftware", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	return ret;
}

CompType ParamGroupMZ5::getType() {
	CompType ret(sizeof(ParamGroupMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	return ret;
}

ParamListMZ5::ParamListMZ5(){
	init(0,0,0,0,0,0);
}

ParamListMZ5::ParamListMZ5(const ParamListMZ5& pl){
	init(pl.cvParamStartID,pl.cvParamEndID,pl.userParamStartID,pl.userParamEndID,pl.refParamGroupStartID,pl.refParamGroupEndID);
}

ParamListMZ5& ParamListMZ5::operator=(const ParamListMZ5& pl){
	if(this != &pl) init(pl.cvParamStartID, pl.cvParamEndID, pl.userParamStartID, pl.userParamEndID, pl.refParamGroupStartID, pl.refParamGroupEndID);
	return *this;
}

ParamListMZ5::~ParamListMZ5() {}

CompType ParamListMZ5::getType() {
	CompType ret(sizeof(ParamListMZ5Data));
	ret.insertMember("cvstart", HOFFSET(ParamListMZ5Data, cvParamStartID), PredType::NATIVE_ULONG);
	ret.insertMember("cvend", HOFFSET(ParamListMZ5Data, cvParamEndID), PredType::NATIVE_ULONG);
	ret.insertMember("usrstart", HOFFSET(ParamListMZ5Data, userParamStartID), PredType::NATIVE_ULONG);
	ret.insertMember("usrend", HOFFSET(ParamListMZ5Data, userParamEndID), PredType::NATIVE_ULONG);
	ret.insertMember("refstart", HOFFSET(ParamListMZ5Data, refParamGroupStartID), PredType::NATIVE_ULONG);
	ret.insertMember("refend", HOFFSET(ParamListMZ5Data, refParamGroupEndID), PredType::NATIVE_ULONG);
	return ret;
}

void ParamListMZ5::init(const unsigned long cvstart, const unsigned long cvend, const unsigned long usrstart, const unsigned long usrend, const unsigned long refstart, const unsigned long refend) {
	cvParamStartID=cvstart;
	cvParamEndID=cvend;
	userParamStartID=usrstart;
	userParamEndID=usrend;
	refParamGroupStartID=refstart;
	refParamGroupEndID=refend;
}

ParamListsMZ5::ParamListsMZ5(){
	init(NULL,0);
}

ParamListsMZ5::ParamListsMZ5(const ParamListsMZ5& pl){
	init(pl.lists,pl.len);
}

ParamListsMZ5& ParamListsMZ5::operator=(const ParamListsMZ5& pl){
	if(this!=&pl) {
		if(lists!=NULL) delete [] lists;
		init(pl.lists, pl.len);
	}
	return *this;
}

ParamListsMZ5::~ParamListsMZ5() {
	if(lists!=NULL) delete [] lists;
}

VarLenType ParamListsMZ5::getType() {
	CompType c(ParamListMZ5::getType());
	VarLenType ret(&c);
	return ret;
}

void ParamListsMZ5::init(const ParamListMZ5* list, const size_t len){
	this->len=len;
	if(len>0){
		lists=new ParamListMZ5[len];
		for(size_t a=0;a<len;a++) lists[a]=list[a];
	} else lists=NULL;
}


PrecursorListMZ5::PrecursorListMZ5(){
	init(NULL,0);
}

PrecursorListMZ5::PrecursorListMZ5(const PrecursorListMZ5& p){
	init(p.list,p.len);
}

PrecursorListMZ5& PrecursorListMZ5::operator=(const PrecursorListMZ5& p){
	if(this!=&p){
		if(list!=NULL) delete [] list;
		list=NULL;
		init(p.list,p.len);
	} 
	return *this;
}

PrecursorListMZ5::~PrecursorListMZ5(){
	if(list!=NULL) delete [] list;
}

VarLenType PrecursorListMZ5::getType() {
	CompType c(PrecursorMZ5::getType());
	VarLenType ret(&c);
	return ret;
}

void PrecursorListMZ5::init(const PrecursorMZ5* list, const size_t len){
	this->len=len;
	if(len>0){
		this->list=new PrecursorMZ5[len];
		for(size_t a=0;a<len;a++) this->list[a]=list[a];
	} else this->list=NULL;
}

PrecursorMZ5::PrecursorMZ5() {
	init(ParamListMZ5(), ParamListMZ5(), ParamListsMZ5(),RefMZ5(),RefMZ5(),"");
}

PrecursorMZ5::PrecursorMZ5(const PrecursorMZ5& precursor) {
	init(precursor.activation,precursor.isolationWindow,precursor.selectedIonList,precursor.spectrumRefID,precursor.sourceFileRefID,precursor.externalSpectrumId);
}

PrecursorMZ5& PrecursorMZ5::operator=(const PrecursorMZ5& pre) {
	if (this != &pre) {
		delete [] externalSpectrumId;
		init(pre.activation, pre.isolationWindow, pre.selectedIonList, pre.spectrumRefID, pre.sourceFileRefID, pre.externalSpectrumId);
	}
	return *this;
}

PrecursorMZ5::~PrecursorMZ5() {
	delete [] externalSpectrumId;
}

CompType PrecursorMZ5::getType() {
	CompType ret(sizeof(PrecursorMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("externalSpectrumId", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("activation", offset, ParamListMZ5::getType());
	offset += ParamListMZ5::getType().getSize();
	ret.insertMember("isolationWindow", offset, ParamListMZ5::getType());
	offset += ParamListMZ5::getType().getSize();
	ret.insertMember("selectedIonList", offset, ParamListsMZ5::getType());
	offset += ParamListsMZ5::getType().getSize();
	ret.insertMember("refSpectrum", offset, RefMZ5::getType());
	offset += RefMZ5::getType().getSize();
	ret.insertMember("refSourceFile", offset, RefMZ5::getType());
	offset += RefMZ5::getType().getSize();
	return ret;
}

void PrecursorMZ5::init(const ParamListMZ5& activation, const ParamListMZ5& isolationWindow, const ParamListsMZ5 selectedIonList, const RefMZ5& refSpectrum, const RefMZ5& refSourceFile, const char* externalSpectrumId){
	this->externalSpectrumId=new char[strlen(externalSpectrumId)+1];
	strcpy(this->externalSpectrumId,externalSpectrumId);
	this->activation=activation;
	this->isolationWindow=isolationWindow;
	this->selectedIonList=selectedIonList;
	this->spectrumRefID=refSpectrum;
	this->sourceFileRefID=refSourceFile;
}

VarLenType ProcessingMethodListMZ5::getType() {
	CompType c = ProcessingMethodMZ5::getType();
	VarLenType ret(&c);
	return ret;
}

CompType ProcessingMethodMZ5::getType() {
	CompType ret(sizeof(ProcessingMethodMZ5));
	size_t offset = 0;
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("refSoftware", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("order", offset, PredType::NATIVE_ULONG);
	return ret;
}

VarLenType RefListMZ5::getType() {
	CompType c = RefMZ5::getType();
	VarLenType ret(&c);
	return ret;
}

RefMZ5::RefMZ5(){
	refID = ULONG_MAX;
}

RefMZ5::RefMZ5(const RefMZ5& ref) {
	refID=ref.refID;
}

RefMZ5& RefMZ5::operator=(const RefMZ5& ref) {
	if (this != &ref) refID = ref.refID;
	return *this;
}

RefMZ5::~RefMZ5(){}

CompType RefMZ5::getType() {
	CompType ret(sizeof(RefMZ5Data));
	ret.insertMember("refID", HOFFSET(RefMZ5Data, refID), PredType::NATIVE_ULONG);
	return ret;
}

CompType RunMZ5::getType() {
	CompType ret(sizeof(RunMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("startTimeStamp", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("fid", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("facc", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5);
	ret.insertMember("refSpectrumDP", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5);
	ret.insertMember("refChromatogramDP", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5);
	ret.insertMember("refDefaultInstrument", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5);
	ret.insertMember("refSourceFile", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5);
	ret.insertMember("refSample", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5);
	return ret;
}

CompType SampleMZ5::getType() {
	CompType ret(sizeof(SampleMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("name", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	return ret;
}

ScanListMZ5::ScanListMZ5(){
	init(NULL,0);
}

ScanListMZ5::ScanListMZ5(const ScanListMZ5& s){
	init(s.list,s.len);
}

//ScanListMZ5::ScanListMZ5(const std::vector<ScanMZ5>& s){
//	len=s.size();
//	if(len>0){
//		list=new ScanMZ5[len];
//		for(size_t a=0;a<s.size();a++) list[a]=s[a];
//	} else list=NULL;
//}

ScanListMZ5& ScanListMZ5::operator=(const ScanListMZ5& s){
	if(this!=&s){
		if(list!=NULL){
			delete [] list;
			list=NULL;
		}
		init(s.list,s.len);
	}
	return *this;
}

ScanListMZ5::~ScanListMZ5(){
	if(list!=NULL) delete [] list;
}

VarLenType ScanListMZ5::getType() {
	CompType c = ScanMZ5::getType();
	VarLenType ret(&c);
	return ret;
}

void ScanListMZ5::init(const ScanMZ5* list, const size_t len){
	this->len=len;
	if(len>0){
		this->list = new ScanMZ5[len];
		for(size_t a=0;a<len;a++) this->list[a]=list[a];
	} else this->list=NULL;
}

ScanMZ5::ScanMZ5(){
	init(ParamListMZ5(),ParamListsMZ5(),RefMZ5(),RefMZ5(),RefMZ5(),"");
}

ScanMZ5::ScanMZ5(const ScanMZ5& s){
	init(s.paramList,s.scanWindowList,s.instrumentConfigurationRefID,s.sourceFileRefID,s.spectrumRefID,s.externalSpectrumID);
}

ScanMZ5& ScanMZ5::operator=(const ScanMZ5& s){
	if(this!=&s){
		delete [] externalSpectrumID;
		init(s.paramList, s.scanWindowList, s.instrumentConfigurationRefID, s.sourceFileRefID, s.spectrumRefID, s.externalSpectrumID);
	} 
	return *this;
}

ScanMZ5::~ScanMZ5(){
	delete[] externalSpectrumID;
}

CompType ScanMZ5::getType() {
	CompType ret(sizeof(ScanMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("externalSpectrumID", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("scanWindowList", offset, ParamListsMZ5::getType());
	offset += sizeof(ParamListsMZ5);
	ret.insertMember("refInstrumentConfiguration", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("refSourceFile", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("refSpectrum", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	return ret;
}

void ScanMZ5::init(const ParamListMZ5& params, const ParamListsMZ5& scanWindowList, const RefMZ5& refInstrument, const RefMZ5& refSourceFile, const RefMZ5& refSpectrum, const char* externalSpectrumID){
	this->externalSpectrumID=new char[strlen(externalSpectrumID)+1];
	strcpy(this->externalSpectrumID, externalSpectrumID);
	this->paramList=params;
	this->scanWindowList=scanWindowList;
	this->instrumentConfigurationRefID=refInstrument;
	this->sourceFileRefID=refSourceFile;
	this->spectrumRefID=refSpectrum;
}

ScansMZ5::ScansMZ5(){
	init(ParamListMZ5(),ScanListMZ5());
}

ScansMZ5::ScansMZ5(const ScansMZ5& s){
	init(s.paramList,s.scanList);
}

ScansMZ5& ScansMZ5::operator=(const ScansMZ5&s) {
	if(this!=&s){
		init(s.paramList,s.scanList);
	}
	return *this;
}

ScansMZ5::~ScansMZ5(){}

CompType ScansMZ5::getType() {
	CompType ret(sizeof(ScansMZ5));
	size_t offset = 0;
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5);
	ret.insertMember("scanList", offset, ScanListMZ5::getType());
	return ret;
}

void ScansMZ5::init(const ParamListMZ5& params, const ScanListMZ5& scanList){
	this->paramList=params;
	this->scanList=scanList;
}

CompType ScanSettingMZ5::getType() {
	CompType ret(sizeof(ScanSettingMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("refSourceFiles", offset, RefListMZ5::getType());
	offset += sizeof(RefListMZ5Data);
	ret.insertMember("targets", offset, ParamListsMZ5::getType());
	return ret;
}

CompType SoftwareMZ5::getType() {
	CompType ret(sizeof(SoftwareMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("version", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	return ret;
}

CompType SourceFileMZ5::getType() {
	CompType ret(sizeof(SourceFileMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("location", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("name", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	return ret;
}

SpectrumMZ5::SpectrumMZ5(){
	init(ParamListMZ5(),ScansMZ5(),PrecursorListMZ5(),ParamListsMZ5(),RefMZ5(),RefMZ5(),ULONG_MAX,"","");
}

SpectrumMZ5::SpectrumMZ5(const SpectrumMZ5& s){
	init(s.paramList,s.scanList, s.precursorList, s.productList, s.dataProcessingRefID, s.sourceFileRefID, s.index, s.id, s.spotID);
}

SpectrumMZ5& SpectrumMZ5::operator=(const SpectrumMZ5& s){
	if(this!=&s){
		if (id != NULL) delete[] id;
		if (spotID != NULL) delete[] spotID;
		init(s.paramList, s.scanList, s.precursorList, s.productList, s.dataProcessingRefID, s.sourceFileRefID, s.index, s.id, s.spotID);
	}
	return *this;
}

SpectrumMZ5::~SpectrumMZ5(){
	if(id!=NULL) delete [] id;
	if(spotID!=NULL) delete [] spotID;
}

CompType SpectrumMZ5::getType() {
	CompType ret(sizeof(SpectrumMZ5));
	StrType stringtype = getStringType();
	size_t offset = 0;
	ret.insertMember("id", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("spotID", offset, stringtype);
	offset += stringtype.getSize();
	ret.insertMember("params", offset, ParamListMZ5::getType());
	offset += sizeof(ParamListMZ5Data);
	ret.insertMember("scanList", offset, ScansMZ5::getType());
	offset += sizeof(ScansMZ5);
	ret.insertMember("precursors", offset, PrecursorListMZ5::getType());
	offset += sizeof(PrecursorListMZ5);
	ret.insertMember("products", offset, ParamListsMZ5::getType());
	offset += sizeof(ParamListsMZ5);
	ret.insertMember("refDataProcessing", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("refSourceFile", offset, RefMZ5::getType());
	offset += sizeof(RefMZ5Data);
	ret.insertMember("index", offset, PredType::NATIVE_ULONG);
	offset += PredType::NATIVE_ULONG.getSize();
	return ret;
}

void SpectrumMZ5::init(const ParamListMZ5& params, const ScansMZ5& scanList, const PrecursorListMZ5& precursors, const ParamListsMZ5& productIonIsolationWindows, const RefMZ5& refDataProcessing, const RefMZ5& refSourceFile, const unsigned long index, const char* id, const char* spotID){
	this->id=new char[strlen(id)+1];
	strcpy(this->id,id);
	this->spotID=new char[strlen(spotID)+1];
	strcpy(this->spotID,spotID);
	this->index=index;
	this->paramList=params;
	this->scanList=scanList;
	this->precursorList=precursors;
	this->productList=productIonIsolationWindows;
	this->dataProcessingRefID=refDataProcessing;
	this->sourceFileRefID=refSourceFile;
}


CompType UserParamMZ5::getType() {
	CompType ret(sizeof(UserParamMZ5Data));
	StrType namestringtype = getFStringType(USRNL);
	StrType valuestringtype = getFStringType(USRVL);
	StrType typestringtype = getFStringType(USRTL);
	ret.insertMember("name", HOFFSET(UserParamMZ5Data, name), namestringtype);
	ret.insertMember("value", HOFFSET(UserParamMZ5Data, value), valuestringtype);
	ret.insertMember("type", HOFFSET(UserParamMZ5Data, type), typestringtype);
	ret.insertMember("uRefID", HOFFSET(UserParamMZ5Data, unitCVRefID), PredType::NATIVE_ULONG);
	return ret;
}
#endif
