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


#include "Common.h"
#include "CometMassSpecUtils.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometPreprocess.h"
#include "CometWriteOut.h"
#include "CometWriteSqt.h"
#include "CometWriteTxt.h"
#include "CometWritePepXML.h"
#include "CometWriteMzIdentML.h"
#include "CometWritePercolator.h"
#include "CometDataInternal.h"
#include "CometSearchManager.h"
#include "CometStatus.h"
#include "CometFragmentIndex.h"
#include "CometPeptideIndex.h"
#include "CometSpecLib.h"
#include "CometAlignment.h"
#include "AScoreOptions.h"
#include "AScoreFactory.h"


#include <sstream>

#undef PERF_DEBUG

std::vector<Query*>           g_pvQuery;
std::vector<QueryMS1*>        g_pvQueryMS1;
std::vector<InputFileInfo *>  g_pvInputFiles;
StaticParams                  g_staticParams;
vector<DBIndex>               g_pvDBIndex;
MassRange                     g_massRange;
Mutex                         g_pvQueryMutex;
Mutex                         g_pvDBIndexMutex;
Mutex                         g_preprocessMemoryPoolMutex;
Mutex                         g_searchMemoryPoolMutex;
CometStatus                   g_cometStatus;
string                        g_sCometVersion;

AScoreProCpp::AScoreOptions   g_AScoreOptions;  // AScore options
AScoreProCpp::AScoreDllInterface* g_AScoreInterface;

vector<vector<comet_fileoffset_t>> g_pvProteinsList;
unsigned int** g_iFragmentIndex[FRAGINDEX_MAX_THREADS][FRAGINDEX_PRECURSORBINS];        // stores fragment index; [thread][pepmass][BIN(mass)][which g_vFragmentPeptides entries]
unsigned int* g_iCountFragmentIndex[FRAGINDEX_MAX_THREADS][FRAGINDEX_PRECURSORBINS];      // stores counts of fragment index; [thread][pepmass][BIN(mass)]
bool* g_bIndexPrecursors;                                   // array for BIN(precursors), set to true if precursor present in file
vector<struct FragmentPeptidesStruct> g_vFragmentPeptides;  // each peptide is represented here iWhichPeptide, which mod if any, calculated mass
vector<PlainPeptideIndexStruct> g_vRawPeptides;             // list of unmodified peptides and their proteins as file pointers
vector<vector<unsigned int>> g_vulSpecLibPrecursorIndex;    // mass index for SpecLib
vector<SpecLibStruct> g_vSpecLib;                           // stores the SpecLib

bool g_bPlainPeptideIndexRead = false;
bool g_bPeptideIndexRead = false;
bool g_bSpecLibRead = false;
bool g_bPerformSpecLibSearch = false;
bool g_bPerformDatabaseSearch = false;
FILE* fpfasta;      // file pointer to FASTA; would be same as fpdb if input db was already FASTA but otherwise needed if input is .idx file

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
std::string GetHostName()
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

static InputType GetInputType(const char *pszFileName)
{
   int iLen = (int)strlen(pszFileName);

   if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 6, ".mzXML")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 5, ".mzML")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 9, ".mzXML.gz")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 8, ".mzML.gz"))

   {
      return InputType_MZXML;
   }
   else if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 4, ".raw"))
   {
      return InputType_RAW;
   }
   else if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 4, ".ms2")
         || !STRCMP_IGNORE_CASE(pszFileName + iLen - 5, ".cms2"))
   {
      return InputType_MS2;
   }
   else if (!STRCMP_IGNORE_CASE(pszFileName + iLen - 4, ".mgf"))
   {
      return InputType_MGF;
   }

   return InputType_UNKNOWN;
}

static bool UpdateInputFile(InputFileInfo *pFileInfo)
{
   bool bUpdateBaseName = false;
   char szTmpBaseName[SIZE_FILE];

   // Make sure not set on command line OR more than 1 input file
   // Need to do this check here before g_staticParams.inputFile is set to *pFileInfo
   if (g_staticParams.inputFile.szBaseName[0] =='\0' || g_pvInputFiles.size()>1)
      bUpdateBaseName = true;
   else
      strcpy(szTmpBaseName, g_staticParams.inputFile.szBaseName);

   g_staticParams.inputFile = *pFileInfo;

   g_staticParams.inputFile.iInputType = GetInputType(g_staticParams.inputFile.szFileName);

   if (InputType_UNKNOWN == g_staticParams.inputFile.iInputType)
   {
       return false;
   }
   int iLen = (int)strlen(g_staticParams.inputFile.szFileName);

   // per request, perform quick check to validate file still exists
   // to avoid creating stub output files in these cases.
   FILE *fp;
   if ( (fp=fopen(g_staticParams.inputFile.szFileName, "r"))==NULL)
   {
      string strErrorMsg = " Error - cannot read input file \"" + string(g_staticParams.inputFile.szFileName) + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
   else
   {
      fclose(fp);
   }

#ifndef CRUX
   if (bUpdateBaseName) // set individual basename from input file
   {
      char *pStr;

      strcpy(g_staticParams.inputFile.szBaseName, g_staticParams.inputFile.szFileName);

      if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '.')))
         *pStr = '\0';

      if (!STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 9, ".mzXML.gz")
            || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 8, ".mzML.gz"))
      {
         if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '.')))
            *pStr = '\0';
      }
   }
   else
   {
      strcpy(g_staticParams.inputFile.szBaseName, szTmpBaseName);  // set basename from command line
   }
#endif

   // Create .out directory.
   if (g_staticParams.options.bOutputOutFiles)
   {
#ifdef _WIN32
      if (_mkdir(g_staticParams.inputFile.szBaseName) == -1)
      {
         errno_t err;
         _get_errno(&err);

         if (err != EEXIST)
         {
            string strErrorMsg = " Error - could not create directory\"" + string(g_staticParams.inputFile.szBaseName) + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }
      if (g_staticParams.options.iDecoySearch == 2)
      {
         string sDecoyDir;
         sDecoyDir = g_staticParams.inputFile.szBaseName + string("_decoy");

         if (_mkdir(sDecoyDir.c_str()) == -1)
         {
            errno_t err;
            _get_errno(&err);

            if (err != EEXIST)
            {
               string strErrorMsg = " Error - could not create directory \"" + sDecoyDir + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               return false;
            }
         }
      }
#else
      if ((mkdir(g_staticParams.inputFile.szBaseName, 0775) == -1) && (errno != EEXIST))
      {
         string strErrorMsg = " Error - could not create directory \"" + string(g_staticParams.inputFile.szBaseName) + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }
      if (g_staticParams.options.iDecoySearch == 2)
      {
         string sDecoyDir;
         sDecoyDir = g_staticParams.inputFile.szBaseName + string("_decoy");

         if ((mkdir(sDecoyDir.c_str(), 0775) == -1) && (errno != EEXIST))
         {
            string strErrorMsg = " Error - could not create directory \"" + sDecoyDir + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }
#endif
   }

   return true;
}

static void SetMSLevelFilter(MSReader &mstReader)
{
   vector<MSSpectrumType> msLevel;

   if (g_staticParams.options.iMSLevel == 3)
      msLevel.push_back(MS3);
   else if (g_staticParams.options.iMSLevel == 2)
      msLevel.push_back(MS2);
   else if (g_staticParams.options.iMSLevel == 1)
      msLevel.push_back(MS1);

   mstReader.setFilter(msLevel);
}

// Allocate memory for the _pResults struct for each g_pvQuery entry.
static bool AllocateResultsMem()
{
   for (std::vector<Query*>::iterator it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
   {
      Query* pQuery = *it;

      try
      {
         pQuery->_pResults = new Results[g_staticParams.options.iNumStored];
      }
      catch (std::bad_alloc& ba)
      {
         string strErrorMsg = " Error - new(_pResults[]). bad_alloc: \"" + std::string(ba.what()) + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }

      if (g_staticParams.options.iDecoySearch==2)
      {
         try
         {
            pQuery->_pDecoys = new Results[g_staticParams.options.iNumStored];
         }
         catch (std::bad_alloc& ba)
         {
            string strErrorMsg = " Error - new(_pDecoys[]). bad_alloc: " + std::string(ba.what()) + "\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
      }

      for (int j=0; j<g_staticParams.options.iNumStored; ++j)
      {
         pQuery->_pResults[j].dPepMass = 0.0;
         pQuery->_pResults[j].dExpect = 999;
         pQuery->_pResults[j].fScoreSp = 0.0;
         pQuery->_pResults[j].fXcorr = (float)g_staticParams.options.dMinimumXcorr;
         pQuery->_pResults[j].usiLenPeptide = 0;
         pQuery->_pResults[j].usiRankSp = 0;
         pQuery->_pResults[j].usiMatchedIons = 0;
         pQuery->_pResults[j].usiTotalIons = 0;
         pQuery->_pResults[j].szPeptide[0] = '\0';
         pQuery->_pResults[j].strSingleSearchProtein = "";
         pQuery->_pResults[j].pWhichProtein.clear();
         pQuery->_pResults[j].sPeffOrigResidues.clear();
         pQuery->_pResults[j].iPeffOrigResiduePosition = -9;
         memset(pQuery->iXcorrHistogram, 0, sizeof(pQuery->iXcorrHistogram));

         if (g_staticParams.options.iDecoySearch)
            pQuery->_pResults[j].pWhichDecoyProtein.clear();

         if (g_staticParams.options.iDecoySearch==2)
         {
            pQuery->_pDecoys[j].dPepMass = 0.0;
            pQuery->_pDecoys[j].dExpect = 999;
            pQuery->_pDecoys[j].fScoreSp = 0.0;
            pQuery->_pDecoys[j].fXcorr = (float)g_staticParams.options.dMinimumXcorr;
            pQuery->_pDecoys[j].usiLenPeptide = 0;
            pQuery->_pDecoys[j].usiRankSp = 0;
            pQuery->_pDecoys[j].usiMatchedIons = 0;
            pQuery->_pDecoys[j].usiTotalIons = 0;
            pQuery->_pDecoys[j].szPeptide[0] = '\0';
            pQuery->_pDecoys[j].strSingleSearchProtein = "";
            //pQuery->_pDecoys[j].cPeffOrigResidue = '\0';
            pQuery->_pDecoys[j].sPeffOrigResidues.clear();
            pQuery->_pDecoys[j].iPeffOrigResiduePosition = -9;
         }
      }
   }

   return true;
}

// Allocate memory for the _pSpecLibResults struct for each g_pvQueryMS1 entry.
static bool AllocateResultsMemMS1()
{
/*
   for (std::vector<QueryMS1*>::iterator it = g_pvQueryMS1.begin(); it != g_pvQueryMS1.end(); ++it)
   {
      QueryMS1* pQueryMS1 = *it;

      try
      {
         pQueryMS1->_pSpecLibResultsMS1 = new SpecLibResultsMS1[g_staticParams.options.iNumStored];
      }
      catch (std::bad_alloc& ba)
      {
         string strErrorMsg = " Error - new(_pSpecLibResults[]). bad_alloc: " + std::string(ba.what()) + "\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }
      for (int j=0; j<g_staticParams.options.iNumStored; ++j)
      {
         pQueryMS1->_pSpecLibResultsMS1[j].fXcorr = (float)g_staticParams.options.dMinimumXcorr;
         pQueryMS1->_pSpecLibResultsMS1[j].fCn = 0;
         pQueryMS1->_pSpecLibResultsMS1[j].fRTime = 0;
      }

   }
*/
   return true;
}

static bool compareByPeptideMass(Query const* a, Query const* b)
{
   return (a->_pepMassInfo.dExpPepMass < b->_pepMassInfo.dExpPepMass);
}

static bool compareByMangoIndex(Query const* a, Query const* b)
{
   return (a->dMangoIndex < b->dMangoIndex);
}

static bool compareByScanNumber(Query const* a, Query const* b)
{
   // sort by charge state if same scan number
   if (a->_spectrumInfoInternal.iScanNumber == b->_spectrumInfoInternal.iScanNumber)
      return (a->_spectrumInfoInternal.usiChargeState < b->_spectrumInfoInternal.usiChargeState);
   return (a->_spectrumInfoInternal.iScanNumber < b->_spectrumInfoInternal.iScanNumber);
}

static void CalcRunTime(time_t tStartTime)
{
   time_t tEndTime;
   int iTmp;

   time(&tEndTime);

   int iElapseTime = static_cast<int>(difftime(tEndTime, tStartTime));

   // Use a C++ string for output instead of C char array
   std::string outFileTimeString = std::string(g_staticParams.szDate) + ",";

   if ((iTmp = static_cast<int>(iElapseTime / 3600)) > 0)
      outFileTimeString += " " + std::to_string(iTmp) + " hr.";
   if ((iTmp = static_cast<int>((iElapseTime - static_cast<int>(iElapseTime / 3600) * 3600) / 60)) > 0)
      outFileTimeString += " " + std::to_string(iTmp) + " min.";
   if ((iTmp = static_cast<int>((iElapseTime - (static_cast<int>(iElapseTime / 3600)) * 3600) % 60)) > 0)
      outFileTimeString += " " + std::to_string(iTmp) + " sec.";
   if (iElapseTime == 0)
      outFileTimeString += " 0 sec.";
   outFileTimeString += " on " + g_staticParams.sHostName;

   g_staticParams.iElapseTime = iElapseTime;
   g_staticParams.strOutFileTimeString = outFileTimeString;
}

// for .out files
static void PrintOutfileHeader()
{
   // print parameters

   char szIsotope[32] = "";
   char szPeak[16] = "";

   sprintf(g_staticParams.szIonSeries, "ion series ABCXYZ nl: %d%d%d%d%d%d%d %d",
         g_staticParams.ionInformation.iIonVal[ION_SERIES_A],
         g_staticParams.ionInformation.iIonVal[ION_SERIES_B],
         g_staticParams.ionInformation.iIonVal[ION_SERIES_C],
         g_staticParams.ionInformation.iIonVal[ION_SERIES_X],
         g_staticParams.ionInformation.iIonVal[ION_SERIES_Y],
         g_staticParams.ionInformation.iIonVal[ION_SERIES_Z],
         g_staticParams.ionInformation.iIonVal[ION_SERIES_Z1],
         g_staticParams.ionInformation.bUseWaterAmmoniaLoss);

   char szUnits[8];
   char szDecoy[20];
   char szReadingFrame[20];
   char szRemovePrecursor[20];

   if (g_staticParams.tolerances.iMassToleranceUnits==0)
      strcpy(szUnits, " AMU");
   else if (g_staticParams.tolerances.iMassToleranceUnits==1)
      strcpy(szUnits, " MMU");
   else
      strcpy(szUnits, " PPM");

   if (g_staticParams.options.iDecoySearch)
      sprintf(szDecoy, " DECOY%d", g_staticParams.options.iDecoySearch);
   else
      szDecoy[0]=0;

   if (g_staticParams.options.iRemovePrecursor)
      sprintf(szRemovePrecursor, " REMOVEPREC%d", g_staticParams.options.iRemovePrecursor);
   else
      szRemovePrecursor[0]=0;

   if (g_staticParams.options.iWhichReadingFrame)
      sprintf(szReadingFrame, " FRAME%d", g_staticParams.options.iWhichReadingFrame);
   else
      szReadingFrame[0]=0;

   if (g_staticParams.tolerances.iIsotopeError > 0)
      sprintf(szIsotope, " ISOTOPE%d", g_staticParams.tolerances.iIsotopeError);

   szPeak[0]='\0';
   if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 1)
      strcpy(szPeak, " PEAK1");

   sprintf(g_staticParams.szDisplayLine, "display top %d,%s%s%s%s%s%s%s%s",
         g_staticParams.options.iNumPeptideOutputLines,
         szRemovePrecursor,
         szReadingFrame,
         szPeak,
         szUnits,
         (g_staticParams.tolerances.iMassToleranceType==0?" MH+":" m/z"),
         szIsotope,
         szDecoy,
         (g_staticParams.options.bClipNtermMet?" CLIPMET":"") );
}

static bool ValidateOutputFormat()
{
   if (!g_staticParams.options.bOutputSqtStream
         && !g_staticParams.options.bOutputSqtFile
         && !g_staticParams.options.bOutputTxtFile
         && !g_staticParams.options.bOutputPepXMLFile
         && !g_staticParams.options.iOutputMzIdentMLFile
         && !g_staticParams.options.bOutputPercolatorFile
         && !g_staticParams.options.bOutputOutFiles)
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
   if (!strcmp(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
      sTmpDB = sTmpDB.erase(sTmpDB.size() - 4); // need plain fasta if indexdb input
   if ((fpfasta = fopen(sTmpDB.c_str(), "r")) == NULL)
   {
      string strErrorMsg = " Error (4) - cannot read FASTA sequence database file \"" + sTmpDB + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // if .idx database specified but does not exist, first see if corresponding
   // fasta exists and if it does, create the .idx file
   if (strstr(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
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
            g_staticParams.options.bCreateFragmentIndex = true;  // set to true to make the index
            return true;
         }
      }
      else
      {
         string strFasta = g_staticParams.databaseInfo.szDatabase;
         strFasta.erase(strFasta.length() - 4);  // remove .idx extension

         if ((fpcheck=fopen(strFasta.c_str(), "r")) == NULL)
         {
            string strErrorMsg = " Error - peptide index file \"" + std::string(g_staticParams.databaseInfo.szDatabase)
               + "\" is present but corresponding FASTA file \"" + strFasta + "\" file is missing.\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            return false;
         }
         else
         {
            fclose(fpcheck);
            g_staticParams.options.bCreateFragmentIndex = false;
            return true;
         }
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
         + std::to_string(g_staticParams.options.scanRange.iStart) + " to "
         + std::to_string(g_staticParams.options.scanRange.iEnd)
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
    singleSearchThreadCount(1)
{
   // Initialize the mutexes we'll use to protect global data.
   Threading::CreateMutex(&g_pvQueryMutex);

   // Initialize the mutexes we'll use to protect DBIndex.
   Threading::CreateMutex(&g_pvDBIndexMutex);

   // Initialize the mutex we'll use to protect the preprocess memory pool
   Threading::CreateMutex(&g_preprocessMemoryPoolMutex);

   // Initialize the mutex we'll use to protect the search memory pool
   Threading::CreateMutex(&g_searchMemoryPoolMutex);

   // Initialize the Comet version
   SetParam("# comet_version", comet_version, comet_version);
   _tp = new ThreadPool();
}

CometSearchManager::~CometSearchManager()
{
   // Destroy the mutex we used to protect g_pvQuery.
   Threading::DestroyMutex(g_pvQueryMutex);

   // Destroy the mutex we used to protect g_pvDBIndex.
   Threading::DestroyMutex(g_pvDBIndexMutex);

   // Destroy the mutex we used to protect the preprocess memory pool
   Threading::DestroyMutex(g_preprocessMemoryPoolMutex);

   // Destroy the mutex we used to protect the search memory pool
   Threading::DestroyMutex(g_searchMemoryPoolMutex);

   //std::vector calls destructor of every element it contains when clear() is called
   g_pvInputFiles.clear();

   _mapStaticParams.clear();

   if (_tp != NULL)
      delete _tp;
   _tp = NULL;
}

bool CometSearchManager::InitializeStaticParams()
{
   int iIntData;
   double dDoubleData;
   string strData;
   IntRange intRangeData;
   DoubleRange doubleRangeData;

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

   if (GetParamValue("spectraL_library_ms_level", iIntData))
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

   GetParamValue("peff_format", g_staticParams.peffInfo.iPeffSearch);

   GetParamValue("mass_offsets", g_staticParams.vectorMassOffsets);

   GetParamValue("precursor_NL_ions", g_staticParams.precursorNLIons);

   GetParamValue("old_mods_encoding", g_staticParams.iOldModsEncoding);

   GetParamValue("xcorr_processing_offset", g_staticParams.iXcorrProcessingOffset);

   GetParamValue("nucleotide_reading_frame", g_staticParams.options.iWhichReadingFrame);

   GetParamValue("mass_type_parent", g_staticParams.massUtility.bMonoMassesParent);

   GetParamValue("mass_type_fragment", g_staticParams.massUtility.bMonoMassesFragment);

   if (GetParamValue("show_fragment_ions", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bShowFragmentIons = false;
      else
         g_staticParams.options.bShowFragmentIons = true;
   }

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

   if (GetParamValue("output_outfiles", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bOutputOutFiles = false;
      else
         g_staticParams.options.bOutputOutFiles = true;
   }

   if (GetParamValue("skip_researching", iIntData))
   {
      if (iIntData == 0)
         g_staticParams.options.bSkipAlreadyDone = false;
      else
         g_staticParams.options.bSkipAlreadyDone = true;
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

   if (GetParamValue("add_U_user_amino_acid", dDoubleData))
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
         g_staticParams.massUtility.pdAAMassUser[(int)'X'] = dDoubleData;
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
      if (g_staticParams.options.iNumThreads < 0)
         g_staticParams.options.iNumThreads = iNumCPUCores + g_staticParams.options.iNumThreads;
      else
         g_staticParams.options.iNumThreads = iNumCPUCores;
#else
      iNumCPUCores = sysconf( _SC_NPROCESSORS_ONLN );

      if (g_staticParams.options.iNumThreads < 0)
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
      if (g_staticParams.options.iNumThreads < 0)
      {
         g_staticParams.options.iNumThreads = 4;
         logout(" Setting number of threads to 4");
      }

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
         for (int ii=i+1; ii<VMODS-1; ++ii)
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
         sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "(%s%c %+0.6f) ",
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

   if (g_staticParams.options.iNumPeptideOutputLines < 1)
      g_staticParams.options.iNumPeptideOutputLines = 1;

   // set iNumStored to be at least 1 bigger than iNumPeptideOutputLines for post processing code
   if (g_staticParams.options.iNumStored <= g_staticParams.options.iNumPeptideOutputLines)
      g_staticParams.options.iNumStored = g_staticParams.options.iNumPeptideOutputLines + 1;

   if (!isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "+ct=%0.6f ",
            g_staticParams.staticModifications.dAddCterminusPeptide);
   }

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0))
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "+nt=%0.6f ",
            g_staticParams.staticModifications.dAddNterminusPeptide);
   }

   if (!isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "+ctprot=%0.6f ",
            g_staticParams.staticModifications.dAddCterminusProtein);
   }

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "+ntprot=%0.6f ",
            g_staticParams.staticModifications.dAddNterminusProtein);
   }

   for (int i=65; i<=90; ++i)  // 65-90 represents upper case letters in ASCII
   {
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[i], 0.0))
      {
         sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "%c=%0.6f ", i,
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

      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "Enzyme:%s (%d%s)",
            g_staticParams.enzymeInformation.szSearchEnzymeName,
            g_staticParams.enzymeInformation.iAllowedMissedCleavage,
            szTmp);
   }
   else
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "Enzyme:%s",
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

   if (!g_staticParams.options.bOutputOutFiles)
      g_staticParams.options.bSkipAlreadyDone = 0;

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

   g_massRange.g_uiMaxFragmentArrayIndex = BIN(g_staticParams.options.dFragIndexMaxMass) + 1;
   g_staticParams.options.iFragIndexNumThreads = (g_staticParams.options.iNumThreads > FRAGINDEX_MAX_THREADS ? FRAGINDEX_MAX_THREADS : g_staticParams.options.iNumThreads);

   // At this point, check extension to set whether index database or not
   if (!strcmp(g_staticParams.databaseInfo.szDatabase + strlen(g_staticParams.databaseInfo.szDatabase) - 4, ".idx"))
   {
      // Has .idx extension.  Now parse first line ot see if peptide index or fragment index.
      // either "Comet peptide index" or "Comet fragment ion index"
      char szTmp[512];
      FILE *fp;

      if ( (fp=fopen(g_staticParams.databaseInfo.szDatabase, "r")) == NULL)
      {
         string strErrorMsg = " Error - cannot open database index file \"" + std::string(g_staticParams.databaseInfo.szDatabase) + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }

      if (fgets(szTmp, 512, fp) == NULL) // grab first line of peptide index
      {
         // throw error
      }
      fclose(fp);

      if (!strncmp(szTmp, "Comet peptide index", 19))
      {
         g_staticParams.iIndexDb = 2;  // peptide index
      }
      else if (!strncmp(szTmp, "Comet fragment ion index", 24))
      {
          g_staticParams.iIndexDb = 1;  // fragment ion index

         // if searching fragment index database, limit load of query spectra as no
         // need to load all spectra into memory since querying spectra sequentially
         if (g_staticParams.options.iSpectrumBatchSize > FRAGINDEX_MAX_BATCHSIZE || g_staticParams.options.iSpectrumBatchSize == 0)
            g_staticParams.options.iSpectrumBatchSize = FRAGINDEX_MAX_BATCHSIZE;
      }
      else
      {
         string strErrorMsg = " Error - first line of database index file \""
            + std::string(g_staticParams.databaseInfo.szDatabase) + "\" contains:\n" + std::string(szTmp) + "\n";
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(strErrorMsg);
         return false;
      }
   }

   if (g_staticParams.options.bCreateFragmentIndex && g_staticParams.iIndexDb)
   {
      string strErrorMsg = " Error - input database already indexed: \"" + std::string(g_staticParams.databaseInfo.szDatabase) + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   if (g_staticParams.iIndexDb == 1)
   {
      g_bIndexPrecursors = (bool*)malloc(BIN(g_staticParams.options.dPeptideMassHigh) * sizeof(bool));
      if (g_bIndexPrecursors == NULL)
      {
         printf("\n Error cannot allocate memory for g_bIndexPrecursors(%d)\n", BIN(g_staticParams.options.dPeptideMassHigh));
         return false;
      }
      for (int x = 0; x < BIN(g_staticParams.options.dPeptideMassHigh); ++x)
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
      g_vulSpecLibPrecursorIndex.resize(BINPREC(g_staticParams.options.dPeptideMassHigh));
   }

   if (g_staticParams.tolerances.dInputToleranceMinus > g_staticParams.tolerances.dInputTolerancePlus)
   {
      printf("\n Error: mass_tolerance_lower is greater than mass_tolerance_upper so no peptides will be analyzed.\n");
      return false;
   }

   double dCushion = CometPreprocess::GetMassCushion(g_staticParams.options.dPeptideMassHigh);
   g_staticParams.iArraySizeGlobal = (int)((g_staticParams.options.dPeptideMassHigh + dCushion) * g_staticParams.dInverseBinWidth);

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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
      _mapStaticParams.erase(name);
      _mapStaticParams.insert(std::pair<std::string, CometParam*>(name, pParam));
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
   g_staticParams.options.bCreateFragmentIndex = 1;

   // The DoSearch will create the index and exit
   return DoSearch();
}

bool CometSearchManager::CreatePeptideIndex()
{
   g_cometStatus.ResetStatus();

   // Override the Create Index flag to force it to create
   g_staticParams.options.bCreatePeptideIndex = 1;

   // The DoSearch will create the index and exit
   return DoSearch();
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

#ifdef PERF_DEBUG
   // print set search parameters
   std::map<std::string, CometParam*> mapParams = GetParamsMap();
   for (std::map<std::string, CometParam*>::iterator it = mapParams.begin(); it != mapParams.end(); ++it)
   {
      if (it->first != "[COMET_ENZYME_INFO]")
      {
         printf("OK parameter name=\"%s\" value=\"%s\"\n", it->first.c_str(), it->second->GetStringValue().c_str());
      }
   }
#endif

   // add git hash to version string if present
   // repeated here from Comet main() as main() is skipped when search invoked via DLL
   if (strlen(GITHUBSHA) > 0)
   {
      string sTmp = std::string(GITHUBSHA);
      if (sTmp.size() > 7)
         sTmp.resize(7);
      g_sCometVersion = std::string(comet_version) + " (" + sTmp + ")";
   }
   else
      g_sCometVersion = std::string(comet_version);

   if (!g_staticParams.options.bOutputSqtStream) // && !g_staticParams.iIndexDb)
   {
      strOut = "\n Comet version \"" + g_sCometVersion + "\"\n\n";
      logout(strOut);
      fflush(stdout);
   }

   if (g_staticParams.iIndexDb == 1)
      tp->fillPool(g_staticParams.options.iFragIndexNumThreads);
   else
      tp->fillPool(g_staticParams.options.iNumThreads < 0 ? 0 : g_staticParams.options.iNumThreads - 1);

   if (g_staticParams.options.bCreatePeptideIndex)
   {
      bSucceeded = CometPeptideIndex::WritePeptideIndex(tp);
      return bSucceeded;
   }

   if (g_staticParams.options.bCreateFragmentIndex || !g_staticParams.iIndexDb)
   {
      // If specified, read in the protein variable mod filter file content.
      // Do this here only for classic search or if creating the plain peptide index.
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
   }

   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dH2O);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dNH3);
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   if (g_bPerformDatabaseSearch && g_staticParams.options.bCreateFragmentIndex) //index
   {
       // write out .idx file containing unmodified peptides and protein refs;
       // this calls RunSearch just to query fasta and generate uniq peptide list
       bSucceeded = CometFragmentIndex::WritePlainPeptideIndex(tp);
       if (!bSucceeded)
          return bSucceeded;

       CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

       if (g_pvInputFiles.size() == 0)
          return bSucceeded;
   }

   if (g_staticParams.options.bOutputOutFiles)
      PrintOutfileHeader();

   bool bBlankSearchFile = false;

   if (g_bPerformDatabaseSearch && g_staticParams.iIndexDb == 1)
   {
      if (!g_staticParams.options.iFragIndexSkipReadPrecursors)
      {
         // read precursors before creating fragment index
         auto tTime1 = chrono::steady_clock::now();
         if (!g_staticParams.options.bOutputSqtStream)
         {
            cout << " - read precursors ... ";
            fflush(stdout);
         }

         for (int i = 0; i < (int)g_pvInputFiles.size(); ++i)
         {
            bSucceeded = UpdateInputFile(g_pvInputFiles.at(i));
            if (!bSucceeded)
               break;

            // For file access using MSToolkit.
            MSReader mstReader;

            // We want to read only MS2/MS3 scans.
            SetMSLevelFilter(mstReader);

            CometPreprocess::Reset();

            bSucceeded = CometPreprocess::ReadPrecursors(mstReader);
         }

         if (!g_staticParams.options.bOutputSqtStream)
            cout << CometFragmentIndex::ElapsedTime(tTime1) << endl;
      }
      else
      {
         cout << " - read precursors ... skipping" << endl;
         fflush(stdout);
      }
   }

   if (g_bPerformSpecLibSearch)
   {
      CometSpecLib::LoadSpecLib(g_staticParams.speclibInfo.strSpecLibFile);
   }

   bool bAScoreInitializationNotDoneYet = true;

   for (int i = 0; i < (int)g_pvInputFiles.size(); ++i)
   {
      bSucceeded = UpdateInputFile(g_pvInputFiles.at(i));
      if (!bSucceeded)
         break;

      time_t tStartTime;
      time(&tStartTime);
      strftime(g_staticParams.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));

      if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.iIndexDb)
      {
         strOut = " Search start:  " + string(g_staticParams.szDate) + "\n";
         strOut += " - Input file: " + string(g_staticParams.inputFile.szFileName) + "\n";
         logout(strOut);
         fflush(stdout);
      }

      int iFirstScan = g_staticParams.inputFile.iFirstScan;             // First scan to search specified by user.
      int iLastScan = g_staticParams.inputFile.iLastScan;               // Last scan to search specified by user.
      int iPercentStart = 0;                                            // percentage within input file for start scan of batch
      int iPercentEnd = 0;                                              // percentage within input file for end scan of batch
      int iAnalysisType = g_staticParams.inputFile.iAnalysisType;       // 1=dta (retired),
                                                                        // 2=specific scan,
                                                                        // 3=specific scan + charge,
                                                                        // 4=scan range,
                                                                        // 5=entire file

      // For SQT & pepXML output file, check if they can be written to before doing anything else.
      FILE *fpout_sqt=NULL;
      FILE *fpoutd_sqt=NULL;
      FILE *fpout_pepxml=NULL;
      FILE *fpoutd_pepxml=NULL;
      FILE *fpout_mzidentml=NULL;
      FILE *fpoutd_mzidentml=NULL;
      FILE *fpout_mzidentmltmp=NULL;
      FILE *fpoutd_mzidentmltmp=NULL;
      FILE *fpout_percolator=NULL;
      FILE *fpout_txt=NULL;
      FILE *fpoutd_txt=NULL;

      std::string sOutputSQT;
      std::string sOutputDecoySQT;
      std::string sOutputPepXML;
      std::string sOutputDecoyPepXML;
      std::string sOutputMzIdentML;
      std::string sOutputDecoyMzIdentML;
      std::string sOutputMzIdentMLtmp;         // temporary file used to hold mzIdentML output before finalizing
      std::string sOutputDecoyMzIdentMLtmp;    // temporary file used to hold decoy mzIdentML output before finalizing
      std::string sOutputPercolator;
      std::string sOutputTxt;
      std::string sOutputDecoyTxt;

      if (g_staticParams.options.bOutputSqtFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
            sOutputSQT = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".sqt";

#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
            {
               sOutputSQT = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".target.sqt";
            }
#endif
         }
         else
         {
            sOutputSQT = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".sqt";
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputSQT = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".target.sqt";
#endif
         }

         if ((fpout_sqt = fopen(sOutputSQT.c_str(), "w")) == NULL)
         {
            string strErrorMsg = " Error - cannot write to file \"" + sOutputSQT + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }

         CometWriteSqt::PrintSqtHeader(fpout_sqt, *this);

         if (bSucceeded && (g_staticParams.options.iDecoySearch == 2))
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sOutputDecoySQT = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".decoy.sqt";
            else
               sOutputDecoySQT = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".decoy.sqt";

            if ((fpoutd_sqt = fopen(sOutputDecoySQT.c_str(), "w")) == NULL)
            {
               string strErrorMsg = " Error - cannot write to decoy file \"" + sOutputDecoySQT + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }

            CometWriteSqt::PrintSqtHeader(fpoutd_sqt, *this);
         }
      }

      if (bSucceeded && g_staticParams.options.bOutputTxtFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
            sOutputTxt = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + "." + g_staticParams.szTxtFileExt;
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputTxt = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".target." + g_staticParams.szTxtFileExt;
#endif
         }
         else
         {
            sOutputTxt = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + "." + g_staticParams.szTxtFileExt;
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputTxt = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".target." + g_staticParams.szTxtFileExt;
#endif
         }

         if ((fpout_txt = fopen(sOutputTxt.c_str(), "w")) == NULL)
         {
            string strErrorMsg = " Error - cannot write to file \"" + sOutputTxt + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }

         CometWriteTxt::PrintTxtHeader(fpout_txt);
         fflush(fpout_txt);

         if (bSucceeded && (g_staticParams.options.iDecoySearch == 2))
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sOutputDecoyTxt = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".decoy." + g_staticParams.szTxtFileExt;
            else
               sOutputDecoyTxt = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".decoy." + g_staticParams.szTxtFileExt;

            fpoutd_txt = fopen(sOutputDecoyTxt.c_str(), "w");
            if (!fpoutd_txt)
            {
               string strErrorMsg = " Error - cannot write to decoy file \"" + sOutputDecoyTxt + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }

            CometWriteTxt::PrintTxtHeader(fpoutd_txt);
         }
      }

      if (bSucceeded && g_staticParams.options.bOutputPepXMLFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
            sOutputPepXML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".pep.xml";
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputPepXML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".target.pep.xml";
#endif
         }
         else
         {
            sOutputPepXML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".pep.xml";
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputPepXML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".target.pep.xml";
#endif
         }

         fpout_pepxml = fopen(sOutputPepXML.c_str(), "w");
         if (!fpout_pepxml)
         {
            string strErrorMsg = " Error - cannot write to file \"" + sOutputPepXML + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }

         if (bSucceeded)
            bSucceeded = CometWritePepXML::WritePepXMLHeader(fpout_pepxml, *this);

         if (bSucceeded && (g_staticParams.options.iDecoySearch == 2))
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sOutputDecoyPepXML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".decoy.pep.xml";
            else
               sOutputDecoyPepXML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".decoy.pep.xml";

            fpoutd_pepxml = fopen(sOutputDecoyPepXML.c_str(), "w");
            if (!fpoutd_pepxml)
            {
               string strErrorMsg = " Error - cannot write to decoy file \"" + sOutputDecoyPepXML + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }

            if (bSucceeded)
               bSucceeded = CometWritePepXML::WritePepXMLHeader(fpoutd_pepxml, *this);
         }
      }

      if (bSucceeded && g_staticParams.options.iOutputMzIdentMLFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
            sOutputMzIdentML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".mzid";
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputMzIdentML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".target.mzid";
#endif
         }
         else
         {
            sOutputMzIdentML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".mzid";
#ifdef CRUX
            if (g_staticParams.options.iDecoySearch == 2)
               sOutputMzIdentML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".target.mzid";
#endif
         }

         fpout_mzidentml = fopen(sOutputMzIdentML.c_str(), "w");
         if (!fpout_mzidentml)
         {
            string strErrorMsg = " Error - cannot write to file \"" + sOutputMzIdentML + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }

         sOutputMzIdentMLtmp = sOutputMzIdentML + ".XXXXXX";
#ifdef _WIN32
         errno_t err = _mktemp_s(&sOutputMzIdentMLtmp[0], sOutputMzIdentMLtmp.size() + 1);
         if (err != 0)
         {
            string strErrorMsg = " Error - cannot create temporary file \"" + sOutputMzIdentMLtmp + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }
#else
         int iRet = mkstemp(&sOutputMzIdentMLtmp[0]);
         if (iRet == -1)
         {
            string strErrorMsg = " Error - cannot create temporary file \"" + sOutputMzIdentMLtmp + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }
#endif

         fpout_mzidentmltmp = fopen(sOutputMzIdentMLtmp.c_str(), "w");
         if (!fpout_mzidentmltmp)
         {
            string strErrorMsg = " Error - cannot write to file \"" + sOutputMzIdentMLtmp + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }

         if (bSucceeded && (g_staticParams.options.iDecoySearch == 2))
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sOutputDecoyMzIdentML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".decoy.mzid";
            else
               sOutputDecoyMzIdentML = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
               "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".decoy.mzid";

            fpoutd_mzidentml = fopen(sOutputDecoyMzIdentML.c_str(), "w");
            if (!fpoutd_mzidentml)
            {
               string strErrorMsg = " Error - cannot write to decoy file \"" + sOutputDecoyMzIdentML + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }

            sOutputDecoyMzIdentMLtmp = sOutputDecoyMzIdentML + ".XXXXXX";
#ifdef _WIN32
            errno_t err = _mktemp_s(&sOutputDecoyMzIdentMLtmp[0], sOutputDecoyMzIdentMLtmp.size() + 1);
            if (err != 0)
            {
               string strErrorMsg = " Error - cannot create temporary file \"" + sOutputDecoyMzIdentMLtmp + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }
#else
            int iRet = mkstemp(&sOutputDecoyMzIdentMLtmp[0]);
            if (iRet == -1)
            {
               string strErrorMsg = " Error - cannot create temporary file \"" + sOutputDecoyMzIdentMLtmp + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }
#endif
            fpoutd_mzidentmltmp = fopen(sOutputDecoyMzIdentMLtmp.c_str(), "w");
            if (!fpoutd_mzidentmltmp)
            {
               string strErrorMsg = " Error - cannot write to decoy file \"" + sOutputDecoyMzIdentMLtmp + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               bSucceeded = false;
            }
         }
      }

      if (bSucceeded && g_staticParams.options.bOutputPercolatorFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
            sOutputPercolator = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix + ".pin";
         else
            sOutputPercolator = std::string(g_staticParams.inputFile.szBaseName) + g_staticParams.szOutputSuffix +
            "." + std::to_string(iFirstScan) + "-" + std::to_string(iLastScan) + ".pin";

         fpout_percolator = fopen(sOutputPercolator.c_str(), "w");
         if (!fpout_percolator)
         {
            string strErrorMsg = " Error - cannot write to file \"" + sOutputPercolator + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
            logerr(strErrorMsg);
            bSucceeded = false;
         }

         if (bSucceeded)
            CometWritePercolator::WritePercolatorHeader(fpout_percolator);
      }

      int iTotalSpectraSearched = 0;
      if (bSucceeded)
      {
         //MH: Allocate memory shared by threads during spectral processing.
         bSucceeded = CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads);
         if (!bSucceeded)
            break;

         // Allocate memory shared by threads during search
         bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
         if (!bSucceeded)
            break;

         // For file access using MSToolkit.
         MSReader mstReader;

         // We want to read only MS2/MS3 scans.
         SetMSLevelFilter(mstReader);

         // We need to reset some of the static variables in-between input files
         CometPreprocess::Reset();

         FILE *fpdb;  // need FASTA file again to grab headers for output (currently just store file positions)

         if (g_bPerformDatabaseSearch)
         {
            string sTmpDB = g_staticParams.databaseInfo.szDatabase;
            if (g_staticParams.iIndexDb > 0)
               sTmpDB = sTmpDB.erase(sTmpDB.size() - 4); // need plain fasta if indexdb input
            if ((fpdb = fopen(sTmpDB.c_str(), "r")) == NULL)
            {
               string strErrorMsg = " Error (1) - cannot read FASTA sequence database file \"" + sTmpDB + "\".\n";
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(strErrorMsg);
               return false;
            }
         }

         if (g_staticParams.options.iSpectrumBatchSize == 0 && !g_staticParams.iIndexDb)
         {
            logout("   - Reading all spectra into memory; set \"spectrum_batch_size\" if search terminates here.\n");
            fflush(stdout);
         }

         CometFragmentIndex sqSearch;

         if (g_bPerformDatabaseSearch && g_staticParams.iIndexDb == 1)
         {
            if (!g_bPlainPeptideIndexRead)
            {
               auto tStartTime = chrono::steady_clock::now();
               if (!g_staticParams.options.bOutputSqtStream)
               {
                  cout <<  " - read .idx ... ";
                  fflush(stdout);
               }

               sqSearch.ReadPlainPeptideIndex();

               if (!g_staticParams.options.bOutputSqtStream)
               {
                  cout << CometFragmentIndex::ElapsedTime(tStartTime) << endl;
               }

               sqSearch.CreateFragmentIndex(tp);
            }
         }

         if (g_staticParams.options.iPrintAScoreProScore && bAScoreInitializationNotDoneYet)
         {
            //FIX make sure this is done only once~!!!
            SetAScoreOptions();

            // Create the AScoreDllInterface using the factory function
            g_AScoreInterface = CreateAScoreDllInterface();
            if (!g_AScoreInterface)
            {
               std::cerr << "Failed to create AScore interface." << std::endl;
               exit(1);
            }

            bAScoreInitializationNotDoneYet = false;
         }

         auto tBeginTime = chrono::steady_clock::now();
         if (g_staticParams.iIndexDb)
         {
            printf(" - searching \"%s\" ... ", g_staticParams.inputFile.szBaseName);
            fflush(stdout);
         }

         int iBatchNum = 0;
         while (!CometPreprocess::DoneProcessingAllSpectra()) // Loop through iMaxSpectraPerSearch
         {
            iBatchNum++;
#ifdef PERF_DEBUG
            time_t tTotalSearchStartTime;
            time_t tTotalSearchEndTime;
            time_t tLoadAndPreprocessSpectraStartTime;
            time_t tLoadAndPreprocessSpectraEndTime;
            time_t tRunSearchStartTime;
            time_t tRunSearchEndTime;
            time_t tPostAnalysisStartTime;
            time_t tPostAnalysisEndTime;

            char szTimeBuffer[32];
            szTimeBuffer[0] = '\0';
#endif
            // Load and preprocess all the spectra.
            if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.iIndexDb)
            {
               logout("   - Load spectra:");

#ifdef PERF_DEBUG
               time(&tLoadAndPreprocessSpectraStartTime);
               strftime(szTimeBuffer, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tLoadAndPreprocessSpectraStartTime));
               strOut = "\n >> Start LoadAndPreprocessSpectra:  " + string(szTimeBuffer) + "\n";
               logout(strOut);
#endif

               fflush(stdout);
            }

            g_cometStatus.SetStatusMsg(string("Loading and processing input spectra"));

            // IMPORTANT: From this point onwards, because we've loaded some
            // spectra, we MUST "goto cleanup_results" before exiting the loop,
            // or we will create a memory leak!

            bSucceeded = CometPreprocess::LoadAndPreprocessSpectra(mstReader, iFirstScan, iLastScan, iAnalysisType, tp);

            if (!bSucceeded)
               goto cleanup_results;

            iPercentStart = iPercentEnd;
            iPercentEnd = mstReader.getPercent();

#ifdef PERF_DEBUG
            if (!g_staticParams.options.bOutputSqtStream)
            {
               time(&tLoadAndPreprocessSpectraEndTime);
               strftime(szTimeBuffer, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tLoadAndPreprocessSpectraEndTime));
               strOut = "\n >> End LoadAndPreprocessSpectra:  " + string(szTimeBuffer) + string("\n");
               logout(strOut);
               int iElapsedTime = (int)difftime(tLoadAndPreprocessSpectraEndTime, tLoadAndPreprocessSpectraStartTime);
               strOut = "\n >> Time spent in LoadAndPreprocessSpectra:  " + iElapsedTime + string(" seconds\n");
               logout(strOut);
               fflush(stdout);
            }
#endif

            if (g_pvQuery.empty())
               continue;    //FIX make sure continue instead of break makes sense
            else            // possible no spectrum in batch passes filters; do not want to break in that case;
               iTotalSpectraSearched += (int)g_pvQuery.size();

            bSucceeded = AllocateResultsMem();

            if (!bSucceeded)
               goto cleanup_results;

            { // need strStatusMsg in it's own scope due to goto statement above
               string strStatusMsg = " " + std::to_string(g_pvQuery.size()) + string("\n");
               if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.iIndexDb)
               {
                  logout(strStatusMsg);
               }
               g_cometStatus.SetStatusMsg(strStatusMsg);
            }

            if (g_staticParams.options.bMango)
            {
               int iCurrentScanNumber = 0;       // used to track multiple Mango precursors from same scan number
               int iMangoIndex=0;

               // sort back to original spectrum order in MS2 scan in order to associate pairs
               // based on sequential order of precursors for each scan
               std::sort(g_pvQuery.begin(), g_pvQuery.end(), compareByMangoIndex);

               for (std::vector<Query*>::iterator it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
               {
                  if ((*it)->_spectrumInfoInternal.iScanNumber != iCurrentScanNumber)
                  {
                     iCurrentScanNumber = (*it)->_spectrumInfoInternal.iScanNumber;
                     iMangoIndex = 0;
                  }
                  else
                     iMangoIndex++;

                  sprintf((*it)->_spectrumInfoInternal.szMango, "%03d_%c", (int)iMangoIndex/2, (iMangoIndex % 2)?'B':'A');
               }
            }

            // Sort g_pvQuery vector by dExpPepMass.
            std::sort(g_pvQuery.begin(), g_pvQuery.end(), compareByPeptideMass);

            g_massRange.dMinMass = g_pvQuery.at(0)->_pepMassInfo.dPeptideMassToleranceMinus;
            g_massRange.dMaxMass = g_pvQuery.at(g_pvQuery.size()-1)->_pepMassInfo.dPeptideMassTolerancePlus;

            if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
               g_massRange.bNarrowMassRange = true;
            else
               g_massRange.bNarrowMassRange = false;

#ifdef PERF_DEBUG
            if (!g_staticParams.options.bOutputSqtStream)
            {
               time(&tRunSearchStartTime);
               strftime(szTimeBuffer, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tRunSearchStartTime));
               strOut = "\n >> Start RunSearch:  " + string(szTimeBuffer) + string("\n");
               logout(strOut);
               fflush(stdout);
            }
#endif

            bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
            if (!bSucceeded)
               goto cleanup_results;

            g_cometStatus.SetStatusMsg(string("Running search..."));

            // Now that spectra are loaded to memory and sorted, do search.
            if (g_bPerformDatabaseSearch)
               bSucceeded = CometSearch::RunSearch(iPercentStart, iPercentEnd, tp);
            if (g_bPerformSpecLibSearch)
               bSucceeded = CometSearch::RunSpecLibSearch(iPercentStart, iPercentEnd, tp);

            if (!bSucceeded)
               goto cleanup_results;

#ifdef PERF_DEBUG
            if (!g_staticParams.options.bOutputSqtStream)
            {
               time(&tRunSearchEndTime);
               strftime(szTimeBuffer, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tRunSearchEndTime));
               strOut = "\n >> End RunSearch:  " + string(szTimeBuffer) + string("\n");
               logout(strOut);

               int iElapsedTime=(int)difftime(tRunSearchEndTime, tRunSearchStartTime);
               strOut = "\n >> Time spent in RunSearch:  " + std::to_string(iElapsedTime) + string("seconds \n");
               logout(strOut);

               time(&tPostAnalysisStartTime);
               strftime(szTimeBuffer, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tPostAnalysisStartTime));
               strOut = "\n >> Start PostAnalysis:  " + string(szTimeBuffer) + string("\n");
               logout(strOut);

               fflush(stdout);
            }
#endif

            bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
            if (!bSucceeded)
               goto cleanup_results;

            if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.iIndexDb)
            {
               logout("     - Post analysis:");
               fflush(stdout);
            }

            if (g_bPerformDatabaseSearch)
            {
               g_cometStatus.SetStatusMsg(string("Performing post-search analysis ..."));

               // Sort each entry by xcorr, calculate E-values, etc.
               bSucceeded = CometPostAnalysis::PostAnalysis(tp);
            }

            if (!bSucceeded)
               goto cleanup_results;

#ifdef PERF_DEBUG
            if (g_bPerformDatabaseSearch && !g_staticParams.options.bOutputSqtStream)
            {
               time(&tPostAnalysisEndTime);
               strftime(szTimeBuffer, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tPostAnalysisEndTime));
               strOut = "\n >> End PostAnalysis:  " + string(szTimeBuffer) + string("\n");
               logout(strOut);

               int iElapsedTime=(int)difftime(tPostAnalysisEndTime, tPostAnalysisStartTime);
               strOut = "\n >> Time spent in PostAnalysis:  " + std::to_string(iElapsedTime) + string("seconds \n");
               logout(strOut);
               fflush(stdout);
            }
#endif
            // Sort g_pvQuery vector by scan.
            std::sort(g_pvQuery.begin(), g_pvQuery.end(), compareByScanNumber);

            // Get flanking amino acid residues
            if (g_bPerformDatabaseSearch && g_staticParams.iIndexDb)
            {
               for (int iWhichQuery = 0; iWhichQuery < (int)g_pvQuery.size(); ++iWhichQuery)
               {
                  int iNumPrintLines;
                  int iPrintTargetDecoy = 0;   // will set to 1 or 2 when index db supports internal decoys

                  iNumPrintLines = g_pvQuery.at(iWhichQuery)->iMatchPeptideCount;
                  if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
                     iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

                  for (int iWhichResult = 0; iWhichResult < iNumPrintLines; ++iWhichResult)
                  {
                     if (g_pvQuery.at(iWhichQuery)->_pResults[iWhichResult].usiLenPeptide > 0 && g_pvQuery.at(iWhichQuery)->_pResults[iWhichResult].fXcorr > g_staticParams.options.dMinimumXcorr)
                     {
                        int iNtermMod = g_pvQuery.at(iWhichQuery)->_pResults[iWhichResult].piVarModSites[0];
                        int iCtermMod = g_pvQuery.at(iWhichQuery)->_pResults[iWhichResult].piVarModSites[g_pvQuery.at(iWhichQuery)->_pResults[iWhichResult].usiLenPeptide - 1];

                        if (!(iNtermMod <0 || iNtermMod > FRAGINDEX_VMODS) && !(iCtermMod < 0 || iCtermMod > FRAGINDEX_VMODS))
                        {
                           if (iNtermMod > 0
                              && g_staticParams.variableModParameters.varModList[iNtermMod - 1].iVarModTermDistance == 0
                              && g_staticParams.variableModParameters.varModList[iNtermMod - 1].iWhichTerm == 0)
                           {
                              // only match to peptides at the N-terminus of proteins as protein terminal mod applied
                              CometMassSpecUtils::GetPrevNextAA(fpfasta, iWhichQuery, iWhichResult, iPrintTargetDecoy, 1);
                           }
                           else if (iCtermMod > 0
                              && g_staticParams.variableModParameters.varModList[iCtermMod - 1].iVarModTermDistance == 0
                              && g_staticParams.variableModParameters.varModList[iCtermMod - 1].iWhichTerm == 1)
                           {
                              // only match to peptides at the C-terminus of proteins as protein terminal mod applied
                              CometMassSpecUtils::GetPrevNextAA(fpfasta, iWhichQuery, iWhichResult, iPrintTargetDecoy, 2);
                           }
                           else
                           {
                              // peptide can be anywhere in sequence
                              CometMassSpecUtils::GetPrevNextAA(fpfasta, iWhichQuery, iWhichResult, iPrintTargetDecoy, 0);
                           }
                        }
                     }
                  }
               }
            }

            if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.iIndexDb)
            {
               logout("  done\n");
               fflush(stdout);
            }

            if (g_staticParams.options.bOutputOutFiles)
            {
               CalcRunTime(tStartTime);

               bSucceeded = CometWriteOut::WriteOut(fpdb);
               if (!bSucceeded)
                  goto cleanup_results;
            }

            if (g_staticParams.options.bOutputPepXMLFile)
               CometWritePepXML::WritePepXML(fpout_pepxml, fpoutd_pepxml, fpdb, iTotalSpectraSearched - (int)g_pvQuery.size());

            // For mzid output, dump psms as tab-delimited text first then collate results to
            // mzid file at very end due to requirements of this format.
            if (g_staticParams.options.iOutputMzIdentMLFile)
               CometWriteMzIdentML::WriteMzIdentMLTmp(fpout_mzidentmltmp, fpoutd_mzidentmltmp, iBatchNum);

            if (g_staticParams.options.bOutputPercolatorFile)
            {
               bSucceeded = CometWritePercolator::WritePercolator(fpout_percolator, fpdb);
               if (!bSucceeded)
                  goto cleanup_results;
            }

            if (g_staticParams.options.bOutputTxtFile)
            {
               CometWriteTxt::WriteTxt(fpout_txt, fpoutd_txt, fpdb);
            }

            // Write SQT last as I destroy the g_staticParams.szMod string during that process
            if (g_staticParams.options.bOutputSqtStream || g_staticParams.options.bOutputSqtFile)
               CometWriteSqt::WriteSqt(fpout_sqt, fpoutd_sqt, fpdb);

cleanup_results:

            // Deleting each Query object in the vector calls its destructor, which
            // frees the spectral memory (see definition for Query in CometData.h).
            for (auto it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
               delete (*it);

            g_pvQuery.clear();

            if (!bSucceeded)
               break;
         }

         if (g_bPerformDatabaseSearch && g_staticParams.iIndexDb)
         {
            const auto duration = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - tBeginTime);
            double dTimePerSpectra = (double)duration.count() / (double)iTotalSpectraSearched;
            cout << CometFragmentIndex::ElapsedTime(tBeginTime) << " (" << std::setprecision(3) << dTimePerSpectra << " ms per spectra)" << endl;
         }

         if (bSucceeded)
         {
            if (iTotalSpectraSearched == 0)
               logout(" Warning - no spectra searched.\n");

            if (NULL != fpout_pepxml)
               CometWritePepXML::WritePepXMLEndTags(fpout_pepxml);

            if (NULL != fpoutd_pepxml)
               CometWritePepXML::WritePepXMLEndTags(fpoutd_pepxml);

            if (NULL != fpout_mzidentml)
            {
               fclose(fpout_mzidentmltmp); // close for writing and re-open for reading

               if ((fpout_mzidentmltmp = fopen(sOutputMzIdentMLtmp.c_str(), "r")) == NULL)
               {
                  string strErrorMsg = " Error - cannot read temporary file \"" + sOutputMzIdentMLtmp + "\".\n";
                  g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                  logerr(strErrorMsg);
                  bSucceeded = false;
               }

               // now read tmp file and write mzIdentML
               CometWriteMzIdentML::WriteMzIdentML(fpout_mzidentml, fpdb, sOutputMzIdentMLtmp.c_str(), *this);

               fclose(fpout_mzidentmltmp);
               remove(sOutputMzIdentMLtmp.c_str());
            }

            if (NULL != fpoutd_mzidentml)
            {
               fclose(fpoutd_mzidentmltmp); // close for writing and re-open for reading

               if ((fpoutd_mzidentmltmp = fopen(sOutputDecoyMzIdentMLtmp.c_str(), "r")) == NULL)
               {
                  string strErrorMsg = " Error - cannot read temporary file \"" + sOutputDecoyMzIdentMLtmp + "\".\n";
                  g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                  logerr(strErrorMsg);
                  bSucceeded = false;
               }

               // now read tmp file and write mzIdentML
               CometWriteMzIdentML::WriteMzIdentML(fpoutd_mzidentml, fpdb, sOutputDecoyMzIdentMLtmp.c_str(), *this);

               fclose(fpoutd_mzidentmltmp);
               remove(sOutputDecoyMzIdentMLtmp.c_str());
            }

            if (!g_staticParams.options.bOutputSqtStream && !g_staticParams.iIndexDb)
            {
               time_t tEndTime;

               time(&tEndTime);
               int iElapsedTime = (int)difftime(tEndTime, tStartTime);

               strftime(g_staticParams.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tEndTime));
               strOut = " Search end:    " + string(g_staticParams.szDate);

               int hours, mins, secs;

               hours = (int)(iElapsedTime/3600);
               mins = (int)(iElapsedTime/60) - (hours*60);
               secs = (int)(iElapsedTime%60);

               if (hours)
                  strOut += ", " + std::to_string(hours) + "h:" + std::to_string(mins) + "m:" + std::to_string(secs) + "s\n\n";
               else
                  strOut += ", " + std::to_string(mins) + "m:" + std::to_string(secs) + "s\n\n";

               logout(strOut);
            }
         }

         fclose(fpdb);
      }

      // Clean up the input files vector
//    g_staticParams.vectorMassOffsets.clear();
//    g_staticParams.precursorNLIons.clear();

      //MH: Deallocate spectral processing memory.
      CometPreprocess::DeallocateMemory(g_staticParams.options.iNumThreads);

      // Deallocate search memory
      CometSearch::DeallocateMemory(g_staticParams.options.iNumThreads);

      if (NULL != fpout_pepxml)
      {
         fclose(fpout_pepxml);
         fpout_pepxml = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputPepXML.c_str());
      }

      if (NULL != fpoutd_pepxml)
      {
         fclose(fpoutd_pepxml);
         fpoutd_pepxml = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputDecoyPepXML.c_str());
      }

      if (NULL != fpout_mzidentml)
      {
         fclose(fpout_mzidentml);
         fpout_mzidentml = NULL;
         if (iTotalSpectraSearched == 0)
         {
            remove(sOutputMzIdentML.c_str());
            remove(sOutputMzIdentMLtmp.c_str());
         }
      }

      if (NULL != fpoutd_mzidentml)
      {
         fclose(fpoutd_mzidentml);
         fpoutd_mzidentml = NULL;
         if (iTotalSpectraSearched == 0)
         {
            remove(sOutputDecoyMzIdentML.c_str());
            remove(sOutputDecoyMzIdentMLtmp.c_str());
         }
      }

      if (NULL != fpout_percolator)
      {
         fclose(fpout_percolator);
         fpout_percolator = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputPercolator.c_str());
      }

      if (NULL != fpout_sqt)
      {
         fclose(fpout_sqt);
         fpout_sqt = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputSQT.c_str());
      }

      if (NULL != fpoutd_sqt)
      {
         fclose(fpoutd_sqt);
         fpoutd_sqt = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputDecoySQT.c_str());
      }

      if (NULL != fpoutd_sqt)
      {
         fclose(fpoutd_sqt);
         fpoutd_sqt = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputDecoySQT.c_str());
      }

      if (NULL != fpout_txt)
      {
         fclose(fpout_txt);
         fpout_txt = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputTxt.c_str());
      }

      if (NULL != fpoutd_txt)
      {
         fclose(fpoutd_txt);
         fpoutd_txt = NULL;
         if (iTotalSpectraSearched == 0)
            remove(sOutputDecoyTxt.c_str());
      }

      if (iTotalSpectraSearched == 0)
         bBlankSearchFile = true;

      g_staticParams.inputFile.szBaseName[0] = '\0';

      if (!bSucceeded)
         break;
   }

   if (g_staticParams.iIndexDb == 1) // clean fragment ion index
   {
      int iNumIndexingThreads = g_staticParams.options.iNumThreads;
      if (iNumIndexingThreads > FRAGINDEX_MAX_THREADS)
         iNumIndexingThreads = FRAGINDEX_MAX_THREADS;

      free(g_bIndexPrecursors);       // allocated in InitializeStaticParams

      for (int iWhichThread = 0; iWhichThread < iNumIndexingThreads; ++iWhichThread)
      {
         for (int iPrecursorBin = 0; iPrecursorBin < FRAGINDEX_PRECURSORBINS; ++iPrecursorBin)
         {
            for (unsigned int iMass = 0; iMass < g_massRange.g_uiMaxFragmentArrayIndex; ++iMass)
            {
               if (g_iFragmentIndex[iWhichThread][iPrecursorBin][iMass] != NULL)
               {
                  delete [] g_iFragmentIndex[iWhichThread][iPrecursorBin][iMass];
               }
            }
            delete[] g_iFragmentIndex[iWhichThread][iPrecursorBin];
            delete[] g_iCountFragmentIndex[iWhichThread][iPrecursorBin];
         }
      }

      printf(" - done.\n\n");
   }
   else if (g_staticParams.iIndexDb == 2)
   {
      printf(" - done.\n\n");
   }

   if (g_staticParams.options.iPrintAScoreProScore)
      DeleteAScoreDllInterface(g_AScoreInterface);

   if (bBlankSearchFile)
      return false;
   else
      return bSucceeded;
}


bool CometSearchManager::InitializeSingleSpectrumSearch()
{

   // Skip doing if already completed successfully.
   if (singleSearchInitializationComplete)
      return true;

   if (!InitializeStaticParams())
      return false;

   if (!ValidateSequenceDatabaseFile())
      return false;

   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dH2O);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dNH3);
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   bool bSucceeded;
   //MH: Allocate memory shared by threads during spectral processing.
   bSucceeded = CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
      return bSucceeded;

   // Allocate memory shared by threads during search
   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
      return bSucceeded;

   ThreadPool* tp = _tp;
   tp->fillPool(g_staticParams.options.iNumThreads); // g_staticParams.options.iNumThreads < 0 ? 0 : g_staticParams.options.iNumThreads - 1);  JKE FIX

   // Load databases
   CometFragmentIndex sqSearch;
   if (g_staticParams.iIndexDb == 1 && !g_bPlainPeptideIndexRead)
   {
      sqSearch.ReadPlainPeptideIndex();
      sqSearch.CreateFragmentIndex(tp);

      if (g_staticParams.options.iPrintAScoreProScore)
      {
         SetAScoreOptions();  // normally set at end of InitializeStaticParams; must do here again after ReadPlainPeptideIndex for single spectrum search

         // Create the AScoreDllInterface using the factory function

         g_AScoreInterface = CreateAScoreDllInterface();
         if (!g_AScoreInterface)
         {
            std::cerr << "Failed to create AScore interface." << std::endl;
            exit(1);
         }
      }
   }
   singleSearchInitializationComplete = true;

   return true;
}


void CometSearchManager::FinalizeSingleSpectrumSearch()
{
   if (singleSearchInitializationComplete)
   {
      // Deallocate search memory
      CometSearch::DeallocateMemory(singleSearchThreadCount);

      fclose(fpfasta);

      if (g_staticParams.options.iPrintAScoreProScore)
         DeleteAScoreDllInterface(g_AScoreInterface);

      singleSearchInitializationComplete = false;
   }
}


bool CometSearchManager::InitializeSingleSpectrumMS1Search()
{
   // Skip doing if already completed successfully.
   if (singleSearchMS1InitializationComplete)
      return true;

   if (!InitializeStaticParams())
      return false;

   if (!ValidateSpecLibFile())
      return false;

   // these two mean nothing for MS1
   g_massRange.dMinMass = g_staticParams.options.dPeptideMassLow;
   g_massRange.dMaxMass = g_staticParams.options.dPeptideMassHigh;

   // Set in InitializeStaticParams for peptide search; must set after for MS1
   g_staticParams.iArraySizeGlobal = (int)((g_staticParams.options.dMS1MaxMass + 2.0) / g_staticParams.tolerances.dMS1BinSize);

   bool bSucceeded;
   //MH: Allocate memory shared by threads during spectral processing.
   bSucceeded = CometPreprocess::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
      return bSucceeded;

   // Allocate memory shared by threads during search
   bSucceeded = CometSearch::AllocateMemory(g_staticParams.options.iNumThreads);
   if (!bSucceeded)
      return bSucceeded;

   pMS1Aligner.reset();

   ThreadPool* tp = _tp;
   tp->fillPool(g_staticParams.options.iNumThreads); // g_staticParams.options.iNumThreads < 0 ? 0 : g_staticParams.options.iNumThreads - 1);

   singleSearchMS1InitializationComplete = true;

   return true;
}


void CometSearchManager::FinalizeSingleSpectrumMS1Search()
{
   if (singleSearchMS1InitializationComplete)
   {
      // Deallocate search memory
      CometSearch::DeallocateMemory(singleSearchThreadCount);

      fclose(fpfasta);

      singleSearchMS1InitializationComplete = false;
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
                                                            vector<Scores>& scores)
{
   if (iNumPeaks == 0)
      return false;

   if (dMZ * iPrecursorCharge - (iPrecursorCharge - 1.0) * PROTON_MASS > g_staticParams.options.dPeptideMassHigh)
      return false;    // this assumes dPeptideMassHigh is set correctly in the calling program

   if (!InitializeSingleSpectrumSearch())
      return false;

   // tRealTimeStart used to track elapsed search time and to exit if g_staticParams.options.iMaxIndexRunTime is surpased
   g_staticParams.tRealTimeStart = std::chrono::high_resolution_clock::now();

   // We need to reset some of the static variables in-between input files
   CometPreprocess::Reset();

#ifdef PERF_DEBUG
   // print set search parameters
   std::map<std::string, CometParam*> mapParams = GetParamsMap();
   for (std::map<std::string, CometParam*>::iterator it = mapParams.begin(); it != mapParams.end(); ++it)
   {
      if (it->first != "[COMET_ENZYME_INFO]")
      {
         printf("OK parameter name=\"%s\" value=\"%s\"\n", it->first.c_str(), it->second->GetStringValue().c_str());
      }
   }
#endif

   // IMPORTANT: From this point onwards, because we've loaded some
   // spectra, we MUST "goto cleanup_results" before exiting the loop,
   // or we will create a memory leak!

   string sTmpDB;
   double* pdTmpSpectrum = new double[g_staticParams.iArraySizeGlobal];  // use this to determine most intense b/y-ions masses to report back

   bool bSucceeded = CometPreprocess::PreprocessSingleSpectrum(iPrecursorCharge, dMZ, pdMass, pdInten, iNumPeaks, pdTmpSpectrum);

   int iSize;
   int takeSearchResultsN;
   ThreadPool* tp = _tp;  // filled in InitializeSingleSpectrumSearch

   if (!bSucceeded)
      goto cleanup_results;

   if (g_pvQuery.empty())
   {
      delete[] pdTmpSpectrum;
      return false; // no search to run
   }
   bSucceeded = AllocateResultsMem();

   int iWhichQuery;

   if (!bSucceeded)
      goto cleanup_results;

   iWhichQuery = 0; // dealing with one query

   Query* pQuery;
   pQuery = g_pvQuery.at(iWhichQuery);  // there's only a single query spectrum

   g_massRange.dMinMass = pQuery->_pepMassInfo.dPeptideMassToleranceMinus;
   g_massRange.dMaxMass = g_pvQuery.at(g_pvQuery.size() - 1)->_pepMassInfo.dPeptideMassTolerancePlus;

   if (g_massRange.dMaxMass - g_massRange.dMinMass > g_massRange.dMinMass)
      g_massRange.bNarrowMassRange = true;  // unused in this context but setting here anyways
   else
      g_massRange.bNarrowMassRange = false;

   g_sCometVersion = comet_version;

   // Now that spectra are loaded to memory and sorted, do search.
   bSucceeded = CometSearch::RunSearch(tp);

   iSize = g_pvQuery.at(0)->iMatchPeptideCount;
   if (iSize > g_staticParams.options.iNumStored)
      iSize = g_staticParams.options.iNumStored;

   if (iSize > 1)
   {
      std::sort(pQuery->_pResults, pQuery->_pResults + iSize, CometPostAnalysis::SortFnXcorr);
   }

   takeSearchResultsN = topN; // return up to the top N results, or iSize

   if (takeSearchResultsN > iSize)
      takeSearchResultsN = iSize;

   //FIX ... is there a way to not have to do this just once and not have to fopen/fclose this db
   // file pointer for each query?
   FILE* fpdb;  // need FASTA file again to grab headers for output (currently just store file positions)
   sTmpDB = g_staticParams.databaseInfo.szDatabase;
   sTmpDB = sTmpDB.erase(sTmpDB.size() - 4); // need plain fasta if indexdb input
   if ((fpdb = fopen(sTmpDB.c_str(), "r")) == NULL)
   {
      string strErrorMsg = " Error (5) - cannot read database file \"" + sTmpDB + "\".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   if (bSucceeded && pQuery->iMatchPeptideCount > 0)
   {
      CometPostAnalysis::CalculateSP(pQuery->_pResults, 0, takeSearchResultsN);
      CometPostAnalysis::CalculateEValue(iWhichQuery, 0);
      CometPostAnalysis::CalculateDeltaCn(iWhichQuery);

      if (g_staticParams.options.iPrintAScoreProScore && g_pvQuery.at(0)->_pResults[0].cHasVariableMod == 2)  //FIX: 2 should be enum
         CometPostAnalysis::CalculateAScorePro(0, g_AScoreInterface);
   }
   else
      goto cleanup_results;

   bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();

   if (!bSucceeded)
      goto cleanup_results;

   for (int iWhichResult = 0; iWhichResult < takeSearchResultsN; ++iWhichResult)
   {
      Scores score;
      score.dCn = 0;
      score.xCorr = g_staticParams.options.dMinimumXcorr;
      score.matchedIons = 0;
      score.totalIons = 0;
      score.dAScorePro = 0;
      score.sAScoreProSiteScores.clear();
      std::string eachStrReturnPeptide;
      std::string eachStrReturnProtein;
      vector<Fragment> eachMatchedFragments;

      if (iSize > 0 && pQuery->_pResults[iWhichResult].fXcorr > g_staticParams.options.dMinimumXcorr && pQuery->_pResults[iWhichResult].usiLenPeptide > 0)
      {
         Results* pOutput = pQuery->_pResults;

         // Comment out this next line to not parse previous/next amino acids from  fasta for RTS reporting
         CometMassSpecUtils::GetPrevNextAA(fpfasta, 0, iWhichResult, 0, 0);

         // Set return values for peptide sequence, protein, xcorr and E-value
         eachStrReturnPeptide = std::string(1, pOutput[iWhichResult].cPrevAA) + ".";

         // n-term variable mod
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide] != 0)
         {
            std::stringstream ss;
            ss << "n[" << std::fixed << std::setprecision(4) << pOutput[iWhichResult].pdVarModSites[pOutput[iWhichResult].usiLenPeptide] << "]";
            eachStrReturnPeptide += ss.str();
         }

         for (int i = 0; i < pOutput[iWhichResult].usiLenPeptide; ++i)
         {
            eachStrReturnPeptide += pOutput[iWhichResult].szPeptide[i];

            if (pOutput[iWhichResult].piVarModSites[i] != 0)
            {
               std::stringstream ss;
               ss << "[" << std::fixed << std::setprecision(4) << pOutput[iWhichResult].pdVarModSites[i] << "]";
               eachStrReturnPeptide += ss.str();
            }
         }

         // c-term variable mod
         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].usiLenPeptide + 1] != 0)
         {
            std::stringstream ss;
            ss << "c[" << std::fixed << std::setprecision(4) << pOutput[iWhichResult].pdVarModSites[pOutput[iWhichResult].usiLenPeptide + 1] << "]";
            eachStrReturnPeptide += ss.str();
         }
         eachStrReturnPeptide += "." + std::string(1, pOutput[iWhichResult].cNextAA);

         std::vector<string> vProteinTargets;  // store vector of target protein names
         std::vector<string> vProteinDecoys;   // store vector of decoy protein names
         std::vector<string>::iterator it;

         int iPrintTargetDecoy = 0;
         unsigned int uiNumTotProteins = 0;
         bool bReturnFullProteinString = true;

         CometMassSpecUtils::GetProteinNameString(fpdb, iWhichQuery, iWhichResult, iPrintTargetDecoy, bReturnFullProteinString, &uiNumTotProteins, vProteinTargets, vProteinDecoys);

         bool bPrintDelim = false;
         if (iPrintTargetDecoy != 2)  // if not decoy only, print target proteins
         {
            for (it = vProteinTargets.begin(); it != vProteinTargets.end(); ++it)
            {
               if (bPrintDelim)
                  eachStrReturnProtein += " ; ";
               else
                  bPrintDelim = true;

               string sTmp = *it;
               std::replace(sTmp.begin(), sTmp.end(), ';', ',');  // since ';' is the delimiter between proteins, change all occurrences to ','
               eachStrReturnProtein += sTmp;
            }
         }

         if (iPrintTargetDecoy != 1)  // if not target only, print decoy proteins
         {
            for (it = vProteinDecoys.begin(); it != vProteinDecoys.end(); ++it)
            {
               if (bPrintDelim)
                  eachStrReturnProtein += " ; ";
               else
                  bPrintDelim = true;

               string sTmp = g_staticParams.szDecoyPrefix;
               sTmp += *it;
               std::replace(sTmp.begin(), sTmp.end(), ';', ',');  // since ';' is the delimiter between proteins, change all occurrences to ','
               eachStrReturnProtein += sTmp;
            }
         }

         score.xCorr = pOutput[iWhichResult].fXcorr;                      // xcorr
         score.dCn = pOutput[iWhichResult].fDeltaCn;                      // deltaCn
         score.dSp = pOutput[iWhichResult].fScoreSp;                      // prelim score
         score.dExpect = pOutput[iWhichResult].dExpect;                   // E-value
         score.mass = pOutput[iWhichResult].dPepMass - PROTON_MASS;       // calc neutral pep mass
         score.matchedIons = pOutput[iWhichResult].usiMatchedIons;        // ions matched
         score.totalIons = pOutput[iWhichResult].usiTotalIons;            // ions tot
         score.dAScorePro = pOutput[iWhichResult].fAScorePro;           // AScore
         score.sAScoreProSiteScores = pOutput[iWhichResult].sAScoreProSiteScores; // AScore site-specific

         int iMinLength = g_staticParams.options.peptideLengthRange.iEnd;
         for (int x = 0; x < iSize; ++x)
         {
            int iLen = (int)strlen(pOutput[x].szPeptide);
            if (iLen == 0)
               break;
            if (iLen < iMinLength)
               iMinLength = iLen;
         }

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

            map<int, double>::iterator it;
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

                  // get the fragment mass if it is n- or c-terimnus
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
                              continue;  // continue if this iMod entry has no mod mass or no NL mass specified
                           }

                           if (isNTerm)
                           {
                              // if have not already come across n-term mod residue for variable mod iMod, see if position i contains the variable mod
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
                              continue;  // no fragment NL yet in peptide so continue
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
         eachStrReturnPeptide = "";  // peptide
         eachStrReturnProtein = "";  // protein
         score.xCorr = -999  ;       // xcorr
         score.dSp = 0;        // prelim score
         score.dExpect = 999;      // E-value
         score.mass = 0;        // calc neutral pep mass
         score.matchedIons = 0;        // ions matched
         score.totalIons = 0;        // ions tot
         score.dAScorePro = 0;        // AScore
         score.dCn = 0;        // dCn
         score.sAScoreProSiteScores.clear();
      }
      strReturnPeptide.push_back(eachStrReturnPeptide);
      strReturnProtein.push_back(eachStrReturnProtein);
      matchedFragments.push_back(eachMatchedFragments);
      scores.push_back(score);
   }

cleanup_results:

   fclose(fpdb);  //FIX: would be nice to not fopen/fclose with each query

   // Deleting each Query object in the vector calls its destructor, which
   // frees the spectral memory (see definition for Query in CometDataInternal.h).
   for (auto it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
      delete (*it);

   g_pvQuery.clear();

   // Clean up the input files vector
   g_staticParams.vectorMassOffsets.clear();
   g_staticParams.precursorNLIons.clear();

   delete[] pdTmpSpectrum;

   return bSucceeded;
}


// Load all MS1 from raw file. Then search each MS1 query.
bool CometSearchManager::DoMS1SearchMultiResults(const double dMaxMS1RTDiff,
                                                 const double dMaxQueryRT,
                                                 const int /*topN*/,
                                                 const double dQueryRT,
                                                 double* pdMass,
                                                 double* pdInten,
                                                 int iNumPeaks,
                                                 vector<ScoresMS1>& scoresMS1)
{
   bool bSucceeded = false;
   double dMatchedSpecLibRT = 0.0;
   double dLinearRegressionRT = 0.0;

   if (iNumPeaks == 0)
      return false;

   bSucceeded = InitializeSingleSpectrumMS1Search();
   if (bSucceeded == false)
      return bSucceeded;

   g_pvQueryMS1.clear();

   ThreadPool* tp = _tp;  // filled in InitializeSingleSpectrumSearch

   //Load all MS1 spectrum from file
   if (g_bSpecLibRead == false)
      CometSpecLib::LoadSpecLibMS1Raw(tp, dMaxQueryRT, &dMaxSpecLibRT);

   // Process current MS1
   bSucceeded = CometPreprocess::PreprocessMS1SingleSpectrum(pdMass, pdInten, iNumPeaks);

   if (!bSucceeded)
      goto cleanup_results;

   if (g_pvQueryMS1.empty())
      return false; // no search to run

   bSucceeded = AllocateResultsMemMS1();

   if (!bSucceeded)
      goto cleanup_results;

   QueryMS1* pQueryMS1;
   pQueryMS1 = g_pvQueryMS1.at(0);
   pQueryMS1->_pSpecLibResultsMS1.fDotProduct = 0.0;
   pQueryMS1->_pSpecLibResultsMS1.fRTime = 0.0;
   pQueryMS1->_pSpecLibResultsMS1.iWhichSpecLib = 0;

   bSucceeded = CometSearch::RunMS1Search(tp, dQueryRT, dMaxMS1RTDiff, dMaxSpecLibRT, dMaxQueryRT);

   // pass best RT match to regression
   dMatchedSpecLibRT = pQueryMS1->_pSpecLibResultsMS1.fRTime;
   dLinearRegressionRT = pMS1Aligner.processRetentionMatch(dQueryRT, dMatchedSpecLibRT);

   if (bSucceeded)
   {
      ScoresMS1 scoreMS1;
      scoreMS1.fDotProduct = pQueryMS1->_pSpecLibResultsMS1.fDotProduct;
//      scoreMS1.fRTime = pQueryMS1->_pSpecLibResultsMS1.fRTime;
      scoreMS1.fRTime = (float)dLinearRegressionRT;
      scoreMS1.iScanNumber = pQueryMS1->_pSpecLibResultsMS1.iWhichSpecLib;
      scoresMS1.push_back(scoreMS1);
   }
   else
      goto cleanup_results;

cleanup_results:

   // close raw file here?
   // fclose(fpdb);  //FIX: would be nice to not fopen/fclose with each query

   // Deleting each Query object in the vector calls its destructor, which
   // frees the spectral memory (see definition for Query in CometDataInternal.h).
   for (auto it = g_pvQuery.begin(); it != g_pvQuery.end(); ++it)
      delete (*it);

   g_pvQuery.clear();

   return bSucceeded;
}


// Restrict variable mods to a list of proteins that are read here
// File format is an "int string" on each line where "int" is the
// variable modfication number and "string" is a single protein accession word
bool CometSearchManager::ReadProteinVarModFilterFile()
{
   FILE* fp;
   char szBuf[512];

   if ((fp = fopen(g_staticParams.variableModParameters.sProteinLModsListFile.c_str(), "r")) != NULL)
   {
      printf(" Protein variable modifications filter:\n");

      while (fgets(szBuf, 512, fp))
      {
         if (strlen(szBuf) > 3)
         {
            char szProtein[512];
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


//SetAScoreOptions(g_AScoreOptions);
void CometSearchManager::SetAScoreOptions(void)
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

         if (g_staticParams.options.iPrintAScoreProScore - 1 == i)
         {
            // Target modification settings
            g_AScoreOptions.setSymbol(i + 1 + '0');
            g_AScoreOptions.setResidues(g_staticParams.variableModParameters.varModList[i].szVarModChar);

            // Set up neutral loss
            AScoreProCpp::NeutralLoss neutralLoss;
            neutralLoss.setMass(-(g_staticParams.variableModParameters.varModList[i].dNeutralLoss));
            neutralLoss.setResidues(g_staticParams.variableModParameters.varModList[i].szVarModChar);
            g_AScoreOptions.setNeutralLoss(neutralLoss);

            if (!isEqual(g_staticParams.variableModParameters.varModList[i].dNeutralLoss, 0.0))
               bSetNeutralLossMask = true;

         }
      }
   }
   g_AScoreOptions.setDiffMods(diffMods);

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

   g_AScoreOptions.setIonSeries(uiIonSeriesMask);
   g_AScoreOptions.setIonSeriesList(ionSeriesList);

   // Peak depth settings
   g_AScoreOptions.setPeakDepth(0);
   g_AScoreOptions.setMaxPeakDepth(50);

   // Fragment matching tolerance

   if (g_staticParams.tolerances.dFragmentBinSize <= 0.05)
      g_AScoreOptions.setTolerance(0.05);
   else
      g_AScoreOptions.setTolerance(0.3);

   g_AScoreOptions.setUnits(Mass::Units::DALTON);
   g_AScoreOptions.setUnitText("Da");

   // Window size for filtering peaks
   g_AScoreOptions.setWindow(70);

   // Enable low mass cutoff
   g_AScoreOptions.setLowMassCutoff(true);

   // Filter low intensity peaks
   g_AScoreOptions.setFilterLowIntensity(0);

   // C-terminal settings
   g_AScoreOptions.setNoCterm(true);

   // Scoring options
   g_AScoreOptions.setUseMobScore(true);
   g_AScoreOptions.setUseDeltaAscore(true);

   // Max peptides and other limits
   g_AScoreOptions.setMaxPeptides(1000);
   // g_AScoreOptions.setMaxDiff(5); // From max_diff in JSON

   // Initialize other fields to default values from JSON
   g_AScoreOptions.setMz(0);
   g_AScoreOptions.setPeptide("");
   g_AScoreOptions.setScan(0);

   // Deisotoping type (empty string means no deisotoping)
   //g_AScoreOptions.setDeisotopingType("");

   // Set up static modifications
   // FIX:  deal with static N-term and C-term mods
   std::vector<PeptideMod> staticMods;
   for (int i = 'A' ; i <= 'Z'; ++i)
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
   g_AScoreOptions.setStaticMods(staticMods);

   // Apply static mods to AminoAcidMasses
   AminoAcidMasses& masses = g_AScoreOptions.getMasses();
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

   // Print all AScoreOptions values
/*
   std::cout << "AScoreOptions values:" << std::endl;
   std::cout << "ionSeriesList: ";
   for (const auto& s : g_AScoreOptions.getIonSeriesList()) std::cout << s << " ";
   std::cout << std::endl;
   std::cout << "ionSeries: " << g_AScoreOptions.getIonSeries() << std::endl;
   std::cout << "diffMods: ";
   for (const auto& mod : g_AScoreOptions.getDiffMods()) std::cout << mod.getResidues() << "(" << mod.getMass() << ") ";
   std::cout << std::endl;
   std::cout << "staticMods: ";
   for (const auto& mod : g_AScoreOptions.getStaticMods()) std::cout << mod.getResidues() << "(" << mod.getMass() << ") ";
   std::cout << std::endl;
   std::cout << "neutralLoss: " << g_AScoreOptions.getNeutralLoss().getMass() << " " << g_AScoreOptions.getNeutralLoss().getResidues() << std::endl;
   std::cout << "peakDepth: " << g_AScoreOptions.getPeakDepth() << std::endl;
   std::cout << "maxPeakDepth: " << g_AScoreOptions.getMaxPeakDepth() << std::endl;
   std::cout << "tolerance: " << g_AScoreOptions.getTolerance() << std::endl;
   std::cout << "unitText: " << g_AScoreOptions.getUnitText() << std::endl;
   std::cout << "units: " << static_cast<int>(g_AScoreOptions.getUnits()) << std::endl;
   std::cout << "window: " << g_AScoreOptions.getWindow() << std::endl;
   std::cout << "lowMassCutoff: " << g_AScoreOptions.getLowMassCutoff() << std::endl;
   std::cout << "filterLowIntensity: " << g_AScoreOptions.getFilterLowIntensity() << std::endl;
   std::cout << "noCterm: " << g_AScoreOptions.getNoCterm() << std::endl;
   std::cout << "useMobScore: " << g_AScoreOptions.getUseMobScore() << std::endl;
   std::cout << "useDeltaAscore: " << g_AScoreOptions.getUseDeltaAscore() << std::endl;
   std::cout << "symbol: " << g_AScoreOptions.getSymbol() << std::endl;
   std::cout << "residues: " << g_AScoreOptions.getResidues() << std::endl;
   std::cout << "maxPeptides: " << g_AScoreOptions.getMaxPeptides() << std::endl;
*/
}
