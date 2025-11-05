namespace RealTimeSearch
{
   using System;
   using System.Collections.Generic;
   using System.Diagnostics;
   using System.IO;
   using System.Linq;
   using System.Text;
   //using System.Threading.Tasks;

   using CometWrapper;

   using ThermoFisher.CommonCore.Data.Business;
   using ThermoFisher.CommonCore.Data.FilterEnums;
   using ThermoFisher.CommonCore.Data.Interfaces;
   using ThermoFisher.CommonCore.RawFileReader;
   
   /// <summary>
   /// Call CometWrapper to run searches, looping through scans in a Thermo RAW file
   /// </summary>
   class RealTimeSearch
   {
      static void Main(string[] args)
      {
         if (args.Length < 2)
         {
            Console.WriteLine(" RTS MS1/MS2\n");
            Console.WriteLine("    USAGE:  {0} [query.raw] [MS1reference.raw] [database.idx]\n",
               System.AppDomain.CurrentDomain.FriendlyName);
            return;
         }

         Console.WriteLine("\n RTS MS1/MS2\n");

         CometSearchManagerWrapper SearchMgr = new CometSearchManagerWrapper();
         SearchSettings searchParams = new SearchSettings();

         string rawFileName = args[0];       // raw file that will supply the query spectra
         string sRawFileReference = args[1]; // raw file containing MS1 scans to search for MS1 alignment
         string sDB = "tmp";

         bool bDatabaseSearch = false;

         if (args.Length == 3)
         {
            sDB = args[2];
            bDatabaseSearch = true;
         }

         double dPeptideMassLow = 0;
         double dPeptideMassHigh = 0;

         // ConfigureInputSettings is an example of how to set search parameters.
         // Will also read the index database and return dPeptideMassLow/dPeptideMassHigh mass range
         searchParams.ConfigureInputSettings(SearchMgr, ref sRawFileReference, ref sDB, ref dPeptideMassLow, ref dPeptideMassHigh, bDatabaseSearch);

         if (File.Exists(rawFileName) && File.Exists(sRawFileReference))
         {
            Console.Write(" query file: {0}  \n", rawFileName);
            Console.Write(" MS1 reference file: {0}  \n", sRawFileReference);
            if (bDatabaseSearch)
               Console.Write(" Indexed database: {0}  \n", sDB);
            Console.Write("\n");

            try
            {
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

               // Loop through to find first MS1 scan; get mass range from that
               // Current assumption is that the MS1 mass range is fixed throughout
               // the acquisition

               for (int iScanNumber = iFirstScan; iScanNumber <= iLastScan; ++iScanNumber)
               {
                  // Get the scan filter for this scan number
                  var scanFilter = rawFile.GetFilterForScanNumber(iScanNumber);

                  if (scanFilter.MSOrder == MSOrderType.Ms)
                  {
                     var stats1 = rawFile.GetScanStatsForScanNumber(1);

                     var MS1MassRange = new DoubleRangeWrapper(stats1.LowMass, stats1.HighMass);
                     string MS1MassRangeString = stats1.LowMass.ToString() + " " + stats1.HighMass.ToString();
                     SearchMgr.SetParam("ms1_mass_range", MS1MassRangeString, MS1MassRange);

                     break;
                  }
               }

               int iNumPeaks;
               int iPrecursorCharge;
               double dPrecursorMZ = 0;
               double[] pdMass;
               double[] pdInten;

               Stopwatch watch = new Stopwatch();
               Stopwatch watchGlobal = new Stopwatch();
               TimeSpan elapsedGlobal;

               int iMaxElapsedTime = 50;
               int[] piTimeSearchMS1 = new int[iMaxElapsedTime];  // histogram of search times
               int[] piTimeSearchMS2 = new int[iMaxElapsedTime];  // histogram of search times

               for (int i = 0; i < iMaxElapsedTime; ++i)
               {
                  piTimeSearchMS1[i] = 0;
                  piTimeSearchMS2[i] = 0;
               } 


               int iPass = 1;  // count number of passes/loops through raw file
               int iTime;


               double dMaxMS1RTDiff = 100.0;    // maximum allowed retention time difference between query and reference, in seconds
                                                // set to 0.0 to not apply aka do not apply any RT restrictions


               double dMaxQueryRT;  // this is the maximum RT in seconds for the query run, used to
                                    // scale the query RTs against the reference run RTs which may
                                    // have a different maximum RT value. Assumes a linear gradient.
               dMaxQueryRT = 60.0 * rawFile.RetentionTimeFromScanNumber(iLastScan);

               int iPrintEveryScan = 500;
               int iMS2TopN = 1; // report up to topN hits per MS/MS query
               bool bContinuousLoop = true; // set to true to continuously loop through the raw file
               bool bPrintHistogram = false;
               bool bPrintMatchedFragmentIons = false;
               bool bPerformMS1Search = false;
               bool bPerformMS2Search = true;

               if (bPerformMS1Search)
               {
                  SearchMgr.InitializeSingleSpectrumMS1Search();
               }

               if (bDatabaseSearch && bPerformMS2Search)
               {
                  // trigger loading the .idx database
                  SearchMgr.InitializeSingleSpectrumSearch();
               }
/*
               iFirstScan = 37174;
*/
               iLastScan = 20000;

               watchGlobal.Start();

               for (int iScanNumber = iFirstScan; iScanNumber <= iLastScan; ++iScanNumber)
               {
                  var scanStatistics = rawFile.GetScanStatsForScanNumber(iScanNumber);
                  double dRT = 60.0 * rawFile.RetentionTimeFromScanNumber(iScanNumber);

                  // Get the scan filter for this scan number
                  var scanFilter = rawFile.GetFilterForScanNumber(iScanNumber);

                  if (scanFilter.MSOrder == MSOrderType.Ms || scanFilter.MSOrder == MSOrderType.Ms2)
                  {
                     // Check to see if the scan has centroid data or profile data.  Depending upon the
                     // type of data, different methods will be used to read the data.
                     if (scanStatistics.IsCentroidScan && (scanStatistics.SpectrumPacketType == SpectrumPacketType.FtCentroid))
                     {
                        // Get the centroid (label) data from the RAW file for this scan
                        var centroidStream = rawFile.GetCentroidStream(iScanNumber, false);
                        iNumPeaks = centroidStream.Length;
                        pdMass = new double[iNumPeaks];   // stores mass of spectral peaks
                        pdInten = new double[iNumPeaks];  // stores inten of spectral peaks
                        pdMass = centroidStream.Masses;
                        pdInten = centroidStream.Intensities;
                     }
                     else
                     {
                        // Get the segmented (low res and profile) scan data
                        var segmentedScan = rawFile.GetSegmentedScanFromScanNumber(iScanNumber, scanStatistics);
                        iNumPeaks = segmentedScan.Positions.Length;
                        pdMass = new double[iNumPeaks];   // stores mass of spectral peaks
                        pdInten = new double[iNumPeaks];  // stores inten of spectral peaks
                        pdMass = segmentedScan.Positions;
                        pdInten = segmentedScan.Intensities;
                     }

                     if (iNumPeaks >= 10)  // don't bother searching sparse spectra
                     {
                        // now run the search on scan

                        int iMS1TopN = 1; // report up to iMS1TopN hits per query; unused right now as only top matching MS1 scan is returned

                        if (bPerformMS1Search && scanFilter.MSOrder == MSOrderType.Ms)
                        {
                           watch.Reset();
                           watch.Start();
                           SearchMgr.DoMS1SearchMultiResults(dMaxMS1RTDiff, dMaxQueryRT, iMS1TopN, dRT, pdMass, pdInten, iNumPeaks, out List<ScoreWrapperMS1> vScores);
                           watch.Stop();

                           if (vScores.Count > 0)
                           {
                              iTime = (int)watch.ElapsedMilliseconds;
                              if (iTime >= iMaxElapsedTime)
                                 iTime = iMaxElapsedTime - 1;
                              if (iTime >= 0)
                                 piTimeSearchMS1[iTime] += 1;

                              if ((iScanNumber % iPrintEveryScan) == 0)
                              {
                                 for (int x = 0; x < 1; ++x)
                                 {
                                    Console.WriteLine("*MS1 {0}  libscan {1}  queryRT {2:F2}  libRT {3:F2}  dotp {4:F3}  {5} ms",
                                       iScanNumber, vScores[x].iScanNumber, dRT, vScores[x].fRTime, vScores[x].fDotProduct, iTime);
                                 }
                              }
                           }
                        }
                        else if (bPerformMS2Search && scanFilter.MSOrder == MSOrderType.Ms2)  // MS2 scan
                        {
                           iPrecursorCharge = 0;
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

                           // skip analysis of spectrum if ion is outside of indexed db mass range
                           double dExpPepMass = (iPrecursorCharge * dPrecursorMZ) - (iPrecursorCharge - 1) * 1.00727646688;

                           if (dExpPepMass < dPeptideMassLow || dExpPepMass > dPeptideMassHigh)
                              continue;

                           // now run the search on scan

                           // these next variables store return value from search
                           List<string> vPeptide = new List<string>();
                           List<string> vProtein = new List<string>();

                           watch.Reset();
                           watch.Start();
                           SearchMgr.DoSingleSpectrumSearchMultiResults(iMS2TopN, iPrecursorCharge, dPrecursorMZ, pdMass, pdInten, iNumPeaks,
                              out vPeptide, out vProtein, out List<List<FragmentWrapper>> vMatchingFragments, out List<ScoreWrapper> vScores);
                           watch.Stop();

                           int iProteinLengthCutoff = 30;

                           if (vPeptide.Count > 0 && (iScanNumber % iPrintEveryScan) == 0)
                           {
                              if (vPeptide[0].Length > 0)
                              {
                                 for (int x = 0; x < 1; ++x)
                                 {
                                    if (vPeptide[x].Length > 0)
                                    {
                                       string protein = vProtein[x];
                                       if (protein.Length > iProteinLengthCutoff)
                                          protein = protein.Substring(0, iProteinLengthCutoff);  // trim to avoid printing long protein description string

                                       Console.WriteLine(" MS2 {0}\t{1}  {2:F4}  {3:0.##E+00}  {4:F4}  AScore {5:F2}  Sites '{6}'  {7} ms", 
                                          iScanNumber, vPeptide[x], vScores[x].xCorr, vScores[x].dExpect, dExpPepMass,
                                          vScores[x].dAScoreScore, vScores[x].sAScoreProSiteScores,
                                          watch.ElapsedMilliseconds);

/*
                                       if (vScores[x].dAScoreScore >= 13.0 && vScores[x].xCorr > 2.0
                                          && (vScores[x].sAScoreProSiteScores.Contains(":0.00") || vScores[x].sAScoreProSiteScores.Contains(":-")))
                                       {
                                          double dMZ = (dExpPepMass - 1.00727646688) / iPrecursorCharge;

                                          Console.WriteLine(" MS2 {0}\t{0}\t{1}\t{2}\t{3:F4}\t{4:0.##E+00}\t{5:F4}\tAScore {6:F2}\tSites '{7}'\t{8} ms",
                                            iScanNumber, vPeptide[x], dMZ, vScores[x].xCorr, vScores[x].dExpect, dExpPepMass,
                                            vScores[x].dAScoreScore, vScores[x].sAScoreProSiteScores,
                                            watch.ElapsedMilliseconds);
                                       }
*/
                                       if (bPrintMatchedFragmentIons)
                                       {
                                          foreach (var myFragment in vMatchingFragments[x]) // print matched fragment ions
                                          {
                                             Console.WriteLine("\t{0:0.0000}\t{1:0.0}\t{2}+\t{3}-ion",
                                                myFragment.Mass,
                                                myFragment.Intensity,
                                                myFragment.Charge,
                                                myFragment.Type);
                                          }
                                       }

                                    }
                                 }
//                                 Console.WriteLine("");
                              }
                           }

                           if (vPeptide.Count > 0)
                           {
                              iTime = (int)watch.ElapsedMilliseconds;
                              if (iTime >= iMaxElapsedTime)
                                 iTime = iMaxElapsedTime - 1;
                              if (iTime >= 0)
                                 piTimeSearchMS2[iTime] += 1;
                           }

                        }
                     }
                  }

                  if (bContinuousLoop && iScanNumber == iLastScan)
                  {
                     iScanNumber = 0;
                     elapsedGlobal = watchGlobal.Elapsed;
                     Console.WriteLine("pass {0}, {1:F2} min", iPass, elapsedGlobal.TotalMinutes);
                     iPass++;
                  }

               }

               SearchMgr.FinalizeSingleSpectrumSearch();

               if (bPrintHistogram)
               {
                  // write out histogram of spectrum search times
                  for (int i = 0; i < iMaxElapsedTime; ++i)
                     Console.WriteLine("histogram\t{0}\t{1}\t{2}", i, piTimeSearchMS1[i], piTimeSearchMS2[i]);
               }

               rawFile.Dispose();
            }

            catch (Exception rawSearchEx)
            {
               Console.WriteLine(" Error: " + rawSearchEx.Message);
            }
         }
         else
         {
            Console.WriteLine("No raw file exists at that path.");
         }


         Console.WriteLine("{0} Done.{1}", Environment.NewLine, Environment.NewLine);
         //Console.ReadLine();
         return;
      }

      class SearchSettings
      {
         public bool ConfigureInputSettings(CometSearchManagerWrapper SearchMgr,
            ref string sRawFileReference,
            ref string sDB,
            ref double dPeptideMassLow,
            ref double dPeptideMassHigh,
            bool bDatabaseSearch)
         {  
            String sTmp;
            int iTmp;
            double dTmp;
//            DoubleRangeWrapper doubleRangeParam = new DoubleRangeWrapper();
//            IntRangeWrapper intRangeParam = new IntRangeWrapper();

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

            iTmp = 0;
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("num_threads", sTmp, iTmp);

            dTmp = 0.02; // fragment bin width
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("fragment_bin_tol", sTmp, dTmp);

            dTmp = 0.0;  // fragment bin offst
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

            iTmp = 100000; // search time cutoff in milliseconds
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
                  dPeptideMassLow =   800.0;
                  dPeptideMassHigh = 4000.0;
                  var digestMassRange = new DoubleRangeWrapper(dPeptideMassLow, dPeptideMassHigh);
                  string digestMassRangeString = dPeptideMassLow.ToString() + " " + dPeptideMassHigh.ToString();
                  SearchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange);

                  // digest length range
                  int iLengthMin =  8;
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
