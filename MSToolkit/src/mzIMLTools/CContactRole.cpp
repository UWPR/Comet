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

#include "CContactRole.h"

CContactRole::CContactRole(){
  contactRef = "null";
}

void CContactRole::writeOut(FILE* f, int tabs){
  if (contactRef.compare("null")==0) return;
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ContactRole contact_ref=\"%s\">\n",&contactRef[0]);
  if (tabs>-1) role.writeOut(f, tabs + 1);
  else role.writeOut(f);
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ContactRole>\n");
}
