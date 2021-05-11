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

#include "CSearchDatabase.h"

using namespace std;

CSearchDatabase::CSearchDatabase(){
  id = "null";
  location = "null";
  name.clear();
  numDatabaseSequences=0;
  numResidues=0;
  releaseDate.clear();
  version.clear();

  cvParam = new vector<sCvParam>;
}

CSearchDatabase::CSearchDatabase(const CSearchDatabase& c){
  id=c.id;
  location=c.location;
  name=c.name;
  numDatabaseSequences=c.numDatabaseSequences;
  numResidues=c.numResidues;
  releaseDate=c.releaseDate;
  version=c.version;

  externalFormatDocumentation=c.externalFormatDocumentation;
  fileFormat=c.fileFormat;
  databaseName=c.databaseName;

  cvParam = new vector<sCvParam>;
  for (size_t i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
}

CSearchDatabase::~CSearchDatabase(){
  delete cvParam;
}

CSearchDatabase& CSearchDatabase::operator=(const CSearchDatabase& c){
  if (this != &c){
    id = c.id;
    location = c.location;
    name = c.name;
    numDatabaseSequences = c.numDatabaseSequences;
    numResidues = c.numResidues;
    releaseDate = c.releaseDate;
    version = c.version;

    externalFormatDocumentation = c.externalFormatDocumentation;
    fileFormat = c.fileFormat;
    databaseName = c.databaseName;

    delete cvParam;
    cvParam = new vector<sCvParam>;
    for (size_t i = 0; i<c.cvParam->size(); i++) cvParam->push_back(c.cvParam->at(i));
  }
  return *this;
}

void CSearchDatabase::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<SearchDatabase location=\"%s\" id=\"%s\"", &location[0],&id[0]);
  if(name.size()>0) fprintf(f," name=\"%s\"",&name[0]);
  if (numDatabaseSequences>0) fprintf(f, " numDatabaseSequences=\"%d\"", numDatabaseSequences);
  if (numResidues>0) fprintf(f, " numResidues=\"%d\"", numResidues);
  if (releaseDate.size()>0) fprintf(f, " releaseDate=\"%s\"", &releaseDate[0]);
  if (version.size()>0) fprintf(f, " version=\"%s\"", &version[0]);
  fprintf(f,">\n");
  if(tabs>-1) {
    fileFormat.writeOut(f,tabs+1);
    databaseName.writeOut(f,tabs+1);
  } else {
    fileFormat.writeOut(f);
    databaseName.writeOut(f);
  }
  for (j = 0; j<cvParam->size(); j++){
    if (tabs>-1) cvParam->at(j).writeOut(f, tabs + 1);
    else cvParam->at(j).writeOut(f);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</SearchDatabase>\n");
}
