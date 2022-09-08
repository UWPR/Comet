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

#include "CDatabaseFilters.h"

using namespace std;

void CDatabaseFilters::writeOut(FILE* f, int tabs){
  if (filter.empty()) {
    cerr << "DatabaseFilters::Filter is required." << endl;
    exit(69);
  }
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<DatabaseFilters>\n");

  int t = tabs;
  if (t>-1) t++;
  for(size_t j=0;j<filter.size();j++) filter[j].writeOut(f, t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</DatabaseFilters>\n");
}
