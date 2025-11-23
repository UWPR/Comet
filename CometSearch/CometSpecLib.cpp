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
#include "CometSpecLib.h"
#include "CometSearch.h"
#include "CometPreprocess.h"
#include "ThreadPool.h"
#include "CometStatus.h"
#include "CometMassSpecUtils.h"


CometSpecLib::CometSpecLib()
{
}


CometSpecLib::~CometSpecLib()
{
}


// SpecLib will be a vector of structs. Structs contain spectra and IDs (peptide, scan #, etc.)
bool CometSpecLib::LoadSpecLib(std::string strSpecLibFile)
{
   FILE *fp;

   if (g_bSpecLibRead)
      return true;

   if ((fp = fopen(g_staticParams.speclibInfo.strSpecLibFile.c_str(), "r")) == NULL)
   {
      std::string strErrorMsg = "Error, spectral library file cannot be read: '" + g_staticParams.speclibInfo.strSpecLibFile + "'.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }
   else
      fclose(fp);

   // Transform to lower case for case insensitive file extension match
   std::string strLowerFileName = strSpecLibFile;
   std::transform(strLowerFileName.begin(), strLowerFileName.end(), strLowerFileName.begin(), ::tolower);

   // Find the position of the last dot in the filename
   size_t dotPos = strLowerFileName.rfind('.');
   if (dotPos == std::string::npos)
   {
      // No dot found, so no extension
      return false;
   }

   // Extract the file extension
   std::string strExtension = strLowerFileName.substr(dotPos);

   if (strExtension == ".db")
   {
      if (!ReadSpecLibSqlite(strSpecLibFile))
         return false;
   }
   else if (strExtension == ".raw")
   {
      if (!ReadSpecLibRaw(strSpecLibFile))
         return false;
   }
   else if (strExtension == ".msp")
   {
      if (!ReadSpecLibMSP(strSpecLibFile))
         return false;
   }
   else
   {
      std::string strErrorMsg = "Error, expecting sqlite .db or Thermo .raw file for the spectral library.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   g_bSpecLibRead = true;

/*
   for (auto it = g_vSpecLib.begin(); it != g_vSpecLib.end(); ++it)
   {
      printf("OK.  %d, %s, %d peaks\n", (*it).iLibEntry, (*it).strName.c_str(), (*it).iNumPeaks);
      
      for (unsigned int i = 0; i < (*it).iNumPeaks; ++i)
      {
         printf("\t%0.2lf\t%0.2lf\n", (*it).vSpecLibPeaks.at(i).first, (*it).vSpecLibPeaks.at(i).second);
         if (i == 4)
            break;
      }
   }
*/

   return true;
}


bool CometSpecLib::ReadSpecLibSqlite(std::string strSpecLibFile)
{

   printf(" Error - sqlite/.db files as spectral libraries are not supported yet.\n");
   exit(1);
/*
   sqlite3* db;
   sqlite3_stmt* stmt;
   const char* sql = "SELECT * FROM SpectrumTable";

   // Open the database
   if (sqlite3_open(strSpecLibFile.c_str(), &db) != SQLITE_OK)
   {
//    std::cerr << "Cannot open sqlite database: " << sqlite3_errmsg(db) << std::endl;
      string strErrorMsg = "Error - cannot open sqlite database file ' " + strSpecLibFile + "': " + string(sqlite3_errmsg(db) + ".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Prepare the SQL statement
   if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
   {
//    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
      string strErrorMsg = "Error - sqlite failed to prepare statment: " + string(sqlite3_errmsg(db) + ".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      sqlite3_close(db);
      return false;
   }

   // Execute the SQL statement and read the rows
   while (sqlite3_step(stmt) == SQLITE_ROW)
   {
      int numCols = sqlite3_column_count(stmt);

      for (int col = 0; col < numCols; ++col)
      {
         const char* colName = sqlite3_column_name(stmt, col);
         int colType = sqlite3_column_type(stmt, col);

         std::cout << colName << ": ";

         switch (colType)
         {
            case SQLITE_INTEGER:
               std::cout << sqlite3_column_int(stmt, col) << std::endl;
               break;
            case SQLITE_FLOAT:
               std::cout << sqlite3_column_double(stmt, col) << std::endl;
               break;
            case SQLITE_TEXT:
               std::cout << sqlite3_column_text(stmt, col) << std::endl;
               break;
            case SQLITE_BLOB:
            {
               const void* blobData = sqlite3_column_blob(stmt, col);
               int blobSize = sqlite3_column_bytes(stmt, col);
               std::vector<double> decodedBlob = decodeBlob(blobData, blobSize);
               printDoubleVector(decodedBlob);
               break;
            }
            case SQLITE_NULL:
               std::cout << "NULL" << std::endl;
               break;
            default:
               std::cout << "Unknown data type" << std::endl;
               break;
         }
      }
      std::cout << "--------------------------------------" << std::endl;
   }

   // Clean up
   sqlite3_finalize(stmt);
   sqlite3_close(db);

   return 0;
*/
}


bool CometSpecLib::ReadSpecLibRaw(std::string strSpecLibFile)
{
   printf(" Error - raw files as spectral libraries are not supported yet.\n");
   exit(1);

   MSReader mstReader;

   std::vector<MSSpectrumType> msLevel;

   mstReader.setFilter(msLevel);

   if (g_staticParams.options.iSpecLibMSLevel == 1)
      msLevel.push_back(MS1);
   else if (g_staticParams.options.iSpecLibMSLevel == 2)
      msLevel.push_back(MS2);
   else if (g_staticParams.options.iSpecLibMSLevel == 3)
      msLevel.push_back(MS3);
   else
   {
      std::string strErrorMsg = "Error, MS level not set for the spectral library input.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

/*
   Spectrum mstSpectrum;           // For holding spectrum.

   // Load all input spectra.
   while (true)
   {
      // Loads in MSMS spectrum data.
      if (_bFirstScan)
      {
         PreloadIons(mstReader, mstSpectrum, false, 0);  // Use 0 as scan num here in last argument instead of iFirstScan; must
         _bFirstScan = false;                            // be MS/MS scan else data not read by MSToolkit so safer to start at 0.
      }                                                  // Not ideal as could be reading non-relevant scans but it's fast enough.
      else
      {
         PreloadIons(mstReader, mstSpectrum, true);
      }

      if (iFileLastScan == -1)
      {
         iFileLastScan = mstReader.getLastScan();
         if (iLastScan == 0 && iFileLastScan != -1)
            iLastScan = iFileLastScan;
      }

      if ((iFileLastScan != -1) && (iFileLastScan < iFirstScan))
      {
         _bDoneProcessingAllSpectra = true;
         break;
      }

      iScanNumber = mstSpectrum.getScanNumber();

      if (iLastScan > 0 && iScanNumber > iLastScan)
         break;

      if (g_staticParams.bSkipToStartScan && iScanNumber < iFirstScan)
      {
         g_staticParams.bSkipToStartScan = false;

         PreloadIons(mstReader, mstSpectrum, false, iFirstScan);
         iScanNumber = mstSpectrum.getScanNumber();

         // iScanNumber will equal 0 if iFirstScan is not the right scan level
         // So need to keep reading the next scan until we get a non-zero scan number
         while (iScanNumber == 0 && iFirstScan < iFileLastScan)
         {
            iFirstScan++;
            PreloadIons(mstReader, mstSpectrum, false, iFirstScan);
            iScanNumber = mstSpectrum.getScanNumber();
         }
      }
      if (iScanNumber != 0)
      {
         if (iLastScan > 0 && iScanNumber > iLastScan)
         {
            _bDoneProcessingAllSpectra = true;
            break;
         }
         if (iFirstScan != 0 && iLastScan > 0 && !(iFirstScan <= iScanNumber && iScanNumber <= iLastScan))
            continue;
         if (iFirstScan != 0 && iLastScan == 0 && iScanNumber < iFirstScan)
            continue;

         g_staticParams.bSkipToStartScan = false;
         iTmpCount = iScanNumber;

         // To run a search, all that's needed is MH+ and Z. So need to generate
         // all combinations of these for each spectrum, whether there's a known
         // Z for each precursor or if Comet has to guess the 1+ or 2+/3+ charges.

         for (int i = 0 ; i < mstSpectrum.sizeMZ(); ++i)  // walk through all precursor m/z's; usually just one
         {
            double dMZ = 0.0;              // m/z to use for analysis
            vector<int> vChargeStates;

            if (mstSpectrum.sizeMZ() == mstSpectrum.sizeZ())
            {
               iSpectrumCharge = mstSpectrum.atZ(i).z;
            }
            else if (mstSpectrum.sizeMZ() == 1 && mstSpectrum.sizeMZ() < mstSpectrum.sizeZ())
            {
               // example from ms2 file with one precursor and multiple Z lines?
               // will need to include all spectrum charges below
               iSpectrumCharge = mstSpectrum.atZ(i).z;
            }
            else if (mstSpectrum.sizeMZ() > mstSpectrum.sizeZ())
            {
               // need to ignore any spectrum charge as don't know which correspond charge to which precursor
               iSpectrumCharge = 0;
            }
            else
            {
               // don't know what condition/spectrum type leads here
               iSpectrumCharge = 0;
               printf(" Warning, scan %d has %d precursors and %d precursor charges\n", iScanNumber, mstSpectrum.sizeMZ(), mstSpectrum.sizeZ());
            }

            // Thermo's monoisotopic m/z determine can fail sometimes. Assume that when
            // the mono m/z value is less than selection window, it is wrong and use the
            // selection m/z as the precursor m/z. This should
            // be invoked when searching Thermo raw files and mzML converted from those.
            // Only applied when single precursor present.
            dMZ = mstSpectrum.getMonoMZ(i);

            if (g_staticParams.options.bCorrectMass && mstSpectrum.sizeMZ() == 1)
            {
               double dSelectionLower = mstSpectrum.getSelWindowLower();
               double dSelectedMZ = mstSpectrum.getMZ(i);

               if (dMZ > 0.1 && dSelectionLower > 0.1 && dMZ+0.1 < dSelectionLower)
                  dMZ = dSelectedMZ;
            }

                  for (int i=0; i<mstSpectrum.size(); ++i)
                  {
                     dSumTotal += mstSpectrum.at(i).intensity;

                     if (mstSpectrum.at(i).mz < mstSpectrum.getMZ())
                        dSumBelow += mstSpectrum.at(i).intensity;
                  }


*/

}


// MSP format:
//
// Name: AAAGELQEDSGLMALAK/2_0_30eV
// MW: 1675.8440
// Comment: Single Pep=Tryptic Mods=0 Fullname=K.AAAGELQEDSGLMALAK.L/2 Charge=2 Parent=837.9220 Mz_diff=1.0ppm  HCD=30.00% Scan=81146 Origfile="2018...
// Num peaks: 201
// 101.071	2366.7	"IQA/0.6ppm"
// 102.0552	4117.3	"IEA/2.4ppm"
// 105.0657	730.7	"?"
// 107.4682	612.6	"?"
bool CometSpecLib::ReadSpecLibMSP(std::string strSpecLibFile)
{
   FILE *fp;

   if ( (fp=fopen(strSpecLibFile.c_str(), "r")) == NULL)
   {
      std::string strErrorMsg = "Error, MSP spectral library file cannot be read: '" + strSpecLibFile + "'\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   char szBuf[SIZE_BUF];
   char szTmp[SIZE_BUF];
   int iWhichLibEntry = 0;

   if (fgets(szBuf, SIZE_BUF, fp) == NULL)
   {
      // throw error
   }
   while (!feof(fp))
   {
      szBuf[SIZE_BUF - 1] = '\0'; // terminate string in case line was longer than SIZE_BUF

      if (!strncmp(szBuf, "Name:", 5))
      {
         iWhichLibEntry++;

         if (szBuf[strlen(szBuf) - 1] != '\n') // really long Name: line, parse to newline char
         {
            char cChar;
            cChar = szBuf[strlen(szBuf)-1];
            while (cChar != '\n')
            {
               cChar = getc(fp);
            }
         }
         while (szBuf[strlen(szBuf) - 1] == '\r' || szBuf[strlen(szBuf) - 1] == '\n')
            szBuf[strlen(szBuf) - 1] = '\0';

         sscanf(szBuf + 6, "%s", szTmp);

         struct SpecLibStruct pTmp;
         pTmp.strName = szTmp;
         pTmp.iLibEntry = iWhichLibEntry;
         pTmp.iSpecLibCharge = 0;
         pTmp.dSpecLibMW = 0;

         while (fgets(szBuf, SIZE_BUF, fp))
         {
            if (!strncmp(szBuf, "Name:", 5))
            {
               break;
            }
            else if (!strncmp(szBuf, "MW:", 3))
            {
               // FIX:  in my example, the MW: field encodes (parent m/z * charge)
               sscanf(szBuf+4, "%lf", &(pTmp.dSpecLibMW));
            }
            else if (!strncmp(szBuf, "Comment:", 8))
            {
               char *pStr;

               if ((pStr = strstr(szBuf, " Charge=")) != NULL)
                  sscanf(pStr + 8, "%d", &(pTmp.iSpecLibCharge));
               else
               {
                  printf(" Error: \" Charge=\" string expected but not present in \"Comment:\" line of MSP file.\n");
                  exit(1);
               }
            }
            else if (!strncmp(szBuf, "Num peaks:", 10))
            {
               int iNumPeaks = 0;
               sscanf(szBuf + 11, "%d", &iNumPeaks);

               pTmp.iNumPeaks = iNumPeaks;

               for (int i = 0; i < iNumPeaks; ++i)
               {
                  double dMass;
                  double dInten;

                  if (fgets(szBuf, SIZE_BUF, fp) == NULL)
                  {
                     // throw error
                  }
                  sscanf(szBuf, "%lf %lf %*s", &dMass, &dInten);

                  if (dMass > 0.0 && dMass < 1e6 && dInten > 0.0)  // some sanity check on parsed mass
                     pTmp.vSpecLibPeaks.push_back(std::make_pair(dMass, (float)dInten));
               }

               break;
            }
         }

         if (pTmp.dSpecLibMW == 0.0 || pTmp.iSpecLibCharge == 0 || pTmp.iNumPeaks == 0)
         {
            printf(" Error with MSP entry parsed mass of %lf, charge of %d, and #peaks of %d\n", pTmp.dSpecLibMW, pTmp.iSpecLibCharge, pTmp.iNumPeaks);
            exit(1);
         }

         pTmp.dSpecLibMW -= pTmp.iSpecLibCharge * PROTON_MASS;  // make neutral mass

         // FIX:  do something with the peak list depending on score/processing
         g_vSpecLib.push_back(pTmp);
         size_t iWhichSpecLib = g_vSpecLib.size() - 1;
         SetSpecLibPrecursorIndex(pTmp.dSpecLibMW, pTmp.iSpecLibCharge, iWhichSpecLib);
      }
      else
      {
         if (fgets(szBuf, SIZE_BUF, fp) == NULL)
         {
            // throw error
         }
      }

   }

   fclose(fp);

   return true;
}

// Loads all MS1 spectra from input file into g_vSpecLib.
// Don't bother storing vSpecLibPeaks
bool CometSpecLib::LoadSpecLibMS1Raw(ThreadPool* tp,
                                     const double dMaxQueryRT,
                                     double* dMaxSpecLibRT)
{
   int iFirstScan = 1;
   int iScanNumber = 0;
   int iTotalScans = 0;
   int iNumSpectraLoaded = 0;
   int iTmpCount = 0;

   // For file access using MSToolkit.
   MSReader mstReader;
   MSReader mstReader2;
   Spectrum mstSpectrum;           // For holding spectrum.

   // We want to read only M1 scans.
   std::vector<MSSpectrumType> msLevel;
   msLevel.push_back(MS1);
   msLevel.push_back(MS2);
   msLevel.push_back(MS3);  // need all levels to get last scan RT
   mstReader2.setFilter(msLevel);

   int iAnalysisType = AnalysisType_EntireFile;

   mstReader2.readFile(g_staticParams.speclibInfo.strSpecLibFile.c_str(), mstSpectrum, 1);

   int iFileLastScan = mstReader2.getLastScan();

   mstReader2.readFile(g_staticParams.speclibInfo.strSpecLibFile.c_str(), mstSpectrum, iFileLastScan);
   *dMaxSpecLibRT = mstSpectrum.getRTime() * 60.0;  // convert from minutes to seconds; max RT for query run

   if (*dMaxSpecLibRT == 0.0)
   {
      std::string strErrorMsg = " Error - read dMaxSpecLibRT as " + std::to_string(*dMaxSpecLibRT) + ".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   if (iFileLastScan <= 0)
   {
      std::string strErrorMsg = " Error - read iFileLastScan as " + std::to_string (iFileLastScan) + "%d.\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      return false;
   }

   // Get the thread pool of threads that will preprocess the data.

   ThreadPool* pLoadSpecThreadPool = tp;

   bool bFirstScan = true;
   bool bDoneProcessingAllSpectra = false;

   printf(" - loading MS1 scan (%d, mass range %0.1lf - %0.1lf): ",
      iFileLastScan, g_staticParams.options.dMS1MinMass, g_staticParams.options.dMS1MaxMass);
   fflush(stdout);

   msLevel.clear();
   msLevel.push_back(MS1);  // we want to read only MS1 scans
   mstReader.setFilter(msLevel);

   auto tStartTime = std::chrono::steady_clock::now();

   // Load all input spectra.
   while (true)
   {
      // Loads in MS1 spectrum data.
      if (bFirstScan)
      {
         mstReader.readFile(g_staticParams.speclibInfo.strSpecLibFile.c_str(), mstSpectrum, 0);
         bFirstScan = false;
      }
      else
      {
         mstReader.readFile(NULL, mstSpectrum);
      }

//      if (iFileLastScan == -1)
//         iFileLastScan = mstReader.getLastScan();

      if ((iFileLastScan != -1) && (iFileLastScan < iFirstScan))
      {
         bDoneProcessingAllSpectra = true;
         break;
      }

      iScanNumber = mstSpectrum.getScanNumber();

      if (iScanNumber % 500)
      {
         printf("%3d%%", (int)(100.0 * (double)iScanNumber / iFileLastScan));
         fflush(stdout);
         printf("\b\b\b\b");
      }

      if (iScanNumber != 0)
      {
         iTmpCount = iScanNumber;

         if (iScanNumber > iFileLastScan)
         {
            bDoneProcessingAllSpectra = true;
            break;
         }

         if (mstSpectrum.size() >= g_staticParams.options.iMinPeaks)
         {
            if (iScanNumber > iFileLastScan)
            {
               bDoneProcessingAllSpectra = true;
               break;
            }

            PreprocessThreadData* pPreprocessThreadDataMS1 = new PreprocessThreadData(mstSpectrum, iAnalysisType, iFileLastScan);

            pLoadSpecThreadPool->doJob(std::bind(CometPreprocess::PreprocessThreadProcMS1, pPreprocessThreadDataMS1, pLoadSpecThreadPool, dMaxQueryRT, *dMaxSpecLibRT));
         }

         iTotalScans++;
      }
      else if (CometPreprocess::IsValidInputType(g_staticParams.inputFile.iInputType))
      {
         bDoneProcessingAllSpectra = true;
         break;
      }
      else
      {
         iTmpCount++;

         if (iTmpCount > iFileLastScan)
         {
            bDoneProcessingAllSpectra = true;
            break;
         }
      }

      Threading::LockMutex(g_pvQueryMutex);

      iNumSpectraLoaded = (int)g_vSpecLib.size();

      if (CometPreprocess::CheckExit(iAnalysisType,
                                     iScanNumber,
                                     iTotalScans,
                                     iFileLastScan,
                                     mstReader.getLastScan(),
                                     iNumSpectraLoaded,
                                     1))
      {
         Threading::UnlockMutex(g_pvQueryMutex);
         break;
      }
      else
      {
         Threading::UnlockMutex(g_pvQueryMutex);
      }
   }

   // Wait for active preprocess threads to complete processing.
   pLoadSpecThreadPool->wait_on_threads();

   bool bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();

   std::cout << "100% (" << CometMassSpecUtils::ElapsedTime(tStartTime) << ")" << std::endl;

   g_bSpecLibRead = true;
//   mstReader.closeFile();   //FIX when does this get closed?

   printf("\n"); fflush(stdout);

   return bSucceeded;
}


double CometSpecLib::ScoreSpecLib(Query *pQuery,
                                  unsigned int iWhichSpecLib)
{
   double dScore = 0.0;
   int x, y, bin;

   int iMax = pQuery->_spectrumInfoInternal.iArraySize/SPARSE_MATRIX_SIZE;

   for (auto it = g_vSpecLib.at(iWhichSpecLib).vSpecLibPeaks.begin(); it != g_vSpecLib.at(iWhichSpecLib).vSpecLibPeaks.end() ; ++it)
   {
      bin = BIN(it->first);
      x = bin / SPARSE_MATRIX_SIZE;

      if (!(bin <= 0 || x>iMax || pQuery->ppfSparseFastXcorrData[x] == NULL))
      {
         y = bin - (x * SPARSE_MATRIX_SIZE);
         dScore += pQuery->ppfSparseFastXcorrData[x][y];
      }
   }

   dScore = std::round(dScore * 0.005 * 1000.0) / 1000.0;  // round to 3 decimal points

   return dScore;
}


// For each spec lib mass "bin", set g_vulSpecLibPrecursorIndex which is a vector of all
// SpecLib entries that are matched to that "bin". This allows a mass query to walk through
// and score against all entries in the vector.
void CometSpecLib::SetSpecLibPrecursorIndex(double dNeutralMass,
                                            int iSpecLibCharge,
                                            size_t iWhichSpecLib)
{
   double dProtonatedMass = dNeutralMass + PROTON_MASS;

   double dToleranceLow = 0;
   double dToleranceHigh = 0;

   int iMaxBin = BINPREC(g_staticParams.options.dPeptideMassHigh);
   int iPrecursorCharge = g_vSpecLib.at(iWhichSpecLib).iSpecLibCharge;

   if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
   {
      dToleranceLow  = g_staticParams.tolerances.dInputToleranceMinus;
      dToleranceHigh = g_staticParams.tolerances.dInputTolerancePlus;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
      {
         dToleranceLow  *= iPrecursorCharge;
         dToleranceHigh *= iPrecursorCharge;
      }
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
   {
      dToleranceLow  = g_staticParams.tolerances.dInputToleranceMinus * 0.001;
      dToleranceHigh = g_staticParams.tolerances.dInputTolerancePlus  * 0.001;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
      {
         dToleranceLow  *= iPrecursorCharge;
         dToleranceHigh *= iPrecursorCharge;
      }
   }

   // tolerances are fixed above except if ppm is specified
   else if (g_staticParams.tolerances.iMassToleranceUnits == 2) // ppm
   {
      dToleranceLow  = g_staticParams.tolerances.dInputToleranceMinus * dProtonatedMass / 1E6;
      dToleranceHigh = g_staticParams.tolerances.dInputTolerancePlus * dProtonatedMass / 1E6;
   }
   else
   {
      std::string strErrorMsg = " Error - peptide_mass_units must be 0, 1 or 2. Value set is "
         + std::to_string(g_staticParams.tolerances.iMassToleranceUnits) + ".\n";
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(strErrorMsg);
      exit(1);
//    return;
   }

   // these are the range of neutral mass bins if any theoretical peptide falls into,
   // we want to add them to the fragment index
   double dMassLow = dProtonatedMass + dToleranceLow;
   double dMassHigh = dProtonatedMass + dToleranceHigh;
   int iStart = BINPREC(dMassLow);  // add dToleranceLow as it will be negative number
   int iEnd   = BINPREC(dMassHigh);

   if (iStart < 0)
      iStart = 0;   // real problems if we actually get here
   if (iEnd > iMaxBin)
      iEnd = iMaxBin;

   for (int x = iStart ; x <= iEnd; ++x)
      g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

   // now go through each isotope offset
   if (g_staticParams.tolerances.iIsotopeError > 0)
   {
      if (g_staticParams.tolerances.iIsotopeError >= 1
            && g_staticParams.tolerances.iIsotopeError <= 6)
      {
         iStart = BINPREC(dMassLow - C13_DIFF * PROTON_MASS);     // do +1 offset
         iEnd   = BINPREC(dMassHigh - C13_DIFF * PROTON_MASS);

         if (iStart < 0)
            iStart = 0;
         if (iEnd > iMaxBin)
            iEnd = iMaxBin;
         for (int x = iStart ; x <= iEnd; ++x)
            g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

         if (g_staticParams.tolerances.iIsotopeError >= 2
               && g_staticParams.tolerances.iIsotopeError <= 6
               && g_staticParams.tolerances.iIsotopeError != 5)
         {
            iStart = BINPREC(dMassLow - 2.0 * C13_DIFF * PROTON_MASS);     // do +2 offset
            iEnd   = BINPREC(dMassHigh - 2.0 * C13_DIFF * PROTON_MASS);

            if (iStart < 0)
               iStart = 0;
            if (iEnd > iMaxBin)
               iEnd = iMaxBin;

            for (int x = iStart ; x <= iEnd; ++x)
               g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

            if (g_staticParams.tolerances.iIsotopeError >= 3
                  && g_staticParams.tolerances.iIsotopeError <= 6
                  && g_staticParams.tolerances.iIsotopeError != 5)
            {
               iStart = BINPREC(dMassLow - 3.0 * C13_DIFF * PROTON_MASS);     // do +3 offset
               iEnd   = BINPREC(dMassHigh - 3.0 * C13_DIFF * PROTON_MASS);

               if (iStart < 0)
                  iStart = 0;
               if (iEnd > iMaxBin)
                  iEnd = iMaxBin;

               for (int x = iStart ; x <= iEnd; ++x)
                  g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);
            }
         }
      }

      if (g_staticParams.tolerances.iIsotopeError == 5
            || g_staticParams.tolerances.iIsotopeError == 6)
      {
         iStart = BINPREC(dMassLow + C13_DIFF * PROTON_MASS);      // do -1 offset
         iEnd   = BINPREC(dMassHigh + C13_DIFF * PROTON_MASS);

         if (iStart < 0)
            iStart = 0;
         if (iEnd > iMaxBin)
            iEnd = iMaxBin;

         for (int x = iStart ; x <= iEnd; ++x)
            g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

         if (g_staticParams.tolerances.iIsotopeError == 6)     // do -2 and -3 offsets
         {
            iStart = BINPREC(dMassLow + 2.0 * C13_DIFF * PROTON_MASS);
            iEnd   = BINPREC(dMassHigh + 2.0 * C13_DIFF * PROTON_MASS);

            if (iStart < 0)
               iStart = 0;
            if (iEnd > iMaxBin)
               iEnd = iMaxBin;

            for (int x = iStart ; x <= iEnd; ++x)
               g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

            iStart = BINPREC(dMassLow + 3.0 * C13_DIFF * PROTON_MASS);
            iEnd   = BINPREC(dMassHigh + 3.0 * C13_DIFF * PROTON_MASS);

            if (iStart < 0)
               iStart = 0;
            if (iEnd > iMaxBin)
               iEnd = iMaxBin;

            for (int x = iStart ; x <= iEnd; ++x)
               g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);
         }
      }
      else if (g_staticParams.tolerances.iIsotopeError == 7)            // do -8, -4, +4, +8 offsets
      {
         iStart = BINPREC(dMassLow + 8.0 * C13_DIFF * PROTON_MASS);
         iEnd   = BINPREC(dMassHigh + 8.0 * C13_DIFF * PROTON_MASS);

         if (iStart < 0)
            iStart = 0;
         if (iEnd > iMaxBin)
            iEnd = iMaxBin;

         for (int x = iStart ; x <= iEnd; ++x)
            g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

         iStart = BINPREC(dMassLow + 4.0 * C13_DIFF * PROTON_MASS);
         iEnd   = BINPREC(dMassHigh + 4.0 * C13_DIFF * PROTON_MASS);

         if (iStart < 0)
            iStart = 0;
         if (iEnd > iMaxBin)
            iEnd = iMaxBin;

         for (int x = iStart ; x <= iEnd; ++x)
            g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

         iStart = BINPREC(dMassLow - 8.0 * C13_DIFF * PROTON_MASS);
         iEnd   = BINPREC(dMassHigh - 8.0 * C13_DIFF * PROTON_MASS);

         if (iStart < 0)
            iStart = 0;
         if (iEnd > iMaxBin)
            iEnd = iMaxBin;

         for (int x = iStart ; x <= iEnd; ++x)
            g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);

         iStart = BINPREC(dMassLow - 4.0 * C13_DIFF * PROTON_MASS);
         iEnd   = BINPREC(dMassHigh - 4.0 * C13_DIFF * PROTON_MASS);

         if (iStart < 0)
            iStart = 0;
         if (iEnd > iMaxBin)
            iEnd = iMaxBin;

         for (int x = iStart ; x <= iEnd; ++x)
            g_vulSpecLibPrecursorIndex.at(x).push_back((unsigned int)iWhichSpecLib);
      }
   }
}


void CometSpecLib::StoreSpecLib(Query *it,
                                unsigned int iWhichSpecLib,
                                double dSpecLibScore)
{
   Query *pQuery = it;

   pQuery->fLowestSpecLibScore = pQuery->_pSpecLibResults[0].fSpecLibScore;

   short int siLowestSpecLibScoreIndex = 0;

   // Get new lowest score
   for (int i = g_staticParams.options.iNumStored - 1; i > 0; --i)
   {
      if (pQuery->_pSpecLibResults[i].fSpecLibScore < pQuery->fLowestSpecLibScore)
      {
         pQuery->fLowestSpecLibScore = pQuery->_pSpecLibResults[i].fSpecLibScore;
         siLowestSpecLibScoreIndex = i;
      }

      if (pQuery->_pSpecLibResults[i].fSpecLibScore == SPECLIB_CUTOFF)
         break;
   }

   pQuery->_pSpecLibResults[siLowestSpecLibScoreIndex].iWhichSpecLib = iWhichSpecLib;
   pQuery->_pSpecLibResults[siLowestSpecLibScoreIndex].fSpecLibScore = (float)dSpecLibScore;

   pQuery->fLowestSpecLibScore = pQuery->_pSpecLibResults[0].fSpecLibScore;
   for (int i = g_staticParams.options.iNumStored - 1; i > 0; --i)
   {
      if (pQuery->_pSpecLibResults[i].fSpecLibScore < pQuery->fLowestSpecLibScore)
         pQuery->fLowestSpecLibScore = pQuery->_pSpecLibResults[i].fSpecLibScore;

      if (pQuery->_pSpecLibResults[i].fSpecLibScore == SPECLIB_CUTOFF)
         break;
   }
}



// Function to decode BLOB data as an array of 8-byte floats
std::vector<double> CometSpecLib::decodeBlob(const void* blob, int size)
{
   std::vector<double> result;

   const double* data = static_cast<const double*>(blob);

   int numDoubles = size / sizeof(double);

   for (int i = 0; i < numDoubles; ++i)
      result.push_back(data[i]);

   return result;
}


// Function to print the contents of a vector of doubles
void CometSpecLib::printDoubleVector(const std::vector<double>& vec)
{
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        std::cout << vec[i];

        if (i < vec.size() - 1)
        {
            std::cout << ", ";
        }

        if (i == 9)
        {
           printf(" ...");
           break;
        }
    }
    std::cout << "]" << std::endl;
}
