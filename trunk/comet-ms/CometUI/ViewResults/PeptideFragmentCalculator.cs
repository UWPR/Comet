using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CometUI.ViewResults
{
    public class PeptideFragmentCalculator
    {
        public void CalculateIons()
        {
            
        }
    }

    public enum IonType
    {
        IonTypeA = 0,
        IonTypeB,
        IonTypeC,
        IonTypeX,
        IonTypeY,
        IonTypeZ
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
