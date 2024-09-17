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


#ifndef _COMETWRITEMZIDENTML_
#define _COMETWRITEMZIDENTML_


class CometWriteMzIdentML
{
   struct MzidTmpStruct        // stores the info from tab-delimited text file
   {
      int    iScanNumber;
      int    iBatchNum;
      int    iXcorrRank;
      int    iCharge;
      int    iMatchedIons;
      int    iTotalIons;
      int    iRankSp;
      int    iWhichQuery;
      int    iWhichResult;
      double dExpMass;         // neutral experimental mass
      double dCalcMass;        // neutral calculated mass
      double dExpect;
      double dRTime;
      float  fXcorr;
      float  fCn;
      float  fSp;
      char   cPrevNext[3];
      string strPeptide;
      string strMods;
      string strProtsTarget;   // delimited list of file offsets
      string strProtsDecoy;    // delimited list of file offsets
   };

public:
   CometWriteMzIdentML();
   ~CometWriteMzIdentML();


   static void WriteMzIdentMLTmp(FILE *fpout,
                                 FILE *fpoutd,
                                 int iBatchNum);

   static void WriteMzIdentML(FILE *fpout,
                              FILE *fpdb,
                              char *szTmpFile,
                              CometSearchManager &searchMgr);

private:

   static bool WriteMzIdentMLHeader(FILE *fpout);

   static void PrintTmpPSM(int iWhichQuery,
                           int iPrintTargetDecoy,
                           int iBatchNum,
                           FILE *fpOut);

   static void WriteMods(FILE *fpout,
                         CometSearchManager &searchMgr);

   static void WriteVariableMod(FILE *fpout,
                                int iWhichVariableMod,
                                bool bWriteTerminalMods);

   static void WriteStaticMod(FILE *fpout,
                              CometSearchManager &searchMgr,
                              string varModName);

   static void GetModificationID(char cResidue,
                                 double dModMass,
                                 string *strModID,
                                 string *strModRef,
                                 string *strModName);

   static string GetAdditionalEnzymeInfo(int iWhichEnzyme);
 
   static void WriteEnzyme(FILE *fpout);

   static void WriteMassTable(FILE *fpout);

   static void WriteTolerance(FILE *fpout);

   static void WriteInputs(FILE *fpout);

   static void WriteSpectrumIdentificationList(FILE* fpout,
                                               FILE *fpdb,
                                               vector<MzidTmpStruct>* vMzid);

   static bool ParseTmpFile(FILE *fpout,
                            FILE *fpdb,
                            char *szTmpFile,
                            CometSearchManager &searchMgr);
};

#endif
