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

#ifndef _COMETWRITEPEPXML_
#define _COMETWRITEPEPXML_


class CometWritePepXML
{
public:
   CometWritePepXML();
   ~CometWritePepXML();

   static bool WritePepXMLHeader(FILE *fpout,
                                 CometSearchManager &searchMgr);

   static void WritePepXML(FILE *fpout,
                           FILE *fpoutd,
                           FILE *fpdb,
                           int iNumSpectraSearched);

   static void WritePepXMLEndTags(FILE *fpout);

   static void ReadInstrument(char *szManufacturer,
                              char *szModel);

   static void CalcNTTNMC(Results *pOutput,
                          int iWhichQuery,
                          int *iNTT,
                          int *iNMC);


private:
   static void PrintResults(int iWhichQuery,
                            int iPrintTargetDecoy,
                            FILE *fpOut,
                            FILE *fpdb,
                            int iNumSpectraSearched);

   static void PrintPepXMLSearchHit(int iWhichQuery,
                                    int iWhichResult,
                                    int iRankXcorr,
                                    int iPrintTargetDecoy,
                                    Results *pOutput,
                                    FILE *fpOut,
                                    FILE *fpdb,
                                    double dDeltaCn,
                                    double dLastDeltaCn);

   static void GetVal(char *szElement,
                      char *szAttribute,
                      char *szAttributeVal);

   static void WriteVariableMod(FILE *fpout,
                                CometSearchManager &searchMgr,
                                string varModName,
                                bool bWriteTerminalMods);

   static void WriteStaticMod(FILE *fpout,
                              CometSearchManager &searchMgr,
                              string varModName);

};

#endif
