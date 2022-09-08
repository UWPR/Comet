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

#include "CModification.h"

using namespace std;

CModification::CModification(){
  avgMassDelta=0;
  location=-1;
  monoisotopicMassDelta=0;
}
//
//CModification::CModification(const CModification& m){
//  avgMassDelta = m.avgMassDelta;
//  location = m.location;
//  monoisotopicMassDelta = m.monoisotopicMassDelta;
//  residues = m.residues;
//
//  size_t i;
//  cvParam = new vector<sCvParam>;
//  for (i = 0; i<m.cvParam->size(); i++) cvParam->push_back(m.cvParam->at(i));
//}
//
//CModification::~CModification(){
//  delete cvParam;
//}
//
//CModification& CModification::operator=(const CModification& m){
//  if (this != &m){
//    avgMassDelta = m.avgMassDelta;
//    location = m.location;
//    monoisotopicMassDelta = m.monoisotopicMassDelta;
//    residues = m.residues;
//
//    size_t i;
//    delete cvParam;
//    cvParam = new vector<sCvParam>;
//    for (i = 0; i<m.cvParam->size(); i++) cvParam->push_back(m.cvParam->at(i));
//  }
//  return *this;
//}

//this is a really slow comparison; it's the nested loops...
bool CModification::operator==(const CModification& m){
  if (this == &m) return true;
  if (avgMassDelta!=m.avgMassDelta) return false;
  if (location!=m.location) return false;
  if (fabs(monoisotopicMassDelta-m.monoisotopicMassDelta)>0.001) return false;
  if (residues.compare(m.residues) != 0) return false;

  size_t i, j;
  //if (cvParam.size() != m.cvParam.size()) return false;
  for (i = 0; i < cvParam.size(); i++){
    if(cvParam[i].accession.compare("MS:1002504")==0) continue; //skip modification index
    for (j = 0; j < m.cvParam.size(); j++){
      if (cvParam[i] == m.cvParam[j]) break;
    }
    if (j == m.cvParam.size()) return false;
  }

  return true;
}

void CModification::clear(){
  avgMassDelta = 0;
  location = -1;
  monoisotopicMassDelta = 0;
  residues.clear();
  cvParam.clear();
}

void CModification::writeOut(FILE* f, int tabs){
  int i;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Modification");
  if (avgMassDelta>0) fprintf(f, " avgMassDelta=\"%.6lf\"",avgMassDelta);
  if (location>-1) fprintf(f, " location=\"%d\"", location);
  if (monoisotopicMassDelta>0) fprintf(f, " monoisotopicMassDelta=\"%.6lf\"", monoisotopicMassDelta);
  if (!residues.empty()) fprintf(f, " residues=\"%s\"", residues.c_str());
  fprintf(f, ">\n");

  int t = tabs;
  if (t>-1)t++;

  for(size_t j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Modification>\n");
}
