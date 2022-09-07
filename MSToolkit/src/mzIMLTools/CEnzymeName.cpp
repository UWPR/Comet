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

#include "CEnzymeName.h"

using namespace std;

//CEnzymeName::CEnzymeName(){
//  cvParam=new vector<sCvParam>;
//  sCvParam c;
//  cvParam->push_back(c);
//
//  sUserParam u;
//  userParam=new vector<sUserParam>;
//  userParam->push_back(u);
//}
//
//CEnzymeName::CEnzymeName(const CEnzymeName& c){
//  cvParam=new vector<sCvParam>(*c.cvParam);
//  userParam=new vector<sUserParam>(*c.userParam);
//}
//
//CEnzymeName::~CEnzymeName(){
//  delete cvParam;
//  delete userParam;
//}
//
////Operators
//CEnzymeName& CEnzymeName::operator=(const CEnzymeName& c){
//  if(this!=&c){
//    delete cvParam;
//    delete userParam;
//    cvParam = new vector<sCvParam>(*c.cvParam);
//    userParam = new vector<sUserParam>(*c.userParam);
//  }
//  return *this;
//}

//Functions
void CEnzymeName::writeOut(FILE* f, int tabs){
  int i,j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<EnzymeName>\n");

  if(tabs>-1){
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs + 1);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, tabs + 1);
  }else {
    for(j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</EnzymeName>\n");
}
