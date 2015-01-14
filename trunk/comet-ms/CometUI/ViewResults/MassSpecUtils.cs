using System;
using System.Linq;

namespace CometUI.ViewResults
{
    /// <summary>
    /// Given a protein sequence, calculate its pI. This code was adapted 
    /// from TPP's calc_pI.cpp
    /// </summary>
    internal class MassSpecUtils
    {
        private const double ProtonMass = 1.00727646688;

        private const int PHMin = 0;                // Minimum pH value
        private const int PHMax = 14;               // Maximum pH value
        private const int MaxIterations = 2000;     // Max number of iterations
        private const double Epsi = 0.0001;         // Desired precision


        // The 7 amino acids which matter
        private const int R = 'R' - 'A';
        private const int H = 'H' - 'A';
        private const int K = 'K' - 'A';
        private const int D = 'D' - 'A';
        private const int E = 'E' - 'A';
        private const int C = 'C' - 'A';
        private const int Y = 'Y' - 'A';

        /*
         *  table of pk values : 
         *  Note: the current algorithm does not use the last two columns. Each 
         *  row corresponds to an amino acid starting with Ala. J, O and U are 
         *  inexistant, but here only in order to have the complete alphabet.
         *
         *          Ct    Nt   Sm     Sc     Sn
         */

        private static readonly double[,] Pk = new[,]
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

        private static double Exp10(double value)
        {
            return Math.Pow(10.0, value);
        }

        public static double CalculatePI(String aaSequence, int chargeIncrement = 0)
        {
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


            double phMin = PHMin;
            double phMax = PHMax;

            double phMid = 0.0;
            for (int i = 0; (i < MaxIterations) && ((phMax - phMin) > Epsi); i++)
            {
                phMid = phMin + (phMax - phMin) / 2.0;

                double cter = Exp10(-Pk[ctermRes, 0]) / (Exp10(-Pk[ctermRes, 0]) + Exp10(-phMid));
                double nter = Exp10(-phMid) / (Exp10(-Pk[ntermRes, 1]) + Exp10(-phMid));

                double carg = aaComposition[R] * Exp10(-phMid) / (Exp10(-Pk[R, 2]) + Exp10(-phMid));
                double chis = aaComposition[H] * Exp10(-phMid) / (Exp10(-Pk[H, 2]) + Exp10(-phMid));
                double clys = aaComposition[K] * Exp10(-phMid) / (Exp10(-Pk[K, 2]) + Exp10(-phMid));

                double casp = aaComposition[D] * Exp10(-Pk[D,2]) / (Exp10(-Pk[D,2]) + Exp10(-phMid));
                double cglu = aaComposition[E] * Exp10(-Pk[E,2]) / (Exp10(-Pk[E,2]) + Exp10(-phMid));

                double ccys = aaComposition[C] * Exp10(-Pk[C,2]) / (Exp10(-Pk[C,2]) + Exp10(-phMid));
                double ctyr = aaComposition[Y] * Exp10(-Pk[Y,2]) / (Exp10(-Pk[Y,2]) + Exp10(-phMid));

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
