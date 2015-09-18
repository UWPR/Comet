/*
   Copyright 2012 University of Washington

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

#ifndef _COMETWRITESQT_
#define _COMETWRITESQT_


class CometWriteSqt
{
public:
   CometWriteSqt();
   ~CometWriteSqt();

   static void WriteSqt(FILE *fpout,
                        FILE *fpoutd);

   static void PrintSqtHeader(FILE *fpout,
                              CometSearchManager &searchMgr);

private:
   static void PrintResults(int iWhichQuery,
                            bool bDecoy,
                            FILE *fpOut);
   static void PrintSqtLine(int iRankXcorr,
                            int iWhichResult,
                            Results *pOutput,
                            FILE *fpOut);
};

#endif
