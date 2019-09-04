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

#include "CProteinDetectionList.h"

using namespace std;

CProteinDetectionList::CProteinDetectionList(){
  id = "null";
  name.clear();
  
  proteinAmbiguityGroup = new vector<CProteinAmbiguityGroup>;
  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
}

CProteinDetectionList::~CProteinDetectionList(){
  delete proteinAmbiguityGroup;
  delete cvParam;
  delete userParam;
}

void CProteinDetectionList::writeOut(FILE* f, int tabs){
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ProteinDetectionList id=\"%s\">\n",&id[0]);
  for (size_t j = 0; j < proteinAmbiguityGroup->size(); j++){
    if (tabs>-1) proteinAmbiguityGroup->at(j).writeOut(f, tabs + 1);
    else proteinAmbiguityGroup->at(j).writeOut(f);
  }
  for (size_t j = 0; j < cvParam->size(); j++){
    if (tabs>-1) cvParam->at(j).writeOut(f, tabs + 1);
    else cvParam->at(j).writeOut(f);
  }
  for (size_t j = 0; j < userParam->size(); j++){
    if (tabs>-1) userParam->at(j).writeOut(f, tabs + 1);
    else userParam->at(j).writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ProteinDetectionList>\n");
}
