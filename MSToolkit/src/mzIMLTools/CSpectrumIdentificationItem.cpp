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

#include "CSpectrumIdentificationItem.h"

using namespace std;

CSpectrumIdentificationItem::CSpectrumIdentificationItem(){
  calculatedMassToCharge=0;
  calculatedPI=0;
  chargeState=0;
  experimentalMassToCharge=0;
  passThreshold=true;
  rank=0;
}

//CSpectrumIdentificationItem::CSpectrumIdentificationItem(const CSpectrumIdentificationItem& s){
//  calculatedMassToCharge = s.calculatedMassToCharge;
//  calculatedPI = s.calculatedPI;
//  chargeState = s.chargeState;
//  experimentalMassToCharge = s.experimentalMassToCharge;
//  id = s.id;
//  massTableRef = s.massTableRef;
//  name = s.name;
//  passThreshold = s.passThreshold;
//  peptideRef = s.peptideRef;
//  rank = s.rank;
//  sampleRef = s.sampleRef;
//  fragmentation = s.fragmentation;
//
//  peptideEvidenceRef = new vector<sPeptideEvidenceRef>(*s.peptideEvidenceRef);
//  cvParam = new vector<sCvParam>(*s.cvParam);
//  userParam = new vector<sUserParam>(*s.userParam);
//}
//
//CSpectrumIdentificationItem::~CSpectrumIdentificationItem(){
//  delete peptideEvidenceRef;
//  delete cvParam;
//  delete userParam;
//}
//
//CSpectrumIdentificationItem& CSpectrumIdentificationItem::operator=(const CSpectrumIdentificationItem& s){
//  if (this != &s){
//    calculatedMassToCharge = s.calculatedMassToCharge;
//    calculatedPI = s.calculatedPI;
//    chargeState = s.chargeState;
//    experimentalMassToCharge = s.experimentalMassToCharge;
//    id = s.id;
//    massTableRef = s.massTableRef;
//    name = s.name;
//    passThreshold = s.passThreshold;
//    peptideRef = s.peptideRef;
//    rank = s.rank;
//    sampleRef = s.sampleRef;
//    fragmentation = s.fragmentation;
//
//    delete peptideEvidenceRef;
//    delete cvParam;
//    delete userParam;
//    peptideEvidenceRef = new vector<sPeptideEvidenceRef>(*s.peptideEvidenceRef);
//    cvParam = new vector<sCvParam>(*s.cvParam);
//    userParam = new vector<sUserParam>(*s.userParam);
//  }
//  return *this;
//}

void CSpectrumIdentificationItem::addCvParam(sCvParam& s){
  //if (cvParam->at(0).accession.compare("null") == 0) cvParam->clear();
  cvParam.push_back(s);
}

void CSpectrumIdentificationItem::addCvParam(string accession, string cvRef, string name, string unitAccession, string unitCvRef, string unitName, string value){
  sCvParam p;
  p.accession=accession;
  p.cvRef=cvRef;
  p.name=name;
  p.unitAccession=unitAccession;
  p.unitCvRef=unitCvRef;
  p.unitName=unitName;
  p.value=value;
  addCvParam(p);
}

void CSpectrumIdentificationItem::addPeptideEvidenceRef(sPeptideEvidenceRef& s){
  //if (peptideEvidenceRef->at(0).peptideEvidenceRef.size() == 0) peptideEvidenceRef->clear();
  peptideEvidenceRef.push_back(s);
}

void CSpectrumIdentificationItem::addPSMValue(string alg, string scoreID, int value,string prefix){
  char str[32];
  sprintf(str, "%d",value);
  addPSMValue(alg, scoreID, string(str),prefix);
}

void CSpectrumIdentificationItem::addPSMValue(string alg, string scoreID, double value, string prefix){
  char str[32];
  if ((int)value == value){
    sprintf(str, "%d", (int)value);
  } else if (value<0.0001 || value>1000.0){
    sprintf(str, "%.2E", value);
  } else {
    sprintf(str, "%.6lf", value);
  }
  addPSMValue(alg, scoreID, string(str),prefix);
}

void CSpectrumIdentificationItem::addPSMValue(string alg, string scoreID, string value, string prefix){
  sCvParam cv;
  cv.cvRef = "PSI-MS";
  if (alg.compare("Comet") == 0){
    if (scoreID.compare("deltacn") == 0) {
      cv.accession = "MS:1002253";  cv.name = "Comet:deltacn";
    } else if (scoreID.compare("deltacnstar") == 0) {
      cv.accession = "MS:1002254";  cv.name = "Comet:deltacnstar";
    } else if (scoreID.compare("expect") == 0) {
      cv.accession = "MS:1002257";  cv.name = "Comet:expectation value";
    } else if (scoreID.compare("sprank") == 0) {
      cv.accession = "MS:1002256";  cv.name = "Comet:sprank";
    } else if (scoreID.compare("spscore") == 0) {
      cv.accession = "MS:1002255";  cv.name = "Comet:spscore";
    } else if (scoreID.compare("xcorr") == 0) {
      cv.accession = "MS:1002252";  cv.name = "Comet:xcorr";
    }
  } else if (alg.compare("PeptideProphet")==0){
    if(scoreID.compare("Probability")==0){
      cv.accession = "MS:1002357";  cv.name = "PSM-level probability";
    }
  } else if (alg.compare("iProphet") == 0){
    if (scoreID.compare("Probability") == 0){
      cv.accession = "MS:1002362";  cv.name = "peptide sequence-level probability";
    }
  }
  if (cv.accession.compare("null") == 0){
    sUserParam u;
    u.name=alg;
    u.name += ":";
    if(prefix.size()>0){
      u.name+=prefix;
      u.name+=":";
    }
    u.name+=scoreID;
    u.value=value;
    //if(userParam->at(0).name.compare("null")==0) userParam->clear();
    userParam.push_back(u);
  } else {
    cv.value=value;
    //if (cvParam->at(0).name.compare("null") == 0) cvParam->clear();
    cvParam.push_back(cv);
  }
}

void CSpectrumIdentificationItem::writeOut(FILE* f, int tabs){
  if(id.empty()) {
    cerr << "SpectrumIdentificationItem::id required" << endl;
    exit(69);
  }
  if (peptideRef.empty()) {
    cerr << "SpectrumIdentificationItem::peptide_ref required" << endl;
    exit(69);
  }
  if (experimentalMassToCharge==0) {
    cerr << "SpectrumIdentificationItem::experimentalMassToCharge required" << endl;
    exit(69);
  }
  if (chargeState == 0) {
    cerr << "SpectrumIdentificationItem::chargeState required" << endl;
    exit(69);
  }

  int i;
  size_t j;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectrumIdentificationItem id=\"%s\" chargeState=\"%d\" experimentalMassToCharge=\"%.6lf\" rank=\"%d\"",&id[0],chargeState,experimentalMassToCharge,rank);
  if (passThreshold) fprintf(f, " passThreshold=\"true\"");
  else fprintf(f, " passThreshold=\"false\""); 
  if (calculatedMassToCharge>0) fprintf(f, " calculatedMassToCharge=\"%.6lf\"", calculatedMassToCharge);
  if (calculatedPI>0) fprintf(f, " calculatedPI=\"%.4f\"", calculatedPI);
  if (massTableRef.size()>0) fprintf(f, " massTable_ref=\"%s\"", &massTableRef[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, " peptide_ref=\"%s\"", peptideRef.c_str());
  if (sampleRef.size()>0) fprintf(f, " sample_ref=\"%s\"", &sampleRef[0]);
  fprintf(f, ">\n");

  int t = tabs;
  if (t>-1) t++;
  for (j = 0; j<peptideEvidenceRef.size(); j++) peptideEvidenceRef[j].writeOut(f, t);
  for(j=0;j<fragmentation.size();j++) fragmentation[j].writeOut(f,t);
  for (j = 0; j<cvParam.size(); j++) cvParam.at(j).writeOut(f, t);
  for (j = 0; j<userParam.size(); j++) userParam.at(j).writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectrumIdentificationItem>\n");

}

