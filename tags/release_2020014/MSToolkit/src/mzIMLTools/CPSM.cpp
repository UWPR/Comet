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

#include "CPSM.h"

using namespace std;

CPSM::CPSM(){
  charge=0;
  modCount=0;
  proteinCount=0;
  scoreCount=0;
  calcNeutMass=0;
  mass=0;
  mzObs=0;
  sequence.clear();
  sequenceMod.clear();
  mods = NULL;
  scores = NULL;
  proteins=NULL;
}

CPSM::CPSM(const CPSM& c){
  charge = c.charge;
  modCount = c.modCount;
  proteinCount=c.proteinCount;
  scoreCount = c.scoreCount;
  calcNeutMass = c.calcNeutMass;
  mass = c.mass;
  mzObs = c.mzObs;
  sequence = c.sequence;
  sequenceMod = c.sequenceMod;
  scanInfo = c.scanInfo;
  if (c.modCount > 0){
    mods = new sPSMMod[c.modCount];
    for (int i = 0; i<c.modCount; i++) mods[i] = c.mods[i];
  } else {
    mods = NULL;
  }
  if (c.scoreCount > 0){
    scores = new sPSMScore[c.scoreCount];
    for (int i = 0; i<c.scoreCount; i++) scores[i] = c.scores[i];
  } else {
    scores = NULL;
  }
  if (c.proteinCount > 0){
    proteins = new string[c.proteinCount];
    for (int i = 0; i<c.proteinCount; i++) proteins[i] = c.proteins[i];
  } else {
    proteins = NULL;
  }
}

CPSM::~CPSM(){
  if (mods != NULL) delete[] mods;
  if (scores != NULL) delete[] scores;
  if (proteins != NULL) delete[] proteins;
}

CPSM& CPSM::operator=(const CPSM& c){
  if (this != &c){
    charge = c.charge;
    modCount = c.modCount;
    proteinCount = c.proteinCount;
    scoreCount = c.scoreCount;
    calcNeutMass = c.calcNeutMass;
    mass = c.mass;
    mzObs = c.mzObs;
    sequence = c.sequence;
    sequenceMod = c.sequenceMod;
    scanInfo = c.scanInfo;
    if (mods != NULL) delete[] mods;
    if (c.modCount > 0){
      mods = new sPSMMod[c.modCount];
      for (int i = 0; i<c.modCount; i++) mods[i] = c.mods[i];
    } else {
      mods = NULL;
    }
    if (scores != NULL) delete[] scores;
    if (c.scoreCount > 0){
      scores = new sPSMScore[c.scoreCount];
      for (int i = 0; i<c.scoreCount; i++) scores[i] = c.scores[i];
    } else {
      scores = NULL;
    }
    if (proteins != NULL) delete[] proteins;
    if (c.proteinCount > 0){
      proteins = new string[c.proteinCount];
      for (int i = 0; i<c.proteinCount; i++) proteins[i] = c.proteins[i];
    } else {
      proteins = NULL;
    }
  }
  return *this;
}

void CPSM::addMods(vector<sPSMMod>& v){
  if (mods != NULL) delete[] mods;
  modCount = (int)v.size();
  mods = new sPSMMod[modCount];
  for (size_t i = 0; i<v.size(); i++) mods[i] = v[i];
}

void CPSM::addProteins(vector<string>& v){
  if (proteins != NULL) delete[] proteins;
  proteinCount = (int)v.size();
  proteins = new string[proteinCount];
  for (size_t i = 0; i<v.size(); i++) proteins[i] = v[i];
}

void CPSM::addScores(vector<sPSMScore>& v){
  if (scores != NULL) delete[] scores;
  scoreCount = (int)v.size();
  scores = new sPSMScore[scoreCount];
  for (size_t i = 0; i<v.size(); i++) scores[i] = v[i];
}

sPSMMod CPSM::getMod(int index){
  if (index >= modCount){
    sPSMMod s;
    return s;
  }
  return mods[index];
}

string CPSM::getProtein(int index){
  if (index >= proteinCount){
    return "";
  }
  return proteins[index];
}

sPSMScore CPSM::getScore(int index){
  if (index >= scoreCount){
    sPSMScore s;
    return s;
  }
  return scores[index];
}