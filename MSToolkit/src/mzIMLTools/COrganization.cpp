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

#include "COrganization.h"

using namespace std;

void COrganization::writeOut(FILE* f, int tabs){
  if (id.empty()){
    cerr << "Organization::id is required." << endl;
    exit(69);
  }
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Organization id=\"%s\"", id.c_str());
  if (!name.empty()) fprintf(f, " name=\"%s\"", name.c_str());
  fprintf(f, ">\n");

  if (tabs>-1){
    if(!parent.organizationRef.empty()) parent.writeOut(f, tabs + 1);
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs + 1);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, tabs + 1);
  } else {
    if (!parent.organizationRef.empty()) parent.writeOut(f, tabs);
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, tabs);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Organization>\n");
}
