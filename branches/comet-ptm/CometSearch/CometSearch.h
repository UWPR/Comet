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
#include "CometDataInternal.h"

#include <bitset>  // Comet-PTM

struct SearchThreadData
{
   sDBEntry dbEntry;
   bool *pbSearchMemoryPool;


   SearchThreadData()
   {
   }

   SearchThreadData(sDBEntry &dbEntry_in)
   {
      dbEntry.strName = dbEntry_in.strName;
      dbEntry.strSeq = dbEntry_in.strSeq;
      dbEntry.lProteinFilePosition = dbEntry_in.lProteinFilePosition;
      dbEntry.vectorPeffMod = dbEntry_in.vectorPeffMod;
      dbEntry.vectorPeffVariantSimple = dbEntry_in.vectorPeffVariantSimple;
   }

   ~SearchThreadData()
   {
      // Mark that the memory is no longer in use.
      // DO NOT FREE MEMORY HERE. Just release pointer.
      Threading::LockMutex(g_searchMemoryPoolMutex);

      if (pbSearchMemoryPool!=NULL)
      {
         *pbSearchMemoryPool=false;
         pbSearchMemoryPool=NULL;
      }

      dbEntry.vectorPeffMod.clear();
      dbEntry.vectorPeffVariantSimple.clear();

      Threading::UnlockMutex(g_searchMemoryPoolMutex);
   }
};

class CometSearch
{
public:
   CometSearch();
   ~CometSearch();

   // Manages memory in the search memory pool
   static bool AllocateMemory(int maxNumThreads);
   static bool DeallocateMemory(int maxNumThreads);
   static bool RunSearch(int minNumThreads,
                         int maxNumThreads,
                         int iPercentStart,
                         int iPercentEnd);
   static void SearchThreadProc(SearchThreadData *pSearchThreadData);
   bool DoSearch(sDBEntry dbe, bool *pbDuplFragment);

private:

   // Core search functions
   void ReadOBO(char *szOBO,
                vector<OBOStruct> *vectorUniModOBO);
   bool MapOBO(string strMod,
               vector<OBOStruct> *vectorPeffOBO,
               struct PeffModStruct *pData);
   int BinarySearchPeffStrMod(int start,
                          int end,
                          string strMod,
                          vector<OBOStruct>& vectorPeffOBO);
   int BinarySearchMass(int start,
                        int end,
                        double dCalcPepMass);
   void SubtractVarMods(int *piVarModCounts,
                        int cResidue,
                        int iResiduePosition);
   void CountVarMods(int *piVarModCounts,
                     int cResidue,
                     int iResiduePosition);
   bool HasVariableMod(int varModCounts[],
                       int iCVarModCount,
                       int iNVarModCount,
                       struct sDBEntry *dbe);
   int WithinMassTolerance(double dCalcPepMass,
                           char *szProteinSeq,
                           int iStartPos,
                           int iEndPos);
   bool WithinMassTolerancePeff(double dCalcPepMass,
                                vector<PeffPositionStruct>* vPeffArray,
                                int iStartPos,
                                int iEndPos);
   void XcorrScore(char *szProteinSeq,
                   int iStartResidue,
                   int iEndResidue,
                   int iStartPos,
                   int iEndPos,
                   bool bFoundVariableMod,
                   double dCalcPepMass,
                   bool bDecoyPep,
                   int iWhichQuery,
                   int iLenPeptide,
                   int *piVarModSites,
                   struct sDBEntry *dbe,
// Comet-PTM start
                   bool bDeltaXcorrSearch = false,
                   double dDeltaXcorrMass = 0.0,
                   std::string sDeltaJumps = "0");
// Comet-PTM end

   bool CheckEnzymeTermini(char *szProteinSeq,
                           int iStartPos,
                           int iEndPos);
   bool CheckEnzymeStartTermini(char *szProteinSeq,
                                int iStartPos);
   bool CheckEnzymeEndTermini(char *szProteinSeq,
                              int iStartPos);
   bool CheckMassMatch(int iWhichQuery,
                       double dCalcPepMass);
   double GetFragmentIonMass(int iWhichIonSeries,
                             int i,
                             int ctCharge,
                             double *pdAAforward,
                             double *pdAAreverse);
   int CheckDuplicate(int iWhichQuery,
                      int iStartResidue,
                      int iEndResidue,
                      int iStartPos,
                      int iEndPos,
                      bool bFoundVariableMod,
                      double dCalcPepMass,
                      char *szProteinSeq,
                      bool bDecoyResults,
                      int *piVarModSites,
                      struct sDBEntry *dbe,
                      bool bDeltaXcorrSearch = false,  // Comet-PTM
                      double dDeltaXcorrMass = 0.0);   // Comet-PTM
   void StorePeptide(int iWhichQuery,
                     int iStartResidue,
                     int iStartPos,
                     int iEndPos,
                     bool bFoundVariableMod,
                     char *szProteinSeq,
                     double dCalcPepMass,
                     double dScoreSp,
                     bool bStoreSeparateDecoy,
                     int *piVarModSites,
                     struct sDBEntry *dbe,
// Comet-PTM start
                     bool bDeltaXcorrSearch = false,
                     double dDeltaXcorrMass = 0.0,
                     std::vector<std::bitset<MAX_PEPTIDE_LEN> > vctBitMods = std::vector<std::bitset<MAX_PEPTIDE_LEN> >(),
                     std::string sDeltaJumps = "0",
                     const int& iModPos = -1);
// Comet-PTM end
   void VariableModSearch(char *szProteinSeq,
                          int varModCounts[],
                          int iStartPos,
                          int iEndPos,
                          bool *pbDuplFragment,
                          struct sDBEntry *dbe);
   double TotalVarModMass(int *pVarModCounts);
   bool PermuteMods(char *szProteinSeq,
                    int iWhichQuery,
                    int iWhichMod,
                    bool *pbDuplFragments,
                    bool *bDoPeffAnalysis,
                    vector <PeffPositionStruct>* vPeffArray,
                    struct sDBEntry *dbe,
                    long *lNumIterations);
   int  twiddle( int *x, int *y, int *z, int *p);
   void inittwiddle(int m, int n, int *p);
   bool MergeVarMods(char *szProteinSeq,
                     int iWhichQuery,
                     bool *pbDuplFragments,
                     bool *bDoPeffAnalysis,
                     vector <PeffPositionStruct>* vPeffArray,
                     struct sDBEntry *dbe,
                     long *lNumIterations);
   bool CalcVarModIons(char *szProteinSeq,
                       int iWhichQuery,
                       bool *pbDuplFragment,
                       int *piVarModSites,
                       double dCalcPepMass,
                       int iLenPeptide,
                       struct sDBEntry *dbe,
                       long *lNumIterations);
   bool IndexSearch(FILE *fptr);
   bool SearchForPeptides(struct sDBEntry dbe,
                          char *szProteinSeq,
                          bool bNtermPeptideOnly,
                          bool *pbDuplFragment);
   void SearchForVariants(struct sDBEntry dbe,
                          char *szProteinSeq,
                          bool *pbDuplFragment);
   bool TranslateNA2AA(int *frame,
                       int iDirection,
                       char *sDNASequence);
   void AnalyzeIndexPep(int iWhichQuery,
                        char *szProteinSeq,
                        double dCalcPepMass,
                        bool *pbDuplFragment,
                        struct sDBEntry *dbe);
   char GetAA(int i,
              int iDirection,
              char *sDNASequence);

   // Processing results
// void CalculateEvalue(struct Results *_pResults);
   void GenerateXcorrDecoys(struct Results *_pResults,
                            bool bDecoy);

// Comet-PTM
   void DeltaXcorrSearch(const double& dDeltaXcorrMass,
                         const int& iWhichQuery,
                         const double& dCalcPepMass,
                         const int& iEndPos,
                         const int& iStartPos,
                         const int& iProteinSeqLengthMinus1,
                         char* szProteinSeq,
                         bool* pbDuplFragment,
                         struct sDBEntry *dbe,
                         const int& iLenPeptide,
                         std::string sJump = "0",
                         int iJump = 0);

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
   void PrintIons(int iTmpCharge,
                  FILE *fpOut);

   // Cleaning up
   void CleanUp();

private:
   struct VarModStat
   {
       int iTotVarModCt;                     // # mod positions in peptide
       int iTotBinaryModCt;                  // # mod positions of all binary mods in group in the peptide
       int iMatchVarModCt;
       int iVarModSites[MAX_PEPTIDE_LEN+2];  // last 2 positions are for n- and c-term
   };

   struct VarModInfo
   {
       VarModStat varModStatList[VMODS];
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
       int  iProteinSeqLength;
       int  iAllocatedProtSeqLength;
       int  iPeffOrigResiduePosition;             // position of PEFF variant substitution; -1 = n-term, iLenPeptide = c-term; -9=unused
       long lProteinFilePosition;
       char szProteinName[WIDTH_REFERENCE];
       char *pszProteinSeq;
       char cPeffOrigResidue;                     // original residue of a PEFF variant
       char cPrevAA;  // hack for indexdb realtime search
       char cNextAA;  // hack for indexdb realtime search
   };

   struct SpectrumInfoInternal
   {
      Spectrum *pSpectrum;
      int      iChargeState;
      int      iArraySize;     // m/z versus intensity array
      int      iHighestIon;
      double   dExperimentalPeptideMass;
      double   dHighestIntensity;
      double   dTotalIntensity;
   };

   double             _pdAAforward[MAX_PEPTIDE_LEN];      // Stores fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAreverse[MAX_PEPTIDE_LEN];      // Stores n-term fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAforwardDelta[MAX_PEPTIDE_LEN]; // Comet-PTM
   double             _pdAAreverseDelta[MAX_PEPTIDE_LEN]; // Comet-PTM
   double             _pdAAforwardDecoy[MAX_PEPTIDE_LEN]; // Stores fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAreverseDecoy[MAX_PEPTIDE_LEN]; // Stores n-term fragment ion fragment ladder calc.; sum AA masses including mods
   int                _iSizepiVarModSites;
   int                _iSizepdVarModSites;
   VarModInfo         _varModInfo;
   ProteinInfo        _proteinInfo;

   unsigned int       _uiBinnedIonMasses[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN];
   unsigned int       _uiBinnedIonMassesDelta[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN]; // Comet-PTM
   unsigned int       _uiBinnedIonMassesDecoy[MAX_FRAGMENT_CHARGE+1][9][MAX_PEPTIDE_LEN];

   static bool *_pbSearchMemoryPool;    // Pool of memory to be shared by search threads
   static bool **_ppbDuplFragmentArr;      // Number of arrays equals number of threads
};

#endif // _COMETSEARCH_H_
