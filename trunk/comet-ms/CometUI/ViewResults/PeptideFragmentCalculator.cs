using System;

namespace CometUI.ViewResults
{
    public class PeptideFragmentCalculator
    {
        public void CalculateIons(SearchResult result, SearchResultParams resultParams)
        {
            MassSpecUtils.InitializeMassTables(resultParams.MassTypeFragment == MassSpecUtils.MassType.Monoisotopic);

            double nTerm = result.ModifiedNTerm ? result.ModNTermMass : MassSpecUtils.ElementMassTable['h'];
            double cterm = result.ModifiedCTerm ? result.ModCTermMass : MassSpecUtils.ElementMassTable['o'] + MassSpecUtils.ElementMassTable['h'];


            double bIion = nTerm - MassSpecUtils.ElementMassTable['h'] + MassSpecUtils.ProtonMass;
            double yion =  result.CalculatedMass - nTerm + MassSpecUtils.ProtonMass;

            
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
