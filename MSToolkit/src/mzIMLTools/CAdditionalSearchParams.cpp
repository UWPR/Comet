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

#include "CAdditionalSearchParams.h"

using namespace std;

CAdditionalSearchParams::CAdditionalSearchParams(){
  sCvParam cv;
  cvParam = new vector<sCvParam>;
  cvParam->push_back(cv);

  sUserParam u;
  userParam = new vector<sUserParam>;
  userParam->push_back(u);
}

CAdditionalSearchParams::CAdditionalSearchParams(const CAdditionalSearchParams& c){
  size_t i;
  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
  for (i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
  for (i = 0; i<c.userParam->size(); i++) userParam->push_back(c.userParam->at(i));
}

CAdditionalSearchParams::~CAdditionalSearchParams(){
  delete cvParam;
  delete userParam;
}

CAdditionalSearchParams& CAdditionalSearchParams::operator=(const CAdditionalSearchParams& c){
  if (this != &c){
    size_t i;
    delete cvParam;
    delete userParam;
    cvParam = new vector<sCvParam>;
    userParam = new vector<sUserParam>;
    for (i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
    for (i = 0; i<c.userParam->size(); i++) userParam->push_back(c.userParam->at(i));
  }
  return *this;
}

bool CAdditionalSearchParams::operator==(const CAdditionalSearchParams& c){
  if (this == &c) return true;
  if (cvParam->size() != c.cvParam->size()) return false;
  if (userParam->size() != c.userParam->size()) return false;
  size_t i, j;
  for (i = 0; i < cvParam->size(); i++){
    for (j = 0; j < c.cvParam->size(); j++) {
      if (cvParam->at(i) == c.cvParam->at(j)) break;
    }
    if (j == c.cvParam->size()) return false;
  }
  for (i = 0; i < userParam->size(); i++){
    for (j = 0; j < c.userParam->size(); j++) {
      if (userParam->at(i) == c.userParam->at(j)) break;
    }
    if (j == c.userParam->size()) return false;
  }
  return true;
}

bool CAdditionalSearchParams::operator!=(const CAdditionalSearchParams& c){
  return !operator==(c);
}

void CAdditionalSearchParams::writeOut(FILE* f, int tabs){
  if(cvParam->size()==0 && userParam->size()==0) return;
  
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<AdditionalSearchParams>\n");

  size_t j;
  if (tabs > -1){
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AdditionalSearchParams>\n");
}
