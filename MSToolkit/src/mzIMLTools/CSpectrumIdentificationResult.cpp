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

#include "CSpectrumIdentificationResult.h"

using namespace std;

//CSpectrumIdentificationResult::CSpectrumIdentificationResult(){
//  id = "null";
//  name.clear();
//  spectraDataRef = "null";
//  spectrumID = "null";
//
//  CSpectrumIdentificationItem sii;
//  spectrumIdentificationItem = new vector<CSpectrumIdentificationItem>;
//  spectrumIdentificationItem->push_back(sii);
//
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//}
//
//CSpectrumIdentificationResult::CSpectrumIdentificationResult(const CSpectrumIdentificationResult& s){
//  id = s.id;
//  name = s.name;
//  spectraDataRef = s.spectraDataRef;
//  spectrumID = s.spectrumID;
//
//  size_t i;
//  spectrumIdentificationItem = new vector<CSpectrumIdentificationItem>;
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//  for (i = 0; i<s.spectrumIdentificationItem->size(); i++) spectrumIdentificationItem->push_back(s.spectrumIdentificationItem->at(i));
//  for (i = 0; i<s.cvParam->size(); i++) cvParam->push_back(s.cvParam->at(i));
//  for (i = 0; i<s.userParam->size(); i++) userParam->push_back(s.userParam->at(i));
//}
//
//CSpectrumIdentificationResult::~CSpectrumIdentificationResult(){
//  delete spectrumIdentificationItem;
//  delete cvParam;
//  delete userParam;
//}
//
//CSpectrumIdentificationResult& CSpectrumIdentificationResult::operator=(const CSpectrumIdentificationResult& s){
//  if (this != &s){
//    id = s.id;
//    name = s.name;
//    spectraDataRef = s.spectraDataRef;
//    spectrumID = s.spectrumID;
//
//    size_t i;
//    delete spectrumIdentificationItem;
//    delete cvParam;
//    delete userParam;
//    spectrumIdentificationItem = new vector<CSpectrumIdentificationItem>;
//    cvParam = new vector<sCvParam>;
//    userParam = new vector<sUserParam>;
//    for (i = 0; i<s.spectrumIdentificationItem->size(); i++) spectrumIdentificationItem->push_back(s.spectrumIdentificationItem->at(i));
//    for (i = 0; i<s.cvParam->size(); i++) cvParam->push_back(s.cvParam->at(i));
//    for (i = 0; i<s.userParam->size(); i++) userParam->push_back(s.userParam->at(i));
//  }
//  return *this;
//}

void CSpectrumIdentificationResult::addCvParam(string id, int value){
  char str[32];
  sprintf(str, "%d", value);
  addCvParam(id, string(str));
}

void CSpectrumIdentificationResult::addCvParam(string id, double value){
  char str[32];
  if ((int)value == value){
    sprintf(str, "%d", (int)value);
  } else {
    sprintf(str, "%.4lf", value);
  }
  addCvParam(id, string(str));
}

void CSpectrumIdentificationResult::addCvParam(string id, string value){
  sCvParam cv;
  cv.accession = id;
  cv.cvRef = "PSI-MS";
  if (id.compare("MS:1000894") == 0) {
    cv.name = "retention time";
    cv.unitAccession = "UO:0000010";
    cv.unitName = "second";
    cv.unitCvRef = "UO";
  }

  if (cv.name.compare("null") == 0) {
    sUserParam u;
    u.name = id;
    u.value = value;
    userParam.push_back(u);
  } else {
    cv.value = value;
    cvParam.push_back(cv);
  }
}

//iterate through sequences to see if we have it already
//if so, return the existing id, else add this new one
//CSpectrumIdentificationItem* CSpectrumIdentificationResult::addSpectrumIdentificationItem(int z, double expMZ, int rnk, vector<sPeptideEvidenceRef>& peRef, bool pass, string pRef) {
//  //get rid of any null placeholders
//  if (spectrumIdentificationItem->at(0).id.compare("null") == 0) spectrumIdentificationItem->clear();
//
//  //add new result
//  CSpectrumIdentificationItem sii;
//  char dbid[32];
//  sprintf(dbid, "%s_%zu", &id[0], spectrumIdentificationItem->size());
//  sii.id = dbid;
//  sii.chargeState=z;
//  sii.experimentalMassToCharge=expMZ;
//  sii.passThreshold=pass;
//  sii.rank=rnk;
//  sii.peptideRef=pRef;
//  sii.peptideEvidenceRef.clear();
//  for (size_t i = 0; i<peRef.size(); i++) sii.peptideEvidenceRef.push_back(peRef[i]);
//
//  //TODO: add optional information
//
//  spectrumIdentificationItem->push_back(sii);
//  return &spectrumIdentificationItem->at(spectrumIdentificationItem->size()-1);
//}
//
//CSpectrumIdentificationItem* CSpectrumIdentificationResult::addSpectrumIdentificationItem(CSpectrumIdentificationItem& s) {
//  //get rid of any null placeholders
//  if (spectrumIdentificationItem->at(0).id.compare("null") == 0) spectrumIdentificationItem->clear();
//
//  //add new result
//  if (s.id.compare("null") == 0){
//    char dbid[32];
//    sprintf(dbid, "%s_%zu", &id[0], spectrumIdentificationItem->size());
//    s.id = dbid;
//  }
//  spectrumIdentificationItem->push_back(s);
//  return &spectrumIdentificationItem->back();
//}

void CSpectrumIdentificationResult::writeOut(FILE* f, int tabs){
  if(id.empty()){
    cerr << "SpectrumIdentificationResult::id is required." << endl;
    exit(69);
  }
  if (spectraDataRef.empty()){
    cerr << "SpectrumIdentificationResult::spectraData_ref is required." << endl;
    exit(69);
  }
  if (spectrumID.empty()){
    cerr << "SpectrumIdentificationResult::spectrumID is required." << endl;
    exit(69);
  }
  if (spectrumIdentificationItem.empty()){
    cerr << "SpectrumIdentificationResult::SpectrumIdentificationItem is required." << endl;
    exit(69);
  }

  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectrumIdentificationResult id=\"%s\" spectrumID=\"%s\" spectraData_ref=\"%s\"", &id[0], &spectrumID[0], &spectraDataRef[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  int t=tabs;
  if(t>-1) t++;

  for (j = 0; j<spectrumIdentificationItem.size(); j++) spectrumIdentificationItem[j].writeOut(f,t);
  for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, t);
  for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectrumIdentificationResult>\n");
}

