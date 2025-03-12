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
#include "CometDataInternal.h"
#include "CometPreprocess.h"
#include "CometStatus.h"

Mutex CometPreprocess::_maxChargeMutex;
bool CometPreprocess::_bDoneProcessingAllSpectra;
bool CometPreprocess::_bFirstScan;
bool *CometPreprocess::pbMemoryPool;
double **CometPreprocess::ppdTmpRawDataArr;
double **CometPreprocess::ppdTmpFastXcorrDataArr;
double **CometPreprocess::ppdTmpCorrelationDataArr;

// Generate data for both sp scoring (pfSpScoreData) and xcorr analysis (FastXcorr).
CometPreprocess::CometPreprocess()
{
}

CometPreprocess::~CometPreprocess()
{
}

void CometPreprocess::Reset()
{
    _bFirstScan = true;
    _bDoneProcessingAllSpectra = false;
}

bool CometPreprocess::ReadPrecursors(MSReader &mstReader)
{
   int iFileLastScan = -1;         // The actual last scan in the file.
   int iFirstScan = 0;
   int iLastScan = 0;
   int iTmpCount = 0;
   int iScanNumber = 0;
   int iSpectrumCharge;
   Spectrum mstSpectrum;           // For holding spectrum.

   int iMaxBin = BIN(g_staticParams.options.dPeptideMassHigh);

   if (g_staticParams.options.scanRange.iStart != 0)
      iFirstScan = g_staticParams.options.scanRange.iStart;

   if (g_staticParams.options.scanRange.iEnd != 0)
      iLastScan = g_staticParams.options.scanRange.iEnd;

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

            if (dMZ == 0)
               dMZ = mstSpectrum.getMZ(i);

            // 1.  Have spectrum charge from file.  It may be 0.
            // 2.  If the precursor_charge range is set and override_charge is set, then do something look into charge range.
            // 3.  Else just use the spectrum charge (and possibly 1 or 2/3 rule)
            if (g_staticParams.options.iStartCharge > 0 && g_staticParams.options.bOverrideCharge > 0)
            {
               if (g_staticParams.options.bOverrideCharge == 1)
               {
                  // ignore spectrum charge and use precursor_charge range
                  for (int z = g_staticParams.options.iStartCharge; z <= g_staticParams.options.iEndCharge; ++z)
                  {
                     vChargeStates.push_back(z);
                  }
               }
               else if (g_staticParams.options.bOverrideCharge == 2)
               {
                  // only use spectrum charge if it's within the charge range
                  for (int z = g_staticParams.options.iStartCharge; z <= g_staticParams.options.iEndCharge; ++z)
                  {
                     if (z == iSpectrumCharge)
                        vChargeStates.push_back(z);
                  }
               }
               else if (g_staticParams.options.bOverrideCharge == 3)
               {
                  if (iSpectrumCharge > 0)
                  {
                     vChargeStates.push_back(iSpectrumCharge);
                  }
                  else // use 1+ or charge range
                  {
                     double dSumBelow = 0.0;
                     double dSumTotal = 0.0;

                     for (int i=0; i<mstSpectrum.size(); ++i)
                     {
                        dSumTotal += mstSpectrum.at(i).intensity;
                        if (mstSpectrum.at(i).mz < mstSpectrum.getMZ())
                           dSumBelow += mstSpectrum.at(i).intensity;
                     }

                     if (isEqual(dSumTotal, 0.0) || ((dSumBelow/dSumTotal) > 0.95))
                     {
                        vChargeStates.push_back(1);
                     }
                     else
                     {
                        for (int z = g_staticParams.options.iStartCharge; z <= g_staticParams.options.iEndCharge; ++z)
                        {
                           vChargeStates.push_back(z);
                        }
                     }
                  }
               }
            }
            else  // use spectrum charge
            {
               if (iSpectrumCharge > 0) // use charge from file
               {
                  vChargeStates.push_back(iSpectrumCharge);

                  // add in any other charge states for the single precursor m/z
                  if (mstSpectrum.sizeMZ() == 1 && mstSpectrum.sizeMZ() < mstSpectrum.sizeZ())
                  {
                     for (int ii = 1 ; ii < mstSpectrum.sizeZ(); ++ii)
                     {
                        vChargeStates.push_back(mstSpectrum.atZ(ii).z);
                     }
                  }
               }
               else
               {
                  double dSumBelow = 0.0;
                  double dSumTotal = 0.0;

                  for (int i=0; i<mstSpectrum.size(); ++i)
                  {
                     dSumTotal += mstSpectrum.at(i).intensity;

                     if (mstSpectrum.at(i).mz < mstSpectrum.getMZ())
                        dSumBelow += mstSpectrum.at(i).intensity;
                  }

                  if (isEqual(dSumTotal, 0.0) || ((dSumBelow/dSumTotal) > 0.95))
                  {
                     vChargeStates.push_back(1);
                  }
                  else
                  {
                     vChargeStates.push_back(2);
                     vChargeStates.push_back(3);
                  }
               }
            }

            // now analyze all possible precursor charges for this spectrum
            for (vector<int>::iterator iter = vChargeStates.begin(); iter != vChargeStates.end(); ++iter)
            {
               int iPrecursorCharge = *iter;
               double dProtonatedMass = (dMZ * iPrecursorCharge) - (iPrecursorCharge * PROTON_MASS)  + PROTON_MASS;

               double dToleranceLow = 0;
               double dToleranceHigh = 0;
   
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
                  char szErrorMsg[256];
                  sprintf(szErrorMsg,  " Error - peptide_mass_units must be 0, 1 or 2. Value set is %d.\n", g_staticParams.tolerances.iMassToleranceUnits);
                  string strErrorMsg(szErrorMsg);
                  g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                  logerr(szErrorMsg);
                  return false;
               }

               // these are the range of neutral mass bins if any theoretical peptide falls into,
               // we want to add them to the fragment index
               double dMassLow = dProtonatedMass + dToleranceLow;
               double dMassHigh = dProtonatedMass + dToleranceHigh;
               int iStart = BIN(dMassLow);  // add dToleranceLow as it will be negative number
               int iEnd   = BIN(dMassHigh);

               if (iStart < 0)
                  iStart = 0;   // real problems if we actually get here
               if (iEnd > iMaxBin)
                  iEnd = iMaxBin;

               for (int x = iStart ; x <= iEnd; ++x)
                  g_bIndexPrecursors[x] = true;

               // now go through each isotope offset
               if (g_staticParams.tolerances.iIsotopeError > 0)
               {
                  if (g_staticParams.tolerances.iIsotopeError >= 1
                        && g_staticParams.tolerances.iIsotopeError <= 6)
                  {
                     iStart = BIN(dMassLow - C13_DIFF * PROTON_MASS);     // do +1 offset
                     iEnd   = BIN(dMassHigh - C13_DIFF * PROTON_MASS);
                     if (iStart < 0)
                        iStart = 0;
                     if (iEnd > iMaxBin)
                        iEnd = iMaxBin;
                     for (int x = iStart ; x <= iEnd; ++x)
                        g_bIndexPrecursors[x] = true;

                     if (g_staticParams.tolerances.iIsotopeError >= 2
                           && g_staticParams.tolerances.iIsotopeError <= 6
                           && g_staticParams.tolerances.iIsotopeError != 5)
                     {
                        iStart = BIN(dMassLow - 2.0 * C13_DIFF * PROTON_MASS);     // do +2 offset
                        iEnd   = BIN(dMassHigh - 2.0 * C13_DIFF * PROTON_MASS);
                        if (iStart < 0)
                           iStart = 0;
                        if (iEnd > iMaxBin)
                           iEnd = iMaxBin;
                        for (int x = iStart ; x <= iEnd; ++x)
                           g_bIndexPrecursors[x] = true;

                        if (g_staticParams.tolerances.iIsotopeError >= 3
                              && g_staticParams.tolerances.iIsotopeError <= 6
                              && g_staticParams.tolerances.iIsotopeError != 5)
                        {
                           iStart = BIN(dMassLow - 3.0 * C13_DIFF * PROTON_MASS);     // do +3 offset
                           iEnd   = BIN(dMassHigh - 3.0 * C13_DIFF * PROTON_MASS);
                           if (iStart < 0)
                              iStart = 0;
                           if (iEnd > iMaxBin)
                              iEnd = iMaxBin;
                           for (int x = iStart ; x <= iEnd; ++x)
                              g_bIndexPrecursors[x] = true;
                        }
                     }
                  }

                  if (g_staticParams.tolerances.iIsotopeError == 5
                        || g_staticParams.tolerances.iIsotopeError == 6)
                  {         
                     iStart = BIN(dMassLow + C13_DIFF * PROTON_MASS);      // do -1 offset
                     iEnd   = BIN(dMassHigh + C13_DIFF * PROTON_MASS);
                     if (iStart < 0)
                        iStart = 0;
                     if (iEnd > iMaxBin)
                        iEnd = iMaxBin;
                     for (int x = iStart ; x <= iEnd; ++x)
                        g_bIndexPrecursors[x] = true;

                     if (g_staticParams.tolerances.iIsotopeError == 6)     // do -2 and -3 offsets
                     {
                        iStart = BIN(dMassLow + 2.0 * C13_DIFF * PROTON_MASS);
                        iEnd   = BIN(dMassHigh + 2.0 * C13_DIFF * PROTON_MASS);
                        if (iStart < 0)
                           iStart = 0;
                        if (iEnd > iMaxBin)
                           iEnd = iMaxBin;
                        for (int x = iStart ; x <= iEnd; ++x)
                           g_bIndexPrecursors[x] = true;

                        iStart = BIN(dMassLow + 3.0 * C13_DIFF * PROTON_MASS);
                        iEnd   = BIN(dMassHigh + 3.0 * C13_DIFF * PROTON_MASS);
                        if (iStart < 0)
                           iStart = 0;
                        if (iEnd > iMaxBin)
                           iEnd = iMaxBin;
                        for (int x = iStart ; x <= iEnd; ++x)
                           g_bIndexPrecursors[x] = true;
                     }
                  }
                  else if (g_staticParams.tolerances.iIsotopeError == 7)            // do -8, -4, +4, +8 offsets
                  {
                     iStart = BIN(dMassLow + 8.0 * C13_DIFF * PROTON_MASS);
                     iEnd   = BIN(dMassHigh + 8.0 * C13_DIFF * PROTON_MASS);
                     if (iStart < 0)
                        iStart = 0;
                     if (iEnd > iMaxBin)
                        iEnd = iMaxBin;
                     for (int x = iStart ; x <= iEnd; ++x)
                        g_bIndexPrecursors[x] = true;

                     iStart = BIN(dMassLow + 4.0 * C13_DIFF * PROTON_MASS);
                     iEnd   = BIN(dMassHigh + 4.0 * C13_DIFF * PROTON_MASS);
                     if (iStart < 0)
                        iStart = 0;
                     if (iEnd > iMaxBin)
                        iEnd = iMaxBin;
                     for (int x = iStart ; x <= iEnd; ++x)
                        g_bIndexPrecursors[x] = true;

                     iStart = BIN(dMassLow - 8.0 * C13_DIFF * PROTON_MASS);
                     iEnd   = BIN(dMassHigh - 8.0 * C13_DIFF * PROTON_MASS);
                     if (iStart < 0)
                        iStart = 0;
                     if (iEnd > iMaxBin)
                        iEnd = iMaxBin;
                     for (int x = iStart ; x <= iEnd; ++x)
                        g_bIndexPrecursors[x] = true;

                     iStart = BIN(dMassLow - 4.0 * C13_DIFF * PROTON_MASS);
                     iEnd   = BIN(dMassHigh - 4.0 * C13_DIFF * PROTON_MASS);
                     if (iStart < 0)
                        iStart = 0;
                     if (iEnd > iMaxBin)
                        iEnd = iMaxBin;
                     for (int x = iStart ; x <= iEnd; ++x)
                        g_bIndexPrecursors[x] = true;
                  }
               }
            }
         }
      }
      else
      {
         // What happens here when iScanNumber == 0 and it is an mzXML file?
         // Best way to deal with this is to keep trying to read but track each
         // attempt and break when the count goes past the mzXML's last scan.
         iTmpCount++;

         if (iTmpCount > iFileLastScan)
            break;
      }
   }

// mstReader.closeFile();

   return true;
}


bool CometPreprocess::LoadAndPreprocessSpectra(MSReader &mstReader,
                                               int iFirstScan,
                                               int iLastScan,
                                               int iAnalysisType,
                                               ThreadPool* tp)
{
   int iFileLastScan = -1;         // The actual last scan in the file.
   int iScanNumber = 0;
   int iTotalScans = 0;
   int iNumSpectraLoaded = 0;
   int iTmpCount = 0;
   Spectrum mstSpectrum;           // For holding spectrum.

   g_massRange.iMaxFragmentCharge = 0;
   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dH2O);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dNH3);

   // Create the mutex we will use to protect g_massRange.iMaxFragmentCharge.
   Threading::CreateMutex(&_maxChargeMutex);

   // Get the thread pool of threads that will preprocess the data.

   ThreadPool *pPreprocessThreadPool = tp;

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
         iFileLastScan = mstReader.getLastScan();

      if ((iFileLastScan != -1) && (iFileLastScan < iFirstScan))
      {
         _bDoneProcessingAllSpectra = true;
         break;
      }

      iScanNumber = mstSpectrum.getScanNumber();

      if (g_staticParams.bSkipToStartScan && iScanNumber < iFirstScan)
      {
         g_staticParams.bSkipToStartScan = false;

         PreloadIons(mstReader, mstSpectrum, false, iFirstScan);
         iScanNumber = mstSpectrum.getScanNumber();

         // iScanNumber will equal 0 if iFirstScan is not the right scan level
         // So need to keep reading the next scan until we get a non-zero scan number
         while (iScanNumber == 0 && iFirstScan < iLastScan)
         {
            iFirstScan++;
            PreloadIons(mstReader, mstSpectrum, false, iFirstScan);
            iScanNumber = mstSpectrum.getScanNumber();
         }
      }

      if (iScanNumber != 0)
      {
         int iNumClearedPeaks = 0;

         iTmpCount = iScanNumber;

         // iFirstScan and iLastScan are both 0 unless scan range is specified.
         // If -L last scan specified, iFirstScan is set to 1
         // However, if -F is specified, iLastscan is still 0
         // If scan range is specified, need to enforce here.
         if (iLastScan != 0 && iScanNumber > iLastScan)
         {
            _bDoneProcessingAllSpectra = true;
            break;
         }
         if (iFirstScan != 0 && iLastScan != 0 && !(iFirstScan <= iScanNumber && iScanNumber <= iLastScan))
            continue;
         if (iFirstScan != 0 && iLastScan == 0 && iScanNumber < iFirstScan)
            continue;

         // Clear out m/z range if clear_mz_range parameter is specified
         // Accomplish this by setting corresponding intensity to 0
         if (g_staticParams.options.clearMzRange.dEnd > 0.0
               && g_staticParams.options.clearMzRange.dStart <= g_staticParams.options.clearMzRange.dEnd)
         {
            int i=0;

            while (true)
            {
                if (i >= mstSpectrum.size() || mstSpectrum.at(i).mz > g_staticParams.options.clearMzRange.dEnd)
                   break;

                if (mstSpectrum.at(i).mz >= g_staticParams.options.clearMzRange.dStart
                      && mstSpectrum.at(i).mz <= g_staticParams.options.clearMzRange.dEnd)
                {
                   mstSpectrum.at(i).intensity = 0.0;
                   iNumClearedPeaks++;
                }

               i++;
            }
         }

         if (mstSpectrum.size() - iNumClearedPeaks >= g_staticParams.options.iMinPeaks)
         {
            if (iAnalysisType == AnalysisType_SpecificScanRange && iLastScan > 0 && iScanNumber > iLastScan)
            {
               _bDoneProcessingAllSpectra = true;
               break;
            }

            if (CheckActivationMethodFilter(mstSpectrum.getActivationMethod()))
            {
               // add this hack when 1 thread is specified otherwise g_pvQuery.size() returns 0
               if (g_staticParams.options.iNumThreads == 1)
                  pPreprocessThreadPool->wait_on_threads();

               Threading::LockMutex(g_pvQueryMutex);
               // this needed because processing can add multiple spectra at a time
               iNumSpectraLoaded = (int)g_pvQuery.size();
               iNumSpectraLoaded++;
               Threading::UnlockMutex(g_pvQueryMutex);

               pPreprocessThreadPool->wait_for_available_thread();

               //-->MH
               //If there are no Z-lines, filter the spectrum for charge state
               //run filter here.

               PreprocessThreadData *pPreprocessThreadData = new PreprocessThreadData(mstSpectrum, iAnalysisType, iFileLastScan);

               pPreprocessThreadPool->doJob(std::bind(PreprocessThreadProc, pPreprocessThreadData, pPreprocessThreadPool));
            }
         }

         iTotalScans++;
      }
      else if (IsValidInputType(g_staticParams.inputFile.iInputType))
      {
         _bDoneProcessingAllSpectra = true;
         break;
      }
      else
      {
         // What happens here when iScanNumber == 0 and it is an mzXML file?
         // Best way to deal with this is to keep trying to read but track each
         // attempt and break when the count goes past the mzXML's last scan.
         iTmpCount++;

         if (iTmpCount > iFileLastScan)
         {
            _bDoneProcessingAllSpectra = true;
            break;
         }
      }

      Threading::LockMutex(g_pvQueryMutex);

      if (CheckExit(iAnalysisType,
                    iScanNumber,
                    iTotalScans,
                    iLastScan,
                    mstReader.getLastScan(),
                    iNumSpectraLoaded))
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
   pPreprocessThreadPool->wait_on_threads();

   Threading::DestroyMutex(_maxChargeMutex);

   bool bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();

   return bSucceeded;
}


void CometPreprocess::PreprocessThreadProc(PreprocessThreadData *pPreprocessThreadData,
                                           ThreadPool* tp)
{
   // This returns false if it fails, but the errors are already logged
   // so no need to check the return value here.

   //MH: Grab available array from shared memory pool.
   int i;

   Threading::LockMutex(g_preprocessMemoryPoolMutex);
   for (i=0; i<g_staticParams.options.iNumThreads; ++i)
   {
      if (pbMemoryPool[i]==false)
      {
         pbMemoryPool[i]=true;
         break;
      }
   }
   Threading::UnlockMutex(g_preprocessMemoryPoolMutex);

   //MH: Fail-safe to stop if memory isn't available for the next thread.
   //Needs better capture and return?
   if (i==g_staticParams.options.iNumThreads)
   {
      printf("Error with memory pool in preprocessing.\n");
      exit(1);
   }

   //MH: Give memory manager access to the thread.
   pPreprocessThreadData->SetMemory(&pbMemoryPool[i]);

   PreprocessSpectrum(pPreprocessThreadData->mstSpectrum,
         ppdTmpRawDataArr[i],
         ppdTmpFastXcorrDataArr[i],
         ppdTmpCorrelationDataArr[i]);

   delete pPreprocessThreadData;
   pPreprocessThreadData = NULL;
}


bool CometPreprocess::DoneProcessingAllSpectra()
{
   return _bDoneProcessingAllSpectra;
}


bool CometPreprocess::Preprocess(struct Query *pScoring,
                                 Spectrum mstSpectrum,
                                 double *pdTmpRawData,
                                 double *pdTmpFastXcorrData,
                                 double *pdTmpCorrelationData)
{
   int i;
   int x;
   int y;
   struct PreprocessStruct pPre;

   pPre.iHighestIon = 0;
   pPre.dHighestIntensity = 0;

   // initialize these temporary arrays before re-using
   size_t iTmp = (size_t)(g_staticParams.iArraySizeGlobal * sizeof(double));
   memset(pdTmpRawData, 0, iTmp);
   memset(pdTmpFastXcorrData, 0, iTmp);
   memset(pdTmpCorrelationData, 0, iTmp);

   // pdTmpRawData is a binned array holding raw data
   if (!LoadIons(pScoring, pdTmpRawData, mstSpectrum, &pPre))
   {
      return false;
   }

   // get mzML nativeID
   char szNativeID[SIZE_NATIVEID];
   if (mstSpectrum.getNativeID(szNativeID, SIZE_NATIVEID))
      strcpy(pScoring->_spectrumInfoInternal.szNativeID, szNativeID);
   else
      pScoring->_spectrumInfoInternal.szNativeID[0]='\0';

   try
   {
      pScoring->pfFastXcorrData = new float[pScoring->_spectrumInfoInternal.iArraySize]();
   }
   catch (std::bad_alloc& ba)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - new(pfFastXcorrData[%d]). bad_alloc: %s.\n", pScoring->_spectrumInfoInternal.iArraySize, ba.what());
      sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {
      try
      {
         pScoring->pfFastXcorrDataNL = new float[pScoring->_spectrumInfoInternal.iArraySize]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pfFastXcorrDataNL[%d]). bad_alloc: %s.\n", pScoring->_spectrumInfoInternal.iArraySize, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }
   }

   // Create data for correlation analysis.
   // pdTmpRawData intensities are normalized to 100; pdTmpCorrelationData is windowed
   MakeCorrData(pdTmpRawData, pdTmpCorrelationData, pScoring, &pPre);

   // Make fast xcorr spectrum.
   double dSum = 0.0;
   int iTmpRange = 2 * g_staticParams.iXcorrProcessingOffset + 1;
   double dTmp = 1.0 / (iTmpRange - 1.0);
   double dMinXcorrInten = 0.0;

   dSum = 0.0;
   for (i = 0; i < g_staticParams.iXcorrProcessingOffset; ++i)
      dSum += pdTmpCorrelationData[i];
   for (i = g_staticParams.iXcorrProcessingOffset; i < pScoring->_spectrumInfoInternal.iArraySize + g_staticParams.iXcorrProcessingOffset; ++i)
   {
      if (dMinXcorrInten < pdTmpCorrelationData[i])
         dMinXcorrInten = pdTmpCorrelationData[i];

      if (i < pScoring->_spectrumInfoInternal.iArraySize)
         dSum += pdTmpCorrelationData[i];
      if (i >= iTmpRange)
         dSum -= pdTmpCorrelationData[i - iTmpRange];
      pdTmpFastXcorrData[i - g_staticParams.iXcorrProcessingOffset] = (dSum - pdTmpCorrelationData[i - g_staticParams.iXcorrProcessingOffset]) * dTmp;
   }

   pScoring->iMinXcorrHisto = (int)(dMinXcorrInten * 10.0 * 0.005 + 0.5);

   pScoring->pfFastXcorrData[0] = 0.0;
   for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      double dTmp = pdTmpCorrelationData[i] - pdTmpFastXcorrData[i];

      pScoring->pfFastXcorrData[i] = (float)dTmp;

      // Add flanking peaks if used
      if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0)
      {
         int iTmp;

         iTmp = i-1;
         pScoring->pfFastXcorrData[i] += (float) ((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp])*0.5);

         iTmp = i+1;
         if (iTmp < pScoring->_spectrumInfoInternal.iArraySize)
            pScoring->pfFastXcorrData[i] += (float) ((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp])*0.5);
      }

      // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL
      if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
            && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
      {
         int iTmp;

         pScoring->pfFastXcorrDataNL[i] = pScoring->pfFastXcorrData[i];

         iTmp = i-g_staticParams.precalcMasses.iMinus17;
         if (iTmp>= 0)
         {
            pScoring->pfFastXcorrDataNL[i] += (float)((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp]) * 0.2);
         }

         iTmp = i-g_staticParams.precalcMasses.iMinus18;
         if (iTmp>= 0)
         {
            pScoring->pfFastXcorrDataNL[i] += (float)((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp]) * 0.2);
         }
      }
   }

   pScoring->iFastXcorrDataSize = (pScoring->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE) + 1;

   // Using sparse matrix which means we free pScoring->pfFastXcorrData, ->pfFastXcorrDataNL here
   // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL.
   if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {

      try
      {
         pScoring->ppfSparseFastXcorrDataNL = new float*[pScoring->iFastXcorrDataSize]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrDataNL[%d]). bad_alloc: %s.", pScoring->iFastXcorrDataSize, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr( szErrorMsg);
         return false;
      }

      for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
      {
         if (pScoring->pfFastXcorrDataNL[i]>FLOAT_ZERO || pScoring->pfFastXcorrDataNL[i]<-FLOAT_ZERO)
         {
            x=i/SPARSE_MATRIX_SIZE;
            if (pScoring->ppfSparseFastXcorrDataNL[x]==NULL)
            {
               try
               {
                  pScoring->ppfSparseFastXcorrDataNL[x] = new float[SPARSE_MATRIX_SIZE]();
               }
               catch (std::bad_alloc& ba)
               {
                  char szErrorMsg[256];
                  sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrDataNL[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
                  sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
                  sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
                  string strErrorMsg(szErrorMsg);
                  g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                  logerr(szErrorMsg);
                  return false;
               }
               for (y=0; y<SPARSE_MATRIX_SIZE; ++y)
                  pScoring->ppfSparseFastXcorrDataNL[x][y]=0;
            }
            y=i-(x*SPARSE_MATRIX_SIZE);
            pScoring->ppfSparseFastXcorrDataNL[x][y] = pScoring->pfFastXcorrDataNL[i];
         }
      }

      delete[] pScoring->pfFastXcorrDataNL;
      pScoring->pfFastXcorrDataNL = NULL;

   }

   //MH: Fill sparse matrix
   try
   {
      pScoring->ppfSparseFastXcorrData = new float*[pScoring->iFastXcorrDataSize]();
   }
   catch (std::bad_alloc& ba)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrData[%d]). bad_alloc: %s.\n", pScoring->iFastXcorrDataSize, ba.what());
      sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      if (pScoring->pfFastXcorrData[i]>FLOAT_ZERO || pScoring->pfFastXcorrData[i]<-FLOAT_ZERO)
      {
         x=i/SPARSE_MATRIX_SIZE;
         if (pScoring->ppfSparseFastXcorrData[x]==NULL)
         {
            try
            {
               pScoring->ppfSparseFastXcorrData[x] = new float[SPARSE_MATRIX_SIZE]();
            }
            catch (std::bad_alloc& ba)
            {
               char szErrorMsg[256];
               sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrData[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
               sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
               sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }
            for (y=0; y<SPARSE_MATRIX_SIZE; ++y)
               pScoring->ppfSparseFastXcorrData[x][y]=0;
         }
         y=i-(x*SPARSE_MATRIX_SIZE);
         pScoring->ppfSparseFastXcorrData[x][y] = pScoring->pfFastXcorrData[i];
      }
   }

   delete[] pScoring->pfFastXcorrData;
   pScoring->pfFastXcorrData = NULL;

   try
   {
      pScoring->pfSpScoreData = new float[pScoring->_spectrumInfoInternal.iArraySize]();
   }
   catch (std::bad_alloc& ba)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - new(pfSpScoreData[%d]). bad_alloc: %s.\n", pScoring->_spectrumInfoInternal.iArraySize, ba.what());
      sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   // Create data for sp scoring which is just the binned peaks normalized to max inten 100
   for (i = 0; i < pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      pScoring->pfSpScoreData[i] = (float)(100.0 * pdTmpRawData[i] / pPre.dHighestIntensity);
   }

   // MH: Fill sparse matrix for SpScore
   pScoring->iSpScoreData = pScoring->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE + 1;

   try
   {
      pScoring->ppfSparseSpScoreData = new float*[pScoring->iSpScoreData]();
   }
   catch (std::bad_alloc& ba)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseSpScoreData[%d]). bad_alloc: %s.\n", pScoring->iSpScoreData, ba.what());
      sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   for (i=0; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      if (pScoring->pfSpScoreData[i] > FLOAT_ZERO)
      {
         x=i/SPARSE_MATRIX_SIZE;
         if (pScoring->ppfSparseSpScoreData[x]==NULL)
         {
            try
            {
               pScoring->ppfSparseSpScoreData[x] = new float[SPARSE_MATRIX_SIZE]();
            }
            catch (std::bad_alloc& ba)
            {
               char szErrorMsg[256];
               sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseSpScoreData[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
               sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
               sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }
            for (y=0; y<SPARSE_MATRIX_SIZE; ++y)
               pScoring->ppfSparseSpScoreData[x][y]=0;
         }
         y=i-(x*SPARSE_MATRIX_SIZE);
         pScoring->ppfSparseSpScoreData[x][y] = pScoring->pfSpScoreData[i];
      }
   }

   delete[] pScoring->pfSpScoreData;
   pScoring->pfSpScoreData = NULL;

   return true;
}


//-->MH
// Loads spectrum into spectrum object.
void CometPreprocess::PreloadIons(MSReader &mstReader,
                                  Spectrum &spec,
                                  bool bNext,
                                  int scNum)
{
   if (!bNext)
   {
      mstReader.readFile(g_staticParams.inputFile.szFileName, spec, scNum);
   }
   else
   {
      mstReader.readFile(NULL, spec);
   }
}


bool CometPreprocess::CheckActivationMethodFilter(MSActivation act)
{
   bool bSearchSpectrum = true;

   // Check possible activation method filter.
   if (strcmp(g_staticParams.options.szActivationMethod, "ALL")!=0 && (act != mstNA))
   {
      if (!strcmp(g_staticParams.options.szActivationMethod, "CID") && (act != mstCID))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "HCD") && (act != mstHCD))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "ETD+SA") && (act != mstETDSA))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "ETD") && (act != mstETD))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "ECD") && (act != mstECD))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "PQD") && (act != mstPQD))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "IRMPD") && (act != mstIRMPD))
      {
         bSearchSpectrum = 0;
      }
      else if (!strcmp(g_staticParams.options.szActivationMethod, "SID") && (act != mstSID))
      {
         bSearchSpectrum = 0;
      }
   }

   return bSearchSpectrum;
}


bool CometPreprocess::CheckExit(int iAnalysisType,
                                int iScanNum,
                                int iTotalScans,
                                int iLastScan,
                                int iReaderLastScan,
                                int iNumSpectraLoaded)
{
   if (g_cometStatus.IsError())
   {
      return true;
   }

   if (g_cometStatus.IsCancel())
   {
       return true;
   }

   if (iAnalysisType == AnalysisType_SpecificScan)
   {
      _bDoneProcessingAllSpectra = true;
      return true;
   }

   if (iAnalysisType == AnalysisType_SpecificScanRange)
   {
      if (iLastScan > 0)
      {
         if (iScanNum >= iLastScan)
         {
            _bDoneProcessingAllSpectra = true;
            return true;
         }
      }
   }

   if (iAnalysisType == AnalysisType_EntireFile
         && IsValidInputType(g_staticParams.inputFile.iInputType)
         && iScanNum == 0)
   {
      _bDoneProcessingAllSpectra = true;
      return true;
   }

   // Horrible way to exit as this typically requires a quick cycle through
   // while loop but not sure what else to do when getScanNumber() returns 0
   // for non MS/MS scans.
   if (IsValidInputType(g_staticParams.inputFile.iInputType)
         && iTotalScans > iReaderLastScan)
   {
      _bDoneProcessingAllSpectra = true;
      return true;
   }

   if ((g_staticParams.options.iSpectrumBatchSize != 0)
         && (iNumSpectraLoaded >= g_staticParams.options.iSpectrumBatchSize))
   {
      return true;
   }

   return false;
}


bool CometPreprocess::PreprocessSpectrum(Spectrum &spec,
                                         double *pdTmpRawData,
                                         double *pdTmpFastXcorrData,
                                         double *pdTmpCorrelationData)
{
   int z;

   int iScanNumber = spec.getScanNumber();

   int iSpectrumCharge = 0;

   // To run a search, all that's needed is MH+ and Z. So need to generate
   // all combinations of these for each spectrum, whether there's a known
   // Z for each precursor or if Comet has to guess the 1+ or 2+/3+ charges.

   for (int i = 0 ; i < spec.sizeMZ(); ++i)  // walk through all precursor m/z's; usually just one
   {
      double dMZ = 0.0;              // m/z to use for analysis
      vector<int> vChargeStates;

      if (spec.sizeMZ() == spec.sizeZ())
      {
         iSpectrumCharge = spec.atZ(i).z;
      }
      else if (spec.sizeMZ() == 1 && spec.sizeMZ() < spec.sizeZ())
      {
         // example from ms2 file with one precursor and multiple Z lines?
         // will need to include all spectrum charges below
         iSpectrumCharge = spec.atZ(i).z;
      }
      else if (spec.sizeMZ() > spec.sizeZ())
      {
         // need to ignore any spectrum charge as don't know which correspond charge to which precursor
         iSpectrumCharge = 0;
      }
      else
      {
         // don't know what condition/spectrum type leads here
         iSpectrumCharge = 0;
         printf(" Warning, scan %d has %d precursors and %d precursor charges\n", iScanNumber, spec.sizeMZ(), spec.sizeZ());
      }

      // Thermo's monoisotopic m/z determine can fail sometimes. Assume that when
      // the mono m/z value is less than selection window, it is wrong and use the
      // selection m/z as the precursor m/z. This should
      // be invoked when searching Thermo raw files and mzML converted from those.
      // Only applied when single precursor present.
      dMZ = spec.getMonoMZ(i);

      if (g_staticParams.options.bCorrectMass && spec.sizeMZ() == 1)
      {
         double dSelectionLower = spec.getSelWindowLower();
         double dSelectedMZ = spec.getMZ(i);

         if (dMZ > 0.1 && dSelectionLower > 0.1 && dMZ+0.1 < dSelectionLower)
            dMZ = dSelectedMZ;
      }

      if (dMZ == 0)
         dMZ = spec.getMZ(i);

      // 1.  Have spectrum charge from file.  It may be 0.
      // 2.  If the precursor_charge range is set and override_charge is set, then do something look into charge range.
      // 3.  Else just use the spectrum charge (and possibly 1 or 2/3 rule)
      if (g_staticParams.options.iStartCharge > 0 && g_staticParams.options.bOverrideCharge > 0)
      {
         if (g_staticParams.options.bOverrideCharge == 1)
         {
            // ignore spectrum charge and use precursor_charge range
            for (int z = g_staticParams.options.iStartCharge; z <= g_staticParams.options.iEndCharge; ++z)
            {
               vChargeStates.push_back(z);
            }
         }
         else if (g_staticParams.options.bOverrideCharge == 2)
         {
            // only use spectrum charge if it's within the charge range
            for (int z = g_staticParams.options.iStartCharge; z <= g_staticParams.options.iEndCharge; ++z)
            {
               if (z == iSpectrumCharge)
                  vChargeStates.push_back(z);
            }
         }
         else if (g_staticParams.options.bOverrideCharge == 3)
         {
            if (iSpectrumCharge > 0)
            {
               vChargeStates.push_back(iSpectrumCharge);
            }
            else // use 1+ or charge range
            {
               double dSumBelow = 0.0;
               double dSumTotal = 0.0;

               for (int i=0; i<spec.size(); ++i)
               {
                  dSumTotal += spec.at(i).intensity;
                  if (spec.at(i).mz < spec.getMZ())
                     dSumBelow += spec.at(i).intensity;
               }

               if (isEqual(dSumTotal, 0.0) || ((dSumBelow/dSumTotal) > 0.95))
               {
                  vChargeStates.push_back(1);
               }
               else
               {
                  for (z = g_staticParams.options.iStartCharge; z <= g_staticParams.options.iEndCharge; ++z)
                  {
                     vChargeStates.push_back(z);
                  }
               }
            }
         }
      }
      else  // use spectrum charge
      {
         if (iSpectrumCharge > 0) // use charge from file
         {
            vChargeStates.push_back(iSpectrumCharge);

            // add in any other charge states for the single precursor m/z
            if (spec.sizeMZ() == 1 && spec.sizeMZ() < spec.sizeZ())
            {
               for (int ii = 1 ; ii < spec.sizeZ(); ++ii)
               {
                  vChargeStates.push_back(spec.atZ(ii).z);
               }
            }
         }
         else
         {
            double dSumBelow = 0.0;
            double dSumTotal = 0.0;

            for (int i=0; i<spec.size(); ++i)
            {
               dSumTotal += spec.at(i).intensity;

               if (spec.at(i).mz < spec.getMZ())
                  dSumBelow += spec.at(i).intensity;
            }

            if (isEqual(dSumTotal, 0.0) || ((dSumBelow/dSumTotal) > 0.95))
            {
               vChargeStates.push_back(1);
            }
            else
            {
               vChargeStates.push_back(2);
               vChargeStates.push_back(3);
            }
         }
      }

      // now analyze all possible precursor charges for this spectrum
      for (vector<int>::iterator iter = vChargeStates.begin(); iter != vChargeStates.end(); ++iter)
      {
         int iPrecursorCharge = *iter;
         double dMass = dMZ * iPrecursorCharge - (iPrecursorCharge - 1.0) * PROTON_MASS;

         if (CheckExistOutFile(iPrecursorCharge, iScanNumber)
               && (isEqual(g_staticParams.options.dPeptideMassLow, 0.0)
                  || ((dMass >= g_staticParams.options.dPeptideMassLow)
                     && (dMass <= g_staticParams.options.dPeptideMassHigh)))
               && iPrecursorCharge <= g_staticParams.options.iMaxPrecursorCharge)
         {
            Query *pScoring = new Query();

            pScoring->dMangoIndex = iScanNumber + 0.001 * iPrecursorCharge;  // for Mango; used to sort by this value to get original file order

            pScoring->_pepMassInfo.dExpPepMass = dMass;
            pScoring->_spectrumInfoInternal.iChargeState = iPrecursorCharge;
            pScoring->_spectrumInfoInternal.dTotalIntensity = 0.0;
            pScoring->_spectrumInfoInternal.dRTime = 60.0*spec.getRTime();
            pScoring->_spectrumInfoInternal.iScanNumber = iScanNumber;

            if (iPrecursorCharge == 1)
               pScoring->_spectrumInfoInternal.iMaxFragCharge = 1;
            else
            {
               pScoring->_spectrumInfoInternal.iMaxFragCharge = iPrecursorCharge - 1;

               if (pScoring->_spectrumInfoInternal.iMaxFragCharge > g_staticParams.options.iMaxFragmentCharge)
                  pScoring->_spectrumInfoInternal.iMaxFragCharge = g_staticParams.options.iMaxFragmentCharge;
            }

            //MH: Find appropriately sized array cushion based on user parameters. Fixes error found by Patrick Pedrioli for
            // very wide mass tolerance searches (i.e. 500 Da).
            double dCushion = GetMassCushion(pScoring->_pepMassInfo.dExpPepMass);
            pScoring->_spectrumInfoInternal.iArraySize = (int)((pScoring->_pepMassInfo.dExpPepMass + dCushion) * g_staticParams.dInverseBinWidth);

            Threading::LockMutex(_maxChargeMutex);
            // g_massRange.iMaxFragmentCharge is global maximum fragment ion charge across all spectra.
            if (pScoring->_spectrumInfoInternal.iMaxFragCharge > g_massRange.iMaxFragmentCharge)
            {
               g_massRange.iMaxFragmentCharge = pScoring->_spectrumInfoInternal.iMaxFragCharge;
            }
            Threading::UnlockMutex(_maxChargeMutex);

            if (!AdjustMassTol(pScoring))
            {
               return false;
            }

            // Populate pdCorrelation data.
            // NOTE: there must be a good way of doing this just once per spectrum instead
            //       of repeating for each charge state.
            if (!Preprocess(pScoring, spec, pdTmpRawData, pdTmpFastXcorrData, pdTmpCorrelationData))
            {
               return false;
            }

            Threading::LockMutex(g_pvQueryMutex);
            g_pvQuery.push_back(pScoring);
            Threading::UnlockMutex(g_pvQueryMutex);
         }
      }
   }

   return true;
}


// Skip repeating a search if output exists only works for .out files
bool CometPreprocess::CheckExistOutFile(int iCharge,
                                        int iScanNum)
{
   bool bSearchSpectrum = 1;

   if (g_staticParams.options.bOutputOutFiles
         && g_staticParams.options.bSkipAlreadyDone
         && !g_staticParams.options.bOutputSqtStream
         && !g_staticParams.options.bOutputSqtFile
         && !g_staticParams.options.bOutputPepXMLFile
         && !g_staticParams.options.bOutputPercolatorFile)
   {
      char szOutputFileName[SIZE_BUF];
      char *pStr;
      FILE *fpcheck;

      if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL
            && (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
      {
         pStr = g_staticParams.inputFile.szBaseName;
      }
      else
         (*pStr)++;

      sprintf(szOutputFileName, "%s/%s.%.5d.%.5d.%d.out",
            g_staticParams.inputFile.szBaseName,
            pStr,
            iScanNum,
            iScanNum,
            iCharge);

      // Check existence of .out file.
      if ((fpcheck = fopen(szOutputFileName, "r")) != NULL)
      {
         bSearchSpectrum = 0;
         fclose(fpcheck);
      }
   }

   return bSearchSpectrum;
}


bool CometPreprocess::AdjustMassTol(struct Query *pScoring)
{
   if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceLow  = g_staticParams.tolerances.dInputToleranceMinus;
      pScoring->_pepMassInfo.dPeptideMassToleranceHigh = g_staticParams.tolerances.dInputTolerancePlus;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceLow  *= pScoring->_spectrumInfoInternal.iChargeState;
         pScoring->_pepMassInfo.dPeptideMassToleranceHigh *= pScoring->_spectrumInfoInternal.iChargeState;
      }
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceLow  = g_staticParams.tolerances.dInputToleranceMinus * 0.001;
      pScoring->_pepMassInfo.dPeptideMassToleranceHigh = g_staticParams.tolerances.dInputTolerancePlus  * 0.001;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceLow  *= pScoring->_spectrumInfoInternal.iChargeState;
         pScoring->_pepMassInfo.dPeptideMassToleranceHigh *= pScoring->_spectrumInfoInternal.iChargeState;
      }
   }
   else // ppm
   {
      int iCharge = pScoring->_spectrumInfoInternal.iChargeState;

      double dMZ = (pScoring->_pepMassInfo.dExpPepMass + (iCharge - 1) * PROTON_MASS) / iCharge;

      // calculate lower/upper ppm bounds in m/z
      double dBoundLower = dMZ + (dMZ * g_staticParams.tolerances.dInputToleranceMinus / 1E6); // dInputToleranceMinus typically has negative sign
      double dBoundUpper = dMZ + (dMZ * g_staticParams.tolerances.dInputTolerancePlus  / 1E6);

      // convert m/z bounds to neutral mass then add a proton as Comet uses protonated ranges
      double dProtonatedLower = (dBoundLower * iCharge) - (iCharge * PROTON_MASS) + PROTON_MASS;
      double dProtonatedUpper = (dBoundUpper * iCharge) - (iCharge * PROTON_MASS) + PROTON_MASS;

      // these are now the mass difference from exp mass (experimental MH+ mass) to lower/upper bounds
      pScoring->_pepMassInfo.dPeptideMassToleranceLow  = dProtonatedLower - pScoring->_pepMassInfo.dExpPepMass; // usually negative
      pScoring->_pepMassInfo.dPeptideMassToleranceHigh = dProtonatedUpper - pScoring->_pepMassInfo.dExpPepMass; // usually positive
   }

   if (g_staticParams.tolerances.iIsotopeError == 0)
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 1) // search 0, +1 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - C13_DIFF * PROTON_MASS;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 2) // search 0, +1, +2 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - 2.0 * C13_DIFF * PROTON_MASS;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 3) // search 0, +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - 3.0 * C13_DIFF * PROTON_MASS;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 4) // search -1, 0, +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - 3.0 * C13_DIFF * PROTON_MASS;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh + 1.0 * C13_DIFF * PROTON_MASS;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 5) // search -1, 0, +1 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - C13_DIFF * PROTON_MASS;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh + C13_DIFF * PROTON_MASS;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 6) // search -3, -2, -1, 0, +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - 3.0 * C13_DIFF * PROTON_MASS;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh + 3.0 * C13_DIFF * PROTON_MASS;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 7) // search -8, -4, 0, 4, 8 windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow - 8.1;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh + 8.1;
   }
   else  // Should not get here.
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - iIsotopeError=%d\n",  g_staticParams.tolerances.iIsotopeError);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   if (g_staticParams.vectorMassOffsets.size() > 0)
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus -= g_staticParams.vectorMassOffsets[g_staticParams.vectorMassOffsets.size()-1];

   if (pScoring->_pepMassInfo.dPeptideMassTolerancePlus > g_staticParams.options.dPeptideMassHigh)
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = g_staticParams.options.dPeptideMassHigh;

   if (pScoring->_pepMassInfo.dPeptideMassToleranceMinus < g_staticParams.options.dPeptideMassLow)
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = g_staticParams.options.dPeptideMassLow;

   if (pScoring->_pepMassInfo.dPeptideMassToleranceMinus < 100.0)   // there should be no reason to have a peptide mass smaller than this
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = 100.0;    // why not a much larger cutoff??

   // now set Low/High to mass range around ExpMass
   pScoring->_pepMassInfo.dPeptideMassToleranceLow = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceLow;
   pScoring->_pepMassInfo.dPeptideMassToleranceHigh = pScoring->_pepMassInfo.dExpPepMass + pScoring->_pepMassInfo.dPeptideMassToleranceHigh;

   return true;
}


double CometPreprocess::GetMassCushion(double dMass)
{
   // MH: Find appropriately sized array cushion based on user parameters.
   // Fixes error found by Patrick Pedrioli for very wide mass tolerance
   //  searches (i.e. 500 Da).

   double dCushion = 0.0;

   if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
   {
      dCushion = g_staticParams.tolerances.dInputTolerancePlus;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
         dCushion *= 8; //MH: hope +8 is large enough charge because g_staticParams.options.iEndCharge can be overridden.
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
   {
      dCushion = g_staticParams.tolerances.dInputTolerancePlus * 0.001;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
         dCushion *= 8; //MH: hope +8 is large enough charge because g_staticParams.options.iEndCharge can be overridden.
   }
   else // ppm
   {
      dCushion = g_staticParams.tolerances.dInputTolerancePlus * dMass / 1E6;
   }

   dCushion += 5.0;

   return dCushion;
}

//  Reads MSMS data file as ASCII mass/intensity pairs.
bool CometPreprocess::LoadIons(struct Query *pScoring,
                               double *pdTmpRawData,
                               Spectrum mstSpectrum,
                               struct PreprocessStruct *pPre)
{
   int  i;
   double dIon,
          dIntensity;

   // set dIntensityCutoff based on either minimum intensity or % of base peak
   double dIntensityCutoff = g_staticParams.options.dMinIntensity;

   if (g_staticParams.options.dMinPercentageIntensity > 0.0 && g_staticParams.options.dMinPercentageIntensity <= 1.0)
   {
      double dBasePeakIntensity = 0.0;

      for (i = 0; i < mstSpectrum.size(); ++i)
      {
         if (mstSpectrum.at(i).intensity > dBasePeakIntensity)
            dBasePeakIntensity = mstSpectrum.at(i).intensity;
      }

      dIntensityCutoff = g_staticParams.options.dMinPercentageIntensity * dBasePeakIntensity;

      if (dIntensityCutoff < g_staticParams.options.dMinIntensity)
         dIntensityCutoff = g_staticParams.options.dMinIntensity;
   }

   int iNumFragmentPeaks = 0;

   if (g_staticParams.iIndexDb && mstSpectrum.size() > FRAGINDEX_MAX_NUMPEAKS)
   {
      // sorts spectrum in ascending order by intensity
      mstSpectrum.sortIntensity();
   }

   // read peaks in reverse order as they're possibly sorted in ascending order by
   // intensity and vdRawFragmentPeakMass needs most intense peaks
   for (i = mstSpectrum.size() - 1; i >= 0; --i)
   {
      dIon = mstSpectrum.at(i).mz;
      dIntensity = mstSpectrum.at(i).intensity;

      pScoring->_spectrumInfoInternal.dTotalIntensity += dIntensity;

      if (dIntensity >= dIntensityCutoff && dIntensity > 0.0)
      {
         if (g_staticParams.iIndexDb && iNumFragmentPeaks < FRAGINDEX_MAX_NUMPEAKS)
         {
            // Store list of fragment masses for fragment index search
            // Intensities don't matter here
            pScoring->vdRawFragmentPeakMass.push_back(dIon);

            iNumFragmentPeaks++;
         }

         if (dIon < (pScoring->_pepMassInfo.dExpPepMass + 50.0))
         {
            int iBinIon = BIN(dIon);

            dIntensity = sqrt(dIntensity);

            if (iBinIon > pPre->iHighestIon)
               pPre->iHighestIon = iBinIon;

            if ((iBinIon < pScoring->_spectrumInfoInternal.iArraySize)
                  && (dIntensity > pdTmpRawData[iBinIon]))
            {
               if (g_staticParams.options.iRemovePrecursor == 1)
               {
                  double dMZ = (pScoring->_pepMassInfo.dExpPepMass
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1.0) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > pdTmpRawData[iBinIon])
                        pdTmpRawData[iBinIon] = dIntensity;

                     if (pdTmpRawData[iBinIon] > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = pdTmpRawData[iBinIon];
                  }
               }
               else if (g_staticParams.options.iRemovePrecursor == 2)
               {
                  int j;
                  int bNotPrec = 1;

                  for (j=1; j <= pScoring->_spectrumInfoInternal.iChargeState; ++j)
                  {
                     double dMZ;

                     dMZ = (pScoring->_pepMassInfo.dExpPepMass + (j - 1.0) * PROTON_MASS) / (double)(j);
                     if (fabs(dIon - dMZ) < g_staticParams.options.dRemovePrecursorTol)
                     {
                        bNotPrec = 0;
                        break;
                     }
                  }
                  if (bNotPrec)
                  {
                     if (dIntensity > pdTmpRawData[iBinIon])
                        pdTmpRawData[iBinIon] = dIntensity;

                     if (pdTmpRawData[iBinIon] > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = pdTmpRawData[iBinIon];
                  }
               }
               else if (g_staticParams.options.iRemovePrecursor == 3)  //phosphate neutral loss
               {
                  double dMZ1 = (pScoring->_pepMassInfo.dExpPepMass - 79.9799
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1.0) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);
                  double dMZ2 = (pScoring->_pepMassInfo.dExpPepMass - 97.9952
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1.0) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ1) > g_staticParams.options.dRemovePrecursorTol
                        && fabs(dIon - dMZ2) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > pdTmpRawData[iBinIon])
                        pdTmpRawData[iBinIon] = dIntensity;

                     if (pdTmpRawData[iBinIon] > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = pdTmpRawData[iBinIon];
                  }
               }
               else if (g_staticParams.options.iRemovePrecursor == 4)  //undocumented TMT
               {
                  double dMZ1 = (pScoring->_pepMassInfo.dExpPepMass - 18.010565    // water
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1.0) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  double dMZ2 = (pScoring->_pepMassInfo.dExpPepMass - 36.021129    // water x2
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1.0) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  double dMZ3 = (pScoring->_pepMassInfo.dExpPepMass - 63.997737    // methanesulfenic acid 
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1.0) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ1) > g_staticParams.options.dRemovePrecursorTol
                        && fabs(dIon - dMZ2) > g_staticParams.options.dRemovePrecursorTol
                        && fabs(dIon - dMZ3) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > pdTmpRawData[iBinIon])
                        pdTmpRawData[iBinIon] = dIntensity;

                     if (pdTmpRawData[iBinIon] > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = pdTmpRawData[iBinIon];
                  }
               }
               else // iRemovePrecursor==0
               {
                  if (dIntensity > pdTmpRawData[iBinIon])
                     pdTmpRawData[iBinIon] = dIntensity;

                  if (pdTmpRawData[iBinIon] > pPre->dHighestIntensity)
                     pPre->dHighestIntensity = pdTmpRawData[iBinIon];
               }
            }
         }
      }
   }

   return true;
}


// pdTmpRawData now holds raw data, pdTmpCorrelationData is windowed data after this function
void CometPreprocess::MakeCorrData(double *pdTmpRawData,
                                   double *pdTmpCorrelationData,
                                   struct Query *pScoring,
                                   struct PreprocessStruct *pPre)
{
   int  i,
        ii,
        iBin,
        iWindowSize,
        iNumWindows=10;
   double dMaxWindowInten,
          dTmp1,
          dTmp2;

   iWindowSize = (int)((pPre->iHighestIon)/iNumWindows) + 1;

   for (i=0; i<iNumWindows; ++i)
   {
      dMaxWindowInten = 0.0;

      for (ii=0; ii<iWindowSize; ++ii)    // Find max inten. in window.
      {
         iBin = i*iWindowSize+ii;
         if (iBin < pScoring->_spectrumInfoInternal.iArraySize)
         {
            if (pdTmpRawData[iBin] > dMaxWindowInten)
               dMaxWindowInten = pdTmpRawData[iBin];
         }
      }

      if (dMaxWindowInten > 0.0)
      {
         dTmp1 = 50.0 / dMaxWindowInten;
         dTmp2 = 0.05 * pPre->dHighestIntensity;

         for (ii=0; ii<iWindowSize; ++ii)    // Normalize to max inten. in window.
         {
            iBin = i*iWindowSize+ii;
            if (iBin < pScoring->_spectrumInfoInternal.iArraySize)
            {
               if (pdTmpRawData[iBin] > dTmp2)
                  pdTmpCorrelationData[iBin] = pdTmpRawData[iBin]*dTmp1;
            }
         }
      }
   }
}


//MH: This function allocates memory to be shared by threads for spectral processing
bool CometPreprocess::AllocateMemory(int maxNumThreads)
{
   int i;

   //MH: Initally mark all arrays as available (i.e. false=not inuse).
   pbMemoryPool = new bool[maxNumThreads];
   for (i=0; i<maxNumThreads; ++i)
   {
      pbMemoryPool[i] = false;
   }

   //MH: Allocate arrays
   ppdTmpRawDataArr = new double*[maxNumThreads]();
   for (i=0; i<maxNumThreads; ++i)
   {
      try
      {
         ppdTmpRawDataArr[i] = new double[g_staticParams.iArraySizeGlobal]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pdTmpRawData[%d]). bad_alloc: %s.\n", g_staticParams.iArraySizeGlobal, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }
   }

   //MH: Allocate arrays
   ppdTmpFastXcorrDataArr = new double*[maxNumThreads]();
   for (i=0; i<maxNumThreads; ++i)
   {
      try
      {
         ppdTmpFastXcorrDataArr[i] = new double[g_staticParams.iArraySizeGlobal]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pdTmpFastXcorrData[%d]). bad_alloc: %s.\n", g_staticParams.iArraySizeGlobal, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }
   }

   //MH: Allocate arrays
   ppdTmpCorrelationDataArr = new double*[maxNumThreads]();
   for (i=0; i<maxNumThreads; ++i)
   {
      try
      {
         ppdTmpCorrelationDataArr[i] = new double[g_staticParams.iArraySizeGlobal]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pdTmpCorrelationData[%d]). bad_alloc: %s.\n", g_staticParams.iArraySizeGlobal, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }
   }

   return true;
}


//MH: Deallocates memory shared by threads during spectral processing.
bool CometPreprocess::DeallocateMemory(int maxNumThreads)
{
   int i;

   delete [] pbMemoryPool;

   for (i=0; i<maxNumThreads; ++i)
   {
      delete [] ppdTmpRawDataArr[i];
      delete [] ppdTmpFastXcorrDataArr[i];
      delete [] ppdTmpCorrelationDataArr[i];
   }

   delete [] ppdTmpRawDataArr;
   delete [] ppdTmpFastXcorrDataArr;
   delete [] ppdTmpCorrelationDataArr;
   return true;
}

bool CometPreprocess::IsValidInputType(int inputType)
{
   return (inputType == InputType_MZXML || inputType == InputType_RAW);
}


bool CometPreprocess::PreprocessSingleSpectrum(int iPrecursorCharge,
                                               double dMZ,
                                               double *pdMass,
                                               double *pdInten,
                                               int iNumPeaks,
                                               double *pdTmpSpectrum)
{
   Query *pScoring = new Query();

   pScoring->_pepMassInfo.dExpPepMass = (iPrecursorCharge * dMZ) - (iPrecursorCharge - 1.0) * PROTON_MASS;

   if (pScoring->_pepMassInfo.dExpPepMass < g_staticParams.options.dPeptideMassLow
      || pScoring->_pepMassInfo.dExpPepMass > g_staticParams.options.dPeptideMassHigh)
   {
      return false;
   }

   pScoring->_spectrumInfoInternal.iChargeState = iPrecursorCharge;

   if (iPrecursorCharge == 1)
      pScoring->_spectrumInfoInternal.iMaxFragCharge = 1;
   else
   {
      pScoring->_spectrumInfoInternal.iMaxFragCharge = iPrecursorCharge - 1;

      if (pScoring->_spectrumInfoInternal.iMaxFragCharge > g_staticParams.options.iMaxFragmentCharge)
         pScoring->_spectrumInfoInternal.iMaxFragCharge = g_staticParams.options.iMaxFragmentCharge;
   }

   g_massRange.dMinMass = pScoring->_pepMassInfo.dExpPepMass;
   g_massRange.dMaxMass = pScoring->_pepMassInfo.dExpPepMass;
   g_massRange.iMaxFragmentCharge = pScoring->_spectrumInfoInternal.iMaxFragCharge;

   //preprocess here
   int i;
   int x;
   int y;
   struct PreprocessStruct pPre;

   pPre.iHighestIon = 0;
   pPre.dHighestIntensity = 0;

   if (!AdjustMassTol(pScoring))
   {
      return false;
   }

   double dCushion = GetMassCushion(pScoring->_pepMassInfo.dExpPepMass);
   pScoring->_spectrumInfoInternal.iArraySize = (int)((pScoring->_pepMassInfo.dExpPepMass + dCushion) * g_staticParams.dInverseBinWidth);

   // initialize these temporary arrays before re-using
   double *pdTmpRawData = ppdTmpRawDataArr[0];
   double *pdTmpFastXcorrData = ppdTmpFastXcorrDataArr[0];
   double *pdTmpCorrelationData = ppdTmpCorrelationDataArr[0];

   size_t iTmp = (size_t)(pScoring->_spectrumInfoInternal.iArraySize * sizeof(double));
   memset(pdTmpRawData, 0, iTmp);
   memset(pdTmpFastXcorrData, 0, iTmp);
   memset(pdTmpCorrelationData, 0, iTmp);
   memset(pdTmpSpectrum, 0, iTmp);

   // Loop through single spectrum and store in pdTmpRawData array
   double dIon=0,
          dIntensity=0;

   // set dIntensityCutoff based on either minimum intensity or % of base peak
   double dIntensityCutoff = g_staticParams.options.dMinIntensity;

   if (g_staticParams.options.dMinPercentageIntensity > 0.0 && g_staticParams.options.dMinPercentageIntensity <= 1.0)
   {
      double dBasePeakIntensity = 0.0;

      for (i = 0; i < iNumPeaks; ++i)
      {
         if (pdInten[i] > dBasePeakIntensity)
            dBasePeakIntensity = pdInten[i];
      }

      dIntensityCutoff = g_staticParams.options.dMinPercentageIntensity * dBasePeakIntensity;

      if (dIntensityCutoff < g_staticParams.options.dMinIntensity)
         dIntensityCutoff = g_staticParams.options.dMinIntensity;
   }

   for (i=0; i<iNumPeaks; ++i)
   {
      dIon = pdMass[i];
      dIntensity = pdInten[i];

      bool bPass = false;
      if (dIntensity >= dIntensityCutoff && dIntensity > 0.0)
         bPass = true;

      if (bPass)
      {
         if (g_staticParams.iIndexDb)
            pScoring->vdRawFragmentPeakMass.push_back(dIon);

         if (dIon < (pScoring->_pepMassInfo.dExpPepMass + 50.0))
         {
            int iBinIon = BIN(dIon);

            dIntensity = sqrt(dIntensity);

            if (iBinIon < g_staticParams.iArraySizeGlobal && pdTmpSpectrum[iBinIon] < dIntensity)  // used in DoSingleSpectrumSearchMultiResults to return matched ions
                pdTmpSpectrum[iBinIon] = dIntensity;

            if (iBinIon > pPre.iHighestIon)
               pPre.iHighestIon = iBinIon;

            if ((iBinIon < pScoring->_spectrumInfoInternal.iArraySize)
                  && (dIntensity > pdTmpRawData[iBinIon]))
            {
               if (dIntensity > pdTmpRawData[iBinIon])
                  pdTmpRawData[iBinIon] = dIntensity;

               if (pdTmpRawData[iBinIon] > pPre.dHighestIntensity)
                  pPre.dHighestIntensity = pdTmpRawData[iBinIon];
            }
         }
      }
   }

   pScoring->pfFastXcorrData = new float[pScoring->_spectrumInfoInternal.iArraySize]();

   if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {
      try
      {
         pScoring->pfFastXcorrDataNL = new float[pScoring->_spectrumInfoInternal.iArraySize]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pfFastXcorrDataNL[%d]). bad_alloc: %s.\n", pScoring->_spectrumInfoInternal.iArraySize, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr(szErrorMsg);
         return false;
      }
   }

   // Create data for correlation analysis.
   // pdTmpRawData intensities are normalized to 100; pdTmpCorrelationData is windowed
   MakeCorrData(pdTmpRawData, pdTmpCorrelationData, pScoring, &pPre);

   // Make fast xcorr spectrum.
   double dSum = 0.0;
   int iTmpRange = 2 * g_staticParams.iXcorrProcessingOffset + 1;
   double dTmp = 1.0 / (iTmpRange - 1.0);
   double dMinXcorrInten = 0.0;

   dSum = 0.0;
   for (i = 0; i < g_staticParams.iXcorrProcessingOffset; ++i)
      dSum += pdTmpCorrelationData[i];
   for (i = g_staticParams.iXcorrProcessingOffset; i < pScoring->_spectrumInfoInternal.iArraySize + g_staticParams.iXcorrProcessingOffset; ++i)
   {
      if (dMinXcorrInten < pdTmpCorrelationData[i])
         dMinXcorrInten = pdTmpCorrelationData[i];

      if (i < pScoring->_spectrumInfoInternal.iArraySize)
         dSum += pdTmpCorrelationData[i];
      if (i >= iTmpRange)
         dSum -= pdTmpCorrelationData[i - iTmpRange];
      pdTmpFastXcorrData[i - g_staticParams.iXcorrProcessingOffset] = (dSum - pdTmpCorrelationData[i - g_staticParams.iXcorrProcessingOffset]) * dTmp;
   }

   pScoring->iMinXcorrHisto = (int)(dMinXcorrInten * 10.0 * 0.005 + 0.5);

   pScoring->pfFastXcorrData[0] = 0.0;
   for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      double dTmp = pdTmpCorrelationData[i] - pdTmpFastXcorrData[i];

      pScoring->pfFastXcorrData[i] = (float)dTmp;

      // Add flanking peaks if used
      if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0)
      {
         int iTmp;

         iTmp = i-1;
         pScoring->pfFastXcorrData[i] += (float) ((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp])*0.5);

         iTmp = i+1;
         if (iTmp < pScoring->_spectrumInfoInternal.iArraySize)
            pScoring->pfFastXcorrData[i] += (float) ((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp])*0.5);
      }

      // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL
      if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
            && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
      {
         int iTmp;

         pScoring->pfFastXcorrDataNL[i] = pScoring->pfFastXcorrData[i];

         iTmp = i-g_staticParams.precalcMasses.iMinus17;
         if (iTmp>= 0)
         {
            pScoring->pfFastXcorrDataNL[i] += (float)((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp]) * 0.2);
         }

         iTmp = i-g_staticParams.precalcMasses.iMinus18;
         if (iTmp>= 0)
         {
            pScoring->pfFastXcorrDataNL[i] += (float)((pdTmpCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp]) * 0.2);
         }
      }
   }

   pScoring->iFastXcorrDataSize = pScoring->_spectrumInfoInternal.iArraySize/SPARSE_MATRIX_SIZE+1;

   // Using sparse matrix which means we free pScoring->pfFastXcorrData, ->pfFastXcorrDataNL here
   // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL.
   if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {
      try
      {
         pScoring->ppfSparseFastXcorrDataNL = new float*[pScoring->iFastXcorrDataSize]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[256];
         sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrDataNL[%d]). bad_alloc: %s.", pScoring->iFastXcorrDataSize, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr( szErrorMsg);
         return false;
      }

      for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
      {
         if (pScoring->pfFastXcorrDataNL[i]>FLOAT_ZERO || pScoring->pfFastXcorrDataNL[i]<-FLOAT_ZERO)
         {
            x=i/SPARSE_MATRIX_SIZE;
            if (pScoring->ppfSparseFastXcorrDataNL[x]==NULL)
            {
               try
               {
                  pScoring->ppfSparseFastXcorrDataNL[x] = new float[SPARSE_MATRIX_SIZE]();
               }
               catch (std::bad_alloc& ba)
               {
                  char szErrorMsg[256];
                  sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrDataNL[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
                  sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
                  sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
                  string strErrorMsg(szErrorMsg);
                  g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                  logerr(szErrorMsg);
                  return false;
               }
               for (y=0; y<SPARSE_MATRIX_SIZE; ++y)
                  pScoring->ppfSparseFastXcorrDataNL[x][y]=0;
            }
            y=i-(x*SPARSE_MATRIX_SIZE);
            pScoring->ppfSparseFastXcorrDataNL[x][y] = pScoring->pfFastXcorrDataNL[i];
         }
      }

      delete[] pScoring->pfFastXcorrDataNL;
      pScoring->pfFastXcorrDataNL = NULL;
   }

   //MH: Fill sparse matrix
   pScoring->ppfSparseFastXcorrData = new float*[pScoring->iFastXcorrDataSize]();

   for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      if (pScoring->pfFastXcorrData[i]>FLOAT_ZERO || pScoring->pfFastXcorrData[i]<-FLOAT_ZERO)
      {
         x=i/SPARSE_MATRIX_SIZE;
         if (pScoring->ppfSparseFastXcorrData[x]==NULL)
         {
            pScoring->ppfSparseFastXcorrData[x] = new float[SPARSE_MATRIX_SIZE]();

            for (y=0; y<SPARSE_MATRIX_SIZE; ++y)
               pScoring->ppfSparseFastXcorrData[x][y]=0;
         }
         y=i-(x*SPARSE_MATRIX_SIZE);
         pScoring->ppfSparseFastXcorrData[x][y] = pScoring->pfFastXcorrData[i];
      }
   }

   delete[] pScoring->pfFastXcorrData;
   pScoring->pfFastXcorrData = NULL;

   // Create data for sp scoring which is just the binned peaks normalized to max inten 100
   pScoring->pfSpScoreData = new float[pScoring->_spectrumInfoInternal.iArraySize]();
   memset(pScoring->pfSpScoreData, 0, sizeof(float) * pScoring->_spectrumInfoInternal.iArraySize);

   for (i = 0; i < pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      pScoring->pfSpScoreData[i] = (float)(100.0 * pdTmpRawData[i] / pPre.dHighestIntensity);
   }

   // MH: Fill sparse matrix for SpScore
   pScoring->iSpScoreData = pScoring->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE + 1;

   pScoring->ppfSparseSpScoreData = new float*[pScoring->iSpScoreData]();

   for (i=0; i<pScoring->_spectrumInfoInternal.iArraySize; ++i)
   {
      if (pScoring->pfSpScoreData[i] > FLOAT_ZERO)
      {
         x=i/SPARSE_MATRIX_SIZE;
         if (pScoring->ppfSparseSpScoreData[x]==NULL)
         {
            pScoring->ppfSparseSpScoreData[x] = new float[SPARSE_MATRIX_SIZE]();
            memset(pScoring->ppfSparseSpScoreData[x], 0, sizeof(float) * SPARSE_MATRIX_SIZE);
         }
         y=i-(x*SPARSE_MATRIX_SIZE);
         pScoring->ppfSparseSpScoreData[x][y] = pScoring->pfSpScoreData[i];
      }
   }

   delete[] pScoring->pfSpScoreData;
   pScoring->pfSpScoreData = NULL;

   g_pvQuery.push_back(pScoring);

   return true;
}
