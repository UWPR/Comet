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

#include "CProvider.h"

using namespace std;

void CProvider::writeOut(FILE* f, int tabs){
  if(id.empty()){
    cerr << "Provider::id required" << endl;
    exit(69);
  }
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Provider id=\"%s\"", &id[0]);
  if(analysisSoftwareRef.size()>0) fprintf(f, " analysisSoftware_ref=\"%s\"",analysisSoftwareRef.c_str());
  if (name.size()>0) fprintf(f, " name=\"%s\"", name.c_str());
  fprintf(f, ">\n");
  if (tabs>-1) {
    for(j=0;j<contactRole.size();j++) contactRole[j].writeOut(f, tabs + 1);
  }  else {
    for (j = 0; j<contactRole.size(); j++) contactRole[j].writeOut(f, tabs);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Provider>\n");
}
