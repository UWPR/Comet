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

#include "CProteinDetectionHypothesis.h"

using namespace std;

CProteinDetectionHypothesis::CProteinDetectionHypothesis(){
  dbSequenceRef = "null";
  id = "null";
  name.clear();
  passThreshold = true;

  CPeptideHypothesis ph;
  peptideHypothesis = new vector<CPeptideHypothesis>;
  peptideHypothesis->push_back(ph);
  
  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
}

CProteinDetectionHypothesis::CProteinDetectionHypothesis(const CProteinDetectionHypothesis& c){
  dbSequenceRef=c.dbSequenceRef;
  id=c.id;
  name=c.name;
  passThreshold=c.passThreshold;

  size_t i;
  peptideHypothesis = new vector<CPeptideHypothesis>;
  for (i = 0; i<c.peptideHypothesis->size(); i++) peptideHypothesis->push_back(c.peptideHypothesis->at(i));
  cvParam = new vector<sCvParam>;
  for (i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
  userParam = new vector<sUserParam>;
  for (i = 0; i<c.userParam->size(); i++) userParam->push_back(c.userParam->at(i));

}

CProteinDetectionHypothesis::~CProteinDetectionHypothesis(){
  delete peptideHypothesis;
  delete cvParam;
  delete userParam;
}

//Operators
CProteinDetectionHypothesis& CProteinDetectionHypothesis::operator=(const CProteinDetectionHypothesis& c){
  if (this != &c){
    dbSequenceRef = c.dbSequenceRef;
    id = c.id;
    name = c.name;
    passThreshold = c.passThreshold;

    delete peptideHypothesis;
    delete cvParam;
    delete userParam;

    size_t i;
    peptideHypothesis = new vector<CPeptideHypothesis>;
    for (i = 0; i<c.peptideHypothesis->size(); i++) peptideHypothesis->push_back(c.peptideHypothesis->at(i));
    cvParam = new vector<sCvParam>;
    for (i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
    userParam = new vector<sUserParam>;
    for (i = 0; i<c.userParam->size(); i++) userParam->push_back(c.userParam->at(i));
  }
  return *this;
}

void CProteinDetectionHypothesis::addParamValue(string alg, string scoreID, double value){
  char str[32];
  if ((int)value == value){
    sprintf(str, "%d", (int)value);
  } else if (value<0.0001 || value>1000.0){
    sprintf(str, "%.2E", value);
  } else {
    sprintf(str, "%.6lf", value);
  }
  addParamValue(alg, scoreID, string(str));
}

void CProteinDetectionHypothesis::addParamValue(string alg, string scoreID, string value){
  sCvParam cv;
  cv.cvRef = "PSI-MS";
  if (alg.compare("proteinprophet") == 0){
    if (scoreID.compare("probability") == 0) {
      cv.accession = "MS:1002376";  cv.name = "protein group-level probability";
    } else if (scoreID.compare("null") == 0) {
      cv.accession = "MS:1002253";  cv.name = "placeholder";
    }
  }
  //some scores are not algorithm specific
  if (cv.accession.compare("null") == 0){
    if (scoreID.compare("coverage") == 0) {
      cv.accession = "MS:1001093";  cv.name = "sequence coverage";
    } else if (scoreID.compare("group_representative") == 0){
      cv.accession = "MS:1002403";  cv.name = "group representative";
    } else if (scoreID.compare("leading_protein") == 0){
      cv.accession = "MS:1002401";  cv.name = "leading protein";
    } else if (scoreID.compare("non_leading_protein") == 0){
      cv.accession = "MS:1002402";  cv.name = "non-leading protein";
    }
  }

  if (cv.accession.compare("null") == 0){
    sUserParam u;
    u.name = alg;
    u.name += ":";
    u.name += scoreID;
    u.value = value;
    userParam->push_back(u);
  } else {
    cv.value = value;
    cvParam->push_back(cv);
  }
}

CPeptideHypothesis* CProteinDetectionHypothesis::addPeptideHypothesis(string& pe){
  if (peptideHypothesis->at(0).peptideEvidenceRef.compare("null") == 0) peptideHypothesis->clear();

  CPeptideHypothesis ph;
  ph.peptideEvidenceRef=pe;
  peptideHypothesis->push_back(ph);
  return &peptideHypothesis->back();
}

void CProteinDetectionHypothesis::writeOut(FILE* f, int tabs){
  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ProteinDetectionHypothesis id=\"%s\" dBSequence_ref=\"%s\"", &id[0], &dbSequenceRef[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  if (passThreshold) fprintf(f, " passThreshold=\"true\">\n");
  else fprintf(f, " passThreshold=\"false\">\n");

  if (tabs > -1){
    for (j = 0; j<peptideHypothesis->size(); j++) peptideHypothesis->at(j).writeOut(f, tabs + 1);
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<peptideHypothesis->size(); j++) peptideHypothesis->at(j).writeOut(f);
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f);
    for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ProteinDetectionHypothesis>\n");
}
