/*
Copyright 2017, Michael R. Hoopmann, Institute for Systems Biology
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

CInputs::CInputs(){
  sourceFile = new vector<CSourceFile>;
  searchDatabase = new vector<CSearchDatabase>;

  CSpectraData sd;
  spectraData = new vector<CSpectraData>;
  spectraData->push_back(sd);
}

CInputs::~CInputs(){
  delete sourceFile;
  delete searchDatabase;
  delete spectraData;
}

string CInputs::addSearchDatabase(string& s){
  //check if DB is already in list
  size_t i;
  for (i = 0; i < searchDatabase->size(); i++){
    if (searchDatabase->at(i).location.compare(s) == 0) return searchDatabase->at(i).id;
  }

  CSearchDatabase sdb;
  char cID[32];
  sprintf(cID, "SDB%zu", spectraData->size());
  sdb.id = cID;
  sdb.location = s;
  i = s.find_last_of("\\/");
  sdb.databaseName.userParam.name = s.substr(i + 1);
  
  i = s.find_last_of(".");
  string format = s.substr(i+1);
  if (format.compare("fasta") == 0){
    sdb.fileFormat.cvParam.cvRef = "PSI-MS";
    sdb.fileFormat.cvParam.accession = "MS:1001348";
    sdb.fileFormat.cvParam.name = "FASTA format";
  }

  //TODO: add optional information

  searchDatabase->push_back(sdb);
  return sdb.id;
}

//Adds the spectrum data file information and returns a reference id.
//FileFormat is determined by evaluating the extension
string CInputs::addSpectraData(string& s){
  //overwrite null file
  if (spectraData->at(0).id.compare("null") == 0) spectraData->clear();

  //check if file is already in list
  size_t i;
  for (i = 0; i < spectraData->size(); i++){
    if (spectraData->at(i).location.compare(s) == 0) return spectraData->at(i).id;
  }

  CSpectraData sd;
  char cID[32];
  sprintf(cID, "SF%zu", spectraData->size());
  sd.id = cID;
  sd.location = s;

  //TODO: check file format & spectrumIDFormat (I think this is the spectrum.scan.scan.charge format....)
  sd.fileFormat.cvParam = checkFileFormat(s);
  sd.spectrumIDFormat.cvParam = checkSpectrumIDFormat(sd.fileFormat.cvParam);

  spectraData->push_back(sd);
  return sd.id;
}

string CInputs::addSpectraData(CSpectraData& c){
  //overwrite null file
  if (spectraData->at(0).id.compare("null") == 0) spectraData->clear();

  if (c.id.compare("null") == 0){
    char cID[32];
    sprintf(cID, "SF%zu", spectraData->size());
    c.id = cID;
  }

  spectraData->push_back(c);
  return c.id;
}

sCvParam CInputs::checkFileFormat(string s){

  size_t i;
  string ext;
  sCvParam cv;

  //extract extension & capitalize
  i = s.rfind('.');
  if (i != string::npos){
    ext = s.substr(i);
    for (i = 0; i<ext.size(); i++) ext[i] = toupper(ext[i]);
    if (ext.compare(".MGF") == 0) {
      cv.accession="MS:1001062"; cv.name="Mascot MGF file";
    } else if (ext.compare(".MZXML") == 0){
      cv.accession = "MS:1000566"; cv.name = "ISB mzXML format";
    } else if (ext.compare(".MZML") == 0){
      cv.accession = "MS:1000584"; cv.name = "mzML format";
    } else if (ext.compare(".MZ5") == 0){
      cv.accession = "MS:1001881"; cv.name = "mz5 format";
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
  }
  if (cv.accession.compare("null") != 0) cv.cvRef = "PSI-MS";

  return cv;

}

void CInputs::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Inputs>\n");
  for (j = 0; j<sourceFile->size(); j++){
    if (tabs>-1) sourceFile->at(j).writeOut(f, tabs + 1);
    else sourceFile->at(j).writeOut(f);
  }
  for (j = 0; j<searchDatabase->size(); j++){
    if (tabs>-1) searchDatabase->at(j).writeOut(f, tabs + 1);
    else searchDatabase->at(j).writeOut(f);
  }
  for (j = 0; j<spectraData->size(); j++){
    if (tabs>-1) spectraData->at(j).writeOut(f, tabs + 1);
    else spectraData->at(j).writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Inputs>\n");
}
