#include "mzParser.h"

MzParser::MzParser(BasicSpectrum* s){
	spec=s;
	fileType=0;
	mzML=NULL;
	mzXML=NULL;
	mz5=NULL;
	mz5Config=NULL;
}

MzParser::MzParser(BasicSpectrum* s, BasicChromatogram* c){
	spec=s;
	chromat=c;
	fileType=0;
	mzML=NULL;
	mzXML=NULL;
	mz5=NULL;
	mz5Config=NULL;
}

MzParser::~MzParser(){
	spec=NULL;
	chromat=NULL;
	if(mzML!=NULL) delete mzML;
	if(mzXML!=NULL) delete mzXML;
	if(mz5!=NULL) delete mz5;
	if(mz5Config!=NULL) delete mz5Config;
}

int MzParser::highChromat(){
	switch(fileType){
		case 1:
		case 3:
			return mzML->highChromat();
			break;
		case 2:
		case 4:
			return 0;
			break;
		case 5:
			return mz5->highChromat();
			break;
		default:
			break;
	}
	return 0;
}

int MzParser::highScan(){
	switch(fileType){
		case 1:
		case 3:
			return mzML->highScan();
			break;
		case 2:
		case 4:
			return mzXML->highScan();
			break;
		case 5:
			return mz5->highScan();
			break;
		default:
			break;
	}
	return 0;
}

bool MzParser::load(char* fname){
	if(mzML!=NULL) {
		delete mzML;
		mzML=NULL;
	}
	if(mzXML!=NULL) {
		delete mzXML;
		mzXML=NULL;
	}
	if(mz5!=NULL) {
		delete mz5;
		delete mz5Config;
		mz5=NULL;
		mz5Config=NULL;
	}
	fileType=checkFileType(fname);
	switch(fileType){
		case 1:
		case 3:
			mzML = new mzpSAXMzmlHandler(spec,chromat);
			if(fileType==3) mzML->setGZCompression(true);
			else mzML->setGZCompression(false);
			return mzML->load(fname);
			break;
		case 2:
		case 4:
			mzXML = new mzpSAXMzxmlHandler(spec);
			if(fileType==4) mzXML->setGZCompression(true);
			else mzXML->setGZCompression(false);
			return mzXML->load(fname);
			break;
		case 5:
			mz5Config = new mzpMz5Config();
			mz5 = new mzpMz5Handler(mz5Config,spec,chromat);
			return mz5->readFile(fname);
			break;
		default:
			break;
	}
	return false;
}

int MzParser::lowScan(){
	switch(fileType){
		case 1:
		case 3:
			return mzML->lowScan();
			break;
		case 2:
		case 4:
			return mzXML->lowScan();
			break;
		case 5:
			return mz5->lowScan();
			break;
		default:
			break;
	}
	return 0;
}

bool MzParser::readChromatogram(int num){
	switch(fileType){
		case 1:
		case 3:
			return mzML->readChromatogram(num);
			break;
		case 5:
			return mz5->readChromatogram(num);
			break;
		default:
			break;
	}
	return false;
}

bool MzParser::readSpectrum(int num){
	switch(fileType){
		case 1:
		case 3:
			return mzML->readSpectrum(num);
			break;
		case 2:
		case 4:
			return mzXML->readSpectrum(num);
			break;
		case 5:
			return mz5->readSpectrum(num);
			break;
		default:
			break;
	}
	return false;
}

bool MzParser::readSpectrumHeader(int num){
	switch(fileType){
		case 1:
		case 3:
			return mzML->readHeader(num);
			break;
		case 2:
		case 4:
			return mzXML->readHeader(num);
			break;
		default:
			break;
	}
	return false;
}

int MzParser::checkFileType(char* fname){
	char file[256];
	char ext[256];
	char *tok;
	char preExt[256];
	unsigned int i;

	strcpy(ext,"");

	strcpy(file,fname);
	tok=strtok(file,".\n");
	while(tok!=NULL){
		strcpy(preExt,ext);
		strcpy(ext,tok);
		tok=strtok(NULL,".\n");
	}

	for(i=0;i<strlen(ext);i++) ext[i]=toupper(ext[i]);
	for(i=0;i<strlen(preExt);i++) preExt[i]=toupper(preExt[i]);

	if(!strcmp(ext,"MZML")) return 1;
	if(!strcmp(ext,"MZXML")) return 2;
	if(!strcmp(ext,"MZ5")) return 5;
	if(!strcmp(ext,"GZ")) {
		if(!strcmp(preExt,"MZML")) return 3;
		if(!strcmp(preExt,"MZXML")) return 4;
		cerr << "Unknown .gz file. Only .mzML.gz and .mzXML.gz allowed. No file loaded." << endl;
		return 0;
	}
	cerr << "Unknown file type. No file loaded." << endl;
	return 0;
}
