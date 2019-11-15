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

#include "CCvList.h"

using namespace std;

CCvList::CCvList(){
  sCV c;
  cv = new vector<sCV>;
  c.id = "PSI-MS";
  c.fullName = "Proteomics Standards Initiative Mass Spectrometry Vocabularies";
  c.uri = "https://raw.githubusercontent.com/HUPO-PSI/psi-ms-CV/master/psi-ms.obo";
  c.version.clear();
  cv->push_back(c);
  c.id = "UNIMOD";
  c.fullName = "UNIMOD";
  c.uri = "http://www.unimod.org/obo/unimod.obo";
  c.version.clear();
  cv->push_back(c);
  c.id = "UO";
  c.fullName = "UNIT-ONTOLOGY";
  c.uri = "https://raw.githubusercontent.com/bio-ontology-research-group/unit-ontology/master/unit.obo";
  c.version.clear();
  cv->push_back(c);
}

CCvList::~CCvList(){
  delete cv;
}

sCV& CCvList::operator[](const size_t& index){
  return cv->at(index);
}

void CCvList::addCV(sCV& s){
  cv->push_back(s);
}

void CCvList::clear(){
  cv->clear();
}

void CCvList::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<cvList>\n");
  if (tabs>-1){
    for (j = 0; j < cv->size(); j++) cv->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j < cv->size(); j++) cv->at(j).writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</cvList>\n");
}