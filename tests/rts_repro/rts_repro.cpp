// Copyright 2012-2026 Jimmy Eng
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

// Phase 2 reproducer for docs/20260714_EvalueJitter.md.
//
// Links directly against libcometsearch and calls the same
// ICometSearchManager API RTS's C# harness calls via CometWrapper.dll
// (InitializeSingleSpectrumSearch() + DoSingleSpectrumSearchMultiResults()),
// against a fixed, pre-extracted set of real spectra (fixture_spectra.txt),
// from N worker threads pulling from a shared work queue -- the same
// concurrency pattern RealtimeSearch/SearchMS1MS2.cs uses (ConcurrentQueue
// of scan numbers, one shared CometSearchManager instance).
//
// No Thermo RawFileReader, no C++/CLI boundary -- buildable and runnable on
// Linux, unlocking sub-second iteration and ThreadSanitizer.
//
// Parameter setup mirrors SearchMS1MS2.cs's ConfigureInputSettings()'s
// "index already exists" branch exactly (see that function for the
// authoritative list) -- RTS never reads comet.params, so this driver
// doesn't either; every param is set here to match what RTS actually sets.

#include "CometInterfaces.h"
#include "CometData.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace CometInterfaces;
using namespace std;

struct FixtureSpectrum
{
   int scanNumber = 0;
   int charge = 0;
   double mz = 0.0;
   vector<double> mass;
   vector<double> inten;
};

static bool LoadFixtures(const string& path, vector<FixtureSpectrum>& out)
{
   ifstream f(path);
   if (!f)
      return false;

   string line;
   while (getline(f, line))
   {
      if (line.rfind("SPECTRUM", 0) != 0)
         continue;

      istringstream iss(line);
      string tag;
      FixtureSpectrum s;
      int numPeaks = 0;
      iss >> tag >> s.scanNumber >> s.charge >> s.mz >> numPeaks;

      if (numPeaks <= 0)
         continue;

      s.mass.resize(numPeaks);
      s.inten.resize(numPeaks);
      for (int i = 0; i < numPeaks; ++i)
      {
         if (!getline(f, line))
            return false;
         istringstream piss(line);
         piss >> s.mass[i] >> s.inten[i];
      }
      out.push_back(std::move(s));
   }
   return true;
}

static void SetIntParam(ICometSearchManager* mgr, const string& name, int val)
{
   mgr->SetParam(name, std::to_string(val), val);
}

static void SetDoubleParam(ICometSearchManager* mgr, const string& name, double val)
{
   mgr->SetParam(name, std::to_string(val), val);
}

static void SetStringParam(ICometSearchManager* mgr, const string& name, const string& val)
{
   mgr->SetParam(name, val, val);
}

// Reads MassRange:/LengthRange: from the .idx header, mirroring the
// "if (System.IO.File.Exists(sDB))" branch of ConfigureInputSettings().
static bool SetRangesFromIndexHeader(ICometSearchManager* mgr, const string& szDB)
{
   ifstream idxHeader(szDB);
   if (!idxHeader)
      return false;

   string line;
   bool bFoundMass = false;
   bool bFoundLen = false;

   while (getline(idxHeader, line))
   {
      istringstream iss(line);
      string tag;
      iss >> tag;

      if (tag == "MassRange:")
      {
         double massLow = 0.0, massHigh = 0.0;
         iss >> massLow >> massHigh;
         DoubleRange r(massLow, massHigh);
         mgr->SetParam("digest_mass_range", line, r);
         bFoundMass = true;
      }
      else if (tag == "LengthRange:")
      {
         int lenMin = 0, lenMax = 0;
         iss >> lenMin >> lenMax;
         IntRange r(lenMin, lenMax);
         mgr->SetParam("peptide_length_range", line, r);
         bFoundLen = true;
      }
      else if (tag == "StaticMod:")
      {
         // Both fields this function wants (MassRange:/LengthRange:) are already found
         // by the time StaticMod: appears; no need to keep scanning through the
         // VariableMod:/ProteinModList:/RequireVariableMod: lines that follow it
         // (docs/20260811_restore_idx_header_mods.md).
         break;
      }
   }

   return bFoundMass && bFoundLen;
}

int main(int argc, char** argv)
{
   if (argc < 5)
   {
      fprintf(stderr, "usage: %s <database.idx> <fixture_file> <num_threads> <output_file> [ascorepro:0|1] [index_search_type:0=PI_DB|1=FI_DB]\n"
                      "          [chimeric:0|1|2] [zmin] [zmax] [tolMZ] [maxExpect] [maxPeptides]\n"
                      "          [flankBins] [isotopes] [isotopeError] [globalMinIonsScore] [histogramFill]\n"
                      "  chimeric 0 (default): DoSingleSpectrumSearchMultiResults, one line per SPECTRUM block\n"
                      "  chimeric 1: DoSingleSpectrumSearchChimeric charge-range form -- each SPECTRUM block searched at\n"
                      "              charges zmin..zmax with +/- tolMZ (m/z units; 0 = configured tolerance)\n"
                      "  chimeric 2: DoSingleSpectrumSearchChimeric explicit-list form -- all SPECTRUM blocks that share\n"
                      "              a scan number are the (m/z, charge) candidates of one spectrum (first block's peaks)\n"
                      "  defaults: zmin 2, zmax 4, tolMZ 0, maxExpect 0.1, maxPeptides 5; chimeric modes set\n"
                      "  max_index_runtime = 0 (unlimited) since one call runs many searches\n"
                      "  flankBins/isotopes: the chimeric_remove_flank_bins (-1) and chimeric_remove_isotopes (2) params\n"
                      "  isotopeError: the isotope_error param, every mode (default 0, matching RealtimeSearch)\n"
                      "  globalMinIonsScore: > 0 sets fragindex_min_ions_score itself, every mode (default: not sent, 3)\n"
                      "  histogramFill: >= 0 sets fragindex_histogram_fill, every mode (default: not sent)\n", argv[0]);
      return 1;
   }

   string szDB = argv[1];
   string szFixture = argv[2];
   int numThreads = atoi(argv[3]);
   string szOutput = argv[4];
   bool bEnableAScorePro = (argc > 5) && (atoi(argv[5]) != 0);
   int iIndexSearchType = (argc > 6) ? atoi(argv[6]) : -1;  // -1 = not given: not sent, matching RealtimeSearch

   // Chimeric-search mode (docs/20261105_RTSchimeric.md); 0 keeps this driver's original behaviour
   int iChimeric = (argc > 7) ? atoi(argv[7]) : 0;
   int iZmin = (argc > 8) ? atoi(argv[8]) : 2;
   int iZmax = (argc > 9) ? atoi(argv[9]) : 4;
   double dTolMZ = (argc > 10) ? atof(argv[10]) : 0.0;
   double dMaxExpect = (argc > 11) ? atof(argv[11]) : 0.1;   // 0.1 since fragindex_histogram_fill calibrated the E-values
   int iMaxPeptides = (argc > 12) ? atoi(argv[12]) : 5;
   int iFlankBins = (argc > 13) ? atoi(argv[13]) : -1;
   int iIsotopes = (argc > 14) ? atoi(argv[14]) : 2;
   int iIsotopeError = (argc > 15) ? atoi(argv[15]) : 0;
   int iGlobalMinIonsScore = (argc > 16) ? atoi(argv[16]) : 0;   // > 0 sets fragindex_min_ions_score itself (every pass, every mode)
   int iHistogramFill = (argc > 17) ? atoi(argv[17]) : -1;      // >= 0 sets fragindex_histogram_fill (every mode); -1 = not sent

   if (numThreads < 1)
      numThreads = 1;

   vector<FixtureSpectrum> spectra;
   if (!LoadFixtures(szFixture, spectra))
   {
      fprintf(stderr, "Error: failed to load fixtures from %s\n", szFixture.c_str());
      return 1;
   }
   fprintf(stderr, "Loaded %zu fixture spectra\n", spectra.size());

   ICometSearchManager* mgr = GetCometSearchManager();

   // Mirror SearchMS1MS2.cs's ConfigureInputSettings(), "index already exists" path.
   SetStringParam(mgr, "spectral_library_name", szDB);  // unused for MS2-only PI_DB search; kept for parity
   SetStringParam(mgr, "database_name", szDB);

   SetIntParam(mgr, "spectral_library_ms_level", 1);
   SetIntParam(mgr, "retentiontime_tol", 300);
   SetDoubleParam(mgr, "ms1_bin_tol", 1.0005);
   SetDoubleParam(mgr, "ms1_bin_offset", 0.4);
   {
      DoubleRange r(0.0, 2000.0);
      mgr->SetParam("ms1_mass_range", "0 2000", r);
   }
   SetIntParam(mgr, "max_index_runtime", 100);
   SetIntParam(mgr, "num_threads", numThreads);
   SetDoubleParam(mgr, "fragment_bin_tol", 0.02);
   SetDoubleParam(mgr, "fragment_bin_offset", 0.0);
   SetIntParam(mgr, "theoretical_fragment_ions", 0);
   SetDoubleParam(mgr, "peptide_mass_tolerance_upper", 20.0);
   SetDoubleParam(mgr, "peptide_mass_tolerance_lower", -20.0);
   SetIntParam(mgr, "peptide_mass_units", 2);
   SetIntParam(mgr, "precursor_tolerance_type", 1);
   SetIntParam(mgr, "isotope_error", iIsotopeError);   // 0 unless given: RealtimeSearch's own setting
   if (iGlobalMinIonsScore > 0)
      SetIntParam(mgr, "fragindex_min_ions_score", iGlobalMinIonsScore);
   if (iHistogramFill >= 0)
      SetIntParam(mgr, "fragindex_histogram_fill", iHistogramFill);
   SetIntParam(mgr, "max_index_runtime", 200);  // overwrites the 100 above -- matches RTS's actual (duplicate) call
   if (iChimeric != 0)
   {
      SetIntParam(mgr, "max_index_runtime", 0);  // the chimeric call's budget spans every candidate x iteration
      SetIntParam(mgr, "chimeric_remove_flank_bins", iFlankBins);
      SetIntParam(mgr, "chimeric_remove_isotopes", iIsotopes);
   }
   SetIntParam(mgr, "minimum_peaks", 10);
   SetIntParam(mgr, "max_fragment_charge", 3);
   SetIntParam(mgr, "max_precursor_charge", 6);
   SetIntParam(mgr, "equal_I_and_L", 0);
   SetDoubleParam(mgr, "percentage_base_peak", 0.0);
   SetIntParam(mgr, "use_B_ions", 1);
   SetIntParam(mgr, "use_Y_ions", 1);
   SetIntParam(mgr, "print_ascorepro_score", bEnableAScorePro ? -1 : 0);
   if (iIndexSearchType != -1)   // only when given; an existing .idx records its own type
      SetIntParam(mgr, "index_search_type", iIndexSearchType);

   // Variable mods: as of docs/20260811_restore_idx_header_mods.md's v4 format, the .idx
   // header carries mod IDENTITY (chars/mass/neutral-loss, restored from Phase 0.5) *and*
   // the count limits (iMaxNumVarModAAPerMod/max_variable_mods_in_peptide -- new; neither
   // this restored format's own v3 predecessor nor pre-121 ever persisted those, confirmed
   // against v2025.03.0). CometPeptideIndex::ParsePeptideIndexHeader() overwrites
   // g_staticParams.variableModParameters from the file entirely, so no variable_modNN/
   // max_variable_mods_in_peptide SetParam calls are needed here at all -- whichever .idx
   // this process searches (T22's single-mod ground-truth fixture or its two-mod
   // determinism fixture) is self-consistent on its own.

   if (!SetRangesFromIndexHeader(mgr, szDB))
   {
      fprintf(stderr, "Error: could not find MassRange:/LengthRange: in %s header\n", szDB.c_str());
      return 1;
   }

   if (!mgr->InitializeSingleSpectrumSearch())
   {
      string msg;
      mgr->GetStatusMessage(msg);
      fprintf(stderr, "Error: InitializeSingleSpectrumSearch failed: %s\n", msg.c_str());
      return 1;
   }

   // Multi-threaded work-stealing over the fixed spectrum set, mirroring RTS's
   // ConcurrentQueue<int> pattern: N threads racing to pull the next spectrum
   // and call DoSingleSpectrumSearchMultiResults on the one shared manager.
   std::atomic<size_t> nextIndex{0};
   std::mutex outMutex;
   ofstream out(szOutput);

   // Work items: one per SPECTRUM block, except chimeric mode 2 where every block sharing a
   // scan number is one candidate of a single spectrum (the first block's peaks are searched).
   vector<vector<size_t>> jobs;
   if (iChimeric == 2)
   {
      std::map<int, size_t> scanToJob;
      for (size_t i = 0; i < spectra.size(); ++i)
      {
         auto it = scanToJob.find(spectra[i].scanNumber);
         if (it == scanToJob.end())
         {
            scanToJob[spectra[i].scanNumber] = jobs.size();
            jobs.push_back(vector<size_t>{i});
         }
         else
            jobs[it->second].push_back(i);
      }
   }
   else
   {
      for (size_t i = 0; i < spectra.size(); ++i)
         jobs.push_back(vector<size_t>{i});
   }

   auto worker = [&]()
   {
      size_t j;
      while ((j = nextIndex.fetch_add(1)) < jobs.size())
      {
         const FixtureSpectrum& s = spectra[jobs[j][0]];
         vector<string> vPeptide, vProtein;
         vector<vector<Fragment>> vFrag;
         vector<CometScores> vScores;

         if (iChimeric == 0)
         {
            bool ok = mgr->DoSingleSpectrumSearchMultiResults(1, s.charge, s.mz,
               const_cast<double*>(s.mass.data()), const_cast<double*>(s.inten.data()), (int)s.mass.size(),
               vPeptide, vProtein, vFrag, vScores);

            std::lock_guard<std::mutex> lk(outMutex);
            if (ok && !vPeptide.empty() && !vPeptide[0].empty())
            {
               out << "MS2 " << s.scanNumber << "\t" << vPeptide[0] << "\t"
                   << vScores[0].xCorr << "\t" << vScores[0].dExpect << "\tz " << s.charge
                   << "\tprot '" << vProtein[0] << "'\n";
            }
            else
            {
               out << "MS2 " << s.scanNumber << "\tNO_MATCH\n";
            }
            continue;
         }

         bool ok;
         if (iChimeric == 1)
         {
            ok = mgr->DoSingleSpectrumSearchChimeric(1, s.mz, iZmin, iZmax, dTolMZ, dMaxExpect, iMaxPeptides,
               const_cast<double*>(s.mass.data()), const_cast<double*>(s.inten.data()), (int)s.mass.size(),
               vPeptide, vProtein, vFrag, vScores);
         }
         else
         {
            vector<PrecursorCandidate> vCand;
            for (size_t k : jobs[j])
               vCand.emplace_back(spectra[k].mz, spectra[k].charge, 0.0);
            ok = mgr->DoSingleSpectrumSearchChimeric(1, vCand, dTolMZ, dMaxExpect, iMaxPeptides,
               const_cast<double*>(s.mass.data()), const_cast<double*>(s.inten.data()), (int)s.mass.size(),
               vPeptide, vProtein, vFrag, vScores);
         }

         // One line per reported peptide, in iteration order, so a scan-sorted diff keeps them adjacent.
         std::lock_guard<std::mutex> lk(outMutex);
         if (!ok)
         {
            out << "MS2 " << s.scanNumber << "\tERROR\n";
         }
         else if (vPeptide.empty())
         {
            out << "MS2 " << s.scanNumber << "\tNO_MATCH\n";
         }
         else
         {
            for (size_t r = 0; r < vPeptide.size(); ++r)
            {
               out << "MS2 " << s.scanNumber << "\titer " << vScores[r].iIteration << "\tz " << vScores[r].iCharge
                   << "\t" << vPeptide[r] << "\txcorr " << vScores[r].xCorr << "\tdcn " << vScores[r].dCn
                   << "\texpect " << vScores[r].dExpect << "\tmass " << vScores[r].mass
                   << "\tions " << vScores[r].matchedIons << "/" << vScores[r].totalIons
                   << "\tprot '" << vProtein[r] << "'\n";
            }
         }
      }
   };

   vector<std::thread> threads;
   threads.reserve(numThreads);
   for (int t = 0; t < numThreads; ++t)
      threads.emplace_back(worker);
   for (auto& th : threads)
      th.join();

   mgr->FinalizeSingleSpectrumSearch();
   ReleaseCometSearchManager();

   fprintf(stderr, "Done. %zu spectra searched, output written to %s\n", spectra.size(), szOutput.c_str());
   return 0;
}
