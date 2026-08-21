// Copyright 2012-2026 Jimmy Eng
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


#include <cstdarg>
#include "Common.h"
#include "CometMassSpecUtils.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometPreprocess.h"
#include "CometWriteSqt.h"
#include "CometWriteTxt.h"
#include "CometWritePepXML.h"
#include "CometWriteMzIdentML.h"
#include "CometWritePercolator.h"
#include "output/IResultWriter.h"
#include "output/SqtWriter.h"
#include "output/TxtWriter.h"
#include "output/PepXmlWriter.h"
#include "output/MzIdentMlWriter.h"
#include "output/PercolatorWriter.h"
#include "CometDataInternal.h"
#include "CometSearchManager.h"
#include "CometStatus.h"
#include "CometFragmentIndex.h"
#include "CometPeptideIndex.h"
#include "CometSpecLib.h"
#include "CometAlignment.h"
#include "AScoreOptions.h"
#include "AScoreFactory.h"
#include "search/SearchSession.h"
#include "search/SearchUtils.h"
#include "search/ISearchStrategy.h"
#include "search/FiStrategy.h"
#include "search/FastaStrategy.h"
#include "search/PiStrategy.h"
#include "search/Pipeline.h"

#include <sstream>
#include <cstdio>
#include <cstdlib>    // _exit()


extern comet_fileoffset_t clSizeCometFileOffset;

std::vector<InputFileInfo *>  g_pvInputFiles;
StaticParams                  g_staticParams;
vector<DBIndex>                                  g_pvDBIndex;
vector<vector<vector<PepGenTupleShort>>>         g_vvvPepGenShort;
vector<vector<vector<PepGenTuple>>>              g_vvvPepGenLong;
MassRange                     g_massRange;
Mutex                         g_pvQueryMutex;
Mutex                         g_pvDBIndexMutex;
Mutex                         g_preprocessMemoryPoolMutex;
Mutex                         g_ms1AlignerMutex;
CometStatus                   g_cometStatus;
string                        g_sCometVersion;
map<long long, IndexProteinStruct>    g_pvProteinNames;  // for either db index

unordered_map<comet_fileoffset_t, string> g_pvProteinNameCache;  // populated at index load; eliminates per-spectrum fopen in RTS path

AScoreProCpp::AScoreOptions   g_AScoreOptions;  // AScore options
// Thread-safety note - g_AScoreInterface is shared across PostAnalysis threads.
// AScoreDllInterface::CalculateScoreWithOptions() is assumed to be thread-safe because
// it does not modify any mutable member state; all intermediate computation uses local
// variables. If AScorePro is ever changed to use internal mutable state, this must be
// protected with a mutex or made thread-local.
AScoreProCpp::AScoreDllInterface* g_AScoreInterface;

ProteinsListCSR g_pvProteinsList;

// Fragment index globals - INITIALIZED ONCE, READ-ONLY DURING SEARCH
unsigned int* g_iFragmentIndex;                             // CSR flat data: concatenated posting lists
uint64_t*     g_iFragmentIndexOffset;                       // CSR offsets [uiMaxFragmentArrayIndex+1]
bool* g_bIndexPrecursors;                                   // array for BIN(precursors), set to true if precursor present in file
vector<struct FragmentPeptidesStruct> g_vFragmentPeptides;  // each peptide is represented here iWhichPeptide, which mod if any, calculated mass
vector<PlainPeptideIndexStruct> g_vRawPeptides;             // list of unmodified peptides and their proteins as file pointers
vector<struct FragmentPeptidesStruct> g_vDBIndexVariants;   // PI_DB's compact per-variant array, regenerated once per session (docs/20260730_PI_reduction.md Phase 0.5); see core/Types.h
vector<vector<unsigned int>> g_vulSpecLibPrecursorIndex;    // mass index for SpecLib
vector<SpecLibStruct> g_vSpecLib;                           // stores the SpecLib

std::atomic<bool> g_bPlainPeptideIndexRead = false;
std::atomic<bool> g_bPeptideIndexRead = false;
// Separate from g_bPeptideIndexRead: CometPeptideIndex::ReadPeptideIndex() sets that flag
// at its own end, before EnsurePeptideIndexLoaded() (CometSearch.cpp) goes on to also run
// InitializeMassesFromPeptideIndex() and (if enabled) create the AScorePro interface --
// this flag is set only once ALL of that has completed, so EnsurePeptideIndexLoaded()'s
// unlocked fast-path check can't observe "read" as true while masses/AScorePro are still
// being set up on another thread. See EnsurePeptideIndexLoaded()'s own comment for detail.
std::atomic<bool> g_bPeptideIndexFullyInitialized = false;
std::atomic<bool> g_bSpecLibRead = false;
bool g_bPerformSpecLibSearch = false;
bool g_bPerformDatabaseSearch = false;
bool g_bCometPreprocessMemoryAllocated = false;
bool g_bCometSearchMemoryAllocated = false;

bool g_bIdxNoFasta = false;  // if true, .idx file has no associated .fasta file

// FI Thread-safety notes:
// - Index is created once in InitializeSingleSpectrumSearch() under mutex protection
// - All search threads access index READ-ONLY
// - No modifications after g_bPlainPeptideIndexRead = true

double dMaxSpecLibRT = 0.0;

// Reset the MS1 run alignment for new run
// Keep past MS1_RT_HISTORY_SIZE RTs and set outlier threshold to MS1_RT_OUTLIER_THRESHOLD stdevs
CometMassSpecAligner pMS1Aligner(MS1_RT_HISTORY_SIZE, MS1_RT_OUTLIER_THRESHOLD);
std::deque<RetentionMatch> RetentionMatchHistory;

/******************************************************************************
*
* Static helper functions
*
******************************************************************************/
static std::string GetHostName()
{
   char hostname[128] = { 0 };

#ifdef _WIN32
   WSADATA wsaData;
   if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
   {
      if (gethostname(hostname, sizeof(hostname)) == 0)
      {
         WSACleanup();
         std::string fullHost(hostname);
         size_t pos = fullHost.find('.');
         if (pos != std::string::npos)
            return fullHost.substr(0, pos);
         else
            return fullHost;
      }
      WSACleanup();
   }
#else
   if (gethostname(hostname, sizeof(hostname)) == 0)
   {
      std::string fullHost(hostname);
      size_t pos = fullHost.find('.');
      if (pos != std::string::npos)
         return fullHost.substr(0, pos);
      else
         return fullHost;
   }
#endif
   return {};
}

static bool ValidateOutputFormat()
{
   if (!g_staticParams.options.bOutputSqtStream
         && !g_staticParams.options.bOutputSqtFile
         && !g_staticParams.options.bOutputTxtFile
         && !g_staticParams.options.bOutputPepXMLFile
         && !g_staticParams.options.iOutputMzIdentMLFile
         && !g_staticParams.options.bOutputPercolatorFile)
   {
      string strErrorMsg = " Please specify at least one output format.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   return true;
}

static bool ValidateSequenceDatabaseFile()
{
   FILE *fpcheck;

   // open FASTA for retrieving protein names
   string sTmpDB = g_staticParams.databaseInfo.szDatabase;

   size_t databaseLen = strlen(g_staticParams.databaseInfo.szDatabase);
   // strcmp returns 0 (falsy) when the suffix IS ".idx" -- negate it so the erase actually
   // runs on an .idx input (stripping the extension to get the plain FASTA name) instead of
   // on every non-.idx input, which truncated a real FASTA path's last 4 characters. Inert
   // today: sTmpDB only ever feeds g_bIdxNoFasta below, which has no readers.
   if (databaseLen >= 4 && !strcmp(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
      sTmpDB = sTmpDB.erase(sTmpDB.size() - 4); // need plain fasta if indexdb input

   if ((fpcheck = fopen(sTmpDB.c_str(), "r")) == NULL)
   {
      g_bIdxNoFasta = true;  // .idx database but corresponding fasta not found
   }
   else
   {
      fclose(fpcheck);
      g_bIdxNoFasta = false;
   }

   // if .idx database specified but does not exist, first see if corresponding
   // fasta exists and if it does, create the .idx file
   if (databaseLen >= 4 &&  strstr(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
   {
      if ((fpcheck=fopen(g_staticParams.databaseInfo.szDatabase, "r")) == NULL)
      {
         string strFasta = g_staticParams.databaseInfo.szDatabase;
         strFasta.erase(strFasta.length() - 4);  // remove .idx extension

         if ((fpcheck=fopen(strFasta.c_str(), "r")) == NULL)
         {
            string strErrorMsg = " Error - peptide index file \"" + std::string(g_staticParams.databaseInfo.szDatabase)
               + "\" and corresponding FASTA file are both missing.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         else
         {
            fclose(fpcheck);

            // The .idx doesn't exist yet, so there's nothing to read a mode from --
            // index_search_type is the only remaining consumer of that parameter
            // (docs/20260811_restore_idx_header_mods.md): it picks which format to
            // auto-build here. Once written, the file records its own mode in its
            // IndexSearchType: header line and no parameter is needed to search it
            // (see the "file already exists" branch below).
            if (g_staticParams.options.iIndexSearchType == 0)
            {
               g_staticParams.options.bCreatePeptideIndex = true;
               g_staticParams.iDbType = DbType::PI_DB;
            }
            else
            {
               g_staticParams.options.bCreateFragmentIndex = true;  // set to true to make the index
               g_staticParams.iDbType = DbType::FI_DB;
            }
            return true;
         }
      }
      else
      {
         // The file already exists, so it's self-describing: g_staticParams.iDbType was
         // already set from its own IndexSearchType: header line by InitializeStaticParams()
         // (which always runs before this function -- see its ".idx extension" block), via
         // CometPeptideIndex::ParsePeptideIndexHeader(). No index_search_type parameter is
         // consulted here.
         fclose(fpcheck);
         g_staticParams.options.bCreateFragmentIndex = false;
         g_staticParams.options.bCreatePeptideIndex = false;
         return true;
      }
   }

#ifndef WIN32
   // do a quick test if specified file is a directory
   struct stat st;
   stat(g_staticParams.databaseInfo.szDatabase, &st );

   if (S_ISDIR( st.st_mode )) 
   {
      string strErrorMsg = " Error - specified database file is a directory: \""
         + std::string(g_staticParams.databaseInfo.szDatabase) + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
   if (!(S_ISREG( st.st_mode ) || S_ISLNK( st.st_mode )))
   {
      string strErrorMsg = " Error - specified database file is not a regular file or symlink: \""
         + std::string(g_staticParams.databaseInfo.szDatabase) + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
#endif

   // Quick sanity check to make sure sequence db file is present before spending
   // time reading & processing spectra and then reporting this error.
   if ((fpcheck = fopen(g_staticParams.databaseInfo.szDatabase, "r")) == NULL)
   {
      string strErrorMsg = " Error (2) - cannot read FASTA sequence database file \""
         + std::string(g_staticParams.databaseInfo.szDatabase) + "\".\n Check that the file exists and is readable.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   fclose(fpcheck);

   return true;
}

static bool ValidateSpecLibFile()  // just check if file is readable for now
{
   FILE* fpcheck;

   // open speclib file
   if ((fpcheck = fopen(g_staticParams.speclibInfo.strSpecLibFile.c_str(), "r")) == NULL)
   {
      string strErrorMsg = " Error (5) - cannot read spectral library file \""
         + g_staticParams.speclibInfo.strSpecLibFile + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   fclose(fpcheck);

   return true;
}

static bool ValidateScanRange()
{
   if (g_staticParams.options.scanRange.iEnd < g_staticParams.options.scanRange.iStart
         && g_staticParams.options.scanRange.iEnd != 0)
   {
      string strErrorMsg = " Error - start scan is "
         + std::to_string(g_staticParams.options.scanRange.iStart)
         + "but end scan is " + std::to_string(g_staticParams.options.scanRange.iEnd)
         + ".\n The end scan must be >= to the start scan.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   return true;
}

static bool ValidatePeptideLengthRange()
{
   if (g_staticParams.options.peptideLengthRange.iEnd < g_staticParams.options.peptideLengthRange.iStart
         && g_staticParams.options.peptideLengthRange.iEnd != 0)
   {
      string strErrorMsg = " Error - peptide length range set as "
         + std::to_string(g_staticParams.options.peptideLengthRange.iStart) + " to "
         + std::to_string(g_staticParams.options.peptideLengthRange.iEnd)
         + ".\n The maximum length must be >= to the minimum length.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   return true;
}

/******************************************************************************
*
* CometSearchManager class implementation.
*
******************************************************************************/

CometSearchManager::CometSearchManager() :
   singleSearchInitializationComplete(false),
   singleSearchMS1InitializationComplete(false),
   staticParamsInitializationComplete(false),
   m_bRTSIndexBuild(false)
{
   // Initialize the mutexes we'll use to protect global data.
   Threading::InitMutex(&g_pvQueryMutex);

   // Initialize the mutexes we'll use to protect DBIndex.
   Threading::InitMutex(&g_pvDBIndexMutex);

   // Initialize the mutex we'll use to protect the preprocess memory pool
   Threading::InitMutex(&g_preprocessMemoryPoolMutex);

   // Initialize the mutex we'll use to protect the MS1 RT aligner
   Threading::InitMutex(&g_ms1AlignerMutex);

   // Initialize the Comet version
   SetParam("# comet_version", comet_version, comet_version);
   _tp = new ThreadPool();

   // Wire up the error handler so that uncaught exceptions in thread pool
   // tasks are propagated to g_cometStatus for the main thread to detect.
   _tp->setErrorHandler([](const std::string& strErrorMsg) {
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      });
}

CometSearchManager::~CometSearchManager()
{
   // Destroy the mutex we used to protect g_pvQueryMutex.
   Threading::DestroyMutex(g_pvQueryMutex);

   // Destroy the mutex we used to protect g_pvDBIndex.
   Threading::DestroyMutex(g_pvDBIndexMutex);

   // Destroy the mutex we used to protect the preprocess memory pool
   Threading::DestroyMutex(g_preprocessMemoryPoolMutex);

   // Destroy the mutex we used to protect the MS1 RT aligner
   Threading::DestroyMutex(g_ms1AlignerMutex);

   // g_pvInputFiles/_mapStaticParams are containers OF POINTERS (vector<InputFileInfo*>,
   // map<string, CometParam*>) -- clear() destroys the pointers themselves, not what they
   // point to (unlike a container of by-value objects, where the comment this replaces
   // would have been correct). AddInputFiles()/SetParam() are the only places these
   // pointers are ever created (Comet.cpp's ParseCmdLine() new's the InputFileInfo but
   // never deletes it, handing ownership to whichever CometSearchManager it's added to),
   // so freeing them here doesn't risk a double-free.
   for (InputFileInfo* p : g_pvInputFiles)
      delete p;
   g_pvInputFiles.clear();

   for (auto& kv : _mapStaticParams)
      delete kv.second;
   _mapStaticParams.clear();

   if (_tp != NULL)
      delete _tp;
   _tp = NULL;
}

// Appends to g_staticParams.szMod[512], bounded by its remaining space. The chain of
// sprintf(szMod + strlen(szMod), ...) calls this replaces below had no such bound: with a
// fully-populated 15-variable-mod (some with multi-character szVarModChar) + several
// static-mod + long-enzyme-name config, the cumulative appended length can exceed 512 bytes,
// overflowing this fixed global buffer. Truncates rather than errors, matching szMod's
// existing role as a display/output string (sqt header), not search-correctness state.
static void AppendSzMod(const char* fmt, ...)
{
   size_t len = strlen(g_staticParams.szMod);
   if (len >= sizeof(g_staticParams.szMod) - 1)
      return;

   va_list args;
   va_start(args, fmt);
   vsnprintf(g_staticParams.szMod + len, sizeof(g_staticParams.szMod) - len, fmt, args);
   va_end(args);
}

bool CometSearchManager::InitializeStaticParams()
{
   int iIntData;
   double dDoubleData;
   string strData;
   IntRange intRangeData;
   DoubleRange doubleRangeData;

   if (staticParamsInitializationComplete)
   {
      // FiStrategy::finalize() frees g_bIndexPrecursors and nulls it out at the end of
      // every FI_DB search specifically so a subsequent search in the same process
      // rebuilds it -- but staticParamsInitializationComplete is intentionally never
      // reset (re-running the rest of this function on a second call risks double-
      // applying cumulative state such as static-mod residue-mass deltas), so the
      // one-time allocation block below never gets a second chance to run and the
      // second FI_DB search null-derefs on g_bIndexPrecursors. Re-run just that
      // allocation here, using whatever param values are already live in
      // g_staticParams (a second SetParam("peptide_mass_high", ...) between searches
      // is silently ignored either way, same as every other post-first-search param
      // change under this one-shot design).
      if (g_staticParams.iDbType == DbType::FI_DB && g_bIndexPrecursors == NULL)
      {
         g_bIndexPrecursors = (bool*)malloc((BIN(g_staticParams.options.dPeptideMassHigh) + 1) * sizeof(bool));
         if (g_bIndexPrecursors == NULL)
         {
            printf("\n Error cannot allocate memory for g_bIndexPrecursors(%d)\n", BIN(g_staticParams.options.dPeptideMassHigh) + 1);
            return false;
         }
         for (int x = 0; x <= BIN(g_staticParams.options.dPeptideMassHigh); ++x)
         {
            if (g_pvInputFiles.size() == 0 || g_staticParams.options.iFragIndexSkipReadPrecursors)
               g_bIndexPrecursors[x] = true;
            else
               g_bIndexPrecursors[x] = false;
         }
      }

      return true;
   }

   if (GetParamValue("database_name", strData))
      strcpy(g_staticParams.databaseInfo.szDatabase, strData.c_str());

   if (GetParamValue("decoy_prefix", strData))
   {
      strcpy(g_staticParams.szDecoyPrefix, strData.c_str());
      g_staticParams.sDecoyPrefix = g_staticParams.szDecoyPrefix;
      CometMassSpecUtils::EscapeString(g_staticParams.sDecoyPrefix);
   }

   if (GetParamValue("spectral_library_name", strData))
      g_staticParams.speclibInfo.strSpecLibFile = strData;

   if (GetParamValue("spectral_library_ms_level", iIntData))
   {
      // FIX add check that input value is some sane range
      g_staticParams.options.iSpecLibMSLevel = iIntData;
   }

   if (GetParamValue("output_suffix", strData))
      strcpy(g_staticParams.szOutputSuffix, strData.c_str());

   if (GetParamValue("text_file_extension", strData))
   {
      if (strData.length() > 0)
         strcpy(g_staticParams.szTxtFileExt, strData.c_str());
   }

   if (GetParamValue("protein_modslist_file", strData))
      g_staticParams.variableModParameters.sProteinLModsListFile = strData;

   if (GetParamValue("peff_obo", strData))
      strcpy(g_staticParams.peffInfo.szPeffOBO, strData.c_str());

   if (GetParamValue("compoundmods_file", strData))
      g_staticParams.variableModParameters.sCompoundModsFile = strData;

   GetParamValue("peff_format", g_staticParams.peffInfo.iPeffSearch);

   GetParamValue("mass_offsets", g_staticParams.vectorMassOffsets);

   GetParamValue("precursor_NL_ions", g_staticParams.precursorNLIons);

   GetParamValue("old_mods_encoding", g_staticParams.iOldModsEncoding);

   GetParamValue("xcorr_processing_offset", g_staticParams.iXcorrProcessingOffset);

   GetParamValue("nucleotide_reading_frame", g_staticParams.options.iWhichReadingFrame);

   GetParamValue("mass_type_parent", g_staticParams.massUtility.bMonoMassesParent);

   GetParamValue("mass_type_fragment", g_staticParams.massUtility.bMonoMassesFragment);

   if (GetParamValue("explicit_deltacn", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bExplicitDeltaCn = false;
      else
         g_staticParams.options.bExplicitDeltaCn = true;
   }

   GetParamValue("num_threads", g_staticParams.options.iNumThreads);

   if (GetParamValue("clip_nterm_methionine", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bClipNtermMet = false;
      else
         g_staticParams.options.bClipNtermMet = true;
   }

   if (GetParamValue("clip_nterm_aa", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bClipNtermAA = false;
      else
         g_staticParams.options.bClipNtermAA = true;
   }

   GetParamValue("minimum_xcorr", g_staticParams.options.dMinimumXcorr);

   GetParamValue("theoretical_fragment_ions", g_staticParams.ionInformation.iTheoreticalFragmentIons);
   if ((g_staticParams.ionInformation.iTheoreticalFragmentIons < 0)
         || (g_staticParams.ionInformation.iTheoreticalFragmentIons > 1))
   {
      g_staticParams.ionInformation.iTheoreticalFragmentIons = 0;
   }

   GetParamValue("use_A_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_A]);

   GetParamValue("use_B_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_B]);

   GetParamValue("use_C_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_C]);

   GetParamValue("use_X_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_X]);

   GetParamValue("use_Y_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]);

   GetParamValue("use_Z_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_Z]);

   GetParamValue("use_Z1_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_Z1]);

   if (GetParamValue("use_NL_ions", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.ionInformation.bUseWaterAmmoniaLoss = false;
      else
         g_staticParams.ionInformation.bUseWaterAmmoniaLoss = true;
   }

   GetParamValue("variable_mod01", g_staticParams.variableModParameters.varModList[VMOD_1_INDEX]);
   GetParamValue("variable_mod02", g_staticParams.variableModParameters.varModList[VMOD_2_INDEX]);
   GetParamValue("variable_mod03", g_staticParams.variableModParameters.varModList[VMOD_3_INDEX]);
   GetParamValue("variable_mod04", g_staticParams.variableModParameters.varModList[VMOD_4_INDEX]);
   GetParamValue("variable_mod05", g_staticParams.variableModParameters.varModList[VMOD_5_INDEX]);
   GetParamValue("variable_mod06", g_staticParams.variableModParameters.varModList[VMOD_6_INDEX]);
   GetParamValue("variable_mod07", g_staticParams.variableModParameters.varModList[VMOD_7_INDEX]);
   GetParamValue("variable_mod08", g_staticParams.variableModParameters.varModList[VMOD_8_INDEX]);
   GetParamValue("variable_mod09", g_staticParams.variableModParameters.varModList[VMOD_9_INDEX]);
   GetParamValue("variable_mod10", g_staticParams.variableModParameters.varModList[VMOD_10_INDEX]);
   GetParamValue("variable_mod11", g_staticParams.variableModParameters.varModList[VMOD_11_INDEX]);
   GetParamValue("variable_mod12", g_staticParams.variableModParameters.varModList[VMOD_12_INDEX]);
   GetParamValue("variable_mod13", g_staticParams.variableModParameters.varModList[VMOD_13_INDEX]);
   GetParamValue("variable_mod14", g_staticParams.variableModParameters.varModList[VMOD_14_INDEX]);
   GetParamValue("variable_mod15", g_staticParams.variableModParameters.varModList[VMOD_15_INDEX]);

   if (GetParamValue("max_variable_mods_in_peptide", iIntData))
   {
      if (iIntData >= 0)
         g_staticParams.variableModParameters.iMaxVarModPerPeptide = iIntData;
   }

   // Note that g_staticParams.variableModParameters.iRequireVarMod could also
   // be set by each mod's iRequireThisMod later on in this function.  The 0th bit
   // is used to require any varmod; each ith bit will require that ith varmod.
   if (GetParamValue("require_variable_mod", iIntData))
   {
      if (iIntData > 0)
         g_staticParams.variableModParameters.iRequireVarMod |= 1UL << 0;
      else
         g_staticParams.variableModParameters.iRequireVarMod = 0;
   }

   GetParamValue("fragment_bin_tol", g_staticParams.tolerances.dFragmentBinSize);

   GetParamValue("fragment_bin_offset", g_staticParams.tolerances.dFragmentBinStartOffset);

   GetParamValue("ms1_bin_tol", g_staticParams.tolerances.dMS1BinSize);

   GetParamValue("ms1_bin_offset", g_staticParams.tolerances.dMS1BinStartOffset);

   if (GetParamValue("ms1_mass_range", doubleRangeData))
   {
      if ((doubleRangeData.dEnd >= doubleRangeData.dStart) && (doubleRangeData.dStart >= 0.0))
      {
         g_staticParams.options.dMS1MinMass = doubleRangeData.dStart;
         g_staticParams.options.dMS1MaxMass = doubleRangeData.dEnd;
      }
   }

   // this parameter superseded by _upper/_lower; will still apply if the other params are missing
   GetParamValue("peptide_mass_tolerance", g_staticParams.tolerances.dInputTolerancePlus);
   g_staticParams.tolerances.dInputToleranceMinus = -1.0 * g_staticParams.tolerances.dInputTolerancePlus;

   if (GetParamValue("peptide_mass_tolerance_upper", g_staticParams.tolerances.dInputToleranceMinus))
      g_staticParams.tolerances.dInputToleranceMinus *= -1.0;

   if (GetParamValue("peptide_mass_tolerance_lower", g_staticParams.tolerances.dInputTolerancePlus))
      g_staticParams.tolerances.dInputTolerancePlus *= -1.0;

   GetParamValue("precursor_tolerance_type", g_staticParams.tolerances.iMassToleranceType);
   if ((g_staticParams.tolerances.iMassToleranceType < 0) || (g_staticParams.tolerances.iMassToleranceType > 1))
   {
      g_staticParams.tolerances.iMassToleranceType = 0;
   }

   GetParamValue("peptide_mass_units", g_staticParams.tolerances.iMassToleranceUnits);
   if ((g_staticParams.tolerances.iMassToleranceUnits < 0) || (g_staticParams.tolerances.iMassToleranceUnits > 2))
   {
      g_staticParams.tolerances.iMassToleranceUnits = 0;  // 0=amu, 1=mmu, 2=ppm
   }

   GetParamValue("isotope_error", g_staticParams.tolerances.iIsotopeError);
   if ((g_staticParams.tolerances.iIsotopeError < 0) || (g_staticParams.tolerances.iIsotopeError > 7))
   {
      g_staticParams.tolerances.iIsotopeError = 0;
   }

   GetParamValue("num_output_lines", g_staticParams.options.iNumPeptideOutputLines);

   GetParamValue("num_results", g_staticParams.options.iNumStored);

   GetParamValue("max_duplicate_proteins", g_staticParams.options.iMaxDuplicateProteins);

   GetParamValue("remove_precursor_peak", g_staticParams.options.iRemovePrecursor);

   GetParamValue("remove_precursor_tolerance", g_staticParams.options.dRemovePrecursorTol);

   if (GetParamValue("clear_mz_range", doubleRangeData))
   {
      if ((doubleRangeData.dEnd >= doubleRangeData.dStart) && (doubleRangeData.dStart >= 0.0))
      {
         g_staticParams.options.clearMzRange.dStart = doubleRangeData.dStart;
         g_staticParams.options.clearMzRange.dEnd = doubleRangeData.dEnd;
      }
   }

   if (GetParamValue("print_expect_score", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bPrintExpectScore = false;
      else
         g_staticParams.options.bPrintExpectScore = true;
   }

   GetParamValue("print_ascorepro_score", g_staticParams.options.iPrintAScoreProScore);

   if (GetParamValue("export_additional_pepxml_scores", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bExportAdditionalScoresPepXML = false;
      else
         g_staticParams.options.bExportAdditionalScoresPepXML = true;
   }

   if (GetParamValue("resolve_fullpaths", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bResolveFullPaths = false;
      else
         g_staticParams.options.bResolveFullPaths = true;
   }

   if (GetParamValue("output_sqtstream", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bOutputSqtStream = false;
      else
         g_staticParams.options.bOutputSqtStream = true;
   }

   if (GetParamValue("output_sqtfile", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bOutputSqtFile = false;
      else
         g_staticParams.options.bOutputSqtFile = true;
   }

   if (GetParamValue("output_txtfile", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bOutputTxtFile = false;
      else
         g_staticParams.options.bOutputTxtFile = true;
   }

   if (GetParamValue("output_pepxmlfile", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bOutputPepXMLFile = false;
      else
         g_staticParams.options.bOutputPepXMLFile = true;
   }

   GetParamValue("output_mzidentmlfile", g_staticParams.options.iOutputMzIdentMLFile);

   if (GetParamValue("output_percolatorfile", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bOutputPercolatorFile = false;
      else
         g_staticParams.options.bOutputPercolatorFile = true;
   }

   if (GetParamValue("mango_search", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bMango = false;
      else
         g_staticParams.options.bMango = true;
   }

   if (GetParamValue("scale_fragmentNL", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bScaleFragmentNL = false;
      else
         g_staticParams.options.bScaleFragmentNL = true;
   }

   if (GetParamValue("create_fragment_index", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bCreateFragmentIndex = false;
      else
         g_staticParams.options.bCreateFragmentIndex = true;
   }

   if (GetParamValue("create_peptide_index", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bCreatePeptideIndex = false;
      else
         g_staticParams.options.bCreatePeptideIndex = true;
   }

   // docs/20260730_PI_reduction.md Phase 0: which search mode (PI_DB or FI_DB) to run
   // against an existing unified .idx file. 0=PI_DB, 1=FI_DB; unset (-1) defaults to FI_DB
   // at the point of use, matching the pre-unification default for a plain ".idx specified
   // but ambiguous" case. RTS sets this via the corresponding RealtimeSearch.exe CLI
   // argument, since it never loads comet.params.
   if (GetParamValue("index_search_type", iIntData))
      g_staticParams.options.iIndexSearchType = iIntData;

   GetParamValue("max_iterations", g_staticParams.options.lMaxIterations);

   GetParamValue("max_index_runtime", g_staticParams.options.iMaxIndexRunTime);

   if (GetParamValue("peff_verbose_output", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bVerboseOutput = false;
      else
         g_staticParams.options.bVerboseOutput = true;
   }

   GetParamValue("add_Cterm_peptide", g_staticParams.staticModifications.dAddCterminusPeptide);

   GetParamValue("add_Nterm_peptide", g_staticParams.staticModifications.dAddNterminusPeptide);

   GetParamValue("add_Cterm_protein", g_staticParams.staticModifications.dAddCterminusProtein);

   GetParamValue("add_Nterm_protein", g_staticParams.staticModifications.dAddNterminusProtein);

   if (GetParamValue("add_G_glycine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'G'] = dDoubleData;

   if (GetParamValue("add_A_alanine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'A'] = dDoubleData;

   if (GetParamValue("add_S_serine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'S'] = dDoubleData;

   if (GetParamValue("add_P_proline", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'P'] = dDoubleData;

   if (GetParamValue("add_V_valine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'V'] = dDoubleData;

   if (GetParamValue("add_T_threonine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'T'] = dDoubleData;

   if (GetParamValue("add_C_cysteine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'C'] = dDoubleData;

   if (GetParamValue("add_L_leucine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'L'] = dDoubleData;

   if (GetParamValue("add_I_isoleucine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'I'] = dDoubleData;

   if (GetParamValue("add_N_asparagine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'N'] = dDoubleData;

   if (GetParamValue("add_O_pyrrolysine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'O'] = dDoubleData;

   if (GetParamValue("add_D_aspartic_acid", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'D'] = dDoubleData;

   if (GetParamValue("add_Q_glutamine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'Q'] = dDoubleData;

   if (GetParamValue("add_K_lysine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'K'] = dDoubleData;

   if (GetParamValue("add_E_glutamic_acid", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'E'] = dDoubleData;

   if (GetParamValue("add_M_methionine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'M'] = dDoubleData;

   if (GetParamValue("add_H_histidine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'H'] = dDoubleData;

   if (GetParamValue("add_F_phenylalanine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'F'] = dDoubleData;

   if (GetParamValue("add_R_arginine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'R'] = dDoubleData;

   if (GetParamValue("add_Y_tyrosine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'Y'] = dDoubleData;

   if (GetParamValue("add_W_tryptophan", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'W'] = dDoubleData;

   if (GetParamValue("add_B_user_amino_acid", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'B'] = dDoubleData;

   if (GetParamValue("add_J_user_amino_acid", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'J'] = dDoubleData;

   if (GetParamValue("add_U_selenocysteine", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'U'] = dDoubleData;

   if (GetParamValue("add_X_user_amino_acid", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'X'] = dDoubleData;

   if (GetParamValue("add_Z_user_amino_acid", dDoubleData))
      g_staticParams.staticModifications.pdStaticMods[(int)'Z'] = dDoubleData;

   if (GetParamValue("set_G_glycine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'G'] = dDoubleData;
   }

   if (GetParamValue("set_A_alanine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'A'] = dDoubleData;
   }

   if (GetParamValue("set_S_serine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'S'] = dDoubleData;
   }

   if (GetParamValue("set_P_proline", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'P'] = dDoubleData;
   }

   if (GetParamValue("set_V_valine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'V'] = dDoubleData;
   }

   if (GetParamValue("set_T_threonine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'T'] = dDoubleData;
   }

   if (GetParamValue("set_C_cysteine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'C'] = dDoubleData;
   }

   if (GetParamValue("set_L_leucine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'L'] = dDoubleData;
   }

   if (GetParamValue("set_I_isoleucine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'I'] = dDoubleData;
   }

   if (GetParamValue("set_N_asparagine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'N'] = dDoubleData;
   }

   if (GetParamValue("set_O_pyrrolysine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'O'] = dDoubleData;
   }

   if (GetParamValue("set_D_aspartic_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'D'] = dDoubleData;
   }

   if (GetParamValue("set_Q_glutamine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'Q'] = dDoubleData;
   }

   if (GetParamValue("set_K_lysine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'K'] = dDoubleData;
   }

   if (GetParamValue("set_E_glutamic_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'E'] = dDoubleData;
   }

   if (GetParamValue("set_M_methionine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'M'] = dDoubleData;
   }

   if (GetParamValue("set_H_histidine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'H'] = dDoubleData;
   }

   if (GetParamValue("set_F_phenylalanine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'F'] = dDoubleData;
   }

   if (GetParamValue("set_R_arginine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'R'] = dDoubleData;
   }

   if (GetParamValue("set_Y_tyrosine", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'Y'] = dDoubleData;
   }

   if (GetParamValue("set_W_tryptophan", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'W'] = dDoubleData;
   }

   if (GetParamValue("set_B_user_amino_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'B'] = dDoubleData;
   }

   if (GetParamValue("set_J_user_amino_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'J'] = dDoubleData;
   }

   if (GetParamValue("set_U_user_amino_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'U'] = dDoubleData;
   }

   if (GetParamValue("set_X_user_amino_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'X'] = dDoubleData;
   }

   if (GetParamValue("set_Z_user_amino_acid", dDoubleData))
   {
      if (dDoubleData != 0.0)
         g_staticParams.massUtility.pdAAMassUser[(int)'Z'] = dDoubleData;
   }

   if (GetParamValue("fragindex_min_fragmentmass", dDoubleData))
   {
      if (dDoubleData >= FRAGINDEX_MIN_MASS && dDoubleData <= FRAGINDEX_MAX_MASS)
         g_staticParams.options.dFragIndexMinMass = dDoubleData;
   }
   if (GetParamValue("fragindex_max_fragmentmass", dDoubleData))
   {
      if (dDoubleData >= FRAGINDEX_MIN_MASS && dDoubleData <= FRAGINDEX_MAX_MASS)
         g_staticParams.options.dFragIndexMaxMass = dDoubleData;
   }
   GetParamValue("fragindex_num_spectrumpeaks", g_staticParams.options.iFragIndexNumSpectrumPeaks);
   GetParamValue("fragindex_min_ions_score", g_staticParams.options.iFragIndexMinIonsScore);
   GetParamValue("fragindex_min_ions_report", g_staticParams.options.iFragIndexMinIonsReport);
   GetParamValue("fragindex_skipreadprecursors", g_staticParams.options.iFragIndexSkipReadPrecursors);

   GetParamValue("num_enzyme_termini", g_staticParams.options.iEnzymeTermini);
   if ((g_staticParams.options.iEnzymeTermini != 1)
         && (g_staticParams.options.iEnzymeTermini != 8)
         && (g_staticParams.options.iEnzymeTermini != 9))
   {
      g_staticParams.options.iEnzymeTermini = 2;
   }

   if (GetParamValue("scan_range", intRangeData))
   {
      if ((intRangeData.iEnd >= intRangeData.iStart) && (intRangeData.iStart > 0))
      {
         g_staticParams.options.scanRange.iStart = intRangeData.iStart;
         g_staticParams.options.scanRange.iEnd = intRangeData.iEnd;
      }
   }

   if (GetParamValue("peptide_length_range", intRangeData))
   {
      if ((intRangeData.iEnd >= intRangeData.iStart) && (intRangeData.iStart > 0))
      {
         g_staticParams.options.peptideLengthRange.iStart = intRangeData.iStart;
         g_staticParams.options.peptideLengthRange.iEnd = intRangeData.iEnd;

         if (g_staticParams.options.peptideLengthRange.iStart < MIN_PEPTIDE_LEN)
            g_staticParams.options.peptideLengthRange.iStart = MIN_PEPTIDE_LEN;

         if (g_staticParams.options.peptideLengthRange.iEnd >= MAX_PEPTIDE_LEN)
            g_staticParams.options.peptideLengthRange.iEnd = MAX_PEPTIDE_LEN - 1;
      }
   }

   if (GetParamValue("spectrum_batch_size", iIntData))
   {
      if (iIntData >= 0)
         g_staticParams.options.iSpectrumBatchSize = iIntData;
   }

   if (GetParamValue("minimum_peaks", iIntData))
   {
      if (iIntData >= 0)
         g_staticParams.options.iMinPeaks = iIntData;
   }

   GetParamValue("override_charge", g_staticParams.options.iOverrideCharge);

   if (GetParamValue("correct_mass", iIntData))
   {
      if (iIntData > 0)
         g_staticParams.options.bCorrectMass = iIntData;
   }

   if (GetParamValue("equal_I_and_L", iIntData))
   {
      g_staticParams.options.bTreatSameIL = iIntData;
   }

   if (GetParamValue("max_index_runtime", iIntData))
   {
      g_staticParams.options.iMaxIndexRunTime = iIntData;
   }

   if (GetParamValue("precursor_charge", intRangeData))
   {
      if ((intRangeData.iStart > 0) && (intRangeData.iEnd >= intRangeData.iStart))
      {
         g_staticParams.options.iStartCharge = intRangeData.iStart;
         g_staticParams.options.iEndCharge = intRangeData.iEnd;
      }
   }

   if (GetParamValue("max_fragment_charge", iIntData))
   {
      if (iIntData > MAX_FRAGMENT_CHARGE)
         iIntData = MAX_FRAGMENT_CHARGE;

      if (iIntData > 0)
         g_staticParams.options.iMaxFragmentCharge = iIntData;

      // else will go to default value (3)
   }

   if (GetParamValue("min_precursor_charge", iIntData))
   {
      if (iIntData < 1)
         iIntData = 1;
      else if (iIntData > MAX_PRECURSOR_CHARGE)
         iIntData = MAX_PRECURSOR_CHARGE;

      g_staticParams.options.iMinPrecursorCharge = iIntData;
   }

   if (GetParamValue("max_precursor_charge", iIntData))
   {
      if (iIntData > MAX_PRECURSOR_CHARGE)
         iIntData = MAX_PRECURSOR_CHARGE;

      if (iIntData > 0)
         g_staticParams.options.iMaxPrecursorCharge = iIntData;
   }

   if (GetParamValue("digest_mass_range", doubleRangeData))
   {
      if ((doubleRangeData.dEnd >= doubleRangeData.dStart) && (doubleRangeData.dStart >= 0.0))
      {
         g_staticParams.options.dPeptideMassLow = doubleRangeData.dStart;
         g_staticParams.options.dPeptideMassHigh = doubleRangeData.dEnd;
      }
   }

   if (GetParamValue("ms_level", iIntData))  // default 2, only supports 2 or 3
   {
      if (iIntData == 3)
         g_staticParams.options.iMSLevel = 3;
   }

   if (GetParamValue("activation_method", strData))
      strcpy(g_staticParams.options.szActivationMethod, strData.c_str());

   if (GetParamValue("pinfile_protein_delimiter", strData))
   {
      if (strData.length() > 0)
         g_staticParams.options.sPinProteinDelimiter = strData;
   }

   GetParamValue("minimum_intensity", g_staticParams.options.dMinIntensity);
   if (g_staticParams.options.dMinIntensity < 0.0)
      g_staticParams.options.dMinIntensity = 0.0;

   GetParamValue("percentage_base_peak", g_staticParams.options.dMinPercentageIntensity);
   if (g_staticParams.options.dMinPercentageIntensity < 0.0)
      g_staticParams.options.dMinPercentageIntensity = 0.0;

   GetParamValue("decoy_search", g_staticParams.options.iDecoySearch);
   if ((g_staticParams.options.iDecoySearch < 0) || (g_staticParams.options.iDecoySearch > 2))
      g_staticParams.options.iDecoySearch = 0;

   // Set dInverseBinWidth to its inverse in order to use a multiply instead of divide in BIN macro.
   // Safe to divide by dFragmentBinSize because of check earlier where minimum value is 0.01.
   g_staticParams.dInverseBinWidth = 1.0 /g_staticParams.tolerances.dFragmentBinSize;
   g_staticParams.dOneMinusBinOffset = 1.0 - g_staticParams.tolerances.dFragmentBinStartOffset;

   // Set masses to either average or monoisotopic.
   CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassParent,
                                  g_staticParams.massUtility.bMonoMassesParent,
                                  &g_staticParams.massUtility.dOH2parent);

   CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassFragment,
                                  g_staticParams.massUtility.bMonoMassesFragment,
                                  &g_staticParams.massUtility.dOH2fragment);

   g_staticParams.massUtility.dCO = g_staticParams.massUtility.pdAAMassFragment[(int)'c']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'o'];

   g_staticParams.massUtility.dH2O = g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'o'];

   g_staticParams.massUtility.dNH3 = g_staticParams.massUtility.pdAAMassFragment[(int)'n']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

   g_staticParams.massUtility.dNH2 = g_staticParams.massUtility.pdAAMassFragment[(int)'n']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

   g_staticParams.massUtility.dCOminusH2 = g_staticParams.massUtility.dCO
            - g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

   g_staticParams.sHostName = GetHostName();

   // If # threads not specified, poll system to get # threads to launch.
   if (g_staticParams.options.iNumThreads <= 0)
   {
      int iNumCPUCores;
#ifdef _WIN32
      SYSTEM_INFO sysinfo;
      GetSystemInfo( &sysinfo );
      iNumCPUCores = sysinfo.dwNumberOfProcessors;

      // if user specifies a negative # threads, subtract this from # cores
      if (g_staticParams.options.iNumThreads < 0 && iNumCPUCores + g_staticParams.options.iNumThreads > 0)
         g_staticParams.options.iNumThreads = iNumCPUCores + g_staticParams.options.iNumThreads;
      else
         g_staticParams.options.iNumThreads = iNumCPUCores;
#else
      iNumCPUCores = sysconf( _SC_NPROCESSORS_ONLN );

      if (g_staticParams.options.iNumThreads < 0 && iNumCPUCores + g_staticParams.options.iNumThreads > 0)
         g_staticParams.options.iNumThreads = iNumCPUCores + g_staticParams.options.iNumThreads;
      else
         g_staticParams.options.iNumThreads = iNumCPUCores;

      // if set, use the environment variable NSLOTS which is defined in the qsub command
      const char * nSlots = ::getenv("NSLOTS");
      if (nSlots != NULL)
      {
         int detectedThreads = atoi(nSlots);
         if (detectedThreads > 0)
            g_staticParams.options.iNumThreads = detectedThreads;
      }
#endif

      if (g_staticParams.options.iNumThreads > MAX_THREADS)
      {
         g_staticParams.options.iNumThreads = MAX_THREADS;
         string strOut = " Setting number of threads to " + std::to_string(MAX_THREADS);
         logout(strOut);
      }
   }

   // Set masses to either average or monoisotopic.
   CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassParent,
                                  g_staticParams.massUtility.bMonoMassesParent,
                                  &g_staticParams.massUtility.dOH2parent);

   CometMassSpecUtils::AssignMass(g_staticParams.massUtility.pdAAMassFragment,
                                  g_staticParams.massUtility.bMonoMassesFragment,
                                  &g_staticParams.massUtility.dOH2fragment);

   // Now that amino acid masses are assigned, see if they are possibly overriden by
   // the user define amino acid masses
   for (int x = 65 ; x <= 90; ++x)
   {
      if (g_staticParams.massUtility.pdAAMassUser[x] > 0.0)
      {
         g_staticParams.massUtility.pdAAMassParent[x] = g_staticParams.massUtility.pdAAMassUser[x];
         g_staticParams.massUtility.pdAAMassFragment[x] = g_staticParams.massUtility.pdAAMassUser[x];
      }
   }

   g_staticParams.massUtility.dCO = g_staticParams.massUtility.pdAAMassFragment[(int)'c']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'o'];

   g_staticParams.massUtility.dH2O = g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'o'];

   g_staticParams.massUtility.dNH3 = g_staticParams.massUtility.pdAAMassFragment[(int)'n']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

   g_staticParams.massUtility.dNH2 = g_staticParams.massUtility.pdAAMassFragment[(int)'n']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

   g_staticParams.massUtility.dCOminusH2 = g_staticParams.massUtility.dCO
            - g_staticParams.massUtility.pdAAMassFragment[(int)'h']
            - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

   GetParamValue("[COMET_ENZYME_INFO]", g_staticParams.enzymeInformation);

   if (!strncmp(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, "-", 1)
         && !strncmp(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, "-", 1))
   {
      g_staticParams.enzymeInformation.bNoEnzymeSelected = 1;
   }
   else
   {
      g_staticParams.enzymeInformation.bNoEnzymeSelected = 0;
   }

   if (!strncmp(g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA, "-", 1)
         && !strncmp(g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA, "-", 1))
   {
      g_staticParams.enzymeInformation.bNoEnzyme2Selected = 1;
   }
   else
   {
      g_staticParams.enzymeInformation.bNoEnzyme2Selected = 0;
   }

   GetParamValue("allowed_missed_cleavage", g_staticParams.enzymeInformation.iAllowedMissedCleavage);
   if (g_staticParams.enzymeInformation.iAllowedMissedCleavage < 0)
      g_staticParams.enzymeInformation.iAllowedMissedCleavage = 0;

   // Load ion series to consider, useA, useB, useY are for neutral losses.
   g_staticParams.ionInformation.iNumIonSeriesUsed = 0;
   for (int i=0; i<NUM_ION_SERIES; ++i)
   {
      if (g_staticParams.ionInformation.iIonVal[i] > 0)
         g_staticParams.ionInformation.piSelectedIonSeries[g_staticParams.ionInformation.iNumIonSeriesUsed++] = i;
   }

   // Variable mod search for AAs listed in szVarModChar.
   g_staticParams.szMod[0] = '\0';
   g_staticParams.variableModParameters.bVarModSearch = false;
   g_staticParams.variableModParameters.bVarTermModSearch = false;
   g_staticParams.variableModParameters.bBinaryModSearch = false;
   g_staticParams.variableModParameters.bUseFragmentNeutralLoss = false;
   g_staticParams.variableModParameters.bVarProteinNTermMod = false;
   g_staticParams.variableModParameters.bVarProteinCTermMod = false;
   g_staticParams.variableModParameters.bVarModProteinFilter = false;
   g_staticParams.variableModParameters.bRareVarModPresent = false;

   if (g_staticParams.peffInfo.iPeffSearch)
      g_staticParams.variableModParameters.bVarModSearch = true;

   for (int i=0; i<VMODS; ++i)
   {
      // Crux has been using "null" as its default modification character string which
      // sadly didn't behave as they might have thought. To address that, this will
      // zero out the modification mass if "null" is the amino acid string.
      if (!strcmp(g_staticParams.variableModParameters.varModList[i].szVarModChar, "null"))
         g_staticParams.variableModParameters.varModList[i].dVarModMass = 0.0;

      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='-'))
      {
         if (g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod > g_staticParams.variableModParameters.iMaxVarModPerPeptide)
            g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod = g_staticParams.variableModParameters.iMaxVarModPerPeptide;

         if (g_staticParams.options.bCreateFragmentIndex)
         {  // limit any user specified modification limits to the max supported by fragment ion indexing
            if (g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod > FRAGINDEX_MAX_MODS_PER_MOD)
               g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod = FRAGINDEX_MAX_MODS_PER_MOD;
         }
      }
   }

   // reduce variable modifications if entries are the same
   for (int i=0; i<VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == -1)
      {
         g_staticParams.variableModParameters.bRareVarModPresent = true;
      }

      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='-'))
      {
         for (int ii=i+1; ii<VMODS; ++ii)
         {
            if (!isEqual(g_staticParams.variableModParameters.varModList[ii].dVarModMass, 0.0)
                  && (g_staticParams.variableModParameters.varModList[ii].szVarModChar[0]!='-'))
            {
               // Merge the modifications (for better performance) if everything else is equal.
               // There are cases where the mods should be merged even if the neutral loss value is 0.0
               // in one entry and non-zero in another but it depends on the list of residues. I'll
               // ignore that case is this will cover 99% of the utility.
               if (     (g_staticParams.variableModParameters.varModList[i].dVarModMass== g_staticParams.variableModParameters.varModList[ii].dVarModMass)
                     && (g_staticParams.variableModParameters.varModList[i].dNeutralLoss == g_staticParams.variableModParameters.varModList[ii].dNeutralLoss)
                     && (g_staticParams.variableModParameters.varModList[i].dNeutralLoss2 == g_staticParams.variableModParameters.varModList[ii].dNeutralLoss2)
                     && (g_staticParams.variableModParameters.varModList[i].iBinaryMod == g_staticParams.variableModParameters.varModList[ii].iBinaryMod)
                     && (g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod == g_staticParams.variableModParameters.varModList[ii].iMaxNumVarModAAPerMod)
                     && (g_staticParams.variableModParameters.varModList[i].iMinNumVarModAAPerMod == g_staticParams.variableModParameters.varModList[ii].iMinNumVarModAAPerMod)
                     && (g_staticParams.variableModParameters.varModList[i].iVarModTermDistance == g_staticParams.variableModParameters.varModList[ii].iVarModTermDistance)
                     && (g_staticParams.variableModParameters.varModList[i].iWhichTerm == g_staticParams.variableModParameters.varModList[ii].iWhichTerm)
                     && (g_staticParams.variableModParameters.varModList[i].iRequireThisMod == g_staticParams.variableModParameters.varModList[ii].iRequireThisMod)
                     &&  g_staticParams.variableModParameters.varModList[i].iRequireThisMod != -1)
               {
                  // everything the same merge the modifications
                  strcat(g_staticParams.variableModParameters.varModList[i].szVarModChar, g_staticParams.variableModParameters.varModList[ii].szVarModChar);
                  sprintf(g_staticParams.variableModParameters.varModList[ii].szVarModChar, "-");
                  g_staticParams.variableModParameters.varModList[ii].dVarModMass = 0.0;
               }
            }
         }

         // quick check  to make sure a residue isn't repeated in szVarModChar
         if (strlen(g_staticParams.variableModParameters.varModList[i].szVarModChar) > 1)
         {
            string sTmp = g_staticParams.variableModParameters.varModList[i].szVarModChar;
            std::sort(sTmp.begin(), sTmp.end());
            sTmp.erase(std::unique(sTmp.begin(), sTmp.end()), sTmp.end());
            strcpy(g_staticParams.variableModParameters.varModList[i].szVarModChar, sTmp.c_str());
         }
      }
   }

   for (int i = 0; i < VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
            && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='-'))
      {
         AppendSzMod("(%s%c %+0.6f) ",
               g_staticParams.variableModParameters.varModList[i].szVarModChar,
               g_staticParams.variableModParameters.cModCode[i],
               g_staticParams.variableModParameters.varModList[i].dVarModMass);

         g_staticParams.variableModParameters.bVarModSearch = true;

         g_staticParams.variableModParameters.varModList[i].bUseMod = true;

         if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'n'))
         {
            g_staticParams.variableModParameters.varModList[i].bNtermMod = true;
            g_staticParams.variableModParameters.bVarTermModSearch = true;

            if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 0)
               g_staticParams.variableModParameters.bVarProteinNTermMod = true;
         }

         if (strchr(g_staticParams.variableModParameters.varModList[i].szVarModChar, 'c'))
         {
            g_staticParams.variableModParameters.varModList[i].bCtermMod = true;
            g_staticParams.variableModParameters.bVarTermModSearch = true;

            if (g_staticParams.variableModParameters.varModList[i].iWhichTerm == 1)
               g_staticParams.variableModParameters.bVarProteinCTermMod = true;
         }

         if (g_staticParams.variableModParameters.varModList[i].iBinaryMod)
            g_staticParams.variableModParameters.bBinaryModSearch = true;

         if (g_staticParams.variableModParameters.varModList[i].iRequireThisMod > 0)
            g_staticParams.variableModParameters.iRequireVarMod |= 1UL << (i+1);  // set i+1 bit variable mods

         if (g_staticParams.variableModParameters.varModList[i].dNeutralLoss != 0.0)
            g_staticParams.variableModParameters.bUseFragmentNeutralLoss = true;
      }
   }

   // AScorePro was only ever intended to be applied to variable_mod01-05 (the FI
   // modification-count limit). CometPostAnalysis.cpp's sequence handoff to AScorePro
   // (`std::to_string(piVarModSites[i])`) embeds a raw mod-slot number (1-15) per
   // modified residue directly into the peptide-string encoding it hands over, and
   // AScorePro's single-char-per-position parser can't distinguish a two-digit slot
   // number (e.g. "12") from two consecutive single-digit ones -- silently
   // mis-localizing/corrupting mod sites onto the wrong slot's mass (see the review
   // write-up this guard was added for). Loudly refuse the combination instead of
   // letting it silently corrupt results.
   if (g_staticParams.options.iPrintAScoreProScore != 0)
   {
      for (int i = 5; i < VMODS; ++i)   // variable_mod06 (index 5) .. variable_mod15 (index 14)
      {
         if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
               && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0] != '-'))
         {
            string strErrorMsg = " Error - print_ascorepro_score is enabled but variable_mod"
               + string(i + 1 < 10 ? "0" : "") + to_string(i + 1)
               + " is active; AScorePro localization is only supported for variable_mod01"
               + " through variable_mod05. Disable print_ascorepro_score or remove/renumber"
               + " the higher-numbered variable mod(s).\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }
   }

   if (g_staticParams.options.iNumPeptideOutputLines < 1)
      g_staticParams.options.iNumPeptideOutputLines = 1;

   // set iNumStored to be at least 1 bigger than iNumPeptideOutputLines for post processing code
   if (g_staticParams.options.iNumStored <= g_staticParams.options.iNumPeptideOutputLines)
      g_staticParams.options.iNumStored = g_staticParams.options.iNumPeptideOutputLines + 1;

   if (!isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
   {
      AppendSzMod("+ct=%0.6f ",
            g_staticParams.staticModifications.dAddCterminusPeptide);
   }

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0))
   {
      AppendSzMod("+nt=%0.6f ",
            g_staticParams.staticModifications.dAddNterminusPeptide);
   }

   if (!isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
   {
      AppendSzMod("+ctprot=%0.6f ",
            g_staticParams.staticModifications.dAddCterminusProtein);
   }

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
   {
      AppendSzMod("+ntprot=%0.6f ",
            g_staticParams.staticModifications.dAddNterminusProtein);
   }

   for (int i=65; i<=90; ++i)  // 65-90 represents upper case letters in ASCII
   {
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[i], 0.0))
      {
         AppendSzMod("%c=%0.6f ", i,
               g_staticParams.massUtility.pdAAMassParent[i] += g_staticParams.staticModifications.pdStaticMods[i]);
         g_staticParams.massUtility.pdAAMassFragment[i] += g_staticParams.staticModifications.pdStaticMods[i];
      }
      else if (i=='B' || i=='J' || i=='X' || i=='Z')
      {
         g_staticParams.massUtility.pdAAMassParent[i] = 999999.;
         g_staticParams.massUtility.pdAAMassFragment[i] = 999999.;
      }
   }

   // Print out enzyme name to g_staticParams.szMod.
   if (!g_staticParams.enzymeInformation.bNoEnzymeSelected)
   {
      char szTmp[4];

      szTmp[0]='\0';
      if (g_staticParams.options.iEnzymeTermini != 2)
         sprintf(szTmp, ":%d", g_staticParams.options.iEnzymeTermini);

      AppendSzMod("Enzyme:%s (%d%s)",
            g_staticParams.enzymeInformation.szSearchEnzymeName,
            g_staticParams.enzymeInformation.iAllowedMissedCleavage,
            szTmp);
   }
   else
   {
      AppendSzMod("Enzyme:%s",
            g_staticParams.enzymeInformation.szSearchEnzymeName);
   }

   if (g_staticParams.tolerances.dFragmentBinStartOffset < 0.0
         || g_staticParams.tolerances.dFragmentBinStartOffset >1.0)
   {
      string strErrorMsg = " Error - bin offset " + std::to_string(g_staticParams.tolerances.dFragmentBinStartOffset) + " must between 0.0 and 1.0\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   g_staticParams.precalcMasses.dNtermProton = g_staticParams.staticModifications.dAddNterminusPeptide
      + PROTON_MASS;

   g_staticParams.precalcMasses.dCtermOH2Proton = g_staticParams.staticModifications.dAddCterminusPeptide
      + g_staticParams.massUtility.dOH2fragment
      + PROTON_MASS;

   g_staticParams.precalcMasses.dOH2ProtonCtermNterm = g_staticParams.massUtility.dOH2parent
      + PROTON_MASS
      + g_staticParams.staticModifications.dAddCterminusPeptide
      + g_staticParams.staticModifications.dAddNterminusPeptide;

   if (g_staticParams.options.iMaxDuplicateProteins == -1)
      g_staticParams.options.iMaxDuplicateProteins = INT_MAX;

   g_staticParams.iPrecursorNLSize = (int)g_staticParams.precursorNLIons.size();

   if (g_staticParams.iPrecursorNLSize > MAX_PRECURSOR_NL_SIZE)
      g_staticParams.iPrecursorNLSize = MAX_PRECURSOR_NL_SIZE;

// for (int x=1; x<=9; ++x)
//    printf("OK bit %d: %d\n", x, (g_staticParams.variableModParameters.iRequireVarMod >> x) & 1U);

   g_massRange.uiMaxFragmentArrayIndex = BIN(g_staticParams.options.dFragIndexMaxMass) + 1;

   // At this point, check extension to set whether index database or not
   size_t databaseLen = strlen(g_staticParams.databaseInfo.szDatabase);
   if (databaseLen >= 4 && !strcmp(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
   {
      // Has .idx extension. If the file doesn't exist yet, index_search_type
      // (docs/20260811_restore_idx_header_mods.md) picks which format to auto-build; if it
      // does exist, the file is self-describing -- ParsePeptideIndexHeader() below reads
      // its IndexSearchType: line (and static/variable mod settings) directly, no parameter
      // needed.
      FILE *fp;

      // If .idx specified but does not exist, Comet will generate the index for the search
      // (both -i and -j build the same unified format; see CometPeptideIndex::WritePeptideIndex()).
      if ( (fp=fopen(g_staticParams.databaseInfo.szDatabase, "r")) == NULL)
      {
         g_staticParams.iDbType = (g_staticParams.options.iIndexSearchType == 0) ? DbType::PI_DB : DbType::FI_DB;

         if (g_staticParams.options.iSpectrumBatchSize > FRAGINDEX_MAX_BATCHSIZE || g_staticParams.options.iSpectrumBatchSize == 0)
            g_staticParams.options.iSpectrumBatchSize = FRAGINDEX_MAX_BATCHSIZE;
      }
      else
      {
         // Validates the magic string/version and sets g_staticParams.iDbType (plus static/
         // variable mods) from the file's own header -- see ParsePeptideIndexHeader()'s
         // doc comment.
         if (!CometPeptideIndex::ParsePeptideIndexHeader(fp))
         {
            fclose(fp);
            return false;
         }
         fclose(fp);

         // This clamp only matters for the legacy load-all-then-search-all path
         // (PiStrategy falls back to it for Mango/speclib runs; see
         // PiStrategy::executeBatch()) -- the fused path ignores iSpectrumBatchSize
         // entirely in favor of its own thread-scaled flush threshold. Applies to both
         // modes identically; no need to load all spectra into memory for either, since
         // both query the index sequentially.
         if (g_staticParams.options.iSpectrumBatchSize > FRAGINDEX_MAX_BATCHSIZE || g_staticParams.options.iSpectrumBatchSize == 0)
            g_staticParams.options.iSpectrumBatchSize = FRAGINDEX_MAX_BATCHSIZE;
      }
   }

   if (g_staticParams.options.bCreateFragmentIndex && g_staticParams.iDbType != DbType::FASTA_DB)
   {
      string strErrorMsg = " Error - input database already indexed: \"" + std::string(g_staticParams.databaseInfo.szDatabase) + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   if (g_staticParams.iDbType == DbType::FI_DB)
   {
      // +1: ReadPrecursors() clamps its fill range to BIN(dPeptideMassHigh) inclusive,
      // so the array needs a valid slot at that top index, not just 0..BIN(...)-1.
      g_bIndexPrecursors = (bool*)malloc((BIN(g_staticParams.options.dPeptideMassHigh) + 1) * sizeof(bool));
      if (g_bIndexPrecursors == NULL)
      {
         printf("\n Error cannot allocate memory for g_bIndexPrecursors(%d)\n", BIN(g_staticParams.options.dPeptideMassHigh) + 1);
         return false;
      }
      for (int x = 0; x <= BIN(g_staticParams.options.dPeptideMassHigh); ++x)
      {
         if (g_pvInputFiles.size() == 0 || g_staticParams.options.iFragIndexSkipReadPrecursors)
            g_bIndexPrecursors[x] = true;  // if RTS search, no input file to read precursors from so all precursors are valid
         else
            g_bIndexPrecursors[x] = false; // set all precursors as invalid; valid precursors will be determined in ReadPrecursors
      }
   }

   if (g_staticParams.speclibInfo.strSpecLibFile.length() > 0)
   {
      // set this such that can access all SpecLib entries at specific precursor mass bin
      // by accessing g_vulSpecLibPrecursorIndex.at(massbin)
      // +1: SetSpecLibPrecursorIndex() clamps its fill range to BINPREC(dPeptideMassHigh)
      // inclusive (iMaxBin), so index iMaxBin itself must be valid -- without the +1 this
      // sized only 0..iMaxBin-1, and .at(iMaxBin) threw std::out_of_range during LoadSpecLib()
      // for any library entry whose tolerance window reached the configured mass ceiling
      // (this runs for every entry regardless of MS level, so it could throw even in an
      // MS1-only run). Matches the +1 convention used elsewhere, e.g.
      // uiMaxFragmentArrayIndex = BIN(dFragIndexMaxMass) + 1 above.
      g_vulSpecLibPrecursorIndex.resize(BINPREC(g_staticParams.options.dPeptideMassHigh) + 1);
   }

   if (g_staticParams.tolerances.dInputToleranceMinus > g_staticParams.tolerances.dInputTolerancePlus)
   {
      printf("\n Error: mass_tolerance_lower is greater than mass_tolerance_upper so no peptides will be analyzed.\n");
      return false;
   }

   // Since g_staticParams.iArraySizeGlobal is used to define the size of the reused memory pool which
   // are shared between MS1 and MS2 queries, make sure to define its size based on the larger of the two.
   double dCushion = CometPreprocess::GetMassCushion(g_staticParams.options.dPeptideMassHigh);
   double dUseBinSize = (g_staticParams.tolerances.dMS1BinSize < g_staticParams.tolerances.dFragmentBinSize ?
      g_staticParams.tolerances.dMS1BinSize : g_staticParams.tolerances.dFragmentBinSize);
   double dUseMaxMass = (g_staticParams.options.dPeptideMassHigh > g_staticParams.options.dMS1MaxMass ?
      g_staticParams.options.dPeptideMassHigh : g_staticParams.options.dMS1MaxMass);

   g_staticParams.iArraySizeGlobal = (int)((dUseMaxMass + dCushion) / dUseBinSize);

   // Ensure pool buffers are large enough for MS1 binning (BINPREC) as well.
   // BINPREC uses dMS1BinSize which can produce larger indices than BIN.
   int iArraySizeMS1 = BINPREC(g_staticParams.options.dMS1MaxMass) + 1;
   if (iArraySizeMS1 > g_staticParams.iArraySizeGlobal)
      g_staticParams.iArraySizeGlobal = iArraySizeMS1;

   staticParamsInitializationComplete = true;

/*
   // Print out all variable modifications being used.
   for (int i = 0; i < VMODS; ++i)
   {
      if (g_staticParams.variableModParameters.varModList[i].bUseMod)
      {
         printf("OK variable modification %d\n", i);
         printf("mass %lf\n", g_staticParams.variableModParameters.varModList[i].dVarModMass);
         printf("chars %s\n", g_staticParams.variableModParameters.varModList[i].szVarModChar);
         printf("max per mod %d\n", g_staticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod);
         printf("min per mod %d\n", g_staticParams.variableModParameters.varModList[i].iMinNumVarModAAPerMod);
         printf("term distance %d\n", g_staticParams.variableModParameters.varModList[i].iVarModTermDistance);
         printf("which term %d\n", g_staticParams.variableModParameters.varModList[i].iWhichTerm);
         printf("require this mod %d\n", g_staticParams.variableModParameters.varModList[i].iRequireThisMod);
         printf("binary mod %d\n", g_staticParams.variableModParameters.varModList[i].iBinaryMod);
         printf("neutral loss %lf\n", g_staticParams.variableModParameters.varModList[i].dNeutralLoss);
         printf("neutral loss2 %lf\n", g_staticParams.variableModParameters.varModList[i].dNeutralLoss2);
         printf("\n");
      }
   }
*/

   return true;
}

void CometSearchManager::AddInputFiles(vector<InputFileInfo*> &pvInputFiles)
{
   int numInputFiles = (int)pvInputFiles.size();

   for (int i = 0; i < numInputFiles; ++i)
      g_pvInputFiles.push_back(pvInputFiles.at(i));
}

void CometSearchManager::SetOutputFileBaseName(const char *pszBaseName)
{
   strcpy(g_staticParams.inputFile.szBaseName, pszBaseName);
}

std::map<std::string, CometParam*>& CometSearchManager::GetParamsMap()
{
   return _mapStaticParams;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const string& value)
{
   CometParam *pParam = new TypedCometParam<string>(CometParamType_String, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, string& value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<string> *pParam = static_cast<TypedCometParam<string>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const std::string &name, const string &strValue, const bool &value)
{
   CometParam *pParam = new TypedCometParam<int>(CometParamType_Bool, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, bool& value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<int> *pParam = static_cast<TypedCometParam<int>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const std::string &name, const string &strValue, const int &value)
{
   CometParam *pParam = new TypedCometParam<int>(CometParamType_Int, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, int& value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<int> *pParam = dynamic_cast<TypedCometParam<int>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const std::string &name, const string &strValue, const long &value)
{
   CometParam *pParam = new TypedCometParam<long>(CometParamType_Long, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, long& value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<long> *pParam = dynamic_cast<TypedCometParam<long>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const double &value)
{
   CometParam *pParam = new TypedCometParam<double>(CometParamType_Double, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, double& value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<double> *pParam = dynamic_cast<TypedCometParam<double>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const VarMods &value)
{
   CometParam *pParam = new TypedCometParam<VarMods>(CometParamType_VarMods, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, VarMods & value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<VarMods> *pParam = dynamic_cast<TypedCometParam<VarMods>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const DoubleRange &value)
{
   CometParam *pParam = new TypedCometParam<DoubleRange>(CometParamType_DoubleRange, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, DoubleRange &value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<DoubleRange> *pParam = dynamic_cast<TypedCometParam<DoubleRange>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const IntRange &value)
{
   CometParam *pParam = new TypedCometParam<IntRange>(CometParamType_IntRange, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, IntRange &value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<IntRange> *pParam = dynamic_cast<TypedCometParam<IntRange>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const EnzymeInfo &value)
{
   CometParam *pParam = new TypedCometParam<EnzymeInfo>(CometParamType_EnzymeInfo, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name, EnzymeInfo &value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam<EnzymeInfo> *pParam = dynamic_cast<TypedCometParam<EnzymeInfo>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::SetParam(const string &name, const string &strValue, const vector<double> &value)
{
   CometParam *pParam = new TypedCometParam< vector<double> >(CometParamType_DoubleVector, strValue, value);
   pair<map<string, CometParam*>::iterator,bool> ret = _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
   if (false == ret.second)
   {
      // insert() left the existing entry unchanged on failure -- ret.first points at it.
      // Free the CometParam* being replaced before overwriting it, instead of erase()+
      // insert(), which drops the map entry but leaks the heap object it pointed to.
      delete ret.first->second;
      ret.first->second = pParam;
   }
}

bool CometSearchManager::GetParamValue(const string &name,  vector<double> &value)
{
   std::map<string, CometParam*>::iterator it;
   it = _mapStaticParams.find(name);
   if (it == _mapStaticParams.end())
      return false;

   TypedCometParam< vector<double> > *pParam = dynamic_cast<TypedCometParam< vector<double> >*>(it->second);
   value = pParam->GetValue();

   return true;
}

bool CometSearchManager::IsSearchError()
{
    return g_cometStatus.IsError();
}

void CometSearchManager::GetStatusMessage(string &strStatusMsg)
{
   g_cometStatus.GetStatusMsg(strStatusMsg);
}

bool CometSearchManager::IsValidCometVersion(const string &version)
{
    // Major version number must match to current binary
    if (strstr(comet_version, version.c_str())
          || strstr(version.c_str(), "2026.0")
          || strstr(version.c_str(), "2025.0")
          || strstr(version.c_str(), "2024.0"))
    {
       return true;
    }
    else
       return false;
}

void CometSearchManager::CancelSearch()
{
    g_cometStatus.SetStatus(CometResult_Cancelled, string("Search was cancelled."));
}

bool CometSearchManager::IsCancelSearch()
{
    return g_cometStatus.IsCancel();
}

void CometSearchManager::ResetSearchStatus()
{
    g_cometStatus.ResetStatus();
}

bool CometSearchManager::CreateFragmentIndex()
{
   g_cometStatus.ResetStatus();

   // Override the Create Index flag to force it to create
   g_staticParams.options.bCreateFragmentIndex = true;

   // Signal DoSearch() that we are building the index for an RTS caller, so
   // it must NOT call _exit(0) after writing the .idx file -- the caller
   // (InitializeSingleSpectrumSearch) still needs to load and use the index.
   m_bRTSIndexBuild = true;
   bool bRet = DoSearch();
   m_bRTSIndexBuild = false;
   return bRet;
}

bool CometSearchManager::CreatePeptideIndex()
{
   g_cometStatus.ResetStatus();

   // Override the Create Index flag to force it to create
   g_staticParams.options.bCreatePeptideIndex = true;

   m_bRTSIndexBuild = true;
   bool bRet = DoSearch();
   m_bRTSIndexBuild = false;
   return bRet;
}

bool CometSearchManager::DoSearch()
{
   string strOut;

   ThreadPool *tp = _tp;

   if (!InitializeStaticParams())
      return false;

   if (!ValidateOutputFormat())
      return false;

   if (strlen(g_staticParams.databaseInfo.szDatabase) == 0 || !ValidateSequenceDatabaseFile())
      g_bPerformDatabaseSearch = false;
   else
      g_bPerformDatabaseSearch = true;

   if (g_staticParams.speclibInfo.strSpecLibFile.size() == 0 || !ValidateSpecLibFile())
      g_bPerformSpecLibSearch = false;
   else
      g_bPerformSpecLibSearch = true;

   if (g_bPerformDatabaseSearch == false && g_bPerformSpecLibSearch == false)
      return false;

   if (!ValidateScanRange())
      return false;

   if (!ValidatePeptideLengthRange())
      return false;

   bool bSucceeded = true;

   // repeated here from Comet main() as main() is skipped when search invoked via DLL
   g_sCometVersion = BuildCometVersionString();

   if (!g_staticParams.options.bOutputSqtStream)
   {
      strOut = "\n Comet version \"" + g_sCometVersion + "\"\n\n";
      logout(strOut);
      fflush(stdout);
   }

   try
   {
      tp->fillPool(g_staticParams.options.iNumThreads);
   }
   catch (const std::exception& e)
   {
      string strErrorMsg = " Error - thread pool initialization failed: " + std::string(e.what()) + "\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // -i and -j are now synonyms (docs/20260730_PI_reduction.md Phase 0): both flags build the
   // same unified index format via CometPeptideIndex::WritePeptideIndex() further below, so
   // there's no separate early-return path here any more -- a -j build now goes through the
   // same protein-var-mod-filter-file/compound-mods-file setup a -i build always did.
   //
   // This used to be gated to bCreateFragmentIndex/FASTA_DB only, on the theory that a PI_DB/
   // FI_DB search against an existing .idx got its ProteinModList:/bVarModProteinFilter
   // setting from the index header instead. Phase 0.5 (docs/20260730_PI_reduction.md) removed
   // ProteinModList: from the header entirely -- variable-mod settings are search-time-only
   // now, the same as a non-indexed FASTA search -- but left this gate unwidened, which
   // silently disabled protein_modslist_file filtering for every indexed search: iDbType is
   // never FASTA_DB there and bCreateFragmentIndex is false once the .idx already exists, so
   // ReadProteinVarModFilterFile() never ran and bVarModProteinFilter stayed at its default
   // (false) regardless of what the search-time comet.params specified. ValidateSequenceDatabaseFile()
   // above has already set iDbType by this point (PI_DB/FI_DB/FASTA_DB, whether the .idx
   // exists yet or needs building), so this now runs unconditionally: the per-peptide
   // siVarModProteinFilter bitmask each raw peptide carries was computed once at build time
   // from whatever protein_modslist_file was active then and is unaffected by this change
   // (see g_vRawPeptides in core/Types.h) -- a build/search protein_modslist_file mismatch is
   // the same category of caveat as MassRange:/LengthRange:/mod-mass mismatches already are
   // for this format, not something this fix introduces.

   // If specified, read in the protein variable mod filter file content.
   if (g_staticParams.variableModParameters.sProteinLModsListFile.length() > 0)
   {
      bool bVarModUsed = false;

      // Do a quick check to confirm there's a variable mod specified,
      // otherwise there's no point in parsing the file.
      for (int iMod = 0; iMod < VMODS; ++iMod)
      {
         if (g_staticParams.variableModParameters.varModList[iMod].dVarModMass != 0.0)
         {
            bVarModUsed = true;
            break;
         }
      }

      if (bVarModUsed)
      {
         bSucceeded = ReadProteinVarModFilterFile();
         if (!bSucceeded)
            return bSucceeded;
      }
   }

   // Load compound mods mass file if specified (B4 fix: only if parameter is explicitly set)
   if (g_staticParams.variableModParameters.sCompoundModsFile.length() > 0)
   {
      FILE *fpCM;
      if ((fpCM = fopen(g_staticParams.variableModParameters.sCompoundModsFile.c_str(), "r")) != NULL)
      {
         char szBuf[512];
         double dTmp;

         printf(" Parsing compoundmods file: %s\n", g_staticParams.variableModParameters.sCompoundModsFile.c_str());

         while (fgets(szBuf, sizeof(szBuf), fpCM))
         {
            if (sscanf(szBuf, "%lf", &dTmp) == 1)
               g_staticParams.variableModParameters.vdCompoundMasses.push_back(dTmp);
         }
         fclose(fpCM);

         sort(g_staticParams.variableModParameters.vdCompoundMasses.begin(),
              g_staticParams.variableModParameters.vdCompoundMasses.end());
         g_staticParams.variableModParameters.vdCompoundMasses.erase(
            unique(g_staticParams.variableModParameters.vdCompoundMasses.begin(),
                   g_staticParams.variableModParameters.vdCompoundMasses.end()),
            g_staticParams.variableModParameters.vdCompoundMasses.end());

         g_staticParams.variableModParameters.uiNumCompoundMasses = (unsigned int)g_staticParams.variableModParameters.vdCompoundMasses.size();

         if (g_staticParams.variableModParameters.uiNumCompoundMasses > 0)
            g_staticParams.variableModParameters.bVarModSearch = true;  // B6: drives WithVariableMods path; see docs for trade-off
      }
      else
      {
         string strErrorMsg = " Error - could not open compoundmods_file \"" + g_staticParams.variableModParameters.sCompoundModsFile + "\"\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }
   }

   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dH2O);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dNH3);
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   if (g_bPerformDatabaseSearch
      && (g_staticParams.options.bCreateFragmentIndex || g_staticParams.options.bCreatePeptideIndex)) //index
   {
       // write out the unified .idx file (docs/20260730_PI_reduction.md Phase 0; -i and -j
       // are now synonyms, both handled by this one builder); this calls RunSearch just to
       // query fasta and generate uniq peptide list
       bSucceeded = CometPeptideIndex::WritePeptideIndex(tp);
       if (!bSucceeded)
          return bSucceeded;

       CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

       if (g_pvInputFiles.size() == 0 && !m_bRTSIndexBuild)
       {
          // Standalone index-only run: the .idx file is written and fclose()
          // has already been called.  Skip all C++ static-duration destructors
          // -- the OS reclaims every page instantly.  g_pvDBIndex's pcVarModSites
          // is now an inline VarModSites (core/Types.h, no per-entry heap
          // allocation), so this mainly just avoids the O(n) vector<DBIndex>
          // teardown itself at ~80-90M entries rather than per-entry free() calls.
          fflush(stdout);
          fflush(stderr);
          _exit(0);
       }

       if (m_bRTSIndexBuild)
          return bSucceeded;  // index written; caller (InitializeSingleSpectrumSearch) will load it
   }

   // AScore initialization happens inside Pipeline::run(), after the strategy has
   // loaded its database/index -- see the comment there for why the ordering matters.

   if (g_bPerformSpecLibSearch)
      CometSpecLib::LoadSpecLib(g_staticParams.speclibInfo.strSpecLibFile);

   // Build search session with run-level flags.
   SearchSession session(g_cometStatus);
   session.bPerformDatabaseSearch = g_bPerformDatabaseSearch;
   session.bPerformSpecLibSearch  = g_bPerformSpecLibSearch;

   // Select strategy and create writers, then run the pipeline.
   std::unique_ptr<ISearchStrategy> pStrategy;
   if (g_staticParams.iDbType == DbType::FI_DB)
      pStrategy = std::make_unique<FiStrategy>();
   else if (g_staticParams.iDbType == DbType::PI_DB)
      pStrategy = std::make_unique<PiStrategy>();
   else
      pStrategy = std::make_unique<FastaStrategy>();

   // PepXML, mzIdentML, Percolator, Txt first; SQT last (WriteSqt modifies szMod).
   std::vector<std::unique_ptr<IResultWriter>> vWriters;
   if (g_staticParams.options.bOutputPepXMLFile)
      vWriters.push_back(std::make_unique<PepXmlWriter>());
   if (g_staticParams.options.iOutputMzIdentMLFile)
      vWriters.push_back(std::make_unique<MzIdentMlWriter>(this));
   if (g_staticParams.options.bOutputPercolatorFile)
      vWriters.push_back(std::make_unique<PercolatorWriter>());
   if (g_staticParams.options.bOutputTxtFile)
      vWriters.push_back(std::make_unique<TxtWriter>());
   if (g_staticParams.options.bOutputSqtFile || g_staticParams.options.bOutputSqtStream)
      vWriters.push_back(std::make_unique<SqtWriter>());

   Pipeline pipeline(std::move(pStrategy), std::move(vWriters), this);
   bSucceeded = pipeline.run(session, g_pvInputFiles, *tp);

   return bSucceeded;
}


// Shared by InitializeSingleSpectrumSearch() and InitializeSingleSpectrumMS1Search() --
// previously each had its own separate mutex, so concurrent first-time MS1+MS2 init from
// two C# Tasks (which the API permits; only the shipped driver happens to init
// sequentially) could run both functions' bodies at once. Both call the same
// InitializeStaticParams() (guarded only by a plain, non-atomic bool -- safe only because
// nothing else races it) and both fillPool() the same ThreadPool (no internal lock of its
// own) -- one shared mutex serializes the two init paths against each other, closing both
// races at once.
static std::mutex g_initSingleSpectrumMutex;

bool CometSearchManager::InitializeSingleSpectrumSearch()
{
   // Fast path: atomic acquire-load avoids data race while bypassing the mutex
   // when initialization is already complete.
   if (singleSearchInitializationComplete.load(std::memory_order_acquire))
      return true;

   std::lock_guard<std::mutex> lock(g_initSingleSpectrumMutex);

   // Double-check under the lock (relaxed is sufficient: the mutex provides
   // the required happens-before relationship).
   if (singleSearchInitializationComplete.load(std::memory_order_relaxed))
      return true;

   // Initialization code (no manual unlock needed!)
   if (!InitializeStaticParams())
      return false;

   if (!ValidateSequenceDatabaseFile())
      return false;

   // g_staticParams.iDbType is already set by this point: InitializeStaticParams() (called
   // just above) reads an existing .idx file's own IndexSearchType: header line via
   // CometPeptideIndex::ParsePeptideIndexHeader(), or ValidateSequenceDatabaseFile() (also
   // already called) set it from index_search_type for the "auto-build a missing .idx"
   // case -- see docs/20260811_restore_idx_header_mods.md. Nothing here re-derives the mode;
   // this block only clears the build-request flags once the mode is known, for the "file
   // already exists" case (an auto-build still in progress has no file yet, so fpCheck below
   // fails to open and this is a no-op, leaving bCreate*Index as ValidateSequenceDatabaseFile()
   // set them).
   size_t databaseLen = strlen(g_staticParams.databaseInfo.szDatabase);
   if (databaseLen >= 4 && strstr(g_staticParams.databaseInfo.szDatabase + databaseLen - 4, ".idx"))
   {
      FILE* fpCheck = fopen(g_staticParams.databaseInfo.szDatabase, "rb");
      if (fpCheck)
      {
         fclose(fpCheck);

         if (g_staticParams.iDbType == DbType::PI_DB)
         {
            g_staticParams.options.bCreatePeptideIndex = false;
            g_bPeptideIndexRead = false;
         }
         else if (g_staticParams.iDbType == DbType::FI_DB)
         {
            g_staticParams.options.bCreateFragmentIndex = false;
         }
      }
   }

   g_sCometVersion = BuildCometVersionString();

   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dH2O);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dNH3);
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   //MH: Allocate memory shared by threads during spectral processing.
   bool bSucceeded = CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
      return false;

   // Allocate memory shared by threads during search
   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
      return false;

   ThreadPool* tp = _tp;

   try
   {
      tp->fillPool(g_staticParams.options.iNumThreads);
   }
   catch (const std::exception& e)
   {
      string strErrorMsg = " Error - thread pool initialization failed: " + std::string(e.what()) + "\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   if (g_staticParams.iDbType == DbType::FI_DB)
   {
      // Load databases
      CometFragmentIndex sqSearch;

      if (g_staticParams.options.bCreateFragmentIndex)
      {
         bSucceeded = CreateFragmentIndex();
         if (!bSucceeded)
            return false;

         // DoSearch() (called inside CreateFragmentIndex) calls DeallocateMemory()
         // after writing the .idx file. Re-allocate here so AcquirePoolSlot() has
         // a valid _pbSearchMemoryPool when worker threads start searching.
         bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
         if (!bSucceeded)
            return false;
      }

      if (g_staticParams.iDbType == DbType::FI_DB && !g_bPlainPeptideIndexRead)
      {
         if (!CometPeptideIndex::ReadPeptideIndex(true))   // RTS: InitializeSingleSpectrumSearch()
         {
            string strErrorMsg = " Error - failed to read fragment index in InitializeSingleSpectrumSearch().\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         if (!sqSearch.CreateFragmentIndex(tp, true))
         {
            string strErrorMsg = " Error - failed to build in-memory fragment index in InitializeSingleSpectrumSearch().\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }

         if (g_staticParams.options.iPrintAScoreProScore)
         {
            // normally set at end of InitializeStaticParams; must do here again after
            // CometPeptideIndex::ReadPeptideIndex() for single spectrum search
            SetAScoreOptions(g_AScoreOptions);
            //       PrintAScoreOptions(g_AScoreOptions);

                     // Create the AScoreDllInterface using the factory function
            g_AScoreInterface = CreateAScoreDllInterface();
            if (!g_AScoreInterface)
            {
               std::cerr << "Failed to create AScore interface." << std::endl;
               return false;
            }
         }
      }

      // Freeze index (make immutable); already set in CometPeptideIndex::ReadPeptideIndex()
      // but doing here again to be safe for no good reason.
      g_bPlainPeptideIndexRead = true;
   }

   // Detect peptide index from .idx file header and load into memory.
   // This runs once under the singleSearchInitializationComplete atomic guard.
   if (g_staticParams.iDbType == DbType::PI_DB && !g_bPeptideIndexRead)
   {
      // Mirrors the FI_DB branch above: if the requested .idx doesn't exist yet,
      // ValidateSequenceDatabaseFile() already set bCreatePeptideIndex=true (and left
      // database_name pointing at the .idx path CometPeptideIndex::WritePeptideIndex()
      // needs to strip/restore -- see its own comment). Before index_search_type existed,
      // RTS could never reach PI_DB mode without an existing PI-format .idx already on
      // disk (dispatch was header-sniffed), so this auto-build path was previously
      // unreachable for PI_DB, not merely untested.
      if (g_staticParams.options.bCreatePeptideIndex)
      {
         if (!CometPeptideIndex::WritePeptideIndex(tp))
         {
            string strErrorMsg = " Error - failed to build peptide index in InitializeSingleSpectrumSearch().\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }

         // WritePeptideIndex() calls RunSearch internally to digest the FASTA, which
         // calls CometSearch::DeallocateMemory() before returning. Re-allocate here so
         // AcquirePoolSlot() has a valid _pbSearchMemoryPool when worker threads start
         // searching (mirrors the FI_DB branch's identical re-allocation above).
         bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
         if (!bSucceeded)
            return false;
      }

      if (!CometPeptideIndex::ReadPeptideIndex(true))   // RTS: InitializeSingleSpectrumSearch()
      {
         string strErrorMsg = " Error - failed to read peptide index in InitializeSingleSpectrumSearch().\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }

      // Re-initialize fragment masses from .idx header so they match
      // the static/variable mods used when building the index.
      // Without this, pdAAMassFragment retains values from
      // InitializeStaticParams() which may have double-applied static mods.
      if (!CometSearch::InitializeMassesFromPeptideIndex())
      {
         string strErrorMsg = " Error - failed to parse .idx header for mass initialization.\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }

      // Search memory (pbDuplFragment arrays) is already allocated by the
      // unconditional CometSearch::AllocateMemory() call earlier in this function;
      // nothing deallocates it on the PI_DB path before here.

      if (g_staticParams.options.iPrintAScoreProScore)
      {
         // Mirrors the FI_DB branch above -- without this, g_AScoreInterface stays
         // NULL for the entire session (CometSearch::EnsurePeptideIndexLoaded()'s own
         // AScoreInterface-creation code never runs for RTS PI_DB, since it's gated on
         // g_bPeptideIndexRead being false, and this function already set it to true
         // above before that code path is ever reached).
         SetAScoreOptions(g_AScoreOptions);

         g_AScoreInterface = CreateAScoreDllInterface();
         if (!g_AScoreInterface)
         {
            string strErrorMsg = " Error - failed to create AScore interface.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }

      g_bPeptideIndexRead = true;

      // Mass-init and (if enabled) AScorePro setup above are both done, so
      // EnsurePeptideIndexLoaded()'s unlocked fast-path check can safely treat the
      // index as fully ready from here on -- without this, every RTS query would take
      // EnsurePeptideIndexLoaded()'s locked slow path forever, since nothing else in
      // this exclusively-mutexed one-time init ever sets that flag.
      g_bPeptideIndexFullyInitialized = true;
   }

   // --- End: peptide index initialization for iDbType == 2 ---

   // Release-store ensures all initialization writes above are visible to any
   // thread that subsequently observes this flag as true.
   singleSearchInitializationComplete.store(true, std::memory_order_release);


   return true;
}


// Does NOT reset g_bPeptideIndexRead/g_bPlainPeptideIndexRead or free
// g_vRawPeptides/MOD_SEQS/MOD_NUMBERS/g_vDBIndexVariants/CometPeptideIndex::
// GetVModSlotForAllModsIdx()'s cached slot map -- see the PROCESS-LIFETIME note above
// CometPeptideIndex::ReadPeptideIndex()'s PermuteIndexPeptideMods() call for why a partial
// reset here (clearing just the two guard bools) would be actively worse than the current
// behavior, not better. Every current caller (RealtimeSearch.exe, rts_repro.cpp, batch
// comet.exe) calls Initialize.../Finalize... exactly once, immediately before the process
// exits, so this is a documented process-lifetime limitation, not a live bug.
void CometSearchManager::FinalizeSingleSpectrumSearch()
{
   if (singleSearchInitializationComplete.load(std::memory_order_acquire))
   {
      // Deallocate search memory
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

      // Deallocate preprocessing memory allocated in InitializeSingleSpectrumSearch()
      CometPreprocess::DeallocateMemory(g_staticParams.options.iNumThreads);

      if (g_staticParams.options.iPrintAScoreProScore)
         DeleteAScoreDllInterface(g_AScoreInterface);

      singleSearchInitializationComplete.store(false, std::memory_order_release);
   }
}


// Task 1.1 + 1.2: Load reference library once during init; fix thread pool deadlock
bool CometSearchManager::InitializeSingleSpectrumMS1Search(const double dMaxQueryRT)
{
   if (singleSearchMS1InitializationComplete.load(std::memory_order_acquire))
      return true;

   std::lock_guard<std::mutex> lock(g_initSingleSpectrumMutex);

   if (singleSearchMS1InitializationComplete.load(std::memory_order_relaxed))
      return true;

   if (!InitializeStaticParams())
      return false;

   // _tp may already be created by InitializeSingleSpectrumSearch(); if so, reuse it.
   // If not, create it here.
   if (_tp == nullptr)
      _tp = new ThreadPool();

   try
   {
      _tp->fillPool(g_staticParams.options.iNumThreads);
   }
   catch (const std::exception& e)
   {
      string strErrorMsg = " Error - thread pool initialization failed: " + std::string(e.what()) + "\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Task 1.1: Load reference library ONCE, here in init (single-threaded context).
   // After this call, g_vSpecLib is populated and g_bSpecLibRead == true.
   if (!CometSpecLib::LoadSpecLibMS1Raw(_tp, dMaxQueryRT, &dMaxSpecLibRT))
      return false;

   singleSearchMS1InitializationComplete.store(true, std::memory_order_release);
   return true;
}


void CometSearchManager::FinalizeSingleSpectrumMS1Search()
{
   if (singleSearchMS1InitializationComplete.load(std::memory_order_acquire))
   {
      // MS1 init (InitializeSingleSpectrumMS1Search) never allocates the CometSearch
      // pool -- it only loads g_vSpecLib. Do not deallocate it here; that pool belongs
      // to the independent MS2 lifecycle (InitializeSingleSpectrumSearch /
      // FinalizeSingleSpectrumSearch) and may still be in use by MS2 search threads.
      singleSearchMS1InitializationComplete.store(false, std::memory_order_release);
   }
}


bool CometSearchManager::DoSingleSpectrumSearchMultiResults(const int topN,
                                                            int iPrecursorCharge,
                                                            double dMZ,
                                                            double* pdMass,
                                                            double* pdInten,
                                                            int iNumPeaks,
                                                            vector<string>& strReturnPeptide,
                                                            vector<string>& strReturnProtein,
                                                            vector<vector<Fragment>>& matchedFragments,
                                                            vector<CometScores>& scores)
{
   if (iNumPeaks == 0)
      return false;

   if (dMZ * iPrecursorCharge - (iPrecursorCharge - 1.0) * PROTON_MASS > g_staticParams.options.dPeptideMassHigh)
      return false;    // this assumes dPeptideMassHigh is set correctly in the calling program

   if (!InitializeSingleSpectrumSearch())
      return false;

   bool bSucceeded = true;

#ifdef RTS_TIMING
   using hrc = std::chrono::high_resolution_clock;
   using chus = std::chrono::microseconds;
   auto   tTimingStart     = hrc::now();
   auto   tTimingMark      = tTimingStart;
   long long llPreprocess  = 0;
   long long llRunSearch   = 0;
   long long llSort        = 0;
   long long llCalcSP      = 0;
   long long llCalcEValue  = 0;
   long long llCalcDeltaCn = 0;
   long long llCalcAScore  = 0;
   long long llResults     = 0;
#endif

   // Obtain the thread-local raw-data buffer managed by RtsScratch.
   // This avoids a per-spectrum new[]/delete[] of iArraySizeGlobal doubles
   // (~40 KB) while also ensuring the pool is initialised for this thread.
   // After PreprocessSingleSpectrumThreadLocal returns, the buffer holds
   // the binned sqrt-intensity spectrum needed for fragment-ion matching below.
   double* pdTmpSpectrum = CometPreprocess::GetRtsRawDataBuffer();

   // Step 1: Preprocess into a thread-local Query* (does NOT touch session.queries)
#ifdef RTS_TIMING
   tTimingMark = hrc::now();
#endif
   Query* pQuery = CometPreprocess::PreprocessSingleSpectrumThreadLocal(
      iPrecursorCharge, dMZ, pdMass, pdInten, iNumPeaks, pdTmpSpectrum);
#ifdef RTS_TIMING
   llPreprocess = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
#endif

   if (pQuery == nullptr)
   {
      return false;
   }

/*
   // Validate the sparse matrix spectrum for pQuery is valid and not empty after preprocessing (e.g. all peaks filtered out).
   // first check if first dimension of ppSparseFastXcorrData is non-null
   bool bEmpty = true;
   int iMax = pQuery->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE;
   for (int i = 0; i < iMax; ++i)
   {
      if (pQuery->ppfSparseFastXcorrData[i] != nullptr)
      {
         bEmpty = false;
         break;
      }
   }
   if (!bEmpty)
   {
      printf("OK this spectrum is not empty\n");
   }
   else
      return false;
*/

   // Record per-query start time for iMaxIndexRunTime timeout (thread-safe; avoids writing to global)
   pQuery->tSearchStart = std::chrono::high_resolution_clock::now();

   // Step 3: Run the fragment index search on the thread-local Query*
   // This uses the new RunSearch(Query*) overload that allocates its own
   // pbDuplFragment and never touches session.queries or _ppbDuplFragmentArr.
#ifdef RTS_TIMING
   tTimingMark = hrc::now();
#endif
   bSucceeded = CometSearch::RunSearch(pQuery);
#ifdef RTS_TIMING
   llRunSearch = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
#endif

   FILE* fp = NULL;
   int iSize;
   int takeSearchResultsN;

   if (!bSucceeded)
      goto cleanup_results;

   iSize = pQuery->iMatchPeptideCount;
   if (iSize > g_staticParams.options.iNumStored)
      iSize = g_staticParams.options.iNumStored;

   takeSearchResultsN = topN;
   if (takeSearchResultsN > iSize)
      takeSearchResultsN = iSize;

   if (g_staticParams.options.iMaxIndexRunTime > 0)
   {
      auto tNow = std::chrono::high_resolution_clock::now();
      auto tElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - pQuery->tSearchStart).count();
      if (tElapsedTime >= g_staticParams.options.iMaxIndexRunTime)
         goto cleanup_results;
   }

#ifdef RTS_TIMING
   tTimingMark = hrc::now();
#endif
   if (iSize > 1)
   {
      // Only a prefix needs to be fully, correctly sorted -- the rest of
      // _pResults[0, iSize) never gets read again this call:
      //  - the output-extraction loop below only reads [0, takeSearchResultsN)
      //  - CalculateSP() only reads [0, takeSearchResultsN) (CometPostAnalysis.cpp:498)
      //  - CalculateEValue() does not depend on sort order at all -- dExpect is a
      //    pure function of each entry's own fXcorr (CometPostAnalysis.cpp:1403-1430)
      //  - CalculateDeltaCn() -> CalculateDeltaCnsAndRank() walks up to
      //    iNumPeptideOutputLines + 1 entries and DOES depend on adjacency-based
      //    sort order to compute delta-Cn/rank correctly for the entries actually
      //    returned (CometPostAnalysis.cpp:269-363) -- this is the binding
      //    constraint whenever it exceeds takeSearchResultsN.
      // iNumStored is already >= iNumPeptideOutputLines + 1 (enforced at
      // CometSearchManager.cpp:1393-1395), so clamping to iSize (<= iNumStored)
      // can only shrink the bound when the search itself found fewer than
      // iNumPeptideOutputLines + 1 candidates -- never invalidate it.
      int iSortBound = takeSearchResultsN;
      if (g_staticParams.options.iNumPeptideOutputLines + 1 > iSortBound)
         iSortBound = g_staticParams.options.iNumPeptideOutputLines + 1;
      if (iSortBound > iSize)
         iSortBound = iSize;

      std::partial_sort(pQuery->_pResults, pQuery->_pResults + iSortBound,
         pQuery->_pResults + iSize, CometPostAnalysis::SortFnXcorr);
   }
#ifdef RTS_TIMING
   llSort = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
#endif

   // Step 4: Post-analysis using Query* overloads (no session.queries access)
   if (pQuery->iMatchPeptideCount > 0)
   {
      if (g_staticParams.options.iMaxIndexRunTime > 0)
      {
         auto tNow = std::chrono::high_resolution_clock::now();
         auto tElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - pQuery->tSearchStart).count();
         if (tElapsedTime >= g_staticParams.options.iMaxIndexRunTime)
            goto cleanup_results;
      }

#ifdef RTS_TIMING
      tTimingMark = hrc::now();
#endif
      CometPostAnalysis::CalculateSP(pQuery->_pResults, pQuery, takeSearchResultsN);
#ifdef RTS_TIMING
      llCalcSP = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
      tTimingMark = hrc::now();
#endif

      if (g_staticParams.options.iMaxIndexRunTime > 0)
      {
         auto tNow = std::chrono::high_resolution_clock::now();
         auto tElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - pQuery->tSearchStart).count();
         if (tElapsedTime >= g_staticParams.options.iMaxIndexRunTime)
            goto cleanup_results;
      }

      CometPostAnalysis::CalculateEValue(pQuery, false);
#ifdef RTS_TIMING
      llCalcEValue = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
      tTimingMark = hrc::now();
#endif

      if (g_staticParams.options.iMaxIndexRunTime > 0)
      {
         auto tNow = std::chrono::high_resolution_clock::now();
         auto tElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - pQuery->tSearchStart).count();
         if (tElapsedTime >= g_staticParams.options.iMaxIndexRunTime)
            goto cleanup_results;
      }

      CometPostAnalysis::CalculateDeltaCn(pQuery);
#ifdef RTS_TIMING
      llCalcDeltaCn = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
#endif

      if ((g_staticParams.options.iPrintAScoreProScore == -1 || g_staticParams.options.iPrintAScoreProScore > 0)
         && pQuery->_pResults[0].cHasVariableMod == HasVariableModType_AScorePro)
      {
         bool bHasTerminalVariableMod = false;
         if (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide] != 0
            || pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide + 1] != 0)
         {
            bHasTerminalVariableMod = true;
         }
         if (!bHasTerminalVariableMod)
         {
            if (g_staticParams.options.iMaxIndexRunTime > 0)
            {
               auto tNow = std::chrono::high_resolution_clock::now();
               auto tElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - pQuery->tSearchStart).count();
               if (tElapsedTime >= g_staticParams.options.iMaxIndexRunTime)
                  goto cleanup_results;
            }

#ifdef RTS_TIMING
            tTimingMark = hrc::now();
#endif
            CometPostAnalysis::CalculateAScorePro(pQuery, g_AScoreInterface);
#ifdef RTS_TIMING
            llCalcAScore = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
#endif
         }
      }
   }
   else
   {
      goto cleanup_results;
   }

   if (g_cometStatus.IsCancel())
   {
      bSucceeded = false;
      goto cleanup_results;
   }

   if (g_staticParams.options.iMaxIndexRunTime > 0)
   {
      auto tNow = std::chrono::high_resolution_clock::now();
      auto tElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - pQuery->tSearchStart).count();
      if (tElapsedTime >= g_staticParams.options.iMaxIndexRunTime)
         goto cleanup_results;
   }

   // Step 5: Open database file for protein name retrieval.
   // For indexed databases (FI_DB, PI_DB), names are served from the in-memory
   // g_pvProteinNameCache populated at init -- no file I/O per spectrum.
   // For FASTA_DB, we still need to open the file.
#ifdef RTS_TIMING
   tTimingMark = hrc::now();
#endif
   if (g_staticParams.iDbType == DbType::FASTA_DB)
   {
      if ((fp = fopen(g_staticParams.databaseInfo.szDatabase, "rb")) == NULL)
      {
         string strErrorMsg = " Error - cannot read database file \"" + std::string(g_staticParams.databaseInfo.szDatabase)
            + "\" " + std::strerror(errno) + "\n.";
         logerr(strErrorMsg);
         bSucceeded = false;
         goto cleanup_results;
      }
   }

   // Step 6: Extract results from thread-local Query*
   strReturnPeptide.reserve(strReturnPeptide.size() + takeSearchResultsN);
   strReturnProtein.reserve(strReturnProtein.size() + takeSearchResultsN);
   matchedFragments.reserve(matchedFragments.size() + takeSearchResultsN);
   scores.reserve(scores.size() + takeSearchResultsN);

   // Reused across results (this call) and across spectra (this thread) instead of
   // being declared fresh per result -- see docs/20260714_rtspostprocessing.md
   // finding 5. This function is the RTS single-spectrum entry point only (never
   // called from the batch multi-threaded path), so thread_local is safe here.
   thread_local std::vector<string> tl_vProteinTargets;
   thread_local std::vector<string> tl_vProteinDecoys;

   for (int iWhichResult = 0; iWhichResult < takeSearchResultsN; ++iWhichResult)
   {
      CometScores score;
      score.dCn = 0;
      score.xCorr = g_staticParams.options.dMinimumXcorr;
      score.matchedIons = 0;
      score.totalIons = 0;
      score.dAScorePro = 0;
      score.sAScoreProSiteScores.clear();
      std::string eachStrReturnPeptide;
      std::string eachStrReturnProtein;
      vector<Fragment> eachMatchedFragments;

      if (iSize > 0 && pQuery->_pResults[iWhichResult].fXcorr > g_staticParams.options.dMinimumXcorr
         && pQuery->_pResults[iWhichResult].usiLenPeptide > 0)
      {
         Results* pOutput = pQuery->_pResults;

         // Set return values for peptide sequence, protein, xcorr and E-value
         eachStrReturnPeptide = std::string(1, pOutput[iWhichResult].cPrevAA) + ".";

         // n-term variable mod
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] != 0)
         {
            char szMod[32];
            snprintf(szMod, sizeof(szMod), "n[%.4f]", pOutput[iWhichResult].pdVarModSites[pOutput[iWhichResult].usiLenPeptide]);
            eachStrReturnPeptide += szMod;
         }

         for (int i = 0; i < pOutput[iWhichResult].usiLenPeptide; ++i)
         {
            eachStrReturnPeptide += pOutput[iWhichResult].szPeptide[i];

            if (pOutput[iWhichResult].piVarModSites[i] != 0)
            {
               char szMod[32];
               snprintf(szMod, sizeof(szMod), "[%.4f]", pOutput[iWhichResult].pdVarModSites[i]);
               eachStrReturnPeptide += szMod;
            }
         }

         // c-term variable mod
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide + 1] != 0)
         {
            char szMod[32];
            snprintf(szMod, sizeof(szMod), "c[%.4f]", pOutput[iWhichResult].pdVarModSites[pOutput[iWhichResult].usiLenPeptide + 1]);
            eachStrReturnPeptide += szMod;
         }
         eachStrReturnPeptide += "." + std::string(1, pOutput[iWhichResult].cNextAA);

         // Protein name retrieval.
         // Indexed DB (FI_DB, PI_DB): look up the in-memory cache built at init time.
         // FASTA_DB: seek and read from the file handle opened above.
         int iLenDecoyPrefix = (int)strlen(g_staticParams.szDecoyPrefix);

         tl_vProteinTargets.clear();
         tl_vProteinDecoys.clear();
         std::vector<string>& vProteinTargets = tl_vProteinTargets;
         std::vector<string>& vProteinDecoys = tl_vProteinDecoys;

         if (g_staticParams.iDbType != DbType::FASTA_DB)
         {
            comet_fileoffset_t lEntry = pOutput[iWhichResult].lProteinFilePosition;
            int iPrintDuplicateProteinCt = 0;

            for (auto itProt = g_pvProteinsList.at(lEntry).begin(); itProt != g_pvProteinsList.at(lEntry).end(); ++itProt)
            {
               auto cacheIt = g_pvProteinNameCache.find(*itProt);
               if (cacheIt == g_pvProteinNameCache.end())
                  continue;

               const string& sName = cacheIt->second;
               if (!strncmp(sName.c_str(), g_staticParams.szDecoyPrefix, iLenDecoyPrefix))
                  vProteinDecoys.push_back(sName);
               else
                  vProteinTargets.push_back(sName);

               iPrintDuplicateProteinCt++;
               if (iPrintDuplicateProteinCt >= g_staticParams.options.iMaxDuplicateProteins)
                  break;
            }
         }
         else
         {
            // Non-indexed (FASTA) path: read protein names from the open file handle
            char szProteinName[WIDTH_REFERENCE];
            int iPrintDuplicateProteinCt = 0;

            if (pOutput[iWhichResult].pWhichProtein.size() > 0)
            {
               for (auto itProt = pOutput[iWhichResult].pWhichProtein.begin(); itProt != pOutput[iWhichResult].pWhichProtein.end(); ++itProt)
               {
                  comet_fseek(fp, (*itProt).lWhichProtein, SEEK_SET);
                  if (fgets(szProteinName, WIDTH_REFERENCE, fp) == NULL)
                  {
                     // error
                  }
                  szProteinName[WIDTH_REFERENCE - 1] = '\0';
                  while (strlen(szProteinName) > 0
                     && (szProteinName[strlen(szProteinName) - 1] == '\n'
                        || szProteinName[strlen(szProteinName) - 1] == '\r'))
                  {
                     szProteinName[strlen(szProteinName) - 1] = '\0';
                  }
                  vProteinTargets.push_back(szProteinName);
                  iPrintDuplicateProteinCt++;
                  if (iPrintDuplicateProteinCt > g_staticParams.options.iMaxDuplicateProteins)
                     break;
               }
            }

            if (pOutput[iWhichResult].pWhichDecoyProtein.size() > 0)
            {
               for (auto itProt = pOutput[iWhichResult].pWhichDecoyProtein.begin(); itProt != pOutput[iWhichResult].pWhichDecoyProtein.end(); ++itProt)
               {
                  if (iPrintDuplicateProteinCt >= g_staticParams.options.iMaxDuplicateProteins)
                     break;
                  comet_fseek(fp, (*itProt).lWhichProtein, SEEK_SET);
                  if (fgets(szProteinName, WIDTH_REFERENCE, fp) == NULL)
                  {
                     // error
                  }
                  szProteinName[WIDTH_REFERENCE - 1] = '\0';
                  while (strlen(szProteinName) > 0
                     && (szProteinName[strlen(szProteinName) - 1] == '\n'
                        || szProteinName[strlen(szProteinName) - 1] == '\r'))
                  {
                     szProteinName[strlen(szProteinName) - 1] = '\0';
                  }
                  vProteinDecoys.push_back(szProteinName);
                  iPrintDuplicateProteinCt++;
               }
            }
         }

         // Build the protein string from targets and decoys
         bool bPrintDelim = false;
         for (auto itProt = vProteinTargets.begin(); itProt != vProteinTargets.end(); ++itProt)
         {
            if (bPrintDelim)
               eachStrReturnProtein += " ; ";
            else
               bPrintDelim = true;

            string sTmp = *itProt;
            std::replace(sTmp.begin(), sTmp.end(), ';', ',');
            eachStrReturnProtein += sTmp;
         }

         for (auto itProt = vProteinDecoys.begin(); itProt != vProteinDecoys.end(); ++itProt)
         {
            if (bPrintDelim)
               eachStrReturnProtein += " ; ";
            else
               bPrintDelim = true;

            string sTmp;
            if ((*itProt).starts_with(g_staticParams.szDecoyPrefix))
               sTmp = *itProt;
            else
            {
               sTmp = g_staticParams.szDecoyPrefix;
               sTmp += *itProt;
            }

            std::replace(sTmp.begin(), sTmp.end(), ';', ',');
            eachStrReturnProtein += sTmp;
         }

         score.xCorr = pOutput[iWhichResult].fXcorr;
         score.dCn = pOutput[iWhichResult].fDeltaCn;
         score.dSp = pOutput[iWhichResult].fScoreSp;
         score.dExpect = pOutput[iWhichResult].dExpect;
         score.mass = pOutput[iWhichResult].dPepMass - PROTON_MASS;
         score.matchedIons = pOutput[iWhichResult].usiMatchedIons;
         score.totalIons = pOutput[iWhichResult].usiTotalIons;
         score.dAScorePro = pOutput[iWhichResult].fAScorePro;
         score.sAScoreProSiteScores = pOutput[iWhichResult].sAScoreProSiteScores;

         // Conversion table from b/y ions to the other types (a,c,x,z)
         const double ionMassesRelative[NUM_ION_SERIES] =
         {
            // N term relative
            -(Carbon_Mono + Oxygen_Mono),                       // a (CO difference from b)
            0,                                                  // b
            (Nitrogen_Mono + (3 * Hydrogen_Mono)),              // c (NH3 difference from b)

            // C Term relative
            (Carbon_Mono + Oxygen_Mono - (2 * Hydrogen_Mono)),  // x (CO-2H difference from y)
            0,                                                  // y
            -(Nitrogen_Mono + (2 * Hydrogen_Mono)),             // z (NH2 difference from y)
            -(Nitrogen_Mono + (3 * Hydrogen_Mono))              // z+1
         };

         // now deal with calculating b- and y-ions and returning most intense matches
         double dBion = g_staticParams.precalcMasses.dNtermProton;
         double dYion = g_staticParams.precalcMasses.dCtermOH2Proton;

         if (pQuery->_pResults[iWhichResult].cPrevAA == '-')
         {
            dBion += g_staticParams.staticModifications.dAddNterminusProtein;
         }
         if (pQuery->_pResults[iWhichResult].cNextAA == '-')
         {
            dYion += g_staticParams.staticModifications.dAddCterminusProtein;
         }

         // mods at peptide length +1 and +2 are for n- and c-terminus
         if (g_staticParams.variableModParameters.bVarModSearch
            && (pQuery->_pResults[iWhichResult].piVarModSites[pQuery->_pResults[iWhichResult].usiLenPeptide] != 0))
         {
            dBion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[iWhichResult].piVarModSites[pQuery->_pResults[iWhichResult].usiLenPeptide] - 1].dVarModMass;
         }

         if (g_staticParams.variableModParameters.bVarModSearch
            && (pQuery->_pResults[iWhichResult].piVarModSites[pQuery->_pResults[iWhichResult].usiLenPeptide + 1] != 0))
         {
            dYion += g_staticParams.variableModParameters.varModList[pQuery->_pResults[iWhichResult].piVarModSites[pQuery->_pResults[iWhichResult].usiLenPeptide + 1] - 1].dVarModMass;
         }

         int iTmp;
         bool bAddNtermFragmentNeutralLoss[VMODS];
         bool bAddCtermFragmentNeutralLoss[VMODS];

         for (int iMod = 0; iMod < VMODS; ++iMod)
         {
            bAddNtermFragmentNeutralLoss[iMod] = false;
            bAddCtermFragmentNeutralLoss[iMod] = false;
         }

         // Upper bound on matched fragments (ignoring neutral losses, which are
         // comparatively rare) -- avoids repeated push_back() reallocation growth
         // for longer peptides / higher charge states / multiple ion series enabled.
         eachMatchedFragments.reserve((size_t)(pQuery->_pResults[iWhichResult].usiLenPeptide - 1)
            * pQuery->_spectrumInfoInternal.usiMaxFragCharge
            * g_staticParams.ionInformation.iNumIonSeriesUsed);

         // Generate pdAAforward for pQuery->_pResults[iWhichResult].szPeptide.
         for (int i = 0; i < pQuery->_pResults[iWhichResult].usiLenPeptide - 1; ++i)
         {
            int iPos = pQuery->_pResults[iWhichResult].usiLenPeptide - i - 1;

            dBion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[iWhichResult].szPeptide[i]];
            dYion += g_staticParams.massUtility.pdAAMassFragment[(int)pQuery->_pResults[iWhichResult].szPeptide[iPos]];

            if (g_staticParams.variableModParameters.bVarModSearch)
            {
               if (pQuery->_pResults[iWhichResult].piVarModSites[i] != 0)
                  dBion += pQuery->_pResults[iWhichResult].pdVarModSites[i];

               if (pQuery->_pResults[iWhichResult].piVarModSites[iPos] != 0)
                  dYion += pQuery->_pResults[iWhichResult].pdVarModSites[iPos];
            }

            for (int ctCharge = 1; ctCharge <= pQuery->_spectrumInfoInternal.usiMaxFragCharge; ++ctCharge)
            {
               // calculate every ion series the user specified
               for (int ionSeries = 0; ionSeries < NUM_ION_SERIES; ++ionSeries)
               {
                  // skip ion series that are not enabled.
                  if (!g_staticParams.ionInformation.iIonVal[ionSeries])
                  {
                     continue;
                  }

                  bool isNTerm = (ionSeries <= ION_SERIES_C);

                  // get the fragment mass if it is n- or c-terminus
                  double mass = (isNTerm) ? dBion : dYion;
                  int fragNumber = i + 1;

                  // Add any conversion factor from different ion series (e.g. b -> a, or y -> z)
                  mass += ionMassesRelative[ionSeries];

                  double mz = (mass + (ctCharge - 1) * PROTON_MASS) / ctCharge;
                  iTmp = BIN(mz);
                  if (iTmp < g_staticParams.iArraySizeGlobal && pdTmpSpectrum[iTmp] > 0.0)
                  {
                     Fragment frag;
                     frag.intensity = pdTmpSpectrum[iTmp];
                     frag.mass = mass;
                     frag.type = ionSeries;
                     frag.number = fragNumber;
                     frag.charge = ctCharge;
                     frag.neutralLoss = false;
                     frag.neutralLossMass = 0.0;
                     eachMatchedFragments.push_back(frag);
                  }

                  if (g_staticParams.variableModParameters.bUseFragmentNeutralLoss)
                  {
                     for (int iMod = 0; iMod < VMODS; ++iMod)
                     {
                        for (int iWhichNL = 0; iWhichNL < 2; ++iWhichNL)
                        {
                           double dNLmass;

                           if (iWhichNL == 0)
                              dNLmass = g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss;
                           else
                              dNLmass = g_staticParams.variableModParameters.varModList[iMod].dNeutralLoss2;

                           if (dNLmass == 0.0 || g_staticParams.variableModParameters.varModList[iMod].dVarModMass == 0.0)
                           {
                              continue;
                           }

                           if (isNTerm)
                           {
                              if (!bAddNtermFragmentNeutralLoss[iMod] && pOutput[iWhichResult].piVarModSites[i] == iMod + 1)
                              {
                                 bAddNtermFragmentNeutralLoss[iMod] = true;
                              }
                           }
                           else
                           {
                              if (!bAddCtermFragmentNeutralLoss[iMod] && pOutput[iWhichResult].piVarModSites[iPos] == iMod + 1)
                              {
                                 bAddCtermFragmentNeutralLoss[iMod] = true;
                              }
                           }

                           if ((isNTerm && !bAddNtermFragmentNeutralLoss[iMod])
                              || (!isNTerm && !bAddCtermFragmentNeutralLoss[iMod]))
                           {
                              continue;
                           }

                           double dNLfragMz = mz - (dNLmass / ctCharge);
                           iTmp = BIN(dNLfragMz);
                           if (iTmp < g_staticParams.iArraySizeGlobal && iTmp >= 0 && pdTmpSpectrum[iTmp] > 0.0)
                           {
                              Fragment frag;
                              frag.intensity = pdTmpSpectrum[iTmp];
                              frag.mass = mass - dNLmass;
                              frag.type = ionSeries;
                              frag.number = fragNumber;
                              frag.charge = ctCharge;
                              frag.neutralLoss = true;
                              frag.neutralLossMass = dNLmass;
                              eachMatchedFragments.push_back(frag);
                           }
                        }
                     }
                  }
               }
            }
         }
      }
      else
      {
         eachStrReturnPeptide = "";
         eachStrReturnProtein = "";
         score.xCorr = -999;
         score.dSp = 0;
         score.dExpect = 999;
         score.mass = 0;
         score.matchedIons = 0;
         score.totalIons = 0;
         score.dAScorePro = 0;
         score.dCn = 0;
         score.sAScoreProSiteScores.clear();
      }

      if (false)  // set to true to enable debug mass check
      {
         // Compare peptide against input mz/charge mass
         double dCalcPepMass = g_staticParams.precalcMasses.dNtermProton + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS;
         for (int i = 0; i < pQuery->_pResults[0].usiLenPeptide; ++i)
         {
            dCalcPepMass += g_staticParams.massUtility.pdAAMassParent[pQuery->_pResults[0].szPeptide[i]];
            if (g_staticParams.variableModParameters.bVarModSearch)
               if (pQuery->_pResults[0].piVarModSites[i] != 0)
                  dCalcPepMass += pQuery->_pResults[0].pdVarModSites[i];
         }
         if (g_staticParams.variableModParameters.bVarModSearch)
         {
            // n-term mod
            if (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide] != 0)
               dCalcPepMass += pQuery->_pResults[0].pdVarModSites[pQuery->_pResults[0].usiLenPeptide];
            // c-term mod
            if (pQuery->_pResults[0].piVarModSites[pQuery->_pResults[0].usiLenPeptide + 1] != 0)
               dCalcPepMass += pQuery->_pResults[0].pdVarModSites[pQuery->_pResults[0].usiLenPeptide + 1];
         }
      }

      strReturnPeptide.push_back(eachStrReturnPeptide);
      strReturnProtein.push_back(eachStrReturnProtein);
      matchedFragments.push_back(eachMatchedFragments);
      scores.push_back(score);
   }
#ifdef RTS_TIMING
   llResults = std::chrono::duration_cast<chus>(hrc::now() - tTimingMark).count();
#endif

cleanup_results:

#ifdef RTS_TIMING
   {
      long long llTotal = std::chrono::duration_cast<chus>(hrc::now() - tTimingStart).count();
      printf("TIMING\t%.4f\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\n",
             dMZ, iPrecursorCharge,
             llPreprocess, llRunSearch, llSort, llCalcSP, llCalcEValue, llCalcDeltaCn, llCalcAScore, llResults, llTotal);
   }
#endif

   // Clean up the thread-local Query* - its destructor frees spectral memory
   // (pfFastXcorrData, pfFastXcorrDataNL, etc.)
   // The _pResults and _pDecoys arrays were allocated by PreprocessSingleSpectrumCore.
   if (fp != NULL)
      fclose(fp);

   delete pQuery;
   // pdTmpSpectrum is owned by the thread-local RtsScratch pool; do not delete.

   return bSucceeded;
}


// Load all MS1 from raw file. Then search each MS1 query.
bool CometSearchManager::DoMS1SearchMultiResults(const double dMaxMS1RTDiff,
                                                 const double dMaxQueryRT,
                                                 const int topN,
                                                 const double dQueryRT,
                                                 double* pdMass,
                                                 double* pdInten,
                                                 int iNumPeaks,
                                                 vector<CometScoresMS1>& scoresMS1)
{
   // Phase 3 / Task 3.2: Verify init completed before any search call.
   // InitializeSingleSpectrumMS1Search() must be called (and return) before
   // any thread calls DoMS1SearchMultiResults(). The C# Task.Run() scheduling
   // provides a happens-before guarantee for all writes made during init.
   if (!singleSearchMS1InitializationComplete.load(std::memory_order_acquire))
   {
      string strErrorMsg = " Error - DoMS1SearchMultiResults() called before InitializeSingleSpectrumMS1Search() completed.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // At this point g_vSpecLib and g_bSpecLibRead are guaranteed visible and immutable.

   if (iNumPeaks == 0)
      return false;

   // Task 2.1: Create thread-local QueryMS1* - no global state touched.
   QueryMS1* pQueryMS1 = CometPreprocess::PreprocessMS1SingleSpectrumThreadLocal(pdMass, pdInten, iNumPeaks);

   if (pQueryMS1 == nullptr)
      return false;

   // Task 2.2: Run search using thread-local overload - reads only g_vSpecLib (immutable).
   vector<CometScoresMS1> localScores;
   bool bSucceeded = CometSearch::RunMS1Search(pQueryMS1, dQueryRT, dMaxMS1RTDiff, dMaxSpecLibRT, dMaxQueryRT, localScores);

   if (bSucceeded && !localScores.empty())
   {
      // Pass best RT match to the global regression aligner.
      // pMS1Aligner accumulates RT history across all scans in the run, so it
      // must be shared. Protect it with its dedicated mutex.
      double dMatchedSpecLibRT = localScores[0].fRTime;

      Threading::LockMutex(g_ms1AlignerMutex);
      double dLinearRegressionRT = pMS1Aligner.processRetentionMatch(dQueryRT, dMatchedSpecLibRT);
      Threading::UnlockMutex(g_ms1AlignerMutex);

      // Overwrite the RT with the regression-adjusted value
      localScores[0].fRTime = (float)dLinearRegressionRT;

      scoresMS1 = std::move(localScores);
   }

   // Task 2.3: Clean up thread-local QueryMS1*
   if (pQueryMS1->pfFastXcorrData != nullptr)
   {
      delete[] pQueryMS1->pfFastXcorrData;
      pQueryMS1->pfFastXcorrData = nullptr;
   }
   delete pQueryMS1;
   pQueryMS1 = nullptr;

   return bSucceeded;
}


// Restrict variable mods to a list of proteins that are read here
// File format is an "int string" on each line where "int" is the
// variable modfication number and "string" is a single protein accession word
bool CometSearchManager::ReadProteinVarModFilterFile()
{
   FILE* fp;
   char szBuf[WIDTH_REFERENCE];

   if ((fp = fopen(g_staticParams.variableModParameters.sProteinLModsListFile.c_str(), "r")) != NULL)
   {
      printf(" Protein variable modifications filter:\n");

      while (fgets(szBuf, WIDTH_REFERENCE, fp))
      {
         if (strlen(szBuf) > 3)
         {
            char szProtein[WIDTH_REFERENCE];
            int iWhichMod;

            if (sscanf(szBuf, "%d %s", &iWhichMod, szProtein) == 2)
            {
               if (iWhichMod > 0 && iWhichMod <= VMODS)
               {
                  // check if specified iWhichMod actually corresponds to a non-zero variable mod
                  if (!isEqual(g_staticParams.variableModParameters.varModList[iWhichMod -1].dVarModMass, 0.0))
                     g_staticParams.variableModParameters.mmapProteinModsList.insert({ iWhichMod, szProtein });
               }
            }
         }
      }
      fclose(fp);

      if (g_staticParams.variableModParameters.mmapProteinModsList.size() > 0)
      {
         g_staticParams.variableModParameters.bVarModProteinFilter = true;

         // print out the parsed proteins
         auto it = g_staticParams.variableModParameters.mmapProteinModsList.begin();
         while (it != g_staticParams.variableModParameters.mmapProteinModsList.end())
         {
            int iWhichMod = it->first;
            int iCount = 0;
            bool bFirst = true;

            printf(" - variable_mod%02d: ", iWhichMod);
            while (it != g_staticParams.variableModParameters.mmapProteinModsList.end() && it->first == iWhichMod)
            {
               if (iCount < 3)
               {
                  if (!bFirst)
                     printf(", ");
                  printf("%s", it->second.c_str());
               }
               else if (iCount == 3)
               {
                  printf(", ...");
               }
               it++;
               iCount++;
               bFirst = false;
            }
            printf("\n");
         }
         printf("\n");
      }
      else
         g_staticParams.variableModParameters.bVarModProteinFilter = false;

      return true;
   }
   else
   {
      string strErrorMsg = " Error - cannot read protein variable mod filter file \""
         + g_staticParams.variableModParameters.sProteinLModsListFile + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
}


void CometSearchManager::SetAScoreOptions(AScoreProCpp::AScoreOptions& options)
{
   using namespace AScoreProCpp;

   std::vector<std::string> ionSeriesList;
   unsigned int uiIonSeriesMask = 0;
   bool bSetNeutralLossMask = false;

   // AScorePro set up differential modifications
   std::vector<AScoreProCpp::PeptideMod> diffMods;

   for (int i = 0; i < VMODS; ++i)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0)
         && (g_staticParams.variableModParameters.varModList[i].szVarModChar[0] != '-'))
      {
         AScoreProCpp::PeptideMod pepMod;

         pepMod.setSymbol(i + 1 + '0');
         pepMod.setResidues(g_staticParams.variableModParameters.varModList[i].szVarModChar);
         pepMod.setMass(g_staticParams.variableModParameters.varModList[i].dVarModMass);
         pepMod.setIsNTerm(false);
         pepMod.setIsCTerm(false);

         diffMods.push_back(pepMod);

         if ((g_staticParams.options.iPrintAScoreProScore == -1 || g_staticParams.options.iPrintAScoreProScore - 1 == i)
            && g_staticParams.variableModParameters.varModList[i].dNeutralLoss != 0.0)
         {
            // Set up neutral loss. If iPrintAScoreProScore == -1, will use the last neutral loss.
            // Else neutral loss is from specified mod only.
            AScoreProCpp::NeutralLoss neutralLoss;
            neutralLoss.setMass(-(g_staticParams.variableModParameters.varModList[i].dNeutralLoss));
            neutralLoss.setResidues(g_staticParams.variableModParameters.varModList[i].szVarModChar);
            options.setNeutralLoss(neutralLoss);
            bSetNeutralLossMask = true;
         }

         if (g_staticParams.options.iPrintAScoreProScore - 1 == i)
         {
            // Target modification settings
            options.setSymbol(i + 1 + '0');
            options.setResidues(g_staticParams.variableModParameters.varModList[i].szVarModChar);
         }
      }
   }
   options.setDiffMods(diffMods);

   if (g_staticParams.options.iPrintAScoreProScore == -1)
   {
      options.setSymbol('\0');
      options.setResidues("");
   }

   //    { "nA", 1 }, { "nB", 2 }, { "nY", 4 }, { "a", 8 }, { "b", 16 }, { "c", 32 },
   //    { "d", 64 }, { "v", 128 }, { "w", 256 }, { "x", 512 }, { "y", 1024 }, { "z", 2048 }

   if (g_staticParams.ionInformation.iIonVal[ION_SERIES_A])
   {
      uiIonSeriesMask += 8;
      ionSeriesList.push_back("a");
      if (bSetNeutralLossMask)
      {
         uiIonSeriesMask += 1; // add ammonia loss to A series
         ionSeriesList.push_back("nA");
      }
   }
   if (g_staticParams.ionInformation.iIonVal[ION_SERIES_B])
   {
      uiIonSeriesMask += 16;
      ionSeriesList.push_back("b");
      if (bSetNeutralLossMask)
      {
         uiIonSeriesMask += 2; // add ammonia loss to B series
         ionSeriesList.push_back("nB");
      }
   }
   if (g_staticParams.ionInformation.iIonVal[ION_SERIES_C])
      uiIonSeriesMask += 32;
   if (g_staticParams.ionInformation.iIonVal[ION_SERIES_X])
      uiIonSeriesMask += 512;
   if (g_staticParams.ionInformation.iIonVal[ION_SERIES_Y])
   {
      uiIonSeriesMask += 1024;
      ionSeriesList.push_back("y");
      if (bSetNeutralLossMask)
      {
         uiIonSeriesMask += 4; // add ammonia loss to Y series
         ionSeriesList.push_back("nY");
      }
   }
   // FIX: need to check if AScorePro uses Z or Z' series
   if (g_staticParams.ionInformation.iIonVal[ION_SERIES_Z]
      || g_staticParams.ionInformation.iIonVal[ION_SERIES_Z1])
   {
      uiIonSeriesMask += 2048;
      ionSeriesList.push_back("z");
   }

   options.setIonSeries(uiIonSeriesMask);
   options.setIonSeriesList(ionSeriesList);

   // Peak depth settings
   options.setPeakDepth(0);
   options.setMaxPeakDepth(50);

   // Fragment matching tolerance

   if (g_staticParams.tolerances.dFragmentBinSize <= 0.05)
      options.setTolerance(0.05);
   else
      options.setTolerance(0.3);

   options.setUnits(Mass::Units::DALTON);
   options.setUnitText("Da");

   // Window size for filtering peaks
   options.setWindow(70);

   // Enable low mass cutoff
   options.setLowMassCutoff(true);

   // Filter low intensity peaks
   options.setFilterLowIntensity(0);

   // C-terminal settings
   options.setNoCterm(true);

   // Scoring options
   options.setUseMobScore(true);
   options.setUseDeltaAscore(true);

   // Max peptides and other limits
   options.setMaxPeptides(1000);
   // options.setMaxDiff(5); // From max_diff in JSON

   // Initialize other fields to default values from JSON
   options.setMz(0);
   options.setPeptide("");
   options.setScan(0);

   // Deisotoping type (empty string means no deisotoping)
   // options.setDeisotopingType("");

   // Set up static modifications
   // FIX:  deal with static N-term and C-term mods
   std::vector<PeptideMod> staticMods;
   for (int i = 'A'; i <= 'Z'; ++i)
   {
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[i], 0.0)
         && (char(i) != 'B' && char(i) != 'J' && char(i) != 'O' && char(i) != 'U' && char(i) != 'X' && char(i) != 'Z'))
      {
         PeptideMod pepMod;
         pepMod.setSymbol(char(i + 32));  // use lowercase letter for static mods symbol
         pepMod.setResidues(std::string(1, char(i)));
         pepMod.setMass(g_staticParams.staticModifications.pdStaticMods[i]);
         pepMod.setIsNTerm(false);
         pepMod.setIsCTerm(false);
         staticMods.push_back(pepMod);
      }
   }
   options.setStaticMods(staticMods);

   // Apply static mods to AminoAcidMasses
   AminoAcidMasses& masses = options.getMasses();
   for (const auto& mod : staticMods)
   {
      const std::string& residues = mod.getResidues();
      if (!residues.empty())
      {
         for (char c : residues)
         {
            masses.modifyAminoAcidMass(c, mod.getMass());
         }
      }

      if (mod.getIsNTerm())
      {
         masses.modifyNTermMass(mod.getMass());
      }

      if (mod.getIsCTerm())
      {
         masses.modifyCTermMass(mod.getMass());
      }
   }
}


void CometSearchManager::PrintAScoreOptions(const AScoreProCpp::AScoreOptions& options)
{
   using std::cout;
   using std::endl;

   cout << endl <<  " AScoreOptions values:" << endl;

   // Ion series
   cout << " ionSeriesList: ";
   for (const auto& s : options.getIonSeriesList())
      cout << s << " ";
   cout << endl;
   cout << " ionSeries bitmask (dec): " << options.getIonSeries()
      << " (hex: 0x" << std::hex << options.getIonSeries() << std::dec << ")" << endl;

   // Differential (variable) mods
   cout << " diffMods (" << options.getDiffMods().size() << "): ";
   for (const auto& mod : options.getDiffMods())
      cout << mod.getResidues() << "(" << mod.getMass() << ",sym=" << mod.getSymbol() << ") ";
   cout << endl;

   // Static mods
   cout << " staticMods (" << options.getStaticMods().size() << "): ";
   for (const auto& mod : options.getStaticMods())
      cout << mod.getResidues() << "(" << mod.getMass()
      << (mod.getIsNTerm() ? ",Nterm" : "")
      << (mod.getIsCTerm() ? ",Cterm" : "")
      << ",sym=" << mod.getSymbol() << ") ";
   cout << endl;

   // Neutral loss
   auto nl = options.getNeutralLoss();
   cout << " neutralLoss mass: " << nl.getMass()
      << " residues: " << nl.getResidues()
      << (nl.getMass() == 0.0 ? " (disabled)" : "") << endl;

   // Core numeric / boolean settings
   cout << std::boolalpha;
   cout << " peakDepth: " << options.getPeakDepth() << endl;
   cout << " maxPeakDepth: " << options.getMaxPeakDepth() << endl;
   cout << " tolerance: " << options.getTolerance() << " " << options.getUnitText() << endl;
   cout << " units enum: " << static_cast<int>(options.getUnits()) << endl;
   cout << " window: " << options.getWindow() << endl;
   cout << " lowMassCutoff: " << options.getLowMassCutoff() << endl;
   cout << " filterLowIntensity: " << options.getFilterLowIntensity() << endl;
   cout << " noCterm: " << options.getNoCterm() << endl;
   cout << " useMobScore: " << options.getUseMobScore() << endl;
   cout << " useDeltaAscore: " << options.getUseDeltaAscore() << endl;
   cout << " maxPeptides: " << options.getMaxPeptides() << endl;

   // Target (scoring) modification focus
   char targetSym = options.getSymbol();
   cout << " targetModSymbol: " << (targetSym ? std::string(1, targetSym) : "(none)") << endl;
   cout << " targetModResidues: " << (options.getResidues().empty() ? "(none)" : options.getResidues()) << endl;

   // Summaries
   size_t totalModSymbols = options.getDiffMods().size() + options.getStaticMods().size();
   cout << " totalModsDefined: " << totalModSymbols << endl;

   cout << " Done printing AScoreOptions." << endl << endl;
}
