using System;
using System.Collections.Generic;
using System.IO;

namespace CometUI.ViewResults
{
    public class PeptideFragmentCalculator
    {
        private void AddFragmentIon(SpectrumGraphUserOptions userOptions, List<FragmentIon> fragmentIons, double singlyChargedMass, int charge, IonType ionType, int ionNum, MassSpecUtils.NeutralLoss neutralLoss = MassSpecUtils.NeutralLoss.None)
        {
            // Calculate this ion only if:
            //   * the user wants to see this particular ion and charge combination
            //   * either it is NOT a neutral loss ion, OR
            //   * if it IS an neutral loss ion, and the user wants to see this particular neutral loss ion
            if (IsShowThisIonAndCharge(userOptions, ionType, charge) &&
                (!IsNeutralLossIon(neutralLoss) || IsShowThisNeutralLossIon(userOptions, neutralLoss)))
            {
                var chargeStr = String.Empty;
                for (int i = 0; i < charge; i++)
                {
                    chargeStr += "+";
                }

                var label = String.Empty;
                switch (ionType)
                {
                    case IonType.A:
                        label = String.Format("a{0}{1}", ionNum, chargeStr);
                        break;

                    case IonType.B:
                        label = String.Format("b{0}{1}", ionNum, chargeStr);
                        break;

                    case IonType.C:
                        label = String.Format("c{0}{1}", ionNum, chargeStr);
                        break;

                    case IonType.X:
                        label = String.Format("x{0}{1}", ionNum, chargeStr);
                        break;

                    case IonType.Y:
                        label = String.Format("y{0}{1}", ionNum, chargeStr);
                        break;

                    case IonType.Z:
                        label = String.Format("z{0}{1}", ionNum, chargeStr);
                        break;
                }


                double mass = singlyChargedMass;
                if (charge == 1)
                {
                    switch (neutralLoss)
                    {
                        case MassSpecUtils.NeutralLoss.None:
                            // Do nothing
                            break;
                        case MassSpecUtils.NeutralLoss.NH3:
                            mass = mass - 17.0;
                            label = label.Insert(0, "[");
                            label += "]";
                            break;
                        case MassSpecUtils.NeutralLoss.H2O:
                            mass = mass - 17.5;
                            label = label.Insert(0, "<");
                            label += ">";
                            break;
                    }
                }
                else if (charge > 1)
                {
                    mass = (mass + (Convert.ToDouble(charge - 1)*MassSpecUtils.ElementMassTable['h']))/
                            Convert.ToDouble(charge);
                }

                fragmentIons.Add(new FragmentIon(mass, ionType, charge, label, neutralLoss));
            }
        }

        private bool IsShowThisIonAndCharge(SpectrumGraphUserOptions userOptions, IonType ionType, int charge)
        {
            List<int> ionCharges;
            return userOptions.UseIonsMap.TryGetValue(ionType, out ionCharges) && ionCharges.Contains(charge);
        }

        private bool IsNeutralLossIon(MassSpecUtils.NeutralLoss neutralLoss)
        {
            return neutralLoss != MassSpecUtils.NeutralLoss.None;
        }

        private bool IsShowThisNeutralLossIon(SpectrumGraphUserOptions userOptions, MassSpecUtils.NeutralLoss neutralLoss)
        {
            return userOptions.NeutralLoss.Contains(neutralLoss);
        }

        public void CalculateIons(SearchResult result, SpectrumGraphUserOptions userOptions)
        {
            MassSpecUtils.InitializeMassTables(userOptions.MassType == MassSpecUtils.MassType.Monoisotopic);

            double nTerm = result.ModifiedNTerm ? result.ModNTermMass : MassSpecUtils.ElementMassTable['h'];
            double cterm = result.ModifiedCTerm ? result.ModCTermMass : MassSpecUtils.ElementMassTable['o'] + MassSpecUtils.ElementMassTable['h'];

            double bIon = nTerm - MassSpecUtils.ElementMassTable['h'] + MassSpecUtils.ProtonMass;
            //double yIon =  result.CalculatedMass - nTerm + MassSpecUtils.ProtonMass;
            double yIon = cterm + MassSpecUtils.ElementMassTable['h'] + MassSpecUtils.ProtonMass;

            var peptideArray = result.Peptide.ToCharArray();
            var peptideLen = peptideArray.Length;
            var modArray = new double[peptideLen];
            for (int i = 0; i < modArray.Length; i++)
            {
                modArray[i] = 0.0;
            }

            foreach (var mod in result.Modifications)
            {
                modArray[mod.Position - 1] = mod.Mass;
            }

            var fragmentIons = new List<FragmentIon>();
            for (int i = 0; i < peptideLen; i++)
            {
                if (i < peptideLen-1)
                {
                    if (modArray[i].Equals(0.0))
                    {
                        bIon += MassSpecUtils.AminoAcidMassTable[peptideArray[i]];
                    }
                    else
                    {
                        bIon += modArray[i];
                    }

                    // a ions
                    double aIon = bIon - MassSpecUtils.CommonCompoundsMassTable["CO"]; 
                    AddFragmentIon(userOptions, fragmentIons, aIon, 1, IonType.A, i + 1);                                   // Singly charged
                    AddFragmentIon(userOptions, fragmentIons, aIon, 1, IonType.A, i + 1, MassSpecUtils.NeutralLoss.NH3);    // Singly charged, NH3 neutral loss
                    AddFragmentIon(userOptions, fragmentIons, aIon, 1, IonType.A, i + 1, MassSpecUtils.NeutralLoss.H2O);    // Singly charged, H2O neutral loss
                    AddFragmentIon(userOptions, fragmentIons, aIon, 2, IonType.A, i + 1);                                   // Doubly charged
                    AddFragmentIon(userOptions, fragmentIons, aIon, 3, IonType.A, i + 1);                                   // Triply charged

                    // b ions
                    AddFragmentIon(userOptions, fragmentIons, bIon, 1, IonType.B, i + 1);                                   // Singly charged
                    AddFragmentIon(userOptions, fragmentIons, bIon, 1, IonType.B, i + 1, MassSpecUtils.NeutralLoss.NH3);    // Singly charged, NH3 neutral loss    
                    AddFragmentIon(userOptions, fragmentIons, bIon, 1, IonType.B, i + 1, MassSpecUtils.NeutralLoss.H2O);    // Singly charged, H2O neutral loss    
                    AddFragmentIon(userOptions, fragmentIons, bIon, 2, IonType.B, i + 1);                                   // Doubly charged  
                    AddFragmentIon(userOptions, fragmentIons, bIon, 3, IonType.B, i + 1);                                   // Triply charged

                    // c ions
                    double cIon = bIon + MassSpecUtils.CommonCompoundsMassTable["NH3"];
                    AddFragmentIon(userOptions, fragmentIons, cIon, 1, IonType.C, i + 1);                                   // Singly charged
                    AddFragmentIon(userOptions, fragmentIons, cIon, 2, IonType.C, i + 1);                                   // Doubly charged
                    AddFragmentIon(userOptions, fragmentIons, cIon, 3, IonType.C, i + 1);                                   // Triply charged  
                }

                if (i > 0)
                {
                    if (modArray[peptideLen - i].Equals(0.0))
                    {
                        yIon += MassSpecUtils.AminoAcidMassTable[peptideArray[peptideLen - i]];
                    }
                    else
                    {
                        yIon += modArray[peptideLen - i];
                    }

                    // x ions
                    double xIon = yIon + MassSpecUtils.CommonCompoundsMassTable["CO"] - (2 * MassSpecUtils.ElementMassTable['h']);
                    AddFragmentIon(userOptions, fragmentIons, xIon, 1, IonType.X, i);                                  // Singly charged
                    AddFragmentIon(userOptions, fragmentIons, xIon, 2, IonType.X, i);                                  // Doubly charged
                    AddFragmentIon(userOptions, fragmentIons, xIon, 3, IonType.X, i);                                  // Triply charged

                    // y ions
                    AddFragmentIon(userOptions, fragmentIons, yIon, 1, IonType.Y, i);                                  // Singly charged
                    AddFragmentIon(userOptions, fragmentIons, yIon, 1, IonType.Y, i, MassSpecUtils.NeutralLoss.NH3);   // Singly charged, NH3 neutral loss
                    AddFragmentIon(userOptions, fragmentIons, yIon, 1, IonType.Y, i, MassSpecUtils.NeutralLoss.H2O);   // Singly charged, H2O neutral loss
                    AddFragmentIon(userOptions, fragmentIons, yIon, 2, IonType.Y, i);                                  // Doubly charged
                    AddFragmentIon(userOptions, fragmentIons, yIon, 3, IonType.Y, i);                                  // Triply charged

                    // z ions
                    double zIon = yIon - MassSpecUtils.CommonCompoundsMassTable["NH3"] + MassSpecUtils.ElementMassTable['h'];
                    AddFragmentIon(userOptions, fragmentIons, zIon, 1, IonType.Z, i);                                  // Singly charged
                    AddFragmentIon(userOptions, fragmentIons, zIon, 2, IonType.Z, i);                                  // Doubly charged
                    AddFragmentIon(userOptions, fragmentIons, zIon, 3, IonType.Z, i);                                  // Triply charged
                }
            }

            //// Write to a test file to check the fragment ion calculations
            //using (var file = new StreamWriter(@"C:\Projects\Comet\TestFiles\FragmentIons.txt"))
            //{
            //    WriteIonsToTestFile(file, fragmentIons, 1, IonType.A);
            //    WriteIonsToTestFile(file, fragmentIons, 2, IonType.A);
            //    WriteIonsToTestFile(file, fragmentIons, 3, IonType.A);

            //    WriteIonsToTestFile(file, fragmentIons, 1, IonType.B);
            //    WriteIonsToTestFile(file, fragmentIons, 2, IonType.B);
            //    WriteIonsToTestFile(file, fragmentIons, 3, IonType.B);

            //    WriteIonsToTestFile(file, fragmentIons, 1, IonType.C);
            //    WriteIonsToTestFile(file, fragmentIons, 2, IonType.C);
            //    WriteIonsToTestFile(file, fragmentIons, 3, IonType.C);

            //    WriteIonsToTestFile(file, fragmentIons, 1, IonType.X);
            //    WriteIonsToTestFile(file, fragmentIons, 2, IonType.X);
            //    WriteIonsToTestFile(file, fragmentIons, 3, IonType.X);

            //    WriteIonsToTestFile(file, fragmentIons, 1, IonType.Y);
            //    WriteIonsToTestFile(file, fragmentIons, 2, IonType.Y);
            //    WriteIonsToTestFile(file, fragmentIons, 3, IonType.Y);

            //    WriteIonsToTestFile(file, fragmentIons, 1, IonType.Z);
            //    WriteIonsToTestFile(file, fragmentIons, 2, IonType.Z);
            //    WriteIonsToTestFile(file, fragmentIons, 3, IonType.Z);
            //}

        }

        private void WriteIonsToTestFile(StreamWriter file, IEnumerable<FragmentIon> fragmentIons, int charge, IonType ionType)
        {
            var header = String.Format("\nCharge {0}\n", charge);
            file.WriteLine(header);
            foreach (var ion in fragmentIons)
            {
                if (ion.Charge == charge && ion.Type == ionType)
                {
                    var line = String.Format("{0}\t{1}", ion.Label, ion.Mass);
                    file.WriteLine(line);
                }
            }
            file.WriteLine("\n");
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
        public MassSpecUtils.NeutralLoss NeutralLoss { get; set; }

        public FragmentIon()
        {
            Label = String.Empty;
            Mass = 0.0;
            Type = IonType.UNKNOWN;
            Charge = 0;
            NeutralLoss = MassSpecUtils.NeutralLoss.None;
        }

        public FragmentIon(double mass, IonType type, int charge, String label, MassSpecUtils.NeutralLoss neutralLoss = MassSpecUtils.NeutralLoss.None)
        {
            Label = label;
            Mass = mass;
            Type = type;
            Charge = charge;
            NeutralLoss = neutralLoss;
        }
    }
}
