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
#include "MSReader.h"
#include <iostream>
using namespace std;
using namespace MSToolkit;

MSReader::MSReader(){
  fileIn=NULL;
  rampFileIn=NULL;
  iIntensityPrecision=1;
  iMZPrecision=4;
  rampFileOpen=false;
  compressMe=false;
  rawFileOpen=false;
  exportMGF=false;
  highResMGF=false;
  mgfOnePlus=false;
  iFType=0;
  iVersion=0;
  for(int i=0;i<16;i++)	header.header[i][0]='\0';
  headerIndex=0;
  sCurrentFile.clear();
  sInstrument="unknown";
  sManufacturer="unknown";
  lastReadScanNum=0;
}

MSReader::~MSReader(){
  closeFile();
  if(rampFileOpen) {
    rampCloseFile(rampFileIn);
    free(pScanIndex);
  }
}

void MSReader::addFilter(MSSpectrumType m){
	filter.push_back(m);
}

void MSReader::appendFile(char* c, bool text, Spectrum& s){
  FILE* fileOut;

  if (c == NULL) return;

  if (text)fileOut = fopen(c, "at");
  else fileOut = fopen(c, "ab");

  //output spectrum header
  writeSpecHeader(fileOut, text, s);

  //output spectrum
  if (text){
    writeTextSpec(fileOut, s);
  } else if (compressMe){
    writeCompressSpec(fileOut, s);
  } else {
    writeBinarySpec(fileOut, s);
  }

  fclose(fileOut);

}

void MSReader::appendFile(char* c, Spectrum& s){
  MSFileFormat ff;
  FILE* fileOut;

  if (c == NULL) return;
  ff = checkFileFormat(c);

  switch (ff){
  case mgf:
    exportMGF = true;
    fileOut = fopen(c, "at");
    writeTextSpec(fileOut, s);
    fclose(fileOut);
    exportMGF = false;
    break;
  case ms1:
  case ms2:
  case  zs:
  case uzs:
    fileOut = fopen(c, "at");
    writeSpecHeader(fileOut, true, s);
    writeTextSpec(fileOut, s);
    fclose(fileOut);
    break;
  case bms1:
  case bms2:
    fileOut = fopen(c, "ab");
    writeSpecHeader(fileOut, false, s);
    writeBinarySpec(fileOut, s);
    fclose(fileOut);
    break;
  case cms1:
  case cms2:
    fileOut = fopen(c, "ab");
    writeSpecHeader(fileOut, false, s);
    writeCompressSpec(fileOut, s);
    fclose(fileOut);
    break;
  case psm:
    cout << "File format no longer supported." << endl;
    break;
  default:
    cout << "Cannot append file: unknown or unsupported file type." << endl;
    break;
  }

}

void MSReader::appendFile(char* c, bool text, MSObject& m){

  FILE* fileOut;
  int i;

  //if a filename isn't specified, check to see if the
  //MSObject has a filename.
  if (c == NULL) {
    return;
  } else {
    if (text) fileOut = fopen(c, "at");
    else fileOut = fopen(c, "ab");
  }

  //output spectra;
  for (i = 0; i<m.size(); i++){

    //output spectrum header
    writeSpecHeader(fileOut, text, m.at(i));

    //output spectrum
    if (text){
      writeTextSpec(fileOut, m.at(i));
    } else if (compressMe){
      writeCompressSpec(fileOut, m.at(i));
    } else {
      writeBinarySpec(fileOut, m.at(i));
    }

  }

  fclose(fileOut);
}

void MSReader::appendFile(char* c, MSObject& m){

  MSFileFormat ff;
  FILE* fileOut;
  int i;

  if (c == NULL) return;
  ff = checkFileFormat(c);

  switch (ff){
  case mgf:
    exportMGF = true;
    fileOut = fopen(c, "at");
    for (i = 0; i<m.size(); i++) writeTextSpec(fileOut, m.at(i));
    fclose(fileOut);
    exportMGF = false;
    break;
  case ms1:
  case ms2:
  case  zs:
  case uzs:
    fileOut = fopen(c, "at");
    for (i = 0; i<m.size(); i++){
      writeSpecHeader(fileOut, true, m.at(i));
      writeTextSpec(fileOut, m.at(i));
    }
    fclose(fileOut);
    break;
  case bms1:
  case bms2:
    fileOut = fopen(c, "ab");
    for (i = 0; i<m.size(); i++){
      writeSpecHeader(fileOut, false, m.at(i));
      writeBinarySpec(fileOut, m.at(i));
    }
    fclose(fileOut);
    break;
  case cms1:
  case cms2:
    fileOut = fopen(c, "ab");
    for (i = 0; i<m.size(); i++){
      writeSpecHeader(fileOut, false, m.at(i));
      writeCompressSpec(fileOut, m.at(i));
    }
    fclose(fileOut);
    break;
  default:
    cout << "Cannot append file: unknown or unsupported file type." << endl;
    break;
  }

}

void MSReader::closeFile(){
	if(fileIn!=NULL) fclose(fileIn);
	if(rampFileOpen) {
		rampCloseFile(rampFileIn);
		rampFileIn=NULL;
		rampFileOpen=false;
		free(pScanIndex);
	}
}

bool MSReader::findSpectrum(int i){
  if (i == 0){
    lPivot = lEnd / 2;
    lFWidth = lPivot / 2;
  } else if (i == -1){
    lPivot -= lFWidth;
    lFWidth /= 2;
  } else {
    lPivot += lFWidth;
    lFWidth /= 2;
  }
  fseek(fileIn, lPivot, 0);
  return (lFWidth>0 && lPivot>0 && lPivot<lEnd);
}

string MSReader::getCurrentFile(){
  return sCurrentFile;
}

MSSpectrumType MSReader::getFileType(){
  return fileType;
}

MSHeader& MSReader::getHeader(){
  return header;
}

//REMOVED 2024.09.06
//void MSReader::getInstrument(char* str){
//  strcpy(str,&sInstrument[0]);
//}

void MSReader::getInstrument(string& str) {
  str=sInstrument;
}

int MSReader::getLastScan(){
  switch (lastFileFormat){
  case mzXML:
  case mzML:
  case mzMLb:
  case mz5:
  case mzXMLgz:
  case mzMLgz:
    if (rampFileIn != NULL) return (rampLastScan);
    break;
  case raw:
#ifdef _MSC_VER
#ifndef _NO_THERMORAW
    if (cRAW.getStatus()) return cRAW.getScanCount();
#endif
#endif
    break;
  default:
    break;
  }
  return -1;
}

//REMOVED 2024.09.06
//void MSReader::getManufacturer(char* str){
//  strcpy(str,&sManufacturer[0]);
//}

void MSReader::getManufacturer(string& str) {
  str=sManufacturer;
}

int MSReader::getPercent(){
  switch (lastFileFormat){
  case ms1:
  case ms2:
  case mgf:
  case  zs:
  case uzs:
  case bms1:
  case bms2:
  case cms1:
  case cms2:
    if (fileIn != NULL) {
      return (int)((double)ftell(fileIn) / lEnd * 100);
    }
    break;
  case mzXML:
  case mz5:
  case mzML:
  case mzMLb:
  case mzXMLgz:
  case mzMLgz:
    if (rampFileIn != NULL){
      return (int)((double)lastReadScanNum / rampLastScan * 100);
    }
    break;
  case raw:
#ifdef _MSC_VER
#ifndef _NO_THERMORAW
    if (cRAW.getStatus()){
      return (int)((double)cRAW.getLastScanNumber() / cRAW.getScanCount() * 100);
    }
#endif
#endif
    break;
  default:
    break;
  }
  return -1;
}

/* 0 = File opened correctly
   1 = Could not open file
*/
int MSReader::openFile(const char *c,bool text){
	int i;
  size_t ret;

	if(text) fileIn=fopen(c,"rt");
	else fileIn=fopen(c,"rb");

  if(fileIn==NULL) {
		for(i=0;i<16;i++) header.header[i][0]='\0';
    headerIndex=0;
    fileType=Unspecified;
    return 1;
  } else {
    fileType=Unspecified;

		//if we don't have the eof position, get it here.
		fseek(fileIn,0,2);
		lEnd=ftell(fileIn);

		lPivot = 0;
    lFWidth = lEnd/2;

		fseek(fileIn,0,0);

		if(text){
			for(i=0;i<16;i++) header.header[i][0]='\0';
			headerIndex=0;
		} else {
      ret=fread(&iFType,4,1,fileIn);
      ret = fread(&iVersion, 4, 1, fileIn);
      ret = fread(&header, sizeof(MSHeader), 1, fileIn);
		}

	  return 0;
  }
}

bool MSReader::nextSpectrum(Spectrum& s){
  return readFile(NULL,s);
}

bool MSReader::prevSpectrum(Spectrum& s){
  return readFile(NULL,s,-1);
}

bool MSReader::readMGFFile(const char* c, Spectrum& s){

  char* tok;
  char* nextTok;
  char str[1024];
  char num[6];
  unsigned int i;
  int ch=0;
  double mz;
  float intensity;
  char* ret;
  bool bMono=false;

  //clear any spectrum data
  s.clear();

  s.setCentroidStatus(2); //unknown if centroided with MGF format.

  //check for valid file and if we can access it
  //Supplying a file name always resets file pointer to the start of the file
  //Otherwise, next scan is read.
  if(c!=NULL){
    closeFile();
    if(openFile(c,true)==1) return false;
    mgfIndex=1;
  } else if(fileIn==NULL) {
    cout << "fileIn is NULL" << endl;
    return false;
  }

  s.setFileType(MS2);

  //Read global header information
  if(c!=NULL){
    if(!fgets(strMGF,1024,fileIn)) return false;
    while(true){

      if(!strncmp(strMGF,"CHARGE",7)) {
        mgfGlobalCharge.clear();
        strcpy(str,strMGF+7);
        tok=strtok_r(str," \t\n\r",&nextTok);
        while(tok!=NULL){
          for(i=0;i<strlen(tok);i++){
            if(isdigit(tok[i])) {
              num[i]=tok[i];
              continue;
            }
            if(tok[i]=='+') {
              num[i]='\0';
              mgfGlobalCharge.push_back(atoi(num));
            }
            if(tok[i]=='-') {
              num[i]='\0';
              mgfGlobalCharge.push_back(-atoi(num));
            }
            break;
          }
          tok=strtok_r(NULL," \t\n\r",&nextTok);
        }
      } else if (!strncmp(strMGF, "MASS", 5)){

      }

      if(!strncmp(strMGF,"BEGIN IONS", 10)) break;
      if(!fgets(strMGF,1024,fileIn)) break;

    }
  } else {
    if(!fgets(strMGF,1024,fileIn)) return false;
  }

  // JKE: skip all whitespace and comment lines 
  while(!feof(fileIn) && (strspn(strMGF, " \r\n\t") == strlen(strMGF) 
    || strMGF[0]=='#' || strMGF[0]==';' || strMGF[0]=='!' || strMGF[0]=='/')) {
    ret=fgets(strMGF,1024,fileIn); 
  }
  // JKE: take care of possibility of blank line at end of file
  if(feof(fileIn)) return true;

  //Sanity check that we are at next spectrum
  if(strstr(strMGF,"BEGIN IONS")==NULL) {
    cout << "Malformed MGF spectrum entry. Exiting." << endl;
    cout << "line: " << strMGF << endl;
    exit(-10);
  }

  //Read [next] spectrum header, modernization from JKE across entire while block
  while(isalpha(strMGF[0]) || strspn(strMGF, " \r\n\t") == strlen(strMGF)){ 

   //allow blank links to appear in spectrum header block 
   if(strspn(strMGF, " \r\n\t") == strlen(strMGF)) { 
     if(!fgets(strMGF,1024,fileIn)) return false; 
     continue; 
   } 

   strMGF[strlen(strMGF)-1]='\0'; 
   if(!strncmp(strMGF, "CHARGE=", 7)) { 
     char *pStr;
     if((pStr = strchr(strMGF, '+'))!=NULL) { 
       *pStr = '\0'; 
       ch = atoi(strMGF+7);      
     } 
     if((pStr = strchr(strMGF, '-'))!=NULL) { 
       *pStr = '\0';         
       ch = -atoi(strMGF+7); 
     } 
     s.setCharge(ch);
   } else if(!strncmp(strMGF, "PEPMASS=", 8)) { 
     s.setMZ(atof(strMGF+8)); 
   } else if(!strncmp(strMGF, "SCANS=", 6)) { 
     s.setScanNumber(atoi(strMGF+6)); 
   } else if(!strncmp(strMGF, "RTINSECONDS=", 12)) { 
     s.setRTime((float)(atof(strMGF+12)/60.0)); 
   } else if(!strncmp(strMGF, "TITLE=", 6)) { 
     s.setNativeID(strMGF+6); 
   } 

   if(!fgets(strMGF,1024,fileIn)) break; 
  }

  //Process header information
  if(s.getMZ()==0) {
    cout << "Error in MGF file: no PEPMASS found." << endl;
    exit(-12);
  }
  if(ch!=0){
    s.addZState(ch,s.getMZ()*ch-1.007276466*(ch-1));
  } else {
    for(i=0;i<mgfGlobalCharge.size();i++){
      s.addZState(mgfGlobalCharge[i],s.getMZ()*mgfGlobalCharge[i]-1.007276466*(mgfGlobalCharge[i]-1));
    }
  }
  if(s.getScanNumber()==0){
    //attempt to obtain scan number from title using ISB/ProteoWizard format
    if (s.getNativeID(str, 1024)){
      tok = strtok_r(str, ".",&nextTok);
      tok = strtok_r(NULL, ".", &nextTok);
      if (tok!=NULL) {
        s.setScanNumber(atoi(tok));
        s.setScanNumber(atoi(tok),true);
      }
    }
    if (s.getScanNumber()==0){
      s.setScanNumber(mgfIndex);
      s.setScanNumber(mgfIndex,true);
      mgfIndex++;
    }
  }

  //Read peak data
  while(!isalpha(strMGF[0])){

    tok=strtok_r(strMGF," \t\n\r", &nextTok);
    if(tok==NULL){
      cout << "Error in MGF file: bad m/z or intensity value." << endl;
      exit(-13);
    }
    mz=atof(tok);
    tok=strtok_r(NULL," \t\n\r", &nextTok);
    if(tok==NULL){
      cout << "Error in MGF file: bad m/z or intensity value." << endl;
      exit(-13);
    }
    intensity=(float)atof(tok);
    if(!mgfOnePlus){
      tok=strtok_r(NULL," \t\n\r", &nextTok);
      if(tok!=NULL) {
        ch=atoi(tok);
        mz *= ch;                  // if fragment charge specified, convert m/z to 1+
        mz -= (ch-1)*1.007276466;
      }
    }
    s.add(mz,intensity);

    if(!fgets(strMGF,1024,fileIn)) break;
  }

  //Sanity check
  if(strstr(strMGF,"END IONS")==NULL){
    cout << "WARNING: Unexpected lines at end of MGF spectrum." << endl;
    bMono=false;
  }

  if(mgfOnePlus) s.sortMZ();

  return true;
}

bool MSReader::readMGFFile2(const char* c, Spectrum& s){

  char* tok;
  char* nextTok;
  char str[1024];
  char num[6];
  unsigned int i;
  int ch = 0;
  double mz;
  float intensity;
  bool bMono = false;
  vector<string> tokens;

  //clear any spectrum data
  s.clear();

  s.setCentroidStatus(2); //unknown if centroided with MGF format.

  //check for valid file and if we can access it
  //Supplying a file name always resets file pointer to the start of the file
  //Otherwise, next scan is read.
  if (c != NULL){
    closeFile();
    if (openFile(c, true) == 1) return false;
    mgfIndex = 1;
    mgfFiles.clear();
  } else if (fileIn == NULL) {
    cout << "fileIn is NULL" << endl;
    return false;
  }

  s.setFileType(MS2);

  //Read global header information
  while(!feof(fileIn)){
    if (!fgets(strMGF, 1024, fileIn)) return false;
    if(strlen(strMGF)<2) continue; //skip blank lines
    if (strMGF[0] == '#' || strMGF[0] == ';' || strMGF[0] == '!' || strMGF[0] == '/') continue; //skip comment lines
    tokens.clear();
    tok=strtok_r(strMGF,"=\n\r", &nextTok);
    while(tok!=NULL){
      tokens.push_back(string(tok));
      tok=strtok_r(NULL,"=\n\r", &nextTok);
    }
    if(tokens[0].compare("BEGIN IONS")==0) break;
    else if(tokens[0].compare("MASS")==0){
      if(tokens[1].compare("Monoisotopic")==0) bMono=true;
      else bMono=false;
    } else if (tokens[0].compare("CHARGE") == 0){
      mgfGlobalCharge.clear();
      strcpy(str,tokens[1].c_str());
      tok = strtok_r(str, " \t\n\r", &nextTok);
      while (tok != NULL){
        for (i = 0; i<strlen(tok); i++){
          if (isdigit(tok[i])) {
            num[i] = tok[i];
            continue;
          }
          if (tok[i] == '+') {
            num[i] = '\0';
            mgfGlobalCharge.push_back(atoi(num));
          }
          if (tok[i] == '-') {
            num[i] = '\0';
            mgfGlobalCharge.push_back(-atoi(num));
          }
          break;
        }
        tok = strtok_r(NULL, " \t\n\r", &nextTok);
      }
    } else if(tokens[0][0]=='_') { //user and reserved parameters
      if(tokens[0].find("_DISTILLER_RAWFILE")==0){
        size_t pos=tokens[1].find_last_of('\\');
        if(pos==string::npos) pos=tokens[1].find_last_of('/');
        if(pos==string::npos) pos=0;
        else pos++;
        mgfFiles.push_back(tokens[1].substr(pos));
      }
    }
  }
  if (feof(fileIn)) return false;

  //read spectrum block
  while (!feof(fileIn)){
    if (!fgets(strMGF, 1024, fileIn)) return false;
    if (strlen(strMGF)<2) continue; //skip blank lines
    if (strMGF[0] == '#' || strMGF[0] == ';' || strMGF[0] == '!' || strMGF[0] == '/') continue; //skip comment lines
    tokens.clear();
    tok = strtok_r(strMGF, "=\n\r", &nextTok);
    while (tok != NULL){
      tokens.push_back(string(tok));
      tok = strtok_r(NULL, "=\n\r", &nextTok);
    }
    if (tokens[0].find("END IONS") != string::npos) {
      //convert any header information to MST spectrum information
      if (s.getMZ() == 0) {
        cout << "Error in MGF file: no PEPMASS found." << endl;
        exit(-12);
      }
      if (ch != 0){
        s.addZState(ch, s.getMZ()*ch - 1.007276466*(ch - 1));
      } else {
        for (i = 0; i<mgfGlobalCharge.size(); i++){
          s.addZState(mgfGlobalCharge[i], s.getMZ()*mgfGlobalCharge[i] - 1.007276466*(mgfGlobalCharge[i] - 1));
        }
      }
      if (s.getScanNumber() == 0){
        //attempt to obtain scan number from title using ISB/ProteoWizard format
        if (s.getNativeID(str, 1024)){
          tok = strtok_r(str, ".", &nextTok);
          tok = strtok_r(NULL, ".", &nextTok);
          if (tok != NULL) {
            s.setScanNumber(atoi(tok));
            s.setScanNumber(atoi(tok), true);
          }
        }
        if (s.getScanNumber() == 0){
          s.setScanNumber(mgfIndex);
          s.setScanNumber(mgfIndex, true);
          mgfIndex++;
        }
      }
      if (mgfOnePlus) s.sortMZ();
      return true;
    } else if (tokens[0].compare("CHARGE")==0) {
      ch=atoi(tokens[1].c_str());
      if(tokens[1].find('-')!=string::npos) ch=-ch;
      s.setCharge(ch);
    } else if (tokens[0].compare("PEPMASS")==0) {
      s.setMZ(atof(tokens[1].c_str()));
    } else if (tokens[0].find("SCANS")==0) {
      s.setScanNumber(atoi(tokens[1].c_str()));
      if(tokens[0].size()>5){ //only process file identifier from SCANS parameter
        size_t fIndex=(size_t)atoi(&tokens[0][6]);
        s.setFileID(mgfFiles[fIndex]);
      }
    } else if (tokens[0].find("RTINSECONDS")==0) {
      s.setRTime((float)(atof(tokens[1].c_str()) / 60.0));
    } else if (tokens[0].compare("TITLE")==0) {
      for(size_t a=2;a<tokens.size();a++) tokens[1]+='='+tokens[a];
      s.setNativeID(tokens[1].c_str());
    } else if(isdigit(tokens[0][0])){
      strcpy(str, tokens[0].c_str());
      tok = strtok_r(str, " \t\n\r", &nextTok);
      mz = atof(tok);
      tok = strtok_r(NULL, " \t\n\r", &nextTok);
      intensity = (float)atof(tok);
      if (!mgfOnePlus){
        tok = strtok_r(NULL, " \t\n\r", &nextTok);
        if (tok != NULL) {
          ch = atoi(tok);
          mz *= ch;                  // if fragment charge specified, convert m/z to 1+
          mz -= (ch - 1)*1.007276466;
        }
      }
      s.add(mz, intensity);
    }

  }

  return false;
}

bool MSReader::readMSTFile(const char *c, bool text, Spectrum& s, int scNum){
  MSScanInfo ms;
  Peak_T p;
  ZState z;
  EZState ez;
  int i;
  size_t ret;

  //variables for text reading only
  bool firstScan = false;
  bool bScan = true;
  bool bDoneHeader = false;
  char tstr[256];
  char ch;
  char *tok;
  char *nextTok;
  char *retC;

  //variables for compressed files
  uLong mzLen, intensityLen;

  //clear any spectrum data
  s.clear();

  s.setCentroidStatus(2); //unknown if centroided with these formats.

  //check for valid file and if we can access it
  if(c!=NULL){
    closeFile();
    if(openFile(c,text)==1) return false;
    lastFileFormat = checkFileFormat(c);
  } else if(fileIn==NULL) {
    return false;
  }

  //set the filetype
  switch(lastFileFormat){
  case ms2:
  case cms2:
  case bms2:
    s.setFileType(MS2);
    break;
  case zs:
    s.setFileType(ZS);
    break;
  case uzs:
    s.setFileType(UZS);
    break;
  case ms1:
  case cms1:
  case bms1:
    s.setFileType(MS1);
    break;
  default:
    s.setFileType(Unspecified);
    break;
  }

	//Handle binary and text files differently
  if(!text){

    //if binary file, read scan info sequentially, skipping to next scan if requested
    //fread(&ms,sizeof(MSScanInfo),1,fileIn);
    readSpecHeader(fileIn,ms);

    if(scNum<0) {
      cerr << "ERROR: readMSTFile(): Cannot request previous scan. Function not supported. " << flush;
      exit(1);
    }
    if(scNum!=0){

      fseek(fileIn,sizeof(MSHeader)+8,0);

      //fread(&ms,sizeof(MSScanInfo),1,fileIn);
      readSpecHeader(fileIn,ms);

      while(ms.scanNumber[0]!=scNum){

        fseek(fileIn,ms.numZStates*12,1);
        fseek(fileIn,ms.numEZStates*20,1);

	      if(compressMe){
	        ret=fread(&i,4,1,fileIn);
	        mzLen = (uLong)i;
          ret = fread(&i, 4, 1, fileIn);
	        intensityLen = (uLong)i;
	        fseek(fileIn,mzLen+intensityLen,1);
	      } else {
	        fseek(fileIn,ms.numDataPoints*12,1);
	      }

	      //fread(&ms,sizeof(MSScanInfo),1,fileIn);
        readSpecHeader(fileIn,ms);
	      if(feof(fileIn)) return false;
      }
    }
    if(feof(fileIn)) return false;

		//read any charge states (for MS2 files)
    for(i=0;i<ms.numZStates;i++){
      ret = fread(&z.z, 4, 1, fileIn);
      ret = fread(&z.mh, 8, 1, fileIn);
      s.addZState(z);
    }

    for(i=0;i<ms.numEZStates;i++){
      ret = fread(&ez.z, 4, 1, fileIn);
      ret = fread(&ez.mh, 8, 1, fileIn);
      ret = fread(&ez.pRTime, 4, 1, fileIn);
      ret = fread(&ez.pArea, 4, 1, fileIn);
      s.addEZState(ez);
    }

    s.setScanNumber(ms.scanNumber[0]);
    s.setScanNumber(ms.scanNumber[1],true);
    s.setRTime(ms.rTime);
		if(ms.mzCount==0) s.setMZ(0);
		for(i=0;i<ms.mzCount;i++){
			if(i==0) s.setMZ(ms.mz[i]);
			else s.addMZ(ms.mz[i]);
		}
    s.setBPI(ms.BPI);
    s.setBPM(ms.BPM);
    s.setConversionA(ms.convA);
    s.setConversionB(ms.convB);
		s.setConversionA(ms.convC);
    s.setConversionB(ms.convD);
		s.setConversionA(ms.convE);
    s.setConversionB(ms.convI);
    s.setIonInjectionTime(ms.IIT);
    s.setTIC(ms.TIC);

    //read compressed data to the spectrum object
    if(compressMe) {

      readCompressSpec(fileIn,ms,s);

      //or read binary data to the spectrum object
    } else {
      for(i=0;i<ms.numDataPoints;i++){
        ret = fread(&p.mz, 8, 1, fileIn);
        ret = fread(&p.intensity, 4, 1, fileIn);
	      //cout << p.mz << " " << p.intensity << endl;
	      s.add(p);
      }
    }

    //return success
    return true;

  } else {

    //if reading text files, some parsing is required.
    while(true){

      //stop when you reach the end of the file
      if(feof(fileIn)) {

        //Special case: when doing binary search, end of file might mean to search
        //the othere end of the file.
        if(scNum != 0){
	        if(s.getScanNumber() != scNum) {
	          bScan=findSpectrum(-1);
            s.clear();
	          s.setScanNumber(0);
	          if(bScan==false) return false;
          } else {
            break;
          }
	      } else {
          break;
        }
      }

      //scan next character in the file
      ch=fgetc(fileIn);
      ungetc(ch,fileIn);

      switch(ch){
      case 'D':
	      //D lines are ignored
	      retC=fgets(tstr,256,fileIn);
	      break;

      case 'H':
	      //Header lines are recorded as strings up to 16 lines at 256 characters each
        retC = fgets(tstr, 256, fileIn);
	      if(!bDoneHeader) {
	        tok=strtok_r(tstr," \t\n\r", &nextTok);
	        tok=strtok_r(NULL,"\n\r", &nextTok);
	        if(headerIndex<16) {
            strcpy(header.header[headerIndex],tok);
            strcat(header.header[headerIndex],"\n");
            headerIndex++;
          }
	        else cout << "Header too big!!" << endl;
	      }
	      break;

      case 'I':
	      //I lines are recorded only if they contain retention times
        retC = fgets(tstr, 256, fileIn);
        tok=strtok_r(tstr," \t\n\r", &nextTok);
        tok=strtok_r(NULL," \t\n\r", &nextTok);
        if(strcmp(tok,"RTime")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setRTime((float)atof(tok));
        }	else if(strcmp(tok,"TIC")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setTIC((float)atof(tok));
        }	else if(strcmp(tok,"IIT")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setIonInjectionTime((float)atof(tok));
        }	else if(strcmp(tok,"BPI")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setBPI((float)atof(tok));
        }	else if(strcmp(tok,"BPM")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setBPM((float)atof(tok));
        }	else if(strcmp(tok,"ConvA")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setConversionA(atof(tok));
        }	else if(strcmp(tok,"ConvB")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setConversionB(atof(tok));
        }	else if(strcmp(tok,"ConvC")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setConversionC(atof(tok));
        }	else if(strcmp(tok,"ConvD")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setConversionD(atof(tok));
        }	else if(strcmp(tok,"ConvE")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setConversionE(atof(tok));
        }	else if(strcmp(tok,"ConvI")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          s.setConversionI(atof(tok));
        } else if(strcmp(tok,"EZ")==0) {
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          ez.z=atoi(tok);
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          ez.mh=atof(tok);
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          ez.pRTime=(float)atof(tok);
          tok=strtok_r(NULL," \t\n\r,", &nextTok);
          ez.pArea=(float)atof(tok);
          s.addEZState(ez);
        }
        break;

      case 'S':
	      //Scan numbers are recorded and mark all following data is spectrum data
	      //until the next tag

	      //Reaching an S tag also indicates there are no more header lines
	      bDoneHeader=true;

	      if(firstScan) {
	        //if we are here, a desired scan was read and we just reached the next scan tag
	        //therefore, stop reading further.
	        return true;

	      } else {
          retC = fgets(tstr, 256, fileIn);
	        tok=strtok_r(tstr," \t\n\r", &nextTok);
	        tok=strtok_r(NULL," \t\n\r", &nextTok);
          s.setScanNumber(atoi(tok));
	        tok=strtok_r(NULL," \t\n\r", &nextTok);
	        s.setScanNumber(atoi(tok),true);
	        tok=strtok_r(NULL," \t\n\r", &nextTok);
	        if(tok!=NULL)	s.setMZ(atof(tok));
					else s.setMZ(0);
					tok=strtok_r(NULL," \t\n\r", &nextTok);
					while(tok!=NULL) {
						s.addMZ(atof(tok));
						tok=strtok_r(NULL," \t\n\r", &nextTok);
					}
	        if(scNum != 0){
	          if(s.getScanNumber() != scNum) {
	            if(s.getScanNumber()<scNum) bScan=findSpectrum(1);
	            else bScan=findSpectrum(-1);
              s.clear();
	            s.setScanNumber(0);
	            if(bScan==false) return false;
	            break;
	          }
	        }
	        firstScan=true;
	      }
	      break;

      case 'Z':
	      //Z lines are recorded for MS2 files
        //don't record z-lines unless this is a scan number that is wanted
        if(!firstScan){
          retC = fgets(tstr, 256, fileIn);
 	        break;
	      }
        retC = fgets(tstr, 256, fileIn);
	      tok=strtok_r(tstr," \t\n\r", &nextTok);
	      tok=strtok_r(NULL," \t\n\r", &nextTok);
	      z.z=atoi(tok);
	      tok=strtok_r(NULL," \t\n\r", &nextTok);
	      z.mh=atof(tok);
	      s.addZState(z);
	      break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	      //lines beginning with numbers are data; if they belong to a scan we are not
	      //interested in, we ignore them.
	      if(scNum != 0){
	        if(s.getScanNumber()!=scNum) {
            retC = fgets(tstr, 256, fileIn);
	          break;
	        }
	      }
	      //otherwise, read in the line
	      i=fscanf(fileIn,"%lf %f\n",&p.mz,&p.intensity);
	      s.add(p);
	      break;

      default:
	      //if the character is not recognized, ignore the entire line.
        retC = fgets(tstr, 256, fileIn);
	      //fscanf(fileIn,"%s\n",tstr);
	      break;
      }
    }

  }

  return true;

}

void MSReader::writeFile(const char* c, bool text, MSObject& m){

  FILE* fileOut;
  int i;

  //if a filename isn't specified, check to see if the
  //MSObject has a filename.
  if(c == NULL) {
    return;
  } else {
    if(text) fileOut =fopen(c,"wt");
    else fileOut =fopen(c,"wb");
  }

  //output file header lines;
  if(text){
    if(exportMGF){
      //MGF file header is here
      fprintf(fileOut,"COM=Generated in the MSToolkit\n");
      if(!highResMGF) fprintf(fileOut,"CHARGE=2+ and 3+\n");
    } else {
      //MSx/SQT file header is here
      for(i=0;i<16;i++){
        if(m.getHeader().header[i][0]!='\0') {
          fputs("H\t",fileOut);
          fputs(m.getHeader().header[i],fileOut);
        }
      }
    }
  } else {
    //version 0 or 1 has basic stats
    //version 2 adds BPI,BPM,TIC,IIT,ConvA,ConvB
    //version 3 adds EZ lines
		//version 4 adds ConvC,ConvD,ConvE,ConvI
		//version 5 adds MSX support (multiple mz values per spectrum)
    fwrite(&iFType,4,1,fileOut); //file type
    i=5;
    fwrite(&i,4,1,fileOut); //version number - in case we change formats
		fwrite(&m.getHeader(),sizeof(MSHeader),1,fileOut);
	}

	//output spectra;
  for(i=0;i<m.size();i++){

		//output spectrum header
		writeSpecHeader(fileOut,text,m.at(i));

		//output scan
		if(text){
			writeTextSpec(fileOut,m.at(i));
		} else if(compressMe){
			writeCompressSpec(fileOut,m.at(i));
		} else {
			writeBinarySpec(fileOut,m.at(i));
		}

  }

	fclose(fileOut);
}

void MSReader::writeFile(const char* c, MSFileFormat ff, MSObject& m, const char* sha1Report){

  switch(ff){
  case mgf:
    exportMGF=true;
    setCompression(false);
    writeFile(c,true,m);
    exportMGF=false;
    break;
  case ms1:
  case ms2:
  case  zs:
  case uzs:
    exportMGF=false;
    setCompression(false);
    writeFile(c,true,m);
    break;
  case psm:
    cout << "File format no longer supported." << endl;
    break;
  case mzXML:
  case mz5:
	case mzML:
	case mzXMLgz:
	case mzMLgz:
    cout << "Cannot write mzXML or mz5 or mzML formats. Nothing written." << endl;
    break;
  case bms1:
    exportMGF=false;
    setCompression(false);
    iFType=1;
    writeFile(c,false,m);
    break;
  case bms2:
    exportMGF=false;
    setCompression(false);
    iFType=3;
    writeFile(c,false,m);
    break;
  case cms1:
    exportMGF=false;
    setCompression(true);
    iFType=2;
    writeFile(c,false,m);
    break;
  case cms2:
    exportMGF=false;
    setCompression(true);
    iFType=4;
    writeFile(c,false,m);
    break;
  default:
    cout << "Unknown file format. Nothing written." << endl;
    break;
  }

}

void MSReader::setPrecision(int i, int j){
  iIntensityPrecision=i;
  iMZPrecision=j;
}

void MSReader::setPrecisionInt(int i){
  iIntensityPrecision=i;
}

void MSReader::setPrecisionMZ(int i){
  iMZPrecision=i;
}

bool MSReader::readFile(const char* c, Spectrum& s, int scNum){

  bool bNewRead=false;
  if(c!=NULL) {
    if(sCurrentFile.compare(c)!=0){
      lastFileFormat = checkFileFormat(c);
      sCurrentFile = c;
      sInstrument.clear();
      sManufacturer.clear();
      sInstrument="unknown";
      sManufacturer="unknown";
      bNewRead=true;
    } 
  } else {
    if(sCurrentFile.empty()){
      cout << "MSReader::readFile must specify file for first read." << endl;
      return false;
    }
  }
  switch(lastFileFormat){
	case ms1:
	case ms2:
	case  zs:
	case uzs:
    if(bNewRead) return readMSTFile(c,true,s,scNum);
    else return readMSTFile(NULL, true, s, scNum);
		break;
	case bms1:
	case bms2:
		setCompression(false);
		if(bNewRead) return readMSTFile(c,false,s,scNum);
    else return readMSTFile(NULL, false, s, scNum);
		break;
	case cms1:
	case cms2:
		setCompression(true);
    if(bNewRead) return readMSTFile(c,false,s,scNum);
    else readMSTFile(NULL, false, s, scNum);
		break;
	case mz5:
  case mzXML:
	case mzML:
  case mzMLb:
	case mzXMLgz:
	case mzMLgz:
		if(bNewRead) return readMZPFile(c,s,scNum);
    else return readMZPFile(NULL, s, scNum);
		break;
  case mgf:
    if(scNum!=0) cout << "Warning: random-access or previous spectrum reads not allowed with MGF format." << endl;
    if(bNewRead) return readMGFFile2(c,s);
    else return readMGFFile2(NULL, s);
    break;
	case raw:
		#ifdef _MSC_VER
    #ifndef _NO_THERMORAW
		//only read the raw file if the dll was present and loaded.
		if(cRAW.getStatus()) {
			cRAW.setMSLevelFilter(&filter);
      bool b;
      if(bNewRead) b=cRAW.readRawFile(c,s,scNum);
      else b=cRAW.readRawFile(NULL, s, scNum);
      if(b && bNewRead) {
        cRAW.getInstrument(sInstrument);
        cRAW.getManufacturer(sManufacturer);
      }
			return b;
		} else {
			cerr << "Could not read Thermo RAW file. The Thermo .dll likely was not loaded or out of date." << endl;
			return false;
		}
		#else
			cerr << "Thermo RAW file format not supported." << endl;
			return false;
		#endif
    #else
      cerr << "Thermo RAW file format not supported." << endl;
      return false;
    #endif
		break;
	case sqlite:
	case psm:
		break;
	case dunno:
	default:
    cout << "Unknown file format" << endl;
		return false;
		break;
  }
	return false;

}

bool MSReader::readMZPFile(const char* c, Spectrum& s, int scNum){
	mzParser::ramp_fileoffset_t indexOffset;
  mzParser::ScanHeaderStruct scanHeader;
  mzParser::RAMPREAL *pPeaks;
  mzParser::sPrecursorIon rPI;
  mzParser::BasicSpectrum *bs=NULL;
	int i,j;
  bool bFoundSpec=false;

	if(c!=NULL) {
		//open the file if new file was requested
		if(rampFileOpen) closeFile();
		rampFileIn = mzParser::rampOpenFile(c);
		if (rampFileIn == NULL) {
      //silence errors. TODO: put in error code for user to lookup
      //cerr << "ERROR: Failure reading input file " << c << endl;
      return false;
		}
		rampFileOpen=true;

		//read the index
		indexOffset = getIndexOffset(rampFileIn);
		pScanIndex = readIndex(rampFileIn,indexOffset,&rampLastScan);
		rampIndex=0;
    lastReadScanNum=0;

	} else { //if no new file requested, check to see if one is open already
		if (rampFileIn == NULL) return false;
	}

  //check scNum is less than rampLastScan otherwise will trigger segfault reading pScanIndex[] below
  if(scNum > rampLastScan) return false;

	//clear any spectrum data
	s.clear();

	MSSpectrumType mslevel;

	//read scan header
	if(scNum!=0) {
    if(scNum<0) rampIndex--; //allow reader to jump to previous scan with [any] negative value
    else rampIndex=scNum;
    if (rampIndex<0) {
      rampIndex=0;
      return false; //don't grab previous scan when we're out of bounds
    }
    while(true){
      readHeader(rampFileIn, pScanIndex[rampIndex], &scanHeader,rampIndex,&bs);
      if (scNum>0 && scanHeader.acquisitionNum != scNum && scanHeader.acquisitionNum != -1) {
        cerr << "ERROR: Failure reading scan, index corrupted.  Line endings may have changed during transfer.\n" << flush;
        return false;
      }
		  switch(scanHeader.msLevel){
		  case 1: mslevel = MS1; break;
		  case 2: mslevel = MS2; break;
		  case 3: mslevel = MS3; break;
		  default: break;
		  }
      if (find(filter.begin(), filter.end(), mslevel) != filter.end())	{
        bFoundSpec=true;
        break;
      } else if(scNum<0){
        rampIndex--;
        if(rampIndex<0) {
          rampIndex=0;
          return false;
        }
      } else {
        break;
      }
    }

  } else /* if scnum == 0 */ {

    if (rampIndex>rampLastScan) return false;
    //read next index
    while (true){
      rampIndex++;

      //reached end of file
      if (rampIndex>rampLastScan) return false;
      if (pScanIndex[rampIndex]<0) continue;

      readHeader(rampFileIn, pScanIndex[rampIndex], &scanHeader,rampIndex,&bs);
      switch (scanHeader.msLevel){
      case 1: mslevel = MS1; break;
      case 2: mslevel = MS2; break;
      case 3: mslevel = MS3; break;
      default: break;
      }
      if (find(filter.begin(), filter.end(), mslevel) != filter.end()) {
        bFoundSpec = true;
        break;
      }
    }
  }
  //if spectrum does not fit filter parameters bail now.
  if (!bFoundSpec) return false;

  //set all sorts of meta information about the spectrum
  if(scanHeader.centroid) s.setCentroidStatus(1);
  else s.setCentroidStatus(0);
  s.setNativeID(scanHeader.idString);
	s.setMsLevel(scanHeader.msLevel);
	s.setScanNumber(scanHeader.acquisitionNum);
	s.setScanNumber(scanHeader.acquisitionNum,true);
	s.setRTime((float)scanHeader.retentionTime/60.0f);
  s.setPositiveScan(scanHeader.positiveScan);
  s.setCompensationVoltage(scanHeader.compensationVoltage);
  s.setInverseReducedIonMobility(scanHeader.inverseReducedIonMobility);
  s.setIonInjectionTime((float)scanHeader.ionInjectionTime);
  s.setIonMobilityDriftTime(scanHeader.ionMobilityDriftTime);
  s.setTIC(scanHeader.totIonCurrent);
  s.setScanWindow(scanHeader.lowMZ,scanHeader.highMZ);
  s.setBPI((float)scanHeader.basePeakIntensity);
  s.setRawFilter(scanHeader.filterLine);
	if(strlen(scanHeader.activationMethod)>1){
		if(strcmp(scanHeader.activationMethod,"CID")==0) s.setActivationMethod(mstCID);
      else if(strcmp(scanHeader.activationMethod,"ECD")==0) s.setActivationMethod(mstECD);
      else if(strcmp(scanHeader.activationMethod,"ETD")==0) s.setActivationMethod(mstETD);
      else if(strcmp(scanHeader.activationMethod,"ETDSA")==0) s.setActivationMethod(mstETDSA);
      else if(strcmp(scanHeader.activationMethod,"ETD+SA") == 0) s.setActivationMethod(mstETDSA);
      else if(strcmp(scanHeader.activationMethod,"PQD")==0) s.setActivationMethod(mstPQD);
      else if(strcmp(scanHeader.activationMethod,"HCD")==0) s.setActivationMethod(mstHCD);
		else s.setActivationMethod(mstNA);
	}
	if(scanHeader.msLevel>1) {
		s.setMZ(scanHeader.precursorMZ,scanHeader.precursorMonoMZ);
		s.setCharge(scanHeader.precursorCharge);
    s.setSelWindow(scanHeader.selectionWindowLower,scanHeader.selectionWindowUpper);
    MSPrecursorInfo pi;
    pi.isoMz=scanHeader.isolationMZ;
    pi.mz=scanHeader.precursorMZ;
    pi.monoMz=scanHeader.precursorMonoMZ;
    pi.charge=scanHeader.precursorCharge;
    pi.isoOffsetLower=scanHeader.isolationWindowLower;
    pi.isoOffsetUpper=scanHeader.isolationWindowUpper;
    pi.precursorScanNumber=scanHeader.precursorScanNum;
    if (strcmp(scanHeader.activationMethod, "HCD") == 0) pi.activation = mstHCD;
    else if (strcmp(scanHeader.activationMethod, "CID") == 0) pi.activation = mstCID;
    else if (strcmp(scanHeader.activationMethod, "ETD") == 0) pi.activation = mstETD;
    s.addPrecursor(pi);
	} else {
		s.setMZ(0);
    s.setSelWindow(0,0);
	}
	if(scanHeader.precursorCharge>0) {
    if(scanHeader.precursorMonoMZ>0.0001) s.addZState(scanHeader.precursorCharge,scanHeader.precursorMonoMZ*scanHeader.precursorCharge-(scanHeader.precursorCharge-1)*1.007276466);
    else s.addZState(scanHeader.precursorCharge,scanHeader.precursorMZ*scanHeader.precursorCharge-(scanHeader.precursorCharge-1)*1.007276466);
  }
  for(i=0;i<scanHeader.numPossibleCharges;i++) {
    j=scanHeader.possibleCharges[i*4];
    s.addZState(j,scanHeader.precursorMZ*j-(j-1)*1.007276466);
  }
  for(i=1;i<scanHeader.precursorCount;i++){
    getPrecursor(&scanHeader,i,rPI);
    s.addMZ(rPI.mz);
    s.addZState(rPI.charge, rPI.mz* rPI.charge -(rPI.charge -1)*1.007276466);
    MSPrecursorInfo pi;
    pi.isoMz=rPI.mz; //MH: Fix this by expanding the extra precursor information.
    pi.mz = rPI.mz;
    pi.charge = rPI.charge;
    pi.isoOffsetLower = rPI.isoLowerOffset;
    pi.isoOffsetUpper = rPI.isoUpperOffset;
    pi.precursorScanNumber=scanHeader.precursorScanNum;
    if(strcmp(scanHeader.activationMethod,"HCD")==0) pi.activation=mstHCD;
    else if (strcmp(scanHeader.activationMethod, "CID") == 0) pi.activation=mstCID;
    else if (strcmp(scanHeader.activationMethod, "ETD") == 0) pi.activation = mstETD;
    s.addPrecursor(pi);
  }
  if(strlen(scanHeader.scanDescription)>0){
    string st=scanHeader.scanDescription;
    s.setScanDescription(st);
  }

  //copy the user params, if any
  if(bs!=NULL){
    size_t index = 0;
    mzParser::sUParam up = bs->getUserParam(index++);
    while (!up.name.empty()) {
      s.addUserParam(up.name,up.value,up.type);
      up = bs->getUserParam(index++);
    }
  }

  //store the spectrum
	pPeaks = readPeaks(rampFileIn, pScanIndex[rampIndex],rampIndex);
	j=0;
	for(i=0;i<scanHeader.peaksCount;i++){
    if(scanHeader.ionMobility){
      s.add((double)pPeaks[j], (float)pPeaks[j + 1],(double)pPeaks[j+2]);
      j+=3;
    } else {
      s.add((double)pPeaks[j],(float)pPeaks[j+1]);
      j+=2;
    }
	}
  lastReadScanNum = scanHeader.acquisitionNum;

	//free(pPeaks);  //don't clean this up anymore. will get cleaned up in the RAMPFILE struct
	return true;

}

void MSReader::setFilter(MSSpectrumType m){
  filter.clear();
  filter.push_back(m);
}

void MSReader::setFilter(vector<MSSpectrumType>& m){
  for(unsigned int i=0; i<m.size();i++)
    filter.push_back(m.at(i));
}

void MSReader::setCompression(bool b){
	compressMe=b;
}

void MSReader::setRawFilter(char *c){
	#ifdef _MSC_VER
  #ifndef _NO_THERMORAW
	cRAW.setRawFilter(c);
	#endif
  #endif
}

void MSReader::setHighResMGF(bool b){
  highResMGF=b;
}

void MSReader::setOnePlusMGF(bool b){
  mgfOnePlus=b;
}

void MSReader::writeCompressSpec(FILE* fileOut, Spectrum& s){

	int j;

	//file compression
	int err;
	uLong len;
	unsigned char *comprM, *comprI;
  uLong comprLenM, comprLenI;
	double *pD;
	float *pF;
	uLong sizeM;
	uLong sizeI;

	//Build arrays to hold scan prior to compression
	// Ideally, we would just use the scan vectors, but I don't know how yet.
	pD = new double[s.size()];
	pF = new float[s.size()];
	for(j=0;j<s.size();j++){
		pD[j]=s.at(j).mz;
		pF[j]=s.at(j).intensity;
	}

	//compress mz
	len = (uLong)s.size()*sizeof(double);
	sizeM = len;
	comprLenM = compressBound(len);
	comprM = (unsigned char*)calloc((uInt)comprLenM, 1);
	err = compress(comprM, &comprLenM, (const Bytef*)pD, len);

	//compress intensity
	len = (uLong)s.size()*sizeof(float);
	sizeI = len;
	comprLenI = compressBound(len);
	comprI = (unsigned char*)calloc((uInt)comprLenI, 1);
	err = compress(comprI, &comprLenI, (const Bytef*)pF, len);

	j=(int)comprLenM;
	fwrite(&j,4,1,fileOut);
	j=(int)comprLenI;
	fwrite(&j,4,1,fileOut);
	fwrite(comprM,comprLenM,1,fileOut);
	fwrite(comprI,comprLenI,1,fileOut);

	//clean up memory
	free(comprM);
	free(comprI);
	delete [] pD;
	delete [] pF;

}

void MSReader::readCompressSpec(FILE* fileIn, MSScanInfo& ms, Spectrum& s){

	int i;
	Peak_T p;
  size_t ret;

	//variables for compressed files
	uLong uncomprLen;
	uLong mzLen, intensityLen;
	unsigned char *compr;
	double *mz;
	float *intensity;

  ret = fread(&i, 4, 1, fileIn);
	mzLen = (uLong)i;
  ret = fread(&i, 4, 1, fileIn);
	intensityLen = (uLong)i;

	compr = new unsigned char[mzLen];
	mz = new double[ms.numDataPoints];
	uncomprLen=ms.numDataPoints*sizeof(double);
  ret = fread(compr, mzLen, 1, fileIn);
	uncompress((Bytef*)mz, &uncomprLen, compr, mzLen);
	delete [] compr;

	compr = new unsigned char[intensityLen];
	intensity = new float[ms.numDataPoints];
	uncomprLen=ms.numDataPoints*sizeof(float);
  ret = fread(compr, intensityLen, 1, fileIn);
	uncompress((Bytef*)intensity, &uncomprLen, compr, intensityLen);
	delete [] compr;

	for(i=0;i<ms.numDataPoints;i++){
		p.mz = mz[i];
		p.intensity = intensity[i];
		s.add(p);
	}

	delete [] mz;
	delete [] intensity;

}

void MSReader::writeTextSpec(FILE* fileOut, Spectrum& s) {

	int i,j,k;
	char t[64];

  if(exportMGF){
    //MGF spectrum header is here
    if(highResMGF){
      for(i=0;i<s.sizeZ();i++){
        fprintf(fileOut,"BEGIN IONS\n");
        fprintf(fileOut,"PEPMASS=%.*f\n",6,(s.atZ(i).mh+(s.atZ(i).z-1)*1.007276466)/s.atZ(i).z);
        fprintf(fileOut,"CHARGE=%d+\n",s.atZ(i).z);
        fprintf(fileOut,"RTINSECONDS=%d\n",(int)(s.getRTime()*60));
        fprintf(fileOut,"TITLE=%s.%d.%d.%d %d %.4f\n","test",s.getScanNumber(),s.getScanNumber(true),s.atZ(i).z,i,s.getRTime());
        for(j=0;j<s.size();j++){
		      sprintf(t,"%.*f",iIntensityPrecision,s.at(j).intensity);
		      k=(int)strlen(t);
		      if(k>2 && iIntensityPrecision>0){
		        if(t[0]=='0'){
		          fprintf(fileOut,"%.*f 0\n",iMZPrecision,s.at(j).mz);
			      } else if(t[k-1]=='0'){
			        fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision-1,s.at(j).intensity);
       			} else {
			        fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
			      }
		      } else {
			      fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
		      }
	      }
        fprintf(fileOut,"END IONS\n");
      }

    } else {
      fprintf(fileOut,"BEGIN IONS\n");
      fprintf(fileOut,"PEPMASS=%.*f\n",6,s.getMZ());
      fprintf(fileOut,"RTINSECONDS=%d\n",(int)(s.getRTime()*60));
      if(s.sizeZ()==1){
        if(s.atZ(0).z==1) fprintf(fileOut,"CHARGE=1+\n");
        fprintf(fileOut,"TITLE=%s.%d.%d.%d %d %.4f\n","test",s.getScanNumber(),s.getScanNumber(true),s.atZ(0).z,0,s.getRTime());
      } else {
        fprintf(fileOut,"TITLE=%s.%d.%d.%d %d %.4f\n","test",s.getScanNumber(),s.getScanNumber(true),0,0,s.getRTime());
      }
      for(j=0;j<s.size();j++){
		    sprintf(t,"%.*f",iIntensityPrecision,s.at(j).intensity);
		    k=(int)strlen(t);
		    if(k>2 && iIntensityPrecision>0){
		      if(t[0]=='0'){
		        fprintf(fileOut,"%.*f 0\n",iMZPrecision,s.at(j).mz);
			    } else if(t[k-1]=='0'){
			      fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision-1,s.at(j).intensity);
      		} else {
			      fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
			    }
		    } else {
			    fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
		    }
	    }
      fprintf(fileOut,"END IONS\n");
    }
    return;
  }

  //Only use this code if not writing MGF file
	for(j=0;j<s.size();j++){
    fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
	}

}

void MSReader::writeBinarySpec(FILE* fileOut, Spectrum& s) {
	int j;

	for(j=0;j<s.size();j++){
		fwrite(&s.at(j).mz,8,1,fileOut);
		fwrite(&s.at(j).intensity,4,1,fileOut);
	}

}

void MSReader::writeSpecHeader(FILE* fileOut, bool text, Spectrum& s) {

	//MSScanInfo ms;
  double d;
  float f;
  int i;
	MSSpectrumType mft;
	int j;

	//output scan info
	if(text){

    //MSx spectrum header is here
		mft=s.getFileType();
   	if(mft==MS2 || mft==MS3 || mft==SRM){
    	fprintf(fileOut,"S\t%d\t%d",s.getScanNumber(),s.getScanNumber(true));
			for(i=0;i<s.sizeMZ();i++){
				fprintf(fileOut,"\t%.*lf",4,s.getMZ());
			}
			fprintf(fileOut,"\n");
		} else {
	  	fprintf(fileOut,"S\t%d\t%d\n",s.getScanNumber(),s.getScanNumber(true));
		}
  	if(s.getRTime()>0) fprintf(fileOut,"I\tRTime\t%.*f\n",4,s.getRTime());
    if(s.getBPI()>0) fprintf(fileOut,"I\tBPI\t%.*f\n",2,s.getBPI());
    if(s.getBPM()>0) fprintf(fileOut,"I\tBPM\t%.*f\n",4,s.getBPM());
    if(s.getConversionA()!=0) fprintf(fileOut,"I\tConvA\t%.*f\n",6,s.getConversionA());
    if(s.getConversionB()!=0) fprintf(fileOut,"I\tConvB\t%.*f\n",6,s.getConversionB());
		if(s.getConversionC()!=0) fprintf(fileOut,"I\tConvC\t%.*f\n",6,s.getConversionC());
    if(s.getConversionD()!=0) fprintf(fileOut,"I\tConvD\t%.*f\n",6,s.getConversionD());
		if(s.getConversionE()!=0) fprintf(fileOut,"I\tConvE\t%.*f\n",6,s.getConversionE());
    if(s.getConversionI()!=0) fprintf(fileOut,"I\tConvI\t%.*f\n",6,s.getConversionI());
    if(s.getTIC()>0) fprintf(fileOut,"I\tTIC\t%.*f\n",2,s.getTIC());
    if(s.getIonInjectionTime()>0) fprintf(fileOut,"I\tIIT\t%.*f\n",4,s.getIonInjectionTime());
    for(j=0;j<s.sizeEZ();j++){
      fprintf(fileOut,"I\tEZ\t%d\t%.*f\t%.*f\t%.*f\n",s.atEZ(j).z,4,s.atEZ(j).mh,4,s.atEZ(j).pRTime,1,s.atEZ(j).pArea);
  	}
	  for(j=0;j<s.sizeZ();j++){
		 	fprintf(fileOut,"Z\t%d\t%.*f\n",s.atZ(j).z,4,s.atZ(j).mh);
		}

	} else {
    i=s.getScanNumber();
    fwrite(&i,4,1,fileOut);

    i=s.getScanNumber(true);
    fwrite(&i,4,1,fileOut);

		i=s.sizeMZ();
		fwrite(&i,4,1,fileOut);
		for(i=0;i<s.sizeMZ();i++){
			d=s.getMZ(i);
			fwrite(&d,8,1,fileOut);
		}

    f=s.getRTime();
    fwrite(&f,4,1,fileOut);

    f=s.getBPI();
    fwrite(&f,4,1,fileOut);

    d=s.getBPM();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionA();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionB();
    fwrite(&d,8,1,fileOut);

		d=s.getConversionC();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionD();
    fwrite(&d,8,1,fileOut);

		d=s.getConversionE();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionI();
    fwrite(&d,8,1,fileOut);

    d=s.getTIC();
    fwrite(&d,8,1,fileOut);

    f=s.getIonInjectionTime();
    fwrite(&f,4,1,fileOut);

    i=s.sizeZ();
    fwrite(&i,4,1,fileOut);

    i=s.sizeEZ();
    fwrite(&i,4,1,fileOut);

    i=s.size();
    fwrite(&i,4,1,fileOut);
    /*
		ms.scanNumber[0]=ms.scanNumber[1]=s.getScanNumber();
		ms.rTime=s.getRTime();
		ms.numDataPoints=s.size();
		ms.numZStates=s.sizeZ();
		fwrite(&ms,sizeof(MSScanInfo),1,fileOut);
    */
    for(j=0;j<s.sizeZ();j++){
			fwrite(&s.atZ(j).z,4,1,fileOut);
			fwrite(&s.atZ(j).mh,8,1,fileOut);
		}

    for(j=0;j<s.sizeEZ();j++){
			fwrite(&s.atEZ(j).z,4,1,fileOut);
			fwrite(&s.atEZ(j).mh,8,1,fileOut);
      fwrite(&s.atEZ(j).pRTime,4,1,fileOut);
      fwrite(&s.atEZ(j).pArea,4,1,fileOut);
		}
	}

}

void MSReader::readSpecHeader(FILE *fileIn, MSScanInfo &ms){
	double d;
  size_t ret;

  ret = fread(&ms.scanNumber[0], 4, 1, fileIn);
  if(feof(fileIn)) return;
  ret = fread(&ms.scanNumber[1], 4, 1, fileIn);
	if(iVersion>=5){
    ret = fread(&ms.mzCount, 4, 1, fileIn);
		if(ms.mz!=NULL) delete [] ms.mz;
		ms.mz = new double[ms.mzCount];
		for(int i=0;i<ms.mzCount;i++){
      ret = fread(&d, 8, 1, fileIn);
			ms.mz[i]=d;
		}
	} else {
    if(ms.mz!=NULL) delete [] ms.mz;
    ms.mzCount=1;
    ms.mz = new double[ms.mzCount];
    ret = fread(&ms.mz[0], 8, 1, fileIn);
	}
  ret = fread(&ms.rTime, 4, 1, fileIn);

  if(iVersion>=2){
    ret = fread(&ms.BPI, 4, 1, fileIn);
    ret = fread(&ms.BPM, 8, 1, fileIn);
    ret = fread(&ms.convA, 8, 1, fileIn);
    ret = fread(&ms.convB, 8, 1, fileIn);
		if(iVersion>=4){
      ret = fread(&ms.convC, 8, 1, fileIn);
      ret = fread(&ms.convD, 8, 1, fileIn);
      ret = fread(&ms.convE, 8, 1, fileIn);
      ret = fread(&ms.convI, 8, 1, fileIn);
		}
    ret = fread(&ms.TIC, 8, 1, fileIn);
    ret = fread(&ms.IIT, 4, 1, fileIn);
  }

  ret = fread(&ms.numZStates, 4, 1, fileIn);

  if (iVersion >= 3) ret = fread(&ms.numEZStates, 4, 1, fileIn);
  else ms.numEZStates=0;

  ret = fread(&ms.numDataPoints, 4, 1, fileIn);

}

MSFileFormat MSReader::checkFileFormat(const char *fn){

  size_t i;
	char ext[32];
	char* c;

	//extract extension & capitalize
	c=(char*)strrchr(fn,'.');
	if(c==NULL) return dunno;
	strcpy(ext,c);
	for(i=0;i<strlen(ext);i++) ext[i]=toupper(ext[i]);

  //check extension first - we must trust MS1 & MS2 & ZS & UZS
  if(strcmp(ext,".MS1")==0 ) return ms1;
  if(strcmp(ext,".MS2")==0 ) return ms2;
	if(strcmp(ext,".BMS1")==0 ) return bms1;
  if(strcmp(ext,".BMS2")==0 ) return bms2;
	if(strcmp(ext,".CMS1")==0 ) return cms1;
  if(strcmp(ext,".CMS2")==0 ) return cms2;
  if(strcmp(ext,".ZS")==0 ) return zs;
  if(strcmp(ext,".UZS")==0 ) return uzs;
  if(strcmp(ext,".MSMAT")==0 ) return msmat_ff;
  if(strcmp(ext,".RAW")==0 ) return raw;
  if(strcmp(ext,".MZXML")==0 ) return mzXML;
#ifdef MZP_HDF
  if(strcmp(ext,".MZ5")==0 ) return mz5;
  if (strcmp(ext, ".MZMLB") == 0) return mzMLb;
#else
  if (strcmp(ext, ".MZ5") == 0 || strcmp(ext, ".MZMLB") == 0) {
    cerr << "mz5 and mzMLb formats require MSToolkit to be compiled with 'MZP_HDF'. Please recompile with correct flag." << endl;
    return dunno;
  }
#endif
	if(strcmp(ext,".MZML")==0 ) return mzML;
  if(strcmp(ext,".MZMLB")==0 ) return mzMLb;
  if(strcmp(ext,".MGF")==0 ) return mgf;
	//add the sqlite3 format
  if(strcmp(ext,".SQLITE3")==0 ) return sqlite;
  if(strcmp(ext,".PSM") == 0) return psm;
	
	if(strcmp(ext,".GZ")==0 ) {
		i=c-fn;
    char tmp[1024];
		strncpy(tmp,fn,i);
		tmp[i]='\0';
		c=strrchr(tmp,'.');
		if(c==NULL) return dunno;
		strcpy(ext,c);
		for(i=0;i<strlen(ext);i++) ext[i]=toupper(ext[i]);
		if(strcmp(ext,".MZXML")==0 ) return mzXMLgz;
		if(strcmp(ext,".MZML")==0 ) return mzMLgz;
	}

  return dunno;

}

