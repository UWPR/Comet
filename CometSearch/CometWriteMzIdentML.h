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
      float  fRTime;           // encoded in seconds
      float  fXcorr;
      float  fCn;
      float  fSp;
      float  fAScorePro;
      char   cHasVariableMod;
      char   cPrevNext[3];
      std::string strPeptide;
      std::string strMods;
      std::string strProtsTarget;   // delimited list of file offsets
      std::string strProtsDecoy;    // delimited list of file offsets
   };

public:
   CometWriteMzIdentML();
   ~CometWriteMzIdentML();


   static void WriteMzIdentMLTmp(FILE *fpout,
                                 FILE *fpoutd,
                                 int iBatchNum);

   static void WriteMzIdentML(FILE *fpout,
                              FILE *fpdb,
                              std::string sTmpFile,
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
                              std::string varModName);

   static void GetModificationID(char cResidue,
                                 double dModMass,
                                 std::string *strModID,
                                 std::string *strModRef,
                                 std::string *strModName);

   static std::string GetAdditionalEnzymeInfo(int iWhichEnzyme);
 
   static void WriteEnzyme(FILE *fpout);

   static void WriteMassTable(FILE *fpout);

   static void WriteTolerance(FILE *fpout);

   static void WriteInputs(FILE *fpout);

   static void WriteSpectrumIdentificationList(FILE* fpout,
                                               FILE *fpdb,
                                               std::vector<MzidTmpStruct>* vMzid);

   static bool ParseTmpFile(FILE *fpout,
                            FILE *fpdb,
                            std::string ssTmpFile,
                            CometSearchManager &searchMgr);
};

#endif
