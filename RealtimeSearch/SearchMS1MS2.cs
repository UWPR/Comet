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

      static void Main(string[] args)
      {
         if (args.Length < 2)
         {
            Console.WriteLine(" RTS MS1/MS2\n");
            Console.WriteLine("    USAGE:  {0} [query.raw] [MS1reference.raw] [database.idx] [num_threads]\n",
               System.AppDomain.CurrentDomain.FriendlyName);
            return;
         }

         Console.WriteLine("\n RTS MS1/MS2\n");

         string rawFileName = args[0];       // raw file that will supply the query spectra
         string sRawFileReference = args[1]; // raw file containing MS1 scans to search for MS1 alignment
         string sDB = "tmp";

         bool bDatabaseSearch = false;

         if (args.Length >= 3)
         {
            sDB = args[2];
            bDatabaseSearch = true;
         }

         // Parse number of threads (default to processor count)
         int numThreads = Environment.ProcessorCount;
         if (args.Length >= 4)
         {
            if (!int.TryParse(args[3], out numThreads) || numThreads < 1)
            {
               Console.WriteLine(" Warning: Invalid num_threads '{0}', using {1} threads", args[3], numThreads);
               numThreads = Environment.ProcessorCount;
            }
         }

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
            bDatabaseSearch);

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
                  // Open raw file ONCE
                  IRawDataPlus rawFile = RawFileReaderAdapter.FileFactory(rawFileName);
                  if (!rawFile.IsOpen || rawFile.IsError)
                  {
                     Console.WriteLine(" Error: unable to access the RAW file using the RawFileReader class.");
                     return;
                  }

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
                  double dMaxMS1RTDiff = 300.0;    // maximum allowed retention time difference between query and reference, in seconds
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
                  int scansProcessed = 0;
                  int totalScans = iLastScan - iFirstScan + 1;

                  // Initialize ONCE (before threading)
                  if (bPerformMS1Search)
                     globalSearchMgr.InitializeSingleSpectrumMS1Search(dMaxQueryRT);


                  Stopwatch watchIndexCreate = new Stopwatch();
                  watchIndexCreate.Start();
                  if (bDatabaseSearch && bPerformMS2Search)
                     globalSearchMgr.InitializeSingleSpectrumSearch();
                  watchIndexCreate.Stop();

                  // Populate scan queue
                  for (int i = iFirstScan; i <= iLastScan; ++i)
                     scanQueue.Enqueue(i);

                  Stopwatch watchGlobal = new Stopwatch();
                  watchGlobal.Start();

                  // Simplified worker thread function
                  void ProcessScans(int threadId)
                  {
                     Stopwatch watch = new Stopwatch();

                     // Process scans from queue using SHARED resources
                     while (scanQueue.TryDequeue(out int iScanNumber))
                     {
                        try
                        {
                           // Use SHARED raw file reader (thread-safe)
                           var scanStatistics = rawFile.GetScanStatsForScanNumber(iScanNumber);
                           double dRT = 60.0 * rawFile.RetentionTimeFromScanNumber(iScanNumber);
                           var scanFilter = rawFile.GetFilterForScanNumber(iScanNumber);

                           if (scanFilter.MSOrder != MSOrderType.Ms && scanFilter.MSOrder != MSOrderType.Ms2)
                           {
                              // Update progress for non-searchable scans
                              lock (progressLock)
                              {
                                 scansProcessed++;
                              }
                              continue;
                           }

                           // Get spectral data
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

                           if (iNumPeaks < 10)  // don't bother searching sparse spectra
                           {
                              lock (progressLock)
                              {
                                 scansProcessed++;
                              }
                              continue;
                           }

                           ScanResult result = new ScanResult
                           {
                              ScanNumber = iScanNumber,
                              ScanType = scanFilter.MSOrder,
                              RT = dRT
                           };

                           // Perform search using SHARED manager (C++ is thread-safe via mutexes)
                           if (bPerformMS1Search && scanFilter.MSOrder == MSOrderType.Ms)
                           {
                              int iMS1TopN = 1;
                              watch.Restart();

                              // Use SHARED globalSearchMgr (thread-safe on C++ side)
                              globalSearchMgr.DoMS1SearchMultiResults(dMaxMS1RTDiff, dMaxQueryRT, iMS1TopN, dRT,
                                 pdMass, pdInten, iNumPeaks, out List<ScoreWrapperMS1> vScores);

                              watch.Stop();
                              result.ScoresMS1 = vScores;
                              result.ElapsedMs = (int)watch.ElapsedMilliseconds;
                           }
                           else if (bPerformMS2Search && scanFilter.MSOrder == MSOrderType.Ms2)
                           {
                              int iPrecursorCharge = 0;
                              double dPrecursorMZ = rawFile.GetScanEventForScanNumber(iScanNumber).GetReaction(0).PrecursorMass;

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

                              double dExpPepMass = (iPrecursorCharge * dPrecursorMZ) - (iPrecursorCharge - 1) * dProtonMass;

                              result.ExpMass = dExpPepMass - dProtonMass;  // store neutral mass
                              result.Charge = iPrecursorCharge;
                              result.MZ = dPrecursorMZ;

                              if (dExpPepMass < dPeptideMassLow || dExpPepMass > dPeptideMassHigh)
                              {
                                 lock (progressLock)
                                 {
                                    scansProcessed++;
                                 }
                                 continue;
                              }

                              watch.Restart();

                              // Use SHARED globalSearchMgr (thread-safe on C++ side)
                              globalSearchMgr.DoSingleSpectrumSearchMultiResults(iMS2TopN, iPrecursorCharge, dPrecursorMZ,
                                 pdMass, pdInten, iNumPeaks,
                                 out List<string> vPeptide,
                                 out List<string> vProtein,
                                 out List<List<FragmentWrapper>> vMatchingFragments,
                                 out List<ScoreWrapper> vScores);

                              watch.Stop();
                              result.Peptides = vPeptide;
                              result.Proteins = vProtein;
                              result.Scores = vScores;
                              result.ElapsedMs = (int)watch.ElapsedMilliseconds;
                           }

                           results.Add(result);

                           // Update progress
                           lock (progressLock)
                           {
                              scansProcessed++;
                              if (scansProcessed % 100 == 0)
                              {
                                 double percentComplete = (double)scansProcessed / totalScans * 100.0;
                                 Console.Write("\r Progress: {0:F1}% ({1}/{2} scans)", percentComplete, scansProcessed, totalScans);
                              }
                           }
                        }
                        catch (Exception ex)
                        {
                           Console.WriteLine("\n Thread {0}: Error processing scan {1}: {2}", threadId, iScanNumber, ex.Message);
                           lock (progressLock)
                           {
                              scansProcessed++;
                           }
                        }
                     }
                  }

                  // Launch worker threads
                  Task[] tasks = new Task[numThreads];
                  for (int i = 0; i < numThreads; ++i)
                  {
                     int threadId = i;
                     tasks[i] = Task.Run(() => ProcessScans(threadId));
                  }

                  // Wait for all threads to complete
                  Task.WaitAll(tasks);
                  Console.WriteLine(); // newline after progress

                  watchGlobal.Stop();
                  TimeSpan elapsedGlobal = watchGlobal.Elapsed;

                  // Cleanup ONCE
                  if (bPerformMS2Search)
                     globalSearchMgr.FinalizeSingleSpectrumSearch();
                  if (bPerformMS1Search)
                     globalSearchMgr.FinalizeSingleSpectrumMS1Search();

                  // Process and display results
                  var sortedResults = results.OrderBy(r => r.ScanNumber).ToList();

                  // Create histograms
                  int iMaxHistogramTime = 50;
                  int[] piTimeSearchMS1 = new int[iMaxHistogramTime];
                  int[] piTimeSearchMS2 = new int[iMaxHistogramTime];
                  var slowestRuns = new List<(int TimeMs, string Peptide, int ScanNumber, double XCorr)>();

                  foreach (var result in sortedResults)
                  {
                     int iTime = result.ElapsedMs;

                     if (result.ScanType == MSOrderType.Ms && result.ScoresMS1 != null && result.ScoresMS1.Count > 0)
                     {
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
                           rtsWriter.Flush();
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
                           rtsWriter.Flush();

                           if (Math.Abs(result.ExpMass - result.Scores[0].mass) > 5.0)
                           {
                              string warn = string.Format(" **** Warning: large mass error for scan {0}: {1:F4} Da", result.ScanNumber, result.ExpMass - result.Scores[0].mass);
                              Console.WriteLine(warn);
                              rtsWriter.WriteLine(warn);
                              rtsWriter.Flush();
                           }
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
                        Console.WriteLine(line1);
                        rtsWriter.WriteLine(line1);

                        iTot += piTimeSearchMS2[i];
                        if (i > 5)
                           iAbove5ms += piTimeSearchMS2[i];
                        if (i > 10)
                           iAbove10ms += piTimeSearchMS2[i];
                     }

                     Console.WriteLine("\n5 Slowest MS2 Runs:");
                     rtsWriter.WriteLine("\n5 Slowest MS2 Runs:");
                     Console.WriteLine("Time(ms)\tScan\tPeptide\tXcorr");
                     rtsWriter.WriteLine("Time(ms)\tScan\tPeptide\tXcorr");
                     foreach (var run in slowestRuns.OrderByDescending(x => x.TimeMs))
                     {
                        string line1 = $"{run.TimeMs}\t{run.ScanNumber}\t{run.Peptide}\t{run.XCorr:F4}";
                        Console.WriteLine(line1);
                        rtsWriter.WriteLine(line1);
                     }

                     string line = string.Format("\n<= 5 ms: {0}, > 5 ms: {1} ({3:F3}%), > 10ms {2} ({4:F3}%)\n",
                        iTot - iAbove5ms, iAbove5ms, iAbove10ms,
                        iTot > 0 ? ((double)iAbove5ms / iTot) * 100.0 : 0,
                        iTot > 0 ? ((double)iAbove10ms / iTot) * 100.0 : 0);
                     rtsWriter.WriteLine(line);

                     line = string.Format("\nTotal elapsed time: {0:F2} minutes", elapsedGlobal.TotalMinutes);
                     rtsWriter.WriteLine(line);

                     line = string.Format("Scans processed: {0}", scansProcessed);
                     rtsWriter.WriteLine(line);

                     line = string.Format("Average time per scan: {0:F2} ms", elapsedGlobal.TotalMilliseconds / scansProcessed);
                     rtsWriter.WriteLine(line);

                     line = string.Format("\nindex creation elapsed time: {0:F2} s", watchIndexCreate.Elapsed.TotalSeconds);
                     rtsWriter.WriteLine(line);
                     Console.WriteLine(line);
                     line = string.Format("search elapsed time: {0:F2} s\n", watchGlobal.Elapsed.TotalSeconds);
                     rtsWriter.WriteLine(line);
                     Console.WriteLine(line);

                  }

                  rawFile.Dispose();
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

         Console.WriteLine("{0} Done.{1}", Environment.NewLine, Environment.NewLine);
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
            bool bDatabaseSearch)
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

            iTmp = 100; // search time cutoff in milliseconds
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_index_runtime", sTmp, iTmp);

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

            iTmp = 2; // 0=off, 1=0/1 (C13 error), 2=0/1/2, 3=0/1/2/3, 4=-1/0/1/2/3, 5=-1/0/1
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("isotope_error", sTmp, iTmp);

            iTmp = 100; // search time cutoff in milliseconds
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_index_runtime", sTmp, iTmp);

            iTmp = 10;
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("minimum_peaks", sTmp, iTmp);

            iTmp = 3; // maximum fragment charge
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_fragment_charge", sTmp, iTmp);

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

            iTmp = -1;  // 0=unused, -1=localize all mods; otherwise 1 for variable_mod01, 2 for variable_mod02, etc.
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("print_ascorepro_score", sTmp, iTmp);

            if (bDatabaseSearch)
            {
               // If the .idx file already exists, read the mass and length ranges from it.
               // Otherwise set those here (before index is created automatically)

               if (System.IO.File.Exists(sDB))
               {
                  using (System.IO.StreamReader dbFile = new System.IO.StreamReader(sDB))
                  {
                     // Now actually open the .idx database to read mass range from it
                     int iLineCount = 0;
                     bool bFoundMassRange = false;
                     string strLine;

                     while ((strLine = dbFile.ReadLine()) != null)
                     {
                        string[] strParsed = strLine.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                        if (strParsed[0].Equals("MassRange:"))
                        {
                           dPeptideMassLow = double.Parse(strParsed[1]);
                           dPeptideMassHigh = double.Parse(strParsed[2]);

                           var digestMassRange = new DoubleRangeWrapper(dPeptideMassLow, dPeptideMassHigh);
                           string digestMassRangeString = dPeptideMassLow.ToString() + " " + dPeptideMassHigh.ToString();
                           SearchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange);

                           bFoundMassRange = true;
                        }

                        if (strParsed[0].Equals("LengthRange:"))
                        {
                           int iLengthMin = int.Parse(strParsed[1]);
                           int iLengthMax = int.Parse(strParsed[2]);

                           var peptideLengthRange = new IntRangeWrapper(iLengthMin, iLengthMax);
                           string peptideLengthRangeString = dPeptideMassLow.ToString() + " " + dPeptideMassHigh.ToString();
                           SearchMgr.SetParam("peptide_length_range", peptideLengthRangeString, peptideLengthRange);

                           bFoundMassRange = true;
                        }

                        iLineCount++;

                        if (strParsed[0].Equals("VariableMod:"))
                           break;
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
                  // .idx file does not exist so set appropriate parameters here for generating fragment ion indexing's plain peptide .idx

                  // digest mass range
                  dPeptideMassLow = 800.0;
                  dPeptideMassHigh = 4000.0;
                  var digestMassRange = new DoubleRangeWrapper(dPeptideMassLow, dPeptideMassHigh);
                  string digestMassRangeString = dPeptideMassLow.ToString() + " " + dPeptideMassHigh.ToString();
                  SearchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange);

                  // digest length range
                  int iLengthMin = 8;
                  int iLengthMax = 40;
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

                  // variable mods
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

                  // static mods
                  dTmp = 57.021464;
                  sTmp = dTmp.ToString();
                  SearchMgr.SetParam("add_C_cysteine", sTmp, dTmp);

                  // enzyme settings
                  var enzymeInfo = new EnzymeInfoWrapper();
                  string enzymeInfoString = "0. Cut_everywhere  0 0 0 \n1. Trypsin 1 KR P";
                  enzymeInfo.set_SearchEnzymeName("Trypsin");
                  enzymeInfo.set_SearchEnzymeBreakAA("KR");
                  enzymeInfo.set_SearchEnzymeNoBreakAA("P");
                  enzymeInfo.set_SearchEnzymeOffSet(1); // 0=cleave before; 1=cleave after
                  enzymeInfo.set_SearchEnzyme2Name("Cut_everywhere");
                  enzymeInfo.set_SearchEnzyme2BreakAA("-");
                  enzymeInfo.set_SearchEnzyme2NoBreakAA("-");
                  enzymeInfo.set_SearchEnzyme2OffSet(0); // 0=cleave before; 1=cleave after
                  SearchMgr.SetParam("[COMET_ENZYME_INFO]", enzymeInfoString, enzymeInfo);

                  iTmp = 3;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("allowed_missed_cleavage", sTmp, iTmp);

                  // FI setting
                  iTmp = 3;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("fragindex_min_ions_score", sTmp, iTmp);
                  iTmp = 3;
                  sTmp = iTmp.ToString();
                  SearchMgr.SetParam("fragindex_min_ions_report", sTmp, iTmp);
                  iTmp = 100;
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
               }
            }

            return true;
         }
      }
   }
}