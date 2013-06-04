#include "mzParser.h"

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

ChromatogramList::ChromatogramList(mzpSAXMzmlHandler* ml, mzpMz5Handler* m5, BasicChromatogram* bc){
	mzML=ml;
	mz5=m5;
	chromat=new Chromatogram();
	chromat->bc=bc;
}

ChromatogramList::~ChromatogramList(){
	mzML=NULL;
	mz5=NULL;
	vChromatIndex=NULL;
	vMz5Index=NULL;
	delete chromat;
}

ChromatogramPtr ChromatogramList::chromatogram(int index, bool binaryData) {
	char str[128];
	if(mzML!=NULL) {
		mzML->readChromatogram(index);
		chromat->bc->getIDString(str);
		chromat->id=str;
		return chromat;
	}	else if(mz5!=NULL) {
		mz5->readChromatogram(index);
		chromat->bc->getIDString(str);
		chromat->id=str;
		return chromat;
	}
	return NULL;
}
bool ChromatogramList::get() {
	if(mzML!=NULL) vChromatIndex = mzML->getChromatIndex();
	else if(mz5!=NULL) vMz5Index = mz5->getChromatIndex();
	else return false;
	return true;
}

unsigned int ChromatogramList::size() {
	if(vChromatIndex==NULL) {
		cerr << "Get chromatogram list first." << endl;
		return 0;
	}
	if(mzML!=NULL) return vChromatIndex->size();
	else if(mz5!=NULL) return vMz5Index->size();
	else return 0;
}

PwizRun::PwizRun(){
	chromatogramListPtr = new ChromatogramList();
}

PwizRun::PwizRun(mzpSAXMzmlHandler* ml, mzpMz5Handler* m5, BasicChromatogram* b){
	mzML=ml;
	mz5=m5;
	bc=b;
	chromatogramListPtr = new ChromatogramList(ml, m5, b);
}

PwizRun::~PwizRun(){
	mzML=NULL;
	mz5=NULL;
	bc=NULL;
	delete chromatogramListPtr;
}

void PwizRun::set(mzpSAXMzmlHandler* ml, mzpMz5Handler* m5, BasicChromatogram* b){
	mzML=ml;
	mz5=m5;
	bc=b;
	delete chromatogramListPtr;
	chromatogramListPtr = new ChromatogramList(ml, m5, b);
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
				break;
			case 2: //mzXML
			case 4:
				cerr << "mzXML not supported in this interface." << endl;
				delete bs;
				delete bc;
				break;
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
			default:
				break;
		}
		run.set(mzML,mz5,bc);
	}
}

MSDataFile::~MSDataFile(){
  if(mzML!=NULL) delete mzML;
  if(mz5!=NULL){
    delete mz5;
    delete mz5Config;
  }
  delete bs;
  delete bc;
}
