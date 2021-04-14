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

#include "CProteinAmbiguityGroup.h"

using namespace std;

CProteinAmbiguityGroup::CProteinAmbiguityGroup(){
  id="null";
  name.clear();

  CProteinDetectionHypothesis p;
  proteinDetectionHypothesis =  new vector<CProteinDetectionHypothesis>;
  proteinDetectionHypothesis->push_back(p);

  cvParam = new vector<sCvParam>;
  userParam = new vector<sUserParam>;
}

CProteinAmbiguityGroup::CProteinAmbiguityGroup(const CProteinAmbiguityGroup& c){
  id = c.id;
  name = c.name;

  size_t i;
  proteinDetectionHypothesis = new vector<CProteinDetectionHypothesis>;
  for(i=0;i<c.proteinDetectionHypothesis->size();i++) proteinDetectionHypothesis->push_back(c.proteinDetectionHypothesis->at(i));

  cvParam = new vector<sCvParam>;
  for (i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));

  userParam = new vector<sUserParam>;
  for (i = 0; i<c.userParam->size(); i++) userParam->push_back(c.userParam->at(i));
}

CProteinAmbiguityGroup::~CProteinAmbiguityGroup(){
  delete proteinDetectionHypothesis;
  delete cvParam;
  delete userParam;
}

CProteinAmbiguityGroup& CProteinAmbiguityGroup::operator=(const CProteinAmbiguityGroup& c){
  if (this != &c){
    id = c.id;
    name = c.name;

    delete proteinDetectionHypothesis;
    delete cvParam;
    delete userParam;

    size_t i;
    proteinDetectionHypothesis = new vector<CProteinDetectionHypothesis>;
    for (i = 0; i<c.proteinDetectionHypothesis->size(); i++) proteinDetectionHypothesis->push_back(c.proteinDetectionHypothesis->at(i));

    cvParam = new vector<sCvParam>;
    for (i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));

    userParam = new vector<sUserParam>;
    for (i = 0; i<c.userParam->size(); i++) userParam->push_back(c.userParam->at(i));
  }
  return *this;
}

void CProteinAmbiguityGroup::addParamValue(string alg, string scoreID, double value){
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

void CProteinAmbiguityGroup::addParamValue(string alg, string scoreID, string value){
  sCvParam cv;
  cv.cvRef = "PSI-MS";
  if (alg.compare("proteinprophet") == 0){
    if (scoreID.compare("null") == 0) {
      cv.accession = "MS:1002253";  cv.name = "placeholder";
    } 
  }

  //some scores are not algorithm specific
  if (cv.accession.compare("null") == 0){
    if (scoreID.compare("protein_group_passes_threshold") == 0) {
      cv.accession = "MS:1002415";  cv.name = "protein group passes threshold";
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

CProteinDetectionHypothesis* CProteinAmbiguityGroup::addProteinDetectionHypothesis(string baseRef, string dbSequenceRef, bool passThreshold){
  //get rid of any null placeholders
  if (proteinDetectionHypothesis->at(0).id.compare("null") == 0) proteinDetectionHypothesis->clear();

  //add new
  CProteinDetectionHypothesis p;
  char dbid[32];
  sprintf(dbid, "%s_%zu", baseRef.c_str(), proteinDetectionHypothesis->size());
  p.id = dbid;
  p.dbSequenceRef = dbSequenceRef;
  proteinDetectionHypothesis->push_back(p);

  return &proteinDetectionHypothesis->back();
}

void CProteinAmbiguityGroup::writeOut(FILE* f, int tabs){
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ProteinAmbiguityGroup id=\"%s\">\n",&id[0]);
  for (size_t j = 0; j < proteinDetectionHypothesis->size(); j++){
    if (tabs>-1) proteinDetectionHypothesis->at(j).writeOut(f, tabs + 1);
    else proteinDetectionHypothesis->at(j).writeOut(f);
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
  fprintf(f, "</ProteinAmbiguityGroup>\n");

}
