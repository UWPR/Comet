/*
   Copyright 2013 University of Washington

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

#include "Common.h"
#include "CometMassSpecUtils.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometPreprocess.h"
#include "CometWriteOut.h"
#include "CometWriteSqt.h"
#include "CometWriteTxt.h"
#include "CometWritePepXML.h"
#include "CometWritePinXML.h"
#include "Threading.h"
#include "ThreadPool.h"
#include "CometDataInternal.h"
#include "CometSearchManager.h"

#ifdef _WIN32
#define STRCMP_IGNORE_CASE(a,b) strcmpi(a,b)
#else 
#define STRCMP_IGNORE_CASE(a,b) strcasecmp(a,b)
#endif

std::vector<Query*>           g_pvQuery;
std::vector<InputFileInfo *>  g_pvInputFiles;
StaticParams                  g_staticParams;
MassRange                     g_massRange;
Mutex                         g_pvQueryMutex;

/******************************************************************************
*
* Static helper functions
*
******************************************************************************/
static void GetHostName()
{
#ifdef _WIN32
    WSADATA WSAData;
    WSAStartup(MAKEWORD(1, 0), &WSAData);

    if (gethostname(g_staticParams.szHostName, SIZE_FILE) != 0)
        strcpy(g_staticParams.szHostName, "locahost");

    WSACleanup();
#else
    if (gethostname(g_staticParams.szHostName, SIZE_FILE) != 0)
        strcpy(g_staticParams.szHostName, "locahost");
#endif

    char *pStr;
    if ((pStr = strchr(g_staticParams.szHostName, '.'))!=NULL)
        *pStr = '\0';
}

static void UpdateInputFile(InputFileInfo *pFileInfo)
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

    int iLen = strlen(g_staticParams.inputFile.szFileName);

    if (!STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 6, ".mzXML")
            || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 5, ".mzML")
            || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 4, ".mz5")
            || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 9, ".mzXML.gz")
            || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 8, ".mzML.gz"))

    {
        g_staticParams.inputFile.iInputType = InputType_MZXML;
    } 

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
            logerr("\n Error - could not create directory \"%s\".\n", g_staticParams.inputFile.szBaseName);
            exit(1);
            }
        }
        if (g_staticParams.options.iDecoySearch == 2)
        {
            char szDecoyDir[SIZE_FILE];
            sprintf(szDecoyDir, "%s_decoy", g_staticParams.inputFile.szBaseName);

            if (_mkdir(szDecoyDir) == -1)
            {
            errno_t err;
            _get_errno(&err);

            if (err != EEXIST) 
            {
                logerr("\n Error - could not create directory \"%s\".\n", szDecoyDir);
                exit(1);
            }
            }
        }
#else
        if ((mkdir(g_staticParams.inputFile.szBaseName, 0775) == -1) && (errno != EEXIST))
        {
            logerr("\n Error - could not create directory \"%s\".\n", g_staticParams.inputFile.szBaseName);
            exit(1);
        }
        if (g_staticParams.options.iDecoySearch == 2)
        {
            char szDecoyDir[SIZE_FILE];
            sprintf(szDecoyDir, "%s_decoy", g_staticParams.inputFile.szBaseName);

            if ((mkdir(szDecoyDir , 0775) == -1) && (errno != EEXIST))
            {
            logerr("\n Error - could not create directory \"%s\".\n\n", szDecoyDir);
            exit(1);
            }
        }
#endif
    }
}

static void SetMSLevelFilter(MSReader &mstReader)
{
    vector<MSSpectrumType> msLevel;
    if (g_staticParams.options.iMSLevel == 3)
    {
        msLevel.push_back(MS3);
    }
    else
    {
        msLevel.push_back(MS2);
    }
    mstReader.setFilter(msLevel);
}
   
// Allocate memory for the _pResults struct for each g_pvQuery entry.
static void AllocateResultsMem()
{
    for (unsigned i=0; i<g_pvQuery.size(); i++)
    {
        Query* pQuery = g_pvQuery.at(i);

        pQuery->_pResults = (struct Results *)malloc(sizeof(struct Results) * g_staticParams.options.iNumStored);

        if (pQuery->_pResults == NULL)
        {
            logerr(" Error malloc(_pResults[])\n");
            exit(1);
        }

        //MH: Initializing iLenPeptide to 0 is necessary to silence Valgrind Errors.
        for(int xx=0;xx<g_staticParams.options.iNumStored;xx++)
            pQuery->_pResults[xx].iLenPeptide=0;

        pQuery->iDoXcorrCount = 0;
        pQuery->siLowestSpScoreIndex = 0;
        pQuery->fLowestSpScore = 0.0;

        if (g_staticParams.options.iDecoySearch==2)
        {
            pQuery->_pDecoys = (struct Results *)malloc(sizeof(struct Results) * g_staticParams.options.iNumStored);

            if (pQuery->_pDecoys == NULL)
            {
            logerr(" Error malloc(_pDecoys[])\n");
            exit(1);
            }

            //MH: same logic as my comment above
            for(int xx=0;xx<g_staticParams.options.iNumStored;xx++)
            pQuery->_pDecoys[xx].iLenPeptide=0;

            pQuery->iDoDecoyXcorrCount = 0;
            pQuery->siLowestDecoySpScoreIndex = 0;
            pQuery->fLowestDecoySpScore = 0.0;
        }

        int j;
        for (j=0; j<HISTO_SIZE; j++)
        {
            pQuery->iCorrelationHistogram[j]=0;
            pQuery->iDecoyCorrelationHistogram[j]=0;
        }

        for (j=0; j<g_staticParams.options.iNumStored; j++)
        {
            pQuery->_pResults[j].fXcorr = 0.0;
            pQuery->_pResults[j].fScoreSp = 0.0;
            pQuery->_pResults[j].dExpect = 0.0;
            pQuery->_pResults[j].szPeptide[0] = '\0';
            pQuery->_pResults[j].szProtein[0] = '\0';

            if (g_staticParams.options.iDecoySearch==2)
            {
            pQuery->_pDecoys[j].fXcorr = 0.0;
            pQuery->_pDecoys[j].fScoreSp = 0.0;
            pQuery->_pDecoys[j].dExpect = 0.0;
            pQuery->_pDecoys[j].szPeptide[0] = '\0';
            pQuery->_pDecoys[j].szProtein[0] = '\0';
            }
        }
    }
}

static bool compareByPeptideMass(Query const* a, Query const* b)
{
    return (a->_pepMassInfo.dExpPepMass < b->_pepMassInfo.dExpPepMass);
}

static void CalcRunTime(time_t tStartTime)
{
    char szTimeBuf[512];
    time_t tEndTime;
    int iTmp;

    time(&tEndTime);

    int iElapseTime=(int)difftime(tEndTime, tStartTime);

    // Print out header/search info.
    sprintf(szTimeBuf, "%s,", g_staticParams.szDate);
    if ( (iTmp = (int)(iElapseTime/3600) )>0)
        sprintf(szTimeBuf+strlen(szTimeBuf), " %d hr.", iTmp);
    if ( (iTmp = (int)((iElapseTime-(int)(iElapseTime/3600)*3600)/60) )>0)
        sprintf(szTimeBuf+strlen(szTimeBuf), " %d min.", iTmp);
    if ( (iTmp = (int)((iElapseTime-((int)(iElapseTime/3600))*3600)%60) )>0)
        sprintf(szTimeBuf+strlen(szTimeBuf), " %d sec.", iTmp);
    if (iElapseTime == 0)
        sprintf(szTimeBuf+strlen(szTimeBuf), " 0 sec.");
    sprintf(szTimeBuf+strlen(szTimeBuf), " on %s", g_staticParams.szHostName);

    g_staticParams.iElapseTime = iElapseTime;
    strncpy(g_staticParams.szTimeBuf, szTimeBuf, 256);
    g_staticParams.szTimeBuf[255]='\0';
}
static void PrintParameters()
{
    // print parameters

    char szIsotope[16];
    char szPeak[16];

    sprintf(g_staticParams.szIonSeries, "ion series ABCXYZ nl: %d%d%d%d%d%d %d",
            g_staticParams.ionInformation.iIonVal[ION_SERIES_A],
            g_staticParams.ionInformation.iIonVal[ION_SERIES_B],
            g_staticParams.ionInformation.iIonVal[ION_SERIES_C],
            g_staticParams.ionInformation.iIonVal[ION_SERIES_X],
            g_staticParams.ionInformation.iIonVal[ION_SERIES_Y],
            g_staticParams.ionInformation.iIonVal[ION_SERIES_Z],
            g_staticParams.ionInformation.bUseNeutralLoss);

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

    szIsotope[0]='\0';
    if (g_staticParams.tolerances.iIsotopeError==1)
        strcpy(szIsotope, "ISOTOPE1");
    else if (g_staticParams.tolerances.iIsotopeError==2)
        strcpy(szIsotope, "ISOTOPE2");

    szPeak[0]='\0';
    if (g_staticParams.ionInformation.iTheoreticalFragmentIons==1)
        strcpy(szPeak, "PEAK1");

    sprintf(g_staticParams.szDisplayLine, "display top %d, %s%s%s%s%s%s%s%s",
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

static void ValidateOutputFormat()
{
   if (!g_staticParams.options.bOutputSqtStream
         && !g_staticParams.options.bOutputSqtFile
         && !g_staticParams.options.bOutputTxtFile
         && !g_staticParams.options.bOutputPepXMLFile
         && !g_staticParams.options.bOutputPinXMLFile
         && !g_staticParams.options.bOutputOutFiles)
   {
      logout("\n Comet version \"%s\"\n", comet_version);
      logout(" Please specify at least one output format.\n\n");
      exit(1);
   }
}

static void ValidateSequenceDatabaseFile()
{
   FILE *fpcheck;
   
   // Quick sanity check to make sure sequence db file is present before spending
   // time reading & processing spectra and then reporting this error.
   if ((fpcheck=fopen(g_staticParams.databaseInfo.szDatabase, "r")) == NULL)
   {
      logerr("\n Error - cannot read database file \"%s\".\n", g_staticParams.databaseInfo.szDatabase);
      logerr(" Check that the file exists and is readable.\n\n");
      exit(1);
   }
   fclose(fpcheck);
}

static void ValidateScanRange()
{
   if (g_staticParams.options.scanRange.iEnd < g_staticParams.options.scanRange.iStart && g_staticParams.options.scanRange.iEnd != 0)
   {
      logerr("\n Comet version %s\n %s\n\n", comet_version, copyright);
      logerr(" Error - start scan is %d but end scan is %d.\n", g_staticParams.options.scanRange.iStart, g_staticParams.options.scanRange.iEnd);
      logerr(" The end scan must be >= to the start scan.\n\n");
      exit(1);
   }
}

/******************************************************************************
*
* CometSearchManager class implementation.
*
******************************************************************************/

CometSearchManager::CometSearchManager()
{
   // Initialize the mutexes we'll use to protect global data.
   Threading::CreateMutex(&g_pvQueryMutex);
}

CometSearchManager::~CometSearchManager()
{
   // Destroy the mutex we used to protect g_pvQuery.
   Threading::DestroyMutex(g_pvQueryMutex);

   // Clean up the input files vector
   for (int i=0; i<(int)g_pvInputFiles.size(); i++)
   {
      delete g_pvInputFiles.at(i);
   }
   g_pvInputFiles.clear();

   for (std::map<string, CometParam*>::iterator it = _mapStaticParams.begin(); it != _mapStaticParams.end(); ++it)
   {
       delete it->second;
   }
   _mapStaticParams.clear();
}

void CometSearchManager::InitializeStaticParams()
{
   int iIntData;
   double dDoubleData;
   string strData;
   IntRange intRangeData;
   DoubleRange doubleRangeData;
    
   if(GetParamValue("database_name", strData))
   {
      strcpy(g_staticParams.databaseInfo.szDatabase, strData.c_str());
   }

   if(GetParamValue("decoy_prefix", strData))
   {
      strcpy(g_staticParams.szDecoyPrefix, strData.c_str());
   }

   GetParamValue("nucleotide_reading_frame", g_staticParams.options.iWhichReadingFrame);
    
   GetParamValue("mass_type_parent", g_staticParams.massUtility.bMonoMassesParent);
    
   GetParamValue("mass_type_fragment", g_staticParams.massUtility.bMonoMassesFragment);

   GetParamValue("show_fragment_ions", g_staticParams.options.bShowFragmentIons);

   GetParamValue("num_threads", g_staticParams.options.iNumThreads);

   GetParamValue("clip_nterm_methionine", g_staticParams.options.bClipNtermMet);

   GetParamValue("theoretical_fragment_ions", g_staticParams.ionInformation.iTheoreticalFragmentIons);
   if ((g_staticParams.ionInformation.iTheoreticalFragmentIons < 0) || 
       (g_staticParams.ionInformation.iTheoreticalFragmentIons > 1))
   {
      g_staticParams.ionInformation.iTheoreticalFragmentIons = 0;
   }

   GetParamValue("use_A_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_A]);

   GetParamValue("use_B_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_B]);

   GetParamValue("use_C_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_C]);

   GetParamValue("use_X_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_X]);

   GetParamValue("use_Y_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]);

   GetParamValue("use_Z_ions", g_staticParams.ionInformation.iIonVal[ION_SERIES_Z]);

   GetParamValue("use_NL_ions", g_staticParams.ionInformation.bUseNeutralLoss);

   GetParamValue("use_sparse_matrix", g_staticParams.options.bSparseMatrix);

   GetParamValue("variable_mod1", g_staticParams.variableModParameters.varModList[VMOD_1_INDEX]);

   GetParamValue("variable_mod2", g_staticParams.variableModParameters.varModList[VMOD_2_INDEX]);

   GetParamValue("variable_mod3", g_staticParams.variableModParameters.varModList[VMOD_3_INDEX]);

   GetParamValue("variable_mod4", g_staticParams.variableModParameters.varModList[VMOD_4_INDEX]);
   
   GetParamValue("variable_mod5", g_staticParams.variableModParameters.varModList[VMOD_5_INDEX]);

   GetParamValue("variable_mod6", g_staticParams.variableModParameters.varModList[VMOD_6_INDEX]);

   if (GetParamValue("max_variable_mods_in_peptide", iIntData))
   {
      if (iIntData > 0)
      {
         g_staticParams.variableModParameters.iMaxVarModPerPeptide = iIntData;
      }
   }

   GetParamValue("fragment_bin_tol", g_staticParams.tolerances.dFragmentBinSize);
   if (g_staticParams.tolerances.dFragmentBinSize < 0.01)
   {
      g_staticParams.tolerances.dFragmentBinSize = 0.01;
   }

   GetParamValue("fragment_bin_offset", g_staticParams.tolerances.dFragmentBinStartOffset);

   GetParamValue("peptide_mass_tolerance", g_staticParams.tolerances.dInputTolerance);

   GetParamValue("precursor_tolerance_type", g_staticParams.tolerances.iMassToleranceType);
   if ((g_staticParams.tolerances.iMassToleranceType < 0) || 
       (g_staticParams.tolerances.iMassToleranceType > 1))
   {
      g_staticParams.tolerances.iMassToleranceType = 0;
   }

   GetParamValue("peptide_mass_units", g_staticParams.tolerances.iMassToleranceUnits);
   if ((g_staticParams.tolerances.iMassToleranceUnits < 0) || 
       (g_staticParams.tolerances.iMassToleranceUnits > 2))
   {
      g_staticParams.tolerances.iMassToleranceUnits = 0;  // 0=amu, 1=mmu, 2=ppm
   }

   GetParamValue("isotope_error", g_staticParams.tolerances.iIsotopeError);
   if ((g_staticParams.tolerances.iIsotopeError < 0) || 
       (g_staticParams.tolerances.iIsotopeError > 2))
   {
      g_staticParams.tolerances.iIsotopeError = 0;
   }

   GetParamValue("num_output_lines", g_staticParams.options.iNumPeptideOutputLines);

   GetParamValue("num_results", g_staticParams.options.iNumStored);
   
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

   GetParamValue("print_expect_score", g_staticParams.options.bPrintExpectScore);

   GetParamValue("output_sqtstream", g_staticParams.options.bOutputSqtStream);

   GetParamValue("output_sqtfile", g_staticParams.options.bOutputSqtFile);

   GetParamValue("output_txtfile", g_staticParams.options.bOutputTxtFile);

   GetParamValue("output_pepxmlfile", g_staticParams.options.bOutputPepXMLFile);

   GetParamValue("output_pinxmlfile", g_staticParams.options.bOutputPinXMLFile);

   GetParamValue("output_outfiles", g_staticParams.options.bOutputOutFiles);

   GetParamValue("skip_researching", g_staticParams.options.bSkipAlreadyDone);

   GetParamValue("variable_C_terminus", g_staticParams.variableModParameters.dVarModMassC);

   GetParamValue("variable_N_terminus", g_staticParams.variableModParameters.dVarModMassN);

   GetParamValue("variable_C_terminus_distance", g_staticParams.variableModParameters.iVarModCtermDistance);

   GetParamValue("variable_N_terminus_distance", g_staticParams.variableModParameters.iVarModNtermDistance);

   GetParamValue("add_Cterm_peptide", g_staticParams.staticModifications.dAddCterminusPeptide);

   GetParamValue("add_Nterm_peptide", g_staticParams.staticModifications.dAddNterminusPeptide);

   GetParamValue("add_Cterm_protein", g_staticParams.staticModifications.dAddCterminusProtein);

   GetParamValue("add_Nterm_protein", g_staticParams.staticModifications.dAddNterminusProtein);

   if (GetParamValue("add_G_glycine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'G'] = dDoubleData;
   }
    
   if (GetParamValue("add_A_alanine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'A'] = dDoubleData;
   }

   if (GetParamValue("add_S_serine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'S'] = dDoubleData;
   }

   if (GetParamValue("add_P_proline", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'P'] = dDoubleData;
   }

   if (GetParamValue("add_V_valine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'V'] = dDoubleData;
   }

   if (GetParamValue("add_T_threonine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'T'] = dDoubleData;
   }

   if (GetParamValue("add_C_cysteine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'C'] = dDoubleData;
   }

   if (GetParamValue("add_L_leucine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'L'] = dDoubleData;
   }

   if (GetParamValue("add_I_isoleucine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'I'] = dDoubleData;
   }

   if (GetParamValue("add_N_asparagine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'N'] = dDoubleData;
   }

   if (GetParamValue("add_O_ornithine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'O'] = dDoubleData;
   }

   if (GetParamValue("add_D_aspartic_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'D'] = dDoubleData;
   }

   if (GetParamValue("add_Q_glutamine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'Q'] = dDoubleData;
   }

   if (GetParamValue("add_K_lysine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'K'] = dDoubleData;
   }

   if (GetParamValue("add_E_glutamic_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'E'] = dDoubleData;
   }
   
   if (GetParamValue("add_M_methionine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'M'] = dDoubleData;
   }
   
   if (GetParamValue("add_H_histidine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'H'] = dDoubleData;
   }

   if (GetParamValue("add_F_phenylalanine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'F'] = dDoubleData;
   }

   if (GetParamValue("add_R_arginine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'R'] = dDoubleData;
   }

   if (GetParamValue("add_Y_tyrosine", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'Y'] = dDoubleData;
   }

   if (GetParamValue("add_W_tryptophan", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'W'] = dDoubleData;
   }

   if (GetParamValue("add_B_user_amino_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'B'] = dDoubleData;
   }

   if (GetParamValue("add_J_user_amino_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'J'] = dDoubleData;
   }

   if (GetParamValue("add_U_user_amino_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'U'] = dDoubleData;
   }

   if (GetParamValue("add_X_user_amino_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'X'] = dDoubleData;
   }

   if (GetParamValue("add_Z_user_amino_acid", dDoubleData))
   {
      g_staticParams.staticModifications.pdStaticMods[(int)'Z'] = dDoubleData;
   }

   GetParamValue("num_enzyme_termini", g_staticParams.options.iEnzymeTermini);
   if ((g_staticParams.options.iEnzymeTermini != 1) && 
       (g_staticParams.options.iEnzymeTermini != 8) && 
       (g_staticParams.options.iEnzymeTermini != 9))
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

   if (GetParamValue("spectrum_batch_size", iIntData))
   {
      if (iIntData > 0)
      {
         g_staticParams.options.iSpectrumBatchSize = iIntData;
      }
   }

   iIntData = 0;
   if (GetParamValue("minimum_peaks", iIntData))
   {
      if (iIntData > 0)
      {
         g_staticParams.options.iMinPeaks = iIntData;
      }
   }

   if (GetParamValue("precursor_charge", intRangeData))
   {
      if ((intRangeData.iEnd >= intRangeData.iStart) && (intRangeData.iStart >= 0) && (intRangeData.iEnd > 0))
      {
         if (intRangeData.iStart==0)
         {
            g_staticParams.options.iStartCharge = 1;
         }
         else
         {
            g_staticParams.options.iStartCharge = intRangeData.iStart;
         }

         g_staticParams.options.iEndCharge = intRangeData.iEnd;
      }
   }

   iIntData = 0;
   if (GetParamValue("max_fragment_charge", iIntData))
   {
      if (iIntData > MAX_FRAGMENT_CHARGE)
      {
         iIntData = MAX_FRAGMENT_CHARGE;
      }

      if (iIntData > 0)
      {
         g_staticParams.options.iMaxFragmentCharge = iIntData;
      }
      // else will go to default value (3)
   }

   iIntData = 0;
   if (GetParamValue("max_precursor_charge", iIntData)) 
   {
      if (iIntData > MAX_PRECURSOR_CHARGE)
      {
         iIntData = MAX_PRECURSOR_CHARGE;
      }

      if (iIntData > 0)
      {
         g_staticParams.options.iMaxPrecursorCharge = iIntData;
      }
      // else will go to default value (6)
   }

   if (GetParamValue("digest_mass_range", doubleRangeData))
   {
      if ((doubleRangeData.dEnd >= doubleRangeData.dStart) && (doubleRangeData.dStart >= 0.0))
      {
         g_staticParams.options.dLowPeptideMass = doubleRangeData.dStart;
         g_staticParams.options.dHighPeptideMass = doubleRangeData.dEnd;
      }
   }

   if (GetParamValue("ms_level", iIntData))
   {
      if (iIntData == 3)
      {
         g_staticParams.options.iMSLevel = 3;
      }
      // else will go to default value (2)
   }

   if (GetParamValue("activation_method", strData))
   {
      strcpy(g_staticParams.options.szActivationMethod, strData.c_str());
   }

   GetParamValue("minimum_intensity", g_staticParams.options.dMinIntensity);
   if (g_staticParams.options.dMinIntensity < 0.0)
   {
      g_staticParams.options.dMinIntensity = 0.0;
   }

   GetParamValue("decoy_search", g_staticParams.options.iDecoySearch);
   if ((g_staticParams.options.iDecoySearch < 0) || (g_staticParams.options.iDecoySearch > 2))
   {
      g_staticParams.options.iDecoySearch = 0;
   }

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

   GetHostName();

   // If # threads not specified, poll system to get # threads to launch.
   if (g_staticParams.options.iNumThreads == 0)
   {
#ifdef _WIN32
      SYSTEM_INFO sysinfo;
      GetSystemInfo( &sysinfo );
      g_staticParams.options.iNumThreads = sysinfo.dwNumberOfProcessors;
#else
      g_staticParams.options.iNumThreads = sysconf( _SC_NPROCESSORS_ONLN );
#endif
      if (g_staticParams.options.iNumThreads < 1 || g_staticParams.options.iNumThreads > MAX_THREADS)
          g_staticParams.options.iNumThreads = 2;  // Default to 2 threads.
   }

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

   GetParamValue("[COMET_ENZYME_INFO]", g_staticParams.enzymeInformation);
   if (!strncmp(g_staticParams.enzymeInformation.szSearchEnzymeBreakAA, "-", 1) && 
       !strncmp(g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA, "-", 1))
   {
      g_staticParams.options.bNoEnzymeSelected = 1;
   }
   else
   {
      g_staticParams.options.bNoEnzymeSelected = 0;
   }

   GetParamValue("allowed_missed_cleavage", g_staticParams.enzymeInformation.iAllowedMissedCleavage);
   if (g_staticParams.enzymeInformation.iAllowedMissedCleavage < 0)
   {
      g_staticParams.enzymeInformation.iAllowedMissedCleavage = 0;
   }

   // Load ion series to consider, useA, useB, useY are for neutral losses.
   g_staticParams.ionInformation.iNumIonSeriesUsed = 0;
   for (int i=0; i<6; i++)
   {
      if (g_staticParams.ionInformation.iIonVal[i] > 0)
         g_staticParams.ionInformation.piSelectedIonSeries[g_staticParams.ionInformation.iNumIonSeriesUsed++] = i;
   }

   // Variable mod search for AAs listed in szVarModChar.
   g_staticParams.szMod[0] = '\0';
   for (int i=0; i<VMODS; i++)
   {
      if (!isEqual(g_staticParams.variableModParameters.varModList[i].dVarModMass, 0.0) &&
          (g_staticParams.variableModParameters.varModList[i].szVarModChar[0]!='\0'))
      {
         sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "(%s%c %+0.6f) ", 
               g_staticParams.variableModParameters.varModList[i].szVarModChar,
               g_staticParams.variableModParameters.cModCode[i],
               g_staticParams.variableModParameters.varModList[i].dVarModMass);
         g_staticParams.variableModParameters.bVarModSearch = 1;
      }
   }

   if (!isEqual(g_staticParams.variableModParameters.dVarModMassN, 0.0))
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "(nt] %+0.6f) ", 
            g_staticParams.variableModParameters.dVarModMassN);       // FIX determine .out file header string for this?
      g_staticParams.variableModParameters.bVarModSearch = 1;
   }
   if (!isEqual(g_staticParams.variableModParameters.dVarModMassC, 0.0))
   {
      sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "(ct[ %+0.6f) ", 
            g_staticParams.variableModParameters.dVarModMassC);
      g_staticParams.variableModParameters.bVarModSearch = 1;
   }

   // Do Sp scoring after search based on how many lines to print out.
   if (g_staticParams.options.iNumStored > NUM_STORED)
      g_staticParams.options.iNumStored = NUM_STORED;
   else if (g_staticParams.options.iNumStored < 1)
      g_staticParams.options.iNumStored = 1;


   if (g_staticParams.options.iNumPeptideOutputLines > g_staticParams.options.iNumStored)
      g_staticParams.options.iNumPeptideOutputLines = g_staticParams.options.iNumStored;
   else if (g_staticParams.options.iNumPeptideOutputLines < 1)
      g_staticParams.options.iNumPeptideOutputLines = 1;

   if (g_staticParams.peaksInformation.iNumMatchPeaks > 5)
      g_staticParams.peaksInformation.iNumMatchPeaks = 5;

   // FIX how to deal with term mod on both peptide and protein?
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

   for (int i=65; i<=90; i++)  // 65-90 represents upper case letters in ASCII
   {
      if (!isEqual(g_staticParams.staticModifications.pdStaticMods[i], 0.0))
      {
         sprintf(g_staticParams.szMod + strlen(g_staticParams.szMod), "%c=%0.6f ", i,
               g_staticParams.massUtility.pdAAMassParent[i] += g_staticParams.staticModifications.pdStaticMods[i]);
         g_staticParams.massUtility.pdAAMassFragment[i] += g_staticParams.staticModifications.pdStaticMods[i];
      }
      else if (i=='B' || i=='J' || i=='U' || i=='X' || i=='Z')
      {
         g_staticParams.massUtility.pdAAMassParent[i] = 999999.;
         g_staticParams.massUtility.pdAAMassFragment[i] = 999999.;
      }
   }

   // Print out enzyme name to g_staticParams.szMod.
   if (!g_staticParams.options.bNoEnzymeSelected)
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

   if (g_staticParams.tolerances.dFragmentBinStartOffset < 0.0 || g_staticParams.tolerances.dFragmentBinStartOffset >1.0)
   {
      logerr(" Error - bin offset %f must between 0.0 and 1.0\n",
            g_staticParams.tolerances.dFragmentBinStartOffset);
      exit(1);
   }

   if (!g_staticParams.options.bOutputOutFiles)
   {
      g_staticParams.options.bSkipAlreadyDone = 0;
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
}

void CometSearchManager::AddInputFiles(vector<InputFileInfo*> &pvInputFiles)
{
   int numInputFiles = pvInputFiles.size();
   for (int i = 0; i < numInputFiles; i++)
   {
      g_pvInputFiles.push_back(pvInputFiles.at(i));
   }
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
   {
      return false;
   }

   TypedCometParam<string> *pParam = static_cast<TypedCometParam<string>*>(it->second);
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
   {
      return false;
   }
    
   TypedCometParam<int> *pParam = static_cast<TypedCometParam<int>*>(it->second);
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
   {
      return false;
   }
    
   TypedCometParam<double> *pParam = static_cast<TypedCometParam<double>*>(it->second);
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
   {
      return false;
   }
    
   TypedCometParam<VarMods> *pParam = static_cast<TypedCometParam<VarMods>*>(it->second);
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
   {
      return false;
   }

   TypedCometParam<DoubleRange> *pParam = static_cast<TypedCometParam<DoubleRange>*>(it->second);
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
   {
      return false;
   }    
    
   TypedCometParam<IntRange> *pParam = static_cast<TypedCometParam<IntRange>*>(it->second);
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
   {
      return false;
   }

   TypedCometParam<EnzymeInfo> *pParam = static_cast<TypedCometParam<EnzymeInfo>*>(it->second);
   value = pParam->GetValue();
   return true;
}

void CometSearchManager::DoSearch()
{
   InitializeStaticParams();

   PrintParameters();

   ValidateOutputFormat();

   ValidateSequenceDatabaseFile();

   ValidateScanRange();
   
   for (int i=0; i<(int)g_pvInputFiles.size(); i++)
   {
      UpdateInputFile(g_pvInputFiles.at(i));

      time_t tStartTime;
      time(&tStartTime);
      strftime(g_staticParams.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));

      if (!g_staticParams.options.bOutputSqtStream)
      {
         logout(" Comet version \"%s\"\n", comet_version);
         logout(" Search start:  %s\n", g_staticParams.szDate);
      }

      int iFirstScan = g_staticParams.inputFile.iFirstScan;             // First scan to search specified by user.
      int iLastScan = g_staticParams.inputFile.iLastScan;               // Last scan to search specified by user.
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
      FILE *fpout_pinxml=NULL;
      FILE *fpout_txt=NULL;
      FILE *fpoutd_txt=NULL;

      char szOutputSQT[SIZE_FILE];
      char szOutputDecoySQT[SIZE_FILE];
      char szOutputPepXML[SIZE_FILE];
      char szOutputDecoyPepXML[SIZE_FILE];
      char szOutputPinXML[SIZE_FILE];
      char szOutputTxt[SIZE_FILE];
      char szOutputDecoyTxt[SIZE_FILE];

      if (g_staticParams.options.bOutputSqtFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
#ifdef CRUX
            sprintf(szOutputSQT, "%s.target.sqt", g_staticParams.inputFile.szBaseName);        
#else
            sprintf(szOutputSQT, "%s.sqt", g_staticParams.inputFile.szBaseName);
#endif
         }
         else
         {
#ifdef CRUX
            sprintf(szOutputSQT, "%s.%d-%d.target.sqt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);
#else
            sprintf(szOutputSQT, "%s.%d-%d.sqt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);
#endif
         }

         if ((fpout_sqt = fopen(szOutputSQT, "w")) == NULL)
         {
            logerr("Error - cannot write to file \"%s\".\n\n", szOutputSQT);
            exit(1);
         }

         if (g_staticParams.options.iDecoySearch == 2)
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sprintf(szOutputDecoySQT, "%s.decoy.sqt", g_staticParams.inputFile.szBaseName);
            else
               sprintf(szOutputDecoySQT, "%s.%d-%d.decoy.sqt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

            if ((fpoutd_sqt = fopen(szOutputDecoySQT, "w")) == NULL)
            {
               logerr("Error - cannot write to decoy file \"%s\".\n\n", szOutputDecoySQT);
               exit(1);
            }
         }
      }

      if (g_staticParams.options.bOutputTxtFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
#ifdef CRUX
            sprintf(szOutputTxt, "%s.target.txt", g_staticParams.inputFile.szBaseName);
#else
            sprintf(szOutputTxt, "%s.txt", g_staticParams.inputFile.szBaseName);
#endif
         }
         else
         {
#ifdef CRUX
            sprintf(szOutputTxt, "%s.%d-%d.target.txt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);
#else
            sprintf(szOutputTxt, "%s.%d-%d.txt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);
#endif
         }

         if ((fpout_txt = fopen(szOutputTxt, "w")) == NULL)
         {
            logerr("Error - cannot write to file \"%s\".\n\n", szOutputTxt);
            exit(1);
         }

         if (g_staticParams.options.iDecoySearch == 2)
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sprintf(szOutputDecoyTxt, "%s.decoy.txt", g_staticParams.inputFile.szBaseName);
            else
               sprintf(szOutputDecoyTxt, "%s.%d-%d.decoy.txt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

            if ((fpoutd_txt= fopen(szOutputDecoyTxt, "w")) == NULL)
            {
               logerr("Error - cannot write to decoy file \"%s\".\n\n", szOutputDecoyTxt);
               exit(1);
            }
         }
      }

      if (g_staticParams.options.bOutputPepXMLFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
         {
#ifdef CRUX
            sprintf(szOutputPepXML, "%s.target.pep.xml", g_staticParams.inputFile.szBaseName);
#else
            sprintf(szOutputPepXML, "%s.pep.xml", g_staticParams.inputFile.szBaseName);
#endif
         }
         else
         {
#ifdef CRUX
            sprintf(szOutputPepXML, "%s.%d-%d.target.pep.xml", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);
#else
            sprintf(szOutputPepXML, "%s.%d-%d.pep.xml", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);
#endif
         }

         if ((fpout_pepxml = fopen(szOutputPepXML, "w")) == NULL)
         {
            logerr("Error - cannot write to file \"%s\".\n\n", szOutputPepXML);
            exit(1);
         }

         CometWritePepXML::WritePepXMLHeader(fpout_pepxml, *this);

         if (g_staticParams.options.iDecoySearch == 2)
         {
            if (iAnalysisType == AnalysisType_EntireFile)
               sprintf(szOutputDecoyPepXML, "%s.decoy.pep.xml", g_staticParams.inputFile.szBaseName);
            else
               sprintf(szOutputDecoyPepXML, "%s.%d-%d.decoy.pep.xml", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

            if ((fpoutd_pepxml = fopen(szOutputDecoyPepXML, "w")) == NULL)
            {
               logerr("Error - cannot write to decoy file \"%s\".\n\n", szOutputDecoyPepXML);
               exit(1);
            }

            CometWritePepXML::WritePepXMLHeader(fpoutd_pepxml, *this);
         }
      }

      if (g_staticParams.options.bOutputPinXMLFile)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
            sprintf(szOutputPinXML, "%s.pin.xml", g_staticParams.inputFile.szBaseName);
         else
            sprintf(szOutputPinXML, "%s.%d-%d.pin.xml", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

         if ((fpout_pinxml = fopen(szOutputPinXML, "w")) == NULL)
         {
            logerr("Error - cannot write to file \"%s\".\n\n", szOutputPinXML);
            exit(1);
         }

         // We need knowledge of max charge state in all searches
         // here in order to write the featureDescription header

         CometWritePinXML::WritePinXMLHeader(fpout_pinxml);
      }

      // For file access using MSToolkit.
      MSReader mstReader;

      // We want to read only MS2/MS3 scans.
      SetMSLevelFilter(mstReader);

      int iTotalSpectraSearched = 0;

      // We need to reset some of the static variables in-between input files 
      CometPreprocess::Reset();

      while (!CometPreprocess::DoneProcessingAllSpectra()) // Loop through iMaxSpectraPerSearch
      {
         // Load and preprocess all the spectra.
         if (!g_staticParams.options.bOutputSqtStream)
            logout(" - Load and process input spectra\n");

         CometPreprocess::LoadAndPreprocessSpectra(mstReader,
               iFirstScan, iLastScan, iAnalysisType,
               g_staticParams.options.iNumThreads,  // min # threads
               g_staticParams.options.iNumThreads); // max # threads

         if (g_pvQuery.empty())
            break; // no search to run
         else
            iTotalSpectraSearched += g_pvQuery.size();

         // Allocate memory to store results for each query spectrum.
         if (!g_staticParams.options.bOutputSqtStream)
            logout(" - Allocate memory to store results\n");

         AllocateResultsMem();

         if (!g_staticParams.options.bOutputSqtStream)
            logout(" - Number of mass-charge spectra loaded: %d\n", (int)g_pvQuery.size());

         // Sort g_pvQuery vector by dExpPepMass.
         std::sort(g_pvQuery.begin(), g_pvQuery.end(), compareByPeptideMass);

         g_massRange.dMinMass = g_pvQuery.at(0)->_pepMassInfo.dPeptideMassToleranceMinus;
         g_massRange.dMaxMass = g_pvQuery.at(g_pvQuery.size()-1)->_pepMassInfo.dPeptideMassTolerancePlus;

         // Now that spectra are loaded to memory and sorted, do search.
         CometSearch::RunSearch(g_staticParams.options.iNumThreads, g_staticParams.options.iNumThreads);

         // Sort each entry by xcorr, calculate E-values, etc.
         CometPostAnalysis::PostAnalysis(g_staticParams.options.iNumThreads, g_staticParams.options.iNumThreads);

         CalcRunTime(tStartTime);

         if (!g_staticParams.options.bOutputSqtStream)
            logout(" - Write output\n");

         if (g_staticParams.options.bOutputOutFiles)
            CometWriteOut::WriteOut();

         if (g_staticParams.options.bOutputPepXMLFile)
            CometWritePepXML::WritePepXML(fpout_pepxml, fpoutd_pepxml);

         if (g_staticParams.options.bOutputPinXMLFile)
            CometWritePinXML::WritePinXML(fpout_pinxml);

         if (g_staticParams.options.bOutputTxtFile)
            CometWriteTxt::WriteTxt(fpout_txt, fpoutd_txt);

         //// Write SQT last as I destroy the g_staticParams.szMod string during that process
         if (g_staticParams.options.bOutputSqtStream || g_staticParams.options.bOutputSqtFile)
            CometWriteSqt::WriteSqt(fpout_sqt, fpoutd_sqt, *this);

         // Deleting each Query object in the vector calls its destructor, which 
         // frees the spectral memory (see definition for Query in CometData.h).
         for (int i=0; i<(int)g_pvQuery.size(); i++)
            delete g_pvQuery.at(i);

         g_pvQuery.clear();
      }

      if (iTotalSpectraSearched == 0)
      {
         logout(" Warning - no spectra searched.\n\n");
      }

      if (!g_staticParams.options.bOutputSqtStream)
      {
         time(&tStartTime);
         strftime(g_staticParams.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));
         logout(" Search end:    %s\n\n", g_staticParams.szDate);
      }

      if (NULL != fpout_pepxml)
      {
         CometWritePepXML::WritePepXMLEndTags(fpout_pepxml);
         fclose(fpout_pepxml);
         fpout_pepxml = NULL;
      }

      if (NULL != fpoutd_pepxml)
      {
         CometWritePepXML::WritePepXMLEndTags(fpoutd_pepxml);
         fclose(fpoutd_pepxml);
         fpoutd_pepxml = NULL;
      }

      if (NULL != fpout_pinxml)
      {
         CometWritePinXML::WritePinXMLEndTags(fpout_pinxml);
         fclose(fpout_pinxml);
         fpout_pinxml = NULL;
      }

      if (NULL != fpout_sqt)
      {
         fclose(fpout_sqt);
         fpout_sqt = NULL;
      }

      if (NULL != fpoutd_sqt)
      {
         fclose(fpoutd_sqt);
         fpoutd_sqt = NULL;
      }
   }
}

