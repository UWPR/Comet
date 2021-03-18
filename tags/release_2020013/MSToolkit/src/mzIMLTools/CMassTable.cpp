#include "CMassTable.h"

using namespace std;

CMassTable::CMassTable(){
  id.clear();
  name.clear();

  msLevel=new vector<int>;
  cvParam=new vector<sCvParam>;
  residue=new vector<CResidue>;
  userParam=new vector<sUserParam>;
}

CMassTable::CMassTable(const CMassTable& m){
  id=m.id;
  name=m.name;
  msLevel=new vector<int>(*m.msLevel);
  cvParam = new vector<sCvParam>(*m.cvParam);
  residue = new vector<CResidue>(*m.residue);
  userParam = new vector<sUserParam>(*m.userParam);
}

CMassTable::~CMassTable(){
  delete msLevel;
  delete cvParam;
  delete residue;
  delete userParam;
}

CMassTable& CMassTable::operator=(const CMassTable& c){
  if(this!=&c){
    id = c.id;
    name = c.name;
    delete msLevel;
    delete cvParam;
    delete residue;
    delete userParam;
    msLevel = new vector<int>(*c.msLevel);
    cvParam = new vector<sCvParam>(*c.cvParam);
    residue = new vector<CResidue>(*c.residue);
    userParam = new vector<sUserParam>(*c.userParam);
  }
  return *this;
}

void CMassTable::writeOut(FILE* f, int tabs){
  int i;
  size_t j;
  string msl;
  char sMSL[8];
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<MassTable id=\"%s\" ",id.c_str());
  for(j=0;j<msLevel->size();j++){
    if(j>0) msl+=' ';
    sprintf(sMSL,"%d",msLevel->at(j));
    msl+=sMSL;
  }
  fprintf(f, "msLevel=\"%s\"", msl.c_str());
  if(name.size()>0) fprintf(f, " name=\"%s\"",name.c_str());
  fprintf(f,">\n");
 
  for (j = 0; j<residue->size(); j++){
    if (tabs>-1) residue->at(j).writeOut(f, tabs + 1);
    else residue->at(j).writeOut(f);
  }
  for (j = 0; j<cvParam->size(); j++){
    if (tabs>-1) cvParam->at(j).writeOut(f, tabs + 1);
    else cvParam->at(j).writeOut(f);
  }
  for (j = 0; j<userParam->size(); j++){
    if (tabs>-1) userParam->at(j).writeOut(f, tabs + 1);
    else userParam->at(j).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</MassTable>\n");
}