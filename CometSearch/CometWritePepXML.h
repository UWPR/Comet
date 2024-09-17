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
                                    int iPrintTargetDecoy,
                                    Results *pOutput,
                                    FILE *fpOut,
                                    FILE *fpdb);

   static void GetVal(char *szElement,
                      char *szAttribute,
                      char *szAttributeVal);

   static void WriteVariableMod(FILE *fpout,
                                int iWhichVariableMod,
                                bool bWriteTerminalMods);

   static void WriteStaticMod(FILE *fpout,
                              CometSearchManager &searchMgr,
                              string varModName);

};

#endif
