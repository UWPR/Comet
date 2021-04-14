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

#ifndef _CSEQUENCECOLLECTION_H
#define _CSEQUENCECOLLECTION_H

#include "CDBSequence.h"
#include "CPeptide.h"
#include "CPeptideEvidence.h"
#include <algorithm>
#include <vector>
#include <iostream>

class CSequenceCollection{
public:

  //Constructors & Destructors
  CSequenceCollection();
  ~CSequenceCollection();

  //Data members
  std::vector<CDBSequence>* dbSequence;
  std::vector<CPeptide>* peptide;
  std::vector<CPeptideEvidence>* peptideEvidence;

  //Functions
  std::string addDBSequence(CDBSequence& dbs);
  std::string addPeptide(CPeptide& p);
  sPeptideEvidenceRef addPeptideEvidence(CPeptideEvidence& p);
  bool addXLPeptides(std::string ID, CPeptide& p1, CPeptide& p2, std::string& ref1, std::string& ref2, std::string& value);
  CDBSequence  getDBSequence(std::string id);
  CDBSequence* getDBSequenceByAcc(std::string acc);
  CPeptide* getPeptide(std::string peptideRef);
  CPeptideEvidence  getPeptideEvidence(std::string& id);
  std::string getPeptideEvidenceFromPeptideAndProtein(CPeptide& p, std::string dbSequenceRef);
  std::string getProtein(sPeptideEvidenceRef& s);
  void writeOut(FILE* f, int tabs = -1);

private:
  //Data members
  bool sortDBSequence;
  bool sortDBSequenceAcc;
  bool sortPeptide;
  bool sortPeptideSeq;
  bool sortPeptideEvidence;
  bool sortPeptideEvidencePepRef;
  std::vector<sPepTable> vPepEvTable;
  std::vector<sPepTable> vPepTable;
  std::vector<sXLPepTable> vXLPepTable;

  //Functions
  void doDBSequenceSort();
  void doDBSequenceSortAcc();
  void doPeptideSort();
  void doPeptideSeqSort();
  void doPeptideEvidenceSort();
  void doPeptideEvidencePepRefSort();
  void rebuildPepEvTable();
  void rebuildPepTable();

  //Sorting Functions
  static bool compareDBSequence(const CDBSequence& a, const CDBSequence& b);
  static bool compareDBSequenceAcc(const CDBSequence& a, const CDBSequence& b);
  static bool comparePeptide(const CPeptide& a, const CPeptide& b);
  static bool comparePeptideSeq(const CPeptide& a, const CPeptide& b);
  static bool comparePeptideTable(const sPepTable& a, const sPepTable& b);
  static bool comparePeptideEvidence(const CPeptideEvidence& a, const CPeptideEvidence& b);
  static bool comparePeptideEvidencePepRef(const CPeptideEvidence& a, const CPeptideEvidence& b);
  static bool compareXLPeptideTable(const sXLPepTable& a, const sXLPepTable& b);
};

#endif
