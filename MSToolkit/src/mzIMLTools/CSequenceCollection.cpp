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

#include "CSequenceCollection.h"

using namespace std;

//CSequenceCollection::CSequenceCollection(){
//  CDBSequence dbs;
//  dbs.id = "null";
//  dbSequence = new vector<CDBSequence>;
//  dbSequence->push_back(dbs);
//
//  peptide = new vector<CPeptide>;
//  peptideEvidence = new vector<CPeptideEvidence>;
//
//  sortDBSequence=false;
//  sortDBSequenceAcc=false;
//  sortPeptide=false;
//  sortPeptideSeq=false;
//  sortPeptideEvidence=false;
//  sortPeptideEvidencePepRef=false;
//}
//
//CSequenceCollection::~CSequenceCollection(){
//  delete dbSequence;
//  delete peptide;
//  delete peptideEvidence;
//}

//iterate through sequences to see if we have it already
//if so, return the existing id, else add this new one
string CSequenceCollection::addDBSequence(CDBSequence& dbs){
  pair<multimap<string, size_t>::iterator, multimap<string, size_t>::iterator> pit;
  pit = mmDBTable.equal_range(dbs.accession);
  for (multimap<string, size_t>::iterator i = pit.first; i != pit.second; i++){
    if (dbSequence[i->second].searchDatabaseRef.compare(dbs.searchDatabaseRef)==0) return dbSequence[i->second].id;
  }

  //add new sequence
  char dbid[32];
  sprintf(dbid, "DBSeq%d", (int)dbSequence.size());
  dbs.id = dbid;
  dbSequence.push_back(dbs);
  mmDBTable.insert(pit.second, pair<string, size_t>(dbs.accession, dbSequence.size() - 1));
  mDBIDTable.insert(pair<string, size_t>(dbs.id, dbSequence.size() - 1));

  return dbs.id;
}

//iterate through peptides to see if we have it already
//if so, return the existing id, else add this new one
string CSequenceCollection::addPeptide(CPeptide& p){

  pair<multimap<string,size_t>::iterator,multimap<string,size_t>::iterator> pit;
  pit = mmPepTable.equal_range(p.peptideSequence.text);
  for (multimap<string, size_t>::iterator i = pit.first; i != pit.second; i++){
    if (peptide[i->second] == p) return peptide[i->second].id;
  }

  //add new peptide 
  char dbid[32];
  sprintf(dbid, "Pep%d", (int)peptide.size());
  p.id = dbid;
  peptide.push_back(p);
  mmPepTable.insert(pit.second,pair<string,size_t>(p.peptideSequence.text,peptide.size()-1));
  mPepIDTable.insert(pair<string,size_t>(p.id, peptide.size() - 1));

  return p.id;
}

//iterate through peptide evidences to see if we have it already
//if so, return the existing id, else add this new one
//BTW, PeptideEvidenceRef seems to be an unneccessary extra layer...
sPeptideEvidenceRef CSequenceCollection::addPeptideEvidence(CPeptideEvidence& p){

  pair<multimap<string, size_t>::iterator, multimap<string, size_t>::iterator> pit;
  pit = mmPepEvTable.equal_range(p.peptideRef);
  for (multimap<string, size_t>::iterator i = pit.first; i != pit.second; i++){
    if (peptideEvidence[i->second].dbSequenceRef.compare(p.dbSequenceRef)==0) {
      sPeptideEvidenceRef peRef;
      peRef.peptideEvidenceRef=peptideEvidence[i->second].id;
      return peRef;
    }
  }

  //add new peptide evidence
  char dbid[32];
  sprintf(dbid, "PE%d", (int)peptideEvidence.size());
  p.id = dbid;
  peptideEvidence.push_back(p);
  sPeptideEvidenceRef peRef;
  peRef.peptideEvidenceRef = p.id;
  mmPepEvTable.insert(pit.second, pair<string, size_t>(p.peptideRef, peptideEvidence.size() - 1));
  mPepEvIDTable.insert(pair<string, size_t>(p.id, peptideEvidence.size() - 1));

  return peRef;
}

//iterate through cross-linked peptides to see if we have them already
//if so, return the existing ids, else add these new ones
bool CSequenceCollection::addXLPeptides(string ID, CPeptide& p1, CPeptide& p2, string& ref1, string& ref2, string& value){

  //Find if peptide already listed by binary search
  size_t sz = vXLPepTable.size() - vXLPepTable.size() % 100; //buffer of 100 unsorted entries
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  if (sz>0){
    i = vXLPepTable[mid].ID.compare(ID);
    while (i != 0){
      if (lower >= upper) break;
      if (i>0){
        if (mid == 0) break;
        upper = mid - 1;
        mid = (lower + upper) / 2;
      } else {
        lower = mid + 1;
        mid = (lower + upper) / 2;
      }
      if (mid == sz) break;
      i = vXLPepTable[mid].ID.compare(ID);
    }

    //match, so return references
    if (i == 0){
      ref1=vXLPepTable[mid].refLong;
      ref2=vXLPepTable[mid].refShort;
      value =vXLPepTable[mid].value;
      return true;
    }
  }

  //check unsorted proteins
  for (mid = sz; mid < vXLPepTable.size(); mid++){
    if (vXLPepTable[mid].ID.compare(ID) == 0) {
      ref1 = vXLPepTable[mid].refLong;
      ref2 = vXLPepTable[mid].refShort;
      value = vXLPepTable[mid].value;
      return true;
    }
  }
  
  //add new peptides
  char str[12];
  sprintf(str,"%d",(int)vXLPepTable.size());
  value=str;
  if (p1.id.empty()){
    char dbid[32];
    sprintf(dbid, "Pep%d", (int)peptide.size());
    p1.id = dbid;
    sCvParam cv;
    cv.cvRef="PSI-MS";
    cv.accession = "MS:1002509";
    cv.name = "cross-link donor";
    cv.value = value;
    p1.modification.back().cvParam.push_back(cv);
  }
  peptide.push_back(p1);
  ref1=p1.id;

  if (p2.id.empty()){
    char dbid[32];
    sprintf(dbid, "Pep%d", (int)peptide.size());
    p2.id = dbid;
    sCvParam cv;
    cv.cvRef = "PSI-MS";
    cv.accession = "MS:1002510";
    cv.name = "cross-link acceptor";
    cv.value = value;
    p2.modification.back().cvParam.push_back(cv);
  }
  peptide.push_back(p2);
  ref2 = p2.id;

  sXLPepTable pt;
  pt.ID = ID;
  pt.refLong = ref1;
  pt.refShort = ref2;
  pt.value = value;
  vXLPepTable.push_back(pt);

  //sort when buffer fills up
  if (vXLPepTable.size() % 100 == 0) {
    sort(vXLPepTable.begin(), vXLPepTable.end(), compareXLPeptideTable);
  }

  return true;
}

//binary searches for the DBSequence. If the sequence list is out of order, it gets sorted NOW.
CDBSequence CSequenceCollection::getDBSequence(string id){
  map<string, size_t>::iterator it;
  it = mDBIDTable.find(id);

  if (it == mDBIDTable.end()){
    cout << "CSequenceCollection::getDBSequence() failed. Returning last DBSequence in list." << endl;
    return dbSequence.back();
  }
  return dbSequence[it->second];

}

//binary searches for the DBSequence by accession. If the sequence list is out of order, it gets sorted NOW.
CDBSequence* CSequenceCollection::getDBSequenceByAcc(string acc){

  multimap<string, size_t>::iterator it;
  it = mmDBTable.find(acc);

  if (it == mmDBTable.end()){
    cout << "CSequenceCollection::getDBSequenceByAcc(string) failed. Returning last DBSequence in list." << endl;
    return &dbSequence.back();
  }
  return &dbSequence[it->second];

}

//binary searches for the DBSequence. If the sequence list is out of order, it gets sorted NOW.
void CSequenceCollection::getDBSequenceByAcc(string acc, vector<CDBSequence>& v){

  pair<multimap<string, size_t>::iterator, multimap<string, size_t>::iterator> pit;
  pit = mmDBTable.equal_range(acc);
  v.clear();
  for (multimap<string, size_t>::iterator i = pit.first; i != pit.second; i++){
    v.push_back(dbSequence[i->second]);
  }

}

//binary searches for the peptide by reference. If the peptide list is out of order, it gets sorted NOW.
CPeptide* CSequenceCollection::getPeptide(string peptideRef){

  map<string,size_t>::iterator pit;
  pit = mPepIDTable.find(peptideRef);

  if(pit==mPepIDTable.end()){
    cout << "CSequenceCollection::getPeptide() failed. Returning last peptide in list." << endl;
    return &peptide.back();
  }
  return &peptide[pit->second];

}

CPeptideEvidence CSequenceCollection::getPeptideEvidence(string& id){
  map<string, size_t>::iterator pit;
  pit = mPepEvIDTable.find(id);

  if (pit == mPepEvIDTable.end()){
    cout << "CSequenceCollection::getPeptideEvidence() failed. Returning last peptideEvidence in list." << endl;
    return peptideEvidence.back();
  }
  return peptideEvidence[pit->second];

}

bool CSequenceCollection::getPeptideEvidenceFromPeptideAndProtein(CPeptide& p, string dbSequenceRef, vector<string>& vPE){

  vPE.clear();
  vector<string> peptideRef;
  pair<multimap<string, size_t>::iterator, multimap<string, size_t>::iterator> pit;
  pair<multimap<string, size_t>::iterator, multimap<string, size_t>::iterator> pit2;
  
  //look up peptideRef from peptide
  pit = mmPepTable.equal_range(p.peptideSequence.text);
  for (multimap<string, size_t>::iterator i = pit.first; i != pit.second; i++){
    if (peptide[i->second].compareModsSoft(p)){

      pit2 = mmPepEvTable.equal_range(peptide[i->second].id);
      for (multimap<string, size_t>::iterator j = pit2.first; j != pit2.second; j++){
        if (peptideEvidence[j->second].dbSequenceRef.compare(dbSequenceRef) == 0){
          vPE.push_back(peptideEvidence[j->second].id);
        }
      }

    }
  }
  if (vPE.size() == 0) return false;
  return true;
}

//binary searches for the peptide by reference. If the peptide list is out of order, it gets sorted NOW.
string CSequenceCollection::getProtein(sPeptideEvidenceRef& s){

  map<string, size_t>::iterator pit;
  pit = mPepEvIDTable.find(s.peptideEvidenceRef);

  if (pit == mPepEvIDTable.end()){
    cout << "CSequenceCollection::getProtein() failed. Returning emptry string." << endl;
    string st;
    return st;
  }

  return getDBSequence(peptideEvidence[pit->second].dbSequenceRef).accession;

}

void CSequenceCollection::rebuildDBTable(){
  mmDBTable.clear();
  mDBIDTable.clear();
  for(size_t a=0;a<dbSequence.size();a++){
    mmDBTable.insert(pair<string, size_t>(dbSequence[a].accession,a));
    mDBIDTable.insert(pair<string, size_t>(dbSequence[a].id, a));
  }
}

void CSequenceCollection::rebuildPepEvTable(){
  mmPepEvTable.clear();
  mPepEvIDTable.clear();
  for(size_t a=0;a<peptideEvidence.size();a++){
    mmPepEvTable.insert(pair<string, size_t>(peptideEvidence[a].peptideRef, a));
    mPepEvIDTable.insert(pair<string, size_t>(peptideEvidence[a].id, a));
  }
}

void CSequenceCollection::rebuildPepTable(){
  mmPepTable.clear();
  mPepIDTable.clear();
  for(size_t a=0;a<peptide.size();a++){
    mmPepTable.insert(pair<string, size_t>(peptide[a].peptideSequence.text, a));
    mPepIDTable.insert(pair<string, size_t>(peptide[a].id, a));
  }
}

void CSequenceCollection::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SequenceCollection>\n");

  int t=tabs;
  if(t>-1) t++;

  for (j = 0; j < dbSequence.size(); j++) dbSequence[j].writeOut(f,t);
  for (j = 0; j < peptide.size(); j++) peptide[j].writeOut(f, t);
  for (j = 0; j < peptideEvidence.size(); j++) peptideEvidence[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SequenceCollection>\n");
}


bool CSequenceCollection::compareDBSequence(const CDBSequence& a, const CDBSequence& b){
  return (a.id.compare(b.id)<0);
}

bool CSequenceCollection::compareDBSequenceAcc(const CDBSequence& a, const CDBSequence& b){
  return (a.accession.compare(b.accession)<0);
}

bool CSequenceCollection::comparePeptide(const CPeptide& a, const CPeptide& b){
  return (a.id.compare(b.id)<0);
}

bool CSequenceCollection::comparePeptideSeq(const CPeptide& a, const CPeptide& b){
  return (a.peptideSequence.text.compare(b.peptideSequence.text)<0);
}

bool CSequenceCollection::comparePeptideTable(const sPepTable& a, const sPepTable& b){
  return (a.seq.compare(b.seq)<0);
}

bool CSequenceCollection::comparePeptideEvidence(const CPeptideEvidence& a, const CPeptideEvidence& b){
  return (a.id.compare(b.id)<0);
}

bool CSequenceCollection::comparePeptideEvidencePepRef(const CPeptideEvidence& a, const CPeptideEvidence& b){
  return (a.peptideRef.compare(b.peptideRef)<0);
}

bool CSequenceCollection::compareXLPeptideTable(const sXLPepTable& a, const sXLPepTable& b){
  return (a.ID.compare(b.ID)<0);
}


