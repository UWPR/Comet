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

#include "CSpectrumIdentificationProtocol.h"

using namespace std;

void CSpectrumIdentificationProtocol::writeOut(FILE* f, int tabs){
  if (analysisSoftwareRef.empty()){
    cerr << "SpectrumIdentificationProtocol::analysisSoftware_ref is required." << endl;
    exit(69);
  }
  if (id.empty()){
    cerr << "SpectrumIdentificationProtocol::id is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectrumIdentificationProtocol id=\"%s\" analysisSoftware_ref=\"%s\"", &id[0], &analysisSoftwareRef[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  int t=tabs;
  if(t>-1) t++;

  size_t j;
  searchType.writeOut(f,t);
  for (j = 0; j<additionalSearchParams.size();j++) additionalSearchParams[j].writeOut(f, t);
  for (j = 0; j<modificationParams.size(); j++) modificationParams[j].writeOut(f, t);
  for (j = 0; j<enzymes.size(); j++)enzymes[j].writeOut(f, t);
  for(j=0;j<fragmentTolerance.size();j++) fragmentTolerance[j].writeOut(f,t);
  for(j=0;j<parentTolerance.size();j++) parentTolerance[j].writeOut(f,t);
  threshold.writeOut(f,t);
  for (j = 0; j<databaseFilters.size(); j++) databaseFilters[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectrumIdentificationProtocol>\n");

}
