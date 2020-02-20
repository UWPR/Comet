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

#include "CAnalysisData.h"

using namespace std;

CAnalysisData::CAnalysisData(){
  CSpectrumIdentificationList sil;
  spectrumIdentificationList = new vector<CSpectrumIdentificationList>;
  spectrumIdentificationList->push_back(sil);

  peptideEvidenceTable=NULL;
}

CAnalysisData::~CAnalysisData(){
  delete spectrumIdentificationList;
  if (peptideEvidenceTable!=NULL) delete peptideEvidenceTable;
}

string CAnalysisData::addSpectrumIdentificationList(){
  //remove any placeholder list
  if (spectrumIdentificationList->at(0).id.compare("null") == 0) spectrumIdentificationList->clear();

  CSpectrumIdentificationList sil;
  char cID[32];
  sprintf(cID, "SIL%zu", spectrumIdentificationList->size());
  sil.id = cID;

  //TODO: add optional information

  spectrumIdentificationList->push_back(sil);
  return sil.id;
}

string CAnalysisData::addSpectrumIdentificationList(CSpectrumIdentificationList& c){
  //remove any placeholder list
  if (spectrumIdentificationList->at(0).id.compare("null") == 0) spectrumIdentificationList->clear();

  if (c.id.compare("null") == 0){
    char cID[32];
    sprintf(cID, "SIL%zu", spectrumIdentificationList->size());
    c.id = cID;
  }

  spectrumIdentificationList->push_back(c);
  return c.id;
}

void CAnalysisData::buildPeptideEvidenceTable(){
  if (peptideEvidenceTable!=NULL) delete peptideEvidenceTable;
  peptideEvidenceTable = new vector<sXRefSIIPE>;

  sXRefSIIPE s;
  size_t i,j,k;

  CSpectrumIdentificationList* sil;
  CSpectrumIdentificationItem* sii;

  //build table
  for (i = 0; i<spectrumIdentificationList->size(); i++){
    sil = &spectrumIdentificationList->at(i);
    for (j = 0; j < sil->spectrumIdentificationResult->size(); j++){
      sii = &sil->spectrumIdentificationResult->at(j).spectrumIdentificationItem->at(0);
      s.spectrumIdentificationItemRef = sii->id;
      s.charge=sii->chargeState;
      for (k = 0; k < sii->peptideEvidenceRef->size(); k++){
        s.peptideEvidenceRef = sii->peptideEvidenceRef->at(k).peptideEvidenceRef;
        peptideEvidenceTable->push_back(s);
      }
    }
  }

  //sort table by peptideEvidenceRef
  sort(peptideEvidenceTable->begin(), peptideEvidenceTable->end(), comparePeptideEvidenceRef);

}

void CAnalysisData::getSpectrumIdentificationItems(string pe, int charge, vector<string>& vSII){
  vSII.clear();

  size_t sz = peptideEvidenceTable->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  i = peptideEvidenceTable->at(mid).peptideEvidenceRef.compare(pe);
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
    i = peptideEvidenceTable->at(mid).peptideEvidenceRef.compare(pe);
  }

  //check all evidences that have this peptide for the requested protein
  if (peptideEvidenceTable->at(mid).charge == charge) vSII.push_back(peptideEvidenceTable->at(mid).spectrumIdentificationItemRef);
  i = (int)mid - 1;
  while (i > -1 && peptideEvidenceTable->at(i).peptideEvidenceRef.compare(pe) == 0){
    if (peptideEvidenceTable->at(i).charge == charge) vSII.push_back(peptideEvidenceTable->at(i).spectrumIdentificationItemRef);
    i--;
  }
  i = (int)mid + 1;
  while (i < (int)peptideEvidenceTable->size() && peptideEvidenceTable->at(i).peptideEvidenceRef.compare(pe) == 0){
    if (peptideEvidenceTable->at(i).charge == charge) vSII.push_back(peptideEvidenceTable->at(i).spectrumIdentificationItemRef);
    i++;
  }

}

void CAnalysisData::writeOut(FILE* f, int tabs){
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<AnalysisData>\n");
  for (size_t j = 0; j < spectrumIdentificationList->size(); j++){
    if (tabs>-1) spectrumIdentificationList->at(j).writeOut(f,tabs+1);
    else spectrumIdentificationList->at(j).writeOut(f);
  }
  if (proteinDetectionList.id.compare("null") != 0){
    if (tabs>-1) proteinDetectionList.writeOut(f,tabs+1);
    else proteinDetectionList.writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</AnalysisData>\n");
}

bool CAnalysisData::comparePeptideEvidenceRef(const sXRefSIIPE& a, const sXRefSIIPE& b){
  return (a.peptideEvidenceRef.compare(b.peptideEvidenceRef)<0);
}
