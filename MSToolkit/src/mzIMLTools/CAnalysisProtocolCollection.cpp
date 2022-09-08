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

#include "CAnalysisProtocolCollection.h"

using namespace std;

//CAnalysisProtocolCollection::CAnalysisProtocolCollection(){
//  CSpectrumIdentificationProtocol sip;
//  spectrumIdentificationProtocol=new vector<CSpectrumIdentificationProtocol>;
//  spectrumIdentificationProtocol->push_back(sip);
//}
//
//CAnalysisProtocolCollection::~CAnalysisProtocolCollection(){
//  delete spectrumIdentificationProtocol;
//}

CProteinDetectionProtocol* CAnalysisProtocolCollection::addProteinDetectionProtocol(string analysisSoftwareRef){
  CProteinDetectionProtocol pdp;
  pdp.analysisSoftwareRef=analysisSoftwareRef;

  char cID[32];
  sprintf(cID, "PDP%d", (int)proteinDetectionProtocol.size());
  pdp.id = cID;

  proteinDetectionProtocol.push_back(pdp);
  return &proteinDetectionProtocol.back();
}

CSpectrumIdentificationProtocol* CAnalysisProtocolCollection::addSpectrumIdentificationProtocol(string analysisSoftwareRef){
  CSpectrumIdentificationProtocol sip;
  sip.analysisSoftwareRef=analysisSoftwareRef;

  char cID[32];
  sprintf(cID, "SIP%d", (int)spectrumIdentificationProtocol.size());
  sip.id = cID;

  //TODO: add optional information

  spectrumIdentificationProtocol.push_back(sip);
  return &spectrumIdentificationProtocol.back();

}

string CAnalysisProtocolCollection::addSpectrumIdentificationProtocol(CSpectrumIdentificationProtocol& c){
  char cID[32];
  sprintf(cID, "SIP%d", (int)spectrumIdentificationProtocol.size());
  c.id = cID;

  spectrumIdentificationProtocol.push_back(c);
  return c.id;

}

void CAnalysisProtocolCollection::writeOut(FILE* f, int tabs){
  if (spectrumIdentificationProtocol.empty()) {
    cerr << "AnalysisProtocolCollection::spectrumIdentificationProtocol required" << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<AnalysisProtocolCollection>\n");

  int t=tabs;
  if(t>-1)t++;

  size_t j;
  for (j = 0; j<spectrumIdentificationProtocol.size(); j++) spectrumIdentificationProtocol[j].writeOut(f, t);
  for (j = 0; j<proteinDetectionProtocol.size(); j++) proteinDetectionProtocol[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AnalysisProtocolCollection>\n");

}
