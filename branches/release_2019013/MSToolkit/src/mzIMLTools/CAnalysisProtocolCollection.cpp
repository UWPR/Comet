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

#include "CAnalysisProtocolCollection.h"

using namespace std;

CAnalysisProtocolCollection::CAnalysisProtocolCollection(){
  CSpectrumIdentificationProtocol sip;
  spectrumIdentificationProtocol=new vector<CSpectrumIdentificationProtocol>;
  spectrumIdentificationProtocol->push_back(sip);
}

CAnalysisProtocolCollection::~CAnalysisProtocolCollection(){
  delete spectrumIdentificationProtocol;
}

string CAnalysisProtocolCollection::addProteinDetectionProtocol(string analysisSoftwareRef){
  proteinDetectionProtocol.id = "PDP0";
  proteinDetectionProtocol.analysisSoftwareRef=analysisSoftwareRef;
  return proteinDetectionProtocol.id;
}

string CAnalysisProtocolCollection::addSpectrumIdentificationProtocol(string analysisSoftwareRef){
  //remove any placeholder
  if (spectrumIdentificationProtocol->at(0).id.compare("null") == 0) spectrumIdentificationProtocol->clear();

  CSpectrumIdentificationProtocol sip;
  sip.analysisSoftwareRef=analysisSoftwareRef;

  char cID[32];
  sprintf(cID, "SIP%zu", spectrumIdentificationProtocol->size());
  sip.id = cID;

  //TODO: add optional information

  spectrumIdentificationProtocol->push_back(sip);
  return sip.id;

}

string CAnalysisProtocolCollection::addSpectrumIdentificationProtocol(CSpectrumIdentificationProtocol& c){
  //remove any placeholder
  if (spectrumIdentificationProtocol->at(0).id.compare("null") == 0) spectrumIdentificationProtocol->clear();

  if (c.id.compare("null") == 0){
    char cID[32];
    sprintf(cID, "SIP%zu", spectrumIdentificationProtocol->size());
    c.id = cID;
  }
  spectrumIdentificationProtocol->push_back(c);
  return c.id;

}

void CAnalysisProtocolCollection::writeOut(FILE* f, int tabs){

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<AnalysisProtocolCollection>\n");

  size_t j;
  if (tabs>-1) {
    for (j = 0; j<spectrumIdentificationProtocol->size(); j++) spectrumIdentificationProtocol->at(j).writeOut(f, tabs + 1);
    proteinDetectionProtocol.writeOut(f,tabs+1);
  } else {
    for (j = 0; j<spectrumIdentificationProtocol->size(); j++) spectrumIdentificationProtocol->at(j).writeOut(f);
    proteinDetectionProtocol.writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AnalysisProtocolCollection>\n");

}
