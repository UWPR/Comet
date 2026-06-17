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

// Parameter structs: Options, DBInfo, StaticParams, and their sub-structs.
// Depends on: core/Constants.h, CometData.h

#ifndef _COMETPARAMS_H_
#define _COMETPARAMS_H_

#include <chrono>
#include <map>
#include <string>
#include <vector>
#include "core/Constants.h"
#include "CometData.h"

using std::string;
using std::vector;
using std::multimap;

class CometSearchManager;

struct Options
{
   int iNumPeptideOutputLines;
   int iWhichReadingFrame;
   int iEnzymeTermini;
   int iNumStored;               // # of search results to store for xcorr analysis
   int iMaxDuplicateProteins;    // maximum number of duplicate proteins to report or store in idx file
   int iSpectrumBatchSize;       // # of spectra to search at a time within the scan range
   int iStartCharge;
   int iEndCharge;
   int iMaxFragmentCharge;
   int iMinPrecursorCharge;
   int iMaxPrecursorCharge;
   int iMSLevel;                 // filter query scans in raw/mzML/mzXML input by ms level (aka MS2, MS3)
   int iSpecLibMSLevel;          // filter speclib scans in raw/mzML/mzXML input by ms level (aka MS2, MS3)
   int iMinPeaks;
   int iRemovePrecursor;         // 0=no, 1=yes, 2=ETD precursors, 3=phosphate neutral loss
   int iDecoySearch;             // 0=no, 1=concatenated search, 2=separate decoy search
   int iNumThreads;              // 0=poll CPU else set # threads to spawn
   int iNumFragmentThreads;      // # threads used for fragment indexing
   bool bResolveFullPaths;       // 0=do not resolve full paths; 1=resolve paths (default)
   bool bOutputSqtStream;
   bool bOutputSqtFile;
   bool bOutputTxtFile;
   bool bOutputPepXMLFile;
   int iOutputMzIdentMLFile;
   bool bOutputPercolatorFile;
   bool bClipNtermMet;           // 0=leave protein sequences alone; 1=also consider w/o N-term methionine
   bool bClipNtermAA;            // 0=leave peptide sequences as-is; 1=clip N-term amino acid from every peptide
   bool bMango;                  // 0=normal; 1=Mango x-link ms2 input
   bool bScaleFragmentNL;        // 0=no; 1=scale fragment NL for each modified residue contained in fragment
   bool bCreateFragmentIndex;    // 0=normal search; 1=create fragment ion index plain peptide file
   bool bCreatePeptideIndex;     // 0=normal search; 1=create peptide index file; only one of bCreateFragmentIndex and bCreatePeptideIndex can be 1
   bool bFastPlainPeptideIdx;    // 0=legacy RunSearch path; 1=use PepGenTuple per-thread buffers (avoids heap alloc)
   bool bVerboseOutput;
   bool bExplicitDeltaCn;        // if set to 1, do not use sequence similarity logic
   bool bPrintExpectScore;
   bool bExportAdditionalScoresPepXML;  // if 1, also report lnrSp, lnExpect, IonFrac, lnNumSP to pepXML output
   bool bCorrectMass;            // use selectionMZ instead of monoMZ if monoMZ is outside selection window
   bool bTreatSameIL;
   int iPrintAScoreProScore;    // 0=no, otherwise specify variable_modXX number e.g. 1 for variable_mod01
   int iMaxIndexRunTime;         // max run time of index search in milliseconds
   int iFragIndexMinIonsScore;   // minimum matched fragment index ions for scoring
   int iFragIndexMinIonsReport;  // minimum matched fragment index ions for reporting
   int iFragIndexNumSpectrumPeaks;   // # of peaks from spectrum to use for querying fragment index
   int iFragIndexSkipReadPrecursors; // if true, skips reading precursors step
   int iOverrideCharge;
   long lMaxIterations;          // max # of modification permutations for each iStart position
   double dMinIntensity;         // intensity cutoff for each peak
   double dMinPercentageIntensity;   // intensity cutoff for each peak as % of base peak
   double dRemovePrecursorTol;
   double dPeptideMassLow;       // MH+ mass
   double dPeptideMassHigh;      // MH+ mass
   double dMinimumXcorr;         // set the minimum xcorr to report (default is 1e-8)
   double dFragIndexMaxMass;     // fragment index maximum fragment mass
   double dFragIndexMinMass;     // fragment index minimum fragment mass
   double dMS1MinMass;           // low mass cutoff in MS1 query/library spectra
   double dMS1MaxMass;           // high mass cutoff in MS1 query/library spectra
   IntRange scanRange;
   IntRange peptideLengthRange;
   DoubleRange clearMzRange;
   char szActivationMethod[24];  // mzXML only
   string sPinProteinDelimiter;  // PIN file protein delimiter; default tab

   Options& operator=(const Options&) = default;
};

// The minimum and maximum mass range of all peptides to consider
// i.e. lowestPepMass - tolerance to highestPepMass + tolerance
struct MassRange
{
   double dMinMass;
   double dMaxMass;
   unsigned short    usiMaxFragmentCharge;  // global maximum fragment charge
   bool   bNarrowMassRange;    // used to determine how to parse peptides in SearchForPeptides
   unsigned int uiMaxFragmentArrayIndex; // BIN(dFragIndexMaxMass); used as fragment array index
};

extern MassRange g_massRange;

struct DBInfo
{
   char   szDatabase[SIZE_FILE];
   char   szFileName[SIZE_FILE];
   int    iTotalNumProteins;
   unsigned long int uliTotAACount;

   DBInfo& operator=(const DBInfo&) = default;
};

struct SpecLibInfo      // why a struct for just a string???
{
   string strSpecLibFile;
};

struct PEFFInfo
{
   char   szPeffOBO[SIZE_FILE];
   int    iPeffSearch;               // 0=no, 1=PSI-MOD, 2=Unimod, 3=PSI-MOD only, 4=Unimod only, 5=variants only
};

struct StaticMod
{
   double dAddCterminusPeptide;
   double dAddNterminusPeptide;
   double dAddCterminusProtein;
   double dAddNterminusProtein;
   double pdStaticMods[SIZE_MASS];

   StaticMod& operator=(const StaticMod&) = default;
};

struct PrecalcMasses
{
   double dNtermProton;          // dAddNterminusPeptide + PROTON_MASS
   double dCtermOH2Proton;       // dAddCterminusPeptide + dOH2fragment + PROTON_MASS
   double dOH2ProtonCtermNterm;  // dOH2parent + PROTON_MASS + dAddCterminusPeptide + dAddNterminusPeptide
   int    iMinus17;              // BIN'd value of mass(NH3)
   int    iMinus18;              // BIN'd value of mass(H2O)

   PrecalcMasses& operator=(const PrecalcMasses&) = default;
};

struct VarModParams
{
   bool    bVarModSearch;            // set to true if variable mods are specified
   bool    bVarTermModSearch;        // set to true if any n-term/c-term variable mods are specified
   bool    bVarProteinNTermMod;      // set to true if a protein n-term variable mod specified
   bool    bVarProteinCTermMod;      // set to true if a protein c-term variable mod specified
   bool    bBinaryModSearch;         // set to true if any of the variable mods are of binary mod variety
   bool    bUseFragmentNeutralLoss;  // set to true if any custom NL is set; applied only to 1+ and 2+ fragments
   bool    bRareVarModPresent;       // set to true if any of iRequireThisMod == -1
   bool    bVarModProteinFilter;     // set to trueif protein mods list is applied
   int     iRequireVarMod;           // 0=no; else use bits to determine which varmods are required
   int     iMaxVarModPerPeptide;
   int     iMaxPermutations;
   VarMods varModList[VMODS];
   char    cModCode[VMODS];          // mod characters
   string  sProteinLModsListFile;                 // file containing list of proteins to restrict application of varmods to
   multimap<int, string> mmapProteinModsList;     // <varmod#, protein name> vector read from sProteinModsListFile if present
   string         sCompoundModsFile;              // path to compound mods mass file; empty = disabled
   vector<double> vdCompoundMasses;               // sorted, deduplicated list of masses read from sCompoundModsFile
   unsigned int   uiNumCompoundMasses;            // vdCompoundMasses.size(); 0 when feature is disabled

   VarModParams& operator=(const VarModParams&) = default;
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
   double pdAAMassParent[SIZE_MASS];
   double pdAAMassFragment[SIZE_MASS];
   double pdAAMassUser[SIZE_MASS];       // user defined default amino acid masses

   MassUtil& operator=(const MassUtil&) = default;
};

struct ToleranceParams
{
   int    iMassToleranceUnits;    // 0=amu, 1=mmu, else ppm (2)
   int    iMassToleranceType;     // 0=MH+ (default), 1=precursor m/z; only valid if iMassToleranceUnits > 0
   int    iIsotopeError;
   double dInputToleranceMinus;   // raw tolerance value from param file, lower bound; gets converted to dPeptideMassToleranceMinus
   double dInputTolerancePlus;    // raw tolerance value from param file, upper bound; gets converted to dPeptideMassTolerancePlus
   double dFragmentBinSize;
   double dFragmentBinStartOffset;
   double dMS1BinSize;
   double dMS1BinStartOffset;

   ToleranceParams& operator=(const ToleranceParams&) = default;
};

struct IonInfo
{
   int iNumIonSeriesUsed;
   int piSelectedIonSeries[NUM_ION_SERIES];
   bool bUseWaterAmmoniaLoss;    // ammonia, water loss
   int iTheoreticalFragmentIons;
   int iIonVal[NUM_ION_SERIES];

   IonInfo& operator=(const IonInfo&) = default;
};

// static user params, won't change per thread - can make global!
struct StaticParams
{
   string          sHostName;
   char            szMod[512];         // used for sqt output
   char            szDecoyPrefix[256]; // used for prefix to indicate decoys
   string          sDecoyPrefix;       // escaped version of szDecoyPrefix for output within XML files
   char            szOutputSuffix[256]; // used for suffix to append to output file base names
   char            szTxtFileExt[256];  // text file extension; default "txt"
   int             iElapseTime;
   char            szDate[32];
   Options         options;
   DBInfo          databaseInfo;
   SpecLibInfo     speclibInfo;
   PEFFInfo        peffInfo;
   InputFileInfo   inputFile;
   int             bPrintDuplReferences;
   VarModParams    variableModParameters;
   ToleranceParams tolerances;
   StaticMod       staticModifications;
   PrecalcMasses   precalcMasses;
   EnzymeInfo      enzymeInformation;
   MassUtil        massUtility;
   double          dInverseBinWidth;    // this is used in BIN() many times so use inverse binWidth to do multiply vs. divide
   int             iArraySizeGlobal;    // (int)((g_staticParams.options.dPeptideMassHigh + plus_tol_in_daltons + buffer) * g_staticParams.dInverseBinWidth)
                                        // for MS1 library search, use dMS1MaxMass instead of dPeptideMassHigh
   double          dOneMinusBinOffset;  // this is used in BIN() many times so calculate once
   IonInfo         ionInformation;
   int             iXcorrProcessingOffset;
   DbType          iDbType;            // FASTA_DB = normal fasta; FI_DB = fragment ion indexed; PI_DB = peptide index
   vector<double>  vectorMassOffsets;
   vector<double>  precursorNLIons;
   int             iPrecursorNLSize;
   int             iOldModsEncoding;
   bool            bSkipToStartScan;
   std::chrono::high_resolution_clock::time_point tRealTimeStart;     // track run time of real-time index search

   StaticParams()
   {
       RestoreDefaults();
   }

   StaticParams& operator=(const StaticParams&) = default;

   void RestoreDefaults()
   {
      int i;

      inputFile.iInputType = InputType_MS2;

      szMod[0] = '\0';

      iXcorrProcessingOffset = 75;
      iDbType = DbType::FASTA_DB;

      databaseInfo.szDatabase[0] = '\0';
      speclibInfo.strSpecLibFile.clear();

      strcpy(szDecoyPrefix, "DECOY_");
      strcpy(szTxtFileExt, "txt");
      szOutputSuffix[0] = '\0';

      peffInfo.szPeffOBO[0] = '\0';
      peffInfo.iPeffSearch = 0;

      variableModParameters.sCompoundModsFile = "";
      variableModParameters.vdCompoundMasses.clear();
      variableModParameters.uiNumCompoundMasses = 0;

      iPrecursorNLSize = 0;

      for (i = 0; i < SIZE_MASS; ++i)
      {
         massUtility.pdAAMassParent[i] = 999999.;
         massUtility.pdAAMassFragment[i] = 999999.;
         massUtility.pdAAMassUser[i] = 0.0;
         staticModifications.pdStaticMods[i] = 0.0;
      }

      massUtility.bMonoMassesFragment = 1;
      massUtility.bMonoMassesParent = 1;

#ifdef CRUX
      staticModifications.pdStaticMods[(int)'C'] = 57.021464;
#endif


      enzymeInformation.iAllowedMissedCleavage = 2;

      for (i = 0; i < VMODS; ++i)
      {
         variableModParameters.varModList[i].iMaxNumVarModAAPerMod = 3;
         variableModParameters.varModList[i].iMinNumVarModAAPerMod = 0;
         variableModParameters.varModList[i].iBinaryMod = 0;
         variableModParameters.varModList[i].iRequireThisMod = 0;
         variableModParameters.varModList[i].iVarModTermDistance = -1;   // distance from N or C-term distance
         variableModParameters.varModList[i].iWhichTerm = 0;             // specify N (0) or C-term (1)
         variableModParameters.varModList[i].dVarModMass = 0.0;
         variableModParameters.varModList[i].dNeutralLoss = 0.0;
         variableModParameters.varModList[i].dNeutralLoss2 = 0.0;
         strcpy(variableModParameters.varModList[i].szVarModChar, "X");

#ifdef CRUX
         if (i==0)
         {
            variableModParameters.varModList[i].dVarModMass = 15.9949;
            strcpy(variableModParameters.varModList[i].szVarModChar, "M");
         }
#endif
      }

      variableModParameters.cModCode[0] = '*';
      variableModParameters.cModCode[1] = '#';
      variableModParameters.cModCode[2] = '@';
      variableModParameters.cModCode[3] = '^';
      variableModParameters.cModCode[4] = '~';
      variableModParameters.cModCode[5] = '$';
      variableModParameters.cModCode[6] = '%';
      variableModParameters.cModCode[7] = '!';
      variableModParameters.cModCode[8] = '+';
      for (int i = 9; i < VMODS; ++i)
      {
         int iAscii = 88 + i;    //start with lower case 'a' ASCII 97
         if (iAscii <= 125)      // thru '}' which is ASCII 125
            variableModParameters.cModCode[i] = (char)(iAscii);
         else
            variableModParameters.cModCode[i] = '_';
      }

      variableModParameters.iMaxVarModPerPeptide = 5;
      variableModParameters.iMaxPermutations = MAX_PERMUTATIONS;
      variableModParameters.bUseFragmentNeutralLoss = false;
      variableModParameters.iRequireVarMod = 0;

      ionInformation.bUseWaterAmmoniaLoss = false;
      ionInformation.iTheoreticalFragmentIons = 1;      // 0 = flanking peaks; 1 = no flanking peaks
      ionInformation.iIonVal[ION_SERIES_A] = 0;
      ionInformation.iIonVal[ION_SERIES_B] = 1;
      ionInformation.iIonVal[ION_SERIES_C] = 0;
      ionInformation.iIonVal[ION_SERIES_X] = 0;
      ionInformation.iIonVal[ION_SERIES_Y] = 1;
      ionInformation.iIonVal[ION_SERIES_Z] = 0;
      ionInformation.iIonVal[ION_SERIES_Z1] = 0;

      options.iNumPeptideOutputLines = 5;
      options.iWhichReadingFrame = 0;
      options.iEnzymeTermini = 2;
      options.iNumStored = 100;                         // default # of search results to store for xcorr analysis.
      options.iMaxDuplicateProteins = 20;               // maximum number of duplicate proteins to report or store in idx file

      options.bExplicitDeltaCn = false;
      options.bPrintExpectScore = true;
      options.iPrintAScoreProScore = 0;
      options.bExportAdditionalScoresPepXML = false;
      options.bCorrectMass = false;
      options.bTreatSameIL = true;
      options.iOverrideCharge = 0;
      options.iMaxIndexRunTime = 0;                     // index run time limit in milliseconds; 0=no time limit
      options.iRemovePrecursor = 0;
      options.dRemovePrecursorTol = 1.5;

      options.bOutputSqtStream = false;
      options.bOutputSqtFile = false;
      options.bOutputTxtFile = false;
      options.bOutputPepXMLFile = true;
      options.iOutputMzIdentMLFile = false;
      options.bOutputPercolatorFile = false;

      options.bResolveFullPaths = true;

      options.bMango = false;
      options.bScaleFragmentNL = false;
      options.bCreatePeptideIndex = false;
      options.bCreateFragmentIndex = false;
      options.bFastPlainPeptideIdx = false;
      options.bVerboseOutput = false;
      options.iDecoySearch = 0;
      options.iNumThreads = 4;
      options.iNumFragmentThreads = 4;
      options.bClipNtermMet = false;
      options.bClipNtermAA = false;

      options.lMaxIterations = 0;

      // These parameters affect mzXML/RAMP spectra only.
      options.scanRange.iStart = 0;
      options.scanRange.iEnd = 0;
      options.iSpectrumBatchSize = 0;
      options.iMinPeaks = 10;
      options.iStartCharge = 0;
      options.iEndCharge = 0;
      options.iMaxFragmentCharge = 3;
      options.iMinPrecursorCharge = 1;
      options.iMaxPrecursorCharge = 6;
      options.iMSLevel = 2;
      options.dMinIntensity = 0.0;
      options.dMinPercentageIntensity = 0.0;
      options.dPeptideMassLow = 600.0;
      options.dPeptideMassHigh = 5000.0;
      options.dMinimumXcorr = XCORR_CUTOFF;
      options.dFragIndexMaxMass = FRAGINDEX_MAX_MASS;
      options.dFragIndexMinMass = FRAGINDEX_MIN_MASS;
      strcpy(options.szActivationMethod, "ALL");
      // End of mzXML specific parameters.

      options.sPinProteinDelimiter = '\t';

      options.dFragIndexMinMass = FRAGINDEX_MIN_MASS;
      options.dFragIndexMaxMass = FRAGINDEX_MAX_MASS;
      options.iFragIndexMinIonsScore = FRAGINDEX_MIN_IONS_SCORE;
      options.iFragIndexMinIonsReport = FRAGINDEX_MIN_IONS_REPORT;
      options.iFragIndexNumSpectrumPeaks = FRAGINDEX_MAX_NUMPEAKS;
      options.iFragIndexSkipReadPrecursors = 1;   // skip reading precursors by default

      options.dMS1MinMass = MS1_MIN_MASS;
      options.dMS1MaxMass = MS1_MAX_MASS;

      options.clearMzRange.dStart = 0.0;
      options.clearMzRange.dEnd = 0.0;

      options.peptideLengthRange.iStart = MIN_PEPTIDE_LEN;
      options.peptideLengthRange.iEnd = MAX_PEPTIDE_LEN - 1;  // -1 as MAX_PEPTIDE_LEN number includes terminating char

      staticModifications.dAddCterminusPeptide = 0.0;
      staticModifications.dAddNterminusPeptide = 0.0;
      staticModifications.dAddCterminusProtein = 0.0;
      staticModifications.dAddNterminusProtein = 0.0;

      tolerances.iMassToleranceUnits = 0;
      tolerances.iMassToleranceType = 0;
      tolerances.iIsotopeError = 0;
      tolerances.dInputToleranceMinus = -3.0;               // peptide_mass_tolerance minus
      tolerances.dInputTolerancePlus = 3.0;                 // peptide_mass_tolerance plus
      tolerances.dFragmentBinSize = 1.0005;
      tolerances.dFragmentBinStartOffset = 0.4;
      tolerances.dMS1BinSize = 1.0005;

      bSkipToStartScan = true;
   }
};

extern StaticParams    g_staticParams;

#endif // _COMETPARAMS_H_
