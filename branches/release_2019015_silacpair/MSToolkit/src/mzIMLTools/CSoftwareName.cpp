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

#include "CSoftwareName.h"

CSoftwareName::CSoftwareName(){
  cvParam.accession = "null";
  cvParam.cvRef = "null";
  cvParam.name = "null";
  cvParam.unitAccession.clear();
  cvParam.unitCvRef.clear();
  cvParam.unitName.clear();
  cvParam.value.clear();
  userParam.name.clear();
  userParam.type.clear();
  userParam.unitAccession.clear();
  userParam.unitCvRef.clear();
  userParam.unitName.clear();
  userParam.value.clear();
}

void CSoftwareName::writeOut(FILE* f, int tabs){
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SoftwareName>\n");
  if (tabs>-1) {
    if (userParam.name.size()>0) userParam.writeOut(f,tabs+1);
    else cvParam.writeOut(f,tabs+1);
  } else {
    if (userParam.name.size()>0) userParam.writeOut(f);
    else cvParam.writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SoftwareName>\n");
}
