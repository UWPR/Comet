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

#include "CSpectrumIdentificationList.h"

using namespace std;

CSpectrumIdentificationList::CSpectrumIdentificationList(){
  numSequencesSearched=0;
}
//
//CSpectrumIdentificationList::CSpectrumIdentificationList(const CSpectrumIdentificationList& s){
//  id=s.id;
//  name=s.name;
//  numSequencesSearched=s.numSequencesSearched;
//
//  fragmentationTable=s.fragmentationTable;
//
//  size_t i;
//  spectrumIdentificationResult = new vector<CSpectrumIdentificationResult>;
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//  for (i = 0; i<s.spectrumIdentificationResult->size(); i++) spectrumIdentificationResult->push_back(s.spectrumIdentificationResult->at(i));
//  for (i = 0; i<s.cvParam->size(); i++) cvParam->push_back(s.cvParam->at(i));
//  for (i = 0; i<s.userParam->size(); i++) userParam->push_back(s.userParam->at(i));
//}
//
//CSpectrumIdentificationList::~CSpectrumIdentificationList(){
//  delete spectrumIdentificationResult;
//  delete cvParam;
//  delete userParam;
//}
//
//CSpectrumIdentificationList& CSpectrumIdentificationList::operator=(const CSpectrumIdentificationList& s){
//  if (this != &s){
//    id = s.id;
//    name = s.name;
//    numSequencesSearched = s.numSequencesSearched;
//
//    fragmentationTable = s.fragmentationTable;
//
//    size_t i;
//    delete spectrumIdentificationResult;
//    delete cvParam;
//    delete userParam;
//    spectrumIdentificationResult = new vector<CSpectrumIdentificationResult>;
//    cvParam = new vector<sCvParam>;
//    userParam = new vector<sUserParam>;
//    for (i = 0; i<s.spectrumIdentificationResult->size(); i++) spectrumIdentificationResult->push_back(s.spectrumIdentificationResult->at(i));
//    for (i = 0; i<s.cvParam->size(); i++) cvParam->push_back(s.cvParam->at(i));
//    for (i = 0; i<s.userParam->size(); i++) userParam->push_back(s.userParam->at(i));
//  }
//  return *this;
//}

//iterate through sequences to see if we have it already
//if so, return the existing id, else add this new one
//CSpectrumIdentificationResult* CSpectrumIdentificationList::addSpectrumIdentificationResult(string specID, string& sdRef) {
//  //get rid of any null placeholders
//  if (spectrumIdentificationResult->at(0).id.compare("null") == 0) spectrumIdentificationResult->clear();
//
//  /* This would only be necessary if results somehow become uncopled from identification items
//  string key=specID+sdRef;
//
//  //Find if peptide already listed by binary search
//  size_t sz = vSIRTable.size() - vSIRTable.size() % 100; //buffer of 100 unsorted entries
//  size_t lower = 0;
//  size_t mid = sz / 2;
//  size_t upper = sz;
//  int i;
//
//  if (sz>0){
//    i = vSIRTable[mid].key.compare(key);
//    while (i != 0){
//      if (lower >= upper) break;
//      if (i>0){
//        if (mid == 0) break;
//        upper = mid - 1;
//        mid = (lower + upper) / 2;
//      } else {
//        lower = mid + 1;
//        mid = (lower + upper) / 2;
//      }
//      if (mid == sz) break;
//      i = vSIRTable[mid].key.compare(key);
//    }
//
//    //match by peptide sequence, so check modifications
//    if (i == 0){
//      return &spectrumIdentificationResult->at(vSIRTable[mid].index);
//    }
//  }
//
//  //check unsorted proteins
//  for (mid = sz; mid < vSIRTable.size(); mid++){
//    if (vSIRTable[mid].key.compare(key) == 0) &spectrumIdentificationResult->at(vSIRTable[mid].index);
//  }
//  */
//
//  //check if we are adding to last result
//  if (spectrumIdentificationResult->size()>0){
//    if (spectrumIdentificationResult->back().spectrumID.compare(specID) == 0 && spectrumIdentificationResult->back().spectraDataRef.compare(sdRef)==0){
//      return &spectrumIdentificationResult->back();
//    }
//  }
//
//  //add new result
//  CSpectrumIdentificationResult sir;
//  char dbid[32];
//  sprintf(dbid, "%s_%zu", &sdRef[0], spectrumIdentificationResult->size());
//  sir.id = dbid;
//  sir.spectrumID=specID;
//  sir.spectraDataRef=sdRef;
//  spectrumIdentificationResult->push_back(sir);
//
//  return &spectrumIdentificationResult->back();
//}
//
//CSpectrumIdentificationResult* CSpectrumIdentificationList::addSpectrumIdentificationResult(CSpectrumIdentificationResult& c) {
//  //get rid of any null placeholders
//  if (spectrumIdentificationResult->at(0).id.compare("null") == 0) spectrumIdentificationResult->clear();
//
//  //add new result
//  spectrumIdentificationResult->push_back(c);
//  return &spectrumIdentificationResult->back();
//}

void CSpectrumIdentificationList::writeOut(FILE* f, int tabs){
  if (id.empty()){
    cerr << "SpectrumIdentificationList::id is required." << endl;
    exit(69);
  }
  if (spectrumIdentificationResult.empty()){
    cerr << "SpectrumIdentificationList::SpectrumIdentificationResult is required." << endl;
    exit(69);
  }

  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectrumIdentificationList id=\"%s\"",&id[0]);
  if(numSequencesSearched>0) fprintf(f," numSequencesSearched=\"%d\"",numSequencesSearched);
  fprintf(f, ">\n");

  int t=tabs;
  if(t>-1) t++;

  for(j=0;j<fragmentationTable.size();j++) fragmentationTable[j].writeOut(f,t);
  for (j = 0; j < spectrumIdentificationResult.size(); j++) spectrumIdentificationResult[j].writeOut(f, t);
  for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, t);
  for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectrumIdentificationList>\n");
}
