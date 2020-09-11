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

#ifndef _COMETWRITEMZIDENTML
#define _COMETWRITEMZIDENTML_


class CometWriteMzIdentML
{
public:
   CometWriteMzIdentML();
   ~CometWriteMzIdentML();


   static void WriteMzIdentMLTmp(FILE *fpout,
                                 FILE *fpoutd);

   static void WriteMzIdentML(FILE *fpout,
                              FILE *fpdb,
                              char *szTmpFile,
                              CometSearchManager &searchMgr);

private:
   static bool WriteMzIdentMLHeader(FILE *fpout);

   static void WriteMzIdentMLEndTags(FILE *fpout);

   static void PrintTmpPSM(int iWhichQuery,
                           int iPrintTargetDecoy,
                           FILE *fpOut);

   static void PrintMzIdentMLSearchHit(int iWhichQuery,
                                    int iWhichResult,
                                    int iRankXcorr,
                                    bool bDecoy,
                                    Results *pOutput,
                                    FILE *fpOut,
                                    FILE *fpdb,
                                    double dDeltaCn,
                                    double dDeltaCnStar);

   static void ReadInstrument(char *szManufacturer,
                              char *szModel);

   static void GetVal(char *szElement,
                      char *szAttribute,
                      char *szAttributeVal);

   static void CalcNTTNMC(Results *pOutput,
                          int iWhichQuery,
                          int *iNTT,
                          int *iNMC);

   static void WriteMods(FILE *fpout,
                         CometSearchManager &searchMgr);

   static void WriteVariableMod(FILE *fpout,
                                CometSearchManager &searchMgr,
                                string varModName,
                                bool bWriteTerminalMods);

   static void WriteStaticMod(FILE *fpout,
                              CometSearchManager &searchMgr,
                              string varModName);

   static void GetModificationID(char cResidue,
                                 double dModMass,
                                 string *strModID,
                                 string *strModRef,
                                 string *strModName);
 
   static void WriteEnzyme(FILE *fpout);

   static bool ParseTmpFile(FILE *fpout,
                            FILE *fpdb,
                            char *szTmpFile,
                            CometSearchManager &searchMgr);

   static void WriteAnalysisProtocol(FILE *fpout);
};

#endif
