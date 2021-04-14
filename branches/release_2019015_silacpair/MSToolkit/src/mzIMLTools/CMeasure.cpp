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

#include "CMeasure.h"

using namespace std;

CMeasure::CMeasure(){
  id = "null";
  name.clear();

  sCvParam cv;
  cvParam = new vector<sCvParam>;
  cvParam->push_back(cv);
}

CMeasure::CMeasure(const CMeasure& m){
  id = m.id;
  name = m.name;

  cvParam = new vector<sCvParam>;
  for (size_t i = 0; i<m.cvParam->size(); i++) cvParam->push_back(m.cvParam->at(i));
}

CMeasure::~CMeasure(){
  delete cvParam;
}

CMeasure& CMeasure::operator=(const CMeasure& m){
  if (this != &m){
    id = m.id;
    name = m.name;

    delete cvParam;
    cvParam = new vector<sCvParam>;
    for (size_t i = 0; i<m.cvParam->size(); i++) cvParam->push_back(m.cvParam->at(i));
  }
  return *this;
}
