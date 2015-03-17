using System;
using System.Collections.Generic;
using System.Linq;

namespace CometUI
{
    /// <summary>
    /// Given a protein sequence, calculate its pI. This code was adapted 
    /// from TPP's calc_pI.cpp
    /// </summary>
    public class MassSpecUtils
    {
        public enum MassType
        {
            MassTypeAverage = 0,
            MassTypeMonoisotopic
        }

        public MassSpecUtils(bool bMonoMass)
        {
            InitializeMassTables(bMonoMass);
        }

        public static Dictionary<char, double> ElementMassTable { get; private set; }
        public static Dictionary<char, double> AminoAcidMassTable { get; private set; }
        public static Dictionary<String, double> CommonCompoundsMassTable { get; private set; }

        private const double ProtonMass = 1.00727646688;

        private static void InitializeMassTables(bool bMonoMass)
        {
            ElementMassTable = new Dictionary<char, double>();
            AminoAcidMassTable = new Dictionary<char, double>();
            CommonCompoundsMassTable = new Dictionary<string, double>();

            // ReSharper disable InconsistentNaming
            double H, O, C, N, P, S;
            // ReSharper restore InconsistentNaming


            // elemental masses from http://www.unimod.org/masses.html
            if (bMonoMass) // monoisotopic masses
            {
                H = 1.007825035;    // hydrogen
                O = 15.99491463;    // oxygen
                C = 12.0000000;     // carbon  
                N = 14.0030740;     // nitrogen
                P = 30.973762;      // phosphorus
                S = 31.9720707;     // sulphur
            }
            else  // average masses
            {
                H = 1.00794;        // hydrogen
                O = 15.9994;        // oxygen
                C = 12.0107;        // carbon
                N = 14.0067;        // nitrogen
                P = 30.973761;      // phosporus
                S = 32.065;         // sulphur
            }

            CommonCompoundsMassTable.Add("OH2", H * 2 + O);
            CommonCompoundsMassTable.Add("NH", N + H);
            CommonCompoundsMassTable.Add("NH3", N + H * 3);
            CommonCompoundsMassTable.Add("CO", C + O);

            ElementMassTable.Add('h', H);
            ElementMassTable.Add('o', O);
            ElementMassTable.Add('c', C);
            ElementMassTable.Add('n', N);
            ElementMassTable.Add('p', P);
            ElementMassTable.Add('s', S);

            AminoAcidMassTable.Add('G', C * 2 + H * 3 + N + O);
            AminoAcidMassTable.Add('A', C * 3 + H * 5 + N + O);
            AminoAcidMassTable.Add('S', C * 3 + H * 5 + N + O * 2);
            AminoAcidMassTable.Add('P', C * 5 + H * 7 + N + O);
            AminoAcidMassTable.Add('V', C * 5 + H * 9 + N + O);
            AminoAcidMassTable.Add('T', C * 4 + H * 7 + N + O * 2);
            AminoAcidMassTable.Add('C', C * 3 + H * 5 + N + O + S);
            AminoAcidMassTable.Add('L', C * 6 + H * 11 + N + O);
            AminoAcidMassTable.Add('I', C * 6 + H * 11 + N + O);
            AminoAcidMassTable.Add('N', C * 4 + H * 6 + N * 2 + O * 2);
            AminoAcidMassTable.Add('D', C * 4 + H * 5 + N + O * 3);
            AminoAcidMassTable.Add('Q', C * 5 + H * 8 + N * 2 + O * 2);
            AminoAcidMassTable.Add('K', C * 6 + H * 12 + N * 2 + O);
            AminoAcidMassTable.Add('E', C * 5 + H * 7 + N + O * 3);
            AminoAcidMassTable.Add('M', C * 5 + H * 9 + N + O + S);
            AminoAcidMassTable.Add('H', C * 6 + H * 7 + N * 3 + O);
            AminoAcidMassTable.Add('F', C * 9 + H * 9 + N + O);
            AminoAcidMassTable.Add('R', C * 6 + H * 12 + N * 4 + O);
            AminoAcidMassTable.Add('Y', C * 9 + H * 9 + N + O * 2);
            AminoAcidMassTable.Add('W', C * 11 + H * 10 + N * 2 + O);

            AminoAcidMassTable.Add('B', 0.0);
            AminoAcidMassTable.Add('J', 0.0);
            AminoAcidMassTable.Add('U', 0.0);
            AminoAcidMassTable.Add('X', 0.0);
            AminoAcidMassTable.Add('Z', 0.0);
        }

        private static double Exp10(double value)
        {
            return Math.Pow(10.0, value);
        }

        public static double CalculatePI(String aaSequence, int chargeIncrement = 0)
        {
            const int pHMin = 0;                // Minimum pH value
            const int pHMax = 14;               // Maximum pH value
            const int maxIterations = 2000;     // Max number of iterations
            const double epsi = 0.0001;         // Desired precision


            // The 7 amino acids which matter
            // ReSharper disable InconsistentNaming
            const int R = 'R' - 'A';
            const int H = 'H' - 'A';
            const int K = 'K' - 'A';
            const int D = 'D' - 'A';
            const int E = 'E' - 'A';
            const int C = 'C' - 'A';
            const int Y = 'Y' - 'A';
            // ReSharper restore InconsistentNaming

        /*
         *  table of pk values : 
         *  Note: the current algorithm does not use the last two columns. Each 
         *  row corresponds to an amino acid starting with Ala. J, O and U are 
         *  inexistant, but here only in order to have the complete alphabet.
         *
         *          Ct    Nt   Sm     Sc     Sn
         */

         var pK = new[,]
                        {
                            {3.55, 7.59, 0.0, 0.0, 0.0},         // A
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // B
                            {3.55, 7.50, 9.00, 9.00, 9.00},      // C
                            {4.55, 7.50, 4.05, 4.05, 4.05},      // D
                            {4.75, 7.70, 4.45, 4.45, 4.45},      // E
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // F
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // G
                            {3.55, 7.50, 5.98, 5.98, 5.98},      // H
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // I
                            {0.00, 0.00, 0.0, 0.0, 0.0},         // J
                            {3.55, 7.50, 10.00, 10.00, 10.00},   // K
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // L
                            {3.55, 7.00, 0.0, 0.0, 0.0},         // M
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // N
                            {0.00, 0.00, 0.0, 0.0, 0.0},         // O
                            {3.55, 8.36, 0.0, 0.0, 0.0},         // P
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // Q
                            {3.55, 7.50, 12.0, 12.0, 12.0},      // R 
                            {3.55, 6.93, 0.0, 0.0, 0.0},         // S
                            {3.55, 6.82, 0.0, 0.0, 0.0},         // T
                            {0.00, 0.00, 0.0, 0.0, 0.0},         // U
                            {3.55, 7.44, 0.0, 0.0, 0.0},         // V
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // W
                            {3.55, 7.50, 0.0, 0.0, 0.0},         // X
                            {3.55, 7.50, 10.00, 10.00, 10.00},   // Y
                            {3.55, 7.50, 0.0, 0.0, 0.0}          // Z
                        };

            var seq = aaSequence.ToCharArray();

            // Compute the amino acid composition of the protein
            var aaComposition = new int[26];
            var seqLen = seq.Count();
            for (int i = 0; i < seqLen; i++)
            {
                aaComposition[seq[i] - 'A']++;
            }

            // Look up the N-terminal residue
            int ntermRes = seq[0] - 'A';

            // Look up the C-terminal residue
            int ctermRes = seq[seqLen - 1] - 'A';


            double phMin = pHMin;
            double phMax = pHMax;

            double phMid = 0.0;
            for (int i = 0; (i < maxIterations) && ((phMax - phMin) > epsi); i++)
            {
                phMid = phMin + (phMax - phMin) / 2.0;

                double cter = Exp10(-pK[ctermRes, 0]) / (Exp10(-pK[ctermRes, 0]) + Exp10(-phMid));
                double nter = Exp10(-phMid) / (Exp10(-pK[ntermRes, 1]) + Exp10(-phMid));

                double carg = aaComposition[R] * Exp10(-phMid) / (Exp10(-pK[R, 2]) + Exp10(-phMid));
                double chis = aaComposition[H] * Exp10(-phMid) / (Exp10(-pK[H, 2]) + Exp10(-phMid));
                double clys = aaComposition[K] * Exp10(-phMid) / (Exp10(-pK[K, 2]) + Exp10(-phMid));

                double casp = aaComposition[D] * Exp10(-pK[D,2]) / (Exp10(-pK[D,2]) + Exp10(-phMid));
                double cglu = aaComposition[E] * Exp10(-pK[E,2]) / (Exp10(-pK[E,2]) + Exp10(-phMid));

                double ccys = aaComposition[C] * Exp10(-pK[C,2]) / (Exp10(-pK[C,2]) + Exp10(-phMid));
                double ctyr = aaComposition[Y] * Exp10(-pK[Y,2]) / (Exp10(-pK[Y,2]) + Exp10(-phMid));

                double charge = carg + clys + chis + nter + chargeIncrement - (casp + cglu + ctyr + ccys + cter);
                if (charge > 0.0)
                {
                    phMin = phMid;
                }
                else
                {
                    phMax = phMid;
                }
            }

            phMid = Math.Round(phMid, 4);

            return phMid;
        }

        public static double CalculateMzRatio(double calcNeutralMass, int assumedCharge)
        {
            double mzRatio = (calcNeutralMass + (assumedCharge * ProtonMass)) / assumedCharge;
            return mzRatio;
        }

        public static double CalculateMassDiff(double calcNeutralMass, double expMass)
        {
            var massDiff = calcNeutralMass - expMass;
            return massDiff;
        }

        public static double CalculateMassErrorPPM(double calcNeutralMass, double expMass)
        {
            var ppm = ((calcNeutralMass - expMass) / calcNeutralMass) * Math.Pow(10, 6);
            return ppm;
        }
    }
}
