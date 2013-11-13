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

// Generate data for both sp scoring (pfSpScoreData) and xcorr analysis (pdCorrelationData).
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
   int iFirstScanInRange = 0;
   int iTmpCount = 0;
   Spectrum mstSpectrum;           // For holding spectrum.
   
   g_massRange.iMaxFragmentCharge = 0;
   g_staticParams.precalcMasses.iMinus17 = BIN(g_staticParams.massUtility.dH2O);
   g_staticParams.precalcMasses.iMinus18 = BIN(g_staticParams.massUtility.dNH3);

   // Create the mutex we will use to protect g_massRange.iMaxFragmentCharge.
   Threading::CreateMutex(&_maxChargeMutex);

   // Get the thread pool of threads that will preprocess the data.
   ThreadPool<PreprocessThreadData *> preprocessThreadPool(PreprocessThreadProc,
         minNumThreads, maxNumThreads);

   // Quick check to make sure first scan isn't greater than last scan in file
/*
   if (g_staticParams.inputFile.iInputType == InputType_MZXML
         && iFirstScan > mstReader.getLastScan() )
   {
      _bDoneProcessingAllSpectra = true;
   }
   else
*/

   // Load all input spectra.
   while(true)
   {
      // Loads in MSMS spectrum data.
      if (_bFirstScan)
      {
         PreloadIons(mstReader, mstSpectrum, false, iFirstScan);
         _bFirstScan = false;

         iFirstScanInRange = mstSpectrum.getScanNumber();
      }
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

      if (iScanNumber != 0)
      {
         int iNumClearedPeaks = 0;

         iTmpCount = iScanNumber;

         if (iFirstScanInRange == 0)
            iFirstScanInRange = iScanNumber;

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

               // Queue at most 1 additional parameter for threads to process.
               preprocessThreadPool.WaitForQueuedParams(1, 1);
              
               //-->MH
               //If there are no Z-lines, filter the spectrum for charge state
               //run filter here.

               PreprocessThreadData *pPreprocessThreadData =
                  new PreprocessThreadData(mstSpectrum, iAnalysisType, iFileLastScan);
               preprocessThreadPool.Launch(pPreprocessThreadData);
            }
         }

         iTotalScans++;
      }
      else if (g_staticParams.inputFile.iInputType != InputType_MZXML)
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
   preprocessThreadPool.WaitForThreads();

   Threading::DestroyMutex(_maxChargeMutex);

   bool bError = false;
   g_cometStatus.GetError(bError);
   bool bSucceeded = !bError;
   
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
   int i;
   int j;
   double *pdTempRawData;
   double *pdTmpFastXcorrData;
   struct msdata pTempSpData[NUM_SP_IONS];
   struct PreprocessStruct pPre;

   pPre.iHighestIon = 0;
   pPre.dHighestIntensity = 0;

   if (!LoadIons(pScoring, mstSpectrum, &pPre))
   {
      return false;
   }

   pdTempRawData = (double *)calloc((size_t)pScoring->_spectrumInfoInternal.iArraySize, (size_t)sizeof(double));
   if (pdTempRawData == NULL)
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - calloc(pdTempRawData[%d]).", pScoring->_spectrumInfoInternal.iArraySize);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));      
      
      logerr("%s\n\n", szErrorMsg);
      
      return false;
   }

   pdTmpFastXcorrData = (double *)calloc((size_t)pScoring->_spectrumInfoInternal.iArraySize, (size_t)sizeof(double));
   if (pdTmpFastXcorrData == NULL)
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - calloc(pdTmpFastXcorrData[%d]).", pScoring->_spectrumInfoInternal.iArraySize);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));      
      
      logerr("%s\n\n", szErrorMsg);
      
      return false;
   }

   pScoring->pfFastXcorrData = (float *)calloc((size_t)pScoring->_spectrumInfoInternal.iArraySize, (size_t)sizeof(float));
   if (pScoring->pfFastXcorrData == NULL)
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - calloc(pfFastXcorrData[%d]).", pScoring->_spectrumInfoInternal.iArraySize);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));      
      
      logerr("%s\n\n", szErrorMsg);
      
      return false;
   }

   if (g_staticParams.ionInformation.bUseNeutralLoss
         && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
            || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
   {
      pScoring->pfFastXcorrDataNL = (float *)calloc((size_t)pScoring->_spectrumInfoInternal.iArraySize, (size_t)sizeof(float));
      if (pScoring->pfFastXcorrDataNL == NULL)
      {
         char szErrorMsg[256];
         szErrorMsg[0] = '\0';
         sprintf(szErrorMsg,  " Error - calloc(pfFastXcorrDataNL[%d]).", pScoring->_spectrumInfoInternal.iArraySize);
                  
         g_cometStatus.SetError(true, string(szErrorMsg));      
      
         logerr("%s\n\n", szErrorMsg);
      
         return false;
      }
   }

   // Create data for correlation analysis.
   MakeCorrData(pdTempRawData, pScoring, &pPre);

   // Make fast xcorr spectrum.
   double dSum=0.0;
   pScoring->iFastXcorrData=1;
   pScoring->iFastXcorrDataNL=1;

   dSum=0.0;
   for (i=0; i<75; i++)
      dSum += pPre.pdCorrelationData[i];
   for (i=75; i < pScoring->_spectrumInfoInternal.iArraySize +75; i++)
   {
      if (i<pScoring->_spectrumInfoInternal.iArraySize)
         dSum += pPre.pdCorrelationData[i];
      if (i>=151)
         dSum -= pPre.pdCorrelationData[i-151];
      pdTmpFastXcorrData[i-75] = (dSum - pPre.pdCorrelationData[i-75])* 0.0066666667;
   }

   pScoring->pfFastXcorrData[0] = 0.0;
   for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; i++)
   {
      double dTmp = pPre.pdCorrelationData[i] - pdTmpFastXcorrData[i];

      pScoring->pfFastXcorrData[i] = (float)dTmp;

      // Add flanking peaks if used
      if (g_staticParams.ionInformation.iTheoreticalFragmentIons == 0)
      {
         int iTmp;

         iTmp = i-1;
         pScoring->pfFastXcorrData[i] += (float) (pPre.pdCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp])*0.5;

         iTmp = i+1;
         if (iTmp < pScoring->_spectrumInfoInternal.iArraySize)
            pScoring->pfFastXcorrData[i] += (float) (pPre.pdCorrelationData[iTmp] - pdTmpFastXcorrData[iTmp])*0.5;
      }

      //MH: Count number of sparse entries needed
      if (g_staticParams.options.bSparseMatrix && i>0 && !isEqual(pScoring->pfFastXcorrData[i], pScoring->pfFastXcorrData[i-1]))
         pScoring->iFastXcorrData++;

      // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL
      if (g_staticParams.ionInformation.bUseNeutralLoss
            && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
      {
         pScoring->pfFastXcorrDataNL[i] = pScoring->pfFastXcorrData[i];

         if (i-g_staticParams.precalcMasses.iMinus17>= 0)
         {
            pScoring->pfFastXcorrDataNL[i] += (float)(pPre.pdCorrelationData[i-g_staticParams.precalcMasses.iMinus17]
                  - pdTmpFastXcorrData[i-g_staticParams.precalcMasses.iMinus17]) * 0.2;
         }

         if (i-g_staticParams.precalcMasses.iMinus18>= 0)
         {
            pScoring->pfFastXcorrDataNL[i] += (float)(pPre.pdCorrelationData[i-g_staticParams.precalcMasses.iMinus18]
                  - pdTmpFastXcorrData[i-g_staticParams.precalcMasses.iMinus18]) * 0.2;
         }

         //MH: Count number of sparse entries needed
         if(g_staticParams.options.bSparseMatrix && i>0 && !isEqual(pScoring->pfFastXcorrDataNL[i], pScoring->pfFastXcorrDataNL[i-1]))
            pScoring->iFastXcorrDataNL++;
      }
   }

   free(pPre.pdCorrelationData);
   free(pdTmpFastXcorrData);

   // Using sparse matrix which means we free pScoring->pfFastXcorrData, ->pfFastXcorrDataNL here
   if (g_staticParams.options.bSparseMatrix)
   {
      //MH: Add one more slot for the last bin
      pScoring->iFastXcorrData++;
      pScoring->iFastXcorrDataNL++;

      //MH: Fill sparse matrix
      pScoring->pSparseFastXcorrData = (SparseMatrix *)calloc((size_t)pScoring->iFastXcorrData, (size_t)sizeof(SparseMatrix));
      if (pScoring->pSparseFastXcorrData == NULL)
      {
         char szErrorMsg[256];
         szErrorMsg[0] = '\0';
         sprintf(szErrorMsg,  " Error - calloc(pScoring->pSparseFastXcorrData[%d]).", pScoring->iFastXcorrData);
                  
         g_cometStatus.SetError(true, string(szErrorMsg));      
      
         logerr("%s\n\n", szErrorMsg);
      
         return false;
      }
      pScoring->pSparseFastXcorrData[0].bin=0;
      pScoring->pSparseFastXcorrData[0].fIntensity=0;
      j=1;
      for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; i++)
      {
         if (!isEqual(pScoring->pfFastXcorrData[i], pScoring->pfFastXcorrData[i-1]))
         {
            pScoring->pSparseFastXcorrData[j].bin = i;
            pScoring->pSparseFastXcorrData[j++].fIntensity = pScoring->pfFastXcorrData[i];
         }
      }

      pScoring->pSparseFastXcorrData[j].bin=i;
      pScoring->pSparseFastXcorrData[j].fIntensity=0;

      free(pScoring->pfFastXcorrData);

      // If A, B or Y ions and their neutral loss selected, roll in -17/-18 contributions to pfFastXcorrDataNL.
      if (g_staticParams.ionInformation.bUseNeutralLoss
            && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
      {
         pScoring->pSparseFastXcorrDataNL = (SparseMatrix *)calloc((size_t)pScoring->iFastXcorrDataNL, (size_t)sizeof(SparseMatrix));
         if (pScoring->pSparseFastXcorrDataNL == NULL)
         {
            char szErrorMsg[256];
            szErrorMsg[0] = '\0';
            sprintf(szErrorMsg,  " Error - calloc(pScoring->pSparseFastXcorrDataNL[%d]).", pScoring->iFastXcorrDataNL);
                  
            g_cometStatus.SetError(true, string(szErrorMsg));      
      
            logerr("%s\n\n", szErrorMsg);
      
            return false;
         }
         pScoring->pSparseFastXcorrDataNL[0].bin=0;
         pScoring->pSparseFastXcorrDataNL[0].fIntensity=0;
         j=1;
         for (i=1; i<pScoring->_spectrumInfoInternal.iArraySize; i++)
         {
            if (!isEqual(pScoring->pfFastXcorrDataNL[i], pScoring->pfFastXcorrDataNL[i-1]))
            {
               pScoring->pSparseFastXcorrDataNL[j].bin = i;
               pScoring->pSparseFastXcorrDataNL[j].fIntensity = pScoring->pfFastXcorrDataNL[i];
               j++;
            }
         }

         pScoring->pSparseFastXcorrDataNL[j].bin=i;
         pScoring->pSparseFastXcorrDataNL[j].fIntensity=0;

         free(pScoring->pfFastXcorrDataNL);
      }
   }

   // Create data for sp scoring.
   // Arbitrary bin size cutoff to do smoothing, peak extraction.
   if (g_staticParams.tolerances.dFragmentBinSize >= 0.10)
   {
      if (!Smooth(pdTempRawData, pScoring->_spectrumInfoInternal.iArraySize))
      {
         return false;
      }

      if (!PeakExtract(pdTempRawData, pScoring->_spectrumInfoInternal.iArraySize))
      {
         return false;
      }
   }

   for (i=0; i<NUM_SP_IONS; i++)
   {
      pTempSpData[i].dIon = 0.0;
      pTempSpData[i].dIntensity = 0.0;
   }

   GetTopIons(pdTempRawData, &(pTempSpData[0]), pScoring->_spectrumInfoInternal.iArraySize);

   qsort(pTempSpData, NUM_SP_IONS, sizeof(struct msdata), QsortByIon);

   // Modify for Sp data.
   StairStep(pTempSpData);

   free(pdTempRawData);

   if (g_staticParams.options.bSparseMatrix)
   {
      // MH: Fill sparse matrix for SpScore - Note that we allocate max number of sp ions,
      // but will use less than this amount.

      pScoring->pSparseSpScoreData = (SparseMatrix *)calloc((size_t)NUM_SP_IONS, (size_t)sizeof(SparseMatrix));
      if (pScoring->pSparseSpScoreData == NULL)
      {
         char szErrorMsg[256];
         szErrorMsg[0] = '\0';
         sprintf(szErrorMsg,  " Error - calloc(pScoring->pSparseSpScoreData[%d]).", pScoring->iSpScoreData);
                  
         g_cometStatus.SetError(true, string(szErrorMsg));      
      
         logerr("%s\n\n", szErrorMsg);
      
         return false;
      }
      pScoring->iSpScoreData=0;
      for (i=0; i<NUM_SP_IONS; i++)
      {
         if((float)pTempSpData[i].dIntensity > FLOAT_ZERO)
         {
            pScoring->pSparseSpScoreData[pScoring->iSpScoreData].bin = (int)(pTempSpData[i].dIon+0.5);
            pScoring->pSparseSpScoreData[pScoring->iSpScoreData].fIntensity = (float) pTempSpData[i].dIntensity;
            pScoring->iSpScoreData++;
         }
      }
   }
   else
   {
      pScoring->pfSpScoreData = (float *)calloc((size_t)pScoring->_spectrumInfoInternal.iArraySize, (size_t)sizeof(float ));
      if (pScoring->pfSpScoreData == NULL)
      {
         char szErrorMsg[256];
         szErrorMsg[0] = '\0';
         sprintf(szErrorMsg,  " Error - calloc(pfSpScoreData[%d]).", pScoring->_spectrumInfoInternal.iArraySize);
                  
         g_cometStatus.SetError(true, string(szErrorMsg));      
      
         logerr("%s\n\n", szErrorMsg);
      
         return false;
      }

      for (i=0; i<NUM_SP_IONS; i++)
         pScoring->pfSpScoreData[(int)(pTempSpData[i].dIon+0.5)] = (float) pTempSpData[i].dIntensity;
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
   bool bError = false;
   g_cometStatus.GetError(bError);
   if (bError)
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
         && g_staticParams.inputFile.iInputType == InputType_MZXML
         && iScanNum == 0)
   {
      _bDoneProcessingAllSpectra = true;
      return true;
   }

   // Horrible way to exit as this typically requires a quick cycle through
   // while loop but not sure what else to do when getScanNumber() returns 0
   // for non MS/MS scans.
   if (g_staticParams.inputFile.iInputType == InputType_MZXML && iTotalScans > iReaderLastScan)
   {
      _bDoneProcessingAllSpectra = true;
      return true;
   }

   if ((g_staticParams.options.iSpectrumBatchSize != 0) &&
       (iNumSpectraLoaded >= g_staticParams.options.iSpectrumBatchSize))
   {
       return true;
   }

   return false;
}


bool CometPreprocess::PreprocessSpectrum(Spectrum &spec)
{
   int z;
   int zStart;
   int zStop;

   int iScanNumber = spec.getScanNumber();

   // Since we have no filter, just add zlines.
   // WARNING: only good up to charge state 3
   if (spec.sizeZ() == 0)
   {
      // Use +1 or +2/+3 rule.
      if (g_staticParams.options.iStartCharge == 0)
      {
         int i=0;
         double dSumBelow = 0.0;
         double dSumTotal = 0.0;

         while(true)
         {
            if(i >= spec.size())
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
            z = 2;
            spec.addZState(z,spec.getMZ()*z-(z-1)*PROTON_MASS);
            z = 3;
            spec.addZState(z,spec.getMZ()*z-(z-1)*PROTON_MASS);
         }
      }
      else
      {
         for (z=g_staticParams.options.iStartCharge; z<=g_staticParams.options.iEndCharge; z++)
         {
            spec.addZState(z,spec.getMZ()*z-(z-1)*PROTON_MASS);
         }
      }
   }

   // Set our boundaries for multiple z lines.
   zStart = 0;
   zStop = spec.sizeZ();

   for (z=zStart; z<zStop; z++)
   {
      int iPrecursorCharge = spec.atZ(z).z;  // I need this before iChargeState gets assigned.
      double dMass = spec.atZ(z).mz;

      if (CheckExistOutFile(iPrecursorCharge, iScanNumber)
            && (isEqual(g_staticParams.options.dLowPeptideMass, 0.0)
               || ((dMass >= g_staticParams.options.dLowPeptideMass)
                  && (dMass <= g_staticParams.options.dHighPeptideMass)))
            && iPrecursorCharge <= g_staticParams.options.iMaxPrecursorCharge)
      {
         Query *pScoring = new Query();

         pScoring->_pepMassInfo.dExpPepMass = dMass;
         pScoring->_spectrumInfoInternal.iArraySize = (int)((dMass + 100.0) * g_staticParams.dInverseBinWidth);
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
         && !g_staticParams.options.bOutputPepXMLFile)
   {
      char szOutputFileName[SIZE_FILE];
      char *pStr;
      FILE *fpcheck;

#ifdef _WIN32
   if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL)
#else
   if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
#endif
      pStr = g_staticParams.inputFile.szBaseName;
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
      pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance;
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 1) // mmu
   {
      pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance * 0.001;
   }
   else // ppm
   {
      pScoring->_pepMassInfo.dPeptideMassTolerance = g_staticParams.tolerances.dInputTolerance
         * pScoring->_pepMassInfo.dExpPepMass / 1000000.0;
   }

   if (g_staticParams.tolerances.iMassToleranceType == 1)  // precursor m/z tolerance
   {
      pScoring->_pepMassInfo.dPeptideMassTolerance *= pScoring->_spectrumInfoInternal.iChargeState;
   }

   if (g_staticParams.tolerances.iIsotopeError == 0)
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 1) // search -1 to +1, +2, +3 isotope windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 3.0*1.00335*PROTON_MASS;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance + 1.0*1.00335*PROTON_MASS;
   }
   else if (g_staticParams.tolerances.iIsotopeError == 2) // search -8, -4, 0, 4, 8 windows
   {
      pScoring->_pepMassInfo.dPeptideMassToleranceMinus = pScoring->_pepMassInfo.dExpPepMass
         - pScoring->_pepMassInfo.dPeptideMassTolerance - 8.1;

      pScoring->_pepMassInfo.dPeptideMassTolerancePlus = pScoring->_pepMassInfo.dExpPepMass
         + pScoring->_pepMassInfo.dPeptideMassTolerance + 8.1;
   }
   else  // Should not get here.
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - iIsotopeError=%d",  g_staticParams.tolerances.iIsotopeError);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));

      logout("%s\n\n", szErrorMsg);
      
      return false;
   }

   return true;
}


//  Reads MSMS data file as ASCII mass/intensity pairs.
bool CometPreprocess::LoadIons(struct Query *pScoring,
                               Spectrum mstSpectrum,
                               struct PreprocessStruct *pPre)
{
   int  i;
   double dIon,
          dIntensity;

   // Just need to pad iArraySize by 75.
   pScoring->_spectrumInfoInternal.iArraySize = (int)((pScoring->_pepMassInfo.dExpPepMass + 100.0)
         / g_staticParams.tolerances.dFragmentBinSize);

   pPre->pdCorrelationData = (double *)calloc((size_t)pScoring->_spectrumInfoInternal.iArraySize, (size_t)sizeof(double));
   if (pPre->pdCorrelationData == NULL)
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - calloc(pdCorrelationData[%d]).", pScoring->_spectrumInfoInternal.iArraySize);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));

      logout("%s\n\n", szErrorMsg);
      
      return false;
   }

   i = 0;
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

            if ((iBinIon < pScoring->_spectrumInfoInternal.iArraySize)
                  && (dIntensity > pPre->pdCorrelationData[iBinIon]))
            {
               if (g_staticParams.options.iRemovePrecursor == 1)
               {
                  double dMZ = (pScoring->_pepMassInfo.dExpPepMass
                        + (pScoring->_spectrumInfoInternal.iChargeState - 1) * PROTON_MASS)
                     / (double)(pScoring->_spectrumInfoInternal.iChargeState);

                  if (fabs(dIon - dMZ) > g_staticParams.options.dRemovePrecursorTol)
                  {
                     if (dIntensity > pPre->pdCorrelationData[iBinIon])
                        pPre->pdCorrelationData[iBinIon] = dIntensity;

                     if (pPre->pdCorrelationData[iBinIon] > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = pPre->pdCorrelationData[iBinIon];
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
                     if (dIntensity > pPre->pdCorrelationData[iBinIon])
                        pPre->pdCorrelationData[iBinIon] = dIntensity;

                     if (pPre->pdCorrelationData[iBinIon] > pPre->dHighestIntensity)
                        pPre->dHighestIntensity = pPre->pdCorrelationData[iBinIon];
                  }
               }
               else // iRemovePrecursor==0
               {
                  if (dIntensity > pPre->pdCorrelationData[iBinIon])
                     pPre->pdCorrelationData[iBinIon] = dIntensity;

                  if (pPre->pdCorrelationData[iBinIon] > pPre->dHighestIntensity)
                     pPre->dHighestIntensity = pPre->pdCorrelationData[iBinIon];
               }
            }
         }
      }
   }

   return true;
}


// pdTempRawData now holds raw data, pdCorrelationData is windowed data.
void CometPreprocess::MakeCorrData(double *pdTempRawData,
                                   struct Query *pScoring,
                                   struct PreprocessStruct *pPre)
{
   int  i,
        ii,
        iBin,
        iWindowSize,
        iNumWindows=10;
   double dMaxWindowInten,
        dMaxOverallInten,
        dTmp1,
        dTmp2;

   dMaxOverallInten = 0.0;

   // Normalize maximum intensity to 100.
   dTmp1 = 1.0;
   if (pPre->dHighestIntensity > FLOAT_ZERO)
      dTmp1 = 100.0 / pPre->dHighestIntensity;

   for (i=0; i < pScoring->_spectrumInfoInternal.iArraySize; i++)
   {
      pdTempRawData[i] = pPre->pdCorrelationData[i]*dTmp1;
      pPre->pdCorrelationData[i]=0.0;

      if (dMaxOverallInten < pdTempRawData[i])
         dMaxOverallInten = pdTempRawData[i];
   }

   iWindowSize = (int) ceil( (double)(pPre->iHighestIon)/iNumWindows);

   for (i=0; i<iNumWindows; i++)
   {
      dMaxWindowInten = 0.0;

      for (ii=0; ii<iWindowSize; ii++)    // Find max inten. in window.
      {
         iBin = i*iWindowSize+ii;
         if (iBin < pScoring->_spectrumInfoInternal.iArraySize)
         {
            if (pdTempRawData[iBin] > dMaxWindowInten)
               dMaxWindowInten = pdTempRawData[iBin];
         }
      }

      if (dMaxWindowInten > 0.0)
      {
         dTmp1 = 50.0 / dMaxWindowInten;
         dTmp2 = 0.05 * dMaxOverallInten;

         for (ii=0; ii<iWindowSize; ii++)    // Normalize to max inten. in window.
         {
            iBin = i*iWindowSize+ii;
            if (iBin < pScoring->_spectrumInfoInternal.iArraySize)
            {
               if (pdTempRawData[iBin] > dTmp2)
                  pPre->pdCorrelationData[iBin] = pdTempRawData[iBin]*dTmp1;
            }
         }
      }
   }
}


// Smooth input data over 5 points.
bool CometPreprocess::Smooth(double *data,
                             int iArraySize)
{
   double *pdSmoothedSpectrum;
   int  i;

   pdSmoothedSpectrum = (double *)calloc((size_t)iArraySize, (size_t)sizeof(double));

   if (pdSmoothedSpectrum == NULL)
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - calloc(pdSmoothedSpectrum[%d]).", iArraySize);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));      
      
      logerr("%s\n\n", szErrorMsg);
      
      return false;
   }

   for (i=2; i<iArraySize-2; i++)
   {
      // *0.0625 is same as divide by 16.
      pdSmoothedSpectrum[i] = (data[i-2]+4.0*data[i-1]+6.0*data[i]+4.0*data[i+1]+data[i+2]) * 0.0625;
   }

   memcpy(data, pdSmoothedSpectrum, iArraySize*sizeof(double));

   free(pdSmoothedSpectrum);

   return true;
}


// Run 2 passes through to pull out peaks.
bool CometPreprocess::PeakExtract(double *data,
                                  int iArraySize)
{
   int  i,
        ii,
        iStartIndex,
        iEndIndex;
   double dStdDev,
        dAvgInten,
        *pdPeakExtracted;

   pdPeakExtracted = (double *)calloc((size_t)iArraySize, (size_t)sizeof(double));
   if (pdPeakExtracted == NULL)
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - calloc(pdPeakExtracted[%d]).", iArraySize);
                  
      g_cometStatus.SetError(true, string(szErrorMsg));      
      
      logerr("%s\n\n", szErrorMsg);
      
      return false;
   }

   // 1st pass, choose only peak greater than avg + dStdDev.
   for (i=0; i<iArraySize; i++)
   {
      pdPeakExtracted[i] = 0.0;
      dAvgInten = 0.0;
      iStartIndex = i-50;

      if (i-50 < 0)
         iStartIndex = 0;

      iEndIndex = i+50;

      if (i+50 > iArraySize-1)
         iEndIndex = iArraySize-1;

      for (ii=iStartIndex; ii<=iEndIndex; ii++)
         dAvgInten += (double)data[ii];
      dAvgInten /= iEndIndex-iStartIndex;

      dStdDev = 0.0;
      for (ii=iStartIndex; ii<=iEndIndex; ii++)
         dStdDev += (data[ii]-dAvgInten)*(data[ii]-dAvgInten);
      dStdDev = sqrt(dStdDev/(iEndIndex-iStartIndex+1));

      if ((i > 0) && (i < iArraySize-1))
      {
         if (data[i] > (dAvgInten+dStdDev))
         {
            pdPeakExtracted[i] = data[i] - dAvgInten + dStdDev;
            data[i] = 0;     // Remove the peak before 2nd pass.
         }
      }
   }

   // 2nd pass, choose only peak greater than avg + 2*dStdDev.
   for (i=0; i<iArraySize; i++)
   {
      dAvgInten = 0.0;
      iStartIndex = i-50;
      if (i-50 < 0)
         iStartIndex = 0;

      iEndIndex = i+50;
      if (i+50 > iArraySize-1)
         iEndIndex = iArraySize-1;

      for (ii=iStartIndex; ii<=iEndIndex; ii++)
         dAvgInten += (double)data[ii];
      dAvgInten /= iEndIndex-iStartIndex;

      dStdDev = 0.0;
      for (ii=iStartIndex; ii<=iEndIndex; ii++)
         dStdDev += (data[ii]-dAvgInten)*(data[ii]-dAvgInten);
      dStdDev = sqrt(dStdDev/(iEndIndex-iStartIndex+1));

      if ((i > 0) && (i < iArraySize-1))
      {
         if (data[i] > (dAvgInten + 2*dStdDev))
            pdPeakExtracted[i] = data[i] - dAvgInten + dStdDev;
      }
   }

   memcpy(data, pdPeakExtracted, iArraySize*sizeof(double));
   free(pdPeakExtracted);

   return true;
}


// Pull out top # ions for intensity matching in search.
void CometPreprocess::GetTopIons(double *pdTempRawData,
                                 struct msdata *pTempSpData,
                                 int iArraySize)
{
   int  i,
        ii,
        iLowestIntenIndex=0;
   double dLowestInten=0.0,
          dMaxInten=0.0;

   for (i=0; i<iArraySize; i++)
   {
      if (pdTempRawData[i] > dLowestInten)
      {
         (pTempSpData+iLowestIntenIndex)->dIntensity = (double)pdTempRawData[i];
         (pTempSpData+iLowestIntenIndex)->dIon = (double)i;

         if ((pTempSpData+iLowestIntenIndex)->dIntensity > dMaxInten)
            dMaxInten = (pTempSpData+iLowestIntenIndex)->dIntensity;

         dLowestInten = (pTempSpData+0)->dIntensity;
         iLowestIntenIndex = 0;

         for (ii=1; ii<NUM_SP_IONS; ii++)
         {
            if ((pTempSpData+ii)->dIntensity < dLowestInten)
            {
               dLowestInten = (pTempSpData+ii)->dIntensity;
               iLowestIntenIndex=ii;
            }
         }
      }
   }

   if (dMaxInten > FLOAT_ZERO)
   {
      for (i=0; i<NUM_SP_IONS; i++)
         (pTempSpData+i)->dIntensity = (((pTempSpData+i)->dIntensity)/dMaxInten)*100.0;
   }
}


int CometPreprocess::QsortByIon(const void *p0, const void *p1)
{
   if ( ((struct msdata *) p1)->dIon < ((struct msdata *) p0)->dIon )
      return (1);
   else if ( ((struct msdata *) p1)->dIon > ((struct msdata *) p0)->dIon )
      return (-1);
   else
      return (0);
}


// Works on Sp data.
void CometPreprocess::StairStep(struct msdata *pTempSpData)
{
   int  i,
        ii,
        iii;
   double dMaxInten,
          dGap;

   i=0;
   while (i < NUM_SP_IONS-1)
   {
      ii = i;
      dMaxInten = (pTempSpData+i)->dIntensity;
      dGap = 0.0;

      while (dGap<=g_staticParams.tolerances.dFragmentBinSize && ii<NUM_SP_IONS-1)
      {
         ii++;
         dGap = (pTempSpData+ii)->dIon - (pTempSpData+ii-1)->dIon;

         // Finds the max intensity for adjacent points.
         if (dGap<=g_staticParams.tolerances.dFragmentBinSize)
         {
            if ((pTempSpData+ii)->dIntensity > dMaxInten)
               dMaxInten = (pTempSpData+ii)->dIntensity;
         }
      }

      // Sets the adjacent points to the dMaxInten.
      for (iii=i; iii<ii; iii++)
         (pTempSpData+iii)->dIntensity = dMaxInten; 

      i = ii;
   }
}
