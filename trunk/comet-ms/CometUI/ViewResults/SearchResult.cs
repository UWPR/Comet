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
            get { return ProteinInfo.PeptidePrevAA + "." + Peptide + "." + ProteinInfo.PeptideNextAA; }
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
