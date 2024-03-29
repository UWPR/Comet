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


#ifndef _COMETWRITEOUT_
#define _COMETWRITEOUT_


class CometWriteOut
{
public:
   CometWriteOut();
   ~CometWriteOut();
   static bool WriteOut(FILE *fpdb);

private:
   static float FindSpScore(Query *pQuery,
                            int bin);
   static bool PrintResults(int iWhichQuery,
                            bool bDecoySearch,
                            FILE *fpdb);
   static void PrintOutputLine(int iLenMaxDuplicates,
                               int iMaxWidthReference,
                               int iWhichResult,
                               bool bDecoySearch,
                               Results *pOutput,
                               FILE *fpout,
                               FILE *fpdb);
   static void PrintIons(int iWhichQuery,
                         FILE *fpout);
};

#endif
