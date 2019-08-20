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

#include "CSpecificityRules.h"

using namespace std;

CSpecificityRules::CSpecificityRules(){
  sCvParam cv;
  cvParam = new vector<sCvParam>;
  cvParam->push_back(cv);
}

CSpecificityRules::CSpecificityRules(const CSpecificityRules& c){
  cvParam = new vector<sCvParam>;
  for (size_t i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
}

CSpecificityRules::~CSpecificityRules(){
  delete cvParam;
}

CSpecificityRules& CSpecificityRules::operator=(const CSpecificityRules& c){
  if (this!=&c){
    delete cvParam;
    cvParam = new vector<sCvParam>;
    for (size_t i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
  }
  return *this;
}

bool CSpecificityRules::operator==(const CSpecificityRules& c){
  if (this == &c) return true;
  if (cvParam->size() != c.cvParam->size()) return false;
  size_t i,j;
  for (i = 0; i < cvParam->size(); i++){
    for (j = 0; j < c.cvParam->size(); j++) {
      if (cvParam->at(i) == c.cvParam->at(j)) break;
    }
    if (j == c.cvParam->size()) return false;
  }
  return true;
}

bool CSpecificityRules::operator!=(const CSpecificityRules& c){
  return !operator==(c);
}

void CSpecificityRules::addCvParam(sCvParam& s){
  if (cvParam->at(0).accession.compare("null") == 0) cvParam->clear();
  cvParam->push_back(s);
}

void CSpecificityRules::clear(){
  delete cvParam;
  sCvParam cv;
  cvParam = new vector<sCvParam>;
  cvParam->push_back(cv);
}

void CSpecificityRules::writeOut(FILE* f, int tabs){

  if (cvParam->at(0).accession.compare("null") == 0) return;

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpecificityRules>\n");

  size_t j;
  if (tabs>-1) {
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpecificityRules>\n");

}