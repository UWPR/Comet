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

#ifndef _CMZIDENTML_H
#define _CMZIDENTML_H

#include "CAnalysisCollection.h"
#include "CAnalysisProtocolCollection.h"
#include "CAnalysisSoftwareList.h"
#include "CCvList.h"
#include "CDataCollection.h"
#include "CPSM.h"
#include "CSequenceCollection.h"
#include "expat.h"
#include <ctime>
#include <string>

#define XMLCLASS		
#ifndef XML_STATIC
#define XML_STATIC	// to statically link the expat libraries
#endif

class CMzIdentML {
public:

  //Constructor & Destructor
  CMzIdentML();
  ~CMzIdentML();

  //Data members
  CCvList cvList;
  CAnalysisSoftwareList analysisSoftwareList;
  //CAnalysisSampleCollection analysisSampleCollection;
  CSequenceCollection sequenceCollection;
  CAnalysisCollection analysisCollection;
  CAnalysisProtocolCollection analysisProtocolCollection;
  CDataCollection dataCollection;

  std::string id;
  std::string name;

  //Functions
  std::string addAnalysisSoftware(std::string software, std::string version);
  std::string addDatabase(std::string s);
  std::string addDBSequence(std::string acc, std::string sdbRef, std::string desc = "");
  std::string addPeptide(std::string seq, std::vector<CModification>& mods);
  sPeptideEvidenceRef addPeptideEvidence(std::string dbRef, std::string pepRef);
  CProteinAmbiguityGroup* addProteinAmbiguityGroup();
  std::string addSpectraData(std::string s);
  CSpectrumIdentification* addSpectrumIdentification(std::string& spectraDataRef, std::string& searchDatabaseRef);
  bool addXLPeptides(std::string seq1, std::vector<CModification>& mods1, std::string& ref1, std::string seq2, std::vector<CModification>& mods2, std::string& ref2, std::string& value);
  void consolidateSpectrumIdentificationProtocol();
  CDBSequence getDBSequence(std::string acc);
  CPeptide getPeptide(std::string peptide_ref);
  CPSM getPSM(int index, int rank=1);
  int getPSMCount();
  CSpectrumIdentificationList* getSpectrumIdentificationList(std::string& spectrumIdentificationList_ref);
  CSpectrumIdentificationProtocol* getSpectrumIdentificationProtocol(std::string& spectrumIdentificationProtocol_ref);
  bool readFile(const char* fn);
  bool writeFile(const char* fn);
  
  //Functions for XML Parsing
  void characters(const XML_Char *s, int len);
  void endElement(const XML_Char *el);
  void startElement(const XML_Char *el, const XML_Char **attr);

protected:
  bool                killRead;
  XML_Parser				  parser;
  std::vector<mzidElement> activeEl;

  //Functions
  void processCvParam(sCvParam& cv);
  void processUserParam(sUserParam& u);

  //Functions for XML Parsing
  inline const char* getAttrValue(const char* name, const XML_Char **attr) {
    for (int i = 0; attr[i]; i += 2) {
      if (isAttr(name, attr[i])) return attr[i + 1];
    }
    return "";
  }
  inline bool isAttr(const char *n1, const XML_Char *n2) { return (strcmp(n1, n2) == 0); }
  inline bool isElement(const char *n1, const XML_Char *n2)	{ return (strcmp(n1, n2) == 0); }

private:
};

#endif