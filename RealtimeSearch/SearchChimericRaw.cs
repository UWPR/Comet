namespace RealTimeSearch
{
   using System;
   using System.Collections.Concurrent;
   using System.Collections.Generic;
   using System.Diagnostics;
   using System.Globalization;
   using System.IO;
   using System.Linq;
   using System.Threading.Tasks;

   using CometWrapper;
   using ThermoFisher.CommonCore.Data.Business;
   using ThermoFisher.CommonCore.Data.FilterEnums;
   using ThermoFisher.CommonCore.Data.Interfaces;
   using ThermoFisher.CommonCore.RawFileReader;

   /// <summary>
   /// Chimeric-spectrum search over Thermo RAW files: the SearchChimericMS2.cs harness with the
   /// spectra read from RAW files the way SearchMS1MS2.cs reads them (RawFileReader, Windows).
   ///
   /// Every MS/MS scan is passed to CometSearchManagerWrapper.DoSingleSpectrumSearchChimeric()
   /// in its charge-range form with the scan's precursor m/z (the monoisotopic m/z from the scan
   /// trailer when the instrument reported one, as SearchMS1MS2.cs does) and a charge range of
   /// zmin..zmax (default 1..4). The precursor charge the instrument assigned is NOT used for the
   /// search; it is only written to the output for comparison.
   ///
   /// Selected by "chimericraw" as the first argument of RealtimeSearch.exe.
   ///
   /// A single thread reads the RAW file (RawFileReader's file handle is shared) and feeds a
   /// bounded queue; the search threads drain it, so a file is searched while it is still being
   /// read and never held in memory whole.
   /// </summary>
   internal static class SearchChimericRaw
   {
      private const double ProtonMass = 1.00727646688;
      private const int MaxPrecursorCharge = 9;   // native MAX_PRECURSOR_CHARGE: the charge range is clamped to it

      private class Options
      {
         public string Database;
         public List<string> RawFiles = new List<string>();
         public int ZMin = 1;
         public int ZMax = 4;
         public double TolMZ = 0.0;           // 0 = peptide_mass_tolerance as configured (20 ppm)
         public double MaxExpect = 0.1;
         public int MaxPeptides = 5;
         public int TopN = 1;             // rows reported per iteration (the winner's ranks 0..TopN-1)
         public int Threads = Environment.ProcessorCount;
         public bool AScorePro = false;
         public int Isotopes = 2;
         public int MaxRuntimeMs = 0;         // per chimeric call; 0 = unlimited
         public int MaxScans = 0;             // 0 = every MS/MS scan (>0: stop after that many, for quick runs)
         public bool TwoStage = false;        // --twostage: instrument precursor first, then the isolation window on the reduced spectrum
         public double IsoHalfWidth = 0.0;    // --iso-halfwidth: secondary window half-width in m/z; 0 = half the RAW file's isolation width
         public double MaxExpect2 = -1.0;     // --max-expect2: E-value cutoff of the secondary stage; < 0 = --max-expect
         public bool SecondaryAlways = false; // --secondary-always: search the isolation window even when the first stage reported nothing
         public double Ms1Window = 0.0;       // --ms1-window X: write the preceding MS1 scan's peaks within isolation center +/- X m/z to <rawname>.ms1win.txt
         public bool Plain = false;           // --plain: the plain search at the instrument's charge instead of the chimeric loop
         public double FragmentBinTol = -1.0;     // < 0 = the harness default (0.02)
         public double FragmentBinOffset = -1.0;  // < 0 = the harness default (0.0)
         public int TheoreticalFragmentIons = -1; // < 0 = the harness default (0 = flanking peaks)
         public string OutputDir = null;      // default: next to each RAW file
      }

      private class ScanItem
      {
         public int ScanNumber;
         public double RetentionTimeSec;
         public double PrecursorMZ;
         public int InstrumentCharge;         // as reported by the instrument; searched only by --plain and --twostage
         public double IsolationMZ;           // isolation window center
         public double IsolationWidth;        // full isolation width in m/z (0 when the file does not report it)
         public double[] Mass;
         public double[] Intensity;
      }

      private class ScanOutcome
      {
         public ScanItem Scan;
         public bool Ok;
         public bool Incomplete;
         public string Error;
         public double ElapsedMs;
         public List<string> Peptides = new List<string>();
         public List<string> Proteins = new List<string>();
         public List<ScoreWrapper> Scores = new List<ScoreWrapper>();
      }

      private static void Usage()
      {
         Console.WriteLine(" RTS chimeric-spectrum search over RAW files\n");
         Console.WriteLine("    USAGE:  {0} chimericraw <database.idx> <file1.raw> [file2.raw ...] [options]\n",
            System.AppDomain.CurrentDomain.FriendlyName);
         Console.WriteLine("    Every MS/MS scan is searched at its precursor m/z over charges zmin..zmax; the");
         Console.WriteLine("    instrument's own charge assignment is ignored (it is only written to the output).\n");
         Console.WriteLine("    --zrange zmin zmax    charges tried for every scan [1 4]");
         Console.WriteLine("    --tol mz              +/- precursor window in m/z units; 0 = configured 20 ppm [0]");
         Console.WriteLine("    --max-expect E        report a peptide only if its E-value <= E [0.1]");
         Console.WriteLine("    --max-peptides N      at most N peptides per spectrum [5]");
         Console.WriteLine("    --topn N              rows reported per peptide: the winner's top N by xcorr [1]");
         Console.WriteLine("    --threads T           search threads [processor count]");
         Console.WriteLine("    --ascorepro 0|1       AScorePro localization [0]");
         Console.WriteLine("    --isotopes N          chimeric_remove_isotopes, 0-3 [2]");
         Console.WriteLine("    --max-runtime ms      max_index_runtime for one chimeric call; 0 = unlimited [0]");
         Console.WriteLine("    --max-scans N         stop after N MS/MS scans per file; 0 = all [0]");
         Console.WriteLine("    --plain               plain search (DoSingleSpectrumSearchMultiResults) at the instrument's charge;");
         Console.WriteLine("                          a scan without an instrument charge is not searched. Same reader, output and timing.");
         Console.WriteLine("    --twostage            two-stage search: the instrument's precursor m/z and charge first (every charge");
         Console.WriteLine("                          zmin..zmax when the instrument gave none); once that peptide passes --max-expect and");
         Console.WriteLine("                          its fragments are removed, the isolation window is searched at charges zmin..zmax");
         Console.WriteLine("    --iso-halfwidth X     two-stage: secondary window half-width in m/z [half the RAW file's isolation width]");
         Console.WriteLine("    --max-expect2 X       two-stage: E-value cutoff of the secondary stage [--max-expect]");
         Console.WriteLine("    --secondary-always    two-stage: search the isolation window even when the first stage reported nothing");
         Console.WriteLine("    --ms1-window X        also write <rawname>.ms1win.txt: per MS/MS scan, the isolation center and width, the");
         Console.WriteLine("                          preceding MS1 scan and its centroids within center +/- X m/z (for checking results; 0 = off)");
         Console.WriteLine("    --fragment-bin-tol X      fragment_bin_tol [0.02]");
         Console.WriteLine("    --fragment-bin-offset X   fragment_bin_offset [0.0]");
         Console.WriteLine("    --theoretical-fragment-ions 0|1   0 = flanking peaks, 1 = M peak only [0]");
         Console.WriteLine("    --outdir dir          write <rawname>.chimeric.txt here [next to each RAW file]\n");
      }

      private static bool ParseOptions(string[] args, Options o)
      {
         // args[0] == "chimericraw"
         if (args.Length < 3)
            return false;
         var inv = CultureInfo.InvariantCulture;
         try
         {
            o.Database = args[1];
            for (int i = 2; i < args.Length; ++i)
            {
               if (!args[i].StartsWith("--"))
               {
                  o.RawFiles.Add(args[i]);
                  continue;
               }
               switch (args[i])
               {
                  case "--zrange": o.ZMin = int.Parse(args[++i], inv); o.ZMax = int.Parse(args[++i], inv); break;
                  case "--tol": o.TolMZ = double.Parse(args[++i], inv); break;
                  case "--max-expect": o.MaxExpect = double.Parse(args[++i], inv); break;
                  case "--max-peptides": o.MaxPeptides = int.Parse(args[++i], inv); break;
                  case "--topn": o.TopN = int.Parse(args[++i], inv); break;
                  case "--threads": o.Threads = Math.Max(1, int.Parse(args[++i], inv)); break;
                  case "--ascorepro": o.AScorePro = int.Parse(args[++i], inv) != 0; break;
                  case "--isotopes": o.Isotopes = int.Parse(args[++i], inv); break;
                  case "--max-runtime": o.MaxRuntimeMs = int.Parse(args[++i], inv); break;
                  case "--max-scans": o.MaxScans = int.Parse(args[++i], inv); break;
                  case "--plain": o.Plain = true; break;
                  case "--ms1-window": o.Ms1Window = double.Parse(args[++i], inv); break;
                  case "--twostage": o.TwoStage = true; break;
                  case "--iso-halfwidth": o.IsoHalfWidth = double.Parse(args[++i], inv); break;
                  case "--max-expect2": o.MaxExpect2 = double.Parse(args[++i], inv); break;
                  case "--secondary-always": o.SecondaryAlways = true; break;
                  case "--fragment-bin-tol": o.FragmentBinTol = double.Parse(args[++i], inv); break;
                  case "--fragment-bin-offset": o.FragmentBinOffset = double.Parse(args[++i], inv); break;
                  case "--theoretical-fragment-ions": o.TheoreticalFragmentIons = int.Parse(args[++i], inv); break;
                  case "--outdir": o.OutputDir = args[++i]; break;
                  default:
                     Console.WriteLine(" Error: unknown option '{0}'", args[i]);
                     return false;
               }
            }
         }
         catch (Exception ex) when (ex is FormatException || ex is IndexOutOfRangeException || ex is OverflowException
                                    || ex is ArgumentOutOfRangeException)
         {
            Console.WriteLine(" Error: bad option value ({0})", ex.Message);
            return false;
         }
         if (o.RawFiles.Count == 0)
         {
            Console.WriteLine(" Error: no RAW file given");
            return false;
         }
         if (o.ZMin < 1 || o.ZMax < o.ZMin)
         {
            Console.WriteLine(" Error: --zrange needs 1 <= zmin <= zmax (got {0} {1})", o.ZMin, o.ZMax);
            return false;
         }
         if (o.MaxPeptides < 1)
         {
            Console.WriteLine(" Error: --max-peptides must be at least 1 (got {0})", o.MaxPeptides);
            return false;
         }
         if (o.TopN < 1)
         {
            Console.WriteLine(" Error: --topn must be at least 1 (got {0})", o.TopN);
            return false;
         }
         if (double.IsNaN(o.MaxExpect))
         {
            Console.WriteLine(" Error: --max-expect must be a number");
            return false;
         }
         if (double.IsNaN(o.TolMZ) || double.IsInfinity(o.TolMZ) || o.TolMZ < 0.0)
         {
            Console.WriteLine(" Error: --tol must be a finite number >= 0 (got {0})", o.TolMZ);
            return false;
         }
         if (o.MaxScans < 0 || o.MaxRuntimeMs < 0)
         {
            Console.WriteLine(" Error: --max-scans and --max-runtime must be >= 0");
            return false;
         }
         return true;
      }

      /// <summary>The output file of one RAW file: --outdir, or next to the RAW file, as &lt;rawname&gt;.chimeric.txt.</summary>
      private static string OutputPathFor(Options o, string rawPath)
      {
         return Path.Combine(o.OutputDir ?? Path.GetDirectoryName(Path.GetFullPath(rawPath)),
            Path.GetFileNameWithoutExtension(rawPath) + ".chimeric.txt");
      }

      private static string OneField(string s)
      {
         if (string.IsNullOrEmpty(s))
            return "";
         return s.Replace('\t', ' ').Replace('\r', ' ').Replace('\n', ' ');
      }

      /// <summary>The scan's peaks, centroided as in SearchMS1MS2.cs.</summary>
      private static void ReadPeaks(IRawDataPlus raw, int scanNumber, out double[] mass, out double[] inten, out int n)
      {
         var stats = raw.GetScanStatsForScanNumber(scanNumber);
         if (stats.IsCentroidScan && stats.SpectrumPacketType == SpectrumPacketType.FtCentroid)
         {
            var centroids = raw.GetCentroidStream(scanNumber, false);
            n = centroids.Length;
            mass = centroids.Masses;
            inten = centroids.Intensities;
         }
         else
         {
            var seg = raw.GetSegmentedScanFromScanNumber(scanNumber, stats);
            n = seg.Positions.Length;
            mass = seg.Positions;
            inten = seg.Intensities;
         }
      }

      /// <summary>Precursor m/z (monoisotopic from the trailer when plausible, as SearchMS1MS2.cs) and the instrument's charge.</summary>
      private static void ReadPrecursor(IRawDataPlus raw, int scanNumber, out double precursorMZ, out int charge,
         out double isolationMZ, out double isolationWidth)
      {
         var reaction = raw.GetScanEventForScanNumber(scanNumber).GetReaction(0);
         precursorMZ = reaction.PrecursorMass;
         isolationMZ = reaction.PrecursorMass;
         isolationWidth = reaction.IsolationWidth;
         charge = 0;
         var trailer = raw.GetTrailerExtraInformation(scanNumber);
         for (int i = 0; i < trailer.Length; ++i)
         {
            if (trailer.Labels[i] == "Monoisotopic M/Z:")
            {
               double dTmp;
               if (double.TryParse(trailer.Values[i], NumberStyles.Float, CultureInfo.InvariantCulture, out dTmp)
                     && dTmp != 0.0 && Math.Abs(dTmp - precursorMZ) < 10.0)
                  precursorMZ = dTmp;
            }
            else if (trailer.Labels[i] == "Charge State:")
            {
               double dZ;
               if (double.TryParse(trailer.Values[i], NumberStyles.Float, CultureInfo.InvariantCulture, out dZ))
                  charge = (int)dZ;
            }
         }
      }

      public static void Run(string[] args)
      {
         var o = new Options();
         if (!ParseOptions(args, o))
         {
            Usage();
            Environment.ExitCode = 1;
            return;
         }
         if (!File.Exists(o.Database))
         {
            Console.WriteLine(" Error: no index database at {0}", o.Database);
            Environment.ExitCode = 1;
            return;
         }
         foreach (string f in o.RawFiles)
         {
            if (!File.Exists(f))
            {
               Console.WriteLine(" Error: no RAW file at {0}", f);
               Environment.ExitCode = 1;
               return;
            }
         }
         if (o.OutputDir != null && !Directory.Exists(o.OutputDir))
         {
            Console.WriteLine(" Error: output directory {0} does not exist", o.OutputDir);
            Environment.ExitCode = 1;
            return;
         }
         // Two inputs that resolve to one output file (same basename, e.g. a/sample.raw and b/sample.raw
         // with --outdir, or a file named twice) would overwrite each other's results: refuse before
         // anything is searched or written.
         var outputOwner = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
         foreach (string f in o.RawFiles)
         {
            string outPath = Path.GetFullPath(OutputPathFor(o, f));
            string earlier;
            if (outputOwner.TryGetValue(outPath, out earlier))
            {
               Console.WriteLine(" Error: {0} and {1} would both write {2}", earlier, f, outPath);
               Environment.ExitCode = 1;
               return;
            }
            outputOwner[outPath] = f;
         }

         Console.WriteLine("\n RTS chimeric-spectrum search over RAW files\n");
         Console.WriteLine(" index:    {0}", o.Database);
         if (o.Plain)
            Console.WriteLine(" mode:     PLAIN search at the instrument's charge (--zrange, --tol, --max-expect, --max-peptides unused)");
         else if (o.TwoStage)
            Console.WriteLine(" mode:     TWO-STAGE: instrument precursor and charge first, then the isolation window (half-width {0}), charges {1}..{2}; secondary max E-value {3}{4}",
               (o.IsoHalfWidth > 0.0) ? o.IsoHalfWidth.ToString(CultureInfo.InvariantCulture) : "from the RAW file", o.ZMin, o.ZMax,
               (o.MaxExpect2 >= 0.0) ? o.MaxExpect2 : o.MaxExpect, o.SecondaryAlways ? "; secondary searched even without a first-stage peptide" : "");
         else
            Console.WriteLine(" charges:  {0}..{1} for every scan (the instrument's charge is ignored)", o.ZMin, o.ZMax);
         Console.WriteLine(" tol:      {0} (m/z; 0 = configured ppm)   max E-value: {1}   max peptides: {2}", o.TolMZ, o.MaxExpect, o.MaxPeptides);
         Console.WriteLine(" threads:  {0}   isotopes {1}   max_index_runtime {2} ms\n", o.Threads, o.Isotopes, o.MaxRuntimeMs);

         using (var searchMgr = new CometSearchManagerWrapper())
         {
            var settings = new RealTimeSearch.SearchSettings();
            string sDB = o.Database;
            string sRef = o.RawFiles[0];   // spectral_library_name: unused here, set for parity with the MS1/MS2 harness
            double dPeptideMassLow = 0, dPeptideMassHigh = 0;
            int numThreads = o.Threads;
            settings.ConfigureInputSettings(searchMgr, ref sRef, ref sDB, ref dPeptideMassLow, ref dPeptideMassHigh,
               ref numThreads, true, o.AScorePro, -1);
            searchMgr.SetParam("max_index_runtime", o.MaxRuntimeMs.ToString(), o.MaxRuntimeMs);
            searchMgr.SetParam("chimeric_remove_isotopes", o.Isotopes.ToString(), o.Isotopes);
            // Fragment binning: the harness defaults are the high-resolution settings; a low-resolution
            // (ion trap) file needs e.g. 1.0005 / 0.4 / 1.
            if (o.FragmentBinTol > 0.0)
               searchMgr.SetParam("fragment_bin_tol", o.FragmentBinTol.ToString(CultureInfo.InvariantCulture), o.FragmentBinTol);
            if (o.FragmentBinOffset >= 0.0)
               searchMgr.SetParam("fragment_bin_offset", o.FragmentBinOffset.ToString(CultureInfo.InvariantCulture), o.FragmentBinOffset);
            if (o.TheoreticalFragmentIons >= 0)
               searchMgr.SetParam("theoretical_fragment_ions", o.TheoreticalFragmentIons.ToString(), o.TheoreticalFragmentIons);

            if (!searchMgr.InitializeSingleSpectrumSearch())
            {
               string strStatusMsg = "";
               searchMgr.GetStatusMessage(ref strStatusMsg);
               Console.WriteLine(" Error initializing search index: " + strStatusMsg);
               Environment.ExitCode = 1;
               return;
            }
            Console.WriteLine(" index mass range {0:0.0} - {1:0.0}\n", dPeptideMassLow, dPeptideMassHigh);

            try
            {
               foreach (string rawPath in o.RawFiles)
               {
                  if (!SearchOneFile(searchMgr, o, rawPath))
                     Environment.ExitCode = 1;
               }
            }
            finally
            {
               searchMgr.FinalizeSingleSpectrumSearch();
            }

            string sPeakMem = searchMgr.GetPeakMemory();
            Console.WriteLine("{0} Done.{1}{2}", Environment.NewLine, string.IsNullOrEmpty(sPeakMem) ? "" : " (" + sPeakMem + ")", Environment.NewLine);
         }
      }

      private static bool SearchOneFile(CometSearchManagerWrapper searchMgr, Options o, string rawPath)
      {
         var inv = CultureInfo.InvariantCulture;
         string outFile = OutputPathFor(o, rawPath);

         Console.WriteLine(" file: {0}", rawPath);

         // Opened before the search so a bad output path fails at once.
         StreamWriter w;
         try
         {
            w = new StreamWriter(outFile, false);
         }
         catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException || ex is ArgumentException
                                    || ex is NotSupportedException)
         {
            Console.WriteLine(" Error: cannot write {0}: {1}", outFile, ex.Message);
            return false;
         }

         var outcomes = new ConcurrentBag<ScanOutcome>();
         var queue = new BlockingCollection<ScanItem>(boundedCapacity: 4 * o.Threads + 64);
         var watchAll = Stopwatch.StartNew();
         int nMs2 = 0;
         string readError = null;

         var tasks = new Task[o.Threads];
         for (int t = 0; t < o.Threads; ++t)
         {
            tasks[t] = Task.Run(() =>
            {
               foreach (ScanItem s in queue.GetConsumingEnumerable())
               {
                  var r = new ScanOutcome { Scan = s };
                  try
                  {
                     var watch = Stopwatch.StartNew();
                     List<string> vPeptide = null, vProtein = null;
                     List<List<FragmentWrapper>> vFrag = null;
                     List<ScoreWrapper> vScores = null;
                     bool bInc = false;
                     if (o.Plain)
                     {
                        // the plain search at the instrument's charge; no charge, no search. A false
                        // return here is "no match" (rejected spectrum or nothing scored), not an error.
                        r.Ok = true;
                        if (s.InstrumentCharge > 0)
                        {
                           searchMgr.DoSingleSpectrumSearchMultiResults(o.TopN, s.InstrumentCharge, s.PrecursorMZ,
                              s.Mass, s.Intensity, s.Mass.Length,
                              out vPeptide, out vProtein, out vFrag, out vScores);
                        }
                     }
                     else if (o.TwoStage)
                     {
                        // the instrument's precursor and charge first; then the isolation window on the reduced spectrum
                        double halfWidth = (o.IsoHalfWidth > 0.0) ? o.IsoHalfWidth : 0.5 * s.IsolationWidth;
                        r.Ok = searchMgr.DoSingleSpectrumSearchChimericTwoStage(s.PrecursorMZ, s.InstrumentCharge,
                           s.IsolationMZ, halfWidth, o.ZMin, o.ZMax, o.MaxExpect,
                           (o.MaxExpect2 >= 0.0) ? o.MaxExpect2 : o.MaxExpect, o.SecondaryAlways,
                           o.MaxPeptides, o.TopN, s.Mass, s.Intensity, s.Mass.Length,
                           out vPeptide, out vProtein, out vFrag, out vScores, out bInc);
                     }
                     else
                     {
                        // the scan's precursor m/z, searched at every charge zmin..zmax
                        r.Ok = searchMgr.DoSingleSpectrumSearchChimeric(s.PrecursorMZ, o.ZMin, o.ZMax, o.TolMZ, o.MaxExpect,
                           o.MaxPeptides, o.TopN, s.Mass, s.Intensity, s.Mass.Length,
                           out vPeptide, out vProtein, out vFrag, out vScores, out bInc);
                     }
                     r.Incomplete = bInc;
                     watch.Stop();
                     r.ElapsedMs = watch.Elapsed.TotalMilliseconds;
                     r.Peptides = vPeptide ?? new List<string>();
                     r.Proteins = vProtein ?? new List<string>();
                     r.Scores = vScores ?? new List<ScoreWrapper>();
                     if (!r.Ok)
                        r.Error = searchMgr.GetLastWrapperError();   // thread-static: read on this thread
                  }
                  catch (Exception ex)
                  {
                     r.Ok = false;
                     r.Incomplete = false;
                     r.Error = ex.GetType().Name + ": " + ex.Message;
                     r.Peptides = new List<string>();
                     r.Proteins = new List<string>();
                     r.Scores = new List<ScoreWrapper>();
                  }
                  s.Mass = null;       // release the peaks; only the row data is kept
                  s.Intensity = null;
                  outcomes.Add(r);
               }
            });
         }

         // Producer: this thread reads the RAW file (single-threaded: the file handle is shared).
         StreamWriter ms1Writer = null;
         if (o.Ms1Window > 0.0)
         {
            ms1Writer = new StreamWriter(outFile.Substring(0, outFile.Length - ".chimeric.txt".Length) + ".ms1win.txt");
            ms1Writer.WriteLine("scan\tisolation_mz\tisolation_width\tms1_scan\tms1_peaks");
         }
         try
         {
            using (IRawDataPlus raw = RawFileReaderAdapter.FileFactory(rawPath))
            {
               if (!raw.IsOpen || raw.IsError)
                  throw new IOException("unable to access the RAW file using the RawFileReader class");
               raw.SelectInstrument(Device.MS, 1);
               int first = raw.RunHeaderEx.FirstSpectrum, last = raw.RunHeaderEx.LastSpectrum;
               int ms1Scan = 0;               // --ms1-window: the most recent MS1 scan and its centroids
               double[] ms1Mass = null, ms1Inten = null;
               for (int scan = first; scan <= last; ++scan)
               {
                  MSOrderType order = raw.GetFilterForScanNumber(scan).MSOrder;
                  if (order == MSOrderType.Ms && ms1Writer != null)
                  {
                     var c = raw.GetCentroidStream(scan, false);
                     if (c != null && c.Length > 0)
                     {
                        ms1Mass = c.Masses;
                        ms1Inten = c.Intensities;
                     }
                     else
                     {
                        int n1;
                        ReadPeaks(raw, scan, out ms1Mass, out ms1Inten, out n1);
                     }
                     ms1Scan = scan;
                  }
                  if (order != MSOrderType.Ms2)
                     continue;
                  double[] mass, inten;
                  int n;
                  ReadPeaks(raw, scan, out mass, out inten, out n);
                  if (n < 1)
                     continue;
                  if (n != mass.Length || n != inten.Length)
                  {
                     Array.Resize(ref mass, n);
                     Array.Resize(ref inten, n);
                  }
                  double mz;
                  int z;
                  double isoMz, isoWidth;
                  ReadPrecursor(raw, scan, out mz, out z, out isoMz, out isoWidth);
                  if (ms1Writer != null)
                  {
                     var sb = new System.Text.StringBuilder();
                     sb.AppendFormat(inv, "{0}\t{1:0.00000}\t{2:0.###}\t{3}\t", scan, isoMz, isoWidth, ms1Scan);
                     if (ms1Mass != null)
                     {
                        int k = Array.BinarySearch(ms1Mass, isoMz - o.Ms1Window);
                        if (k < 0)
                           k = ~k;
                        for (; k < ms1Mass.Length && ms1Mass[k] <= isoMz + o.Ms1Window; ++k)
                           sb.AppendFormat(inv, "{0:0.00000}:{1:0};", ms1Mass[k], ms1Inten[k]);
                     }
                     ms1Writer.WriteLine(sb.ToString());
                  }
                  queue.Add(new ScanItem
                  {
                     ScanNumber = scan,
                     RetentionTimeSec = 60.0 * raw.RetentionTimeFromScanNumber(scan),
                     PrecursorMZ = mz,
                     InstrumentCharge = z,
                     IsolationMZ = isoMz,
                     IsolationWidth = isoWidth,
                     Mass = mass,
                     Intensity = inten
                  });
                  ++nMs2;
                  if (nMs2 % 2000 == 0)
                     Console.Write("\r read {0} MS/MS scans (scan {1} of {2})", nMs2, scan, last);
                  if (o.MaxScans > 0 && nMs2 >= o.MaxScans)
                     break;
               }
            }
         }
         catch (Exception ex)
         {
            readError = ex.Message;
         }
         finally
         {
            queue.CompleteAdding();
            if (ms1Writer != null)
               ms1Writer.Dispose();
         }
         Task.WaitAll(tasks);
         watchAll.Stop();
         Console.WriteLine();

         if (readError != null)
            Console.WriteLine(" Error reading {0}: {1} (the scans read before it are still reported)", rawPath, readError);

         // ---- output: one row per reported peptide, scans in file order ----
         var ordered = outcomes.OrderBy(x => x.Scan.ScanNumber).ToList();
         int[] histogram = new int[4];   // spectra with 0 / 1 / 2 / 3+ peptides
         int[] byCharge = new int[MaxPrecursorCharge + 1];
         int rows = 0, rowsLater = 0, errors = 0, incomplete = 0, decoyRows = 0;
         double sumMs = 0.0;
         using (w)
         {
            w.WriteLine("scan\titeration\tcharge\tprecursor_mz\tcandidates\tcalc_neutral_mass\tpeptide\tprotein\txcorr\tdeltacn\texpect\tions_matched\tions_total\telapsed_ms\trt_sec\tinstrument_charge\trank");
            int nCand = o.Plain ? 1 : Math.Max(0, Math.Min(o.ZMax, MaxPrecursorCharge) - o.ZMin + 1);
            foreach (ScanOutcome r in ordered)
            {
               ScanItem s = r.Scan;
               sumMs += r.ElapsedMs;
               string tail = string.Format(inv, "{0:0.00}\t{1}", s.RetentionTimeSec, s.InstrumentCharge);
               if (!r.Ok && !r.Incomplete)
               {
                  ++errors;
                  w.WriteLine(string.Format(inv, "{0}\tERROR\t{1}\t{2:0.0000}\t{3}", s.ScanNumber, OneField(r.Error), s.PrecursorMZ, nCand));
                  continue;
               }
               // A row past rank 0 may be an empty placeholder, and placeholders can sit between iterations'
               // rows, so rows are walked by index and empty ones skipped. The histogram counts iteration
               // winners (rank 0): with --topn > 1 one peptide's runner-ups are not further peptides.
               int n = r.Peptides.Count(p => !string.IsNullOrEmpty(p));
               int nWinners = 0;
               for (int i = 0; i < r.Peptides.Count; ++i)
               {
                  if (!string.IsNullOrEmpty(r.Peptides[i]) && r.Scores[i].Rank == 0)
                     ++nWinners;
               }
               histogram[Math.Min(nWinners, 3)] += 1;
               rows += n;
               if (r.Incomplete)
                  ++incomplete;
               if (n == 0 && !r.Incomplete)
               {
                  w.WriteLine(string.Format(inv, "{0}\tNO_MATCH\t\t{1:0.0000}\t{2}\t\t\t\t\t\t\t\t\t{3:0.00}\t{4}",
                     s.ScanNumber, s.PrecursorMZ, nCand, r.ElapsedMs, tail));
                  continue;
               }
               for (int i = 0; i < r.Peptides.Count; ++i)
               {
                  if (string.IsNullOrEmpty(r.Peptides[i]))
                     continue;
                  ScoreWrapper sc = r.Scores[i];
                  if (sc.Iteration >= 1)
                     ++rowsLater;
                  if (sc.Charge >= 0 && sc.Charge < byCharge.Length)
                     ++byCharge[sc.Charge];
                  if (r.Proteins[i].StartsWith("DECOY_", StringComparison.Ordinal))
                     ++decoyRows;
                  w.WriteLine(string.Format(inv, "{0}\t{1}\t{2}\t{3:0.0000}\t{4}\t{5:0.0000}\t{6}\t{7}\t{8:0.0000}\t{9:0.0000}\t{10:E3}\t{11}\t{12}\t{13:0.00}\t{14}\t{15}",
                     s.ScanNumber, sc.Iteration, sc.Charge, s.PrecursorMZ, nCand, sc.mass, r.Peptides[i], r.Proteins[i],
                     sc.xCorr, sc.dCn, sc.dExpect, sc.MatchedIons, sc.TotalIons, r.ElapsedMs, tail, sc.Rank));
               }
               if (r.Incomplete)
                  w.WriteLine(string.Format(inv, "{0}\tINCOMPLETE\t{1}\t{2:0.0000}\t{3}{4}", s.ScanNumber, r.Ok ? "budget" : "stopped",
                     s.PrecursorMZ, nCand, (!r.Ok && !string.IsNullOrEmpty(r.Error)) ? "\t" + OneField(r.Error) : ""));
            }
         }

         int nScans = ordered.Count;
         Console.WriteLine(" wrote {0}", outFile);
         Console.WriteLine(" MS/MS scans searched:             {0} in {1:0.0} s wall ({2} threads); mean call {3:0.0} ms",
            nScans, watchAll.Elapsed.TotalSeconds, o.Threads, nScans > 0 ? sumMs / nScans : 0.0);
         Console.WriteLine(" spectra with 0 / 1 / 2 / 3+ peptides: {0} / {1} / {2} / {3}", histogram[0], histogram[1], histogram[2], histogram[3]);
         Console.WriteLine(" peptide rows:                     {0} ({1} at iteration >= 1; {2} decoy)", rows, rowsLater, decoyRows);
         Console.Write(" rows by charge found at:         ");
         for (int z = o.ZMin; z <= Math.Min(o.ZMax, MaxPrecursorCharge); ++z)
            Console.Write(" {0}+:{1}", z, byCharge[z]);
         Console.WriteLine();
         if (errors > 0)
            Console.WriteLine(" calls that returned an error:     {0}", errors);
         if (incomplete > 0)
            Console.WriteLine(" calls that stopped early:         {0} (rows kept; budget or cancel)", incomplete);
         Console.WriteLine();
         return readError == null && !(nScans > 0 && errors == nScans);
      }
   }
}
