#include "mzParser.h"

BasicChromatogram::BasicChromatogram(){}

BasicChromatogram::BasicChromatogram(const BasicChromatogram& c){
	vData.clear();
	for(unsigned int i=0;i<c.vData.size();i++) vData.push_back(c.vData[i]);
	strcpy(idString,c.idString);
}

BasicChromatogram::~BasicChromatogram(){}

BasicChromatogram& BasicChromatogram::operator=(const BasicChromatogram& c){
	if(this != &c){
		vData.clear();
		for(unsigned int i=0;i<c.vData.size();i++) vData.push_back(c.vData[i]);
		strcpy(idString,c.idString);
	}
	return *this;
}
	
TimeIntensityPair& BasicChromatogram::operator[ ](const unsigned int index){ return vData[index];	}

void BasicChromatogram::addTIP(TimeIntensityPair tip){ vData.push_back(tip); }
void BasicChromatogram::clear(){
	vData.clear();
	strcpy(idString,"");
}
void BasicChromatogram::setIDString(char* str) { strcpy(idString,str); }

vector<TimeIntensityPair>&  BasicChromatogram::getData() { return vData; }
int BasicChromatogram::getIDString(char* str){
	strcpy(str,idString);
	return strlen(str);
}
unsigned int BasicChromatogram::size(){	return vData.size(); }

