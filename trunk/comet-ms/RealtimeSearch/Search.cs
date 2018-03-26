using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CometWrapper;

using System.Runtime.InteropServices;

namespace RealtimeSearch
{
   class Search
   {
      static void Main()
      {
         String sTmp;
         int iTmp;
         double dTmp;

         CometSearchManagerWrapper SearchMgr;
         SearchMgr = new CometSearchManagerWrapper();
         // can set other parameters this way
         string sDB = "C:\\Users\\Jimmy\\Desktop\\work\\cometsearch\\human.fasta.idx";
         SearchMgr.SetParam("database_name", sDB, sDB);

         dTmp = 20.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("peptide_mass_tolerance", sDB, dTmp);

         iTmp = 2; // ppm
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

         iTmp = 0; // m/z tolerance
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("precursor_tolerance_type", sTmp, iTmp);

         iTmp = 1; // isotope error 0 to 3
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("isotope_error", sTmp, iTmp);

         dTmp = 1.0005; // fragment bin width
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("fragment_bin_tol", sTmp, dTmp);

         dTmp = 0.4; // fragment bin offset
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("fragment_bin_offset", sTmp, dTmp);

         dTmp = 229.162932;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Nterm_peptide", sTmp, dTmp);

         dTmp = 229.162932;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_K_lysine", sTmp, dTmp);

         dTmp = 57.0214637236;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_C_cysteine", sTmp, dTmp);

         // set variable mod ... note this realtime this search can only set 1 variable mod
         // sadly have to follow this format unless you want to deal with more code
         string modString = "M,15.9949146221,0,3,-1,0,0";
         var varModsWrapper = new VarModsWrapper();
         varModsWrapper.set_VarModChar("M");
         varModsWrapper.set_VarModMass(15.9949146221);
         varModsWrapper.set_BinaryMod(0);
         varModsWrapper.set_MaxNumVarModAAPerMod(3);  // allow up to 3 of these mods in peptide
         varModsWrapper.set_VarModTermDistance(-1);   // allow mod to be anywhere in peptide
         varModsWrapper.set_WhichTerm(0);             // unused if -1 set above; 0=protein terminus reference
         varModsWrapper.set_RequireThisMod(0);        // 0=not required, 1=required
         SearchMgr.SetParam("variable_mod01", modString, varModsWrapper);


         // need precursor charge, precursor m/z and peaklist to do search

         int iPrecursorCharge = 2;
         double dMZ = 785.473206;

         int iNumPeaks = 662;                         // number of peaks in spectrum
         double[] pdMass = new double[iNumPeaks];   // stores mass of spectral peaks
         double[] pdInten = new double[iNumPeaks];  // stores inten of spectral peaks

         iNumPeaks = 0;
         string line;

         // Read the file and display it line by line.  
         System.IO.StreamReader file = new System.IO.StreamReader(@"C:\Users\Jimmy\Desktop\debug\scan.txt");
         while ((line = file.ReadLine()) != null)
         {
            var parts = line.Split(' ');
            double a = Convert.ToDouble(parts[0]);
            double b = Convert.ToDouble(parts[1]);

            pdMass[iNumPeaks] = a;
            pdInten[iNumPeaks] = b;
            iNumPeaks++;
         }

         file.Close();

         if (iNumPeaks != 662)
         {
            Console.WriteLine("Error\n");
            System.Environment.Exit(1);
         }
         
         /*
         int iNumPeaks = 6;                         // number of peaks in spectrum
         double[] pdMass = new double[iNumPeaks];   // stores mass of spectral peaks
         double[] pdInten = new double[iNumPeaks];  // stores inten of spectral peaks
         
         // now populate the spectrum as mass and intensity arrays from whatever is retrieved from Thermo interface
         // example below is y-ions from human tryptic peptide EAGAQAVPETREAGAQAVPET[79.9663]R
         pdMass[0] = 376.275; pdInten[0] = 100.0;
         pdMass[1] = 507.316; pdInten[1] = 90.0;
         pdMass[2] = 606.348; pdInten[2] = 120.0;
         pdMass[3] = 735.427; pdInten[3] = 110.0;
         pdMass[4] = 822.459; pdInten[4] = 300.0;
         pdMass[5] = 921.527; pdInten[5] = 85.0;
         */
          
         // these are the return information from search
         sbyte[] szPeptide = new sbyte[512];
         sbyte[] szProtein = new sbyte[512];
         int iNumFragIons = 10;              // return 10 most intense matched b- and y-ions
         double[] pdYions = new double[iNumFragIons];
         double[] pdBions = new double[iNumFragIons];
         double[] pdScores = new double[2];   // dScores[0] = xcorr, dScores[1] = E-value

         // call Comet search here
         SearchMgr.DoSingleSpectrumSearch(iPrecursorCharge, dMZ, pdMass, pdInten, iNumPeaks, szPeptide, szProtein, pdYions, pdBions, iNumFragIons, pdScores);

         string peptide = Encoding.UTF8.GetString(szPeptide.Select(b=>(byte) b).ToArray());
         string protein = Encoding.UTF8.GetString(szProtein.Select(b => (byte)b).ToArray());
         int index = peptide.IndexOf('\0');
         if (index >= 0)
            peptide = peptide.Remove(index);

         double xcorr = pdScores[0];
         double expect = pdScores[1];

         if (xcorr > 0)
         {
            Console.WriteLine("peptide: {0}\nprotein: {1}\nxcorr {2}\nexpect {3}", peptide, protein, xcorr, expect);

            for (int i = 0; i < iNumFragIons; i++)
            {
               if (pdBions[i] > 0.0)
                  Console.WriteLine("matched b-ion {0}", pdBions[i]);
               else
                  break;
            }
            for (int i = 0; i < iNumFragIons; i++)
            {
               if (pdYions[i] > 0.0)
                  Console.WriteLine("matched y-ion {0}", pdYions[i]);
               else
                  break;
            }
         }
         else
         {
            Console.WriteLine("no match");
         }
      }
   }
}