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
         public string OutputDir = null;      // default: next to each RAW file
      }

      private class ScanItem
      {
         public int ScanNumber;
         public double RetentionTimeSec;
         public double PrecursorMZ;
         public int InstrumentCharge;         // as reported by the instrument; not used for the search
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
      private static void ReadPrecursor(IRawDataPlus raw, int scanNumber, out double precursorMZ, out int charge)
      {
         precursorMZ = raw.GetScanEventForScanNumber(scanNumber).GetReaction(0).PrecursorMass;
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

         Console.WriteLine("\n RTS chimeric-spectrum search over RAW files\n");
         Console.WriteLine(" index:    {0}", o.Database);
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
         string outFile = Path.Combine(o.OutputDir ?? Path.GetDirectoryName(Path.GetFullPath(rawPath)),
            Path.GetFileNameWithoutExtension(rawPath) + ".chimeric.txt");

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
                     bool bInc;
                     // the scan's precursor m/z, searched at every charge zmin..zmax
                     r.Ok = searchMgr.DoSingleSpectrumSearchChimeric(s.PrecursorMZ, o.ZMin, o.ZMax, o.TolMZ, o.MaxExpect,
                        o.MaxPeptides, o.TopN, s.Mass, s.Intensity, s.Mass.Length,
                        out vPeptide, out vProtein, out vFrag, out vScores, out bInc);
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
         try
         {
            using (IRawDataPlus raw = RawFileReaderAdapter.FileFactory(rawPath))
            {
               if (!raw.IsOpen || raw.IsError)
                  throw new IOException("unable to access the RAW file using the RawFileReader class");
               raw.SelectInstrument(Device.MS, 1);
               int first = raw.RunHeaderEx.FirstSpectrum, last = raw.RunHeaderEx.LastSpectrum;
               for (int scan = first; scan <= last; ++scan)
               {
                  if (raw.GetFilterForScanNumber(scan).MSOrder != MSOrderType.Ms2)
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
                  ReadPrecursor(raw, scan, out mz, out z);
                  queue.Add(new ScanItem
                  {
                     ScanNumber = scan,
                     RetentionTimeSec = 60.0 * raw.RetentionTimeFromScanNumber(scan),
                     PrecursorMZ = mz,
                     InstrumentCharge = z,
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
            int nCand = Math.Max(0, Math.Min(o.ZMax, MaxPrecursorCharge) - o.ZMin + 1);
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
