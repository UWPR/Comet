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

#include "CAnalysisData.h"

using namespace std;

//CAnalysisData::CAnalysisData(){
//  CSpectrumIdentificationList sil;
//  spectrumIdentificationList = new vector<CSpectrumIdentificationList>;
//  spectrumIdentificationList->push_back(sil);
//
//  peptideEvidenceTable=NULL;
//}
//
//CAnalysisData::~CAnalysisData(){
//  delete spectrumIdentificationList;
//  if (peptideEvidenceTable!=NULL) delete peptideEvidenceTable;
//}

string CAnalysisData::addProteinDetectionList(){

  CProteinDetectionList pdl;
  char cID[32];
  sprintf(cID, "PDL%d", (int)proteinDetectionList.size());
  pdl.id = cID;

  //TODO: add optional information

  proteinDetectionList.push_back(pdl);
  return pdl.id;
}

string CAnalysisData::addSpectrumIdentificationList(){

  CSpectrumIdentificationList sil;
  char cID[32];
  sprintf(cID, "SIL%d", (int)spectrumIdentificationList.size());
  sil.id = cID;

  //TODO: add optional information

  spectrumIdentificationList.push_back(sil);
  return sil.id;
}

string CAnalysisData::addSpectrumIdentificationList(CSpectrumIdentificationList& c){

  if (c.id.compare("null") == 0){
    char cID[32];
    sprintf(cID, "SIL%d", (int)spectrumIdentificationList.size());
    c.id = cID;
  }

  spectrumIdentificationList.push_back(c);
  return c.id;
}

void CAnalysisData::buildPeptideEvidenceTable(){
  peptideEvidenceTable.clear();

  sXRefSIIPE s;
  size_t i,j,k,n;

  CSpectrumIdentificationList* sil;
  CSpectrumIdentificationItem* sii;

  //build table
  for (i = 0; i<spectrumIdentificationList.size(); i++){
    sil = &spectrumIdentificationList[i];
    for (j = 0; j < sil->spectrumIdentificationResult.size(); j++){
      for(n=0;n<sil->spectrumIdentificationResult[j].spectrumIdentificationItem.size();n++){
        sii = &sil->spectrumIdentificationResult[j].spectrumIdentificationItem[n];
        s.spectrumIdentificationItemRef = sii->id;
        s.charge=sii->chargeState;
        for (k = 0; k < sii->peptideEvidenceRef.size(); k++){
          s.peptideEvidenceRef = sii->peptideEvidenceRef[k].peptideEvidenceRef;
          peptideEvidenceTable.push_back(s);
        }
      }
    }
  }

  //sort table by peptideEvidenceRef
  sort(peptideEvidenceTable.begin(), peptideEvidenceTable.end(), comparePeptideEvidenceRef);

}

void CAnalysisData::getSpectrumIdentificationItems(string pe, int charge, vector<string>& vSII){
  vSII.clear();

  size_t sz = peptideEvidenceTable.size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  i = peptideEvidenceTable[mid].peptideEvidenceRef.compare(pe);
  while (i != 0){
    if (lower >= upper) return;
    if (i>0){
      if (mid == 0) return;
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return;
    i = peptideEvidenceTable[mid].peptideEvidenceRef.compare(pe);
  }

  //check all evidences that have this peptide for the requested protein
  if (peptideEvidenceTable[mid].charge == charge) vSII.push_back(peptideEvidenceTable[mid].spectrumIdentificationItemRef);
  i = (int)mid - 1;
  while (i > -1 && peptideEvidenceTable[i].peptideEvidenceRef.compare(pe) == 0){
    if (peptideEvidenceTable[i].charge == charge) vSII.push_back(peptideEvidenceTable[i].spectrumIdentificationItemRef);
    i--;
  }
  i = (int)mid + 1;
  while (i < (int)peptideEvidenceTable.size() && peptideEvidenceTable[i].peptideEvidenceRef.compare(pe) == 0){
    if (peptideEvidenceTable[i].charge == charge) vSII.push_back(peptideEvidenceTable[i].spectrumIdentificationItemRef);
    i++;
  }

}

void CAnalysisData::writeOut(FILE* f, int tabs){
  if(spectrumIdentificationList.empty()){
    cerr << "AnalysisData::SpectrumIdentificationList is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<AnalysisData>\n");

  int t=tabs;
  if(t>-1)t++;

  size_t j;
  for (j = 0; j < spectrumIdentificationList.size(); j++) spectrumIdentificationList[j].writeOut(f,t);
  for (j = 0; j < proteinDetectionList.size(); j++) proteinDetectionList[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AnalysisData>\n");
}

bool CAnalysisData::comparePeptideEvidenceRef(const sXRefSIIPE& a, const sXRefSIIPE& b){
  return (a.peptideEvidenceRef.compare(b.peptideEvidenceRef)<0);
}
