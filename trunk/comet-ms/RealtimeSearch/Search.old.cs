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

   //using ThermoFisher.CommonCore.Data.Business;
   //using ThermoFisher.CommonCore.Data.FilterEnums;
   //using ThermoFisher.CommonCore.Data.Interfaces;
   //using ThermoFisher.CommonCore.RawFileReader;
   
   /// <summary>
   /// Call CometWrapper to run searches, looping through scans in a Thermo RAW file
   /// </summary>
   class RealTimeSearch
   {
      static void Main(string[] args)
      {
         if (args.Length < 1)
         {
            Console.WriteLine("{1} RealTimeSearch:  enter raw file and index db on command line\n");
            return;
         }

         Console.WriteLine("\n RealTimeSearch\n");

         CometSearchManagerWrapper SearchMgr = new CometSearchManagerWrapper();
         SearchSettings searchParams = new SearchSettings();
                 
//         string rawFileName = args[0];
         string sDB = args[0];
         double  dPeptideMassLow = 0;
         double  dPeptideMassHigh = 0;

         // Configure search parameters here
         // Will also read the index database and return dPeptideMassLow/dPeptideMassHigh mass range
         searchParams.ConfigureInputSettings(SearchMgr, ref dPeptideMassLow, ref dPeptideMassHigh, ref sDB);

         //         if (File.Exists(rawFileName) && File.Exists(sDB))
         if (File.Exists(sDB))
         {
            int iNumPeaks;
            int iPrecursorCharge;
            double dPrecursorMZ = 0;
            double[] pdMass;
            double[] pdInten;
            Stopwatch watch = new Stopwatch();

            int iMaxElapsedTime = 10;
            int[] piElapsedTime = new int[iMaxElapsedTime];  // histogram of search times
            for (int i = 0; i < iMaxElapsedTime; i++)
               piElapsedTime[i] = 0;

            SearchMgr.InitializeSingleSpectrumSearch();

            iNumPeaks = 7;

            pdMass = new double[iNumPeaks];
            pdInten = new double[iNumPeaks];

            //-.MSTLLENIFAIINLFK.Q
            pdMass[0] = 294.181; pdInten[0] = 100;
            pdMass[1] = 407.265; pdInten[1] = 100;
            pdMass[2] = 521.308; pdInten[2] = 100;
            pdMass[3] = 634.392; pdInten[3] = 100;
            pdMass[4] = 747.476; pdInten[4] = 100;
            pdMass[5] = 818.513; pdInten[5] = 100;
            pdMass[6] = 965.582; pdInten[6] = 100;

            if (iNumPeaks > 0)
            {
               iPrecursorCharge = 2;
               dPrecursorMZ = 934.0235;

               // skip analysis of spectrum if ion is outside of indexed db mass range
               double dExpPepMass = (iPrecursorCharge * dPrecursorMZ) - (iPrecursorCharge - 1) * 1.00727646688;

               // now run the search on scan

               // these next variables store return value from search
               ScoreWrapper score;
               List<FragmentWrapper> matchingFragments;
               string peptide;
               string protein;

               SearchMgr.DoSingleSpectrumSearch(iPrecursorCharge, dPrecursorMZ, pdMass, pdInten, iNumPeaks,
                  out peptide, out protein, out matchingFragments, out score);

               double xcorr = score.xCorr;
               int iIonsMatch = score.MatchedIons;
               int iIonsTotal = score.TotalIons;
               double dCn = score.dCn;

               double dPepMass = (dPrecursorMZ * iPrecursorCharge) - (iPrecursorCharge - 1) * 1.00727646688;

               if (protein.Length > 10)
                  protein = protein.Substring(0, 10);  // trim to avoid printing long protein description string

               Console.WriteLine("{0}\t{2}\t{3:0.0000}\t{9:0.0000}\t{5}\t{6}\t{7:0.00}\t{8}\t{10}/{11}",
                  1, 1, iPrecursorCharge, dPrecursorMZ, iNumPeaks, peptide, protein,
                  xcorr, watch.ElapsedMilliseconds, dPepMass, iIonsMatch, iIonsTotal);

               foreach (var myFragment in matchingFragments)
               {
                  Console.WriteLine("{0:0000.0000} {1:0.0} {2} {3}",
                     myFragment.Mass,
                     myFragment.Intensity,
                     myFragment.Charge,
                     myFragment.Type);
               }
            }
            SearchMgr.FinalizeSingleSpectrumSearch();
         }

         Console.WriteLine("{0} Done.{1}", Environment.NewLine, Environment.NewLine);
         Console.ReadKey();
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

            SearchMgr.SetParam("database_name", sDB, sDB);

            dTmp = 20.0; //ppm window
            sTmp = dTmp.ToString();
            SearchMgr.SetParam("peptide_mass_tolerance", sTmp, dTmp);

            iTmp = 2; // 0=Da, 2=ppm
            sTmp = iTmp.ToString();
            SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

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

            int iPeptideLengthMin = 10;
            int iPeptideLengthMax = 15;
            
            var peptideLengthRange = new IntRangeWrapper(iPeptideLengthMin, iPeptideLengthMax);
            string peptideLengthRangeString = iPeptideLengthMin.ToString() + " " + iPeptideLengthMax.ToString();
            SearchMgr.SetParam("digest_mass_range", peptideLengthRangeString, peptideLengthRange);


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
