#include "CEnzyme.h"

using namespace std;

CEnzyme::CEnzyme(){
  cTermGain.clear();
  id="null";
  minDistance=0;
  missedCleavages=-1;
  nTermGain.clear();
  name.clear();
  semiSpecific=false;
}

void CEnzyme::writeOut(FILE* f, int tabs){
  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Enzyme id=\"%s\"",id.c_str());
  if(cTermGain.size()>0) fprintf(f," cTermGain=\"%s\"",cTermGain.c_str());
  if (minDistance>0) fprintf(f, " minDistance=\"%d\"", minDistance);
  if (missedCleavages>-1) fprintf(f, " missedCleavages=\"%d\"", missedCleavages);
  if (nTermGain.size()>0) fprintf(f, " nTermGain=\"%s\"", nTermGain.c_str());
  if (name.size()>0) fprintf(f, " name=\"%s\"", name.c_str());
  if (semiSpecific) fprintf(f, " semiSpecific=\"true\"");
  fprintf(f,">\n");

  if (enzymeName.cvParam->at(0).accession.compare("null") != 0 || enzymeName.userParam->at(0).name.compare("null") != 0){
    if(tabs>-1) enzymeName.writeOut(f,tabs+1);
    else enzymeName.writeOut(f);
  }

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</EnzymeName>\n");
}