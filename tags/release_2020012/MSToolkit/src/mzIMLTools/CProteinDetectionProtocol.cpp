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

#include "CProteinDetectionProtocol.h"

CProteinDetectionProtocol::CProteinDetectionProtocol(){
  analysisSoftwareRef = "null";
  id = "null";
  name.clear();
}

void CProteinDetectionProtocol::writeOut(FILE* f, int tabs){
  int i;

  if (id.compare("null")==0) return;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ProteinDetectionProtocol id=\"%s\" analysisSoftware_ref=\"%s\"",&id[0],&analysisSoftwareRef[0]);
  if (name.size()>0) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  if (tabs > -1){
    if (analysisParams.cvParam->at(0).name.compare("null") != 0 || analysisParams.userParam->at(0).name.compare("null") != 0){
      analysisParams.writeOut(f,tabs+1);
    }
    threshold.writeOut(f, tabs + 1);
  } else {
    if (analysisParams.cvParam->at(0).name.compare("null") != 0 || analysisParams.userParam->at(0).name.compare("null") != 0){
      analysisParams.writeOut(f);
    }
    threshold.writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ProteinDetectionProtocol>\n");
}