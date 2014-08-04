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

#ifndef _COMETWRITEPERCOLATOR_
#define _COMETWRITEPERCOLATOR_


class CometWritePercolator
{
public:
   CometWritePercolator();
   ~CometWritePercolator();
   static void WritePercolatorHeader(FILE *fpout);
   static bool WritePercolator(FILE *fpout);


private:
   static bool PrintResults(int iWhichQuery,
                            FILE *fpOut,
                            bool bDecoy);
   static void PrintPercolatorSearchHit(int iWhichQuery,
                                    int iWhichResult,
                                    bool bDecoy,
                                    Results *pOutput,
                                    FILE *fpOut,
                                    double dDeltaCn,
                                    double dLastDeltaCn);
   static void CalcNTTNMC(Results *pOutput,
                          int iWhichQuery,
                          int *iNterm,
                          int *iCterm,
                          int *iNMC);
};

#endif
