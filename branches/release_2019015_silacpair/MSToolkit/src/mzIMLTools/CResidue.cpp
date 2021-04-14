#include "CResidue.h"

using namespace std;

CResidue::CResidue(){
  code=' ';
  mass=0;
}

void CResidue::writeOut(FILE* f, int tabs){
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Residue code=\"%c\" mass=\"%.6f\" />\n", code,mass);
}