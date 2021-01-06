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
#include "CometDataInternal.h"
#include "CometPreprocess.h"
#include "CometStatus.h"

Mutex CometPreprocess::_maxChargeMutex;
bool CometPreprocess::_bDoneProcessingAllSpectra;
bool CometPreprocess::_bFirstScan;


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


bool CometPreprocess::LoadAndPreprocessSpectra(MSReader &mstReader,
                                               int iFirstScan,
                                               int iLastScan,
                                               int iAnalysisType,
                                               int minNumThreads,
                                               int maxNumThreads)
{
   int iFileLastScan = -1;         // The actual last scan in the file.
   int iScanNumber = 0;
   int iTotalScans = 0;
   int iNumSpectraLoaded = 0;
   int iTmpCount = 0;
   Spectrum mstSpectrum;           // For holding spectrum.

   g_massRange.iMaxFragmentCharge = 0;
   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dNH3);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dH2O);

   // Create the mutex we will use to protect g_massRange.iMaxFragmentCharge.
   Threading::CreateMutex(&_maxChargeMutex);

   // Get the thread pool of threads that will preprocess the data.
   // NOTE: We are specifying a "maxNumParamsToQueue" to indicate that,
   // at most, we will only read in and queue "maxNumParamsToQueue"
   // additional parameters (1 in this case)
   ThreadPool<PreprocessThreadData *> *pPreprocessThreadPool = new ThreadPool<PreprocessThreadData *>(PreprocessThreadProc,
         minNumThreads, maxNumThreads, 1 /*maxNumParamsToQueue*/);

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

         if (mstSpectrum.size()-iNumClearedPeaks >= g_staticParams.options.iMinPeaks)
         {
            if (iAnalysisType == AnalysisType_SpecificScanRange && iLastScan > 0 && iScanNumber > iLastScan)
            {
               _bDoneProcessingAllSpectra = true;
               break;
            }

            if (CheckActivationMethodFilter(mstSpectrum.getActivationMethod()))
            {
               Threading::LockMutex(g_pvQueryMutex);
               // this needed because processing can add multiple spectra at a time
               iNumSpectraLoaded = (int)g_pvQuery.size();
               iNumSpectraLoaded++;
               Threading::UnlockMutex(g_pvQueryMutex);

               // When we created the thread pool above, we specified the max number of
               // additional params to queue. Here, we must call this method if we want
               // to wait for the queued params to be processed by the threads before we
               // load any more params.
               pPreprocessThreadPool->WaitForQueuedParams();

               //-->MH
               //If there are no Z-lines, filter the spectrum for charge state
               //run filter here.

               PreprocessThreadData *pPreprocessThreadData =
                  new PreprocessThreadData(mstSpectrum, iAnalysisType, iFileLastScan);

               pPreprocessThreadPool->Launch(pPreprocessThreadData);
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

      if (CheckExit(iAnalysisType,
                    iScanNumber,
                    iTotalScans,
                    iLastScan,
                    mstReader.getLastScan(),
                    iNumSpectraLoaded))
      {
         break;
      }
   }

   // Wait for active preprocess threads to complete processing.
   pPreprocessThreadPool->WaitForThreads();

   Threading::DestroyMutex(_maxChargeMutex);

   delete pPreprocessThreadPool;
   pPreprocessThreadPool = NULL;

   bool bSucceeded = !g_cometStatus.IsError() && !g_cometStatus.IsCancel();

   return bSucceeded;
}


void CometPreprocess::PreprocessThreadProc(PreprocessThreadData *pPreprocessThreadData)
{
   // This returns false if it fails, but the errors are already logged
   // so no need to check the return value here.

   PreprocessSpectrum(pPreprocessThreadData->mstSpectrum);

   delete pPreprocessThreadData;
   pPreprocessThreadData = NULL;
}


bool CometPreprocess::DoneProcessingAllSpectra()
{
   return _bDoneProcessingAllSpectra;
}


bool CometPreprocess::Preprocess(struct Query *pScoring,
                                 Spectrum mstSpectrum)
{
   struct PreprocessStruct pPre;

   pPre.iHighestIon = 0;
   pPre.dHighestIntensity = 0;

   // HiXCorr preprocessing
   map<int, double> mapSpectrum;
   if (!LoadIons(pScoring, &mapSpectrum, mstSpectrum, &pPre))
   {
      return false;
   }

   // get mzML nativeID
   char szNativeID[SIZE_NATIVEID];
   if (mstSpectrum.getNativeID(szNativeID, SIZE_NATIVEID))
      strcpy(pScoring->_spectrumInfoInternal.szNativeID, szNativeID);
   else
      pScoring->_spectrumInfoInternal.szNativeID[0]='\0';

   bool bSuccess = FillSparseMatrixMap(pScoring, &mapSpectrum, &pPre);

   return bSuccess;
}


bool CometPreprocess::FillSparseMatrixMap(struct Query *pScoring,
                                          map<int, double> *mapSpectrum,
                                          struct PreprocessStruct *pPre)
{
   // Make fast xcorr spectrum
   vector< pair<int, double> > vBinnedSpectrumXcorr, vBinnedSpectrumSP;
   pScoring->iFastXcorrDataSize = 1;

   double dSum;   // sum of intensities within iXcorrProcessingOffset
   map<int, double>::iterator itStart, itEnd, itCurr;

   // normalize intensities across spectrum and duplicate raw data to vBinnedSpectrumSP
   NormalizeIntensities(mapSpectrum, &vBinnedSpectrumSP, pPre);

   // vBinnedSpectrumXcorr stores the fast xcorr data as a vector of pairs
   // Add in zero points within iXcorrProcessingOffset
   // If flanking peaks are used ... add extra zero point for the flanking peaks
   int iAddFlank = 0;
   if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0)
      iAddFlank = 1;

   vector <int> vAddOffsets;  // this stores the offsets that need to be added to mapSpectrum

   // Also walk through all peaks once, tracking local sum
   dSum = 0.0;
   itStart = mapSpectrum->begin();
   itEnd = mapSpectrum->begin();

   for (itCurr = mapSpectrum->begin(); itCurr != mapSpectrum->end(); ++itCurr)
   {
      // this for loop adds the +- iXcorrProcessingOffset points to each data point
      for (int i = 1; i <= g_staticParams.iXcorrProcessingOffset + iAddFlank; i++)
      {
         int ii = itCurr->first - i;
         if (ii > 0)
            vAddOffsets.push_back(ii);

         ii = itCurr->first + i;
         if (ii < pScoring->_spectrumInfoInternal.iArraySize)
            vAddOffsets.push_back(ii);
      }
   }

   // add offsets to mapSpectrum; will be filled in during xcorr processing
   for (unsigned int ii = 0; ii < vAddOffsets.size(); ii++)
      mapSpectrum->insert(make_pair(vAddOffsets.at(ii), 0.0));

   bool bAllZeroes = true;
   for (itCurr = mapSpectrum->begin(); itCurr != mapSpectrum->end(); ++itCurr)
   {
      if (itEnd != mapSpectrum->end())
      {
         // this sums all intensities up to iXcorrProcessingOffset from current position
         while (itEnd->first - itCurr->first <= g_staticParams.iXcorrProcessingOffset)
         {
            dSum += itEnd->second;
            if (itEnd->second > 0.0 && !bAllZeroes)
               bAllZeroes = false;
            itEnd++;
            if (itEnd == mapSpectrum->end())
               break;
         }
      }
      if (itStart != mapSpectrum->end())
      {
         // this deletes all intensities of offsets less than iXcorrProcessingOffset from current position
         while (itCurr->first - itStart->first > g_staticParams.iXcorrProcessingOffset)
         {
            dSum -= itStart->second;
            itStart++;
            if (itStart == mapSpectrum->end())
               break;
         }
      }

      vBinnedSpectrumXcorr.push_back(make_pair(itCurr->first,
               itCurr->second - (1.0/(2.0*g_staticParams.iXcorrProcessingOffset) * (dSum - itCurr->second))));
   }

   if (bAllZeroes)  // skip scan where all peaks have zero intensity
      return false;

   vector< pair<int, double> > vBinnedSpectrumXcorrNL;
   bool bUseWaterAmmonia = false;

   if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {
      bUseWaterAmmonia = true;
   }

   // Add flanking peaks and water/ammonia loss to vBinnedSpectrumXcorrFlank
   if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0 || bUseWaterAmmonia)
   {
      vector< pair<int, double> > vBinnedSpectrumXcorrFlank;
      vector< pair<int, double> >::iterator it, it1, it17, it18;
      double dNewInten;

      // it17 and it118 point to entries that are -17 and -18 from current iterator
      it17 = vBinnedSpectrumXcorr.begin();
      it18 = vBinnedSpectrumXcorr.begin();

      for (it = vBinnedSpectrumXcorr.begin(); it != vBinnedSpectrumXcorr.end(); ++it)
      {
         dNewInten = it->second;  // get current intensity

         if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0)
         {
            if (it != vBinnedSpectrumXcorr.begin())
            {
               it1 = it - 1;
               if (it1->first == it->first - 1)
                  dNewInten += 0.5 * it1->second;  // add intensity of lower flank
            }

            if (it != vBinnedSpectrumXcorr.end())
            {
               it1 = it + 1;
               if (it1->first == it->first + 1)
                  dNewInten += 0.5 * it1->second;  // add intensity of upper flank
            }

            vBinnedSpectrumXcorrFlank.push_back(make_pair(it->first, dNewInten));
         }

         if (bUseWaterAmmonia)
         {
            while (it17->first < it->first - g_staticParams.precalcMasses.iMinus17)
               it17++;
            while (it18->first < it->first - g_staticParams.precalcMasses.iMinus18)
               it18++;

            if (it->first == it17->first + g_staticParams.precalcMasses.iMinus17)
               dNewInten += 0.2 * it17->second;  // add intensity 0.2 * intensity at -17

            if (it->first == it18->first + g_staticParams.precalcMasses.iMinus18)
               dNewInten += 0.2 * it18->second;  // add intensity 0.2 * intensity at -18

            vBinnedSpectrumXcorrNL.push_back(make_pair(it->first, dNewInten));
         }
      }

      if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0)
      {
         vBinnedSpectrumXcorr.clear();
         vBinnedSpectrumXcorr = vBinnedSpectrumXcorrFlank;
      }
   }

   pScoring->iFastXcorrDataSize =  pScoring->_spectrumInfoInternal.iArraySize/SPARSE_MATRIX_SIZE + 1;

   // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL.
   if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {
      //MH: Fill NL sparse matrix
      try
      {
         pScoring->ppfSparseFastXcorrDataNL = new float*[pScoring->iFastXcorrDataSize]();
      }
      catch (std::bad_alloc& ba)
      {
         char szErrorMsg[SIZE_ERROR];
         sprintf(szErrorMsg, " Error - new(pScoring->ppfSparseFastXcorrDataNL[%d]). bad_alloc: %s.", pScoring->iFastXcorrDataSize, ba.what());
         sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
         sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
         string strErrorMsg(szErrorMsg);
         g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
         logerr( szErrorMsg);
         return false;
      }

      for (size_t iii = 0; iii < vBinnedSpectrumXcorrNL.size(); iii++)
      {
         if (vBinnedSpectrumXcorrNL[iii].second > FLOAT_ZERO || vBinnedSpectrumXcorrNL[iii].second < -FLOAT_ZERO)
         {
            int x = vBinnedSpectrumXcorrNL[iii].first / SPARSE_MATRIX_SIZE;
            int y;

            if (pScoring->ppfSparseFastXcorrDataNL[x]==NULL)
            {
               try
               {
                  pScoring->ppfSparseFastXcorrDataNL[x] = new float[SPARSE_MATRIX_SIZE]();
               }
               catch (std::bad_alloc& ba)
               {
                  char szErrorMsg[SIZE_ERROR];
                  sprintf(szErrorMsg, " Error - new(pScoring->ppfSparseFastXcorrDataNL[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
                  sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
                  sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
                  string strErrorMsg(szErrorMsg);
                  g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
                  logerr(szErrorMsg);
                  return false;
               }
               for (y=0; y<SPARSE_MATRIX_SIZE; y++)
                  pScoring->ppfSparseFastXcorrDataNL[x][y]=0;
            }
            y = vBinnedSpectrumXcorrNL[iii].first - (x * SPARSE_MATRIX_SIZE);
            pScoring->ppfSparseFastXcorrDataNL[x][y] = (float)vBinnedSpectrumXcorrNL[iii].second;
         }
      }
   }

   //MH: Fill sparse matrix
   try
   {
      pScoring->ppfSparseFastXcorrData = new float*[pScoring->iFastXcorrDataSize]();
   }
   catch (std::bad_alloc& ba)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrData[%d]). bad_alloc: %s.\n", pScoring->iFastXcorrDataSize, ba.what());
      sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   for (size_t iii = 0; iii < vBinnedSpectrumXcorr.size(); iii++)
   {
      if (vBinnedSpectrumXcorr[iii].second > FLOAT_ZERO || vBinnedSpectrumXcorr[iii].second < -FLOAT_ZERO)
      {
         int x = vBinnedSpectrumXcorr[iii].first / SPARSE_MATRIX_SIZE;
         int y;

         if (pScoring->ppfSparseFastXcorrData[x]==NULL)
         {
            try
            {
               pScoring->ppfSparseFastXcorrData[x] = new float[SPARSE_MATRIX_SIZE]();
            }
            catch (std::bad_alloc& ba)
            {
               char szErrorMsg[SIZE_ERROR];
               sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrData[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
               sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
               sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }
            for (y=0; y<SPARSE_MATRIX_SIZE; y++)
               pScoring->ppfSparseFastXcorrData[x][y]=0;
         }
         y = vBinnedSpectrumXcorr[iii].first - (x * SPARSE_MATRIX_SIZE);
         pScoring->ppfSparseFastXcorrData[x][y] = (float)vBinnedSpectrumXcorr[iii].second;
      }
   }

   vBinnedSpectrumXcorr.clear();

   // sort map by intensity
   sort(vBinnedSpectrumSP.begin(), vBinnedSpectrumSP.end(), SortVectorByInverseIntensity);

   double dGlobalMax = vBinnedSpectrumSP[0].second;

   size_t iSize = vBinnedSpectrumSP.size();

   // trim to NUM_SP_IONS entries
   if (iSize > NUM_SP_IONS)
   {
      vBinnedSpectrumSP.resize(NUM_SP_IONS);
      iSize = NUM_SP_IONS;
   }

/*
   // now sort by ion
   sort(vBinnedSpectrumSP.begin(), vBinnedSpectrumSP.end(), SortVectorByBin);

   // stairstep
   for (size_t iii = 0; iii < iSize; iii++)
   {
      size_t iCurrent = iii;
      double dLocalMax = vBinnedSpectrumSP[iii].second;

      // increment iCurrent for all adjacent .first entries, get the largest
      // intensity within iii to iCurrent
      while (iCurrent + 1 < iSize
            && vBinnedSpectrumSP[iCurrent + 1].second > 0.0
            && vBinnedSpectrumSP[iCurrent + 1].first -1 == vBinnedSpectrumSP[iCurrent].first)
      {
         iCurrent++;
         if (vBinnedSpectrumSP[iCurrent].second > dLocalMax)
            dLocalMax = vBinnedSpectrumSP[iCurrent].second;
      }

      // assigned local max intensity across all adjacent bins
      for (size_t iv = iii; iv <= iCurrent; iv++)
         vBinnedSpectrumSP[iv].second = dLocalMax;

      if (dLocalMax > dGlobalMax)
         dGlobalMax = dLocalMax;

      iii = iCurrent;
   }
*/

   // normalize max intensity to 100
   for (size_t iii = 0; iii < iSize; iii++)
   {
      vBinnedSpectrumSP[iii].second = (vBinnedSpectrumSP[iii].second * 100.0 / dGlobalMax);
   }

   // MH: Fill sparse matrix for SpScore
   pScoring->iSpScoreData = pScoring->_spectrumInfoInternal.iArraySize / SPARSE_MATRIX_SIZE + 1;

   try
   {
      pScoring->ppfSparseSpScoreData = new float*[pScoring->iSpScoreData]();
   }
   catch (std::bad_alloc& ba)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseSpScoreData[%d]). bad_alloc: %s.\n", pScoring->iSpScoreData, ba.what());
      sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   for (size_t iii = 0; iii < vBinnedSpectrumSP.size(); iii++)
   {
      if (vBinnedSpectrumSP[iii].second > FLOAT_ZERO || vBinnedSpectrumSP[iii].second < -FLOAT_ZERO)
      {
         int x = vBinnedSpectrumSP[iii].first / SPARSE_MATRIX_SIZE;
         int y;

         if (pScoring->ppfSparseSpScoreData[x]==NULL)
         {
            try
            {
               pScoring->ppfSparseSpScoreData[x] = new float[SPARSE_MATRIX_SIZE]();
            }
            catch (std::bad_alloc& ba)
            {
               char szErrorMsg[SIZE_ERROR];
               sprintf(szErrorMsg,  " Error - new(pScoring->ppfSparseFastXcorrData[%d][%d]). bad_alloc: %s.\n", x, SPARSE_MATRIX_SIZE, ba.what());
               sprintf(szErrorMsg+strlen(szErrorMsg), "Comet ran out of memory. Look into \"spectrum_batch_size\"\n");
               sprintf(szErrorMsg+strlen(szErrorMsg), "parameters to address mitigate memory use.\n");
               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               return false;
            }
            for (y=0; y<SPARSE_MATRIX_SIZE; y++)
               pScoring->ppfSparseSpScoreData[x][y]=0;
         }
         y = vBinnedSpectrumSP[iii].first - (x * SPARSE_MATRIX_SIZE);
         pScoring->ppfSparseSpScoreData[x][y] = (float)vBinnedSpectrumSP[iii].second;
      }
   }

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


bool CometPreprocess::PreprocessSpectrum(Spectrum &spec)
{
   int z;
   int zStop;

   int iScanNumber = spec.getScanNumber();

   int iAddCharge = -1;  // specifies how charge states are going to be applied
                         //-1 = should never apply
                         // 0 = use charge in file, else use range
                         // 1 = use precursor_charge range
                         // 2 = search only charge state in file within precursor_charge
                         // 3 = use charge in file else use 1+ or precursor_charge range
   int bFileHasCharge = 0;
   if (spec.sizeZ() > 0)
      bFileHasCharge = 1;

   if (g_staticParams.options.iStartCharge > 0)
   {
      if (!bFileHasCharge)    // override_charge specified, no charge in file
      {
         if (g_staticParams.options.bOverrideCharge == 0
               || g_staticParams.options.bOverrideCharge == 1
               || g_staticParams.options.bOverrideCharge == 2)
         {
            iAddCharge = 2;
         }
         else if (g_staticParams.options.bOverrideCharge == 3)
         {
            iAddCharge = 3;
         }
      }
      else                    // have a charge from file //
      {
         // bOverrideCharge == 0, 2, 3 are not relevant here
         if (g_staticParams.options.bOverrideCharge == 1)
         {
            iAddCharge = 2;
         }
         else
         {
            iAddCharge = 0;   // do nothing
         }
      }
   }
   else  // precursor_charge range not specified
   {
      if (!bFileHasCharge)    // no charge in file
      {
         iAddCharge = 1;
      }
      else
      {
         iAddCharge = 0;      // have a charge from file; nothing to do
      }
   }

   if (iAddCharge == 2)  // use specific charge range
   {
      // if charge is already specified, don't re-add it here when overriding charge range
      int iChargeFromFile = 0;
      if (bFileHasCharge)                  // FIX: no reason that file only has one charge so iChargeFromFile should be an array
         iChargeFromFile = spec.atZ(0).z;  // should read all charge states up to spec.sizeZ();

      for (z=g_staticParams.options.iStartCharge; z<=g_staticParams.options.iEndCharge; z++)
      {
         if (z != iChargeFromFile)
            spec.addZState(z, spec.getMZ() * z - (z-1) * PROTON_MASS);
      }
   }
   else if (iAddCharge == 1 || iAddCharge == 3)  // do 1+ or charge range rule
   {
      int i=0;
      double dSumBelow = 0.0;
      double dSumTotal = 0.0;

      while (true)
      {
         if (i >= spec.size())
            break;

         dSumTotal += spec.at(i).intensity;

         if (spec.at(i).mz < spec.getMZ())
            dSumBelow += spec.at(i).intensity;

         i++;
      }

      if (isEqual(dSumTotal, 0.0) || ((dSumBelow/dSumTotal) > 0.95))
      {
         z = 1;
         spec.addZState(z, spec.getMZ() * z - (z-1) * PROTON_MASS);
      }
      else
      {
         if (iAddCharge == 1)  // 2+ and 3+
         {
            z=2;
            spec.addZState(z, spec.getMZ() * z - (z-1) * PROTON_MASS);
            z=3;
            spec.addZState(z, spec.getMZ() * z - (z-1) * PROTON_MASS);
         }
         else // iAddCharge == 3
         {
            // This option will either use charge from file or
            // charge_range with 1+ rule.  So no redundant addition
            // of charges possible

            for (z=g_staticParams.options.iStartCharge; z<=g_staticParams.options.iEndCharge; z++)
            {
               spec.addZState(z, spec.getMZ() * z - (z-1) * PROTON_MASS);
            }
         }
      }
   }
   else if (iAddCharge == -1)  // should never get here
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg,  " Error - iAddCharge=%d in charge state rule\n", iAddCharge);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }

   // Set our boundaries for multiple z lines.
   zStop = spec.sizeZ();

   double dSelectionLower = 0.0;
   double dSelectedMZ = 0.0;
   double dMonoMZ = 0.0;

   if (g_staticParams.options.bCorrectMass)
   {
      dSelectionLower = spec.getSelWindowLower();
      dSelectedMZ = spec.getMZ();
      dMonoMZ = spec.getMonoMZ();
   }

   for (z=0; z<zStop; z++)
   {
      if (g_staticParams.options.bOverrideCharge == 2 && g_staticParams.options.iStartCharge > 0)
      {
         // ignore spectra that aren't 2+ or 3+.
         if (spec.atZ(z).z < g_staticParams.options.iStartCharge || z > g_staticParams.options.iEndCharge)
         {
            continue;
         }
      }

      int iPrecursorCharge = spec.atZ(z).z;  // I need this before iChargeState gets assigned.
      double dMass = spec.atZ(z).mh;

      // Thermo's monoisotopic m/z determine can fail sometimes. Assume that when
      // the mono m/z value is less than selection window, it is wrong and use the
      // selection m/z as the precursor m/z. This also assumes zStop=1.  This should
      // be invoked when searching Thermo raw files and mzML converted from those.
      if (g_staticParams.options.bCorrectMass && dMonoMZ > 0.1 && dSelectionLower > 0.1 && zStop==1 && dMonoMZ+0.1 < dSelectionLower)
         dMass = dSelectedMZ*iPrecursorCharge - (iPrecursorCharge-1)*PROTON_MASS;

      if (!g_staticParams.options.bOverrideCharge
            || g_staticParams.options.iStartCharge == 0
            || (g_staticParams.options.bOverrideCharge == 3 && bFileHasCharge)
            || (g_staticParams.options.bOverrideCharge
               && (g_staticParams.options.iStartCharge > 0
                  && ((iPrecursorCharge>=g_staticParams.options.iStartCharge
                        && iPrecursorCharge<=g_staticParams.options.iEndCharge )
                     || (iPrecursorCharge == 1 && g_staticParams.options.bOverrideCharge == 3)))))
      {
         if (CheckExistOutFile(iPrecursorCharge, iScanNumber)
               && (isEqual(g_staticParams.options.dPeptideMassLow, 0.0)
                  || ((dMass >= g_staticParams.options.dPeptideMassLow)
                     && (dMass <= g_staticParams.options.dPeptideMassHigh)))
               && iPrecursorCharge <= g_staticParams.options.iMaxPrecursorCharge)
         {
            Query *pScoring = new Query();

            pScoring->dMangoIndex = iScanNumber + 0.001 * z;  // for Mango; used to sort by this value to get original file order

            pScoring->_pepMassInfo.dExpPepMass = dMass;
            pScoring->_spectrumInfoInternal.iChargeState = iPrecursorCharge;
            pScoring->_spectrumInfoInternal.dTotalIntensity = 0.0;
            pScoring->_spectrumInfoInternal.dRTime = 60.0*spec.getRTime();;
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
            double dCushion = 0.0;
            if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
            {
               dCushion = g_staticParams.tolerances.dInputTolerance;

               if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
                  dCushion *= pScoring->_spectrumInfoInternal.iChargeState;
            }
            else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
            {
               dCushion = g_staticParams.tolerances.dInputTolerance * 0.001;

               if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
                  dCushion *= pScoring->_spectrumInfoInternal.iChargeState;
            }
            else // ppm
            {
               dCushion = g_staticParams.tolerances.dInputTolerance * dMass / 1000000.0;
            }
            pScoring->_spectrumInfoInternal.iArraySize = (int)((dMass + dCushion + 2.0) * g_staticParams.dInverseBinWidth);

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
            if (!Preprocess(pScoring, spec))
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
   if (g_pvDIAWindows.size() == 0)
   {
      if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
      {
         pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance;
   
         if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
         {
            pScoring->_pepMassInfo.dPeptideMassTolerance *= pScoring->_spectrumInfoInternal.iChargeState;
         }
      }
      else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
      {
         pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance * 0.001;
   
         if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
         {
            pScoring->_pepMassInfo.dPeptideMassTolerance *= pScoring->_spectrumInfoInternal.iChargeState;
         }
      }
      else // ppm
      {
         pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance
            * pScoring->_pepMassInfo.dExpPepMass / 1000000.0;
      }
   
      if (g_staticParams.tolerances.iIsotopeError == 0)
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance;
      }
      else if (g_staticParams.tolerances.iIsotopeError == 1) // search 0, +1 isotope windows
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance - C13_DIFF * PROTON_MASS;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance;
      }
      else if (g_staticParams.tolerances.iIsotopeError == 2) // search 0, +1, +2 isotope windows
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance - 2.0 * C13_DIFF * PROTON_MASS;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance;
      }
      else if (g_staticParams.tolerances.iIsotopeError == 3) // search 0, +1, +2, +3 isotope windows
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0 * C13_DIFF * PROTON_MASS;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance;
      }
      else if (g_staticParams.tolerances.iIsotopeError == 4) // search -8, -4, 0, 4, 8 windows
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance - 8.1;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance + 8.1;
      }
      else if (g_staticParams.tolerances.iIsotopeError == 5) // search -1, 0, +1, +2, +3 isotope windows
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0 * C13_DIFF * PROTON_MASS;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance + 1.0 * C13_DIFF * PROTON_MASS;
   
      }
      else if (g_staticParams.tolerances.iIsotopeError == 6) // search -3, -2, -1, 0, +1, +2, +3 isotope windows
      {
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
            - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0 * C13_DIFF * PROTON_MASS;
   
         pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
            + pScoring->_pepMassInfo.dPeptideMassTolerance + 3.0 * C13_DIFF * PROTON_MASS;
      }
	  else if (g_staticParams.tolerances.iIsotopeError == 7) // search -1, 0, +1 isotope windows
	  {
		  pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
			  - pScoring->_pepMassInfo.dPeptideMassTolerance - C13_DIFF * PROTON_MASS;

		  pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
			  + pScoring->_pepMassInfo.dPeptideMassTolerance + C13_DIFF * PROTON_MASS;
	  }
      else  // Should not get here.
      {
         char szErrorMsg[SIZE_ERROR];
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
   
      if (pScoring->_pepMassInfo.dPeptideMassToleranceMinus < 100.0)
         pScoring->_pepMassInfo.dPeptideMassToleranceMinus = 100.0;
   }
   else
   {
      // if DIA windows file is specified, this overrides any mass tolerance
      // setting including isotope offsets and mass offsets


      int iCharge;
      double dPrecMZ;
      double dStartWindowMZ;
      double dStartWindowMass;
      double dEndWindowMZ;
      double dEndWindowMass;
     
      iCharge = pScoring->_spectrumInfoInternal.iChargeState;

      dPrecMZ =  (pScoring->_pepMassInfo.dExpPepMass + (iCharge-1)*PROTON_MASS)/iCharge;  //dExpPepMass is MH+

      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = 0.0;
      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = 0.0;

      for (std::vector<double>::iterator it = g_pvDIAWindows.begin(); it != g_pvDIAWindows.end(); ++it)
      {  
         dStartWindowMZ = *it;
         *it++;
         dEndWindowMZ = *it;


         // see if precursor m/z is within a DIA window
         if (dStartWindowMZ <= dPrecMZ && dPrecMZ <= dEndWindowMZ)
         {
            // translate windows to neutral mass space
            dStartWindowMass = (dStartWindowMZ * iCharge) - (iCharge-1)*PROTON_MASS;
            dEndWindowMass = (dEndWindowMZ * iCharge) - (iCharge-1)*PROTON_MASS;

            if (pScoring->_pepMassInfo.dPeptideMassToleranceMinus == 0.0
                  || pScoring->_pepMassInfo.dPeptideMassToleranceMinus > dStartWindowMass)
            {
               pScoring->_pepMassInfo.dPeptideMassToleranceMinus = dStartWindowMass;
            }
            if (pScoring->_pepMassInfo.dPeptideMassTolerancePlus == 0.0
                  || pScoring->_pepMassInfo.dPeptideMassTolerancePlus < dEndWindowMass)
            {
               pScoring->_pepMassInfo.dPeptideMassTolerancePlus = dEndWindowMass;
            }
         }
      }
   }

   return true;
}


//  Reads MSMS data file as ASCII mass/intensity pairs.
bool CometPreprocess::LoadIons(struct Query *pScoring,
                               map<int, double> *mapSpectrum,
                               Spectrum mstSpectrum,
                               struct PreprocessStruct *pPre)
{
   int  i=0;
   double dIon,
          dIntensity;

   while(true)
   {
      if (i >= mstSpectrum.size())
         break;

      dIon = mstSpectrum.at(i).mz;
      dIntensity = mstSpectrum.at(i).intensity;
      i++;

      pScoring->_spectrumInfoInternal.dTotalIntensity += dIntensity;

      if ((dIntensity >= g_staticParams.options.dMinIntensity) && (dIntensity > 0.0))
      {
         if (dIon < (pScoring->_pepMassInfo.dExpPepMass + 50.0))
         {
            int iBinIon = BIN(dIon);
            dIntensity = sqrt(dIntensity);

            if (iBinIon > pPre->iHighestIon)
               pPre->iHighestIon = iBinIon;

            if ((iBinIon < pScoring->_spectrumInfoInternal.iArraySize))
            {
               if (g_staticParams.options.iRemovePrecursor == 0 || g_staticParams.options.iRemovePrecursor > 4)
               {
                  if (dIntensity > (*mapSpectrum)[iBinIon])
                     (*mapSpectrum)[iBinIon] = dIntensity;

                  if (dIntensity > pPre->dHighestIntensity)
                     pPre->dHighestIntensity = dIntensity;
               }
               else if (g_staticParams.options.iRemovePrecursor == 1)
               {
                  double dMZ = (pScoring->_pepMassInfo.dExpPepMass
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > (*mapSpectrum)[iBinIon])
                        (*mapSpectrum)[iBinIon] = dIntensity;

                     if (dIntensity > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = dIntensity;
                  }
               }
               else if (g_staticParams.options.iRemovePrecursor == 2)
               {
                  int j;
                  int bNotPrec = 1;

                  for (j=1; j <= pScoring->_spectrumInfoInternal.iChargeState; j++)
                  {
                     double dMZ;

                     dMZ = (pScoring->_pepMassInfo.dExpPepMass + (j - 1)*PROTON_MASS) / (double)(j);
                     if (fabs(dIon - dMZ) < g_staticParams.options.dRemovePrecursorTol)
                     {
                        bNotPrec = 0;
                        break;
                     }
                  }
                  if (bNotPrec)
                  {
                     if (dIntensity > (*mapSpectrum)[iBinIon])
                        (*mapSpectrum)[iBinIon] = dIntensity;

                     if (dIntensity > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = dIntensity;
                  }
               }
               else if (g_staticParams.options.iRemovePrecursor == 3)  //phosphate neutral loss
               {
                  double dMZ1 = (pScoring->_pepMassInfo.dExpPepMass - 79.9799
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);
                  double dMZ2 = (pScoring->_pepMassInfo.dExpPepMass - 97.9952
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ1) > g_staticParams.options.dRemovePrecursorTol
                        && fabs(dIon - dMZ2) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > (*mapSpectrum)[iBinIon])
                        (*mapSpectrum)[iBinIon] = dIntensity;

                     if (dIntensity > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = dIntensity;
                  }
               }
               else if (g_staticParams.options.iRemovePrecursor == 4)  //undocumented TMT
               {
                  double dMZ1 = (pScoring->_pepMassInfo.dExpPepMass - 18.010565    // water
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  double dMZ2 = (pScoring->_pepMassInfo.dExpPepMass - 36.021129    // water x2
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  double dMZ3 = (pScoring->_pepMassInfo.dExpPepMass - 63.997737    // methanesulfenic acid 
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ1) > g_staticParams.options.dRemovePrecursorTol
                        && fabs(dIon - dMZ2) > g_staticParams.options.dRemovePrecursorTol
                        && fabs(dIon - dMZ3) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > (*mapSpectrum)[iBinIon])
                        (*mapSpectrum)[iBinIon] = dIntensity;

                     if (dIntensity > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = dIntensity;
                  }
               }
            }
         }
      }
   }

   return true;
}

#define WINDOWCOUNT 10

// mapSpectrum now holds raw data, pdTmpCorrelationData is windowed data after this function
// Also duplicate raw data to vBinnedSpectrumSP
void CometPreprocess::NormalizeIntensities(map<int, double> *mapSpectrum,
                                           vector<pair<int, double> > *vBinnedSpectrumSP, 
                                           struct PreprocessStruct *pPre)
{
   map<int, double>::iterator it;

   double dWindowWidth = 1.0 + (int)((pPre->iHighestIon) / (double)WINDOWCOUNT);  // # bins per window

   double pdMaxInten[WINDOWCOUNT] = {};  // maximum intensity for each window initialied to 0
   double dIntensityCutoff = 0.05 * pPre->dHighestIntensity;  // must be withn 5% of sqrt(highest intensity)

   // find max intensity in each window
   int iWhichWindow;
   for (it = mapSpectrum->begin(); it != mapSpectrum->end(); ++it)
   {
      iWhichWindow = (int)(it->first / dWindowWidth);
      if (pdMaxInten[iWhichWindow] < it->second)
         pdMaxInten[iWhichWindow] = it->second;
 
      // also duplicate raw data in this loop for SP analysis
      vBinnedSpectrumSP->push_back(make_pair(it->first, it->second));
   }

   // normalize intensities within each window
   for (it = mapSpectrum->begin(); it != mapSpectrum->end(); ++it)
   {
      if (it->second > dIntensityCutoff)
      {
         iWhichWindow = (int)(it->first / dWindowWidth);
         it->second *= 50.0/pdMaxInten[iWhichWindow];
      }
      else
         it->second = 0.0;
   }
}


bool CometPreprocess::SortVectorByInverseIntensity(const pair<int,double> &a,  
                                                   const pair<int,double> &b) 
{
   return (a.second > b.second);
}


bool CometPreprocess::SortVectorByBin(const pair<int,double> &a,  
                                      const pair<int,double> &b) 
{
   return (a.first < b.first);
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
                                               map<int, double> &mapRawSpectrum)
{
   Query *pScoring = new Query();

   pScoring->_pepMassInfo.dExpPepMass = (iPrecursorCharge*dMZ) - (iPrecursorCharge-1)*PROTON_MASS;

   if (pScoring->_pepMassInfo.dExpPepMass < g_staticParams.options.dPeptideMassLow
      || pScoring->_pepMassInfo.dExpPepMass > g_staticParams.options.dPeptideMassHigh)
   {
      return false;
   }

   pScoring->_spectrumInfoInternal.iChargeState = iPrecursorCharge;

   g_massRange.dMinMass = pScoring->_pepMassInfo.dExpPepMass;
   g_massRange.dMaxMass = pScoring->_pepMassInfo.dExpPepMass;

   if (iPrecursorCharge == 1)
      pScoring->_spectrumInfoInternal.iMaxFragCharge = 1;
   else
   {
      pScoring->_spectrumInfoInternal.iMaxFragCharge = iPrecursorCharge - 1;

      if (pScoring->_spectrumInfoInternal.iMaxFragCharge > g_staticParams.options.iMaxFragmentCharge)
         pScoring->_spectrumInfoInternal.iMaxFragCharge = g_staticParams.options.iMaxFragmentCharge;
   }

   //preprocess here

   //MH: Find appropriately sized array cushion based on user parameters. Fixes error found by Patrick Pedrioli for
   // very wide mass tolerance searches (i.e. 500 Da).
   double dCushion = 0.0;
   if (g_staticParams.tolerances.iMassToleranceUnits == 0) // amu
   {
      dCushion = g_staticParams.tolerances.dInputTolerance;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
        dCushion *= 8; //MH: hope +8 is large enough charge because g_staticParams.options.iEndCharge can be overridden.
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
   {
      dCushion = g_staticParams.tolerances.dInputTolerance * 0.001;

      if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
         dCushion *= 8; //MH: hope +8 is large enough charge because g_staticParams.options.iEndCharge can be overridden.
   }
   else // ppm
   {
      dCushion = g_staticParams.tolerances.dInputTolerance * g_staticParams.options.dPeptideMassHigh / 1000000.0;
   }

   if (!AdjustMassTol(pScoring))
   {
      return false;
   }

   pScoring->_spectrumInfoInternal.iArraySize = (int)((pScoring->_pepMassInfo.dExpPepMass + dCushion + 2.0) * g_staticParams.dInverseBinWidth);
   g_massRange.iMaxFragmentCharge = pScoring->_spectrumInfoInternal.iMaxFragCharge;

   struct PreprocessStruct pPre;
   pPre.iHighestIon = 0;
   pPre.dHighestIntensity = 0;

   double dIon;
   double dIntensity;
   map<int, double> mapSpectrum;

   for (int i=0; i<iNumPeaks; i++)
   {
      dIon = pdMass[i];
      dIntensity = pdInten[i];

      if ((dIntensity >= g_staticParams.options.dMinIntensity) && (dIntensity > 0.0))
      {
         if (dIon < (pScoring->_pepMassInfo.dExpPepMass + 50.0))
         {
            int iBinIon = BIN(dIon);

            // mapRawSpectrum simply stores the binned spectrum to return matched ions
            if (mapRawSpectrum[iBinIon] < dIntensity)  // used in DoSingleSpectrumSearch to return matched ions
               mapRawSpectrum[iBinIon] = dIntensity;

            dIntensity = sqrt(dIntensity);

            if (iBinIon > pPre.iHighestIon)
               pPre.iHighestIon = iBinIon;

            if ((iBinIon < pScoring->_spectrumInfoInternal.iArraySize)
                  && (dIntensity > mapSpectrum[iBinIon]))
            {
               if (dIntensity > mapSpectrum[iBinIon])
                  mapSpectrum[iBinIon] = dIntensity;

               if (mapSpectrum[iBinIon] > pPre.dHighestIntensity)
                  pPre.dHighestIntensity = mapSpectrum[iBinIon];
            }
         }
      }
   }

   bool bSuccess = FillSparseMatrixMap(pScoring, &mapSpectrum, &pPre);

   g_pvQuery.push_back(pScoring);
   return bSuccess;
}
