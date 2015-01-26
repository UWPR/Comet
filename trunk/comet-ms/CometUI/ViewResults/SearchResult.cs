using System;
using System.Collections.Generic;

namespace CometUI.ViewResults
{
    class SearchResult
    {
        public int Index { get; set; }
        public int AssumedCharge { get; set; }
        public int StartScan { get; set; }
        public String Spectrum { get; set; }
        public String Peptide { get; set; }
        public List<ModificationInfo> Modifications { get; set; }
        public ProteinInfo ProteinInfo { get; set; }
        public List<ProteinInfo> AltProteins { get; set; }
        public double ExperimentalMass { get; set; }
        public double CalculatedMass { get; set; }
        public double RetentionTimeSec { get; set; }
        public int NumMatchedIons { get; set; }
        public int TotalNumIons { get; set; }
        public double XCorr { get; set; }
        public double DeltaCN { get; set; }
        public double DeltaCNStar { get; set; }
        public double SpScore { get; set; }
        public int SpRank { get; set; }
        public double Expect { get; set; }
        public double Probability { get; set; }
        public double PrecursorIntensity { get; set; }

        public String ProteinDisplayStr
        {
            get
            {
                String proteinDisplayStr;
                if (AltProteins.Count > 0)
                {
                    proteinDisplayStr = String.Format(ProteinInfo.Name + " +{0}", AltProteins.Count);
                }
                else
                {
                    proteinDisplayStr = ProteinInfo.Name;
                }
                return proteinDisplayStr;
            }
        }

        public String ProteinDescr
        {
            get { return ProteinInfo.ProteinDescr; }
        }
        
        public String PeptideDisplayStr
        {
            get
            {
                var peptideStringBuilder = new System.Text.StringBuilder();
                
                // Append the previous amino acid to the peptide sequence
                peptideStringBuilder.Append(ProteinInfo.PeptidePrevAA + ".");
                
                if (Modifications.Count == 0)
                {
                    // If there are no mods, just append the peptide sequence
                    peptideStringBuilder.Append(Peptide);
                }
                else
                {
                    // Todo: Verify this logic using the TPP pep XML viewer
                    // If we have modifications, we need to append the mod mass
                    // next to each modified amino acid
                    int index = 0;
                    foreach (var mod in Modifications)
                    {
                        // Append the peptide sequence up until the position of the mod
                        peptideStringBuilder.Append(Peptide, index, mod.Position - index);

                        // Append the mod mass next to the modified AA
                        peptideStringBuilder.Append(String.Format("{0}", Math.Round(mod.Mass, 2)));

                        index = mod.Position;
                    }

                    if (index < Peptide.Length)
                    {
                        // Append the rest of the peptide sequence after the last mod
                        peptideStringBuilder.Append(Peptide, index, Peptide.Length - index);
                    }
                }

                // Append the next amino acid to the end of the peptide sequence
                peptideStringBuilder.Append("." + ProteinInfo.PeptideNextAA);

                return peptideStringBuilder.ToString();
            }
        }

        public String Ions2
        {
            get
            {
                // Eventually, clicking this value should show something like Vagisha's "lorikeet" tool to show graphical drawing: https://code.google.com/p/lorikeet/
                return String.Format("{0}/{1}", NumMatchedIons, TotalNumIons);
            }
        }

        public double MzRatio
        {
            get
            {
                double mzRatio = MassSpecUtils.CalculateMzRatio(CalculatedMass, AssumedCharge);
                mzRatio = Math.Round(mzRatio, 4);
                return mzRatio;
            }
        }

        public double MassDiff
        {
            get
            {
                var massDiff = MassSpecUtils.CalculateMassDiff(CalculatedMass, ExperimentalMass);
                massDiff = Math.Round(massDiff, 4);
                return massDiff;
            }
        }

        public double PPM
        {
            get
            {
                var ppm = MassSpecUtils.CalculateMassErrorPPM(CalculatedMass, ExperimentalMass);
                ppm = Math.Round(ppm, 4);
                return ppm;
            }
        }

        public double PI
        {
            get
            {
                var pI = MassSpecUtils.CalculatePI(Peptide);
                pI = Math.Round(pI, 2);
                return pI;
            }
        }

        public SearchResult()
        {
            AltProteins = new List<ProteinInfo>();
            Modifications = new List<ModificationInfo>();
        }
    }

    public class ProteinInfo
    {
        public String Name { get; set; }
        public String ProteinDescr { get; set; }
        public String PeptidePrevAA { get; set; }
        public String PeptideNextAA { get; set; }

        public ProteinInfo()
        {
            Name = String.Empty;
            ProteinDescr = String.Empty;
            PeptidePrevAA = String.Empty;
            PeptideNextAA = String.Empty;
        }

        public ProteinInfo(String name, String protDescr, String prevAA, String nextAA)
        {
            Name = name;
            ProteinDescr = protDescr;
            PeptidePrevAA = prevAA;
            PeptideNextAA = nextAA;
        }
    }

    public class ModificationInfo
    {
        public int Position { get; set; }
        public double Mass { get; set; }

        public ModificationInfo()
        {
            Position = 0;
            Mass = 0.0;
        }

        public ModificationInfo(int pos, double mass)
        {
            Position = pos;
            Mass = mass;
        }
    }

    public class SearchResultColumn
    {
        public String Aspect { get; set; }  // E.g. "SearchResult.AssumedCharge"
        public String Header { get; set; }
        public String CondensedHeader { get; set; }
        public bool Hyperlink { get; set; }

        public SearchResultColumn(String aspect, String header, String condensedHeader)
        {
            Aspect = aspect;
            Header = header;
            CondensedHeader = condensedHeader;
            Hyperlink = false;
        }

        public SearchResultColumn(String aspect, String header, String condensedHeader, bool hyperlink)
        {
            Aspect = aspect;
            Header = header;
            CondensedHeader = condensedHeader;
            Hyperlink = hyperlink;
        }
    }
}
