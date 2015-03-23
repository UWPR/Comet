using System;

namespace CometUI.ViewResults
{
    public class PeptideFragmentCalculator
    {
        public void CalculateIons(SearchResult result, SpectrumGraphUserOptions userOptions)
        {
            MassSpecUtils.InitializeMassTables(userOptions.MassType == MassSpecUtils.MassType.Monoisotopic);

            double nTerm = result.ModifiedNTerm ? result.ModNTermMass : MassSpecUtils.ElementMassTable['h'];
            double cterm = result.ModifiedCTerm ? result.ModCTermMass : MassSpecUtils.ElementMassTable['o'] + MassSpecUtils.ElementMassTable['h'];


            double bIion = nTerm - MassSpecUtils.ElementMassTable['h'] + MassSpecUtils.ProtonMass;
            double yion =  result.CalculatedMass - nTerm + MassSpecUtils.ProtonMass;

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

            switch (userOptions.NeutralLoss)
            {
                case MassSpecUtils.NeutralLoss.None:
                    // Do nothing
                    break;

                case MassSpecUtils.NeutralLoss.H2O:
                    //for (int i = 0; i < result.Peptide.Length; i++)
                    //{
                    //    yion -= (pEnvironment.pdModPeptide[i] -
                    //        MassSpecUtils.AminoAcidMassTable[peptideArray[i]] +
                    //        MassSpecUtils.CommonCompoundsMassTable["H2O"]);
                    //}
                    break;

                case MassSpecUtils.NeutralLoss.NH3:
                    for (int i = 0; i < result.Peptide.Length; i++)
                    {
                        // Todo: need to check if if this AA was modified
                        //yion -= (pEnvironment.pdModPeptide[i] -
                        //    MassSpecUtils.AminoAcidMassTable[] /*pdMassAA[pEnvironment.szPeptide[i]]*/ + 
                        //    MassSpecUtils.ElementMassTable['h'] +
                        //    MassSpecUtils.ElementMassTable['h'] +
                        //    MassSpecUtils.ElementMassTable['h']);
                    }
                    break;
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
        Z
    }

    public class FragmentIon
    {
        public String Label { get; set; }
        public double Mass { get; set; }
        public IonType Type { get; set; }
        public bool Used { get; set; }

        public FragmentIon()
        {
        }

        public FragmentIon(String label, double mass, IonType type, bool used)
        {
            Label = label;
            Mass = mass;
            Type = type;
            Used = used;
        }
    }
}
