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

#include "CProteinDetection.h"

using namespace std;

CProteinDetection::CProteinDetection(){
  activityDate.clear();
  id = "null";
  name.clear();
  proteinDetectionListRef = "null";
  proteinDetectionProtocolRef = "null";

  sInputSpectrumIdentifications isi;
  inputSpectrumidentifications = new vector<sInputSpectrumIdentifications>;
  inputSpectrumidentifications->push_back(isi);
}

CProteinDetection::CProteinDetection(const CProteinDetection& c){
  activityDate=c.activityDate;
  id=c.id;
  name=c.name;
  proteinDetectionListRef = c.proteinDetectionListRef;
  proteinDetectionProtocolRef = c.proteinDetectionProtocolRef;

  inputSpectrumidentifications = new vector<sInputSpectrumIdentifications>;
  for(size_t i=0;i<c.inputSpectrumidentifications->size();i++) inputSpectrumidentifications->push_back(c.inputSpectrumidentifications->at(i));
}

CProteinDetection::~CProteinDetection(){
  delete inputSpectrumidentifications;
}

CProteinDetection& CProteinDetection::operator=(const CProteinDetection& c){
  if (this != &c){
    activityDate = c.activityDate;
    id = c.id;
    name = c.name;
    proteinDetectionListRef = c.proteinDetectionListRef;
    proteinDetectionProtocolRef = c.proteinDetectionProtocolRef;

    delete inputSpectrumidentifications;
    inputSpectrumidentifications = new vector<sInputSpectrumIdentifications>;
    for (size_t i = 0; i<c.inputSpectrumidentifications->size(); i++) inputSpectrumidentifications->push_back(c.inputSpectrumidentifications->at(i));
  }
  return *this;
}

void CProteinDetection::addInputSpectrumIdentification(string s){
  sInputSpectrumIdentifications si;
  si.spectrumIdentificationListRef=s;
  inputSpectrumidentifications->push_back(si);
}

void CProteinDetection::writeOut(FILE* f, int tabs){

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ProteinDetection id=\"%s\" proteinDetectionList_ref=\"%s\" proteinDetectionProtocol_ref=\"%s\"", &id[0], &proteinDetectionListRef[0], &proteinDetectionProtocolRef[0]);
  if (activityDate.size()>0) fprintf(f, " activityDate=\"%s\"", &activityDate[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  size_t j;
  if (tabs>-1) {
    for (j = 0; j<inputSpectrumidentifications->size(); j++) inputSpectrumidentifications->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<inputSpectrumidentifications->size(); j++) inputSpectrumidentifications->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ProteinDetection>\n");

}
