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

#include "CExclude.h"

using namespace std;

void CExclude::writeOut(FILE* f, int tabs){
  if (cvParam.empty() && userParam.empty()) {
    cerr << "Exclude::cvParam or Exclude::userParam required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Exclude>\n");

  int t = tabs;
  if (t>-1) t++;

  for (size_t j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, t);
  for (size_t j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Exclude>\n");
}
