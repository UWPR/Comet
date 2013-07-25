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
#include "CometData.h"
#include "CometMassSpecUtils.h"
#include "CometSearch.h"
#include "CometPostAnalysis.h"
#include "CometWriteOut.h"
#include "CometWriteSqt.h"
#include "CometWriteTxt.h"
#include "CometWritePepXML.h"
#include "CometWritePinXML.h"
#include "Threading.h"
#include "ThreadPool.h"
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

// EVA TODO: Need to fix this later!  We can't pass in the params file, the UI won't have one.
CometSearchManager::CometSearchManager(StaticParams &staticParams, vector<InputFileInfo*> &pvInputFiles, char *pszParamsFile)
{
    if (NULL != pszParamsFile)
    {
        _strParamsFile = pszParamsFile;
    }
    g_staticParams = staticParams;

    int numInputFiles = pvInputFiles.size();
    for (int i = 0; i < numInputFiles; i++)
    {
        g_pvInputFiles.push_back(pvInputFiles.at(i));
    }

    Initialize();
}

CometSearchManager::~CometSearchManager()
{
   // Destroy the mutex we used to protect g_pvQuery.
   Threading::DestroyMutex(g_pvQueryMutex);

   // Clean up the input files vector
   for (int i=0; i<(int)g_pvInputFiles.size(); i++)
      delete g_pvInputFiles.at(i);
   g_pvInputFiles.clear();
}

void CometSearchManager::Initialize()
{
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

   // Initialize the mutexes we'll use to protect global data.
   Threading::CreateMutex(&g_pvQueryMutex);
}

void CometSearchManager::GetHostName(void)
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

void CometSearchManager::DoSearch()
{
   for (int i=0; i<(int)g_pvInputFiles.size(); i++)
   {
       UpdateInputFile(g_pvInputFiles.at(i));

       time_t tStartTime;
       time(&tStartTime);
       strftime(g_staticParams.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));

       if (!g_staticParams.options.bOutputSqtStream)
       {
          printf(" Comet version \"%s\"\n", comet_version);
          printf(" Search start:  %s\n", g_staticParams.szDate);
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
             sprintf(szOutputSQT, "%s.sqt", g_staticParams.inputFile.szBaseName);
          else
             sprintf(szOutputSQT, "%s.%d-%d.sqt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

          if ((fpout_sqt = fopen(szOutputSQT, "w")) == NULL)
          {
             fprintf(stderr, "Error - cannot write to file \"%s\".\n\n", szOutputSQT);
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
                fprintf(stderr, "Error - cannot write to decoy file \"%s\".\n\n", szOutputDecoySQT);
                exit(1);
             }
          }
       }

       if (g_staticParams.options.bOutputTxtFile)
       {
          if (iAnalysisType == AnalysisType_EntireFile)
             sprintf(szOutputTxt, "%s.txt", g_staticParams.inputFile.szBaseName);
          else
             sprintf(szOutputTxt, "%s.%d-%d.txt", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

          if ((fpout_txt = fopen(szOutputTxt, "w")) == NULL)
          {
             fprintf(stderr, "Error - cannot write to file \"%s\".\n\n", szOutputTxt);
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
                fprintf(stderr, "Error - cannot write to decoy file \"%s\".\n\n", szOutputDecoyTxt);
                exit(1);
             }
          }
       }

       if (g_staticParams.options.bOutputPepXMLFile)
       {
          if (iAnalysisType == AnalysisType_EntireFile)
             sprintf(szOutputPepXML, "%s.pep.xml", g_staticParams.inputFile.szBaseName);
          else
             sprintf(szOutputPepXML, "%s.%d-%d.pep.xml", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

          if ((fpout_pepxml = fopen(szOutputPepXML, "w")) == NULL)
          {
             fprintf(stderr, "Error - cannot write to file \"%s\".\n\n", szOutputPepXML);
             exit(1);
          }

          // EVA TODO: Need to fix this later!  We can't pass in the params file, the UI won't have one.
          CometWritePepXML::WritePepXMLHeader(fpout_pepxml, _strParamsFile.c_str());

          if (g_staticParams.options.iDecoySearch == 2)
          {
             if (iAnalysisType == AnalysisType_EntireFile)
                sprintf(szOutputDecoyPepXML, "%s.decoy.pep.xml", g_staticParams.inputFile.szBaseName);
             else
                sprintf(szOutputDecoyPepXML, "%s.%d-%d.decoy.pep.xml", g_staticParams.inputFile.szBaseName, iFirstScan, iLastScan);

             if ((fpoutd_pepxml = fopen(szOutputDecoyPepXML, "w")) == NULL)
             {
                fprintf(stderr, "Error - cannot write to decoy file \"%s\".\n\n", szOutputDecoyPepXML);
                exit(1);
             }

             // EVA TODO: Need to fix this later!  We can't pass in the params file, the UI won't have one.
             CometWritePepXML::WritePepXMLHeader(fpoutd_pepxml, _strParamsFile.c_str());
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
             fprintf(stderr, "Error - cannot write to file \"%s\".\n\n", szOutputPinXML);
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
             printf(" - Load and process input spectra\n");

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
             printf(" - Allocate memory to store results\n");

          AllocateResultsMem();

          if (!g_staticParams.options.bOutputSqtStream)
             printf(" - Number of mass-charge spectra loaded: %d\n", (int)g_pvQuery.size());

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
             printf(" - Write output\n");

          if (g_staticParams.options.bOutputOutFiles)
             CometWriteOut::WriteOut();

          if (g_staticParams.options.bOutputPepXMLFile)
             CometWritePepXML::WritePepXML(fpout_pepxml, fpoutd_pepxml, szOutputPepXML, szOutputDecoyPepXML);

          if (g_staticParams.options.bOutputPinXMLFile)
             CometWritePinXML::WritePinXML(fpout_pinxml);

          if (g_staticParams.options.bOutputTxtFile)
             CometWriteTxt::WriteTxt(fpout_txt, fpoutd_txt, szOutputTxt, szOutputDecoyTxt);

          // EVA TODO: Need to fix this later - the UI won't have a params file to pass
          //// Write SQT last as I destroy the g_staticParams.szMod string during that process
          if (g_staticParams.options.bOutputSqtStream || g_staticParams.options.bOutputSqtFile)
             CometWriteSqt::WriteSqt(fpout_sqt, fpoutd_sqt, szOutputSQT, szOutputDecoySQT, _strParamsFile.c_str());

          // Deleting each Query object in the vector calls its destructor, which 
          // frees the spectral memory (see definition for Query in CometData.h).
          for (int i=0; i<(int)g_pvQuery.size(); i++)
             delete g_pvQuery.at(i);

          g_pvQuery.clear();
       }
       if (iTotalSpectraSearched == 0)
       {
          printf(" Warning - no spectra searched.\n\n");
       }

       if (!g_staticParams.options.bOutputSqtStream)
       {
          time(&tStartTime);
          strftime(g_staticParams.szDate, 26, "%m/%d/%Y, %I:%M:%S %p", localtime(&tStartTime));
          printf(" Search end:    %s\n\n", g_staticParams.szDate);
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

void CometSearchManager::UpdateInputFile(InputFileInfo *pFileInfo)
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
            fprintf(stderr, "\n Error - could not create directory \"%s\".\n", g_staticParams.inputFile.szBaseName);
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
               fprintf(stderr, "\n Error - could not create directory \"%s\".\n", szDecoyDir);
               exit(1);
            }
         }
      }
#else
      if ((mkdir(g_staticParams.inputFile.szBaseName, 0775) == -1) && (errno != EEXIST))
      {
         fprintf(stderr, "\n Error - could not create directory \"%s\".\n", g_staticParams.inputFile.szBaseName);
         exit(1);
      }
      if (g_staticParams.options.iDecoySearch == 2)
      {
         char szDecoyDir[SIZE_FILE];
         sprintf(szDecoyDir, "%s_decoy", g_staticParams.inputFile.szBaseName);

         if ((mkdir(szDecoyDir , 0775) == -1) && (errno != EEXIST))
         {
            fprintf(stderr, "\n Error - could not create directory \"%s\".\n\n", szDecoyDir);
            exit(1);
         }
      }
#endif
   }
}

void CometSearchManager::SetMSLevelFilter(MSReader &mstReader)
{
   vector<MSSpectrumType> msLevel;
   if (g_staticParams.options.iStartMSLevel == 3)
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
void CometSearchManager::AllocateResultsMem()
{
   for (unsigned i=0; i<g_pvQuery.size(); i++)
   {
      Query* pQuery = g_pvQuery.at(i);

      pQuery->_pResults = (struct Results *)malloc(sizeof(struct Results) * g_staticParams.options.iNumStored);

      if (pQuery->_pResults == NULL)
      {
         fprintf(stderr, " Error malloc(_pResults[])\n");
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
            fprintf(stderr, " Error malloc(_pDecoys[])\n");
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
         pQuery->_pResults[j].szPeptide[0] = '\0';
         pQuery->_pResults[j].szProtein[0] = '\0';

         if (g_staticParams.options.iDecoySearch==2)
         {
            pQuery->_pDecoys[j].fXcorr = 0.0;
            pQuery->_pDecoys[j].fScoreSp = 0.0;
            pQuery->_pDecoys[j].szPeptide[0] = '\0';
            pQuery->_pDecoys[j].szProtein[0] = '\0';
         }
      }
   }
}

bool CometSearchManager::compareByPeptideMass(Query const* a, Query const* b)
{
   return (a->_pepMassInfo.dExpPepMass < b->_pepMassInfo.dExpPepMass);
}

void CometSearchManager::CalcRunTime(time_t tStartTime)
{
   char szTimeBuf[500];
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
   strncpy(g_staticParams.szTimeBuf, szTimeBuf, 200);
   g_staticParams.szTimeBuf[199]='\0';
}
