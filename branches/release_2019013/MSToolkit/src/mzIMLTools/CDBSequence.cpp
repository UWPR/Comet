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

#include "CDBSequence.h"

using namespace std;

CDBSequence::CDBSequence(){
  accession = "null";
  id = "null";
  length=0;
  name.clear();
  searchDatabaseRef = "null";

  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
}

CDBSequence::CDBSequence(const CDBSequence& d){
  accession = d.accession;
  id = d.id;
  length = d.length;
  name = d.name;
  searchDatabaseRef = d.searchDatabaseRef;

  seq = d.seq;

  size_t i;
  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
  for (i = 0; i<d.cvParam->size(); i++) cvParam->push_back(d.cvParam->at(i));
  for (i = 0; i<d.userParam->size(); i++) userParam->push_back(d.userParam->at(i));
}

CDBSequence::~CDBSequence(){
  delete cvParam;
  delete userParam;
}

CDBSequence& CDBSequence::operator=(const CDBSequence& d){
  if (this!=&d){
    accession = d.accession;
    id = d.id;
    length = d.length;
    name = d.name;
    searchDatabaseRef = d.searchDatabaseRef;

    seq = d.seq;

    size_t i;
    delete cvParam;
    delete userParam;
    cvParam = new vector<sCvParam>;
    userParam = new vector<sUserParam>;
    for (i = 0; i<d.cvParam->size(); i++) cvParam->push_back(d.cvParam->at(i));
    for (i = 0; i<d.userParam->size(); i++) userParam->push_back(d.userParam->at(i));
  }
  return *this;
}

void CDBSequence::writeOut(FILE* f, int tabs){
  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<DBSequence id=\"%s\" accession=\"%s\" searchDatabase_ref=\"%s\"",&id[0],&accession[0],&searchDatabaseRef[0]);
  if (length>0) fprintf(f, " length=\"%d\"",length);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  if (tabs > -1){
    seq.writeOut(f,tabs+1);
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f, tabs + 1);
  } else {
    seq.writeOut(f);
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</DBSequence>\n");

}
