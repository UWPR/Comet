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

#include "CSpectrumIdentification.h"

using namespace std;

CSpectrumIdentification::CSpectrumIdentification(){
  activityDate.clear();
  id = "null";
  name.clear();
  spectrumIdentificationListRef = "null";
  spectrumIdentificationProtocolRef = "null";

  sInputSpectra is;
  inputSpectra = new vector<sInputSpectra>;
  inputSpectra->push_back(is);
  
  sSearchDatabaseRef sdb;
  searchDatabaseRef = new vector<sSearchDatabaseRef>;
  searchDatabaseRef->push_back(sdb);
}

CSpectrumIdentification::CSpectrumIdentification(const CSpectrumIdentification& c){
  activityDate=c.activityDate;
  id =c.id;
  name=c.name;
  spectrumIdentificationListRef=c.spectrumIdentificationListRef;
  spectrumIdentificationProtocolRef=c.spectrumIdentificationProtocolRef;

  size_t i;
  inputSpectra = new vector<sInputSpectra>;
  searchDatabaseRef = new vector<sSearchDatabaseRef>;
  for (i = 0; i<c.inputSpectra->size(); i++) inputSpectra->push_back(c.inputSpectra->at(i));
  for (i = 0; i<c.searchDatabaseRef->size(); i++) searchDatabaseRef->push_back(c.searchDatabaseRef->at(i));
}

CSpectrumIdentification::~CSpectrumIdentification(){
  delete inputSpectra;
  delete searchDatabaseRef;
}

CSpectrumIdentification& CSpectrumIdentification::operator=(const CSpectrumIdentification& c){
  if (this != &c){
    activityDate = c.activityDate;
    id = c.id;
    name = c.name;
    spectrumIdentificationListRef = c.spectrumIdentificationListRef;
    spectrumIdentificationProtocolRef = c.spectrumIdentificationProtocolRef;

    size_t i;
    delete inputSpectra;
    delete searchDatabaseRef;
    inputSpectra = new vector<sInputSpectra>;
    searchDatabaseRef = new vector<sSearchDatabaseRef>;
    for (i = 0; i<c.inputSpectra->size(); i++) inputSpectra->push_back(c.inputSpectra->at(i));
    for (i = 0; i<c.searchDatabaseRef->size(); i++) searchDatabaseRef->push_back(c.searchDatabaseRef->at(i));
  }
  return *this;
}

//this is a really slow comparison; it's the nested loops...
bool CSpectrumIdentification::operator==(const CSpectrumIdentification& c){
  if (this == &c) return true;
  if (activityDate.compare(c.activityDate)!=0) return false;
  if (id.compare(c.id)!=0) return false;
  if (name.compare(c.name)!=0) return false;
  if (spectrumIdentificationListRef.compare(c.spectrumIdentificationListRef) != 0) return false;
  if (spectrumIdentificationProtocolRef.compare(c.spectrumIdentificationProtocolRef)!=0) return false;

  size_t i, j;
  if (inputSpectra->size() != c.inputSpectra->size()) return false;
  for (i = 0; i < inputSpectra->size(); i++){
    for (j = 0; j < c.inputSpectra->size(); j++){
      if (inputSpectra->at(i) == c.inputSpectra->at(j)) break;
    }
    if (j == c.inputSpectra->size()) return false;
  }
  if (searchDatabaseRef->size() != c.searchDatabaseRef->size()) return false;
  for (i = 0; i < searchDatabaseRef->size(); i++){
    for (j = 0; j < c.searchDatabaseRef->size(); j++){
      if (searchDatabaseRef->at(i) == c.searchDatabaseRef->at(j)) break;
    }
    if (j == c.searchDatabaseRef->size()) return false;
  }

  return true;
}

//this is a really slow comparison; it's the nested loops...
//like the comparison operator, but only checks the database and spectra names
bool CSpectrumIdentification::compare(const CSpectrumIdentification& c){
  if (this == &c) return true;

  size_t i, j;
  if (inputSpectra->size() != c.inputSpectra->size()) return false;
  for (i = 0; i < inputSpectra->size(); i++){
    for (j = 0; j < c.inputSpectra->size(); j++){
      if (inputSpectra->at(i) == c.inputSpectra->at(j)) break;
    }
    if (j == c.inputSpectra->size()) return false;
  }
  if (searchDatabaseRef->size() != c.searchDatabaseRef->size()) return false;
  for (i = 0; i < searchDatabaseRef->size(); i++){
    for (j = 0; j < c.searchDatabaseRef->size(); j++){
      if (searchDatabaseRef->at(i) == c.searchDatabaseRef->at(j)) break;
    }
    if (j == c.searchDatabaseRef->size()) return false;
  }

  return true;
}

void CSpectrumIdentification::writeOut(FILE* f, int tabs){

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectrumIdentification id=\"%s\" spectrumIdentificationList_ref=\"%s\" spectrumIdentificationProtocol_ref=\"%s\"",&id[0],&spectrumIdentificationListRef[0],&spectrumIdentificationProtocolRef[0]);
  if(activityDate.size()>0) fprintf(f, " activityDate=\"%s\"", &activityDate[0]);
  if(name.size()>0) fprintf(f, " name=\"%s\"",&name[0]);
  fprintf(f,">\n");

  size_t j;
  if (tabs>-1) {
    for (j = 0; j<inputSpectra->size(); j++) inputSpectra->at(j).writeOut(f, tabs + 1); 
    for (j = 0; j<searchDatabaseRef->size(); j++) searchDatabaseRef->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<inputSpectra->size(); j++) inputSpectra->at(j).writeOut(f);
    for (j = 0; j<searchDatabaseRef->size(); j++) searchDatabaseRef->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectrumIdentification>\n");

}
