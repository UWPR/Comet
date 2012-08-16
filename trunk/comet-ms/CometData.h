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

#ifndef _COMETDATA_H_
#define _COMETDATA_H_

#include "Threading.h"

#define PROTON_MASS                 1.00727646688

#define FLOAT_ZERO                  0.000001
#define MAX_VARMOD_AA               20       // max # of modified AAs in a peptide per variable modification 
#define MAX_ENZYME_AA               20       // max # of AA for enzyme break point
#define MAX_PEPTIDE_LEN             64       // max # of AA for a peptide
#define MAX_PEPTIDE_LEN_P2          66       // max # of AA for a peptide plus 2 for N/C-term
#define NUM_SP_IONS                 200      // num ions for preliminary scoring
#define NUM_STORED                  100      // number of internal search results to store

#define DEFAULT_FRAGMENT_CHARGE     3
#define DEFAULT_PRECURSOR_CHARGE    6
#define MAX_FRAGMENT_CHARGE         5
#define MAX_PRECURSOR_CHARGE        9
#define MINIMUM_PEAKS               5

#define SIZE_BUF                    8192
#define SIZE_FILE                   512
#define WIDTH_REFERENCE             40       // size of the protein accession field to store
#define DEFAULT_PREC_TOL            2.0      // default precursor removal tolerance
#define MAX_THREADS                 32
#define DEFAULT_BIN_WIDTH           0.36
#define DEFAULT_OFFSET              0.11

#define HISTO_SIZE                  152      // some number greater than 150; chose 152 for byte alignment?

#define DECOY_SIZE                  500      // minimum # of decoys to have for e-value calculation

#define VMODS                       6
#define VMODS_ALL                   VMODS + 2
#define VMOD_1_INDEX                0
#define VMOD_2_INDEX                1
#define VMOD_3_INDEX                2
#define VMOD_4_INDEX                3
#define VMOD_5_INDEX                4
#define VMOD_6_INDEX                5
#define VMOD_N_INDEX                6
#define VMOD_C_INDEX                7

#define ENZYME_SINGLE_TERMINI       1
#define ENZYME_DOUBLE_TERMINI       2
#define ENZYME_N_TERMINI            8
#define ENZYME_C_TERMINI            9

#define ION_SERIES_A                0
#define ION_SERIES_B                1
#define ION_SERIES_C                2
#define ION_SERIES_X                3
#define ION_SERIES_Y                4
#define ION_SERIES_Z                5


enum AnalysisType 
{
   AnalysisType_Unknown = 0,
   AnalysisType_DTA,
   AnalysisType_SpecificScan,
   AnalysisType_SpecificScanAndCharge,
   AnalysisType_SpecificScanRange,
   AnalysisType_StartScanAndCount,
   AnalysisType_EntireFile
};

enum InputType 
{
   InputType_MS2 = 0,           // ms2, cms2, bms2, etc.
   InputType_MZXML
};

struct msdata                    // used in the preprocessing
{
   double dIon;
   double dIntensity;
};

struct Options             // output parameters 
{
   int iNumPeptideOutputLines;
   int iWhichReadingFrame;
   int iEnzymeTermini;
   int iNumStored;               // # of search results to store for xcorr analysis
   int iStartScan;
   int iEndScan;
   int iStartCharge;
   int iEndCharge;
   int iMaxFragmentCharge;
   int iMaxPrecursorCharge;
   int iStartMSLevel;            // mzXML only
   int iEndMSLevel;              // mzXML only
   int iMinPeaks;
   int bOutputSqtStream;
   int bOutputSqtFile;
   int bOutputPepXMLFile;
   int bOutputOutFiles;
   int iMinIntensity;
   int iRemovePrecursor;         // 0=no, 1=yes, 2=ETD precursors
   int iDecoySearch;             // 0=no, 1=concatenated search, 2=separate decoy search
   int iNumThreads;              // 0=poll CPU else set # threads to spawn
   int bClipNtermMet;            // 0=leave sequences alone; 1=also consider w/o N-term methionine
   int bSkipAlreadyDone;         // 0=search everything; 1=don't re-search if .out exists
   int bNoEnzymeSelected;
   int bPrintFragIons;
   int bPrintExpectScore;
   double dRemovePrecursorTol;
   double dLowPeptideMass;       // MH+ mass
   double dHighPeptideMass;      // MH+ mass
   char szActivationMethod[24];  // mzXML only
};

struct Results 
{
   double dPepMass;
   double dExpect;
   float  fScoreSp;
   float  fXcorr;
   unsigned int   iDuplicateCount;
   unsigned short iLenPeptide;
   unsigned short iRankSp;
   unsigned short iMatchedIons;
   unsigned short iTotalIons;
   char pcVarModSites[MAX_PEPTIDE_LEN_P2];    // store variable mods encoding, +2 to accomodate N/C-term
   char szProtein[WIDTH_REFERENCE];
   char szPeptide[MAX_PEPTIDE_LEN];
   char szPrevNextAA[4];                      // [0] stores prev AA, [1] stores next AA
};

struct PepMassInfo
{
   double dCalcPepMass;
   double dExpPepMass;
   double dPeptideMassTolerance;
   double dPeptideMassToleranceMinus;
   double dPeptideMassTolerancePlus;
};

struct SpectrumInfoInternal
{
   int iArraySize;     // m/z versus intensity array
   int iHighestIon;
   int iScanNumber;
   unsigned short iChargeState;
   unsigned short iMaxFragCharge;
   double dTotalIntensity;
   double dRTime;
};

// The minimum and maximum mass range of all peptides to consider
// i.e. lowestPepMass - tolerance to highestPepMass + tolerance
struct MassRange
{
   double dMinMass;
   double dMaxMass;
   unsigned short iMaxFragmentCharge;  // global maximum fragment charge
};

extern MassRange g_MassRange;

// PreprocessStruct stores information used in preprocessing
// each spectrum.  Information not kept around otherwise
struct PreprocessStruct
{
   int iHighestIon;
   double dHighestIntensity;
   double *pdCorrelationData;
   struct msdata pTempSpData[NUM_SP_IONS];
};

//-->MH
typedef struct sDBEntry
{
   string strName;
   string strSeq;
} sDBEntry;

typedef struct sDBTable
{
   int  iStart;   // pointer to the start of this mass value
   int  iStop;    // pointer to the end of this mass valuse
   char cFile;    // which index file has the start of the data
} sDBTable; 

struct DBInfo
{  
   char szDatabase[SIZE_FILE];
   char szFileName[SIZE_FILE];
   int  iTotalNumProteins;
   unsigned long int liTotAACount;
};

struct InputFileInfo
{
   int  iInputType;
   char szFileName[SIZE_FILE];
   char szBaseName[SIZE_FILE];
};

struct StaticMod
{
   double dAddCterminusPeptide;
   double dAddNterminusPeptide;
   double dAddCterminusProtein;
   double dAddNterminusProtein;
};

struct PrecalcMasses
{
   double dNtermProton;          // dAddNterminusPeptide + PROTON_MASS
   double dCtermOH2Proton;       // dAddCterminusPeptide + dOH2fragment + PROTON_MASS
   double dOH2ProtonCtermNterm;  // dOH2parent + PROTON_MASS + dAddCterminusPeptide + dAddNterminusPeptide
   int iMinus17;      // BIN'd value of mass(NH3)
   int iMinus18;      // BIN'd value of mass(H2O)
};

struct VarMods 
{
   int    bBinaryMod;
   int    iMaxNumVarModAAPerMod;
   double dVarModMass;
   char   szVarModChar[MAX_VARMOD_AA];
};

struct VarModParams
{
   int     bVarModSearch; 
   double  dVarModMassN;
   double  dVarModMassC;
   int     iVarModNtermDistance;
   int     iVarModCtermDistance;
   int     iMaxVarModPerPeptide;
   VarMods varModList[VMODS];
   char    cModCode[VMODS];
};

struct EnzymeInfo
{
   int  iAllowedMissedCleavage;

   int  iSearchEnzymeOffSet;
   char szSearchEnzymeName[48];
   char szSearchEnzymeBreakAA[MAX_ENZYME_AA];
   char szSearchEnzymeNoBreakAA[MAX_ENZYME_AA];

   int  iSampleEnzymeOffSet;
   char szSampleEnzymeName[48];
   char szSampleEnzymeBreakAA[MAX_ENZYME_AA];
   char szSampleEnzymeNoBreakAA[MAX_ENZYME_AA];
};

struct MassUtil
{
   int    bMonoMassesParent;
   int    bMonoMassesFragment;
   double dCO;
   double dNH3;
   double dNH2;
   double dH2O;
   double dCOminusH2;
   double dOH2fragment;
   double dOH2parent;
   double pdAAMassParent[128];
   double pdAAMassFragment[128];
};

struct ToleranceParams
{
   int    iMassToleranceUnits;    // 0=ppm, 1=da (default)
   int    iMassToleranceType;     // 0=MH+ (default), 1=precursor m/z
   int    iIsotopeError;
   double dInputTolerance;        // tolerance from param file
   double dFragmentBinSize;
   double dFragmentBinStartOffset;
   double dMatchPeakTolerance;
};

struct PeaksInfo
{
   int iNumMatchPeaks;
   int iNumAllowedMatchPeakErrors;
};

struct IonInfo
{
   int iNumIonSeriesUsed;
   int piSelectedIonSeries[9];
   int bUseNeutralLoss;
   int iTheoreticalFragmentIons;
   int iIonVal[9];
};

struct DateTimeInfo
{
   char szDate[28];
};

// static user params, won't change per thread - can make global!
struct StaticParams
{
   char            szHostName[SIZE_FILE];
   char            szTimeBuf[200];
   char            szIonSeries[200];   // used for .out files
   char            szDisplayLine[200]; // used for .out files
   char            szMod[280];         // used for .out files
   int             iElapseTime;
   DateTimeInfo    _dtInfoStart;
   Options         options;
   DBInfo          databaseInfo;
   InputFileInfo   inputFile;
   int             bPrintDuplReferences;
   VarModParams    variableModParameters;
   ToleranceParams tolerances;
   StaticMod       staticModifications;
   PrecalcMasses   precalcMasses;
   EnzymeInfo      enzymeInformation;
   MassUtil        massUtility;
   double          dBinWidth;
   double          dBinWidthMinusOffset;  // this is used in BIN() many times so calculate once
   PeaksInfo       peaksInformation;
   IonInfo         ionInformation;
};

extern StaticParams g_StaticParams;


// Query stores information for peptide scoring and results
// This struct is allocated for each spectrum/charge combination
struct Query
{
   int   iCorrelationHistogram[HISTO_SIZE];
   int   iDoXcorrCount;
   float fPar[4];           // parameters of LMA regression

   int   iDecoyCorrelationHistogram[HISTO_SIZE];
   int   iDoDecoyXcorrCount;
   float fDecoyPar[4];      // parameters of LMA regression

   short siMaxXcorr;        // index of maximum correlation score in iCorrelationHistogram
   short siMaxDecoyXcorr;   // index of maximum correlation score in iDecoyCorrelationHistogram

   short siLowestSpScoreIndex;
   short siLowestDecoySpScoreIndex;

   float fLowestSpScore;
   float fLowestDecoySpScore;

   float fLowestCorrScore;
   float fLowestDecoyCorrScore;

   unsigned long int  _liNumMatchedPeptides;
   unsigned long int  _liNumMatchedDecoyPeptides;

   float *pfSpScoreData;
   float *pfFastXcorrData;
   float *pfFastXcorrDataNL;  // pfFastXcorrData with NH3, H2O contributions

   PepMassInfo          _pepMassInfo;
   SpectrumInfoInternal _spectrumInfoInternal;
   Results              *_pResults;
   Results              *_pDecoys;

   Mutex  accessMutex;

   Query()
   {
      for (int i = 0; i < HISTO_SIZE; i++)
      {
         iCorrelationHistogram[i] = 0;
         iDecoyCorrelationHistogram[i] = 0;
      }

      iDoXcorrCount = 0;
      fPar[0]=0.0;
      fPar[1]=0.0;
      fPar[2]=0.0;

      iDoDecoyXcorrCount = 0;
      fDecoyPar[0]=0.0;
      fDecoyPar[1]=0.0;
      fDecoyPar[2]=0.0;

      siMaxXcorr = 0;                        // index of maximum correlation score in iCorrelationHistogram
      siMaxDecoyXcorr = 0;                   // index of maximum correlation score in iDecoyCorrelationHistogram
      siLowestSpScoreIndex = 0;
      siLowestDecoySpScoreIndex = 0;

      fLowestSpScore = 0.0;
      fLowestDecoySpScore = 0.0;

      fLowestCorrScore = 0.0;
      fLowestDecoyCorrScore = 0.0;

      _liNumMatchedPeptides = 0;
      _liNumMatchedDecoyPeptides = 0;

      pfSpScoreData = NULL;
      pfFastXcorrData = NULL;
      pfFastXcorrDataNL= NULL;           // pfFastXcorrData with NH3, H2O contributions

      _pepMassInfo.dCalcPepMass = 0;
      _pepMassInfo.dExpPepMass = 0;
      _pepMassInfo.dPeptideMassTolerance = 0;
      _pepMassInfo.dPeptideMassToleranceMinus = 0;
      _pepMassInfo.dPeptideMassTolerancePlus = 0;
   
      _spectrumInfoInternal.dTotalIntensity = 0;
      _spectrumInfoInternal.iArraySize = 0;
      _spectrumInfoInternal.iHighestIon = 0;
      _spectrumInfoInternal.iScanNumber = 0;
      _spectrumInfoInternal.dTotalIntensity = 0;

      _pResults = NULL;
      _pDecoys = NULL;

      Threading::CreateMutex(&accessMutex);
   }

   ~Query()
   {
      if (NULL != pfFastXcorrData)
         free(pfFastXcorrData);

      if (NULL != pfFastXcorrDataNL)
         free(pfFastXcorrDataNL);

      if (NULL != pfSpScoreData)
         free(pfSpScoreData);

      if (NULL != _pResults)
         free(_pResults);

      if (g_StaticParams.options.iDecoySearch==2)
      {
         if (NULL != _pDecoys)
            free(_pDecoys);
      }

      Threading::DestroyMutex(accessMutex);
   }
};

extern vector <Query*>  g_pvQuery;
extern Mutex            g_pvQueryMutex;

struct IonSeriesStruct         // defines which fragment ion series are considered
{
   int bPreviousMatch[8];
};

#endif // _COMETDATA_H_
