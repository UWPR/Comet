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

#include "CSequenceCollection.h"

using namespace std;

CSequenceCollection::CSequenceCollection(){
  CDBSequence dbs;
  dbs.id = "null";
  dbSequence = new vector<CDBSequence>;
  dbSequence->push_back(dbs);

  peptide = new vector<CPeptide>;
  peptideEvidence = new vector<CPeptideEvidence>;

  sortDBSequence=false;
  sortDBSequenceAcc=false;
  sortPeptide=false;
  sortPeptideSeq=false;
  sortPeptideEvidence=false;
  sortPeptideEvidencePepRef=false;
}

CSequenceCollection::~CSequenceCollection(){
  delete dbSequence;
  delete peptide;
  delete peptideEvidence;
}

//iterate through sequences to see if we have it already
//if so, return the existing id, else add this new one
string CSequenceCollection::addDBSequence(CDBSequence& dbs){
  //get rid of any null placeholders
  if (dbSequence->at(0).id.compare("null") == 0) dbSequence->clear();

  size_t i;
  for (i = 0; i < dbSequence->size(); i++){
    if (dbSequence->at(i).searchDatabaseRef.compare(dbs.searchDatabaseRef)!=0) continue;
    if (dbSequence->at(i).accession.compare(dbs.accession) == 0) return dbSequence->at(i).id;
  }

  //add new sequence
  if (dbs.id.compare("null") == 0) {
    char dbid[32];
    sprintf(dbid, "DBSeq%zu", dbSequence->size());
    dbs.id=dbid;
  }
  dbSequence->push_back(dbs);

  sortDBSequence = true;
  sortDBSequenceAcc = true;
  return dbs.id;
}

//iterate through peptides to see if we have it already
//if so, return the existing id, else add this new one
string CSequenceCollection::addPeptide(CPeptide& p){

  //Find if peptide already listed by binary search
  size_t sz = vPepTable.size() - vPepTable.size() % 100; //buffer of 100 unsorted entries
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  if (sz>0){
    i = vPepTable[mid].seq.compare(p.peptideSequence.text);
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
      i = vPepTable[mid].seq.compare(p.peptideSequence.text);
    }

    //match by peptide sequence, so check modifications
    if (i == 0){
      if (peptide->at(vPepTable[mid].index) == p) return peptide->at(vPepTable[mid].index).id;
      lower = mid - 1;
      while (lower < mid && vPepTable[lower].seq.compare(p.peptideSequence.text) == 0){
        if (peptide->at(vPepTable[lower].index) == p) return peptide->at(vPepTable[lower].index).id;
        lower--;
      }
      upper = mid + 1;
      while (upper < sz && vPepTable[upper].seq.compare(p.peptideSequence.text) == 0){
        if (peptide->at(vPepTable[upper].index) == p) return peptide->at(vPepTable[upper].index).id;
        upper++;
      }
    }
  }

  //check unsorted proteins
  for (mid = sz; mid < vPepTable.size(); mid++){
    if (peptide->at(vPepTable[mid].index) == p) return peptide->at(vPepTable[mid].index).id;
  }

  //add new peptide - should the id ever NOT be null when we get here?
  if (p.id.compare("null") == 0){
    char dbid[32];
    sprintf(dbid, "Pep%zu", peptide->size());
    p.id = dbid;
  }
  peptide->push_back(p);

  sPepTable pt;
  pt.index = peptide->size()-1;
  pt.seq = p.peptideSequence.text;
  vPepTable.push_back(pt);

  //sort when buffer fills up
  if (vPepTable.size() % 100 == 0) {
    sort(vPepTable.begin(), vPepTable.end(), comparePeptideTable);
  }

  sortPeptide=true;
  sortPeptideSeq=true;
  return p.id;
}

//iterate through peptide evidences to see if we have it already
//if so, return the existing id, else add this new one
//BTW, PeptideEvidenceRef seems to be an unneccessary extra layer...
sPeptideEvidenceRef CSequenceCollection::addPeptideEvidence(CPeptideEvidence& p){

  sPeptideEvidenceRef peRef;

  //Find if peptide already listed by binary search
  size_t sz = vPepEvTable.size() - vPepEvTable.size() % 100; //buffer of 100 unsorted entries
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  if (sz>0){
    i = vPepEvTable[mid].seq.compare(p.peptideRef);
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
      i = vPepEvTable[mid].seq.compare(p.peptideRef);
    }

    //match by peptide sequence, so check modifications
    if (i == 0){
      if (peptideEvidence->at(vPepEvTable[mid].index) == p) { //this can look just at dbRef to save time
        peRef.peptideEvidenceRef = peptideEvidence->at(vPepEvTable[mid].index).id;
        return peRef;
      }
      lower = mid - 1;
      while (lower < mid && vPepEvTable[lower].seq.compare(p.peptideRef) == 0){
        if (peptideEvidence->at(vPepEvTable[lower].index) == p) {
          peRef.peptideEvidenceRef = peptideEvidence->at(vPepEvTable[lower].index).id;
          return peRef;
        }
        lower--;
      }
      upper = mid + 1;
      while (upper < sz && vPepEvTable[upper].seq.compare(p.peptideRef) == 0){
        if (peptideEvidence->at(vPepEvTable[upper].index) == p) {
          peRef.peptideEvidenceRef = peptideEvidence->at(vPepEvTable[upper].index).id;
          return peRef;
        }
        upper++;
      }
    }
  }

  //check unsorted proteins
  for (mid = sz; mid < vPepEvTable.size(); mid++){
    if (peptideEvidence->at(vPepEvTable[mid].index) == p) {
      peRef.peptideEvidenceRef = peptideEvidence->at(vPepEvTable[mid].index).id;
      return peRef;
    }
  }

  //add new peptide evidence
  if (p.id.compare("null") == 0) {
    char dbid[32];
    sprintf(dbid, "PE%zu", peptideEvidence->size());
    p.id = dbid;
  }
  peptideEvidence->push_back(p);
  peRef.peptideEvidenceRef=p.id;

  sPepTable pt;
  pt.index = peptideEvidence->size() - 1;
  pt.seq = p.peptideRef;
  vPepEvTable.push_back(pt);

  //sort when buffer fills up
  if (vPepEvTable.size() % 100 == 0) {
    sort(vPepEvTable.begin(), vPepEvTable.end(), comparePeptideTable);
  }

  sortPeptideEvidence = true;
  sortPeptideEvidencePepRef = true;
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
  size_t a,b;
  char str[12];
  sprintf(str,"%zu",vXLPepTable.size());
  value=str;
  if (p1.id.compare("null") == 0){
    char dbid[32];
    sprintf(dbid, "Pep%zu", peptide->size());
    p1.id = dbid;
    //find xl modification and change placeholder to "Donor"
    for(a=0;a<p1.modification->size();a++){
      for(b=0;b<p1.modification->at(a).cvParam->size();b++){
        if(p1.modification->at(a).cvParam->at(b).cvRef.compare("XLtmp")==0){
          p1.modification->at(a).cvParam->at(b).cvRef="PSI-MS";
          p1.modification->at(a).cvParam->at(b).accession = "MS:1002509";
          p1.modification->at(a).cvParam->at(b).name = "cross-link donor";
          p1.modification->at(a).cvParam->at(b).value = value;
          break;
        }
      }
      if (b<p1.modification->at(a).cvParam->size()) break;
    }
    if (a == p1.modification->size()){
      cout << "ERROR: cannot find XL peptide placeholder" << endl;
      exit(671);
    }
  }
  peptide->push_back(p1);
  ref1=p1.id;

  if (p2.id.compare("null") == 0){
    char dbid[32];
    sprintf(dbid, "Pep%zu", peptide->size());
    p2.id = dbid;
    //find xl modification and change placeholder to "acceptor"
    for (a = 0; a<p2.modification->size(); a++){
      for (b = 0; b<p2.modification->at(a).cvParam->size(); b++){
        if (p2.modification->at(a).cvParam->at(b).cvRef.compare("XLtmp") == 0){
          p2.modification->at(a).cvParam->at(b).cvRef = "PSI-MS";
          p2.modification->at(a).cvParam->at(b).accession = "MS:1002510";
          p2.modification->at(a).cvParam->at(b).name = "cross-link acceptor";
          p2.modification->at(a).cvParam->at(b).value = value;
          break;
        }
      }
      if (b<p2.modification->at(a).cvParam->size()) {
        p2.modification->at(a).monoisotopicMassDelta=0;
        p2.modification->at(a).cvParam->pop_back(); //remove cross-linker cvParam, it was used on the donor.
        break;
      }
    }
    if (a == p2.modification->size()){
      cout << "ERROR: cannot find XL peptide placeholder" << endl;
      exit(671);
    }
  }
  peptide->push_back(p2);
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

  sortPeptide = true;
  sortPeptideSeq = true;
  return true;
}

void CSequenceCollection::doDBSequenceSort(){
  sort(dbSequence->begin(), dbSequence->end(), compareDBSequence);
  sortDBSequence = false;
  sortDBSequenceAcc = true;
}

void CSequenceCollection::doDBSequenceSortAcc(){
  sort(dbSequence->begin(), dbSequence->end(), compareDBSequenceAcc);
  sortDBSequence = true;
  sortDBSequenceAcc = false;
}

void CSequenceCollection::doPeptideSort(){
  sort(peptide->begin(), peptide->end(), comparePeptide);
  sortPeptide = false;
  sortPeptideSeq = true;
  rebuildPepTable();
}

void CSequenceCollection::doPeptideSeqSort(){
  sort(peptide->begin(), peptide->end(), comparePeptideSeq);
  sortPeptideSeq = false;
  sortPeptide=true;
  rebuildPepTable();
}

void CSequenceCollection::doPeptideEvidenceSort(){
  sort(peptideEvidence->begin(),peptideEvidence->end(),comparePeptideEvidence);
  sortPeptideEvidence=false;
  sortPeptideEvidencePepRef=true;
  rebuildPepEvTable();
}

void CSequenceCollection::doPeptideEvidencePepRefSort(){
  sort(peptideEvidence->begin(), peptideEvidence->end(), comparePeptideEvidencePepRef);
  sortPeptideEvidencePepRef=false;
  sortPeptideEvidence = true;
  rebuildPepEvTable();
}

//binary searches for the DBSequence. If the sequence list is out of order, it gets sorted NOW.
CDBSequence CSequenceCollection::getDBSequence(string id){
  CDBSequence blank;
  if (sortDBSequence) doDBSequenceSort();
  size_t sz = dbSequence->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  i = dbSequence->at(mid).id.compare(id);
  while (i != 0){
    if (lower >= upper) return blank;
    if (i>0){
      if (mid == 0) return blank;
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return blank;
    i = dbSequence->at(mid).id.compare(id);
  }
  return dbSequence->at(mid);

}

//binary searches for the DBSequence by accession. If the sequence list is out of order, it gets sorted NOW.
CDBSequence* CSequenceCollection::getDBSequenceByAcc(string acc){
  if (sortDBSequenceAcc) doDBSequenceSortAcc();
  size_t sz = dbSequence->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  i = dbSequence->at(mid).accession.compare(acc);
  while (i != 0){
    if (lower >= upper) return NULL;
    if (i>0){
      if (mid == 0) return NULL;
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return NULL;
    i = dbSequence->at(mid).accession.compare(acc);
  }
  return &dbSequence->at(mid);

}

//binary searches for the peptide by reference. If the peptide list is out of order, it gets sorted NOW.
CPeptide* CSequenceCollection::getPeptide(string peptideRef){
  if (sortPeptide) doPeptideSort();
  size_t sz = peptide->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  //cout << "Starting search for " << peptideRef << endl;

  i = peptide->at(mid).id.compare(peptideRef);
  while (i != 0){
    if (lower >= upper) return NULL;
    if (i>0){
      if (mid == 0) return NULL;
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return NULL;
    i = peptide->at(mid).id.compare(peptideRef);
  }
  //cout << "found " << peptide->at(mid).id << " at " << mid <<  " " << peptide->at(mid).peptideSequence.text << endl;
  return &peptide->at(mid);
}

CPeptideEvidence CSequenceCollection::getPeptideEvidence(string& id){
  if (sortPeptideEvidence) doPeptideEvidenceSort();

  CPeptideEvidence blank;
  size_t sz = peptideEvidence->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  i = peptideEvidence->at(mid).id.compare(id);
  while (i != 0){
    if (lower >= upper) return blank;
    if (i>0){
      if (mid == 0) return blank;
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return blank;
    i = peptideEvidence->at(mid).id.compare(id);
  }
  return peptideEvidence->at(mid);

}

string CSequenceCollection::getPeptideEvidenceFromPeptideAndProtein(CPeptide& p, string dbSequenceRef){
  if (sortPeptideSeq)  doPeptideSeqSort();
  size_t sz = peptide->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;
  string peptideRef;

  //find correct peptide sequence
  i = peptide->at(mid).peptideSequence.text.compare(p.peptideSequence.text);
  while (i != 0){
    if (lower >= upper) return "";
    if (i>0){
      if (mid == 0) return "";
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return "";
    i = peptide->at(mid).peptideSequence.text.compare(p.peptideSequence.text);
  }

  //check this, and neighbors for correct(ish) modifications
  peptideRef.clear();
  if (peptide->at(mid).compareModsSoft(p)){
    peptideRef = peptide->at(mid).id;
  }
  if (peptideRef.size()==0){
    i = (int)mid-1;
    while (i > -1 && peptide->at(i).peptideSequence.text.compare(p.peptideSequence.text) == 0){
      if (peptide->at(i).compareModsSoft(p)){
        peptideRef = peptide->at(i).id;
        break;
      }
      i--;
    }
  }
  if (peptideRef.size() == 0){
    i = (int)mid +1;
    while (i < (int)peptide->size() && peptide->at(i).peptideSequence.text.compare(p.peptideSequence.text) == 0){
      if (peptide->at(i).compareModsSoft(p)){
        peptideRef = peptide->at(i).id;
        break;
      }
      i++;
    }
  }
  if (peptideRef.size() == 0) return "";

  //iterate peptideEvidence for peptide and protein references
  if (sortPeptideEvidencePepRef)  doPeptideEvidencePepRefSort();
  sz = peptideEvidence->size();
  lower = 0;
  mid = sz / 2;
  upper = sz;

  i = peptideEvidence->at(mid).peptideRef.compare(peptideRef);
  while (i != 0){
    if (lower >= upper) return "";
    if (i>0){
      if (mid == 0) return "";
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return "";
    i = peptideEvidence->at(mid).peptideRef.compare(peptideRef);
  }

  //check all evidences that have this peptide for the requested protein
  if (peptideEvidence->at(mid).dbSequenceRef.compare(dbSequenceRef) == 0) return peptideEvidence->at(mid).id;
  i = (int)mid - 1;
  while (i > -1 && peptideEvidence->at(i).peptideRef.compare(peptideRef) == 0){
    if (peptideEvidence->at(i).dbSequenceRef.compare(dbSequenceRef)==0){
      return peptideEvidence->at(i).id;
    }
    i--;
  }
  i = (int)mid + 1;
  while (i < (int)peptideEvidence->size() && peptideEvidence->at(i).peptideRef.compare(peptideRef) == 0){
    if (peptideEvidence->at(i).dbSequenceRef.compare(dbSequenceRef)==0){
      return peptideEvidence->at(i).id;
    }
    i++;
  }
  return "";
}

//binary searches for the peptide by reference. If the peptide list is out of order, it gets sorted NOW.
string CSequenceCollection::getProtein(sPeptideEvidenceRef& s){
  if (sortPeptideEvidence) doPeptideEvidenceSort();
  size_t sz = peptideEvidence->size();
  size_t lower = 0;
  size_t mid = sz / 2;
  size_t upper = sz;
  int i;

  i = peptideEvidence->at(mid).id.compare(s.peptideEvidenceRef);
  while (i != 0){
    if (lower >= upper) return "";
    if (i>0){
      if (mid == 0) return "";
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return "";
    i = peptideEvidence->at(mid).id.compare(s.peptideEvidenceRef);
  }
  string st=peptideEvidence->at(mid).dbSequenceRef;

  if (sortDBSequence) doDBSequenceSort();
  sz = dbSequence->size();
  lower = 0;
  mid = sz / 2;
  upper = sz;
  i;

  i = dbSequence->at(mid).id.compare(st);
  while (i != 0){
    if (lower >= upper) return "";
    if (i>0){
      if (mid == 0) return "";
      upper = mid - 1;
      mid = (lower + upper) / 2;
    } else {
      lower = mid + 1;
      mid = (lower + upper) / 2;
    }
    if (mid == sz) return "";
    i = dbSequence->at(mid).id.compare(st);
  }
  return dbSequence->at(mid).accession;


}

void CSequenceCollection::rebuildPepEvTable(){
  sPepTable pt;
  vPepEvTable.clear();
  for (size_t i = 0; i < peptideEvidence->size(); i++){
    pt.index = i;
    pt.seq = peptideEvidence->at(i).peptideRef;
    vPepEvTable.push_back(pt);
  }
  sort(vPepEvTable.begin(), vPepEvTable.end(), comparePeptideTable);
}

void CSequenceCollection::rebuildPepTable(){
  sPepTable pt;
  vPepTable.clear();
  for (size_t i = 0; i < peptide->size(); i++){
    pt.index = i;
    pt.seq = peptide->at(i).peptideSequence.text;
    vPepTable.push_back(pt);
  }
  sort(vPepTable.begin(), vPepTable.end(), comparePeptideTable);
}

void CSequenceCollection::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SequenceCollection>\n");

  if (sortDBSequence) doDBSequenceSort();
  if (sortPeptide) doPeptideSort(); //always sort first. Saves trouble later.
  if (sortPeptideEvidence) doPeptideEvidenceSort();

  if (tabs>-1) {
    for (j = 0; j < dbSequence->size(); j++) dbSequence->at(j).writeOut(f,tabs+1);
    for (j = 0; j < peptide->size(); j++) peptide->at(j).writeOut(f, tabs + 1);
    for (j = 0; j < peptideEvidence->size(); j++) peptideEvidence->at(j).writeOut(f, tabs + 1);
  } else {
    for (j = 0; j < dbSequence->size(); j++) dbSequence->at(j).writeOut(f);
    for (j = 0; j < peptide->size(); j++) peptide->at(j).writeOut(f);
    for (j = 0; j < peptideEvidence->size(); j++) peptideEvidence->at(j).writeOut(f);
  }

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


