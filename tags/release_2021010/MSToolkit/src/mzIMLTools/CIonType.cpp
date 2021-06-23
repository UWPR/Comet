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

#include "CIonType.h"

using namespace std;

CIonType::CIonType(){
  charge=0;
  index=new vector<int>;
  fragmentArray = new vector<CFragmentArray>;
}

CIonType::CIonType(const CIonType& c){
  charge=c.charge;

  size_t i;
  index = new vector<int>;
  fragmentArray = new vector<CFragmentArray>;
  for (i = 0; i<c.index->size(); i++) index->push_back(c.index->at(i));
  for (i = 0; i<c.fragmentArray->size(); i++) fragmentArray->push_back(c.fragmentArray->at(i));
}

CIonType::~CIonType(){
  delete index;
  delete fragmentArray;
}

CIonType& CIonType::operator=(const CIonType& c){
  if (this != &c){
    charge = c.charge;

    size_t i;
    delete index;
    delete fragmentArray;
    index = new vector<int>;
    fragmentArray = new vector<CFragmentArray>;
    for (i = 0; i<c.index->size(); i++) index->push_back(c.index->at(i));
    for (i = 0; i<c.fragmentArray->size(); i++) fragmentArray->push_back(c.fragmentArray->at(i));
  }
  return *this;
}
