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

#ifndef _COMETWRITEPEPXML
#define _COMETWRITEPEPXML_


class CometWritePepXML
{
public:
   CometWritePepXML();
   ~CometWritePepXML();
   static void WritePepXML(FILE *fpout,
                           FILE *fpoutd,
                           char *szOutput,
                           char *szOutputDecoy,
                           char *szParamsFile);


private:
   static void WriteXMLHeader(FILE *fpout,
                              char *szParamsFile);
   static void PrintResults(int iWhichQuery,
                            bool bDecoy,
                            FILE *fpOut,
                            char *szOutput);
   static void PrintPepXMLSearchHit(int iWhichQuery,
                                    int iRankXcorr,
                                    int iWhichResult,
                                    int iDoXcorrCount,
                                    bool bDecoy,
                                    Results *pOutput,
                                    FILE *fpOut);
   static void ReadInstrument(char *szManufacturer,
                              char *szModel);
   static void GetVal(char *szElement,
                      char *szAttribute,
                      char *szAttributeVal);

   static void CalcNTTNMC(Results *pOutput,
                          int iWhichQuery,
                          int *iNTT,
                          int *iNMC);
};

#endif
