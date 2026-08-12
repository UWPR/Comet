namespace RealTimeSearch
{
   using System.Threading.Tasks;
   using System.Collections.Concurrent;

   using CometWrapper;
   using System;
   using System.Collections.Generic;
   using System.Diagnostics;
   using System.IO;
   using System.Linq;
   using System.Text;
   using System.Threading;
   using ThermoFisher.CommonCore.Data.Business;
   using ThermoFisher.CommonCore.Data.FilterEnums;
   using ThermoFisher.CommonCore.Data.Interfaces;
   using ThermoFisher.CommonCore.RawFileReader;

   /// <summary>
   /// Call CometWrapper to run searches, looping through scans in a Thermo RAW file
   /// </summary>
   class RealTimeSearch
   {
      // Thread-safe storage for results
      private class ScanResult
      {
         public int ScanNumber { get; set; }
         public MSOrderType ScanType { get; set; }
         public int ElapsedMs { get; set; }
         public List<string> Peptides { get; set; }
         public List<string> Proteins { get; set; }
         public List<ScoreWrapper> Scores { get; set; }
         public List<ScoreWrapperMS1> ScoresMS1 { get; set; }
         public double RT { get; set; }
         public double ExpMass { get; set; }
         public double Charge { get; set; }
         public double MZ { get; set; }

      }

      // Prototype: result of a single-threaded upfront pass over the raw file, done
      // once before any search thread starts, so no thread ever touches the shared
      // IRawDataPlus instance during the parallel search phase -- removes the need
      // for rawFileLock entirely instead of working around it. Valid=false covers
      // both non-MS1/MS2 scan types and MS1 scans skipped because MS1 search is off
      // (see the preload loop -- fetching a full MS1 profile trace that nothing will
      // ever search is pure waste, and under the old code it paid the rawFileLock
      // cost during the parallel phase for zero benefit).
      private struct PreloadedScan
      {
         public int ScanNumber;
         public MSOrderType ScanType;
         public double RT;
         public double[] Mass;
         public double[] Intensity;
         public int NumPeaks;
         public double PrecursorMZ;
         public int PrecursorCharge;
         public bool Valid;
      }

      // Pulls the value following a recognized flag out of args, advancing i past it.
      // Exits (rather than throwing/returning a sentinel) on a dangling flag with no value,
      // matching how int.TryParse failures below are handled -- a bad CLI invocation should
      // stop the process with a clear message, not silently fall through with a null/garbage
      // value that surfaces confusingly much later.
      private static string NextArg(string[] args, ref int i, string flagName)
      {
         if (i + 1 >= args.Length)
         {
            Console.WriteLine(" Error: {0} requires a value", flagName);
            Environment.Exit(1);
         }
         return args[++i];
      }

      private static void PrintUsage()
      {
         Console.WriteLine(" RTS MS1/MS2\n");
         Console.WriteLine("    USAGE:  {0} --query <query.raw> --ms1ref <MS1reference.raw> [options]\n",
            System.AppDomain.CurrentDomain.FriendlyName);
         Console.WriteLine("    Required:");
         Console.WriteLine("      --query <path>             raw file supplying the query MS2 spectra");
         Console.WriteLine("      --ms1ref <path>            raw file containing MS1 scans for MS1 alignment\n");
         Console.WriteLine("    Optional:");
         Console.WriteLine("      --db <path>                Comet .idx database to search (omit to skip MS2 database search)");
         Console.WriteLine("      --threads <n>              number of search threads (default: processor count)");
         Console.WriteLine("      --ascorepro <0|1>          0=off, 1=localize all variable mods (default: 1)");
         Console.WriteLine("      --index-search-type <0|1>  0=PI_DB (peptide index), 1=FI_DB (fragment ion index, default: 1)");
         Console.WriteLine("      --mask <path>              Carafe predicted-fragment mask file");
         Console.WriteLine("                                 (fragment_index_predicted_mask_file); FI_DB only, omit/empty = disabled");
         Console.WriteLine("      -h, --help                 show this message");
      }

      static void Main(string[] args)
      {
         string rawFileName = null;          // raw file that will supply the query spectra
         string sRawFileReference = null;     // raw file containing MS1 scans to search for MS1 alignment
         string sDB = "tmp";
         bool bDatabaseSearch = false;

         int defaultNumThreads = Environment.ProcessorCount;
         int numThreads = defaultNumThreads;
         bool bEnableAScorePro = true;         // 0=off, 1=localize all variable mods (maps to print_ascorepro_score=-1 internally)
         int iIndexSearchType = 1;             // 0=PI_DB (peptide index), 1=FI_DB (fragment ion index, default)
         string sPredictedMaskFile = "";       // fragment_index_predicted_mask_file; FI_DB only, empty = disabled

         for (int i = 0; i < args.Length; ++i)
         {
            string arg = args[i];

            switch (arg)
            {
               case "-h":
               case "--help":
                  PrintUsage();
                  return;

               case "--query":
                  rawFileName = NextArg(args, ref i, arg);
                  break;

               case "--ms1ref":
                  sRawFileReference = NextArg(args, ref i, arg);
                  break;

               case "--db":
                  sDB = NextArg(args, ref i, arg);
                  bDatabaseSearch = true;
                  break;

               case "--threads":
                  {
                     string sVal = NextArg(args, ref i, arg);
                     if (!int.TryParse(sVal, out numThreads) || numThreads < 1)
                     {
                        Console.WriteLine(" Warning: Invalid --threads '{0}', using {1} threads", sVal, defaultNumThreads);
                        numThreads = defaultNumThreads;
                     }
                  }
                  break;

               case "--ascorepro":
                  {
                     string sVal = NextArg(args, ref i, arg);
                     if (!int.TryParse(sVal, out int iAScoreProArg) || (iAScoreProArg != 0 && iAScoreProArg != 1))
                        Console.WriteLine(" Warning: Invalid --ascorepro '{0}', using default (1, on)", sVal);
                     else
                        bEnableAScorePro = (iAScoreProArg == 1);
                  }
                  break;

               case "--index-search-type":
                  {
                     // docs/20260730_PI_reduction.md Phase 0: matches the pre-unification
                     // default for an ambiguous .idx.
                     string sVal = NextArg(args, ref i, arg);
                     if (!int.TryParse(sVal, out iIndexSearchType) || (iIndexSearchType != 0 && iIndexSearchType != 1))
                     {
                        Console.WriteLine(" Warning: Invalid --index-search-type '{0}', using default (1, FI_DB)", sVal);
                        iIndexSearchType = 1;
                     }
                  }
                  break;

               case "--mask":
                  // docs/20260805_carafe.md Phase 4.
                  sPredictedMaskFile = NextArg(args, ref i, arg);
                  break;

               default:
                  Console.WriteLine(" Error: unrecognized argument '{0}'\n", arg);
                  PrintUsage();
                  Environment.Exit(1);
                  break;
            }
         }

         if (rawFileName == null || sRawFileReference == null)
         {
            PrintUsage();
            return;
         }

         Console.WriteLine("\n RTS MS1/MS2\n");
         Console.WriteLine(" Using {0} search threads\n", numThreads);

         // Create SINGLE global search manager
         CometSearchManagerWrapper globalSearchMgr = new CometSearchManagerWrapper();
         SearchSettings searchParams = new SearchSettings();
         double dPeptideMassLow = 0;
         double dPeptideMassHigh = 0;
         double dProtonMass = 1.00727646688;

         // Configure ONCE (before threading)
         searchParams.ConfigureInputSettings(globalSearchMgr,
            ref sRawFileReference,
            ref sDB,
            ref dPeptideMassLow,
            ref dPeptideMassHigh,
            ref numThreads,
            bDatabaseSearch,
            bEnableAScorePro,
            iIndexSearchType,
            sPredictedMaskFile);

         if (File.Exists(rawFileName) && File.Exists(sRawFileReference))
         {
            Console.Write(" query file: {0}  \n", rawFileName);
            Console.Write(" MS1 reference file: {0}  \n", sRawFileReference);
            if (bDatabaseSearch)
               Console.Write(" Indexed database: {0}  \n", sDB);
            Console.Write("\n");

            string outputFile = "rts.out";
            using (var rtsWriter = new StreamWriter(outputFile, append: false))
            {
               try
               {
                  // Open raw file ONCE; using ensures Dispose on all paths
                  using (IRawDataPlus rawFile = RawFileReaderAdapter.FileFactory(rawFileName))
                  {
                     if (!rawFile.IsOpen || rawFile.IsError)
                     {
                        Console.WriteLine(" Error: unable to access the RAW file using the RawFileReader class.");
                        return;
                     }

                     Stopwatch watchGlobal = new Stopwatch();
                     Stopwatch watchIndexCreate = new Stopwatch();
                     Stopwatch watchPreload = new Stopwatch();        // single-threaded wall clock around the upfront raw-file preload pass
                     Stopwatch watchParallelPhase = new Stopwatch();   // single-threaded wall clock around Task.Run..Task.WaitAll; Start/Stop each called exactly once, so unlike a Stopwatch shared/toggled from multiple concurrent threads, it can't race or under-count

                     watchGlobal.Start();

                     rawFile.SelectInstrument(Device.MS, 1);

                     // Get the first and last scan from the RAW file
                     int iFirstScan = rawFile.RunHeaderEx.FirstSpectrum;
                     int iLastScan = rawFile.RunHeaderEx.LastSpectrum;

                     // Get MS1 mass range from first MS1 scan
                     DoubleRangeWrapper MS1MassRange = null;

                     for (int iScanNumber = iFirstScan; iScanNumber <= iLastScan; ++iScanNumber)
                     {
                        var scanFilter = rawFile.GetFilterForScanNumber(iScanNumber);

                        if (scanFilter.MSOrder == MSOrderType.Ms)
                        {
                           var stats1 = rawFile.GetScanStatsForScanNumber(iScanNumber);
                           MS1MassRange = new DoubleRangeWrapper(stats1.LowMass, stats1.HighMass);
                           break;
                        }
                     }

                     if (MS1MassRange != null)
                     {
                        string MS1MassRangeString = MS1MassRange.get_dStart() + " " + MS1MassRange.get_dEnd();
                        globalSearchMgr.SetParam("ms1_mass_range", MS1MassRangeString, MS1MassRange);
                     }

                     // MS1 RT parameters
                     double dMaxMS1RTDiff = 100.0;    // maximum allowed retention time difference between query and reference, in seconds
                     double dMaxQueryRT = 60.0 * rawFile.RetentionTimeFromScanNumber(iLastScan);

                     int iPrintEveryScan = 1;
                     int iMS2TopN = 1; // report up to topN hits per MS/MS query
                     int iProteinLengthCutoff = 50;

                     bool bPerformMS1Search = false;
                     bool bPerformMS2Search = true;

                     // Thread-safe collections
                     var scanQueue = new ConcurrentQueue<int>();
                     var results = new ConcurrentBag<ScanResult>();
                     var progressLock = new object();
                     int scansProcessedMS2 = 0;
                     int totalScans = iLastScan - iFirstScan + 1;

                     // Initialize ONCE (before threading)
                     if (bPerformMS1Search)
                        globalSearchMgr.InitializeSingleSpectrumMS1Search(dMaxQueryRT);

                     watchIndexCreate.Start();
                     if (bDatabaseSearch && bPerformMS2Search)
                     {
                        if (!globalSearchMgr.InitializeSingleSpectrumSearch())
                        {
                           // A corrupt/truncated .idx (or a missing underlying FASTA for auto-build)
                           // makes this return false with the specific error already logged on the
                           // native side (CometSearchManager::InitializeSingleSpectrumSearch()) --
                           // this return value used to be silently discarded here, so a load failure
                           // fell through into per-spectrum search with no index loaded instead of
                           // aborting.
                           string strStatusMsg = "";
                           globalSearchMgr.GetStatusMessage(ref strStatusMsg);
                           Console.WriteLine(" Error initializing search index: " + strStatusMsg);
                           System.Environment.Exit(1);
                        }
                     }
                     watchIndexCreate.Stop();

                     Console.WriteLine();

                     // Prototype: single-threaded upfront preload of every scan's data from the
                     // raw file, done BEFORE any search thread starts. This is the only code that
                     // ever touches `rawFile` -- no lock is needed because nothing else runs
                     // concurrently with this loop. MS1 scans are only fully read (full profile
                     // trace -- can be ~20K points, far bigger than a centroided MS2 spectrum)
                     // when bPerformMS1Search is actually on; otherwise only cheap metadata
                     // (RT/scan type) is captured so downstream skip logic still works.
                     var preloadedScans = new PreloadedScan[totalScans];

                     watchPreload.Start();
                     for (int iScanNumber = iFirstScan; iScanNumber <= iLastScan; ++iScanNumber)
                     {
                        int idx = iScanNumber - iFirstScan;

                        var scanStatistics = rawFile.GetScanStatsForScanNumber(iScanNumber);
                        double dRT = 60.0 * rawFile.RetentionTimeFromScanNumber(iScanNumber);
                        var scanFilter = rawFile.GetFilterForScanNumber(iScanNumber);
                        MSOrderType eScanOrder = scanFilter.MSOrder;

                        if (eScanOrder != MSOrderType.Ms && eScanOrder != MSOrderType.Ms2)
                        {
                           preloadedScans[idx] = new PreloadedScan { ScanNumber = iScanNumber, ScanType = eScanOrder, RT = dRT, Valid = false };
                           continue;
                        }

                        if (eScanOrder == MSOrderType.Ms && !bPerformMS1Search)
                        {
                           preloadedScans[idx] = new PreloadedScan { ScanNumber = iScanNumber, ScanType = eScanOrder, RT = dRT, Valid = false };
                           continue;
                        }

                        double[] pdMass;
                        double[] pdInten;
                        int iNumPeaks;

                        if (scanStatistics.IsCentroidScan && (scanStatistics.SpectrumPacketType == SpectrumPacketType.FtCentroid))
                        {
                           var centroidStream = rawFile.GetCentroidStream(iScanNumber, false);
                           iNumPeaks = centroidStream.Length;
                           pdMass = centroidStream.Masses;
                           pdInten = centroidStream.Intensities;
                        }
                        else
                        {
                           var segmentedScan = rawFile.GetSegmentedScanFromScanNumber(iScanNumber, scanStatistics);
                           iNumPeaks = segmentedScan.Positions.Length;
                           pdMass = segmentedScan.Positions;
                           pdInten = segmentedScan.Intensities;
                        }

                        double dPrecursorMZ = 0;
                        int iPrecursorCharge = 0;

                        if (eScanOrder == MSOrderType.Ms2)
                        {
                           dPrecursorMZ = rawFile.GetScanEventForScanNumber(iScanNumber).GetReaction(0).PrecursorMass;

                           var trailerData = rawFile.GetTrailerExtraInformation(iScanNumber);
                           for (int i = 0; i < trailerData.Length; ++i)
                           {
                              if (trailerData.Labels[i] == "Monoisotopic M/Z:")
                              {
                                 double dTmp = double.Parse(trailerData.Values[i]);
                                 double dMassDiff = Math.Abs(dTmp - dPrecursorMZ);

                                 if (dTmp != 0.0 && dMassDiff < 10.0)
                                    dPrecursorMZ = dTmp;
                              }
                              else if (trailerData.Labels[i] == "Charge State:")
                                 iPrecursorCharge = (int)double.Parse(trailerData.Values[i]);
                           }
                        }

                        preloadedScans[idx] = new PreloadedScan
                        {
                           ScanNumber = iScanNumber,
                           ScanType = eScanOrder,
                           RT = dRT,
                           Mass = pdMass,
                           Intensity = pdInten,
                           NumPeaks = iNumPeaks,
                           PrecursorMZ = dPrecursorMZ,
                           PrecursorCharge = iPrecursorCharge,
                           Valid = true
                        };
                     }
                     watchPreload.Stop();

                     // Populate scan queue
                     for (int i = iFirstScan; i <= iLastScan; ++i)
                        scanQueue.Enqueue(i);

                     // Simplified worker thread function
                     void ProcessScans(int threadId)
                     {
                        Stopwatch watch = new Stopwatch();

                        // Process scans from queue -- purely in-memory, no shared-resource locking:
                        // every scan's data was already read by the single-threaded preload pass above.
                        while (scanQueue.TryDequeue(out int iScanNumber))
                        {
                           try
                           {
                              PreloadedScan pre = preloadedScans[iScanNumber - iFirstScan];

                              if (!pre.Valid)
                                 continue;

                              if (pre.NumPeaks < 10)  // don't bother searching sparse spectra
                              {
                                 continue;
                              }

                              ScanResult result = new ScanResult
                              {
                                 ScanNumber = pre.ScanNumber,
                                 ScanType = pre.ScanType,
                                 RT = pre.RT
                              };

                              // Perform search using SHARED manager (C++ is thread-safe via mutexes)
                              if (bPerformMS1Search && pre.ScanType == MSOrderType.Ms)
                              {
                                 int iMS1TopN = 1;
                                 watch.Restart();

                                 // Use SHARED globalSearchMgr (thread-safe on C++ side)
                                 globalSearchMgr.DoMS1SearchMultiResults(dMaxMS1RTDiff, dMaxQueryRT, iMS1TopN, pre.RT,
                                    pre.Mass, pre.Intensity, pre.NumPeaks, out List<ScoreWrapperMS1> vScores);

                                 watch.Stop();

                                 result.ScoresMS1 = vScores;
                                 result.ElapsedMs = (int)watch.ElapsedMilliseconds;
                              }
                              else if (bPerformMS2Search && pre.ScanType == MSOrderType.Ms2)
                              {
                                 double dExpPepMass = (pre.PrecursorCharge * pre.PrecursorMZ) - (pre.PrecursorCharge - 1) * dProtonMass;

                                 result.ExpMass = dExpPepMass - dProtonMass;  // store neutral mass
                                 result.Charge = pre.PrecursorCharge;
                                 result.MZ = pre.PrecursorMZ;

                                 if (dExpPepMass < dPeptideMassLow || dExpPepMass > dPeptideMassHigh)
                                 {
                                    continue;
                                 }

                                 watch.Restart();

                                 // Use SHARED globalSearchMgr (thread-safe on C++ side)
                                 globalSearchMgr.DoSingleSpectrumSearchMultiResults(iMS2TopN, pre.PrecursorCharge, pre.PrecursorMZ,
                                    pre.Mass, pre.Intensity, pre.NumPeaks,
                                    out List<string> vPeptide,
                                    out List<string> vProtein,
                                    out List<List<FragmentWrapper>> vMatchingFragments,
                                    out List<ScoreWrapper> vScores);

                                 watch.Stop();

                                 double elapsedThisSpec = watch.Elapsed.TotalMilliseconds;

                                 result.Peptides = vPeptide;
                                 result.Proteins = vProtein;
                                 result.Scores = vScores;
                                 result.ElapsedMs = (int)elapsedThisSpec;

                                 lock (progressLock)
                                 {
                                    scansProcessedMS2++;
                                 }
                              }

                              results.Add(result);

                              // Update progress
                              lock (progressLock)
                              {
                                 if (scansProcessedMS2 % 500 == 0)
                                 {
                                    double percentComplete = (double)iScanNumber / iLastScan * 100.0;
                                    Console.Write("\r Progress: {0:F1}% ({1} MS2 scans of {2} total scans)", percentComplete, scansProcessedMS2, totalScans);
                                 }
                              }
                           }
                           catch (Exception ex)
                           {
                              Console.WriteLine("\n Thread {0}: Error processing scan {1}: {2}", threadId, iScanNumber, ex.Message);
                           }
                        }
                     }

                     TimeSpan elapsedGlobal = TimeSpan.Zero;
                     try
                     {
                        // Launch worker threads
                        Task[] tasks = new Task[numThreads];
                        watchParallelPhase.Start();
                        for (int i = 0; i < numThreads; ++i)
                        {
                           int threadId = i;
                           tasks[i] = Task.Run(() => ProcessScans(threadId));
                        }

                        // Wait for all threads to complete
                        Task.WaitAll(tasks);
                        watchParallelPhase.Stop();
                        Console.WriteLine(); // newline after progress

                        watchGlobal.Stop();
                        elapsedGlobal = watchGlobal.Elapsed;
                     }
                     finally
                     {
                        // Cleanup ONCE -- in finally so resources are freed even on exception
                        if (bDatabaseSearch && bPerformMS2Search)
                           globalSearchMgr.FinalizeSingleSpectrumSearch();
                        if (bPerformMS1Search)
                           globalSearchMgr.FinalizeSingleSpectrumMS1Search();
                     }

                     // Process and display results
                     var sortedResults = results.OrderBy(r => r.ScanNumber).ToList();

                     // Create histograms
                     int iMaxHistogramTime = 500;
                     int[] piTimeSearchMS1 = new int[iMaxHistogramTime];
                     int[] piTimeSearchMS2 = new int[iMaxHistogramTime];
                     var slowestRuns = new List<(int TimeMs, string Peptide, int ScanNumber, double XCorr)>();
                     double ms1ElapsedTotalMs = 0.0;   // sum of each MS1 search's own accurately-measured (thread-local watch) duration

                     foreach (var result in sortedResults)
                     {
                        int iTime = result.ElapsedMs;

                        if (result.ScanType == MSOrderType.Ms && result.ScoresMS1 != null && result.ScoresMS1.Count > 0)
                        {
                           ms1ElapsedTotalMs += result.ElapsedMs;   // use the uncapped value, not the histogram-clamped iTime below

                           if (iTime >= iMaxHistogramTime)
                              iTime = iMaxHistogramTime - 1;
                           if (iTime >= 0)
                              piTimeSearchMS1[iTime] += 1;

                           if ((result.ScanNumber % iPrintEveryScan) == 0)
                           {
                              string line = string.Format("*MS1 {0}  libscan {1}  queryRT {2:F2}  libRT {3:F2}  dotp {4:F3}  {5} ms",
                                  result.ScanNumber, result.ScoresMS1[0].iScanNumber, result.RT,
                                  result.ScoresMS1[0].fRTime, result.ScoresMS1[0].fDotProduct, iTime);
                              //Console.WriteLine(line);
                              rtsWriter.WriteLine(line);
                           }
                        }
                        else if (result.ScanType == MSOrderType.Ms2 && result.Peptides != null && result.Peptides.Count > 0)
                        {
                           if (iTime >= iMaxHistogramTime)
                              iTime = iMaxHistogramTime - 1;
                           if (iTime >= 0)
                              piTimeSearchMS2[iTime] += 1;

                           if (result.Peptides[0].Length > 0)
                           {
                              slowestRuns.Add((iTime, result.Peptides[0], result.ScanNumber, result.Scores[0].xCorr));
                              slowestRuns = slowestRuns.OrderByDescending(x => x.TimeMs).Take(5).ToList();
                           }

                           if ((result.ScanNumber % iPrintEveryScan) == 0)
                           {
                              string protein = result.Proteins[0];
                              if (protein.Length > iProteinLengthCutoff)
                                 protein = protein.Substring(0, iProteinLengthCutoff);

                              // replace space with semicolon for easy parsing
                              string sAScoreProSiteScores = result.Scores[0].sAScoreProSiteScores.Replace(' ', ';');

                              string line = string.Format(" MS2 {0}\t{1}  {2:F4}  {3:0.##E+00}  z {10}  exp {4:F4}  calc {5:F4}  AScore {6:F2}  Sites '{7}'  {8} ms  prot '{9}'",
                                  result.ScanNumber, result.Peptides[0], result.Scores[0].xCorr, result.Scores[0].dExpect,
                                  result.ExpMass, result.Scores[0].mass,
                                  result.Scores[0].dAScorePro, sAScoreProSiteScores,
                                  iTime, protein, result.Charge);
                              //Console.WriteLine(line);
                              rtsWriter.WriteLine(line);
/*
                              if (Math.Abs(result.ExpMass - result.Scores[0].mass) > 5.0)
                              {
                                 string warn = string.Format(" **** Warning: large mass error for scan {0}: {1:F4} Da", result.ScanNumber, result.ExpMass - result.Scores[0].mass);
                                 Console.WriteLine(warn);
                                 rtsWriter.WriteLine(warn);
                              }
*/
                           }
                        }
                     }

                     // Write histogram
                     {
                        int iTot = 0;
                        int iAbove5ms = 0;
                        int iAbove10ms = 0;
                        for (int i = 0; i < iMaxHistogramTime; ++i)
                        {
                           string line1 = $"histogram\t{i}\t{piTimeSearchMS1[i]}\t{piTimeSearchMS2[i]}";
                           rtsWriter.WriteLine(line1);

                           iTot += piTimeSearchMS2[i];
                           if (i > 5)
                              iAbove5ms += piTimeSearchMS2[i];
                           if (i > 10)
                              iAbove10ms += piTimeSearchMS2[i];
                        }

                        Console.WriteLine("\n 5 Slowest MS2 Runs:");
                        rtsWriter.WriteLine("\n 5 Slowest MS2 Runs:");
                        Console.WriteLine(" Time(ms)\tScan\tPeptide\tXcorr");
                        rtsWriter.WriteLine(" Time(ms)\tScan\tPeptide\tXcorr");
                        foreach (var run in slowestRuns.OrderByDescending(x => x.TimeMs))
                        {
                           string line1 = $" {run.TimeMs}\t{run.ScanNumber}\t{run.Peptide}\t{run.XCorr:F4}";
                           Console.WriteLine(line1);
                           rtsWriter.WriteLine(line1);
                        }

                        string line = string.Format("\n <= 5 ms: {0}, > 5 ms: {1} ({3:F3}%), > 10ms {2} ({4:F3}%)\n",
                           iTot - iAbove5ms, iAbove5ms, iAbove10ms,
                           iTot > 0 ? ((double)iAbove5ms / iTot) * 100.0 : 0,
                           iTot > 0 ? ((double)iAbove10ms / iTot) * 100.0 : 0);
                        rtsWriter.WriteLine(line);

                        double dAvgTimePerScan = scansProcessedMS2 > 0 ? watchParallelPhase.Elapsed.TotalMilliseconds / (double)scansProcessedMS2 : 0;
                        double dHz = dAvgTimePerScan > 0 ? 1000.0 / dAvgTimePerScan : 0;

                        line = string.Format("\n initialize elapsed time: {0:F2} s", watchIndexCreate.Elapsed.TotalSeconds);
                        rtsWriter.WriteLine(line);
                        Console.WriteLine(line);

                        line = string.Format(" raw file preload elapsed time: {0:F2} s", watchPreload.Elapsed.TotalSeconds);
                        rtsWriter.WriteLine(line);
                        Console.WriteLine(line);

                        line = string.Format( " MS2 search elapsed time: {0:F2} s", watchParallelPhase.Elapsed.TotalSeconds);
                        rtsWriter.WriteLine(line);
                        Console.WriteLine(line);

                        line = string.Format(" MS2 average search time: {0:F2} ms/spectrum ({1} spectra), {2:F0} Hz", dAvgTimePerScan, scansProcessedMS2, dHz);
                        rtsWriter.WriteLine(line);
                        Console.WriteLine(line);

                        line = string.Format(" MS1 search elapsed time: {0:F2} s", ms1ElapsedTotalMs / 1000.0);
                        rtsWriter.WriteLine(line);
                        Console.WriteLine(line);

                        line = string.Format("      total elapsed time: {0:F2} s", watchGlobal.Elapsed.TotalSeconds);
                        rtsWriter.WriteLine(line);
                        Console.WriteLine(line);

                     }

                  } // end using rawFile -- Dispose called automatically
               }
               catch (Exception rawSearchEx)
               {
                  Console.WriteLine(" Error: " + rawSearchEx.Message);
                  Console.WriteLine(" Stack trace: " + rawSearchEx.StackTrace);
               }
            }
         }
         else
         {
            Console.WriteLine("No raw file exists at that path.");
         }

         string sPeakMem = globalSearchMgr.GetPeakMemory();
         string sMemSuffix = string.IsNullOrEmpty(sPeakMem) ? "" : $" ({sPeakMem})";
         Console.WriteLine("{0} Done.{1}{2}", Environment.NewLine, sMemSuffix, Environment.NewLine);
         return;
      }

      class SearchSettings
      {
         public bool ConfigureInputSettings(CometSearchManagerWrapper SearchMgr,
            ref string sRawFileReference,
            ref string sDB,
            ref double dPeptideMassLow,
            ref double dPeptideMassHigh,
            ref int numThreads,
            bool bDatabaseSearch,
            bool bEnableAScorePro,
            int iIndexSearchType,
            string sPredictedMaskFile)
         {
            String sTmp;
            int iTmp;
            double dTmp;

            SearchMgr.SetParam("spectral_library_name", sRawFileReference, sRawFileReference);
            SearchMgr.SetParam("database_name", sDB, sDB);

            iTmp = 1; // MS level
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("spectral_library_ms_level", sTmp, iTmp);

            iTmp = 300; // retention time tolerance in seconds
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("retentiontime_tol", sTmp, iTmp);

            dTmp = 1.0005; // MS1 mass bin width
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("ms1_bin_tol", sTmp, dTmp);

            dTmp = 0.4;  // MS1 mass bin offset
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("ms1_bin_offset", sTmp, dTmp);

            double dMS1MassLow = 0.0;
            double dMS1MassHigh = 2000.0;
            var MS1MassRange = new DoubleRangeWrapper(dMS1MassLow, dMS1MassHigh);
            string MS1MassRangeString = dMS1MassLow.ToString() + " " + dMS1MassHigh.ToString();
            SearchMgr.SetParam("ms1_mass_range", MS1MassRangeString, MS1MassRange);

            SearchMgr.SetParam("num_threads", numThreads.ToString(), numThreads);

            dTmp = 0.02; // fragment bin width
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("fragment_bin_tol", sTmp, dTmp);

            dTmp = 0.0;  // fragment bin offset
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("fragment_bin_offset", sTmp, dTmp);

            iTmp = 0; // 0=use flanking peaks, 1=M peak only
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("theoretical_fragment_ions", sTmp, iTmp);

            dTmp = 20.0;  // peptide mass tolerance plus
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("peptide_mass_tolerance_upper", sTmp, dTmp);

            dTmp = -20.0;  // peptide mass tolerance minus ; if this is not set, will use -1*peptide_mass_tolerance_plus
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("peptide_mass_tolerance_lower", sTmp, dTmp);

            iTmp = 2; // 0=Da, 2=ppm
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

            iTmp = 1; // 0 = Da, 1 = m/z tolerance
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("precursor_tolerance_type", sTmp, iTmp);

            iTmp = 0; // 0=off, 1=0/1 (C13 error), 2=0/1/2, 3=0/1/2/3, 4=-1/0/1/2/3, 5=-1/0/1
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("isotope_error", sTmp, iTmp);

            iTmp = 500; // search time cutoff in milliseconds
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_index_runtime", sTmp, iTmp);

            iTmp = 10;
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("minimum_peaks", sTmp, iTmp);

            dTmp = 0.0; // intensity cutoff for each peak (default)
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("minimum_intensity", sTmp, dTmp);

            iTmp = 3; // maximum fragment charge
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_fragment_charge", sTmp, iTmp);

            iTmp = 1; // minimum precursor charge
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("min_precursor_charge", sTmp, iTmp);

            iTmp = 6; // maximum precursor charge
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_precursor_charge", sTmp, iTmp);

            iTmp = 0; // 0=I and L are different, 1=I and L are same
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("equal_I_and_L", sTmp, iTmp);

            dTmp = 0.0; // base peak percentage cutoff
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("percentage_base_peak", sTmp, dTmp);

            iTmp = 1;
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("use_B_ions", sTmp, iTmp);

            iTmp = 1;
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("use_Y_ions", sTmp, iTmp);

            // 0=off, -1=localize all mods; otherwise 1 for variable_mod01, 2 for variable_mod02, etc.
            iTmp = bEnableAScorePro ? -1 : 0;
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("print_ascorepro_score", sTmp, iTmp);

            // docs/20260730_PI_reduction.md Phase 0: which search mode to run against the
            // (now-shared-format) .idx file. 0=PI_DB, 1=FI_DB (default). Since PI_DB and
            // FI_DB share one on-disk format, the file itself no longer implies a mode --
            // this is the RTS-side equivalent of batch's index_search_type comet.params key.
            sTmp = iIndexSearchType.ToString();
            SearchMgr.SetParam("index_search_type", sTmp, iIndexSearchType);

            // docs/20260805_carafe.md Phase 4: Carafe predicted-fragment mask
            // (fragment_index_predicted_mask_file). Unlike the mods/enzyme/etc. settings
            // below, which only matter the one time a missing .idx gets auto-built, the mask
            // is consumed by CometFragmentIndex::CreateFragmentIndex() -- which runs on
            // EVERY process start against an FI_DB .idx, not just the one that wrote the file
            // -- so this must be set unconditionally here, outside the
            // System.IO.File.Exists(sDB) branch below. A no-op on the native side (empty
            // string, or a PI_DB search) -- see CometPredictedMask::Load()'s own doc comment.
            SearchMgr.SetParam("fragment_index_predicted_mask_file", sPredictedMaskFile, sPredictedMaskFile);

            if (bDatabaseSearch)
            {
               if (System.IO.File.Exists(sDB))
               {
                  // The .idx already exists and is fully self-describing as of
                  // docs/20260811_restore_idx_header_mods.md's v4 format:
                  // CometPeptideIndex::ParsePeptideIndexHeader() reads enzyme, static mods,
                  // variable mods (identity AND count limits), decoy mode, and mass/length
                  // range straight from the file's own header on the C++ side and overwrites
                  // whatever comet.params/SetParam() supplied -- nothing needs to be set here
                  // for any of that any more. The one thing still needed on the C# side is
                  // dPeptideMassLow/dPeptideMassHigh themselves: they're used above (as a
                  // cheap pre-filter to skip spectra outside the index's mass range before
                  // ever calling into the search) independently of anything forwarded to
                  // SearchMgr, so this still peeks MassRange: directly, but no longer
                  // forwards it (or LengthRange:, which nothing on the C# side needs) via
                  // SetParam -- the C++ side already has its own copy.
                  using (System.IO.StreamReader dbFile = new System.IO.StreamReader(sDB))
                  {
                     bool bFoundMassRange = false;
                     string strLine;

                     while ((strLine = dbFile.ReadLine()) != null)
                     {
                        string[] strParsed = strLine.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

                        // Blank line: end of header, start of the protein-name section
                        // (matches CometPeptideIndex::ParsePeptideIndexHeader()'s own loop
                        // terminator on the C++ side). Also guards strParsed[0] below --
                        // Split(...RemoveEmptyEntries) on a blank line returns an empty
                        // array, which would otherwise throw here on a malformed/truncated
                        // .idx before the "missing MassRange header" error ever got a
                        // chance to report it.
                        if (strParsed.Length == 0)
                           break;

                        if (strParsed[0].Equals("MassRange:"))
                        {
                           if (strParsed.Length >= 3
                              && double.TryParse(strParsed[1], out double dLow)
                              && double.TryParse(strParsed[2], out double dHigh))
                           {
                              dPeptideMassLow = dLow;
                              dPeptideMassHigh = dHigh;
                              bFoundMassRange = true;
                           }
                           break;
                        }
                     }
                     dbFile.Close();

                     if (!bFoundMassRange)
                     {
                        Console.WriteLine(" Error with indexed database format; missing MassRange header.\n");
                        System.Environment.Exit(1);
                     }
                  }
               }
               else
               {
                  // .idx file does not exist so set appropriate parameters here for generating
                  // fragment ion indexing's plain peptide .idx -- everything in this branch
                  // only matters at build time; once the .idx exists, its own header carries
                  // all of it and none of this runs again. TODO(user): extend this to
                  // optionally load these settings from an actual comet.params file instead
                  // of hardcoding them here.

                  // digest mass range
                  dPeptideMassLow = 800.0;
                  dPeptideMassHigh = 4000.0;
                  var digestMassRange = new DoubleRangeWrapper(dPeptideMassLow, dPeptideMassHigh);
                  string digestMassRangeString = dPeptideMassLow.ToString() + " " + dPeptideMassHigh.ToString();
                  SearchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange);

                  // digest length range
                  int iLengthMin = 8;
                  int iLengthMax = 25;
                  var peptideLengthRange = new IntRangeWrapper(iLengthMin, iLengthMax);
                  string peptideLengthRangeString = dPeptideMassLow.ToString() + " " + dPeptideMassHigh.ToString();
                  SearchMgr.SetParam("peptide_length_range", peptideLengthRangeString, peptideLengthRange);

                  // parent/fragment mass types
                  iTmp = 1;  // 0=average, 1=monoisotopic
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("mass_type_parent", sTmp, iTmp);

                  iTmp = 1;  // 0=average, 1=monoisotopic
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("mass_type_fragment", sTmp, iTmp);

                  // static mods
                  dTmp = 57.021464;
                  sTmp = dTmp.ToString();
                  SearchMgr.SetParam("add_C_cysteine", sTmp, dTmp);

                  // enzyme settings
                  var enzymeInfo = new EnzymeInfoWrapper();
                  string enzymeInfoString = "0. Cut_everywhere  0 - -\n1. Trypsin 1 KR P";
                  enzymeInfo.set_SearchEnzymeName("Cut_everywhere");
                  enzymeInfo.set_SearchEnzymeBreakAA("-");
                  enzymeInfo.set_SearchEnzymeNoBreakAA("-");
                  enzymeInfo.set_SearchEnzymeOffSet(1); // 0=cleave before; 1=cleave after
                  enzymeInfo.set_SearchEnzyme2Name("Cut_everywhere");
                  enzymeInfo.set_SearchEnzyme2BreakAA("-");
                  enzymeInfo.set_SearchEnzyme2NoBreakAA("-");
                  enzymeInfo.set_SearchEnzyme2OffSet(0); // 0=cleave before; 1=cleave after
                  SearchMgr.SetParam("[COMET_ENZYME_INFO]", enzymeInfoString, enzymeInfo);

                  iTmp = 3;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("allowed_missed_cleavage", sTmp, iTmp);

                  iTmp = 2; // 1=semi-digested, 2=fully-digested (default), 8=C-term unspecific, 9=N-term unspecific
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("num_enzyme_termini", sTmp, iTmp);

                  iTmp = 0; // 0=no decoys (default), 1=internal decoy concatenated, 2=internal decoy separate
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("decoy_search", sTmp, iTmp);

                  iTmp = 0; // 0=leave protein sequences alone (default), 1=also consider w/o N-term methionine
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("clip_nterm_methionine", sTmp, iTmp);

                  iTmp = 20; // maximum number of duplicate proteins to report/store per peptide (default)
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("max_duplicate_proteins", sTmp, iTmp);

                  // FI setting
                  iTmp = 3;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("fragindex_min_ions_score", sTmp, iTmp);
                  iTmp = 3;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("fragindex_min_ions_report", sTmp, iTmp);
                  iTmp = 150;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("fragindex_num_spectrumpeaks", sTmp, iTmp);
                  dTmp = 200.0;
                  sTmp = dTmp.ToString();
                  SearchMgr.SetParam("fragindex_min_fragmentmass", sTmp, dTmp);
                  dTmp = 2000.0;
                  sTmp = dTmp.ToString();
                  SearchMgr.SetParam("fragindex_max_fragmentmass", sTmp, dTmp);
                  iTmp = 1;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("fragindex_skipreadprecursors", sTmp, iTmp);

                  // Variable mods: only matter here, at build time -- baked into the new .idx's
                  // own header (docs/20260811_restore_idx_header_mods.md v4) and never needed
                  // again for any subsequent run against it. M oxidation + STY phospho (with
                  // phospho's neutral loss), matching data/comet_phospho.params.
                  VarModsWrapper varMods = new VarModsWrapper();
                  sTmp = "15.9949 M 0 2 -1 0 0 0.0";
                  varMods.set_VarModMass(15.9949);
                  varMods.set_VarModChar("M");
                  varMods.set_BinaryMod(0);
                  varMods.set_MaxNumVarModAAPerMod(2);
                  varMods.set_RequireThisMod(0);
                  varMods.set_VarModTermDistance(-1);
                  varMods.set_WhichTerm(0);
                  varMods.set_VarNeutralLoss(0.0);
                  SearchMgr.SetParam("variable_mod01", sTmp, varMods);

                  sTmp = "79.9663 STY 0 2 -1 0 0 97.976896";
                  varMods.set_VarModMass(79.9663);
                  varMods.set_VarModChar("STY");
                  varMods.set_BinaryMod(0);
                  varMods.set_MaxNumVarModAAPerMod(2);
                  varMods.set_RequireThisMod(0);
                  varMods.set_VarModTermDistance(-1);
                  varMods.set_WhichTerm(0);
                  varMods.set_VarNeutralLoss(97.976896);
                  SearchMgr.SetParam("variable_mod02", sTmp, varMods);

                  iTmp = 4;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("max_variable_mods_in_peptide", sTmp, iTmp);
               }
            }

            return true;
         }
      }
   }
}