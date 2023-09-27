﻿namespace RealTimeSearch
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
            Console.WriteLine("{1} RealTimeSearch:  enter raw file and index db on command line\n");
            return;
         }

         Console.WriteLine("\n RealTimeSearch\n");

         CometSearchManagerWrapper SearchMgr = new CometSearchManagerWrapper();
         SearchSettings searchParams = new SearchSettings();
                 
         string rawFileName = args[0];
         string sDB = args[1];
         double  dPeptideMassLow = 0;
         double  dPeptideMassHigh = 0;

         // Configure search parameters here
         // Will also read the index database and return dPeptideMassLow/dPeptideMassHigh mass range
         searchParams.ConfigureInputSettings(SearchMgr, ref dPeptideMassLow, ref dPeptideMassHigh, ref sDB);

         if (File.Exists(rawFileName) && File.Exists(sDB))
         {
            Console.Write(" input: {0}  \n", rawFileName);

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

               int iNumPeaks;
               int iPrecursorCharge;
               double dPrecursorMZ = 0;
               double[] pdMass;
               double[] pdInten;
               Stopwatch watch = new Stopwatch();

               int iMaxElapsedTime = 100;
               int[] piTimeSearch = new int[iMaxElapsedTime];  // histogram of search times
               for (int i = 0; i < iMaxElapsedTime; ++i)
               {
                  piTimeSearch[i] = 0;
               }
               int iPass = 1;  // count number of passes/loops through raw file

               int iTime = 0;

               SearchMgr.InitializeSingleSpectrumSearch();

               for (int iScanNumber = iFirstScan; iScanNumber <= iLastScan; ++iScanNumber)
               {
                  var scanStatistics = rawFile.GetScanStatsForScanNumber(iScanNumber);
                  //double dRT = rawFile.RetentionTimeFromScanNumber(iScanNumber);

                  // Get the scan filter for this scan number
                  var scanFilter = rawFile.GetFilterForScanNumber(iScanNumber);

                  if (scanFilter.MSOrder == MSOrderType.Ms2)
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

                     if (iNumPeaks > 0)
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
                        ScoreWrapper score;
                        List<FragmentWrapper> matchingFragments;
                        string peptide;
                        string protein;

                        watch.Start();
                        SearchMgr.DoSingleSpectrumSearch(iPrecursorCharge, dPrecursorMZ, pdMass, pdInten, iNumPeaks,
                           out peptide, out protein, out matchingFragments, out score);
                        watch.Stop();

                        double xcorr = score.xCorr;
                        double dExpect = score.dExpect;
                        double dSp = score.dSp;
                        double dCn = score.dCn;
                        int iIonsMatch = score.MatchedIons;
                        int iIonsTotal = score.TotalIons;

                        double dPepMass = (dPrecursorMZ * iPrecursorCharge) - (iPrecursorCharge - 1) * 1.00727646688;

                        // do not decode peptide/proteins strings unless xcorr>0
                        if (xcorr > 0)
                        {
                           if ((iScanNumber % 1000) == 0)
                           {
                              if (protein.Length > 20)
                                 protein = protein.Substring(0, 20);  // trim to avoid printing long protein description string

                              Console.WriteLine("{0}\t{1}\t{2}\t{3:0.0000}\t{10:0.0000}\t{9:0.0}\t{4:E4}\t{5:0.0000}\t{6}\t{7}\t{8}",
                                 iScanNumber, peptide, protein, xcorr, dExpect, dPepMass, iIonsMatch, iIonsTotal, iPass, dSp, dCn);

/*
                              Console.WriteLine("pass {12}\t{0}\t{2}\t{3:0.0000}\t{9:0.0000}\t{5}\t{6}\t{7:0.0000}\t{8}\t{10}/{11}",
                                 iScanNumber, iLastScan, iPrecursorCharge, dPrecursorMZ, iNumPeaks, peptide, protein,
                                 xcorr, watch.ElapsedMilliseconds, dPepMass, iIonsMatch, iIonsTotal, iPass);

                              foreach (var myFragment in matchingFragments)
                              {
                                 Console.WriteLine("{0:0000.0000} {1:0.0} {2} {3}",
                                    myFragment.Mass,
                                    myFragment.Intensity,
                                    myFragment.Charge,
                                    myFragment.Type);
                              }
*/
                           }
                        }

                        iTime = (int)watch.ElapsedMilliseconds;
                        if (iTime >= iMaxElapsedTime)
                           iTime = iMaxElapsedTime-1;
                        if (iTime >= 0)
                           piTimeSearch[iTime] += 1;

                        watch.Reset();
                     }
                  }
/*
                  if (iScanNumber == iLastScan)
                  {
                     iScanNumber = 0;
                     Console.WriteLine("pass {0}", iPass);
                     iPass++;
                  }
*/
               }

               SearchMgr.FinalizeSingleSpectrumSearch();

               // write out histogram of spectrum search times
               for (int i = 0; i < iMaxElapsedTime; ++i)
                  Console.WriteLine("{0}\t{1}", i, piTimeSearch[i]);

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
            ref double dPeptideMassLow,
            ref double dPeptideMassHigh,
            ref string sDB)
         {  
            String sTmp;
            int iTmp;
            double dTmp;
            DoubleRangeWrapper doubleRangeParam = new DoubleRangeWrapper();

            SearchMgr.SetParam("database_name", sDB, sDB);

            sTmp = "DECOY_";
            SearchMgr.SetParam("decoy_prefix", sTmp, sTmp);

            iTmp = 0; // 0=no internal decoys, 1=concatenated target/decoy
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("decoy_search", sTmp, iTmp);

            doubleRangeParam.set_dStart(-20.0);
            doubleRangeParam.set_dEnd(20.0);
            sTmp = "-20.0 20.0";
            SearchMgr.SetParam("peptide_mass_tolerance", sTmp, doubleRangeParam);

            iTmp = 2; // 0=Da, 2=ppm
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

            iTmp = 100; // search time cutoff in milliseconds
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_index_runtime", sTmp, iTmp);

            iTmp = 1; // m/z tolerance
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("precursor_tolerance_type", sTmp, iTmp);

            iTmp = 2; // 0=off, 1=0/1 (C13 error), 2=0/1/2, 3=0/1/2/3, 4=-8/-4/0/4/8 (for +4/+8 labeling)
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("isotope_error", sTmp, iTmp);

            dTmp = 1.0005; // fragment bin width
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("fragment_bin_tol", sTmp, dTmp);

            dTmp = 0.4; // fragment bin offset
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("fragment_bin_offset", sTmp, dTmp);

            iTmp = 1; // 0=use flanking peaks, 1=M peak only
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("theoretical_fragment_ions", sTmp, iTmp);

            iTmp = 3; // maximum fragment charge
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_fragment_charge", sTmp, iTmp);

            iTmp = 6; // maximum precursor charge
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("max_precursor_charge", sTmp, iTmp);

            iTmp = 0; // 0=I and L are different, 1=I and L are same
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("equal_I_and_L", sTmp, iTmp);
/*
            // these are read from the indexed database but this is an example
            // on how to set them here;
            VarModsWrapper varMods = new VarModsWrapper();
            sTmp = "15.9949 M 0 3 -1 0 0";
            varMods.set_VarModMass(15.9949);
            varMods.set_VarModChar("M");
            SearchMgr.SetParam("variable_mod01", sTmp, varMods);

            sTmp = "79.9663 STY 0 3 -1 0 0";
            varMods.set_VarModMass(79.9663);
            varMods.set_VarModChar("STY");
            SearchMgr.SetParam("variable_mod02", sTmp, varMods);
*/
            // Now actually open the .idx database to read mass range from it
            int iLineCount = 0;
            bool bFoundMassRange = false;
            string strLine;
            System.IO.StreamReader dbFile = new System.IO.StreamReader(@sDB);

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
               iLineCount++;

               if (iLineCount > 6)  // header information should only be in first few lines
                  break;
            }
            dbFile.Close();

            if (!bFoundMassRange)
            {
               Console.WriteLine(" Error with indexed database format; missing MassRange header.\n");
               System.Environment.Exit(1);
            }

            return true;
         }
      }
   }
}
