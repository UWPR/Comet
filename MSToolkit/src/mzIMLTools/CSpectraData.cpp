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

#include "CSpectraData.h"

using namespace std;

//CSpectraData::CSpectraData(){
//  id = "null";
//  location = "null";
//  name.clear();
//}

void CSpectraData::writeOut(FILE* f, int tabs){
  if (id.empty()){
    cerr << "SpectraData::id is required." << endl;
    exit(69);
  }
  if (location.empty()){
    cerr << "SpectraData::location is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SpectraData location=\"%s\" id=\"%s\"", location.c_str(), id.c_str());
  if(name.size()>0) fprintf(f," name=\"%s\"",name.c_str());
  fprintf(f,">\n");

  int t = tabs;
  if (t>-1) t++;

  if(!externalFormatDocumentation.text.empty()) externalFormatDocumentation.writeOut(f,t);
  fileFormat.writeOut(f,t);
  spectrumIDFormat.writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SpectraData>\n");
}
