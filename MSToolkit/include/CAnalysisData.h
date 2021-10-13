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

#ifndef _CANALYSISDATA_H
#define _CANALYSISDATA_H

#include "CProteinDetectionList.h"
#include "CSpectrumIdentificationList.h"
#include <algorithm>
#include <vector>

class CAnalysisData {
public:

  //Constructors & Destructor
  CAnalysisData();
  ~CAnalysisData();

  //Data members
  std::vector<CSpectrumIdentificationList>* spectrumIdentificationList;
  CProteinDetectionList proteinDetectionList;
  std::vector<sXRefSIIPE>* peptideEvidenceTable;

  //Functions
  std::string addSpectrumIdentificationList();
  std::string addSpectrumIdentificationList(CSpectrumIdentificationList& c);
  void buildPeptideEvidenceTable();
  void getSpectrumIdentificationItems(std::string pe, int charge, std::vector<std::string>& vSII);
  void writeOut(FILE* f, int tabs = -1);

private:

  static bool comparePeptideEvidenceRef(const sXRefSIIPE& a, const sXRefSIIPE& b);

};

#endif
