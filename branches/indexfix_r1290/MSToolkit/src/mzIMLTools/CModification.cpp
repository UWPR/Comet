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

#include "CModification.h"

using namespace std;

CModification::CModification(){
  avgMassDelta=0;
  location=-1;
  monoisotopicMassDelta=0;
  residues.clear();
  
  sCvParam cv;
  cvParam = new vector<sCvParam>;
  cvParam->push_back(cv);
}

CModification::CModification(const CModification& m){
  avgMassDelta = m.avgMassDelta;
  location = m.location;
  monoisotopicMassDelta = m.monoisotopicMassDelta;
  residues = m.residues;

  size_t i;
  cvParam = new vector<sCvParam>;
  for (i = 0; i<m.cvParam->size(); i++) cvParam->push_back(m.cvParam->at(i));
}

CModification::~CModification(){
  delete cvParam;
}

CModification& CModification::operator=(const CModification& m){
  if (this != &m){
    avgMassDelta = m.avgMassDelta;
    location = m.location;
    monoisotopicMassDelta = m.monoisotopicMassDelta;
    residues = m.residues;

    size_t i;
    delete cvParam;
    cvParam = new vector<sCvParam>;
    for (i = 0; i<m.cvParam->size(); i++) cvParam->push_back(m.cvParam->at(i));
  }
  return *this;
}

//this is a really slow comparison; it's the nested loops...
bool CModification::operator==(const CModification& m){
  if (this == &m) return true;
  if (avgMassDelta!=m.avgMassDelta) return false;
  if (location!=m.location) return false;
  if (monoisotopicMassDelta!=m.monoisotopicMassDelta) return false;
  if (residues.compare(m.residues) != 0) return false;

  size_t i, j;
  if (cvParam->size() != m.cvParam->size()) return false;
  for (i = 0; i < cvParam->size(); i++){
    for (j = 0; j < m.cvParam->size(); j++){
      if (cvParam->at(i) == m.cvParam->at(j)) break;
    }
    if (j == m.cvParam->size()) return false;
  }

  return true;
}

void CModification::clear(){
  delete cvParam;

  avgMassDelta = 0;
  location = -1;
  monoisotopicMassDelta = 0;
  residues.clear();

  sCvParam cv;
  cvParam = new vector<sCvParam>;
  cvParam->push_back(cv);
}

void CModification::writeOut(FILE* f, int tabs){
  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Modification");
  if (avgMassDelta>0) fprintf(f, " avgMassDelta=\"%.6lf\"",avgMassDelta);
  if (location>-1) fprintf(f, " location=\"%d\"", location);
  if (monoisotopicMassDelta>0) fprintf(f, " monoisotopicMassDelta=\"%.6lf\"", monoisotopicMassDelta);
  if (residues.size()>0) fprintf(f, " residues=\"%s\"", &residues[0]);
  fprintf(f, ">\n");

  if (tabs > -1){
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Modification>\n");
}
