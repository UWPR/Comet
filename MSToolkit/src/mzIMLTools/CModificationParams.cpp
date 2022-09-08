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

#include "CModificationParams.h"

using namespace std;

//CModificationParams::CModificationParams(){
//  CSearchModification sm;
//  searchModification = new vector<CSearchModification>;
//  searchModification->push_back(sm);
//}
//
//CModificationParams::CModificationParams(const CModificationParams& c){
//  searchModification = new vector<CSearchModification>;
//  for (size_t i = 0; i<c.searchModification->size(); i++) searchModification->push_back(c.searchModification->at(i));
//}
//
//CModificationParams::~CModificationParams(){
//  delete searchModification;
//}
//
//CModificationParams& CModificationParams::operator=(const CModificationParams& c){
//  if (this != &c){
//    delete searchModification;
//    searchModification = new vector<CSearchModification>;
//    for (size_t i = 0; i<c.searchModification->size(); i++) searchModification->push_back(c.searchModification->at(i));
//  }
//  return *this;
//}

bool CModificationParams::operator==(const CModificationParams& c){
  if (this==&c) return true;
  if (searchModification.size() != c.searchModification.size()) return false;
  size_t i,j;
  for (i = 0; i < searchModification.size(); i++){
    for (j = 0; j < c.searchModification.size(); j++) {
      if (searchModification[i] == c.searchModification[j]) break;
    }
    if (j == c.searchModification.size()) return false;
  }
  return true;
}

bool CModificationParams::operator!=(const CModificationParams& c){
  return !operator==(c);
}

void CModificationParams::addSearchModification(bool fixed, double mass, string residues, bool protTerm){

  CSearchModification sm;
  sm.fixedMod=fixed;
  sm.massDelta=mass;
  if (protTerm) {
    sm.residues = ".";
  } else {
    if (residues[0] == 'n' || residues[0] == 'c') sm.residues = ".";
    else sm.residues=residues;
  }

  //special case for pyro-carbamidomethyl cysteine as represented in pep.xml
  if(residues[0]=='C' && fabs(-17.0265-mass)<0.1){
    //see if we've already added carbamidomethyl-C as a fixed mod
    for(size_t i=0;i<searchModification.size();i++){
      if (searchModification[i].residues[0] == 'C' && fabs(searchModification[i].massDelta-57.0215)<0.1){
        sm.massDelta=39.994915;
        break;
      }
    }
  }

  //add cvParams from lookup table
  sm.cvParam.push_back(findCvParam(mass, residues));

  //add specificity rules
  sCvParam cv;
  if (protTerm){
    if (residues.size()>0 && residues[0] == 'n'){
      cv.accession = "MS:1002057";
      cv.cvRef = "PSI-MS";
      cv.name = "modification specificity protein N-term";
    }
    if (residues.size()>0 && residues[0] == 'c'){
      cv.accession = "MS:1002058";
      cv.cvRef = "PSI-MS";
      cv.name = "modification specificity protein C-term";
    }
  } else {
    if (residues.size()>0 && residues[0] == 'n'){
      cv.accession = "MS:1001189";
      cv.cvRef = "PSI-MS";
      cv.name = "modification specificity peptide N-term";
    }
    if (residues.size()>0 && residues[0] == 'c'){
      cv.accession = "MS:1001190";
      cv.cvRef = "PSI-MS";
      cv.name = "modification specificity peptide C-term";
    }
  }
  sm.cvParam.push_back(cv);

  //only add if not already defined.
  size_t a;
  for(a=0;a<searchModification.size();a++){
    if(sm==searchModification[a]) break;
  }
  if(a==searchModification.size()) searchModification.push_back(sm);

}

void CModificationParams::addSearchModificationXL(double mass, string residues, string residues2){

  vector<CSearchModification> v;
  CSearchModification sm;
  sm.fixedMod = false;
  sm.massDelta = mass;
  
  //process termini
  size_t i;
  sCvParam param;
  for(i=0;i<residues.size();i++){
    if (residues[i] == 'n') {
      sm.residues = ".";
      param.accession = "MS:1002057";
      param.cvRef = "PSI-MS";
      param.name = "modification specificity protein N-term";
      if(sm.specificityRules.size()==0){
        CSpecificityRules sr;
        sm.specificityRules.push_back(sr);
      }
      sm.specificityRules.back().cvParam.push_back(param);
    } else if (residues[i] == 'c') {
      sm.residues = ".";
      param.accession = "MS:1002058";
      param.cvRef = "PSI-MS";
      param.name = "modification specificity protein C-term";
      if (sm.specificityRules.size() == 0){
        CSpecificityRules sr;
        sm.specificityRules.push_back(sr);
      }
      sm.specificityRules.back().cvParam.push_back(param);
    }
  }
  sm.cvParam.push_back(findCvParam(mass, residues)); 
  param.accession="MS:1002509";
  param.cvRef="PSI-MS";
  param.name="cross-link donor";
  sm.cvParam.push_back(param);
  v.push_back(sm);
  sm.massDelta=0;
  sm.cvParam.back().accession = "MS:1002510";
  sm.cvParam.back().name = "cross-link acceptor";
  v.push_back(sm);

  //process amino acids
  sm.clear();
  sm.fixedMod = false;
  sm.massDelta = mass;
  sm.residues="";
  for (i = 0; i<residues.size(); i++){
    if (residues[i] != 'n' && residues[i] != 'c') sm.residues += residues[i];
  }
  sm.cvParam.push_back(findCvParam(mass, residues));
  param.accession = "MS:1002509";
  param.cvRef = "PSI-MS";
  param.name = "cross-link donor";
  sm.cvParam.push_back(param);
  v.push_back(sm);
  sm.massDelta = 0;
  sm.cvParam.back().accession = "MS:1002510";
  sm.cvParam.back().name = "cross-link acceptor";
  v.push_back(sm);

  //repeat for all heterobifunctional cross-linkers. Makes the paramaters look light a mess in mzID, right?
  if(residues.compare(residues2)!=0) {

    //termini
    sm.clear();
    sm.fixedMod = false;
    sm.massDelta = mass;
    for (i = 0; i<residues2.size(); i++){
      if (residues2[i] == 'n') {
        sm.residues = ".";
        param.accession = "MS:1002057";
        param.cvRef = "PSI-MS";
        param.name = "modification specificity protein N-term";
        if (sm.specificityRules.size() == 0){
          CSpecificityRules sr;
          sm.specificityRules.push_back(sr);
        }
        sm.specificityRules.back().cvParam.push_back(param);
      } else if (residues2[i] == 'c') {
        sm.residues = ".";
        param.accession = "MS:1002058";
        param.cvRef = "PSI-MS";
        param.name = "modification specificity protein C-term";
        if (sm.specificityRules.size() == 0){
          CSpecificityRules sr;
          sm.specificityRules.push_back(sr);
        }
        sm.specificityRules.back().cvParam.push_back(param);
      }
    }
    sm.cvParam.push_back(findCvParam(mass, residues2));
    param.accession = "MS:1002509";
    param.cvRef = "PSI-MS";
    param.name = "cross-link donor";
    sm.cvParam.push_back(param);
    v.push_back(sm);
    sm.massDelta = 0;
    sm.cvParam.back().accession = "MS:1002510";
    sm.cvParam.back().name = "cross-link acceptor";
    v.push_back(sm);

    //process amino acids
    sm.clear();
    sm.fixedMod = false;
    sm.massDelta = mass;
    sm.residues = "";
    for (i = 0; i<residues2.size(); i++){
      if (residues2[i] != 'n' && residues2[i] != 'c') sm.residues += residues2[i];
    }
    sm.cvParam.push_back(findCvParam(mass, residues2));
    param.accession = "MS:1002509";
    param.cvRef = "PSI-MS";
    param.name = "cross-link donor";
    sm.cvParam.push_back(param);
    v.push_back(sm);
    sm.massDelta = 0;
    sm.cvParam.back().accession = "MS:1002510";
    sm.cvParam.back().name = "cross-link acceptor";
    v.push_back(sm);
  }

  //add our mods
  for(i=0;i<v.size();i++){
    searchModification.push_back(v[i]);
  }
}

//void CModificationParams::addSearchModification(CSearchModification& c){
//
//  //clear any placeholders
//  if (searchModification->at(0).residues.compare("null") == 0) searchModification->clear();
//  searchModification->push_back(c);
//
//}

sCvParam CModificationParams::findCvParam(double mass, string residues){
  sCvParam cv;
  cv.accession = "MS:1001460";
  cv.name = "unknown modification";
  cv.cvRef = "PSI-MS";
  if (fabs(mass - 15.994915) < 0.001){
    cv.accession = "UNIMOD:35"; cv.cvRef = "UNIMOD"; cv.name = "Oxidation";
  } else if (fabs(mass - 57.021464) < 0.001){
    if (residues.size()>0 && residues[0] == 'C') {
      cv.accession = "UNIMOD:4"; cv.cvRef = "UNIMOD"; cv.name = "Carbamidomethylation";
    } 
  } else if (fabs(mass +17.026549) < 0.001){
    if (residues.size()>0 && residues[0] == 'Q') {
      cv.accession = "UNIMOD:28"; cv.cvRef = "UNIMOD"; cv.name = "Gln->pryo-Glu";
    } else if (residues.size() > 0 && residues[0] == 'C') {
      cv.accession = "UNIMOD:385"; cv.cvRef = "UNIMOD"; cv.name = "Ammonia-loss";
    } else {
      cv.accession = "UNIMOD:385"; cv.cvRef = "UNIMOD"; cv.name = "Ammonia-loss";
    }
  } else if (fabs(mass + 18.010565) < 0.001){
    if (residues.size() > 0 && residues[0] == 'E') {
      cv.accession = "UNIMOD:27"; cv.cvRef = "UNIMOD"; cv.name = "Glu->pyro-Glu";
    } else {
      cv.accession = "UNIMOD:23"; cv.cvRef = "UNIMOD"; cv.name = "Dehydrated";
    }
  } else if (fabs(mass - 144.102063) < 0.001){
    cv.accession = "UNIMOD:214"; cv.cvRef = "UNIMOD"; cv.name = "iTRAQ4plex";
  } else if (fabs(mass - 79.966331) < 0.001){
    cv.accession = "UNIMOD:21"; cv.cvRef = "UNIMOD"; cv.name = "Phospho";
  } else if (fabs(mass - 39.994915) < 0.001){
    cv.accession = "UNIMOD:26"; cv.cvRef = "UNIMOD"; cv.name = "Pyro-carbamidomethyl";
  } else if (fabs(mass - 156.078644) < 0.001){
    cv.accession = "UNIMOD:1020"; cv.cvRef = "UNIMOD"; cv.name = "Xlink:DSS[156]";
  } else if (fabs(mass - 155.094629) < 0.001){
    cv.accession = "UNIMOD:1789"; cv.cvRef = "UNIMOD"; cv.name = "Xlink:DSS[155]";
  } else if (fabs(mass - 138.06808) < 0.001){
    cv.accession = "UNIMOD:1898"; cv.cvRef = "UNIMOD"; cv.name = "Xlink:DSS[138]";
  } else if (fabs(mass - 42.010565) < 0.001) {
    cv.accession = "UNIMOD:1"; cv.cvRef = "UNIMOD"; cv.name = "Acetyl";
  } else if (fabs(mass - 0.984016) < 0.001) {
    cv.accession = "UNIMOD:7"; cv.cvRef = "UNIMOD"; cv.name = "Deamidated";
  } else if (fabs(mass - 27.994915) < 0.001) {
    cv.accession = "UNIMOD:122"; cv.cvRef = "UNIMOD"; cv.name = "Formyl";
  } else if (fabs(mass - 304.202200) < 0.001) { //note that the actual mass in UNIMOD is only the first four channels (304.20536)
    cv.accession = "UNIMOD:730"; cv.cvRef = "UNIMOD"; cv.name = "iTRAQ8plex";
  }
  return cv;
}

sCvParam CModificationParams::getModificationCvParam(double monoisotopicMassDelta, string residues, bool nTerm, bool cTerm){
  size_t i;
  for (i = 0; i < searchModification.size(); i++){
    //cout << i << "\t" << fabs(searchModification[i].massDelta - monoisotopicMassDelta) << "\t" << searchModification[i].residues << endl;
    if ((nTerm || cTerm) && fabs(searchModification[i].massDelta - monoisotopicMassDelta)<0.001 && searchModification[i].residues.compare(".") == 0){
      return searchModification[i].cvParam[0];
    } else if (fabs(searchModification[i].massDelta - monoisotopicMassDelta)<0.001 && searchModification[i].residues.compare(residues)==0) {
      return searchModification[i].cvParam[0];
    } 
  }
  sCvParam cv;
  cv.accession = "MS:1001460";
  cv.name = "unknown modification";
  cv.cvRef = "PSI-MS";
  //cout << "Unknown: " << monoisotopicMassDelta << "\t" << residues << endl;
  //exit(1);
  return cv;
}

void CModificationParams::writeOut(FILE* f, int tabs){
  if (searchModification.empty()){
    cerr << "ModificationParams::SearchModification is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ModificationParams>\n");

  int t = tabs;
  if (t>-1)t++;

  for (size_t j = 0; j<searchModification.size(); j++) searchModification[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ModificationParams>\n");

}
