// Throwaway Phase 0 spike (RawFileReader migration, docs/20260618_RawFileReaderMigration.md).
// Reads a .raw file through the EXISTING COM-based MSToolkit::RAWReader path (via MSReader)
// and prints the same fields the RawFileReaderSpike prints, for side-by-side diffing.
// Not part of any production build target.

#include "MSReader.h"
#include "Spectrum.h"
#include <cstdio>
#include <vector>

using namespace MSToolkit;

static void PrintScan(MSReader& r, int scanNum)
{
   Spectrum s;
   if (!r.readFile(NULL, s, scanNum))
   {
      printf("scan=%d READ_FAILED\n", scanNum);
      return;
   }

   MSPrecursorInfo p = s.getPrecursor(0);

   printf("scan=%d rt=%.6f centroid=%d tic=%.6g bpi=%.6f bpm=%.6f mslevel=%d positivescan=%d numpeaks=%d\n",
      s.getScanNumber(), s.getRTime(), s.getCentroidStatus(), s.getTIC(), s.getBPI(), s.getBPM(),
      s.getMsLevel(), (int)s.getPositiveScan(), s.size());

   printf("scan=%d precursor: mz=%.6f monoMz=%.6f isoMz=%.6f charge=%d precursorScanNumber=%d\n",
      scanNum, p.mz, p.monoMz, p.isoMz, p.charge, p.precursorScanNumber);

   size_t n = s.size();
   size_t toPrint = n < 5 ? n : 5;
   for (size_t i = 0; i < toPrint; i++)
   {
      printf("scan=%d peak[%zu] mz=%.6f intensity=%.6f\n", scanNum, i, s.at((int)i).mz, s.at((int)i).intensity);
   }
}

int main(int argc, char** argv)
{
   if (argc < 2)
   {
      printf("usage: LegacyRawReaderSpike.exe <path-to-raw> [scanNum ...]\n");
      return 1;
   }

   MSReader r;
   std::vector<MSSpectrumType> filter;
   filter.push_back(MS1);
   filter.push_back(MS2);
   r.setFilter(filter);

   Spectrum first;
   if (!r.readFile(argv[1], first, 0))
   {
      printf("FAILED to open %s\n", argv[1]);
      return 1;
   }
   printf("opened=%s firstScan=%d lastScan=%d\n", argv[1], first.getScanNumber(), r.getLastScan());
   PrintScan(r, first.getScanNumber());

   for (int i = 2; i < argc; i++)
   {
      int scanNum = atoi(argv[i]);
      PrintScan(r, scanNum);
   }

   return 0;
}
