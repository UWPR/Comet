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

#ifndef _COMETSEARCH_H_
#define _COMETSEARCH_H_

#include "Common.h"

struct SearchThreadData
{
   sDBEntry dbEntry;

   SearchThreadData()
   {
   }

   SearchThreadData(sDBEntry &dbEntry_in)
   {
      dbEntry.strName = dbEntry_in.strName;
      dbEntry.strSeq = dbEntry_in.strSeq;
   }
};

class CometSearch
{
public:
   CometSearch();
   ~CometSearch();

   static void RunSearch(int minNumThreads, int maxNumThreads);
   static void SearchThreadProc(SearchThreadData *pSearchThreadData);
   void DoSearch(sDBEntry dbe);
    
private:
    
   // Initializations and preprocessing in prepraration for search
   bool Initialize(Spectrum *pSpectrum, int iZLine);
   void AdjustMassTolerances();
   void CalcMatchedPeaksMasses();

   void PrintSearchParams();

   // qsort functions - they need to be static
   static int QsortMatchedPeaksCompare(const void *p0, const void *p1);
   static int QsortByIon(const void *p0, const void *p1);
   static int QsortSpScore(const void *p0, const void *p1);
   static int QsortCorr(const void *p0, const void *p1);
    

   // Core search functions
   void IndexSearchRec(int var,
                       double winA,
                       double winB);
   void IndexSearch(double winA,
                    double winB);
   void ReadPeptideIndex(int winA,
                         int winB,
                         sDBTable &First,
                         sDBTable &Last);
   void SearchInBuffer(int bufSize, FILE *fptr);
   bool CheckEnzymeSpecific(char *szProteinSeq,
                            int iStartPos,
                            int iEndPos);
   int BinarySearchMass(int start,
                        int end,
                        double dCalcPepMass);
   void SearchForPeptides_MH(char *szProteinSeq, 
                             char *szProteinName, 
                             int iStartPos, 
                             int iEndPos); 
   void SubtractVarMods(int *piVarModCounts,
                        int character);
   void CountVarMods(int *piVarModCounts,
                   int character);
   void CountBinaryModN(int *piVarModCounts,
                         int iStartPos);
   void CountBinaryModC(int *piVarModCounts,
                         int iEndPos);
   int  TotalVarModCount(int varModCounts[],
                       int iCVarModCount,
                       int iNVarModCount);
   int WithinMassTolerance(double dCalcPepMass,
                           char *szProteinSeq,
                           int iStartPos,
                           int iEndPos);
   void XcorrScore(char *szProteinSeq,
                   char *szProteinName,
                   int iStartPos,
                   int iEndPos,
                   bool bFoundVariableMod,
                   double dCalcPepMass,
                   bool bDecoyPep,
                   int iWhichQuery,
                   int iLenPeptide,
                   char *pcVarModSites);
   bool CheckEnzymeTermini(char *szProteinSeq,
                           int iStartPos,
                           int iEndPos);
   bool CheckMassMatch(int iWhichQuery,
                       double dCalcPepMass);
   bool CheckMatchedPeaks(int iTmpCharge,
                          int iLenPeptide);
   double GetFragmentIonMass(int iWhichIonSeries,
                             int i,
                             int ctCharge,
                             double *pdAAforward,
                             double *pdAAreverse);
   int CheckDuplicate(int iWhichQuery,
                      int iStartPos,
                      int iEndPos,
                      bool bFoundVariableMod,
                      double dCalcPepMass,
                      char *szProteinSeq,
                      char *szProteinName,
                      bool bDecoyResults,
                      char *pcVarModSites);
   void StorePeptide(int iWhichQuery,
                     int iStartPos,
                     int iEndPos,
                     bool bFoundVariableMod,
                     char *szProteinSeq,
                     double dCalcPepMass,
                     double dScoreSp,
                     char *szProteinName,
                     bool bStoreSeparateDecoy,
                     char *pcVarModSites);
   void StoreDecoyPeptide(int iWhichQuery,
                     int iStartPos,
                     int iEndPos,
                     bool bFoundVariableMod,
                     int iMatchedFragmentIonCt,
                     char *szProteinSeq,
                     double dCalcPepMass,
                     double dScoreSp,
                     char *szProteinName);
   void VarModSearch(char *szProteinSeq,
                   char *szProteinName,
                   int varModCounts[],
                   int iStartPos,
                   int iEndPos);
   double TotalVarModMass(int *pVarModCounts,
                         int iCVarModCount,
                         int iNVarModCount);
   void Permute1(char *szProteinSeq, 
                 int iWhichQuery);
   void Permute2(char *szProteinSeq,
                 int iWhichQuery);
   void Permute3(char *szProteinSeq,
                 int iWhichQuery);
   void Permute4(char *szProteinSeq,
                 int iWhichQuery);
   void Permute5(char *szProteinSeq,
                 int iWhichQuery);
   void Permute6(char *szProteinSeq,
                 int iWhichQuery);
   int  twiddle( int *x, int *y, int *z, int *p);
   void inittwiddle(int m, int n, int *p);
   void CalcVarModIons(char *szProteinSeq,
                    int iWhichQuery);
   void ResetIndexTable();
   void FullDBSearch();
   void SearchForPeptides(char *szProteinSeq,
                          char *szProteinName,
                          bool bNtermPeptideOnly);
   void TranslateNA2AA(int *frame,
                       int iDirection,
                       char *sDNASequence);
   char GetAA(int i,
              int iDirection,
              char *sDNASequence);

   // Processing results
   void SortResults(struct Results *_pResults,
                    bool bDecoy);
   void CalculateEvalue(struct Results *_pResults,
                        bool bDecoy);
   void GenerateXcorrDecoys(struct Results *_pResults,
                        bool bDecoy);

    // Displaying results
   void PrintResults(struct Results *_pResults,
                     bool bDecoySearch);
   void PrintOutputLine(struct Results *_pResults,
                        int iRankXcorr,
                        int iLenMaxDuplicates,
                        int iMaxWidthReference,
                        int iWhichResult,
                        bool bDecoySearch,
                        FILE *fpOut);
   void PrintIons(int iTmpCharge, FILE *fpOut);

   // Cleaning up
   void CleanUp();

private:
   struct VarModStat
   {
       int     iTotVarModCt;
       int     iMatchVarModCt;
       int     iVarModSites[MAX_PEPTIDE_LEN];
   };
    
   struct VarModInfo
   {
       VarModStat varModStatList[VMODS];
       int        iNumVarModSiteN;
       int        iNumVarModSiteC;
       int        iStartPos;     // The start position of the peptide sequence
       int        iEndPos;       // The end position of the peptide sequence
       double     dCalcPepMass;  // Mass of peptide with mods
   };

   struct PepMassTolerance
   {
       double dPeptideMassTolerance;
       double dPeptideMassToleranceMinus;
       double dPeptideMassTolerancePlus;
   };

   struct PepMassInfo
   {
       double           dCalcPepMass;
       PepMassTolerance pepMassTol;
   };

   struct MatchedPeaksStruct
   {
      double dMass;
      double dIntensity;
   };

   struct ProteinInfo
   {
       char szProteinName[WIDTH_REFERENCE];
       char *pszProteinSeq;
       int  iProteinSeqLength;
       int  iAllocatedProtSeqLength;
   };

   struct SpectrumInfoInternal
   {
      Spectrum *pSpectrum;
      int      iZLine;
      int      iChargeState;  
      int      iArraySize;     // m/z versus intensity array
      int      iHighestIon;
      double   dExperimentalPeptideMass; 
      double   dHighestIntensity;
      double   dTotalIntensity;         
   };

   char               *_pszSearchedWin;
   double             _pdAAforward[MAX_PEPTIDE_LEN];      // Stores fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAreverse[MAX_PEPTIDE_LEN];      // Stores n-term fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAforwardDecoy[MAX_PEPTIDE_LEN]; // Stores fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAreverseDecoy[MAX_PEPTIDE_LEN]; // Stores n-term fragment ion fragment ladder calc.; sum AA masses including mods
   int                _iSizepcVarModSites;
   VarModInfo         _varModInfo;
   ProteinInfo        _proteinInfo;       
   MatchedPeaksStruct *_pMatchedPeaks;

   unsigned int       _uiBinnedIonMasses[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN];
   unsigned int       _uiBinnedIonMassesDecoy[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN];
};

#endif // _COMETSEARCH_H_
