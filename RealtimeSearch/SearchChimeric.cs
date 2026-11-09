namespace RealTimeSearch
{
   using System;
   using System.Collections.Concurrent;
   using System.Collections.Generic;
   using System.Diagnostics;
   using System.Globalization;
   using System.IO;
   using System.Linq;
   using System.Text;
   using System.Threading.Tasks;

   using CometWrapper;

   /// <summary>
   /// Chimeric-spectrum search harness (docs/20261105_RTSchimeric.md, section 6): drives
   /// CometSearchManagerWrapper.DoSingleSpectrumSearchChimeric() over every scan of an .ms2
   /// file, from N worker threads pulling off a shared queue -- the same concurrency pattern as
   /// SearchMS1MS2.cs -- and writes one tab-delimited row per reported peptide plus a run summary.
   ///
   /// Two forms, selected on the command line:
   ///   charge-range form (default): each scan's precursor m/z is searched at every charge in
   ///       [zmin, zmax] with a +/- tolMZ window (the isolation-window use case);
   ///   explicit-list form (--zlines): each Z line of the scan is one (m/z, charge) candidate,
   ///       m/z = (MH+ + (z-1) * proton) / z, searched with the configured (ppm) tolerance unless
   ///       --tol is given -- the multi-Z-line .ms2 that mango_search_wrapper.py consumes and writes.
   ///
   /// Selected by "chimeric" as the first argument of RealtimeSearch.exe (see Main in
   /// SearchMS1MS2.cs). Search parameters come from SearchSettings.ConfigureInputSettings(), the
   /// same block the MS1/MS2 harness uses, plus the chimeric_* knobs set from the options below.
   /// </summary>
   internal static class SearchChimeric
   {
      private const double ProtonMass = 1.00727646688;

      private class Ms2Scan
      {
         public int ScanNumber;
         public double PrecursorMZ;   // S-line m/z; 0.0 in files that carry only Z-line MH+ values
         public List<KeyValuePair<int, double>> ZLines = new List<KeyValuePair<int, double>>();   // (charge, MH+)
         public double[] Mass;
         public double[] Intensity;
      }

      private class Options
      {
         public string Ms2File;
         public string Database;
         public bool ZLines = false;
         public int ZMin = 2;
         public int ZMax = 4;
         public double TolMZ = 0.0;
         public double MaxExpect = 0.1;   // 0.1 since fragindex_histogram_fill calibrated the E-values (docs/20261106_FI_evalue_histogram_fill.md)
         public int MaxPeptides = 5;
         public int Threads = Environment.ProcessorCount;
         public bool AScorePro = false;
         public int Isotopes = 2;
         public int MaxRuntimeMs = 0;
         public string OutputFile = null;
      }

      private class ScanOutcome
      {
         public int ScanNumber;
         public double PrecursorMZ;
         public int Candidates;
         public bool Ok;
         public bool Incomplete;   // the call stopped early: budget (Ok) or cancelled / failed (!Ok); its rows are kept
         public string Error;      // why an ERROR call failed: GetLastWrapperError(), or the exception this scan threw
         public double ElapsedMs;
         public List<string> Peptides;
         public List<string> Proteins;
         public List<ScoreWrapper> Scores;
      }

      private static void Usage()
      {
         Console.WriteLine(" RTS chimeric-spectrum search\n");
         Console.WriteLine("    USAGE:  {0} chimeric <spectra.ms2> <database.idx> [options]\n",
            System.AppDomain.CurrentDomain.FriendlyName);
         Console.WriteLine("    --zrange zmin zmax    charge-range form (default): each scan at charges zmin..zmax [2 4]");
         Console.WriteLine("    --zlines              explicit-list form: one candidate per Z line of the scan");
         Console.WriteLine("    --tol mz              +/- precursor window in m/z units; 0 = configured 20 ppm [0]");
         Console.WriteLine("    --max-expect E        report a peptide only if its E-value <= E [0.1]");
         Console.WriteLine("    --max-peptides N      at most N peptides per spectrum [5]");
         Console.WriteLine("    --threads T           search threads [processor count]");
         Console.WriteLine("    --ascorepro 0|1       AScorePro localization [0]");
         Console.WriteLine("    --isotopes N          chimeric_remove_isotopes: +1..+N 13C peaks of matched ions, 0-3 [2]");
         Console.WriteLine("    --max-runtime ms      max_index_runtime budget for one whole chimeric call; 0 = unlimited [0]");
         Console.WriteLine("    --out file            output file [<spectra>.chimeric.txt]\n");
      }

      private static bool ParseOptions(string[] args, Options o)
      {
         // args[0] == "chimeric"
         if (args.Length < 3)
            return false;
         o.Ms2File = args[1];
         o.Database = args[2];
         var inv = CultureInfo.InvariantCulture;
         try
         {
            for (int i = 3; i < args.Length; ++i)
            {
               switch (args[i])
               {
                  case "--zrange": o.ZMin = int.Parse(args[++i], inv); o.ZMax = int.Parse(args[++i], inv); break;
                  case "--zlines": o.ZLines = true; break;
                  case "--tol": o.TolMZ = double.Parse(args[++i], inv); break;
                  case "--max-expect": o.MaxExpect = double.Parse(args[++i], inv); break;
                  case "--max-peptides": o.MaxPeptides = int.Parse(args[++i], inv); break;
                  case "--threads": o.Threads = Math.Max(1, int.Parse(args[++i], inv)); break;
                  case "--ascorepro": o.AScorePro = int.Parse(args[++i], inv) != 0; break;
                  case "--isotopes": o.Isotopes = int.Parse(args[++i], inv); break;
                  case "--max-runtime": o.MaxRuntimeMs = int.Parse(args[++i], inv); break;
                  case "--out": o.OutputFile = args[++i]; break;
                  default:
                     Console.WriteLine(" Error: unknown option '{0}'", args[i]);
                     return false;
               }
            }
         }
         catch (Exception ex) when (ex is FormatException || ex is IndexOutOfRangeException || ex is OverflowException)
         {
            Console.WriteLine(" Error: bad option value ({0})", ex.Message);
            return false;
         }
         if (o.OutputFile == null)
            o.OutputFile = Path.ChangeExtension(o.Ms2File, null) + ".chimeric.txt";
         return true;
      }

      /// <summary>
      /// Minimal .ms2 reader: H lines skipped; S scan scan m/z; Z charge MH+; I/D lines skipped;
      /// everything else "m/z intensity". Every Z line of a scan is kept (the explicit-list form).
      /// </summary>
      private static List<Ms2Scan> ReadMs2(string path)
      {
         var scans = new List<Ms2Scan>();
         var inv = CultureInfo.InvariantCulture;
         Ms2Scan cur = null;
         var mass = new List<double>();
         var inten = new List<double>();
         var sep = new char[] { ' ', '\t' };

         Action flush = () =>
         {
            if (cur != null)
            {
               cur.Mass = mass.ToArray();
               cur.Intensity = inten.ToArray();
               scans.Add(cur);
            }
            mass.Clear();
            inten.Clear();
         };

         using (var reader = new StreamReader(path))
         {
            string line;
            int iLine = 0;
            while ((line = reader.ReadLine()) != null)
            {
               ++iLine;
               if (line.Length == 0)
                  continue;
               char c = line[0];
               if (c == 'H' || c == 'I' || c == 'D')
                  continue;
               string[] f = line.Split(sep, StringSplitOptions.RemoveEmptyEntries);
               try
               {
                  if (c == 'S')
                  {
                     flush();
                     cur = new Ms2Scan();
                     cur.ScanNumber = int.Parse(f[1], inv);
                     cur.PrecursorMZ = f.Length >= 4 ? double.Parse(f[3], inv) : 0.0;
                  }
                  else if (c == 'Z')
                  {
                     if (cur != null && f.Length >= 3)
                        cur.ZLines.Add(new KeyValuePair<int, double>(int.Parse(f[1], inv), double.Parse(f[2], inv)));
                  }
                  else if (cur != null && f.Length >= 2 && (char.IsDigit(c) || c == '.'))
                  {
                     mass.Add(double.Parse(f[0], inv));
                     inten.Add(double.Parse(f[1], inv));
                  }
               }
               catch (Exception ex) when (ex is FormatException || ex is IndexOutOfRangeException || ex is OverflowException)
               {
                  throw new InvalidDataException(string.Format("{0} line {1}: cannot parse \"{2}\" ({3})", path, iLine, line, ex.Message));
               }
            }
            flush();
         }
         return scans;
      }

      // Tabs and line breaks would split an output row, so a free-text field is flattened.
      private static string OneField(string s)
      {
         if (string.IsNullOrEmpty(s))
            return "";
         return s.Replace('\t', ' ').Replace('\r', ' ').Replace('\n', ' ');
      }

      private static double MzFromMH(double dMH, int z)
      {
         return (dMH + (z - 1) * ProtonMass) / z;
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
         if (!File.Exists(o.Ms2File))
         {
            Console.WriteLine(" Error: no .ms2 file at {0}", o.Ms2File);
            Environment.ExitCode = 1;
            return;
         }
         if (!File.Exists(o.Database))
         {
            Console.WriteLine(" Error: no index database at {0}", o.Database);
            Environment.ExitCode = 1;
            return;
         }

         Console.WriteLine("\n RTS chimeric-spectrum search\n");
         Console.WriteLine(" spectra: {0}", o.Ms2File);
         Console.WriteLine(" index:   {0}", o.Database);
         Console.WriteLine(" form:    {0}", o.ZLines ? "explicit list (one candidate per Z line)" : string.Format("charge range {0}..{1}", o.ZMin, o.ZMax));
         Console.WriteLine(" tol:     {0} (m/z; 0 = configured ppm)   max E-value: {1}   max peptides: {2}", o.TolMZ, o.MaxExpect, o.MaxPeptides);
         Console.WriteLine(" removal: flank follows theoretical_fragment_ions, isotopes {0}; max_index_runtime {1} ms",
            o.Isotopes, o.MaxRuntimeMs);
         Console.WriteLine(" threads: {0}\n", o.Threads);

         List<Ms2Scan> scans;
         try
         {
            scans = ReadMs2(o.Ms2File);
         }
         catch (Exception ex) when (ex is InvalidDataException || ex is IOException || ex is UnauthorizedAccessException)
         {
            Console.WriteLine(" Error reading spectra: {0}", ex.Message);
            Environment.ExitCode = 1;
            return;
         }
         Console.WriteLine(" read {0} scans", scans.Count);

         // Opened before the search, so a bad --out path fails at once rather than after it.
         StreamWriter w;
         try
         {
            w = new StreamWriter(o.OutputFile, false);
         }
         catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException || ex is ArgumentException
                                    || ex is NotSupportedException)
         {
            Console.WriteLine(" Error: cannot write {0}: {1}", o.OutputFile, ex.Message);
            Environment.ExitCode = 1;
            return;
         }

         using (var searchMgr = new CometSearchManagerWrapper())
         {
            var settings = new RealTimeSearch.SearchSettings();
            string sDB = o.Database;
            string sRef = o.Ms2File;   // spectral_library_name: unused here, set for parity with the MS1/MS2 harness
            double dPeptideMassLow = 0, dPeptideMassHigh = 0;
            int numThreads = o.Threads;
            settings.ConfigureInputSettings(searchMgr, ref sRef, ref sDB, ref dPeptideMassLow, ref dPeptideMassHigh,
               ref numThreads, true, o.AScorePro, -1);

            // One chimeric call runs (candidates x iterations) searches, so the per-call budget the
            // MS1/MS2 harness sets (500 ms) is replaced by the caller's choice (0 = unlimited).
            searchMgr.SetParam("max_index_runtime", o.MaxRuntimeMs.ToString(), o.MaxRuntimeMs);
            searchMgr.SetParam("chimeric_remove_isotopes", o.Isotopes.ToString(), o.Isotopes);

            if (!searchMgr.InitializeSingleSpectrumSearch())
            {
               string strStatusMsg = "";
               searchMgr.GetStatusMessage(ref strStatusMsg);
               Console.WriteLine(" Error initializing search index: " + strStatusMsg);
               Environment.ExitCode = 1;
               w.Dispose();
               return;
            }
            Console.WriteLine(" index mass range {0:0.0} - {1:0.0}\n", dPeptideMassLow, dPeptideMassHigh);

            var queue = new ConcurrentQueue<int>(Enumerable.Range(0, scans.Count));
            var outcomes = new ConcurrentBag<ScanOutcome>();
            var watchAll = Stopwatch.StartNew();

            var tasks = new Task[o.Threads];
            for (int t = 0; t < o.Threads; ++t)
            {
               tasks[t] = Task.Run(() =>
               {
                  int idx;
                  while (queue.TryDequeue(out idx))
                  {
                     Ms2Scan s = scans[idx];
                     var r = new ScanOutcome { ScanNumber = s.ScanNumber, PrecursorMZ = s.PrecursorMZ };
                     // One scan that throws is recorded as an ERROR row; it no longer ends the run in
                     // Task.WaitAll, losing every result and skipping FinalizeSingleSpectrumSearch().
                     try
                     {
                        if (s.PrecursorMZ == 0.0 && s.ZLines.Count > 0)
                           r.PrecursorMZ = MzFromMH(s.ZLines[0].Value, s.ZLines[0].Key);   // Mango-style file: S m/z is 0, Z lines carry MH+

                        var watch = Stopwatch.StartNew();
                        List<string> vPeptide = null, vProtein = null;
                        List<List<FragmentWrapper>> vFrag = null;
                        List<ScoreWrapper> vScores = null;
                        if (o.ZLines)
                        {
                           int n = s.ZLines.Count;
                           var mz = new double[n];
                           var z = new int[n];
                           for (int k = 0; k < n; ++k)
                           {
                              z[k] = s.ZLines[k].Key;
                              mz[k] = MzFromMH(s.ZLines[k].Value, z[k]);
                           }
                           r.Candidates = n;
                           bool bInc = false;
                           r.Ok = n > 0 && searchMgr.DoSingleSpectrumSearchChimeric(mz, z, null, o.TolMZ, o.MaxExpect, o.MaxPeptides,
                              s.Mass, s.Intensity, s.Mass.Length, out vPeptide, out vProtein, out vFrag, out vScores, out bInc);
                           r.Incomplete = bInc;
                        }
                        else
                        {
                           r.Candidates = o.ZMax - o.ZMin + 1;
                           bool bInc;
                           r.Ok = searchMgr.DoSingleSpectrumSearchChimeric(r.PrecursorMZ, o.ZMin, o.ZMax, o.TolMZ, o.MaxExpect, o.MaxPeptides,
                              s.Mass, s.Intensity, s.Mass.Length, out vPeptide, out vProtein, out vFrag, out vScores, out bInc);
                           r.Incomplete = bInc;
                        }
                        watch.Stop();
                        r.ElapsedMs = watch.Elapsed.TotalMilliseconds;
                        r.Peptides = vPeptide ?? new List<string>();
                        r.Proteins = vProtein ?? new List<string>();
                        r.Scores = vScores ?? new List<ScoreWrapper>();
                        if (!r.Ok && !r.Incomplete)
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
                     outcomes.Add(r);
                  }
               });
            }
            Task.WaitAll(tasks);
            watchAll.Stop();

            searchMgr.FinalizeSingleSpectrumSearch();

            // ---- output: one row per reported peptide, scans in file order ----
            var ordered = outcomes.OrderBy(x => x.ScanNumber).ToList();
            var inv = CultureInfo.InvariantCulture;
            int[] histogram = new int[4];   // spectra with 0 / 1 / 2 / 3+ peptides
            int rows = 0, rowsLater = 0, repeats = 0, errors = 0, incomplete = 0;
            double sumMs = 0.0;

            using (w)
            {
               w.WriteLine("scan\titeration\tcharge\tprecursor_mz\tcandidates\tcalc_neutral_mass\tpeptide\tprotein\txcorr\tdeltacn\texpect\tions_matched\tions_total\telapsed_ms");
               foreach (ScanOutcome r in ordered)
               {
                  sumMs += r.ElapsedMs;
                  // ERROR: false without Incomplete (invalid input). A call that stopped early keeps its
                  // rows (complete iterations) and gets an INCOMPLETE line after them: "budget" when
                  // max_index_runtime ran out (Ok), "stopped" when cancelled or a search failed (!Ok).
                  if (!r.Ok && !r.Incomplete)
                  {
                     ++errors;
                     w.WriteLine(string.Format(inv, "{0}\tERROR\t{1}\t{2:0.0000}\t{3}", r.ScanNumber, OneField(r.Error), r.PrecursorMZ, r.Candidates));
                     continue;
                  }
                  int n = r.Peptides.Count;
                  histogram[Math.Min(n, 3)] += 1;
                  rows += n;
                  if (r.Incomplete)
                     ++incomplete;
                  if (n == 0 && !r.Incomplete)
                  {
                     w.WriteLine(string.Format(inv, "{0}\tNO_MATCH\t\t{1:0.0000}\t{2}\t\t\t\t\t\t\t\t\t{3:0.00}", r.ScanNumber, r.PrecursorMZ, r.Candidates, r.ElapsedMs));
                     continue;
                  }
                  var seen = new HashSet<string>();
                  for (int i = 0; i < n; ++i)
                  {
                     ScoreWrapper sc = r.Scores[i];
                     if (sc.Iteration >= 1)
                        ++rowsLater;
                     if (!seen.Add(r.Peptides[i]))
                        ++repeats;
                     w.WriteLine(string.Format(inv, "{0}\t{1}\t{2}\t{3:0.0000}\t{4}\t{5:0.0000}\t{6}\t{7}\t{8:0.0000}\t{9:0.0000}\t{10:E3}\t{11}\t{12}\t{13:0.00}",
                        r.ScanNumber, sc.Iteration, sc.Charge, r.PrecursorMZ, r.Candidates, sc.mass, r.Peptides[i], r.Proteins[i],
                        sc.xCorr, sc.dCn, sc.dExpect, sc.MatchedIons, sc.TotalIons, r.ElapsedMs));
                  }
                  if (r.Incomplete)
                     w.WriteLine(string.Format(inv, "{0}\tINCOMPLETE\t{1}\t{2:0.0000}\t{3}", r.ScanNumber, r.Ok ? "budget" : "stopped", r.PrecursorMZ, r.Candidates));
               }
            }

            int nScans = ordered.Count;
            Console.WriteLine(" wrote {0}", o.OutputFile);
            Console.WriteLine(" spectra searched:                 {0} in {1:0.0} s wall ({2} threads); mean call {3:0.0} ms",
               nScans, watchAll.Elapsed.TotalSeconds, o.Threads, nScans > 0 ? sumMs / nScans : 0.0);
            Console.WriteLine(" spectra with 0 / 1 / 2 / 3+ peptides: {0} / {1} / {2} / {3}", histogram[0], histogram[1], histogram[2], histogram[3]);
            Console.WriteLine(" peptide rows:                     {0} ({1} at iteration >= 1)", rows, rowsLater);
            Console.WriteLine(" same peptide twice in a spectrum: {0}", repeats);
            if (errors > 0)
               Console.WriteLine(" calls that returned an error:     {0}", errors);
            if (incomplete > 0)
               Console.WriteLine(" calls that stopped early:         {0} (rows kept; budget or cancel)", incomplete);
            string sPeakMem = searchMgr.GetPeakMemory();
            Console.WriteLine("{0} Done.{1}{2}", Environment.NewLine, string.IsNullOrEmpty(sPeakMem) ? "" : " (" + sPeakMem + ")", Environment.NewLine);
         }
      }
   }
}
