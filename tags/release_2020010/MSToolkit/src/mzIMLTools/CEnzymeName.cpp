#include "CEnzymeName.h"

using namespace std;

CEnzymeName::CEnzymeName(){
  cvParam=new vector<sCvParam>;
  sCvParam c;
  cvParam->push_back(c);

  sUserParam u;
  userParam=new vector<sUserParam>;
  userParam->push_back(u);
}

CEnzymeName::CEnzymeName(const CEnzymeName& c){
  cvParam=new vector<sCvParam>(*c.cvParam);
  userParam=new vector<sUserParam>(*c.userParam);
}

CEnzymeName::~CEnzymeName(){
  delete cvParam;
  delete userParam;
}

//Operators
CEnzymeName& CEnzymeName::operator=(const CEnzymeName& c){
  if(this!=&c){
    delete cvParam;
    delete userParam;
    cvParam = new vector<sCvParam>(*c.cvParam);
    userParam = new vector<sUserParam>(*c.userParam);
  }
  return *this;
}

//Functions
void CEnzymeName::writeOut(FILE* f, int tabs){
  int i,j;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<EnzymeName>\n");

  if (cvParam->at(0).accession.compare("null") != 0) {
    if(tabs>-1){
      for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f, tabs + 1);
    } else {
      for (j = 0; j<cvParam->size(); j++) cvParam->at(j).writeOut(f);
    }
  }

  if (userParam->at(0).name.compare("null") != 0) {
    if (tabs>-1){
      for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f, tabs + 1);
    } else {
      for (j = 0; j<userParam->size(); j++) userParam->at(j).writeOut(f);
    }
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</EnzymeName>\n");
}
