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

#include "CPeptide.h"

using namespace std;

//CPeptide::CPeptide(){
//  id = "null";
//  name.clear();
//
//  modification = new vector<CModification>;
//  substitutionModification = new vector<sSubstitutionModification>;
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//}
//
//CPeptide::CPeptide(const CPeptide& p){
//  id = p.id;
//  name = p.name;
//  peptideSequence = p.peptideSequence;
//
//  size_t i;
//  modification = new vector<CModification>;
//  substitutionModification = new vector<sSubstitutionModification>;
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//  for (i = 0; i<p.modification->size(); i++) modification->push_back(p.modification->at(i));
//  for (i = 0; i<p.substitutionModification->size(); i++) substitutionModification->push_back(p.substitutionModification->at(i));
//  for (i = 0; i<p.cvParam->size(); i++) cvParam->push_back(p.cvParam->at(i));
//  for (i = 0; i<p.userParam->size(); i++) userParam->push_back(p.userParam->at(i));
//}
//
//CPeptide::~CPeptide(){
//  delete modification;
//  delete substitutionModification;
//  delete cvParam;
//  delete userParam;
//}
//
//CPeptide& CPeptide::operator=(const CPeptide& p){
//  if (this != &p){
//    id = p.id;
//    name = p.name;
//    peptideSequence = p.peptideSequence;
//
//    size_t i;
//    delete modification;
//    delete substitutionModification;
//    delete cvParam;
//    delete userParam;
//    modification = new vector<CModification>;
//    substitutionModification = new vector<sSubstitutionModification>;
//    cvParam = new vector<sCvParam>;
//    userParam = new vector<sUserParam>;
//    for (i = 0; i<p.modification->size(); i++) modification->push_back(p.modification->at(i));
//    for (i = 0; i<p.substitutionModification->size(); i++) substitutionModification->push_back(p.substitutionModification->at(i));
//    for (i = 0; i<p.cvParam->size(); i++) cvParam->push_back(p.cvParam->at(i));
//    for (i = 0; i<p.userParam->size(); i++) userParam->push_back(p.userParam->at(i));
//  }
//  return *this;
//}

//this is a really slow comparison; it's the nested loops...
bool CPeptide::operator==(const CPeptide& p){
  if (this==&p) return true;
  if (peptideSequence.text.compare(p.peptideSequence.text)!=0) return false;

  size_t i,j;
  if (modification.size()!=p.modification.size()) return false;
  for (i = 0; i < modification.size(); i++){
    for (j = 0; j < p.modification.size(); j++){
      if (modification[i] == p.modification[j]) break;
    }
    if (j == p.modification.size()) return false;
  }

  if (substitutionModification.size() != p.substitutionModification.size()) return false;
  for (i = 0; i < substitutionModification.size(); i++){
    for (j = 0; j < p.substitutionModification.size(); j++){
      if (substitutionModification[i] == p.substitutionModification[j]) break;
    }
    if (j == p.substitutionModification.size()) return false;
  }

  return true;
}

bool CPeptide::compareModsSoft(CPeptide& p){
  size_t i, j;
  if (modification.size() != p.modification.size()) return false;
  for (i = 0; i < modification.size(); i++){
    for (j = 0; j < p.modification.size(); j++){
      //cout << (int)(modification->at(i).monoisotopicMassDelta + 0.5) << " " << modification->at(i).location << " vs " << (int)p.modification->at(j).monoisotopicMassDelta << " " << p.modification->at(j).location << endl;
      //cout << modification->at(i).monoisotopicMassDelta << " " << modification->at(i).location << " vs " << p.modification->at(j).monoisotopicMassDelta << " " << p.modification->at(j).location << endl;
      if (modification[i].location==p.modification[j].location && fabs(modification[i].monoisotopicMassDelta - p.modification[j].monoisotopicMassDelta)<0.5){
      //if ((int)(modification->at(i).monoisotopicMassDelta+0.5) == (int)p.modification->at(j).monoisotopicMassDelta && modification->at(i).location==p.modification->at(j).location) {
        break;
      }
    }
    if (j == p.modification.size()) return false;
  }
  return true;
}

void CPeptide::writeOut(FILE* f, int tabs){
  if (id.empty()){
    cerr << "Peptide::id is required." << endl;
    exit(69);
  }

  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Peptide id=\"%s\"", &id[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  int t=tabs;
  if(t>-1) t++;

  peptideSequence.writeOut(f,t);
  for (j = 0; j<modification.size(); j++) modification[j].writeOut(f, t);
  for (j = 0; j<substitutionModification.size(); j++) substitutionModification[j].writeOut(f, t);
  for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, t);
  for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Peptide>\n");
}
