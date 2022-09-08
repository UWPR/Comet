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

#include "CSearchModification.h"

using namespace std;

CSearchModification::CSearchModification(){
  fixedMod=false;
  massDelta=0;
}
//
//CSearchModification::CSearchModification(const CSearchModification& c){
//  fixedMod = c.fixedMod;
//  massDelta = c.massDelta;
//  residues = c.residues;
//  specificityRules=c.specificityRules;
//  cvParam = new vector<sCvParam>;
//  for(size_t i=0;i<c.cvParam->size();i++) cvParam->push_back(c.cvParam->at(i));
//}
//
//CSearchModification::~CSearchModification(){
//  delete cvParam;
//}
//
//CSearchModification& CSearchModification::operator=(const CSearchModification& c){
//  if (this != &c){
//    fixedMod = c.fixedMod;
//    massDelta = c.massDelta;
//    residues = c.residues;
//    specificityRules = c.specificityRules;
//    delete cvParam;
//    cvParam = new vector<sCvParam>;
//    for (size_t i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
//  }
//  return *this;
//}

bool CSearchModification::operator==(const CSearchModification& c){
  if (this == &c) return true;
  if(fixedMod!=c.fixedMod) return false;
  if(fabs(massDelta-c.massDelta)>0.001) return false; //fuzzy maths here because people use different precisions
  if(residues.compare(c.residues)!=0) return false;
  if (specificityRules.size()!=c.specificityRules.size()) return false;
  if (cvParam.size() != c.cvParam.size()) return false;
  size_t i,j;

  for (i = 0; i < specificityRules.size(); i++){
    for (j = 0; j < c.specificityRules.size(); j++) {
      if (specificityRules[i] == c.specificityRules[j]) break;
    }
    if (j == c.specificityRules.size()) return false;
  }

  for (i = 0; i < cvParam.size(); i++){
    for (j = 0; j < c.cvParam.size(); j++) {
      if (cvParam[i] == c.cvParam[j]) break;
    }
    if (j == c.cvParam.size()) return false;
  }
  return true;
}

bool CSearchModification::operator!=(const CSearchModification& c){
  return !operator==(c);
}

void CSearchModification::clear(){
  fixedMod = false;
  massDelta = 0;
  residues.clear();
  cvParam.clear();
  specificityRules.clear();
}

void CSearchModification::writeOut(FILE* f, int tabs){
  //if (massDelta==0){ //allowing this to be zero.
  //  cerr << "SearchModification::massDelta is required." << endl;
  //  exit(69);
  //}
  if (cvParam.empty()){
    cerr << "SearchModification::cvParam is required." << endl;
    exit(69);
  }
  if (residues.empty()){
    cerr << "SearchModification::residues is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SearchModification residues=\"%s\" massDelta=\"%.6lf\"", &residues[0], massDelta);
  if (fixedMod) fprintf(f, " fixedMod=\"true\">\n");
  else fprintf(f, " fixedMod=\"false\">\n");

  int t = tabs;
  if (t>-1)t++;

  size_t j;
  for(j=0;j<specificityRules.size();j++) specificityRules[j].writeOut(f,t);
  for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SearchModification>\n");
}