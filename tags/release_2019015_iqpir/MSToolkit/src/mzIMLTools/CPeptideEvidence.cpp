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

#include "CPeptideEvidence.h"

using namespace std;

CPeptideEvidence::CPeptideEvidence(){
  dbSequenceRef = "null";
  end=0;
  id="null";
  isDecoy=false;
  name.clear();
  peptideRef="null";
  post='?';
  pre='?';
  start=0;
  translationTableRef.clear();
  cvParam= new vector<sCvParam>;
  userParam = new vector<sUserParam>;
}

CPeptideEvidence::CPeptideEvidence(const CPeptideEvidence& p){
  dbSequenceRef=p.dbSequenceRef;
  end=p.end;
  id=p.id;
  isDecoy=p.isDecoy;
  name=p.name;
  peptideRef=p.peptideRef;
  post=p.post;
  pre=p.pre;
  start=p.start;
  translationTableRef=p.translationTableRef;
  
  size_t i;
  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
  for (i = 0; i<p.cvParam->size(); i++) cvParam->push_back(p.cvParam->at(i));
  for (i = 0; i<p.userParam->size(); i++) userParam->push_back(p.userParam->at(i));

}

CPeptideEvidence::~CPeptideEvidence(){
  delete cvParam;
  delete userParam;
}

CPeptideEvidence& CPeptideEvidence::operator=(const CPeptideEvidence& p){
  if (this != &p){
    dbSequenceRef = p.dbSequenceRef;
    end = p.end;
    id = p.id;
    isDecoy = p.isDecoy;
    name = p.name;
    peptideRef = p.peptideRef;
    post = p.post;
    pre = p.pre;
    start = p.start;
    translationTableRef = p.translationTableRef;

    size_t i;
    delete cvParam;
    delete userParam;
    cvParam = new vector<sCvParam>;
    userParam = new vector<sUserParam>;
    for (i = 0; i<p.cvParam->size(); i++) cvParam->push_back(p.cvParam->at(i));
    for (i = 0; i<p.userParam->size(); i++) userParam->push_back(p.userParam->at(i));
  }
  return *this;
}

bool CPeptideEvidence::operator==(const CPeptideEvidence& p){
  if (this == &p) return true;
  if (dbSequenceRef.compare(p.dbSequenceRef) != 0) return false;
  if (peptideRef.compare(p.peptideRef) != 0) return false;
  return true;
}

void CPeptideEvidence::writeOut(FILE* f, int tabs){
  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<PeptideEvidence id=\"%s\" dBSequence_ref=\"%s\" peptide_ref=\"%s\"", &id[0], &dbSequenceRef[0],&peptideRef[0]);
  if (end>0) fprintf(f, " end=\"%d\"",end);
  if (isDecoy) fprintf(f, " isDecoy=\"true\"");
  else fprintf(f, " isDecoy=\"false\"");
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  if (post>0) fprintf(f, " post=\"%c\"", post);
  if (pre>0) fprintf(f, " pre=\"%c\"", pre);
  if (start>0) fprintf(f, " start=\"%d\"", start);
  if (translationTableRef.size()>0) fprintf(f, " translationTable_ref=\"%s\"", &translationTableRef[0]);
  fprintf(f, ">\n");

  if (tabs > -1){
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</PeptideEvidence>\n");
}
