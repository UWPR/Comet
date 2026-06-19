// Throwaway Phase 0 spike (RawFileReader migration, docs/20260618_RawFileReaderMigration.md).
// Validates, in a native .vcxproj for the first time in this codebase:
//   1. Referencing ThermoFisher.CommonCore.RawFileReader/.Data from a /clr project.
//   2. Opening a real .raw file through IRawDataPlus and marshaling a managed double[]
//      (GetCentroidStream) into a native buffer.
//   3. Mixing this /clr-compiled file with the plain-native NativeUtils.cpp in one project.
// Prints the same fields as LegacyRawReaderSpike for side-by-side diffing. Not part of any
// production build target.

#include "NativeUtils.h"
#include <vector>
#include <cstdio>
#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

using namespace System;
using namespace ThermoFisher::CommonCore::Data::Business;
using namespace ThermoFisher::CommonCore::Data::FilterEnums;
using namespace ThermoFisher::CommonCore::Data::Interfaces;
using namespace ThermoFisher::CommonCore::RawFileReader;

static int MsOrderToLevel(MSOrderType order)
{
   switch (order)
   {
   case MSOrderType::Ms:  return 1;
   case MSOrderType::Ms2: return 2;
   case MSOrderType::Ms3: return 3;
   case MSOrderType::Ms4: return 4;
   case MSOrderType::Ms5: return 5;
   default: return 0;
   }
}

static void PrintScan(IRawDataPlus^ rawFile, int scanNum)
{
   ScanStatistics^ stats = rawFile->GetScanStatsForScanNumber(scanNum);
   IScanEvent^ scanEvent = rawFile->GetScanEventForScanNumber(scanNum);
   double rtMin = rawFile->RetentionTimeFromScanNumber(scanNum);
   bool positiveScan = (scanEvent->Polarity != PolarityType::Negative);

   // Marshal the managed double[] peak arrays into native buffers, mirroring the SAFEARRAY
   // copy RAWReader.cpp does today for the COM path (Phase 0 item 2).
   CentroidStream^ centroid = rawFile->GetCentroidStream(scanNum, false);
   int numPeaks = centroid->Length;
   std::vector<double> nativeMasses(numPeaks);
   std::vector<double> nativeIntensities(numPeaks);
   for (int i = 0; i < numPeaks; i++)
   {
      nativeMasses[i] = centroid->Masses[i];
      nativeIntensities[i] = centroid->Intensities[i];
   }
   double intensitySumViaNativeCode = numPeaks > 0 ? NativeSumBuffer(nativeIntensities.data(), nativeIntensities.size()) : 0.0;

   // Precursor info (MS2+ only) -- mirrors RAWReader.cpp's GetPrecursorMassForScanNum path.
   double precursorMz = 0.0, monoMz = 0.0, isoMz = 0.0;
   int charge = 0, precursorScanNumber = 0;
   if (MsOrderToLevel(scanEvent->MSOrder) >= 2)
   {
      IReaction^ reaction = scanEvent->GetReaction(0);
      precursorMz = isoMz = reaction->PrecursorMass;

      LogEntry^ trailer = rawFile->GetTrailerExtraInformation(scanNum);
      for (int i = 0; i < trailer->Length; i++)
      {
         if (trailer->Labels[i] == "Monoisotopic M/Z:")
            monoMz = Double::Parse(trailer->Values[i]);
         else if (trailer->Labels[i] == "Charge State:")
            charge = (int)Double::Parse(trailer->Values[i]);
      }
   }

   printf("scan=%d rt=%.6f centroid=%d tic=%.6g bpi=%.6f bpm=%.6f mslevel=%d positivescan=%d numpeaks=%d\n",
      scanNum, rtMin, stats->IsCentroidScan ? 1 : 0, stats->TIC, stats->BasePeakIntensity, stats->BasePeakMass,
      MsOrderToLevel(scanEvent->MSOrder), positiveScan ? 1 : 0, numPeaks);

   printf("scan=%d precursor: mz=%.6f monoMz=%.6f isoMz=%.6f charge=%d precursorScanNumber=%d\n",
      scanNum, precursorMz, monoMz, isoMz, charge, precursorScanNumber);

   int toPrint = numPeaks < 5 ? numPeaks : 5;
   for (int i = 0; i < toPrint; i++)
      printf("scan=%d peak[%d] mz=%.6f intensity=%.6f\n", scanNum, i, nativeMasses[i], nativeIntensities[i]);

   printf("scan=%d nativeSumIntensity=%.6f (cross-checks managed->native marshal)\n", scanNum, intensitySumViaNativeCode);
}

int main(array<System::String^>^ args)
{
   if (args->Length < 1)
   {
      printf("usage: RawFileReaderSpike.exe <path-to-raw> [scanNum ...]\n");
      return 1;
   }

   IRawDataPlus^ rawFile = RawFileReaderAdapter::FileFactory(args[0]);
   if (!rawFile->IsOpen || rawFile->IsError)
   {
      printf("FAILED to open raw file\n");
      return 1;
   }
   rawFile->SelectInstrument(Device::MS, 1);

   int firstScan = rawFile->RunHeaderEx->FirstSpectrum;
   int lastScan = rawFile->RunHeaderEx->LastSpectrum;
   printf("opened=ok firstScan=%d lastScan=%d\n", firstScan, lastScan);
   PrintScan(rawFile, firstScan);

   for (int i = 1; i < args->Length; i++)
   {
      if (String::Equals(args[i], "--stress"))
      {
         // Phase 0 item 2: repeatedly open scans + marshal the managed double[] peak arrays
         // into native buffers, watching managed heap and process working set for unbounded
         // growth (a crude but workable leak smoke test for a throwaway spike).
         bool sameSan = (args->Length > i + 1) && String::Equals(args[i + 1], "samescan");
         PROCESS_MEMORY_COUNTERS pmc;
         for (int rep = 0; rep < 130000; rep++)
         {
            int scanNum = sameSan ? (firstScan + 1000) : (firstScan + (rep % (lastScan - firstScan)));
            CentroidStream^ centroid = rawFile->GetCentroidStream(scanNum, false);
            int n = centroid->Length;
            std::vector<double> nativeMasses(n), nativeIntensities(n);
            for (int j = 0; j < n; j++)
            {
               nativeMasses[j] = centroid->Masses[j];
               nativeIntensities[j] = centroid->Intensities[j];
            }
            NativeSumBuffer(nativeIntensities.data(), nativeIntensities.size());

            if (rep % 10000 == 0)
            {
               GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
               printf("stress rep=%d managedHeapBytes=%lld workingSetBytes=%zu\n",
                  rep, GC::GetTotalMemory(false), pmc.WorkingSetSize);
            }
         }
         break;
      }
      else
      {
         int scanNum = Int32::Parse(args[i]);
         PrintScan(rawFile, scanNum);
      }
   }

   delete rawFile;
   return 0;
}
