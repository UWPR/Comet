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

#include "CEnzyme.h"

using namespace std;

CEnzyme::CEnzyme(){
  minDistance=0;
  missedCleavages=-1;
  semiSpecific=false;
}

void CEnzyme::writeOut(FILE* f, int tabs){
  if (id.empty()){
    cerr << "Enzyme::id is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Enzyme id=\"%s\"",id.c_str());
  if(!cTermGain.empty()) fprintf(f," cTermGain=\"%s\"",cTermGain.c_str());
  if (minDistance>0) fprintf(f, " minDistance=\"%d\"", minDistance);
  if (missedCleavages>-1) fprintf(f, " missedCleavages=\"%d\"", missedCleavages);
  if (!nTermGain.empty()) fprintf(f, " nTermGain=\"%s\"", nTermGain.c_str());
  if (!name.empty()) fprintf(f, " name=\"%s\"", name.c_str());
  if (semiSpecific) fprintf(f, " semiSpecific=\"true\"");
  fprintf(f,">\n");

  int t = tabs;
  if (t>-1)t++;

  size_t j;
  for(j=0;j<siteRegexp.size();j++) siteRegexp[j].writeOut(f,t);
  for(j=0;j<enzymeName.size();j++) enzymeName[j].writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Enzyme>\n");
}