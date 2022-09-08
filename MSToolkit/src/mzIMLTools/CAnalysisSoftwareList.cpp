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

#include "CAnalysisSoftwareList.h"

using namespace std;

//CAnalysisSoftwareList::CAnalysisSoftwareList(){
//  CAnalysisSoftware as;
//  analysisSoftware = new vector<CAnalysisSoftware>;
//  analysisSoftware->push_back(as);
//}
//
//CAnalysisSoftwareList::~CAnalysisSoftwareList(){
//  delete analysisSoftware;
//}
//
//CAnalysisSoftware& CAnalysisSoftwareList::operator[](const size_t& index){
//  return analysisSoftware->at(index);
//}

//void CAnalysisSoftwareList::addAnalysisSoftware(CAnalysisSoftware& as){
//  analysisSoftware->push_back(as);
//}
//
//void CAnalysisSoftwareList::clear(){
//  analysisSoftware->clear();
//}

void CAnalysisSoftwareList::writeOut(FILE* f, int tabs){
  if(analysisSoftware.size()<1){
    cerr << "AnalysisSoftwareList::AnalysisSoftware is required." << endl;
    exit(69);
  }
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f,"<AnalysisSoftwareList>\n");
  if (tabs>-1){
    for (j = 0; j < analysisSoftware.size(); j++) analysisSoftware[j].writeOut(f, tabs + 1);
  } else {
    for (j = 0; j < analysisSoftware.size(); j++) analysisSoftware[j].writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AnalysisSoftwareList>\n");
}
