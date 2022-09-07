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

#include "CBibliographicReference.h"

using namespace std;

void CBibliographicReference::writeOut(FILE* f, int tabs){
  for (int i = 0; i<tabs; i++) fprintf(f, " ");
  fprintf(f, "<BibliograpicReference id=\"%s\"",id.c_str());
  if(!authors.empty()) fprintf(f," authors=\"%s\"",authors.c_str());
  if (!doi.empty()) fprintf(f, " doi=\"%s\"", doi.c_str());
  if (!editor.empty()) fprintf(f, " editor=\"%s\"", editor.c_str());
  if (!issue.empty()) fprintf(f, " issue=\"%s\"", issue.c_str());
  if (!name.empty()) fprintf(f, " name=\"%s\"", name.c_str());
  if (!pages.empty()) fprintf(f, " pages=\"%s\"", pages.c_str());
  if (!publication.empty()) fprintf(f, " publication=\"%s\"", publication.c_str());
  if (!publisher.empty()) fprintf(f, " publisher=\"%s\"", publisher.c_str());
  if (!title.empty()) fprintf(f, " title=\"%s\"", title.c_str());
  if (!volume.empty()) fprintf(f, " volume=\"%s\"", volume.c_str());
  if (!year.empty()) fprintf(f, " year=\"%s\"", year.c_str());
  fprintf(f,"/>\n");
}
