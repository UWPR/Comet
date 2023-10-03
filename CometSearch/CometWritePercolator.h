/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _COMETWRITEPERCOLATOR_
#define _COMETWRITEPERCOLATOR_


class CometWritePercolator
{
public:
   CometWritePercolator();
   ~CometWritePercolator();
   static void WritePercolatorHeader(FILE *fpout);
   static bool WritePercolator(FILE *fpout,
                               FILE *fpdb);


private:
   static bool PrintResults(int iWhichQuery,
                            FILE *fpOut,
                            FILE *fpdb,
                            int iPrintTargetDecoy,
                            int iLenDecoyPrefix);
   static void PrintPercolatorSearchHit(int iWhichQuery,
                                    int iWhichResult,
                                    int iPrintTargetDecoy,
                                    Results *pOutput,
                                    FILE *fpOut,
                                    vector<string> vProteinTargets,
                                    vector<string> vProteinDecoys);
   static void CalcNTTNMC(Results *pOutput,
                          int iWhichQuery,
                          int *iNterm,
                          int *iCterm,
                          int *iNMC);
};

#endif
