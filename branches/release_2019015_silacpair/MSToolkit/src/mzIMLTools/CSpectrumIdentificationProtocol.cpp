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

#include "CSpectrumIdentificationProtocol.h"

using namespace std;

CSpectrumIdentificationProtocol::CSpectrumIdentificationProtocol(){
  analysisSoftwareRef = "null";
  id = "null";
  name.clear();
  massTable = new vector<CMassTable>;
}

CSpectrumIdentificationProtocol::CSpectrumIdentificationProtocol(const CSpectrumIdentificationProtocol& c){
  analysisSoftwareRef = c.analysisSoftwareRef;
  id = c.id;
  name=c.name;

  massTable = new vector<CMassTable>(*c.massTable);

  searchType=c.searchType;
  additionalSearchParams=c.additionalSearchParams;
  modificationParams=c.modificationParams;
  threshold=c.threshold;
}

CSpectrumIdentificationProtocol::~CSpectrumIdentificationProtocol(){
  delete massTable;
}

CSpectrumIdentificationProtocol& CSpectrumIdentificationProtocol::operator=(const CSpectrumIdentificationProtocol& c){
  if(this!=&c){
    analysisSoftwareRef = c.analysisSoftwareRef;
    id = c.id;
    name = c.name;

    delete massTable;
    massTable = new vector<CMassTable>(*c.massTable);

    searchType = c.searchType;
    additionalSearchParams = c.additionalSearchParams;
    modificationParams = c.modificationParams;
    threshold = c.threshold;
  }
  return *this;
}

bool CSpectrumIdentificationProtocol::operator==(const CSpectrumIdentificationProtocol& c){
  if(this==&c) return true;
  if(analysisSoftwareRef.compare(c.analysisSoftwareRef)!=0) return false;
  //if(id.compare(c.id)!=0) return false;
  if(name.compare(c.name)!=0) return false;
  if(searchType!=c.searchType) return false;
  if(additionalSearchParams!=c.additionalSearchParams) return false;
  if(modificationParams!=c.modificationParams) return false;
  if(threshold!=c.threshold) return false;
  return true;
}

void CSpectrumIdentificationProtocol::writeOut(FILE* f, int tabs){

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectrumIdentificationProtocol id=\"%s\" analysisSoftware_ref=\"%s\"", &id[0], &analysisSoftwareRef[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  //size_t j;
  if (tabs>-1) {
    searchType.writeOut(f,tabs+1);
    additionalSearchParams.writeOut(f,tabs+1);
    modificationParams.writeOut(f,tabs+1);
    enzymes.writeOut(f,tabs+1);
    threshold.writeOut(f,tabs+1);
  } else {
    searchType.writeOut(f);
    additionalSearchParams.writeOut(f);
    modificationParams.writeOut(f);
    enzymes.writeOut(f);
    threshold.writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectrumIdentificationProtocol>\n");

}
