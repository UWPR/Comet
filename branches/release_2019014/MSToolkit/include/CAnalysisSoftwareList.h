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

#ifndef _CANALYSISSOFTWARELIST_H
#define _CANALYSISSOFTWARELIST_H

#include "CAnalysisSoftware.h"
#include <string>
#include <vector>

class CAnalysisSoftwareList {
public:

  //Constructors & Destructor
  CAnalysisSoftwareList();
  ~CAnalysisSoftwareList();

  //Data members
  std::vector<CAnalysisSoftware>* analysisSoftware;

  //operators
  CAnalysisSoftware& operator[](const size_t& index);

  //Functions
  void addAnalysisSoftware(CAnalysisSoftware& as);
  void clear();
  void writeOut(FILE* f, int tabs = -1);

private:
};

#endif