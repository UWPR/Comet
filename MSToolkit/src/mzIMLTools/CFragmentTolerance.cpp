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

#include "CFragmentTolerance.h"

using namespace std;

void CFragmentTolerance::writeOut(FILE* f, int tabs){

  //check for valid entry per spec version 1.2.0
  size_t j;
  bool b1001412=false;
  bool b1001413=false;
  for(j=0;j<cvParam.size();j++){
    if(cvParam[j].accession.compare("MS:1001412")==0){
      if(!b1001412) b1001412=true;
      else {
        cerr << "Multiple FragmentTolerance::cvParam::accession::MS:1001412 detected." << endl;
        exit(69);
      }
    } else if (cvParam[j].accession.compare("MS:1001413") == 0){
      if (!b1001413) b1001413 = true;
      else {
        cerr << "Multiple FragmentTolerance::cvParam::accession::MS:1001413 detected." << endl;
        exit(69);
      }
    }
  }
  if(!b1001412 || !b1001413){
    cerr << "FragmentTolerance::cvParam requires both accession::MS:1001412 and accession::MS:1001413" << endl;
    exit(69);
  }

  int i;
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<FragmentTolerance>\n");
  if (tabs>-1){
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs + 1);
  } else {
    for (j = 0; j<cvParam.size(); j++) cvParam[j].writeOut(f, tabs);
  }
  for (i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "</FragmentTolerance>\n");
}
