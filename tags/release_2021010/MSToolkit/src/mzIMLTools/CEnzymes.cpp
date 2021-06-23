#include "CEnzymes.h"

using namespace std;

CEnzymes::CEnzymes(){
  independent=false;
  enzyme=new vector<CEnzyme>;
  CEnzyme c;
  enzyme->push_back(c);
}

CEnzymes::CEnzymes(const CEnzymes& c){
  independent=c.independent;
  enzyme = new vector<CEnzyme>(*c.enzyme);
}

CEnzymes::~CEnzymes(){
  delete enzyme;
}

CEnzymes& CEnzymes::operator=(const CEnzymes& c){
  if(this!=&c){
    independent = c.independent;
    delete enzyme;
    enzyme = new vector<CEnzyme>(*c.enzyme);
  }
  return *this;
}

void CEnzymes::writeOut(FILE* f, int tabs){
  if(enzyme->at(0).id.compare("null")==0) return;

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Enzymes");
  if (independent) fprintf(f, " independent=\"true\"");
  fprintf(f, ">\n");

  if (tabs>-1) {
    for(i=0;i<(int)enzyme->size();i++) enzyme->at(i).writeOut(f,tabs+1);
  } else {
    for (i = 0; i<(int)enzyme->size(); i++) enzyme->at(i).writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Enzymes>\n");

}
