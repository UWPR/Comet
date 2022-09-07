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

#include "CDBSequence.h"

using namespace std;

CDBSequence::CDBSequence(){
  length=0;
}

//CDBSequence::CDBSequence(const CDBSequence& d){
//  accession = d.accession;
//  id = d.id;
//  length = d.length;
//  name = d.name;
//  searchDatabaseRef = d.searchDatabaseRef;
//
//  seq = d.seq;
//
//  size_t i;
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//  for (i = 0; i<d.cvParam->size(); i++) cvParam->push_back(d.cvParam->at(i));
//  for (i = 0; i<d.userParam->size(); i++) userParam->push_back(d.userParam->at(i));
//}
//
//CDBSequence::~CDBSequence(){
//  delete cvParam;
//  delete userParam;
//}

//CDBSequence& CDBSequence::operator=(const CDBSequence& d){
//  if (this!=&d){
//    accession = d.accession;
//    id = d.id;
//    length = d.length;
//    name = d.name;
//    searchDatabaseRef = d.searchDatabaseRef;
//
//    seq = d.seq;
//
//    size_t i;
//    delete cvParam;
//    delete userParam;
//    cvParam = new vector<sCvParam>;
//    userParam = new vector<sUserParam>;
//    for (i = 0; i<d.cvParam->size(); i++) cvParam->push_back(d.cvParam->at(i));
//    for (i = 0; i<d.userParam->size(); i++) userParam->push_back(d.userParam->at(i));
//  }
//  return *this;
//}

void CDBSequence::writeOut(FILE* f, int tabs){
  if (id.empty()) {
    cerr << "DBSequence::id required" << endl;
    exit(69);
  }
  if(accession.empty()) {
    cerr << "DBSequence::accession required" << endl;
    exit(69);
  }
  if (searchDatabaseRef.empty()) {
    cerr << "DBSequence::searchDatabase_ref required" << endl;
    exit(69);
  }
  if(seq.size()>1){
    cerr << "DBSequence::seq only zero or one entry allowed" << endl;
    exit(69);
  }

  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<DBSequence id=\"%s\" accession=\"%s\" searchDatabase_ref=\"%s\"",&id[0],&accession[0],&searchDatabaseRef[0]);
  if (length>0) fprintf(f, " length=\"%d\"",length);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  int t=tabs;
  if(t>-1) t++;

  for (j = 0; j<seq.size(); j++) seq[j].writeOut(f, t);
  for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f,t);
  for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</DBSequence>\n");

}
