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

#include "CEnzymes.h"

using namespace std;

CEnzymes::CEnzymes(){
  independent=false;
}

//CEnzymes::CEnzymes(const CEnzymes& c){
//  independent=c.independent;
//  enzyme = new vector<CEnzyme>(*c.enzyme);
//}
//
//CEnzymes::~CEnzymes(){
//  delete enzyme;
//}
//
//CEnzymes& CEnzymes::operator=(const CEnzymes& c){
//  if(this!=&c){
//    independent = c.independent;
//    delete enzyme;
//    enzyme = new vector<CEnzyme>(*c.enzyme);
//  }
//  return *this;
//}

void CEnzymes::writeOut(FILE* f, int tabs){
  if (enzyme.empty()){
    cerr << "Enzymes::Enzyme is required." << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<Enzymes");
  if (independent) fprintf(f, " independent=\"true\"");
  fprintf(f, ">\n");

  int t = tabs;
  if (t>-1)t++;

  for(size_t j=0;j<enzyme.size();j++) enzyme[j].writeOut(f,t);

  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</Enzymes>\n");

}
