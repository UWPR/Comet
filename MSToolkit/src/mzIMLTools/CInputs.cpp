/*
Copyright 2020, Michael R. Hoopmann, Institute for Systems Biology
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

#include "CInputs.h"

using namespace std;

//CInputs::CInputs(){
//  sourceFile = new vector<CSourceFile>;
//  searchDatabase = new vector<CSearchDatabase>;
//
//  CSpectraData sd;
//  spectraData = new vector<CSpectraData>;
//  spectraData->push_back(sd);
//}
//
//CInputs::~CInputs(){
//  delete sourceFile;
//  delete searchDatabase;
//  delete spectraData;
//}

CSearchDatabase* CInputs::addSearchDatabase(string loc){

  size_t i;

  //not using the path+name (i.e. location) anymore. This causes duplicates when someone passes the same file from multiple locations.
  //check if DB is already in list  
  //for (i = 0; i < searchDatabase.size(); i++){
  //  if (searchDatabase[i].location.compare(loc) == 0) return &searchDatabase[i];
  //}

  CSearchDatabase sdb;
  char cID[32];
  sprintf(cID, "SDB%d", (int)spectraData.size());
  sdb.id = cID;
  sdb.location = loc;
  i = loc.find_last_of("\\/");
  sdb.databaseName.userParam.name = loc.substr(i + 1);
  
  i = loc.find_last_of(".");
  string format = loc.substr(i+1);
  if (format.compare("fasta") == 0){
    sdb.fileFormat.cvParam.cvRef = "PSI-MS";
    sdb.fileFormat.cvParam.accession = "MS:1001348";
    sdb.fileFormat.cvParam.name = "FASTA format";
  }

  //check if DB is already in list  
  for (i = 0; i < searchDatabase.size(); i++){
    if (searchDatabase[i].databaseName.userParam.name.compare(sdb.databaseName.userParam.name) == 0) return &searchDatabase[i];
  }

  //TODO: add optional information

  searchDatabase.push_back(sdb);
  return &searchDatabase.back();
}

//Adds the spectrum data file information and returns a reference id.
//FileFormat is determined by evaluating the extension
CSpectraData* CInputs::addSpectraData(string loc){

  //check if file is already in list
  size_t i;
  for (i = 0; i < spectraData.size(); i++){
    if (spectraData[i].location.compare(loc) == 0) return &spectraData[i];
  }

  CSpectraData c;
  char cID[32];
  sprintf(cID, "SF%d", (int)spectraData.size());
  c.id = cID;
  c.location = loc;

  //TODO: check file format & spectrumIDFormat (I think this is the spectrum.scan.scan.charge format....)
  c.fileFormat.cvParam = checkFileFormat(loc);
  c.spectrumIDFormat.cvParam = checkSpectrumIDFormat(c.fileFormat.cvParam);

  spectraData.push_back(c);
  return &spectraData.back();
}

sCvParam CInputs::checkFileFormat(string s){

  size_t i;
  string ext;
  sCvParam cv;

  //extract extension & capitalize
  i = s.rfind('.');
  if (i != string::npos){
    ext = s.substr(i);
    for (size_t a = 0; a<ext.size(); a++) ext[a] = toupper(ext[a]);
    if (ext.compare(".GZ") == 0) {
      size_t j = s.rfind('.',i-1);
      string ext2=s.substr(j);
      for (size_t a = 0; a < ext2.size(); a++) ext2[a] = toupper(ext2[a]);
      if(ext2.compare(".MZML.GZ")==0){
        cv.accession = "MS:1000584"; cv.name = "mzML format";
      } else if(ext2.compare(".MZXML.GZ")==0){
        cv.accession = "MS:1000566"; cv.name = "ISB mzXML format";
      } else {
        cerr  << "mzIMLTools CInputs::checkFileFormat(): Cannot find CV for extension: " << ext2 << endl;
        exit(1);
      }
    } else if (ext.compare(".MGF") == 0) {
      cv.accession="MS:1001062"; cv.name="Mascot MGF file";
    } else if (ext.compare(".MZXML") == 0){
      cv.accession = "MS:1000566"; cv.name = "ISB mzXML format";
    } else if (ext.compare(".MZML") == 0){
      cv.accession = "MS:1000584"; cv.name = "mzML format";
    } else if (ext.compare(".MZ5") == 0){
      cv.accession = "MS:1001881"; cv.name = "mz5 format";
    } else {
      cerr << "mzIMLTools CInputs::checkFileFormat(): Cannot find CV for extension: " << ext << endl;
      exit(1);
    }
  }
  if (cv.accession.compare("null") != 0) cv.cvRef = "PSI-MS";

  return cv;

}

sCvParam CInputs::checkSpectrumIDFormat(sCvParam& s){

  sCvParam cv;

  //match terms for known file formats
  if (s.accession.compare("MS:1000584") == 0){
    cv.accession = "MS:1001530"; cv.name = "mzML unique identifier";
  } else if (s.accession.compare("MS:1000566") == 0){ //note that mzXML is being given mzML identifier
    cv.accession = "MS:1001530"; cv.name = "mzML unique identifier"; 
  } else if (s.accession.compare("MS:1001062") == 0){ 
    cv.accession = "MS:1000774"; cv.name = "multiple peak list nativeID format";
  }
  if (cv.accession.compare("null") != 0) cv.cvRef = "PSI-MS";

  return cv;

}

void CInputs::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Inputs>\n");
  if(tabs>-1){
    for (j = 0; j<sourceFile.size(); j++) sourceFile[j].writeOut(f, tabs + 1);
    for (j = 0; j<searchDatabase.size(); j++) searchDatabase[j].writeOut(f, tabs + 1);
    for (j = 0; j<spectraData.size(); j++) spectraData[j].writeOut(f, tabs + 1);
  } else{
    for (j = 0; j<sourceFile.size(); j++) sourceFile[j].writeOut(f);
    for (j = 0; j<searchDatabase.size(); j++) searchDatabase[j].writeOut(f);
    for (j = 0; j<spectraData.size(); j++) spectraData[j].writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Inputs>\n");
}
