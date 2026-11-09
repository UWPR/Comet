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
   double tolMZ = 0.0;   // optional 6th SPECTRUM field: this candidate's own tolerance (chimeric mode 2)
   vector<double> mass;
   vector<double> inten;
};

// Numbers are read with strtod, not operator>>, so "nan", "inf" and "-inf" load as those
// values: tests feed invalid m/z and peak values through the API on purpose (T73).
static double ParseDouble(const string& tok)
{
   return strtod(tok.c_str(), nullptr);
}

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
      string szMZ;
      FixtureSpectrum s;
      int numPeaks = 0;
      iss >> tag >> s.scanNumber >> s.charge >> szMZ >> numPeaks;
      s.mz = ParseDouble(szMZ);
      string szTol;
      if (iss >> szTol)
         s.tolMZ = ParseDouble(szTol);   // optional: per-candidate tolerance in m/z (0 = the per-call value)

      if (numPeaks <= 0)
         continue;

      s.mass.resize(numPeaks);
      s.inten.resize(numPeaks);
      for (int i = 0; i < numPeaks; ++i)
      {
         if (!getline(f, line))
            return false;
         istringstream piss(line);
         string szMass;
         string szInten;
         piss >> szMass >> szInten;
         s.mass[i] = ParseDouble(szMass);
         s.inten[i] = ParseDouble(szInten);
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

// RTS_REPRO_FRAGMENTS=1: one FRAG line per matched fragment of result row iRow, as the API
// returns them (type = ION_SERIES_* index, mass = the fragment's 1+ mass, z = its charge).
static void WriteFragments(ostream& out, int iScan, size_t iRow, const vector<vector<Fragment>>& vFrag)
{
   if (iRow >= vFrag.size())
      return;
   for (const Fragment& f : vFrag[iRow])
   {
      char szFrag[256];
      snprintf(szFrag, sizeof(szFrag), "FRAG %d\trow %zu\ttype %d\tnum %d\tz %d\tmass %.5f\tnl %d\tinten %.3f\n",
         iScan, iRow, f.type, f.number, f.charge, f.mass, f.neutralLoss ? 1 : 0, f.intensity);
      out << szFrag;
   }
}

// RTS_REPRO_PARAMS: extra parameters for tests and experiments, so a new knob needs no new
// positional argument. Entries are separated by ';', each "<type>:<name>=<value>" with type
// i (int), d (double) or s (string), e.g. "i:use_Z1_ions=1;d:fragment_bin_tol=1.0005".
// The type must be the one Comet reads the parameter as: GetParamValue() casts to the
// requested type, and a mismatch is a null dereference, not an error. Applied after every
// built-in setting (so an entry overrides it) and before InitializeSingleSpectrumSearch().
// Returns false, after printing why, on a malformed entry.
static bool ApplyExtraParams(ICometSearchManager* mgr)
{
   const char* pszEnv = getenv("RTS_REPRO_PARAMS");
   if (pszEnv == nullptr || *pszEnv == '\0')
      return true;

   stringstream ss(pszEnv);
   string entry;
   while (getline(ss, entry, ';'))
   {
      if (entry.empty())
         continue;
      size_t iColon = entry.find(':');
      size_t iEq = entry.find('=');
      if (iColon != 1 || iEq == string::npos || iEq <= iColon + 1)
      {
         fprintf(stderr, "Error: RTS_REPRO_PARAMS entry \"%s\" is not <i|d|s>:<name>=<value>\n", entry.c_str());
         return false;
      }
      char cType = entry[0];
      string name = entry.substr(iColon + 1, iEq - iColon - 1);
      string value = entry.substr(iEq + 1);
      char* pEnd = nullptr;
      if (cType == 'i')
      {
         long lVal = strtol(value.c_str(), &pEnd, 10);
         if (value.empty() || *pEnd != '\0')
         {
            fprintf(stderr, "Error: RTS_REPRO_PARAMS %s: \"%s\" is not an int\n", name.c_str(), value.c_str());
            return false;
         }
         SetIntParam(mgr, name, (int)lVal);
      }
      else if (cType == 'd')
      {
         double dVal = strtod(value.c_str(), &pEnd);
         if (value.empty() || *pEnd != '\0')
         {
            fprintf(stderr, "Error: RTS_REPRO_PARAMS %s: \"%s\" is not a number\n", name.c_str(), value.c_str());
            return false;
         }
         SetDoubleParam(mgr, name, dVal);
      }
      else if (cType == 's')
         SetStringParam(mgr, name, value);
      else
      {
         fprintf(stderr, "Error: RTS_REPRO_PARAMS entry \"%s\": type must be i, d or s\n", entry.c_str());
         return false;
      }
      fprintf(stderr, "RTS_REPRO_PARAMS: %s = %s\n", name.c_str(), value.c_str());
   }
   return true;
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
                      "          [flankBins] [isotopes] [isotopeError] [globalMinIonsScore] [histogramFill] [useNLIons]\n"
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
                      "  histogramFill: >= 0 sets fragindex_histogram_fill, every mode (default: not sent)\n"
                      "  useNLIons: >= 0 sets use_NL_ions, every mode (default: not sent, 0)\n"
                      "  environment: RTS_REPRO_PARAMS=\"<i|d|s>:<name>=<value>;...\" sets any further parameter, applied\n"
                      "              last (e.g. \"i:use_Z1_ions=1\"); the type must be the one Comet reads it as\n"
                      "              RTS_REPRO_FRAGMENTS=1 adds a FRAG line per matched fragment after each result line\n", argv[0]);
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
   int iUseNLIons = (argc > 18) ? atoi(argv[18]) : -1;          // >= 0 sets use_NL_ions (every mode); -1 = not sent

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
   if (iUseNLIons >= 0)
      SetIntParam(mgr, "use_NL_ions", iUseNLIons);
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

   if (!ApplyExtraParams(mgr))
      return 1;

   const char* pszFrag = getenv("RTS_REPRO_FRAGMENTS");
   const bool bPrintFragments = (pszFrag != nullptr && atoi(pszFrag) != 0);

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
               if (bPrintFragments)
                  WriteFragments(out, s.scanNumber, 0, vFrag);
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
               vCand.emplace_back(spectra[k].mz, spectra[k].charge, spectra[k].tolMZ);
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
               if (bPrintFragments)
                  WriteFragments(out, s.scanNumber, r, vFrag);
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
