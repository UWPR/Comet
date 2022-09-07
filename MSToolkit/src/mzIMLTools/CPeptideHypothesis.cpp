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

#include "CPeptideHypothesis.h"

using namespace std;

//CPeptideHypothesis::CPeptideHypothesis(){
//  peptideEvidenceRef = "null";
//
//  sSpectrumIdentificationItemRef sii;
//  spectrumIdentificationItemRef = new vector<sSpectrumIdentificationItemRef>;
//  spectrumIdentificationItemRef->push_back(sii);
//
//}
//
//CPeptideHypothesis::CPeptideHypothesis(const CPeptideHypothesis& c){
//  peptideEvidenceRef = c.peptideEvidenceRef;
//
//  spectrumIdentificationItemRef = new vector<sSpectrumIdentificationItemRef>;
//  for (size_t i = 0; i<c.spectrumIdentificationItemRef->size(); i++) spectrumIdentificationItemRef->push_back(c.spectrumIdentificationItemRef->at(i));
//
//}
//
//CPeptideHypothesis::~CPeptideHypothesis(){
//  delete spectrumIdentificationItemRef;
//}
//
//CPeptideHypothesis& CPeptideHypothesis::operator=(const CPeptideHypothesis& c){
//  if (this != &c){
//    peptideEvidenceRef = c.peptideEvidenceRef;
//    delete spectrumIdentificationItemRef;
//    spectrumIdentificationItemRef = new vector<sSpectrumIdentificationItemRef>;
//    for (size_t i = 0; i<c.spectrumIdentificationItemRef->size(); i++) spectrumIdentificationItemRef->push_back(c.spectrumIdentificationItemRef->at(i));
//  }
//  return *this;
//
//}

//void CPeptideHypothesis::addSpectrumIdentificationItemRef(string& ref){
//  if (spectrumIdentificationItemRef->at(0).text.compare("null") == 0) spectrumIdentificationItemRef->clear();
//
//  sSpectrumIdentificationItemRef s;
//  s.text=ref;
//  spectrumIdentificationItemRef->push_back(s);
//}

void CPeptideHypothesis::writeOut(FILE* f, int tabs){
  if (peptideEvidenceRef.empty()){
    cerr << "PeptideHypothesis::peptideEvidence_ref is required." << endl;
    exit(69);
  }
  if (spectrumIdentificationItemRef.empty()){
    cerr << "PeptideHypothesis::SpectrumIdentificationItemRef is required." << endl;
    exit(69);
  }

  int i;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<PeptideHypothesis peptideEvidence_ref=\"%s\">\n", &peptideEvidenceRef[0]);

  int t = tabs;
  if (t>-1)t++;

  for (size_t j = 0; j<spectrumIdentificationItemRef.size(); j++) spectrumIdentificationItemRef[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</PeptideHypothesis>\n");
}
