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

#include "CFragmentation.h"

using namespace std;

CFragmentation::CFragmentation(){
  CIonType it;
  ionType = new vector<CIonType>;
  ionType->push_back(it);
}

CFragmentation::CFragmentation(const CFragmentation& f){
  ionType = new vector<CIonType>;
  for (size_t i = 0; i<f.ionType->size(); i++) ionType->push_back(f.ionType->at(i));
}

CFragmentation::~CFragmentation(){
  delete ionType;
}

CFragmentation& CFragmentation::operator=(const CFragmentation& f){
  if (this != &f){
    delete ionType;
    ionType = new vector<CIonType>;
    for (size_t i = 0; i<f.ionType->size(); i++) ionType->push_back(f.ionType->at(i));
  }
  return *this;
}

void CFragmentation::writeOut(FILE* f, int tabs){

}
