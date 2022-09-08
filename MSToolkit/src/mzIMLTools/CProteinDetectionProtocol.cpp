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

#include "CProteinDetectionProtocol.h"

using namespace std;

//CProteinDetectionProtocol::CProteinDetectionProtocol(){
//  analysisSoftwareRef = "null";
//  id = "null";
//  name.clear();
//}

void CProteinDetectionProtocol::writeOut(FILE* f, int tabs){
  if (analysisSoftwareRef.empty()){
    cerr << "ProteinDetectionProtocol::analysisSoftware_ref is required." << endl;
    exit(69);
  }
  if (id.empty()){
    cerr << "ProteinDetectionProtocol::id is required." << endl;
    exit(69);
  }

  int i;

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<ProteinDetectionProtocol id=\"%s\" analysisSoftware_ref=\"%s\"",&id[0],&analysisSoftwareRef[0]);
  if (!name.empty()) fprintf(f, " name=\"%s\"", &name[0]);
  fprintf(f, ">\n");

  int t = tabs;
  if (t>-1)t++;

  for(size_t j=0;j<analysisParams.size();j++) analysisParams[j].writeOut(f,t);
  threshold.writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</ProteinDetectionProtocol>\n");
}