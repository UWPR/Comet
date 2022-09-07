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

#include "CSourceFile.h"

using namespace std;

//CSourceFile::CSourceFile(){
//  id = "null";
//  location = "null";
//  name.clear();
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//}
//
//CSourceFile::CSourceFile(const CSourceFile& s){
//  id = s.id;
//  location = s.location;
//  name = s.name;
//
//  externalFormatDocumentation = s.externalFormatDocumentation;
//  fileFormat = s.fileFormat;
//
//  size_t i;
//  cvParam = new vector<sCvParam>;
//  userParam = new vector<sUserParam>;
//  for (i = 0; i<s.cvParam->size(); i++) cvParam->push_back(s.cvParam->at(i));
//  for (i = 0; i<s.userParam->size(); i++) userParam->push_back(s.userParam->at(i));
//}
//
//CSourceFile::~CSourceFile(){
//  delete cvParam;
//  delete userParam;
//}
//
//CSourceFile& CSourceFile::operator=(const CSourceFile& s){
//  if (this != &s){
//    id = s.id;
//    location = s.location;
//    name = s.name;
//
//    externalFormatDocumentation = s.externalFormatDocumentation;
//    fileFormat = s.fileFormat;
//
//    size_t i;
//    delete cvParam;
//    delete userParam;
//    cvParam = new vector<sCvParam>;
//    userParam = new vector<sUserParam>;
//    for (i = 0; i<s.cvParam->size(); i++) cvParam->push_back(s.cvParam->at(i));
//    for (i = 0; i<s.userParam->size(); i++) userParam->push_back(s.userParam->at(i));
//  }
//  return *this;
//}

void CSourceFile::writeOut(FILE* f, int tabs){
  if(id.empty()){
    cerr << "SourceFile::id required." << endl;
    exit(69);
  }
  if (location.empty()){
    cerr << "SourceFile::location required." << endl;
    exit(69);
  }

  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SourceFile location=\"%s\" id=\"%s\">\n",location.c_str(),id.c_str());
  if (tabs>-1) {
    for (j = 0; j<externalFormatDocumentation.size(); j++) externalFormatDocumentation[j].writeOut(f, tabs + 1);
    fileFormat.writeOut(f,tabs+1);
    for(j=0;j<cvParam.size();j++) cvParam[j].writeOut(f,tabs+1);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<externalFormatDocumentation.size(); j++) externalFormatDocumentation[j].writeOut(f);
    fileFormat.writeOut(f);
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f);
    for (j = 0; j<userParam.size(); j++) userParam[j].writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SourceFile>\n");
}
