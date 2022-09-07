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

#include "CIonType.h"

using namespace std;

CIonType::CIonType(){
  charge=-1;
}

//CIonType::CIonType(const CIonType& c){
//  charge=c.charge;
//
//  size_t i;
//  index = new vector<int>;
//  fragmentArray = new vector<CFragmentArray>;
//  for (i = 0; i<c.index->size(); i++) index->push_back(c.index->at(i));
//  for (i = 0; i<c.fragmentArray->size(); i++) fragmentArray->push_back(c.fragmentArray->at(i));
//}
//
//CIonType::~CIonType(){
//  delete index;
//  delete fragmentArray;
//}
//
//CIonType& CIonType::operator=(const CIonType& c){
//  if (this != &c){
//    charge = c.charge;
//
//    size_t i;
//    delete index;
//    delete fragmentArray;
//    index = new vector<int>;
//    fragmentArray = new vector<CFragmentArray>;
//    for (i = 0; i<c.index->size(); i++) index->push_back(c.index->at(i));
//    for (i = 0; i<c.fragmentArray->size(); i++) fragmentArray->push_back(c.fragmentArray->at(i));
//  }
//  return *this;
//}

void CIonType::writeOut(FILE* f, int tabs){
  if (charge< 0){
    cerr << "CIonType::charge required." << endl;
    exit(69);
  }

  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<IonType charge=\"%d\"",charge);
  if(!index.empty()) fprintf(f, " index=\"%s\"",index.c_str());
  fprintf(f,">\n");
  if (tabs > -1){
    for (j = 0; j<fragmentArray.size(); j++) fragmentArray[j].writeOut(f, tabs + 1);
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs + 1);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<fragmentArray.size(); j++) fragmentArray[j].writeOut(f, tabs);
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, tabs);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</IonType>\n");
}
