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
         string sDB = "C:\\Users\\Jimmy\\Desktop\\devin\\HUMAN.fasta.idx";
         SearchMgr.SetParam("database_name", sDB, sDB);

         iTmp = 0; // 0=no, 1=concatenated, 2=separate
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("decoy_search", sTmp, iTmp);

         dTmp = 20.0; //ppm window
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("peptide_mass_tolerance", sDB, dTmp);

         iTmp = 2; // 0=Da, 2=ppm
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

         iTmp = 1; // 1=monoisotopic, do not change
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("mass_type_parent", sTmp, iTmp);

         iTmp = 1; // 1=monoisotopic, do not change
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("mass_type_fragment", sTmp, iTmp);

         iTmp = 1; // m/z tolerance
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("precursor_tolerance_type", sTmp, iTmp);

         iTmp = 3; // 0=off, 1=0/1 (C13 error), 2=0/1/2, 3=0/1/2/3, 4=-8/-4/0/4/8 (for +4/+8 labeling)
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("isotope_error", sTmp, iTmp);

         iTmp = 0; // 0=no, 1=yes
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("require_variable_mod", sTmp, iTmp);

         dTmp = 1.0005; // fragment bin width
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("fragment_bin_tol", sTmp, dTmp);

         dTmp = 0.4; // fragment bin offset
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("fragment_bin_offset", sTmp, dTmp);

         iTmp = 1; // 0=use flanking peaks, 1=M peak only
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("theoretical_fragment_ions", sTmp, iTmp);

         iTmp = 3;
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("max_fragment_charge", sTmp, iTmp);

         iTmp = 6;
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("max_precursor_charge", sTmp, iTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Cterm_peptide", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Nterm_peptide", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Cterm_protein", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Nterm_protein", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_G_glucine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_A_alanine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_S_serine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_P_proline", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_V_valine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_T_threonine", sTmp, dTmp);

         dTmp = 57.0214637236;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_C_cysteine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_L_leucine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_I_isoleucine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_N_asparagine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_D_aspartic_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Q_glutamine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_K_lysine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_E_glutamic_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_M_methionine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_O_ornithine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_H_histidine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_F_phenylalanine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_U_selenocysteine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_R_arginine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Y_tyrosine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_W_tryptophan", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Y_tyrosine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_B_user_amino_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_J_user_amino_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_X_user_amino_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Z_user_amino_acid", sTmp, dTmp);

         var digestMassMin = 100.0;
         var digestMassMax = 10000.0;
         var digestMassRange = new DoubleRangeWrapper(digestMassMin, digestMassMax);
         string digestMassRangeString = digestMassMin.ToString() + " " + digestMassMax.ToString();
         SearchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange);

         string modString = "ST,79.9663,0,3,-1,0,0";
         var varModsWrapper = new VarModsWrapper();
         varModsWrapper.set_VarModChar("ST");
         varModsWrapper.set_VarModMass(79.9663);
         varModsWrapper.set_BinaryMod(0);
         varModsWrapper.set_MaxNumVarModAAPerMod(3);  // allow up to 3 of these mods in peptide
         varModsWrapper.set_VarModTermDistance(-1);   // allow mod to be anywhere in peptide
         varModsWrapper.set_WhichTerm(0);             // unused if -1 set above; 0=protein terminus reference
         varModsWrapper.set_RequireThisMod(0);        // 0=not required, 1=required
         SearchMgr.SetParam("variable_mod01", modString, varModsWrapper);   

         int iPrecursorCharge = 2;
         double dMZ = 604.768827;
         int iNumPeaks = 9;                         // number of peaks in spectrum
         double[] pdMass = new double[iNumPeaks];   // stores mass of spectral peaks
         double[] pdInten = new double[iNumPeaks];  // stores inten of spectral peaks

         // now populate the spectrum as mass and intensity arrays from whatever is retrieved from Thermo interface
         // example below is y-ions from human tryptic peptide EAGAQAVPET[79.9663]R
         pdMass[0] = 356.132; pdInten[0] = 100.0;
         pdMass[1] = 485.176; pdInten[1] = 90.0;
         pdMass[2] = 582.228; pdInten[2] = 120.0;
         pdMass[3] = 627.310; pdInten[3] = 110.0;  // threw in this b-ion
         pdMass[4] = 681.297; pdInten[4] = 300.0;
         pdMass[5] = 880.392; pdInten[5] = 85.0;
         pdMass[6] = 951.430; pdInten[6] = 113.0;
         pdMass[7] = 1008.45; pdInten[7] = 99.0;
         pdMass[8] = 1079.49; pdInten[8] = 95.0;

         // these are the return information from search
         sbyte[] szPeptide = new sbyte[512];
         sbyte[] szProtein = new sbyte[512];
         int iNumFragIons = 10;              // return 10 most intense matched b- and y-ions
         double[] pdYions = new double[iNumFragIons];
         double[] pdBions = new double[iNumFragIons];
         double[] pdScores = new double[5];   // 0=xcorr, 1=calc pep mass, 2=matched ions, 3=tot ions, 4=dCn

         // call Comet search here
         SearchMgr.DoSingleSpectrumSearch(iPrecursorCharge, dMZ, pdMass, pdInten, iNumPeaks,
            szPeptide, szProtein, pdYions, pdBions, iNumFragIons, pdScores);

         string peptide = Encoding.UTF8.GetString(szPeptide.Select(b=>(byte) b).ToArray());
         string protein = Encoding.UTF8.GetString(szProtein.Select(b => (byte)b).ToArray());
         int index = peptide.IndexOf('\0');
         if (index >= 0)
            peptide = peptide.Remove(index);
         index = protein.IndexOf('\0');
         if (index >= 0)
            protein = protein.Remove(index);

         double xcorr = pdScores[0];

         if (xcorr > 0)
         {
            Console.WriteLine("peptide: {0}\nprotein: {1}\nxcorr {2}\npepmass {3}\ndCn {4}\nions {5}/{6}",
               peptide, protein, pdScores[0], pdScores[1], pdScores[4], pdScores[2], (int)pdScores[3], (int)pdScores[4]);

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

   public class IntRange
   {
      public int Start { get; set; }
      public int End { get; set; }

      public IntRange()
      {
         Start = 0;
         End = 0;
      }

      public IntRange(int start, int end)
      {
         Start = start;
         End = end;
      }
   }

   public class DoubleRange
   {
      public double Start { get; set; }
      public double End { get; set; }

      public DoubleRange()
      {
         Start = 0;
         End = 0;
      }

      public DoubleRange(double start, double end)
      {
         Start = start;
         End = end;
      }
   }
}