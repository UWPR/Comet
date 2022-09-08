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

#include "CFragmentArray.h"

using namespace std;

void CFragmentArray::writeOut(FILE* f, int tabs){
  if (measureRef.empty()){
    cerr << "FragmentArray::measure_ref required." << endl;
    exit(69);
  }
  if (values.empty()){
    cerr << "FragmentArray::values required." << endl;
    exit(69);
  }

  for (int i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<FragmentArray measure_ref=\"%s\" values=\"%s\" />\n", measureRef.c_str(),values.c_str());

}
