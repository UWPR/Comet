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

#ifndef _CINPUTS_H
#define _CINPUTS_H

#include "CSearchDatabase.h"
#include "CSourceFile.h"
#include "CSpectraData.h"
#include <string>
#include <vector>

class CInputs {
public:

  //Constructors & Destructor
  CInputs();
  ~CInputs();

  //Data members
  std::vector<CSourceFile>* sourceFile;
  std::vector<CSearchDatabase>* searchDatabase;
  std::vector<CSpectraData>* spectraData;

  //Functions
  std::string addSearchDatabase(std::string& sd);
  std::string addSpectraData(std::string& sd);
  std::string addSpectraData(CSpectraData& c);
  sCvParam checkFileFormat(std::string s);
  sCvParam checkSpectrumIDFormat(sCvParam& s);
  void writeOut(FILE* f, int tabs = -1);

private:
};

#endif
