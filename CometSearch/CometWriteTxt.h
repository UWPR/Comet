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
