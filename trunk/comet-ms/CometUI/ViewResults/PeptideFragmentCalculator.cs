using System;
using System.Collections.Generic;

namespace CometUI.ViewResults
{
    public class PeptideFragmentCalculator
    {
        public void CalculateIons(SearchResult result, SpectrumGraphUserOptions userOptions)
        {
            MassSpecUtils.InitializeMassTables(userOptions.MassType == MassSpecUtils.MassType.Monoisotopic);

            double nTerm = result.ModifiedNTerm ? result.ModNTermMass : MassSpecUtils.ElementMassTable['h'];
            double cterm = result.ModifiedCTerm ? result.ModCTermMass : MassSpecUtils.ElementMassTable['o'] + MassSpecUtils.ElementMassTable['h'];

            double bIon = nTerm - MassSpecUtils.ElementMassTable['h'] + MassSpecUtils.ProtonMass;
            double yIon =  result.CalculatedMass - nTerm + MassSpecUtils.ProtonMass;

            var peptideArray = result.Peptide.ToCharArray();
            var modArray = new double[result.Peptide.Length];
            for (int i = 0; i < modArray.Length; i++)
            {
                modArray[i] = 0.0;
            }

            foreach (var mod in result.Modifications)
            {
                modArray[mod.Position - 1] = mod.Mass;
            }


            // Todo: Ask Jimmy what this is all about
            ///* correct y-ion calculation assuming neutral loss of phosphate */
            //if (pEnvironment.bRemoveMods)
            //{
            //    for (i = 0; i < pEnvironment.iLenPeptide; i++)
            //    {
            //        if (pEnvironment.pdModPeptide[i] != 0.0)
            //            dYion -= (pEnvironment.pdModPeptide[i] - pdMassAA[pEnvironment.szPeptide[i]] + pdMassAA['h'] + pdMassAA['h'] + pdMassAA['o']);
            //    }
            //}

            var fragmentIons = new List<FragmentIon>();
            var fragmentIonsH2OLoss = new List<FragmentIon>();
            var fragmentIonsNH3Loss = new List<FragmentIon>();

            for (int i = 0; i < peptideArray.Length; i++)
            {
                if (modArray[i].Equals(0.0))
                {
                    bIon += MassSpecUtils.AminoAcidMassTable[peptideArray[i]];

                    // Todo: Ask Jimmy what this is all about
                    //if (pEnvironment.bRemoveMods && pEnvironment.pdModPeptide[i] != 0.0)
                    //    dBion -= 18.0;
                }
                else
                {
                    bIon += modArray[i];
                }

                // Singly charged a ion
                double aIon = bIon - MassSpecUtils.CommonCompoundsMassTable["CO"];
                fragmentIons.Add(new FragmentIon(aIon, IonType.A, 1, String.Format("a{0}+", i+1)));
                fragmentIonsNH3Loss.Add(new FragmentIon(aIon-17.0, IonType.A, 1, String.Format("[a{0}+]", i+1)));
                fragmentIonsH2OLoss.Add(new FragmentIon(aIon-17.5, IonType.A, 1, String.Format("<a{0}+>", i+1)));

                // Doubly charged a ion
                double aIonCharge2 = (aIon + MassSpecUtils.ElementMassTable['h'])/2.0;
                fragmentIons.Add(new FragmentIon(aIonCharge2, IonType.A, 2, String.Format("a{0}++", i + 1)));
      
                // Triply charged a ion
                double aIonCharge3 = (aIon + (2*MassSpecUtils.ElementMassTable['h'])) / 3.0;
                fragmentIons.Add(new FragmentIon(aIonCharge3, IonType.A, 3, String.Format("a{0}+++", i + 1)));

                // Singly charged b ion
                fragmentIons.Add(new FragmentIon(bIon, IonType.B, 1, String.Format("b{0}+", i + 1)));
                fragmentIonsNH3Loss.Add(new FragmentIon(bIon - 17.0, IonType.B, 1, String.Format("[b{0}+]", i + 1)));
                fragmentIonsH2OLoss.Add(new FragmentIon(bIon - 17.5, IonType.B, 1, String.Format("<b{0}+>", i + 1)));

                // Doubly charged b ion
                double bIonCharge2 = (bIon + MassSpecUtils.ElementMassTable['h']) / 2.0;
                fragmentIons.Add(new FragmentIon(bIonCharge2, IonType.B, 2, String.Format("b{0}++", i + 1)));

                // Triply charged b ion
                double bIonCharge3 = (bIon + (2 * MassSpecUtils.ElementMassTable['h'])) / 3.0;
                fragmentIons.Add(new FragmentIon(bIonCharge3, IonType.B, 3, String.Format("b{0}+++", i + 1)));

                // Singly charged c ion
                double cIon = bIon + MassSpecUtils.CommonCompoundsMassTable["NH3"];
                fragmentIons.Add(new FragmentIon(cIon, IonType.C, 1, String.Format("c{0}+", i + 1)));
                
                // Doubly charged c ion
                double cIonCharge2 = (cIon + MassSpecUtils.ElementMassTable['h']) / 2.0;
                fragmentIons.Add(new FragmentIon(cIonCharge2, IonType.C, 2, String.Format("c{0}++", i + 1)));

                // Triply charged c ion
                double cIonCharge3 = (cIon + (2 * MassSpecUtils.ElementMassTable['h'])) / 3.0;
                fragmentIons.Add(new FragmentIon(cIonCharge3, IonType.C, 3, String.Format("c{0}+++", i + 1)));
            
                // Todo: X, Y, Z ions
            }
        }
    }


    public enum IonType
    {
        A = 0,
        B,
        C,
        X,
        Y,
        Z,
        UNKNOWN
    }

    public class FragmentIon
    {
        public String Label { get; set; }
        public double Mass { get; set; }
        public IonType Type { get; set; }
        public int Charge { get; set; }
        //public bool Used { get; set; }

        public FragmentIon()
        {
        }

        public FragmentIon(double mass, IonType type, int charge, String label)//, bool used)
        {
            Label = label;
            Mass = mass;
            Type = type;
            Charge = charge;
            //Used = used;
        }
    }
}
