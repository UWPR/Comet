// Copyright 2023 Jimmy Eng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef _COMETWRITETXT_
#define _COMETWRITETXT_


class CometWriteTxt
{
public:
   CometWriteTxt();
   ~CometWriteTxt();
   static void WriteTxt(FILE *fpout,
                        FILE *fpoutd,
                        FILE *fpdb);

   static void PrintTxtHeader(FILE *fpout);
   static void PrintModifications(FILE *fpout,
                                  Results *pOutput,
                                  int iWhichResult);
   static void PrintProteins(FILE *fpout,
                             FILE *fpdb,
                             int iWhichQuery,
                             int iWhichResult,
                             int iPrintTargetDecoy,
                             size_t *iNumProteins);

private:
   static void PrintResults(int iWhichQuery,
                            int iPrintTargetDecoy,
                            FILE *fpOut,
                            FILE *fpdb);
};

#endif
