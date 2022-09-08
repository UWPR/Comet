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

#include "CMassTable.h"

using namespace std;

//CMassTable::CMassTable(){
//  id.clear();
//  name.clear();
//
//  msLevel=new vector<int>;
//  cvParam=new vector<sCvParam>;
//  residue=new vector<CResidue>;
//  userParam=new vector<sUserParam>;
//}
//
//CMassTable::CMassTable(const CMassTable& m){
//  id=m.id;
//  name=m.name;
//  msLevel=new vector<int>(*m.msLevel);
//  cvParam = new vector<sCvParam>(*m.cvParam);
//  residue = new vector<CResidue>(*m.residue);
//  userParam = new vector<sUserParam>(*m.userParam);
//}
//
//CMassTable::~CMassTable(){
//  delete msLevel;
//  delete cvParam;
//  delete residue;
//  delete userParam;
//}
//
//CMassTable& CMassTable::operator=(const CMassTable& c){
//  if(this!=&c){
//    id = c.id;
//    name = c.name;
//    delete msLevel;
//    delete cvParam;
//    delete residue;
//    delete userParam;
//    msLevel = new vector<int>(*c.msLevel);
//    cvParam = new vector<sCvParam>(*c.cvParam);
//    residue = new vector<CResidue>(*c.residue);
//    userParam = new vector<sUserParam>(*c.userParam);
//  }
//  return *this;
//}

void CMassTable::writeOut(FILE* f, int tabs){
  if (id.empty()){
    cerr << "MassTable::id is required." << endl;
    exit(69);
  }
  if (msLevel.empty()){
    cerr << "MassTable::msLevel is required." << endl;
    exit(69);
  }
  int i;
  size_t j;
 
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<MassTable id=\"%s\" msLevel=\"%s\"",id.c_str(),msLevel.c_str());
  if(name.size()>0) fprintf(f, " name=\"%s\"",name.c_str());
  fprintf(f,">\n");
 
  int t=tabs;
  if(t>-1) t++;
  for (j = 0; j<residue.size(); j++) residue[j].writeOut(f,t);
  for (j = 0; j<ambiguousResidue.size(); j++) ambiguousResidue[j].writeOut(f,t);
  for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f,t);
  for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</MassTable>\n");
}