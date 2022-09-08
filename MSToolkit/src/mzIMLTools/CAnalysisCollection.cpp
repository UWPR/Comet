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

#include "CAnalysisCollection.h"

using namespace std;

//CAnalysisCollection::CAnalysisCollection(){
//  CSpectrumIdentification si;
//  spectrumIdentification = new vector<CSpectrumIdentification>;
//  spectrumIdentification->push_back(si);
//}
//
//CAnalysisCollection::CAnalysisCollection(const CAnalysisCollection& c){
//  spectrumIdentification = new vector<CSpectrumIdentification>;
//  for(size_t i=0;i<c.spectrumIdentification->size();i++) spectrumIdentification->push_back(c.spectrumIdentification->at(i));
//}
//
//CAnalysisCollection::~CAnalysisCollection(){
//  delete spectrumIdentification;
//}
//
//CAnalysisCollection& CAnalysisCollection::operator=(const CAnalysisCollection& c){
//  if (this != &c){
//    delete spectrumIdentification;
//    spectrumIdentification = new vector<CSpectrumIdentification>;
//    for (size_t i = 0; i<c.spectrumIdentification->size(); i++) spectrumIdentification->push_back(c.spectrumIdentification->at(i));
//  }
//  return *this;
//}
//
//void CAnalysisCollection::addProteinDetection(CProteinDetection& c){
//  proteinDetection.push_back(c);
//}
//
//void CAnalysisCollection::addSpectrumIdentification(CSpectrumIdentification& c){
//
//  if (c.id.compare("null") == 0){
//    char cID[32];
//    sprintf(cID, "SI%zu", spectrumIdentification->size());
//    c.id = cID;
//  }
//  spectrumIdentification->push_back(c);
//  //return &spectrumIdentification->back();
//}

void CAnalysisCollection::writeOut(FILE* f, int tabs){

  if(spectrumIdentification.size()<1){
    cerr << "AnalysisCollection::SpectrumIdentification is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<AnalysisCollection>\n");

  int t=tabs;
  if(t>-1) t++;

  size_t j;
  for (j = 0; j<spectrumIdentification.size(); j++) spectrumIdentification[j].writeOut(f,t);
  for (j=0; j<proteinDetection.size(); j++) proteinDetection[j].writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AnalysisCollection>\n");

}

