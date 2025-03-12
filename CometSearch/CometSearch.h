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


#ifndef _COMETSEARCH_H_
#define _COMETSEARCH_H_

#include "CometDataInternal.h"

struct SearchThreadData
{
   sDBEntry dbEntry;
   bool *pbSearchMemoryPool;
   ThreadPool *tp;
  
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
      dbEntry.vectorPeffVariantComplex = dbEntry_in.vectorPeffVariantComplex;
   }

   ~SearchThreadData()
   {
      // Mark that the memory is no longer in use.
      // DO NOT FREE MEMORY HERE. Just release pointer.
     //      Threading::LockMutex(g_searchMemoryPoolMutex);

      if (pbSearchMemoryPool!=NULL)
      {
         *pbSearchMemoryPool=false;
         pbSearchMemoryPool=NULL;
      }

      dbEntry.vectorPeffMod.clear();
      dbEntry.vectorPeffVariantSimple.clear();

      //Threading::UnlockMutex(g_searchMemoryPoolMutex);
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
   static bool RunSearch(int iPercentStart,
                         int iPercentEnd,
                         ThreadPool *tp);
   static bool RunSearch(ThreadPool *tp);    // for DoSingleSpectrumSearchMultiResults() to call IndexSearch()
   static void SearchThreadProc(SearchThreadData *pSearchThreadData,
                                ThreadPool *tp);
   bool DoSearch(sDBEntry dbe,
                 bool *pbDuplFragment);
   bool CheckEnzymeTermini(char *szProteinSeq,
                           int iStartPos,
                           int iEndPos);
   bool CheckEnzymeStartTermini(char *szProteinSeq,
                                int iStartPos);
   bool CheckEnzymeEndTermini(char *szProteinSeq,
                              int iEndPos);
   int BinarySearchMass(int start,
                        int end,
                        double dCalcPepMass);
   static bool CheckMassMatch(int iWhichQuery,
                              double dCalcPepMass);

   struct ProteinInfo
   {
       int  iProteinSeqLength;                    // length of sequence
       int  iTmpProteinSeqLength;                 // either length of sequence or 1 less for skip N-term M; or more for PEFF insertions
       int  iAllocatedProtSeqLength;              // used in nucleotide to AA translation
       int  iPeffOrigResiduePosition;             // position of PEFF variant substitution; -1 = n-term, iLenPeptide = c-term; -9=unused
       comet_fileoffset_t lProteinFilePosition;
       char szProteinName[WIDTH_REFERENCE];
       char *pszProteinSeq;
       //char cPeffOrigResidue;                     // original residue of a PEFF variant
       string sPeffOrigResidues;                  // original residue(s) of a PEFF variant
       int    iPeffNewResidueCount;               // number of new residue(s) being substituted/added in PEFF variant
       char cPrevAA;  // hack for indexdb realtime search
       char cNextAA;  // hack for indexdb realtime search
   };

   ProteinInfo _proteinInfo;

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
   static int BinarySearchIndexMass(int iWhichThread,
                                    int iPrecursorBin,
                                    int start,
                                    int end,
                                    double dQueryMass,
                                    unsigned int *uiFragmentMass);
   void SubtractVarMods(int *piVarModCounts,
                        int cResidue,
                        int iResiduePosition);
   void CountVarMods(int *piVarModCounts,
                     int cResidue,
                     int iResiduePosition);
   bool HasVariableMod(int *varModCounts,
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
                   int iFoundVariableMod,
                   double dCalcPepMass,
                   bool bDecoyPep,
                   int iWhichQuery,
                   int iLenPeptide,
                   int *piVarModSites,
                   struct sDBEntry *dbe);
   static void XcorrScoreI(char *szProteinSeq,
                   int iStartPos,
                   int iEndPos,
                   int iFoundVariableMod,
                   double dCalcPepMass,
                   bool bDecoyPep,
                   int iWhichQuery,
                   int iLenPeptide,
                   int *piVarModSites,
                   struct sDBEntry *dbe,
                   unsigned int uiBinnedIonMasses[MAX_FRAGMENT_CHARGE+1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][FRAGINDEX_VMODS+2],
                   unsigned int uiBinnedPrecursorNL[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE],
                   int iNumMatchedFragmentIons);
/*
   static double GetFragmentIonMass(int iWhichIonSeries,
                             int i,
                             int ctCharge,
                             double *pdAAforward,
                             double *pdAAreverse);
*/
   int CheckDuplicate(int iWhichQuery,
                      int iStartResidue,
                      int iEndResidue,
                      int iStartPos,
                      int iEndPos,
                      int iFoundVariableMod,
                      double dCalcPepMass,
                      char *szProteinSeq,
                      bool bDecoyResults,
                      int *piVarModSites,
                      struct sDBEntry *dbe);
   void StorePeptide(int iWhichQuery,
                     int iStartResidue,
                     int iStartPos,
                     int iEndPos,
                     int iFoundVariableMod,
                     char *szProteinSeq,
                     double dCalcPepMass,
                     double dXcorr,
                     bool bStoreSeparateDecoy,
                     int *piVarModSites,
                     struct sDBEntry *dbe);
   static void StorePeptideI(int iWhichQuery,
                     int iStartPos,
                     int iEndPos,
                     int iFoundVariableMod,
                     char *szProteinSeq,
                     double dCalcPepMass,
                     double dXcorr,
                     bool bStoreSeparateDecoy,
                     int *piVarModSites,
                     struct sDBEntry *dbe);
   void VariableModSearch(char *szProteinSeq,
                          int piVarModCounts[],
                          int iStartPos,
                          int iEndPos,
                          int iClipNtermMetOffset,
                          bool *pbDuplFragment,
                          struct sDBEntry *dbe);
   double TotalVarModMass(int *pVarModCounts);
   bool PermuteMods(char *szProteinSeq,
                    int iWhichQuery,
                    int iWhichMod,
                    int iClipNtermMetOffset,
                    bool *pbDuplFragments,
                    bool *bDoPeffAnalysis,
                    vector <PeffPositionStruct>* vPeffArray,
                    struct sDBEntry *dbe);
   int  twiddle( int *x, int *y, int *z, int *p);
   void inittwiddle(int m, int n, int *p);
   bool MergeVarMods(char *szProteinSeq,
                     int iWhichQuery,
                     int iClipNtermMetOffset,
                     bool *pbDuplFragments,
                     bool *bDoPeffAnalysis,
                     vector <PeffPositionStruct>* vPeffArray,
                     struct sDBEntry *dbe);
   bool CalcVarModIons(char *szProteinSeq,
                       int iWhichQuery,
                       bool *pbDuplFragment,
                       int *piVarModSites,
                       double dCalcPepMass,
                       int iLenPeptide,
                       struct sDBEntry *dbe);
   static void SearchFragmentIndex(size_t iWhichQuery,
                                   ThreadPool *tp);
   bool SearchPeptideIndex(void);
   void AnalyzePeptideIndex(int iWhichQuery,
                            DBIndex sDBI,
                            bool *pbDuplFragment,
                            struct sDBEntry *dbe);
   bool SearchForPeptides(struct sDBEntry dbe,
                          char *szProteinSeq,
                          int iNtermPeptideOnly,  // used in clipped methionine sequence
                          bool *pbDuplFragment);
   void SearchForVariants(struct sDBEntry dbe,
                          char *szProteinSeq,
                          bool *pbDuplFragment);
   bool TranslateNA2AA(int *frame,
                       int iDirection,
                       char *sDNASequence);

   char GetAA(int i,
              int iDirection,
              char *sDNASequence);

   struct VarModStat
   {
       int iTotVarModCt;                     // # mod positions in peptide
       int iTotBinaryModCt;                  // # mod positions of all binary mods in group in the peptide
       int iMatchVarModCt;
       int iVarModSites[MAX_PEPTIDE_LEN_P2]; // last 2 positions are for n- and c-term
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
       double dPeptideMassToleranceLow;           // mass tolerance low in amu from experimental mass
       double dPeptideMassToleranceHigh;          // mass tolerance high in amu from experimental mass
       double dPeptideMassToleranceMinus;         // low end of mass tolerance range including isotope offsets
       double dPeptideMassTolerancePlus;          // high end of mass tolerance range including isotope offsets
   };

   struct MatchedPeaksStruct
   {
      double dMass;
      double dIntensity;
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
   double             _pdAAforwardDecoy[MAX_PEPTIDE_LEN]; // Stores fragment ion fragment ladder calc.; sum AA masses including mods
   double             _pdAAreverseDecoy[MAX_PEPTIDE_LEN]; // Stores n-term fragment ion fragment ladder calc.; sum AA masses including mods
   int                _iSizepiVarModSites;
   int                _iSizepdVarModSites;
   VarModInfo         _varModInfo;

   unsigned int       _uiBinnedIonMasses[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 2];   // +2 for two fragment NL series
   unsigned int       _uiBinnedIonMassesDecoy[MAX_FRAGMENT_CHARGE + 1][NUM_ION_SERIES][MAX_PEPTIDE_LEN][VMODS + 2];
   unsigned int       _uiBinnedPrecursorNL[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE];
   unsigned int       _uiBinnedPrecursorNLDecoy[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE];

   static bool *_pbSearchMemoryPool;    // Pool of memory to be shared by search threads
   static bool **_ppbDuplFragmentArr;   // Number of arrays equals number of threads
};

#endif // _COMETSEARCH_H_
