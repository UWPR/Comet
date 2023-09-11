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
