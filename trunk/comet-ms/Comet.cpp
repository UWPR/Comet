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


#include "Common.h"
#include "CometData.h"
#include "CometMassSpecUtils.h"
#include "CometSearch.h"
#include "CometPreprocess.h"
#include "CometPostAnalysis.h"
#include "CometWriteOut.h"
#include "CometWriteSqt.h"
#include "CometWritePepXML.h"
#include "Threading.h"
#include "ThreadPool.h"

#include <algorithm>


std::vector <Query *>   g_pvQuery;
StaticParams            g_StaticParams;
MassRange               g_MassRange;
Mutex                   g_pvQueryMutex;


void Usage(int failure,
           char *pszCmd);
void GetHostName();
void ProcessCmdLine(int argc, 
                    char *argv[], 
                    int *iFirstScan, 
                    int *iLastScan, 
                    int *iZLine, 
                    int *iScanCount, 
                    int *iAnalysisType,
                    char *szParamsFile);
void SetOptions(char *arg,
                char *szParamsFile,
                bool *bPrintParams);
void InitializeParameters();
void LoadParameters(char *pszParamsFile);
void AllocateResultsMem(void);
void CalcRunTime(time_t tStartTime);
void PrintParams();
void ParseCmdLine(char *cmd,
                  int *iFirst,
                  int *iLast,
                  int *Z,
                  int *iCount,
                  int *iType,
                  char *pszFileName);

// For MSToolkit and compressed file support.
MSFileFormat GetMstFileType(char* c);


bool compareByPeptideMass(Query const* a, Query const* b)
{
   return (a->_pepMassInfo.dExpPepMass < b->_pepMassInfo.dExpPepMass);
}


int main(int argc, char *argv[])
{
   int iZLine = 0;
   int iFirstScan = 0;             // First scan to search specified by user.
   int iLastScan = 0;              // Last scan to search specified by user.
   int iScanCount = 0;
   int iAnalysisType = AnalysisType_Unknown; // 1=dta (retired),
                                             // 2=specific scan,
                                             // 3=specific scan + charge,
                                             // 4=scan range,
                                             // 5=start scan + count,
                                             // 6=entire file
   char szParamsFile[SIZE_FILE];

   if (argc < 2) 
       Usage(0, argv[0]);

   time_t tStartTime;
   time(&tStartTime);
   strftime(g_StaticParams._dtInfoStart.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));

   GetHostName();

   // Process command line and read comet.params here.
   ProcessCmdLine(argc, argv, &iFirstScan, &iLastScan, &iZLine,
         &iScanCount, &iAnalysisType, szParamsFile);

   if (!g_StaticParams.options.bOutputSqtStream
         && !g_StaticParams.options.bOutputSqtFile
         && !g_StaticParams.options.bOutputPepXMLFile
         && !g_StaticParams.options.bOutputOutFiles)
   {
      printf(" Comet version \"%s\"\n", version);
      printf(" Please specify at least one output format.\n\n");
      exit(1);
   }

   if (!g_StaticParams.options.bOutputSqtStream)
   {
      printf(" Comet version \"%s\"\n", version);
      printf(" Search start:  %s\n", g_StaticParams._dtInfoStart.szDate);
   }

   // For SQT & pepXML output file, check if they can be written to before doing anything else.
   FILE *fpout_sqt=NULL;
   FILE *fpoutd_sqt=NULL;
   FILE *fpout_pepxml=NULL;
   FILE *fpoutd_pepxml=NULL;

   char szOutputSQT[SIZE_FILE];
   char szOutputDecoySQT[SIZE_FILE];
   char szOutputPepXML[SIZE_FILE];
   char szOutputDecoyPepXML[SIZE_FILE];

   // If # threads not specified, poll system to get # threads to launch.
   if (g_StaticParams.options.iNumThreads == 0)
   {
#ifdef _WIN32
      SYSTEM_INFO sysinfo;
      GetSystemInfo( &sysinfo );
      g_StaticParams.options.iNumThreads = sysinfo.dwNumberOfProcessors;
#else
      g_StaticParams.options.iNumThreads = sysconf( _SC_NPROCESSORS_ONLN );
#endif
      if (g_StaticParams.options.iNumThreads < 1 || g_StaticParams.options.iNumThreads > MAX_THREADS)
         g_StaticParams.options.iNumThreads = 2;  // Default to 2 threads.
   }

   // Initialize the mutexes we'll use to protect global data.
   Threading::CreateMutex(&g_pvQueryMutex);
   
   // Load and preprocess all the spectra.
   if (!g_StaticParams.options.bOutputSqtStream)
      printf(" Load and process input spectra\n");

   CometPreprocess::LoadAndPreprocessSpectra(iZLine, 
         iFirstScan, iLastScan, iScanCount, iAnalysisType,
         g_StaticParams.options.iNumThreads,  g_StaticParams.options.iNumThreads);

   if (g_StaticParams.options.bOutputSqtFile)
   {
      if (iAnalysisType == AnalysisType_EntireFile)
         sprintf(szOutputSQT, "%s.sqt", g_StaticParams.inputFile.szBaseName);
      else
         sprintf(szOutputSQT, "%s.sqt:%d-%d", g_StaticParams.inputFile.szBaseName, iFirstScan, iLastScan);

      if ((fpout_sqt = fopen(szOutputSQT, "w")) == NULL)
      {
         fprintf(stderr, "Error - cannot write to file %s\n\n", szOutputSQT);
         exit(1);
      }

      if (g_StaticParams.options.iDecoySearch == 2)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
            sprintf(szOutputDecoySQT, "%s.decoy.sqt", g_StaticParams.inputFile.szBaseName);
         else
            sprintf(szOutputDecoySQT, "%s.decoy.sqt:%d-%d", g_StaticParams.inputFile.szBaseName, iFirstScan, iLastScan);

         if ((fpoutd_sqt = fopen(szOutputDecoySQT, "w")) == NULL)
         {
            fprintf(stderr, "Error - cannot write to decoy file %s\n\n", szOutputDecoySQT);
            exit(1);
         }
      }
   }

   if (g_StaticParams.options.bOutputPepXMLFile)
   {
      if (iAnalysisType == AnalysisType_EntireFile)
         sprintf(szOutputPepXML, "%s.pep.xml", g_StaticParams.inputFile.szBaseName);
      else
         sprintf(szOutputPepXML, "%s.pep.xml:%d-%d", g_StaticParams.inputFile.szBaseName, iFirstScan, iLastScan);

      if ((fpout_pepxml = fopen(szOutputPepXML, "w")) == NULL)
      {
         fprintf(stderr, "Error - cannot write to file %s\n\n", szOutputPepXML);
         exit(1);
      }

      if (g_StaticParams.options.iDecoySearch == 2)
      {
         if (iAnalysisType == AnalysisType_EntireFile)
            sprintf(szOutputDecoyPepXML, "%s.decoy.pep.xml", g_StaticParams.inputFile.szBaseName);
         else
            sprintf(szOutputDecoyPepXML, "%s.decoy.pep.xml:%d-%d", g_StaticParams.inputFile.szBaseName, iFirstScan, iLastScan);

         if ((fpoutd_pepxml = fopen(szOutputDecoyPepXML, "w")) == NULL)
         {
            fprintf(stderr, "Error - cannot write to decoy file %s\n\n", szOutputDecoyPepXML);
            exit(1);
         }
      }
   }

   // Allocate memory to store results for each query spectrum.
   if (!g_StaticParams.options.bOutputSqtStream)
      printf(" Allocate memory to store results\n");

   AllocateResultsMem();

   if (g_pvQuery.empty())
   {
      printf(" Warning - no searches to run.\n\n");
      exit(1);
   }

   if (!g_StaticParams.options.bOutputSqtStream)
      printf(" Number of mass-charge spectra loaded: %d\n", (int)g_pvQuery.size());

   // Sort g_pvQuery vector by dExpPepMass.
   std::sort(g_pvQuery.begin(), g_pvQuery.end(), compareByPeptideMass);

   g_MassRange.dMinMass = g_pvQuery.at(0)->_pepMassInfo.dPeptideMassToleranceMinus;
   g_MassRange.dMaxMass = g_pvQuery.at(g_pvQuery.size()-1)->_pepMassInfo.dPeptideMassTolerancePlus;

   // Now that spectra are loaded to memory and sorted, do search.
   CometSearch::RunSearch(g_StaticParams.options.iNumThreads, g_StaticParams.options.iNumThreads);

   // Sort each entry by xcorr, calculate E-values, etc.
   CometPostAnalysis::PostAnalysis(g_StaticParams.options.iNumThreads, g_StaticParams.options.iNumThreads);
   
   CalcRunTime(tStartTime);

   if (!g_StaticParams.options.bOutputSqtStream)
      printf(" Write output\n");

   if (g_StaticParams.options.bOutputOutFiles)
   {
      CometWriteOut::WriteOut();
   }

   if (g_StaticParams.options.bOutputPepXMLFile)
   {
      CometWritePepXML::WritePepXML(fpout_pepxml, fpoutd_pepxml, szOutputPepXML, szOutputDecoyPepXML, szParamsFile);
   }

   // Write SQT last as I destroy the g_StaticParams.szMod string during that process
   if (g_StaticParams.options.bOutputSqtStream || g_StaticParams.options.bOutputSqtFile)
   {
      CometWriteSqt::WriteSqt(fpout_sqt, fpoutd_sqt, szOutputSQT, szOutputDecoySQT, szParamsFile);
   }


   // Deleting each Query object in the vector calls its destructor, which 
   // frees the spectral memory (see definition for Query in CometData.h).
   for (int i=0; i<(int)g_pvQuery.size(); i++)
      delete g_pvQuery.at(i);

   g_pvQuery.clear();
   
   // Destroy the mutex we used to protect g_pvQuery.
   Threading::DestroyMutex(g_pvQueryMutex);
 
   if (!g_StaticParams.options.bOutputSqtStream)
   {
      time(&tStartTime);
      strftime(g_StaticParams._dtInfoStart.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));
      printf(" Search end:    %s\n\n", g_StaticParams._dtInfoStart.szDate);
   }

   return (0);

} // main


// Allocate memory for the _pResults struct for each g_pvQuery entry.
void AllocateResultsMem(void)
{

   for (unsigned i=0; i<g_pvQuery.size(); i++)
   {
      Query* pQuery = g_pvQuery.at(i);

      pQuery->_pResults = (struct Results *)malloc(sizeof(struct Results) * g_StaticParams.options.iNumStored);

      if (pQuery->_pResults == NULL)
      {
         fprintf(stderr, " Error malloc(_pResults[])\n");
         exit(1);
      }
	  
	  //MH: Initializing iLenPeptide to 0 is necessary to silence Valgrind Errors. Claims it is possible to
      //make conditional jump or move on this uninitialized value in CometSearch.cpp:1343
      for(int xx=0;xx<g_StaticParams.options.iNumStored;xx++)
        pQuery->_pResults[xx].iLenPeptide=0;

      pQuery->iDoXcorrCount = 0;
      pQuery->siLowestSpScoreIndex = 0;
      pQuery->fLowestSpScore = 0.0;

      if (g_StaticParams.options.iDecoySearch==2)
      {
         pQuery->_pDecoys = (struct Results *)malloc(sizeof(struct Results) * g_StaticParams.options.iNumStored);

         if (pQuery->_pDecoys == NULL)
         {
            fprintf(stderr, " Error malloc(_pDecoys[])\n");
            exit(1);
         }
		 
		 //MH: same logic as my comment above
         for(int x=0;x<g_StaticParams.options.iNumStored;x++)
          pQuery->_pDecoys[x].iLenPeptide=0;

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

      for (j=0; j<g_StaticParams.options.iNumStored; j++)
      {
         pQuery->_pResults[j].fXcorr = 0.0;
         pQuery->_pResults[j].fScoreSp = 0.0;
         pQuery->_pResults[j].szPeptide[0] = '\0';
         pQuery->_pResults[j].szProtein[0] = '\0';

         if (g_StaticParams.options.iDecoySearch==2)
         {
            pQuery->_pDecoys[j].fXcorr = 0.0;
            pQuery->_pDecoys[j].fScoreSp = 0.0;
            pQuery->_pDecoys[j].szPeptide[0] = '\0';
            pQuery->_pDecoys[j].szProtein[0] = '\0';
         }
      }
   }
}


void Usage(int failure, char *pszCmd)
{
   printf("\n");
   printf(" Comet version \"%s\"\n %s\n", version, copyright);
   printf("\n");
   printf(" Comet usage:  %s [options] <input_files>[:range]\n", pszCmd);
   printf("\n");
   printf(" Supported input formats include mzXML, mzXML, mz5 and ms2 variants (cms2, bms2, ms2)\n");
   printf(" The optional [:range] parameter specifies a scan range to search.\n");
   printf("\n");
   printf("       options:  -p         to print out a comet.params file (named comet.params.new)\n");
   printf("                 -P<params> to specify an alternate parameters file (default comet.params)\n");
   printf("                 -N<name>   to specify an alternate output base name\n");
   printf("                 -D<dbase>  to specify a sequence database, overriding entry in parameters file\n");
   printf("\n");
   printf("       example:  %s run1.mzXML\n", pszCmd);
   printf("            or   %s run1.mzXML:1000-1500    <- to search a subset scan range\n", pszCmd);
   printf("            or   %s run1.cms2:1000-1500\n", pszCmd);
   printf("\n");

   exit(1);
}


void GetHostName(void)
{
#ifdef _WIN32
   WSADATA WSAData;
   WSAStartup(MAKEWORD(1, 0), &WSAData);

   if (gethostname(g_StaticParams.szHostName, SIZE_FILE) != 0)
      strcpy(g_StaticParams.szHostName, "locahost");

   WSACleanup();
#else
   if (gethostname(g_StaticParams.szHostName, SIZE_FILE) != 0)
      strcpy(g_StaticParams.szHostName, "locahost");
#endif

   char *pStr;
   if ((pStr = strchr(g_StaticParams.szHostName, '.'))!=NULL)
      *pStr = '\0';
}


void ProcessCmdLine(int argc, 
                    char *argv[], 
                    int *iFirstScan, 
                    int *iLastScan, 
                    int *iZLine, 
                    int *iScanCount, 
                    int *iAnalysisType,
                    char *szParamsFile)
{
   bool bPrintParams = false;
   int iStartInputFile = 1;
   char *arg;
   FILE *fpcheck;

   strcpy(szParamsFile, "comet.params");

   g_StaticParams.databaseInfo.szDatabase[0] = '\0';
   g_StaticParams.inputFile.szBaseName[0] = '\0';

   arg = argv[iStartInputFile];

   while (iStartInputFile < argc)
   {
      if (arg[0] == '-')
         SetOptions(arg, szParamsFile, &bPrintParams);
      else
         break;

      arg = argv[++iStartInputFile];
   }

   if (bPrintParams)
   {
      PrintParams();
      exit(0);
   }

   if (iStartInputFile == argc)
   {
      printf("\n");
      printf(" Comet version %s\n %s\n", version, copyright);
      printf("\n");
      printf(" Error - nothing to do.\n\n");
      exit(1);
   }

   InitializeParameters();

   // Loads search parameters from comet.params file.
   LoadParameters(szParamsFile);

   // Quick sanity check to make sure sequence db file is present before spending
   // time reading & processing spectra and then reporting this error.
   if ((fpcheck=fopen(g_StaticParams.databaseInfo.szDatabase, "r")) == NULL)
   {
      fprintf(stderr, "\n Error - cannot read database file %s.\n\n", g_StaticParams.databaseInfo.szDatabase);
      exit(1);
   }
   fclose(fpcheck);

   if (!g_StaticParams.options.bOutputOutFiles)
   {
      g_StaticParams.options.bSkipAlreadyDone = 0;
   }

   //-->MH
   //Parse file line.
   char szFName[SIZE_FILE];
   ParseCmdLine(argv[iStartInputFile], 
             iFirstScan, 
             iLastScan, 
             iZLine, 
             iScanCount, 
             iAnalysisType, 
             szFName);

   strcpy(g_StaticParams.inputFile.szFileName, szFName);

   if ((fpcheck = fopen(g_StaticParams.inputFile.szFileName, "r")) == NULL)
   {
      fprintf(stderr, "\n Error - cannot open input file %s.\n\n", g_StaticParams.inputFile.szFileName);
      exit(1);
   }
   fclose(fpcheck);

   if (!strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-6, ".mzXML")
         || !strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-5, ".mzML")
         || !strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-4, ".mz5")
         || !strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-9, ".mzXML.gz")
         || !strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-8, ".mzML.gz"))
   {
      g_StaticParams.inputFile.iInputType = InputType_MZXML;
   }

   if (g_StaticParams.inputFile.szBaseName[0]=='\0')  // Make sure not set on command line.
   {
      char *pStr;

      strcpy(g_StaticParams.inputFile.szBaseName, g_StaticParams.inputFile.szFileName);

      if ( (pStr = strrchr(g_StaticParams.inputFile.szBaseName, '.')))
         *pStr = '\0';

      if (!strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-9, ".mzXML.gz")
         || !strcmp(argv[iStartInputFile] + strlen(argv[iStartInputFile])-8, ".mzML.gz"))
      {
         if ( (pStr = strrchr(g_StaticParams.inputFile.szBaseName, '.')))
            *pStr = '\0';
      }
   }

   // Create .out directory.
   if (g_StaticParams.options.bOutputOutFiles)
   {
#ifdef _WIN32
      if (_mkdir(g_StaticParams.inputFile.szBaseName) == -1)
      {
         errno_t err;
         _get_errno(&err);

         if (err != EEXIST) 
         {
            fprintf(stderr, "\n Error - could not create directory %s.\n", g_StaticParams.inputFile.szBaseName);
            exit(1);
         }
      }
      if (g_StaticParams.options.iDecoySearch == 2)
      {
         char szDecoyDir[SIZE_FILE];
         sprintf(szDecoyDir, "%s_decoy", g_StaticParams.inputFile.szBaseName);

         if (_mkdir(szDecoyDir) == -1)
         {
            errno_t err;
            _get_errno(&err);

            if (err != EEXIST) 
            {
               fprintf(stderr, "\n Error - could not create directory %s.\n", szDecoyDir);
               exit(1);
            }
         }
      }
#else
      if ((mkdir(g_StaticParams.inputFile.szBaseName, 0775) == -1) && (errno != EEXIST))
      {
         fprintf(stderr, "\n Error - could not create directory %s.\n", g_StaticParams.inputFile.szBaseName);
         exit(1);
      }
      if (g_StaticParams.options.iDecoySearch == 2)
      {
         char szDecoyDir[SIZE_FILE];
         sprintf(szDecoyDir, "%s_decoy", g_StaticParams.inputFile.szBaseName);

         if ((mkdir(szDecoyDir , 0775) == -1) && (errno != EEXIST))
         {
            fprintf(stderr, "\n Error - could not create directory %s.\n\n", szDecoyDir);
            exit(1);
         }
      }
#endif
   }

   g_StaticParams.precalcMasses.dNtermProton = g_StaticParams.staticModifications.dAddNterminusPeptide
      + PROTON_MASS;

   g_StaticParams.precalcMasses.dCtermOH2Proton = g_StaticParams.staticModifications.dAddCterminusPeptide
      + g_StaticParams.massUtility.dOH2fragment
      + PROTON_MASS;

   g_StaticParams.precalcMasses.dOH2ProtonCtermNterm = g_StaticParams.massUtility.dOH2parent
      + PROTON_MASS
      + g_StaticParams.staticModifications.dAddCterminusPeptide
      + g_StaticParams.staticModifications.dAddNterminusPeptide;
}


void SetOptions(char *arg,
      char *szParamsFile,
      bool *bPrintParams)
{
   char szTmp[512];

   switch (arg[1])
   {
      case 'D':   // Alternate sequence database.
         if (sscanf(arg+2, "%s", szTmp) == 0)
            fprintf(stderr, "Cannot read command line database: '%s'.  Ignored.\n", szTmp);
         else
            strcpy(g_StaticParams.databaseInfo.szDatabase, szTmp);
         break;
      case 'P':   // Alternate parameters file.
         if (sscanf(arg+2, "%s", szTmp) == 0 )
            fprintf(stderr, "Missing text for parameter option -P<params>.  Ignored.\n");
         else
            strcpy(szParamsFile, szTmp);
         break;
      case 'N':   // Set basename of output file (for .out, SQT, and pepXML)
         if (sscanf(arg+2, "%s", szTmp) == 0 )
            fprintf(stderr, "Missing text for parameter option -N<basename>.  Ignored.\n");
         else
         {
            strcpy(g_StaticParams.inputFile.szBaseName, szTmp);
            printf("basename %s\n", g_StaticParams.inputFile.szBaseName);
         }
         break;
      case 'p':
         *bPrintParams = true;
         break;
      default:
         break;
   }
   arg[0] = '\0';
}


void InitializeParameters()
{
   int i = 0;

   g_StaticParams.inputFile.iInputType = InputType_MS2;

   g_StaticParams.szMod[0] = '\0';

   for (i=0; i<128; i++)
   {
      g_StaticParams.massUtility.pdAAMassParent[i] = 999999.;
      g_StaticParams.massUtility.pdAAMassFragment[i] = 999999.;
   }

   g_StaticParams.enzymeInformation.iAllowedMissedCleavage = 2;

   for (i=0; i<VMODS; i++)
   {
      g_StaticParams.variableModParameters.varModList[i].iMaxNumVarModAAPerMod = 4;
      g_StaticParams.variableModParameters.varModList[i].bBinaryMod = 0;
      g_StaticParams.variableModParameters.varModList[i].dVarModMass = 0.0;
      g_StaticParams.variableModParameters.varModList[i].szVarModChar[0] = '\0';
   }

   g_StaticParams.variableModParameters.cModCode[0] = '*';
   g_StaticParams.variableModParameters.cModCode[1] = '#';
   g_StaticParams.variableModParameters.cModCode[2] = '@';
   g_StaticParams.variableModParameters.cModCode[3] = '^';
   g_StaticParams.variableModParameters.cModCode[4] = '~';
   g_StaticParams.variableModParameters.cModCode[5] = '$';

   g_StaticParams.variableModParameters.iMaxVarModPerPeptide = 10;
   g_StaticParams.variableModParameters.iVarModNtermDistance = -1;
   g_StaticParams.variableModParameters.iVarModCtermDistance = -1;

   g_StaticParams.ionInformation.iTheoreticalFragmentIons = 0;
   g_StaticParams.options.iNumPeptideOutputLines = 1;
   g_StaticParams.options.iWhichReadingFrame = 0;
   g_StaticParams.options.iEnzymeTermini = 2;
   g_StaticParams.options.iNumStored = NUM_STORED;                  // # of search results to store for xcorr analysis.

   g_StaticParams.options.bNoEnzymeSelected = 1;
   g_StaticParams.options.bPrintFragIons = 0;
   g_StaticParams.options.bPrintExpectScore = 0;
   g_StaticParams.options.iRemovePrecursor = 0;
   g_StaticParams.options.dRemovePrecursorTol = DEFAULT_PREC_TOL;  

   g_StaticParams.options.bOutputSqtStream = 0;
   g_StaticParams.options.bOutputSqtFile = 0;
   g_StaticParams.options.bOutputPepXMLFile = 1;
   g_StaticParams.options.bOutputOutFiles = 0;

   g_StaticParams.options.bSkipAlreadyDone = 0;
   g_StaticParams.options.iDecoySearch = 0;
   g_StaticParams.options.iNumThreads = 0;
   g_StaticParams.options.bClipNtermMet = 0;

   // These parameters affect mzXML/RAMP spectra only.
   g_StaticParams.options.iStartScan = 0;
   g_StaticParams.options.iEndScan = 0;
   g_StaticParams.options.iMinPeaks = MINIMUM_PEAKS;
   g_StaticParams.options.iStartCharge = 0;
   g_StaticParams.options.iMaxFragmentCharge = 3;
   g_StaticParams.options.iMaxPrecursorCharge = 6;
   g_StaticParams.options.iEndCharge = 0;
   g_StaticParams.options.iStartMSLevel = 2;
   g_StaticParams.options.iEndMSLevel = 0;
   g_StaticParams.options.iMinIntensity = 0;
   g_StaticParams.options.dLowPeptideMass = 0.0;
   g_StaticParams.options.dHighPeptideMass = 0.0;
   strcpy(g_StaticParams.options.szActivationMethod, "ALL");
   // End of mzXML specific parameters.

   g_StaticParams.staticModifications.dAddCterminusPeptide = 0.0;
   g_StaticParams.staticModifications.dAddNterminusPeptide = 0.0;
   g_StaticParams.staticModifications.dAddCterminusProtein = 0.0;
   g_StaticParams.staticModifications.dAddNterminusProtein = 0.0;

   g_StaticParams.tolerances.iMassToleranceUnits = 0;
   g_StaticParams.tolerances.iMassToleranceType = 0;
   g_StaticParams.tolerances.iIsotopeError = 0;
   g_StaticParams.tolerances.dInputTolerance = 1.0;
   g_StaticParams.tolerances.dFragmentBinSize = DEFAULT_BIN_WIDTH;
   g_StaticParams.tolerances.dFragmentBinStartOffset = DEFAULT_OFFSET;
   g_StaticParams.tolerances.dMatchPeakTolerance = 0.5;
}


// Reads comet.params parameter file.
void LoadParameters(char *pszParamsFile)
{
   double dTempMass;
   int   i,
         iSearchEnzymeNumber,
         iSampleEnzymeNumber;
   char  szParamBuf[SIZE_BUF],
         szParamName[128],
         szParamVal[128];
   FILE  *fp;
   bool  bCurrentParamsFile = 0; // Track a parameter to make sure present.
   char *pStr;

   for (i=0; i<128; i++)
      g_StaticParams.staticModifications.pdStaticMods[i] = 0.0;

   if ((fp=fopen(pszParamsFile, "r")) == NULL)
   {
      fprintf(stderr, " Error - cannot open parameter file %s.\n\n", pszParamsFile);
      exit(1);
   }

   while (!feof(fp))
   {
      fgets(szParamBuf, SIZE_BUF, fp);

      if (!strncmp(szParamBuf, "[COMET_ENZYME_INFO]", 19))
         break;

      if (! (szParamBuf[0]=='#' || (pStr = strchr(szParamBuf, '='))==NULL))
      {
         strcpy(szParamVal, pStr + 1);  // Copy over value.
         *pStr = 0;                     // Null rest of szParamName at equal char.

         sscanf(szParamBuf, "%s", szParamName);

         if (!strcmp(szParamName, "database_name"))
         {
            sscanf(szParamVal, "%s", g_StaticParams.databaseInfo.szDatabase);
         }
         else if (!strcmp(szParamName, "nucleotide_reading_frame"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iWhichReadingFrame));
         }
         else if (!strcmp(szParamName, "mass_type_parent"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.massUtility.bMonoMassesParent);
         }
         else if (!strcmp(szParamName, "mass_type_fragment"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.massUtility.bMonoMassesFragment);
         }
         else if (!strcmp(szParamName, "show_fragment_ions"))
         {
            sscanf(szParamVal, "%d",  &(g_StaticParams.options.bPrintFragIons));
         }
         else if (!strcmp(szParamName, "num_threads"))
         {
            sscanf(szParamVal, "%d",  &(g_StaticParams.options.iNumThreads));
         }
         else if (!strcmp(szParamName, "clip_nterm_methionine"))
         {
            sscanf(szParamVal, "%d",  &(g_StaticParams.options.bClipNtermMet));
         }
         else if (!strcmp(szParamName, "theoretical_fragment_ions"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.ionInformation.iTheoreticalFragmentIons);
            if ((g_StaticParams.ionInformation.iTheoreticalFragmentIons < 0) || 
                (g_StaticParams.ionInformation.iTheoreticalFragmentIons > 1))
            {
               g_StaticParams.ionInformation.iTheoreticalFragmentIons = 0;
            }
         }
         else if (!strcmp(szParamName, "use_A_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.iIonVal[0]));
         }
         else if (!strcmp(szParamName, "use_B_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.iIonVal[1]));
         }
         else if (!strcmp(szParamName, "use_C_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.iIonVal[2]));
         }
         else if (!strcmp(szParamName, "use_X_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.iIonVal[3]));
         }
         else if (!strcmp(szParamName, "use_Y_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.iIonVal[4]));
         }
         else if (!strcmp(szParamName, "use_Z_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.iIonVal[5]));
         }
         else if (!strcmp(szParamName, "use_NL_ions"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.ionInformation.bUseNeutralLoss));
         }
         else if (!strcmp(szParamName, "variable_mod1"))
         {
            sscanf(szParamVal, "%lf %s %d %d",
                  &g_StaticParams.variableModParameters.varModList[VMOD_1_INDEX].dVarModMass,
                  g_StaticParams.variableModParameters.varModList[VMOD_1_INDEX].szVarModChar,
                  &g_StaticParams.variableModParameters.varModList[VMOD_1_INDEX].bBinaryMod,
                  &g_StaticParams.variableModParameters.varModList[VMOD_1_INDEX].iMaxNumVarModAAPerMod);
         }
         else if (!strcmp(szParamName, "variable_mod2"))
         {
            sscanf(szParamVal, "%lf %s %d %d",
                  &g_StaticParams.variableModParameters.varModList[VMOD_2_INDEX].dVarModMass,
                  g_StaticParams.variableModParameters.varModList[VMOD_2_INDEX].szVarModChar,
                  &g_StaticParams.variableModParameters.varModList[VMOD_2_INDEX].bBinaryMod,
                  &g_StaticParams.variableModParameters.varModList[VMOD_2_INDEX].iMaxNumVarModAAPerMod);
         }
         else if (!strcmp(szParamName, "variable_mod3"))
         {
            sscanf(szParamVal, "%lf %s %d %d",
                  &g_StaticParams.variableModParameters.varModList[VMOD_3_INDEX].dVarModMass,
                  g_StaticParams.variableModParameters.varModList[VMOD_3_INDEX].szVarModChar,
                  &g_StaticParams.variableModParameters.varModList[VMOD_3_INDEX].bBinaryMod,
                  &g_StaticParams.variableModParameters.varModList[VMOD_3_INDEX].iMaxNumVarModAAPerMod);
         }
         else if (!strcmp(szParamName, "variable_mod4"))
         {
            sscanf(szParamVal, "%lf %s %d %d",
                  &g_StaticParams.variableModParameters.varModList[VMOD_4_INDEX].dVarModMass,
                  g_StaticParams.variableModParameters.varModList[VMOD_4_INDEX].szVarModChar,
                  &g_StaticParams.variableModParameters.varModList[VMOD_4_INDEX].bBinaryMod,
                  &g_StaticParams.variableModParameters.varModList[VMOD_4_INDEX].iMaxNumVarModAAPerMod);
         }
         else if (!strcmp(szParamName, "variable_mod5"))
         {
            sscanf(szParamVal, "%lf %s %d %d",
                  &g_StaticParams.variableModParameters.varModList[VMOD_5_INDEX].dVarModMass,
                  g_StaticParams.variableModParameters.varModList[VMOD_5_INDEX].szVarModChar,
                  &g_StaticParams.variableModParameters.varModList[VMOD_5_INDEX].bBinaryMod,
                  &g_StaticParams.variableModParameters.varModList[VMOD_5_INDEX].iMaxNumVarModAAPerMod);
         }
         else if (!strcmp(szParamName, "variable_mod6"))
         {
            sscanf(szParamVal, "%lf %s %d %d",
                  &g_StaticParams.variableModParameters.varModList[VMOD_6_INDEX].dVarModMass,
                  g_StaticParams.variableModParameters.varModList[VMOD_6_INDEX].szVarModChar,
                  &g_StaticParams.variableModParameters.varModList[VMOD_6_INDEX].bBinaryMod,
                  &g_StaticParams.variableModParameters.varModList[VMOD_6_INDEX].iMaxNumVarModAAPerMod);
         }
         else if (!strcmp(szParamName, "max_variable_mods_in_peptide"))
         {
            int iTmp = 0;
            sscanf(szParamVal, "%d", &iTmp);

            if (iTmp > 0)
               g_StaticParams.variableModParameters.iMaxVarModPerPeptide = iTmp;
         }
         else if (!strcmp(szParamName, "fragment_bin_tol"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.tolerances.dFragmentBinSize);
            if (g_StaticParams.tolerances.dFragmentBinSize < 0.01)
               g_StaticParams.tolerances.dFragmentBinSize = 0.01;
         }
         else if (!strcmp(szParamName, "fragment_bin_offset"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.tolerances.dFragmentBinStartOffset);
         }
         else if (!strcmp(szParamName, "peptide_mass_tolerance"))
         {
            sscanf(szParamVal, "%lf",  &g_StaticParams.tolerances.dInputTolerance);
         }
         else if (!strcmp(szParamName, "precursor_tolerance_type"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.tolerances.iMassToleranceType);
            if ((g_StaticParams.tolerances.iMassToleranceType < 0) || 
                (g_StaticParams.tolerances.iMassToleranceType > 1))
            {
                g_StaticParams.tolerances.iMassToleranceType = 0;
            }
         }
         else if (!strcmp(szParamName, "peptide_mass_units"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.tolerances.iMassToleranceUnits);
            if ((g_StaticParams.tolerances.iMassToleranceUnits < 0) || 
                (g_StaticParams.tolerances.iMassToleranceUnits > 2))
            {
                g_StaticParams.tolerances.iMassToleranceUnits = 0;  // 0=amu, 1=mmu, 2=ppm
            }
            bCurrentParamsFile = 1;
         }
         else if (!strcmp(szParamName, "isotope_error"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.tolerances.iIsotopeError);
            if ((g_StaticParams.tolerances.iIsotopeError < 0) || 
                (g_StaticParams.tolerances.iIsotopeError > 2))
            {
                g_StaticParams.tolerances.iIsotopeError = 0;
            }
         }
         else if (!strcmp(szParamName, "num_output_lines"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iNumPeptideOutputLines));
         }
         else if (!strcmp(szParamName, "num_results"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iNumStored));
         }
         else if (!strcmp(szParamName, "remove_precursor_peak"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iRemovePrecursor));
         }
         else if (!strcmp(szParamName, "remove_precursor_tolerance"))
         {
            sscanf(szParamVal, "%lf", &(g_StaticParams.options.dRemovePrecursorTol));
         }
         else if (!strcmp(szParamName, "print_expect_score"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.bPrintExpectScore));
         }
         else if (!strcmp(szParamName, "output_sqtstream"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.bOutputSqtStream));
         }
         else if (!strcmp(szParamName, "output_sqtfile"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.bOutputSqtFile));
         }
         else if (!strcmp(szParamName, "output_pepxmlfile"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.bOutputPepXMLFile));
         }
         else if (!strcmp(szParamName, "output_outfiles"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.bOutputOutFiles));
         }
         else if (!strcmp(szParamName, "skip_researching"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.bSkipAlreadyDone));
         }
         else if (!strcmp(szParamName, "variable_C_terminus"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.variableModParameters.dVarModMassC);
         }
         else if (!strcmp(szParamName, "variable_N_terminus"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.variableModParameters.dVarModMassN);
         }
         else if (!strcmp(szParamName, "variable_C_terminus_distance"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.variableModParameters.iVarModCtermDistance);
         }
         else if (!strcmp(szParamName, "variable_N_terminus_distance"))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.variableModParameters.iVarModNtermDistance);
         }
         else if (!strcmp(szParamName, "add_Cterm_peptide"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.staticModifications.dAddCterminusPeptide);
         }
         else if (!strcmp(szParamName, "add_Nterm_peptide"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.staticModifications.dAddNterminusPeptide);
         }
         else if (!strcmp(szParamName, "add_Cterm_protein"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.staticModifications.dAddCterminusProtein);
         }
         else if (!strcmp(szParamName, "add_Nterm_protein"))
         {
            sscanf(szParamVal, "%lf", &g_StaticParams.staticModifications.dAddNterminusProtein);
         }
         else if (!strcmp(szParamName, "add_G_glycine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['G'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_A_alanine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['A'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_S_serine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['S'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_P_proline"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['P'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_V_valine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['V'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_T_threonine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['T'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_C_cysteine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['C'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_L_leucine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['L'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_I_isoleucine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['I'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_N_asparagine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['N'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_O_ornithine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['O'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_D_aspartic_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['D'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_Q_glutamine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['Q'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_K_lysine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['K'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_E_glutamic_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['E'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_M_methionine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['M'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_H_histidine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['H'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_F_phenylalanine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['F'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_R_arginine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['R'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_Y_tyrosine"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['Y'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_W_tryptophan"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['W'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_B_user_amino_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['B'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_J_user_amino_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['J'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_U_user_amino_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['U'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_X_user_amino_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['X'] = dTempMass;
         }
         else if (!strcmp(szParamName, "add_Z_user_amino_acid"))
         {
            sscanf(szParamVal, "%lf", &dTempMass);
            g_StaticParams.staticModifications.pdStaticMods['Z'] = dTempMass;
         }
         else if (!strcmp(szParamName, "search_enzyme_number"))
         {
            sscanf(szParamVal, "%d", &iSearchEnzymeNumber);
         }
         else if (!strcmp(szParamName, "sample_enzyme_number"))
         {
            sscanf(szParamVal, "%d", &iSampleEnzymeNumber);
         }
         else if (!strcmp(szParamName, "num_enzyme_termini"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iEnzymeTermini));
            if ((g_StaticParams.options.iEnzymeTermini != 1) && 
                (g_StaticParams.options.iEnzymeTermini != 8) && 
                (g_StaticParams.options.iEnzymeTermini != 9))
            {
               g_StaticParams.options.iEnzymeTermini = 2;
            }
         }
         else if (!strcmp(szParamName, "allowed_missed_cleavage ="))
         {
            sscanf(szParamVal, "%d", &g_StaticParams.enzymeInformation.iAllowedMissedCleavage);
            if (g_StaticParams.enzymeInformation.iAllowedMissedCleavage < 0)
            {
               g_StaticParams.enzymeInformation.iAllowedMissedCleavage = 0;
            }
         }
         else if (!strcmp(szParamName, "scan_range"))
         {
            int iStart=0,
                iEnd=0;

            sscanf(szParamVal, "%d %d", &iStart, &iEnd);
            if ((iEnd >= iStart) && (iStart > 0))
            {
               g_StaticParams.options.iStartScan = iStart;
               g_StaticParams.options.iEndScan = iEnd;
            }
         }
         else if (!strcmp(szParamName, "minimum_peaks"))
         {
            int iNum = 0;
            sscanf(szParamVal, "%d", &iNum);
            if (iNum > 0)
            {
               g_StaticParams.options.iMinPeaks = iNum;
            }
         }
         else if (!strcmp(szParamName, "precursor_charge"))
         {
            int iStart = 0,
               iEnd=0;

            sscanf(szParamVal, "%d %d", &iStart, &iEnd);
            if ((iEnd >= iStart) && (iStart >= 0) && (iEnd > 0))
            {
               if (iStart==0)
               {
                  g_StaticParams.options.iStartCharge = 1;
               }
               else
               {
                  g_StaticParams.options.iStartCharge = iStart;
               }

               g_StaticParams.options.iEndCharge = iEnd;
            }
         }
         else if (!strcmp(szParamName, "max_fragment_charge"))
         {
            int iCharge = 0;

            sscanf(szParamVal, "%d", &iCharge);
            if (iCharge > MAX_FRAGMENT_CHARGE)
               iCharge = MAX_FRAGMENT_CHARGE;

            if (iCharge > 0)
               g_StaticParams.options.iMaxFragmentCharge = iCharge;
            else
               g_StaticParams.options.iMaxFragmentCharge = DEFAULT_FRAGMENT_CHARGE;
         }
         else if (!strcmp(szParamName, "max_precursor_charge"))
         {
            int iCharge = 0;

            sscanf(szParamVal, "%d", &iCharge);
            if (iCharge > MAX_PRECURSOR_CHARGE)
               iCharge = MAX_PRECURSOR_CHARGE;

            if (iCharge > 0)
               g_StaticParams.options.iMaxPrecursorCharge = iCharge;
            else
               g_StaticParams.options.iMaxPrecursorCharge = DEFAULT_PRECURSOR_CHARGE;
         }
         else if (!strcmp(szParamName, "digest_mass_range"))
         {
            double dStart = 0.0,
                   dEnd = 0.0;

            sscanf(szParamVal, "%lf %lf", &dStart, &dEnd);
            if ((dEnd >= dStart) && (dStart >= 0.0))
            {
               g_StaticParams.options.dLowPeptideMass = dStart;
               g_StaticParams.options.dHighPeptideMass = dEnd;
            }
         }
         else if (!strcmp(szParamName, "ms_level"))
         {
            int iNum = 0;

            sscanf(szParamVal, "%d", &iNum);
            if (iNum == 2)
            {
               g_StaticParams.options.iStartMSLevel = 2;
               g_StaticParams.options.iEndMSLevel = 0;
            }
            else if (iNum == 3)
            {
               g_StaticParams.options.iStartMSLevel = 3;
               g_StaticParams.options.iEndMSLevel = 0;
            }
            else
            {
               g_StaticParams.options.iStartMSLevel = 2;
               g_StaticParams.options.iEndMSLevel = 3;
            }
         }
         else if (!strcmp(szParamName, "activation_method"))
         {
            sscanf(szParamVal, "%s", g_StaticParams.options.szActivationMethod);
         }
         else if (!strcmp(szParamName, "minimum_intensity"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iMinIntensity));
            if (g_StaticParams.options.iMinIntensity < 0)
            {
               g_StaticParams.options.iMinIntensity = 0;
            }
         }
         else if (!strcmp(szParamName, "decoy_search"))
         {
            sscanf(szParamVal, "%d", &(g_StaticParams.options.iDecoySearch));
            if ((g_StaticParams.options.iDecoySearch < 0) || (g_StaticParams.options.iDecoySearch > 2))
            {
               g_StaticParams.options.iDecoySearch = 0;
            }
         }
      }

   } // while

   if (g_StaticParams.tolerances.dFragmentBinSize == 0.0)
      g_StaticParams.tolerances.dFragmentBinSize = DEFAULT_BIN_WIDTH;

   // Set dBinWidth to its inverse in order to use a multiply instead of divide in BIN macro.
   g_StaticParams.dBinWidth = 1.0 /g_StaticParams.tolerances.dFragmentBinSize;
   g_StaticParams.dBinWidthMinusOffset = g_StaticParams.tolerances.dFragmentBinSize
      - g_StaticParams.tolerances.dFragmentBinStartOffset;
 
   // Set masses to either average or monoisotopic.
   CometMassSpecUtils::AssignMass(g_StaticParams.massUtility.pdAAMassParent, 
                                  g_StaticParams.massUtility.bMonoMassesParent, 
                                  &g_StaticParams.massUtility.dOH2parent);

   CometMassSpecUtils::AssignMass(g_StaticParams.massUtility.pdAAMassFragment, 
                                  g_StaticParams.massUtility.bMonoMassesFragment, 
                                  &g_StaticParams.massUtility.dOH2fragment); 

   g_StaticParams.massUtility.dCO = g_StaticParams.massUtility.pdAAMassFragment['c'] 
            + g_StaticParams.massUtility.pdAAMassFragment['o'];

   g_StaticParams.massUtility.dH2O = g_StaticParams.massUtility.pdAAMassFragment['h'] 
            + g_StaticParams.massUtility.pdAAMassFragment['h']
            + g_StaticParams.massUtility.pdAAMassFragment['o'];

   g_StaticParams.massUtility.dNH3 = g_StaticParams.massUtility.pdAAMassFragment['n'] 
            + g_StaticParams.massUtility.pdAAMassFragment['h'] 
            + g_StaticParams.massUtility.pdAAMassFragment['h'] 
            + g_StaticParams.massUtility.pdAAMassFragment['h'];

   g_StaticParams.massUtility.dNH2 = g_StaticParams.massUtility.pdAAMassFragment['n'] 
            + g_StaticParams.massUtility.pdAAMassFragment['h'] 
            + g_StaticParams.massUtility.pdAAMassFragment['h'];

   g_StaticParams.massUtility.dCOminusH2 = g_StaticParams.massUtility.dCO
            - g_StaticParams.massUtility.pdAAMassFragment['h']
            - g_StaticParams.massUtility.pdAAMassFragment['h'];

   fgets(szParamBuf, SIZE_BUF, fp);

   // Get enzyme specificity.
   strcpy(g_StaticParams.enzymeInformation.szSearchEnzymeName, "-");
   strcpy(g_StaticParams.enzymeInformation.szSampleEnzymeName, "-");
   while (!feof(fp))
   {
      int iCurrentEnzymeNumber;

      sscanf(szParamBuf, "%d.", &iCurrentEnzymeNumber);

      if (iCurrentEnzymeNumber == iSearchEnzymeNumber)
      {
         sscanf(szParamBuf, "%lf %s %d %s %s\n",
               &dTempMass, 
               g_StaticParams.enzymeInformation.szSearchEnzymeName, 
               &g_StaticParams.enzymeInformation.iSearchEnzymeOffSet, 
               g_StaticParams.enzymeInformation.szSearchEnzymeBreakAA, 
               g_StaticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
      }

      if (iCurrentEnzymeNumber == iSampleEnzymeNumber)
      {
         sscanf(szParamBuf, "%lf %s %d %s %s\n",
               &dTempMass, 
               g_StaticParams.enzymeInformation.szSampleEnzymeName, 
               &g_StaticParams.enzymeInformation.iSampleEnzymeOffSet, 
               g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, 
               g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA);
      }

      fgets(szParamBuf, SIZE_BUF, fp);
   }
   fclose(fp);

   if (!strcmp(g_StaticParams.enzymeInformation.szSearchEnzymeName, "-"))
   {
      printf(" Error - search enzyme number %d is missing definition in params file.\n\n", iSearchEnzymeNumber);
      exit(1);
   }
   if (!strcmp(g_StaticParams.enzymeInformation.szSampleEnzymeName, "-"))
   {
      printf(" Error - sample enzyme number %d is missing definition in params file.\n\n", iSampleEnzymeNumber);
      exit(1);
   }

   if (!strncmp(g_StaticParams.enzymeInformation.szSearchEnzymeBreakAA, "-", 1) && 
       !strncmp(g_StaticParams.enzymeInformation.szSearchEnzymeNoBreakAA, "-", 1))
   {
      g_StaticParams.options.bNoEnzymeSelected = 1;
   }
   else
   {
      g_StaticParams.options.bNoEnzymeSelected = 0;
   }

   // Load ion series to consider, useA, useB, useY are for neutral losses.
   g_StaticParams.ionInformation.iNumIonSeriesUsed = 0;
   for (i=0; i<9; i++)
   {
      if (g_StaticParams.ionInformation.iIonVal[i] > 0)
         g_StaticParams.ionInformation.piSelectedIonSeries[g_StaticParams.ionInformation.iNumIonSeriesUsed++] = i;
   }

   // Variable mod search for AAs listed in szVarModChar.
   g_StaticParams.szMod[0] = '\0';
   for (i=0; i<VMODS; i++)
   {
      if ((g_StaticParams.variableModParameters.varModList[i].dVarModMass != 0.0) &&
          (g_StaticParams.variableModParameters.varModList[i].szVarModChar[0] != '\0'))
      {
         sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "(%s%c %+0.6f) ", 
               g_StaticParams.variableModParameters.varModList[i].szVarModChar,
               g_StaticParams.variableModParameters.cModCode[i],
               g_StaticParams.variableModParameters.varModList[i].dVarModMass);
         g_StaticParams.variableModParameters.bVarModSearch = 1;
      }
   }

   if (g_StaticParams.variableModParameters.dVarModMassN != 0.0)
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "(nt] %+0.6f) ", 
            g_StaticParams.variableModParameters.dVarModMassN);       // FIX determine .out file header string for this?
      g_StaticParams.variableModParameters.bVarModSearch = 1;
   }
   if (g_StaticParams.variableModParameters.dVarModMassC != 0.0)
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "(ct[ %+0.6f) ", 
            g_StaticParams.variableModParameters.dVarModMassC);
      g_StaticParams.variableModParameters.bVarModSearch = 1;
   }

   // Do Sp scoring after search based on how many lines to print out.
   if (g_StaticParams.options.iNumStored > NUM_STORED)
      g_StaticParams.options.iNumStored = NUM_STORED;
   else if (g_StaticParams.options.iNumStored < 1)
      g_StaticParams.options.iNumStored = 1;


   if (g_StaticParams.options.iNumPeptideOutputLines > g_StaticParams.options.iNumStored)
      g_StaticParams.options.iNumPeptideOutputLines = g_StaticParams.options.iNumStored;
   else if (g_StaticParams.options.iNumPeptideOutputLines < 1)
      g_StaticParams.options.iNumPeptideOutputLines = 1;

   if (g_StaticParams.peaksInformation.iNumMatchPeaks > 5)
      g_StaticParams.peaksInformation.iNumMatchPeaks = 5;

   // FIX how to deal with term mod on both peptide and protein?
   if (g_StaticParams.staticModifications.dAddCterminusPeptide != 0.0)
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "+ct=%0.6f ", 
            g_StaticParams.staticModifications.dAddCterminusPeptide);
   }
   if (g_StaticParams.staticModifications.dAddNterminusPeptide != 0.0)
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "+nt=%0.6f ", 
            g_StaticParams.staticModifications.dAddNterminusPeptide);
   }
   if (g_StaticParams.staticModifications.dAddCterminusProtein!= 0.0)
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "+ctprot=%0.6f ", 
            g_StaticParams.staticModifications.dAddCterminusProtein);
   }
   if (g_StaticParams.staticModifications.dAddNterminusProtein!= 0.0)
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "+ntprot=%0.6f ", 
            g_StaticParams.staticModifications.dAddNterminusProtein);
   }

   for (i=65; i<=90; i++)  // 65-90 represents upper case letters in ASCII
   {
      if (g_StaticParams.staticModifications.pdStaticMods[i] != 0.0)
      {
         sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "%c=%0.6f ", i,
               g_StaticParams.massUtility.pdAAMassParent[i] += g_StaticParams.staticModifications.pdStaticMods[i]);
         g_StaticParams.massUtility.pdAAMassFragment[i] += g_StaticParams.staticModifications.pdStaticMods[i];
      }
      else if (i=='B' || i=='J' || i=='U' || i=='X' || i=='Z')
      {
         g_StaticParams.massUtility.pdAAMassParent[i] = 999999.;
         g_StaticParams.massUtility.pdAAMassFragment[i] = 999999.;
      }
   }

   // Print out enzyme name to g_StaticParams.szMod.
   if (!g_StaticParams.options.bNoEnzymeSelected)
   {
      char szTmp[4];

      szTmp[0]='\0';
      if (g_StaticParams.options.iEnzymeTermini != 2)
         sprintf(szTmp, ":%d", g_StaticParams.options.iEnzymeTermini);

      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "Enzyme:%s (%d%s)", 
            g_StaticParams.enzymeInformation.szSearchEnzymeName,
            g_StaticParams.enzymeInformation.iAllowedMissedCleavage,
            szTmp);
   }
   else
   {
      sprintf(g_StaticParams.szMod + strlen(g_StaticParams.szMod), "Enzyme:%s",
            g_StaticParams.enzymeInformation.szSearchEnzymeName);
   }

   if (!bCurrentParamsFile)
   {
      fprintf(stderr, " Error - outdated params file; update params file using '-p' option.\n\n");
      exit(1);
   }

   if (g_StaticParams.tolerances.dFragmentBinSize < g_StaticParams.tolerances.dFragmentBinStartOffset)
   {
      fprintf(stderr, " Error - tolerance %f < offset %f\n",
            g_StaticParams.tolerances.dFragmentBinSize,
            g_StaticParams.tolerances.dFragmentBinStartOffset);
      exit(1);
   }


   // print parameters

   char szIsotope[16];
   char szPeak[16];

   sprintf(g_StaticParams.szIonSeries, "ion series ABCXYZ nl: %d%d%d%d%d%d %d",
           g_StaticParams.ionInformation.iIonVal[0],
           g_StaticParams.ionInformation.iIonVal[1],
           g_StaticParams.ionInformation.iIonVal[2],
           g_StaticParams.ionInformation.iIonVal[3],
           g_StaticParams.ionInformation.iIonVal[4],
           g_StaticParams.ionInformation.iIonVal[5],
           g_StaticParams.ionInformation.bUseNeutralLoss);

   char szUnits[8];
   char szDecoy[20];
   char szReadingFrame[20];
   char szRemovePrecursor[20];

   if (g_StaticParams.tolerances.iMassToleranceUnits==0)
      strcpy(szUnits, " AMU");
   else if (g_StaticParams.tolerances.iMassToleranceUnits==1)
      strcpy(szUnits, " MMU");
   else
      strcpy(szUnits, " PPM");

   if (g_StaticParams.options.iDecoySearch)
      sprintf(szDecoy, " DECOY%d", g_StaticParams.options.iDecoySearch);
   else
      szDecoy[0]=0;

   if (g_StaticParams.options.iRemovePrecursor)
      sprintf(szRemovePrecursor, " REMOVEPREC%d", g_StaticParams.options.iRemovePrecursor);
   else
      szRemovePrecursor[0]=0;

   if (g_StaticParams.options.iWhichReadingFrame)
      sprintf(szReadingFrame, " FRAME%d", g_StaticParams.options.iWhichReadingFrame);
   else
      szReadingFrame[0]=0;

   szIsotope[0]='\0';
   if (g_StaticParams.tolerances.iIsotopeError==1)
      strcpy(szIsotope, "ISOTOPE1");
   else if (g_StaticParams.tolerances.iIsotopeError==2)
      strcpy(szIsotope, "ISOTOPE2");

   szPeak[0]='\0';
   if (g_StaticParams.ionInformation.iTheoreticalFragmentIons==1)
      strcpy(szPeak, "PEAK1");

   sprintf(g_StaticParams.szDisplayLine, "display top %d, %s%s%s%s%s%s%s%s",
         g_StaticParams.options.iNumPeptideOutputLines,
         szRemovePrecursor,
         szReadingFrame,
         szPeak,
         szUnits,
         (g_StaticParams.tolerances.iMassToleranceType==0?" MH+":" m/z"),
         szIsotope,
         szDecoy,
         (g_StaticParams.options.bClipNtermMet?" CLIPMET":"") );

} // LoadParameters


//-->MH
// Reads a file name and assumes type based on usage of our suggested file extensions.
MSFileFormat GetMstFileType(char* c)
{
   char file[256];
   char ext[256];
   char *tok;

   strcpy(file,c);
   tok=strtok(file,".\n");

   while(tok!=NULL)
   {
      strcpy(ext,tok);
      tok=strtok(NULL,".\n");
   }

   if (strcmp(ext,"ms1")==0 || strcmp(ext,"MS1")==0)
      return ms1;
   if (strcmp(ext,"ms2")==0 || strcmp(ext,"MS2")==0)
      return ms2;
   if (strcmp(ext,"bms1")==0 || strcmp(ext,"BMS1")==0)
      return bms1;
   if (strcmp(ext,"bms2")==0 || strcmp(ext,"BMS2")==0)
      return bms2;
   if (strcmp(ext,"cms1")==0 || strcmp(ext,"CMS1")==0)
      return cms1;
   if (strcmp(ext,"cms2")==0 || strcmp(ext,"CMS2")==0)
      return cms2;
   if (strcmp(ext,"zs")==0 || strcmp(ext,"ZS")==0)
      return zs;
   if (strcmp(ext,"uzs")==0 || strcmp(ext,"UZS")==0)
      return uzs;
   if (strcmp(ext,"mzXML")==0 || strcmp(ext,"mzML")==0)
      return mzXML;

   return dunno;
}
//-->endMH


// Parses the command line and determines the type of analysis to perform.
void ParseCmdLine(char *cmd,
                  int *iFirst,
                  int *iLast,
                  int *Z,
                  int *iCount,
                  int *iType,
                  char *pszFileName)
{
   char *tok;
   char *scan;

   *iType = 0;

   // Get the file name.
   tok = strtok(cmd,":\n");
   strcpy(pszFileName, tok);

   // Get additional filters.
   scan = strtok(NULL, ":\n");

   // Analyze entire file.
   if (scan == NULL)
   {
      if (g_StaticParams.options.iStartScan == 0)
      {
         *iType = AnalysisType_EntireFile;
         return;
      }
      else
      {
         *iFirst = g_StaticParams.options.iStartScan;
         *iLast = g_StaticParams.options.iEndScan;
         *iType = AnalysisType_SpecificScanRange;
         return;
      }
   }

   // Analyze a portion of the file.
   if (strchr(scan,'.') != NULL)
   {
      *iType = AnalysisType_SpecificScanAndCharge;
      tok = strtok(scan,".\n");
      *iFirst = atoi(tok);
      tok = strtok(NULL,".\n");
      *Z = atoi(tok);
   }
   else if (strchr(scan,'-') != NULL)
   {
      *iType = AnalysisType_SpecificScanRange;
      tok = strtok(scan, "-\n");
      *iFirst = atoi(tok);
      tok = strtok(NULL,"-\n");
      *iLast = atoi(tok);
   }
   else if (strchr(scan,'+') != NULL)
   {
      *iType = AnalysisType_StartScanAndCount;
      tok = strtok(scan,"+\n");
      *iFirst = atoi(tok);
      tok = strtok(NULL,"+\n");
      *iCount = atoi(tok);
   }
   else
   {
      *iType = AnalysisType_SpecificScan;
      *iFirst = atoi(scan);
   }

} // ParseCmdLine


// Print out comet.params to file.
void PrintParams()
{
   FILE *fp;

   if ( (fp=fopen("comet.params.new", "w"))==NULL)
   {
      fprintf(stderr, "\n Error - cannot write file comet.paramsnew\n\n");
      exit(1);
   }

   fprintf(fp, "# Comet MS/MS search engine parameters file.\n\
# Everything following the '#' symbol is treated as a comment.\n\
\n\
database_name = /some/path/db.fasta\n\
decoy_search = 0                       # 0=no (default), 1=concatenated search, 2=separate search\n\
\n\
num_threads = 0                        # 0=poll CPU to set num threads; else specify num threads directly (max %d)\n\
\n", MAX_THREADS);

   fprintf(fp,
"#\n\
# masses\n\
#\n\
peptide_mass_tolerance = 3.00\n\
peptide_mass_units = 0                 # 0=amu, 1=mmu, 2=ppm\n\
mass_type_parent = 1                   # 0=average masses, 1=monoisotopic masses\n\
mass_type_fragment = 1                 # 0=average masses, 1=monoisotopic masses\n\
precursor_tolerance_type = 0           # 0=MH+ (default), 1=precursor m/z\n\
isotope_error = 0                      # 0=off, 1=on -1/0/1/2/3 (standard C13 error), 2= -8/-4/0/4/8 (for +4/+8 labeling)\n\
\n\
#\n\
# search enzyme\n\
#\n\
search_enzyme_number = 1               # choose from list at end of this params file\n\
num_enzyme_termini = 2                 # valid values are 1 (semi-digested), 2 (fully digested, default), 8 N-term, 9 C-term\n\
allowed_missed_cleavage = 2            # maximum value is 5; for enzyme search\n\
\n\
#\n\
# Up to 6 variable modifications are supported\n\
# format:  <mass> <residues> <0=variable/1=binary> <max mods per a peptide>\n\
#     e.g. 79.966331 STY 0 3\n\
#\n\
variable_mod1 = 15.9949 M 0 3\n\
variable_mod2 = 0.0 X 0 3\n\
variable_mod3 = 0.0 X 0 3\n\
variable_mod4 = 0.0 X 0 3\n\
variable_mod5 = 0.0 X 0 3\n\
variable_mod6 = 0.0 X 0 3\n\
max_variable_mods_in_peptide = 5\n\
\n\
#\n\
# fragment ions\n\
#\n\
# ion trap ms/ms:  0.36 tolerance, 0.11 offset (mono masses)\n\
# high res ms/ms:  0.01 tolerance, 0.00 offset (mono masses)\n\
#\n\
fragment_bin_tol = 0.36                # binning to use on fragment ions\n\
fragment_bin_offset = 0.11             # offset position to start the binning\n\
theoretical_fragment_ions = 0          # 0=default peak shape, 1=M peak only\n\
use_A_ions = 0\n\
use_B_ions = 1\n\
use_C_ions = 0\n\
use_X_ions = 0\n\
use_Y_ions = 1\n\
use_Z_ions = 0\n\
use_NL_ions = 1                        # 0=no, 1=yes to consider NH3/H2O neutral loss peaks\n\
\n\
#\n\
# output\n\
#\n\
output_sqtstream = 0                   # 0=no, 1=yes  write sqt to standard output\n\
output_sqtfile = 0                     # 0=no, 1=yes  write sqt file\n\
output_pepxmlfile = 1                  # 0=no, 1=yes  write pep.xml file\n\
output_outfiles = 0                    # 0=no, 1=yes  write .out files\n\
print_expect_score = 1                 # 0=no, 1=yes to replace Sp with expect in out & sqt\n\
num_output_lines = 5                   # num peptide results to show\n\
show_fragment_ions = 0                 # 0=no, 1=yes for out files only\n\
\n\
sample_enzyme_number = 1               # Sample enzyme which is possibly different than the one applied to the search.\n\
                                       # Used to calculate NTT & NMC in pepXML output (default=1 for trypsin).\n\
\n\
#\n\
# mzXML parameters\n\
#\n\
scan_range = 0 0                       # start and scan scan range to search; 0 as 1st entry ignores parameter\n\
precursor_charge = 0 0                 # precursor charge range to analyze; does not override mzXML charge; 0 as 1st entry ignores parameter\n\
ms_level = 2                           # MS level to analyze, valid are levels 2 (default) or 3\n\
activation_method = ALL                # activation method; used if activation method set; allowed ALL, CID, ECD, ETD, PQD, HCD, IRMPD\n\
\n\
#\n\
# misc parameters\n\
#\n\
digest_mass_range = 600.0 5000.0       # MH+ peptide mass range to analyze\n\
num_results = 50                       # number of search hits to store internally\n\
skip_researching = 1                   # for '.out' file output only, 0=search everything again (default), 1=don't search if .out exists\n\
max_fragment_charge = %d                # set maximum fragment charge state to analyze (allowed max %d)\n\
max_precursor_charge = %d               # set maximum precursor charge state to analyze (allowed max %d)\n",
      DEFAULT_FRAGMENT_CHARGE,
      MAX_FRAGMENT_CHARGE,
      DEFAULT_PRECURSOR_CHARGE,
      MAX_PRECURSOR_CHARGE);

fprintf(fp,
"nucleotide_reading_frame = 0           # 0=proteinDB, 1-6, 7=forward three, 8=reverse three, 9=all six\n\
clip_nterm_methionine = 0              # 0=leave sequences as-is; 1=also consider sequence w/o N-term methionine\n\
\n\
#\n\
# spectral processing\n\
#\n\
minimum_peaks = 5                      # minimum num. of peaks in spectrum to search (default %d)\n", MINIMUM_PEAKS);

fprintf(fp,
"minimum_intensity = 0                  # minimum intensity value to read in\n\
remove_precursor_peak = 0              # 0=no, 1=yes, 2=all charge reduced precursor peaks (for ETD)\n\
remove_precursor_tolerance = 1.5       # +- Da tolerance for precursor removal\n\
\n\
#\n\
# additional modifications\n\
#\n\
\n\
variable_C_terminus = 0.0\n\
variable_N_terminus = 0.0\n\
variable_C_terminus_distance = -1      # -1=all peptides, 0=protein terminus, 1-N = maximum offset from C-terminus\n\
variable_N_terminus_distance = -1      # -1=all peptides, 0=protein terminus, 1-N = maximum offset from N-terminus\n\
\n\
add_Cterm_peptide = 0.0\n\
add_Nterm_peptide = 0.0\n\
add_Cterm_protein = 0.0\n\
add_Nterm_protein = 0.0\n\
\n\
add_G_glycine = 0.0000                 # added to G - avg.  57.0513, mono.  57.02146\n\
add_A_alanine = 0.0000                 # added to A - avg.  71.0779, mono.  71.03711\n\
add_S_serine = 0.0000                  # added to S - avg.  87.0773, mono.  87.02303\n\
add_P_proline = 0.0000                 # added to P - avg.  97.1152, mono.  97.05276\n\
add_V_valine = 0.0000                  # added to V - avg.  99.1311, mono.  99.06841\n\
add_T_threonine = 0.0000               # added to T - avg. 101.1038, mono. 101.04768\n\
add_C_cysteine = 57.021464             # added to C - avg. 103.1429, mono. 103.00918\n\
add_L_leucine = 0.0000                 # added to L - avg. 113.1576, mono. 113.08406\n\
add_I_isoleucine = 0.0000              # added to I - avg. 113.1576, mono. 113.08406\n\
add_N_asparagine = 0.0000              # added to N - avg. 114.1026, mono. 114.04293\n\
add_D_aspartic_acid = 0.0000           # added to D - avg. 115.0874, mono. 115.02694\n\
add_Q_glutamine = 0.0000               # added to Q - avg. 128.1292, mono. 128.05858\n\
add_K_lysine = 0.0000                  # added to K - avg. 128.1723, mono. 128.09496\n\
add_E_glutamic_acid = 0.0000           # added to E - avg. 129.1140, mono. 129.04259\n\
add_M_methionine = 0.0000              # added to M - avg. 131.1961, mono. 131.04048\n\
add_O_ornithine = 0.0000               # added to O - avg. 132.1610, mono  132.08988\n\
add_H_histidine = 0.0000               # added to H - avg. 137.1393, mono. 137.05891\n\
add_F_phenylalanine = 0.0000           # added to F - avg. 147.1739, mono. 147.06841\n\
add_R_arginine = 0.0000                # added to R - avg. 156.1857, mono. 156.10111\n\
add_Y_tyrosine = 0.0000                # added to Y - avg. 163.0633, mono. 163.06333\n\
add_W_tryptophan = 0.0000              # added to W - avg. 186.0793, mono. 186.07931\n\
add_B_user_amino_acid = 0.0000         # added to B - avg.   0.0000, mono.   0.00000\n\
add_J_user_amino_acid = 0.0000         # added to J - avg.   0.0000, mono.   0.00000\n\
add_U_user_amino_acid = 0.0000         # added to U - avg.   0.0000, mono.   0.00000\n\
add_X_user_amino_acid = 0.0000         # added to X - avg.   0.0000, mono.   0.00000\n\
add_Z_user_amino_acid = 0.0000         # added to Z - avg.   0.0000, mono.   0.00000\n\
\n\
#\n\
# COMET_ENZYME_INFO _must_ be at the end of this parameters file\n\
#\n\
[COMET_ENZYME_INFO]\n\
0.  No_enzyme              0      -           -\n\
1.  Trypsin                1      KR          P\n\
2.  Trypsin/P              1      KR          -\n\
3.  Lys_C                  1      K           P\n\
4.  Lys_N                  0      K           -\n\
5.  Arg_C                  1      R           P\n\
6.  Asp_N                  0      D           -\n\
7.  CNBr                   1      M           -\n\
8.  Glu_C                  1      DE          P\n\
9.  PepsinA                1      FL          P\n\
10. Chymotrypsin           1      FWYL        P\n\
\n");

   printf("\n Created:  comet.params.new\n\n");
   fclose(fp);

} // PrintParams


void CalcRunTime(time_t tStartTime)
{
   char szTimeBuf[500];
   time_t tEndTime;
   int iTmp;

   time(&tEndTime);

   int iElapseTime=(int)difftime(tEndTime, tStartTime);

   // Print out header/search info.
   sprintf(szTimeBuf, "%s,", g_StaticParams._dtInfoStart.szDate);
   if ( (iTmp = (int)(iElapseTime/3600) )>0)
      sprintf(szTimeBuf+strlen(szTimeBuf), " %d hr.", iTmp);
   if ( (iTmp = (int)((iElapseTime-(int)(iElapseTime/3600)*3600)/60) )>0)
      sprintf(szTimeBuf+strlen(szTimeBuf), " %d min.", iTmp);
   if ( (iTmp = (int)((iElapseTime-((int)(iElapseTime/3600))*3600)%60) )>0)
      sprintf(szTimeBuf+strlen(szTimeBuf), " %d sec.", iTmp);
   if (iElapseTime == 0)
      sprintf(szTimeBuf+strlen(szTimeBuf), " 0 sec.");
   sprintf(szTimeBuf+strlen(szTimeBuf), " on %s", g_StaticParams.szHostName);

   g_StaticParams.iElapseTime = iElapseTime;
   strncpy(g_StaticParams.szTimeBuf, szTimeBuf, 200);
   g_StaticParams.szTimeBuf[199]='\0';
}


void PRINT_SQT_HEADER(FILE *fpout,
                      char *szParamsFile)
{
/*
   char *pStr;
   char szTmp[100];
*/
   char szParamBuf[SIZE_BUF];
   time_t tTime;
   FILE *fp;

   fprintf(fpout, "H\tSQTGenerator Comet\n");
   fprintf(fpout, "H\tCometVersion\t%s\n", version);
   fprintf(fpout, "H\tStartTime %s\n", g_StaticParams._dtInfoStart.szDate);
   time(&tTime);
   strftime(g_StaticParams._dtInfoStart.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tTime));
   fprintf(fpout, "H\tEndTime %s\n", g_StaticParams._dtInfoStart.szDate);

   if ((fp=fopen(szParamsFile, "r")) == NULL)
   {
      fprintf(stderr, " Error - cannot open parameter file %s.\n\n", szParamsFile);
      exit(1);
   }

   fprintf(fpout, "H\tDBSeqLength\t%lu\n", g_StaticParams.databaseInfo.liTotAACount);
   fprintf(fpout, "H\tDBLocusCount\t%d\n", g_StaticParams.databaseInfo.iTotalNumProteins);

   while (fgets(szParamBuf, SIZE_BUF, fp))
      fprintf(fpout, "H\tCometParams\t%s", szParamBuf);

   fclose(fp);
}
