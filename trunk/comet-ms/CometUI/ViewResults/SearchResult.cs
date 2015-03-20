using System;
using System.Collections.Generic;
using System.Xml.XPath;
using CometUI.Properties;
using CometWrapper;

namespace CometUI.ViewResults
{
    public class SearchResultsManager
    {
        public String ErrorMessage { get; set; }
        public String ResultsPepXMLFile { get; set; }
        public String SearchDatabaseFile { get; set; }
        public String SpectraFile { get; set; }
        public List<SearchResult> SearchResults { get; set; }
        public Dictionary<String, SearchResultColumn> ResultColumns { get; set; }
        public SearchResultParams SearchParams { get; private set; }

        private const MSSpectrumTypeWrapper DefaultMSLevel = MSSpectrumTypeWrapper.MS2;

        public SearchResultsManager()
        {
            ErrorMessage = String.Empty;
            SearchResults = new List<SearchResult>();
            SearchParams = new SearchResultParams();
            InitializeResultsColumns();
        }

        private void InitializeResultsColumns()
        {
            ResultColumns = new Dictionary<String, SearchResultColumn>
                                {
                                    {"index", new SearchResultColumn("Index", "INDEX", "#")},
                                    {"assumed_charge", new SearchResultColumn("AssumedCharge", "ASSUMED_CHARGE", "Z")},
                                    {"start_scan", new SearchResultColumn("StartScan", "START_SCAN", "SSCAN")},
                                    {"spectrum", new SearchResultColumn("Spectrum", "SPECTRUM", "SPECTRUM")},
                                    {"precursor_neutral_mass", new SearchResultColumn("ExperimentalMass", "PRECURSOR_NEUTRAL_MASS", "EXP_MASS")},
                                    {"calc_neutral_pep_mass", new SearchResultColumn("CalculatedMass", "CALC_NEUTRAL_PEP_MASS", "CALC_MASS")},
                                    {"retention_time_sec", new SearchResultColumn("RetentionTimeSec", "RETENTION_TIME_SEC", "RTIME_SEC")},
                                    {"xcorr", new SearchResultColumn("XCorr", "XCORR", "XCORR")},
                                    {"deltacn", new SearchResultColumn("DeltaCN", "DELTACN", "DELTACN")},
                                    {"deltacnstar", new SearchResultColumn("DeltaCNStar", "DELTACNSTAR", "DELTACNSTAR")},
                                    {"spscore", new SearchResultColumn("SpScore", "SPSCORE", "SPSCORE")},
                                    {"sprank", new SearchResultColumn("SpRank", "SPRANK", "SPRANK")},
                                    {"expect", new SearchResultColumn("Expect", "EXPECT", "EXPECT")},
                                    {"probability", new SearchResultColumn("Probability", "PROBABILITY", "PROB", true)},
                                    {"precursor_intensity", new SearchResultColumn("PrecursorIntensity", "PRECURSOR_INTENSITY", "INTENSITY")},
                                    {"protein", new SearchResultColumn("ProteinDisplayStr", "PROTEIN", "PROTEIN", true)},
                                    {"protein_descr", new SearchResultColumn("ProteinDescr", "PROTEIN_DESCR", "PROTEIN_DESCR")},
                                    {"peptide", new SearchResultColumn("PeptideDisplayStr", "PEPTIDE", "PEPTIDE", true)},
                                    {"ions", new SearchResultColumn("Ions", "IONS", "IONS", true)},
                                    {"mzratio", new SearchResultColumn("MzRatio", "MZRATIO", "MZRATIO")},
                                    {"massdiff", new SearchResultColumn("MassDiff", "MASSDIFF", "MASSDIFF")},
                                    {"ppm", new SearchResultColumn("PPM", "PPM", "PPM")},
                                    {"pi", new SearchResultColumn("PI", "PI", "PPM")}
                                };
        }

        public bool UpdateResults(String resultsPepXMLFile)
        {
            ResultsPepXMLFile = resultsPepXMLFile;
            return ReadSearchResults();
        }


        private bool ReadSearchResults()
        {
            SearchResults.Clear();

            String resultsFile = ResultsPepXMLFile;
            if (String.Empty == resultsFile)
            {
                return true;
            }

            // Create a reader for the results file
            PepXMLReader pepXMLReader;
            try
            {
                pepXMLReader = new PepXMLReader(resultsFile);
            }
            catch (Exception e)
            {
                ErrorMessage =
                    Resources.ViewSearchResultsControl_UpdateSearchResultsList_Could_not_read_the_results_pep_xml_file__ +
                    e.Message;
                return false;
            }

            if (!ReadResultsFromPepXML(pepXMLReader))
            {
                ErrorMessage = "Could not read the search results. " + ErrorMessage;
                return false;
            }

            return true;
        }

        private bool ReadResultsFromPepXML(PepXMLReader pepXMLReader)
        {
            ReadSpectraFile(pepXMLReader);

            // Read the database file name
            SearchDatabaseFile =
                pepXMLReader.ReadAttributeFromFirstMatchingNode(
                    "/msms_pipeline_analysis/msms_run_summary/search_summary/search_database", "local_path");

            if (!ReadSearchParams(pepXMLReader))
            {
                return false;
            }

            var spectrumQueryNodes = pepXMLReader.ReadNodes("/msms_pipeline_analysis/msms_run_summary/spectrum_query");
            while (spectrumQueryNodes.MoveNext())
            {
                bool noSearchHits = false;

                var spectrumQueryNavigator = spectrumQueryNodes.Current;
                var result = new SearchResult();

                if (!ReadSpectrumQueryAttributes(pepXMLReader, spectrumQueryNavigator, result))
                {
                    return false;
                }

                var searchResultNodes = pepXMLReader.ReadChildren(spectrumQueryNavigator, "search_result");
                while (searchResultNodes.MoveNext())
                {
                    var searchResultNavigator = searchResultNodes.Current;
                    var searchHitNavigator = pepXMLReader.ReadFirstMatchingChild(searchResultNavigator, "search_hit");
                    if (null == searchHitNavigator)
                    {
                        noSearchHits = true;
                        break;
                    }
                    if (!ReadSearchHitAttributes(pepXMLReader, searchHitNavigator, result))
                    {
                        return false;
                    }

                    int numProteins;
                    if (!pepXMLReader.ReadAttribute(searchHitNavigator, "num_tot_proteins", out numProteins))
                    {
                        ErrorMessage = "Could not read the num_tot_proteins attribute.";
                        return false;
                    }

                    if (numProteins > 1)
                    {
                        if (!ReadAlternativeProteins(pepXMLReader, searchHitNavigator, result))
                        {
                            return false;
                        }
                    }

                    if (!ReadModifications(pepXMLReader, searchHitNavigator, result))
                    {
                        return false;
                    }

                    if (!ReadSearchScores(pepXMLReader, searchHitNavigator, result))
                    {
                        return false;
                    }

                    var peptideprophetResultNavigator = pepXMLReader.ReadFirstMatchingDescendant(searchHitNavigator,
                                                                                                 "peptideprophet_result");
                    if (null != peptideprophetResultNavigator)
                    {
                        if (!ReadPeptideProphetResults(pepXMLReader, peptideprophetResultNavigator, result))
                        {
                            return false;
                        }
                    }
                }

                if (!noSearchHits)
                {
                    SearchResults.Add(result);
                }
            }

            return true;
        }

        private bool ReadSearchParams(PepXMLReader pepXMLReader)
        {
            ReadMSLevel(pepXMLReader);

            if (!ReadUseIons(pepXMLReader))
            {
                return false;
            }

            return true;
        }

        private void ReadMSLevel(PepXMLReader pepXMLReader)
        {
            int msLevel;
            if (
                pepXMLReader.ReadAttributeFromFirstMatchingNode(
                    "/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='ms_level']", "value",
                    out msLevel))
            {
                switch (msLevel)
                {
                    case 2:
                        SearchParams.MSLevel = MSSpectrumTypeWrapper.MS2;
                        break;
                    case 3:
                        SearchParams.MSLevel = MSSpectrumTypeWrapper.MS3;
                        break;
                    default:
                        SearchParams.MSLevel = DefaultMSLevel;
                        break;
                }
            }
            else
            {
                SearchParams.MSLevel = DefaultMSLevel;
            }
        }

        private bool ReadUseIons(PepXMLReader pepXMLReader)
        {
            int useIon;

            if (pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='use_A_ions']", "value", out useIon))
            {
                SearchParams.UseAIons = useIon != 0;
            }
            else
            {
                ErrorMessage = "Could not read the use_A_ions attribute.";
                return false;
            }

            if (pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='use_B_ions']", "value", out useIon))
            {
                SearchParams.UseBIons = useIon != 0;
            }
            else
            {
                ErrorMessage = "Could not read the use_B_ions attribute.";
                return false;
            }

            if (pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='use_C_ions']", "value", out useIon))
            {
                SearchParams.UseCIons = useIon != 0;
            }
            else
            {
                ErrorMessage = "Could not read the use_C_ions attribute.";
                return false;
            }

            if (pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='use_X_ions']", "value", out useIon))
            {
                SearchParams.UseXIons = useIon != 0;
            }
            else
            {
                ErrorMessage = "Could not read the use_X_ions attribute.";
                return false;
            }

            if (pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='use_Y_ions']", "value", out useIon))
            {
                SearchParams.UseYIons = useIon != 0;
            }
            else
            {
                ErrorMessage = "Could not read the use_Y_ions attribute.";
                return false;
            }

            if (pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/parameter[@name='use_Z_ions']", "value", out useIon))
            {
                SearchParams.UseZIons = useIon != 0;
            }
            else
            {
                ErrorMessage = "Could not read the use_Z_ions attribute.";
                return false;
            }

            return true;
        }

        private void ReadSpectraFile(PepXMLReader pepXMLReader)
        {
            SpectraFile = String.Empty;
            var spectraFileName = pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary", "base_name");
            if (!String.IsNullOrEmpty(spectraFileName))
            {
                var spectraFileExtension =
                    pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary",
                                                                    "raw_data");
                if (!String.IsNullOrEmpty(spectraFileExtension))
                {
                    SpectraFile = spectraFileName + spectraFileExtension;
                }
            }
        }

        private bool ReadSpectrumQueryAttributes(PepXMLReader pepXMLReader, XPathNavigator spectrumQueryNavigator, SearchResult result)
        {
            String spectrum;
            if (!pepXMLReader.ReadAttribute(spectrumQueryNavigator, "spectrum", out spectrum))
            {
                ErrorMessage = "Could not read the spectrum attribute.";
                return false;
            }
            result.Spectrum = spectrum;

            int startScan;
            if (!pepXMLReader.ReadAttribute(spectrumQueryNavigator, "start_scan", out startScan))
            {
                ErrorMessage = "Could not read the start_scan attribute.";
                return false;
            }
            result.StartScan = startScan;

            int index;
            if (!pepXMLReader.ReadAttribute(spectrumQueryNavigator, "index", out index))
            {
                ErrorMessage = "Could not read the index attribute.";
                return false;
            }
            result.Index = index;

            int assumedCharge;
            if (!pepXMLReader.ReadAttribute(spectrumQueryNavigator, "assumed_charge", out assumedCharge))
            {
                ErrorMessage = "Could not read the assumed_charge attribute.";
                return false;
            }
            result.AssumedCharge = assumedCharge;

            double precursorNeutralMass;
            if (!pepXMLReader.ReadAttribute(spectrumQueryNavigator, "precursor_neutral_mass", out precursorNeutralMass))
            {
                ErrorMessage = "Could not read the precursor_neutral_mass attribute.";
                return false;
            }
            result.ExperimentalMass = precursorNeutralMass;

            double retentionTimeSec;
            if (!pepXMLReader.ReadAttribute(spectrumQueryNavigator, "retention_time_sec", out retentionTimeSec))
            {
                ErrorMessage = "Could not read the retention_time_sec attribute.";
                return false;
            }
            result.RetentionTimeSec = retentionTimeSec;

            // The "precursor_intensity" field may or may not be there, so just ignore the return value.
            double precursorIntensity;
            pepXMLReader.ReadAttribute(spectrumQueryNavigator, "precursor_intensity", out precursorIntensity);
            result.PrecursorIntensity = precursorIntensity;

            return true;
        }


        private bool ReadSearchHitAttributes(PepXMLReader pepXMLReader, XPathNavigator searchHitNavigator, SearchResult result)
        {
            double calculatedMass;
            if (!pepXMLReader.ReadAttribute(searchHitNavigator, "calc_neutral_pep_mass", out calculatedMass))
            {
                ErrorMessage = "Could not read the calc_neutral_pep_mass attribute.";
                return false;
            }
            result.CalculatedMass = calculatedMass;

            int numMatchedIons;
            if (!pepXMLReader.ReadAttribute(searchHitNavigator, "num_matched_ions", out numMatchedIons))
            {
                ErrorMessage = "Could not read the num_matched_ions attribute.";
                return false;
            }
            result.NumMatchedIons = numMatchedIons;

            int totalNumIons;
            if (!pepXMLReader.ReadAttribute(searchHitNavigator, "tot_num_ions", out totalNumIons))
            {
                ErrorMessage = "Could not read the tot_num_ions attribute.";
                return false;
            }
            result.TotalNumIons = totalNumIons;

            String peptide = pepXMLReader.ReadAttribute(searchHitNavigator, "peptide");
            if (String.IsNullOrEmpty(peptide))
            {
                ErrorMessage = "Could not read the peptide attribute.";
                return false;
            }
            result.Peptide = peptide;

            String peptidePrevAAA = pepXMLReader.ReadAttribute(searchHitNavigator, "peptide_prev_aa");
            if (peptidePrevAAA.Equals(String.Empty))
            {
                ErrorMessage = "Could not read the peptide_prev_aa attribute.";
                return false;
            }

            String peptideNextAAA = pepXMLReader.ReadAttribute(searchHitNavigator, "peptide_next_aa");
            if (peptideNextAAA.Equals(String.Empty))
            {
                ErrorMessage = "Could not read the peptide_next_aa attribute.";
                return false;
            }

            String proteinName = pepXMLReader.ReadAttribute(searchHitNavigator, "protein");
            if (proteinName.Equals(String.Empty))
            {
                ErrorMessage = "Could not read the protein attribute.";
                return false;
            }

            var proteinDescr = pepXMLReader.ReadAttribute(searchHitNavigator, "protein_descr");

            result.ProteinInfo = new ProteinInfo(proteinName, proteinDescr, peptidePrevAAA, peptideNextAAA);

            return true;
        }

        private bool ReadAlternativeProteins(PepXMLReader pepXMLReader, XPathNavigator searchHitNavigator, SearchResult result)
        {
            var alternativeProteinNodes = pepXMLReader.ReadDescendants(searchHitNavigator,
                                                                        "alternative_protein");
            while (alternativeProteinNodes.MoveNext())
            {
                var altProteinNavigator = alternativeProteinNodes.Current;
                String altProteinName = pepXMLReader.ReadAttribute(altProteinNavigator, "protein");
                if (altProteinName.Equals(String.Empty))
                {
                    ErrorMessage = "Could not read the protein attribute.";
                    return false;
                }

                String altPeptidePrevAAA = pepXMLReader.ReadAttribute(altProteinNavigator,
                                                                        "peptide_prev_aa");
                if (altPeptidePrevAAA.Equals(String.Empty))
                {
                    ErrorMessage = "Could not read the peptide_prev_aa attribute.";
                    return false;
                }

                String altPeptideNextAAA = pepXMLReader.ReadAttribute(altProteinNavigator,
                                                                        "peptide_next_aa");
                if (altPeptideNextAAA.Equals(String.Empty))
                {
                    ErrorMessage = "Could not read the peptide_next_aa attribute.";
                    return false;
                }

                String altProteinDescr = pepXMLReader.ReadAttribute(altProteinNavigator, "protein_descr");

                var altProteinInfo = new ProteinInfo(altProteinName, altProteinDescr, altPeptidePrevAAA,
                                                        altPeptideNextAAA);
                result.AltProteins.Add(altProteinInfo);
            }

            return true;
        }

        private bool ReadSearchScores(PepXMLReader pepXMLReader, XPathNavigator searchHitNavigator, SearchResult result)
        {
            var searchScoreNodes = pepXMLReader.ReadDescendants(searchHitNavigator, "search_score");
            while (searchScoreNodes.MoveNext())
            {
                var searchScoreNavigator = searchScoreNodes.Current;
                String name = pepXMLReader.ReadAttribute(searchScoreNavigator, "name");
                if (name.Equals(String.Empty))
                {
                    ErrorMessage = "Could not read a search score name attribute.";
                    return false;
                }

                switch (name)
                {
                    case "xcorr":
                        double xcorrValue;
                        if (!pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out xcorrValue))
                        {
                            ErrorMessage = "Could not read the xcorr value attribute.";
                            return false;
                        }
                        result.XCorr = xcorrValue;
                        break;

                    case "deltacn":
                        double deltaCNValue;
                        if (!pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out deltaCNValue))
                        {
                            ErrorMessage = "Could not read the deltacn value attribute.";
                            return false;
                        }
                        result.DeltaCN = deltaCNValue;
                        break;

                    case "deltacnstar":
                        double deltaCNStar;
                        if (!pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out deltaCNStar))
                        {
                            ErrorMessage = "Could not read the deltacnstar value attribute.";
                            return false;
                        }
                        result.DeltaCNStar = deltaCNStar;
                        break;

                    case "spscore":
                        double spscore;
                        if (!pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out spscore))
                        {
                            ErrorMessage = "Could not read the spscore value attribute.";
                            return false;
                        }
                        result.SpScore = spscore;
                        break;

                    case "sprank":
                        int sprank;
                        pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out sprank);
                        result.SpRank = sprank;
                        break;

                    case "expect":
                        double expect;
                        pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out expect);
                        result.Expect = expect;
                        break;
                }
            }

            return true;
        }

        private bool ReadPeptideProphetResults(PepXMLReader pepXMLReader, XPathNavigator peptideprophetResultNavigator, SearchResult result)
        {
            double probability;
            if (!pepXMLReader.ReadAttribute(peptideprophetResultNavigator, "probability", out probability))
            {
                ErrorMessage = "Could not read the probability attribute.";
                return false;
            }
            result.Probability = probability;
            return true;
        }

        private bool ReadModifications(PepXMLReader pepXMLReader, XPathNavigator searchHitNavigator, SearchResult result)
        {
            var modInfoNavigator = pepXMLReader.ReadFirstMatchingDescendant(searchHitNavigator, "modification_info");
            if (null != modInfoNavigator)
            {
                double modNTermMass;
                if (pepXMLReader.ReadAttribute(modInfoNavigator, "mod_nterm_mass", out modNTermMass))
                {
                    result.ModNTermMass = modNTermMass;
                }
                else
                {
                    result.ModNTermMass = 0.0;
                }

                double modCTermMass;
                if (pepXMLReader.ReadAttribute(modInfoNavigator, "mod_cterm_mass", out modCTermMass))
                {
                    result.ModCTermMass = modCTermMass;
                }
                else
                {
                    result.ModCTermMass = 0.0;
                }

                var modNodes = pepXMLReader.ReadDescendants(modInfoNavigator, "mod_aminoacid_mass");
                while (modNodes.MoveNext())
                {
                    var modNavigator = modNodes.Current;
                    int pos;
                    if (!pepXMLReader.ReadAttribute(modNavigator, "position", out pos))
                    {
                        ErrorMessage = "Could not read the mod_aminoacid_mass position attribute.";
                        return false;
                    }

                    double mass;
                    if (!pepXMLReader.ReadAttribute(modNavigator, "mass", out mass))
                    {
                        ErrorMessage = "Could not read the mod_aminoacid_mass mass attribute.";
                        return false;
                    }

                    result.Modifications.Add(new ModificationInfo(pos, mass));
                }
            }

            return true;
        }


    }

    public class SearchResult
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
        public double ModNTermMass { get; set; }
        public double ModCTermMass { get; set; }
        public bool ModifiedNTerm { get { return ModNTermMass > 0.0; } }
        public bool ModifiedCTerm { get { return ModCTermMass > 0.0; } }


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

        public String Ions
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

    public class SearchResultParams
    {
        public MSSpectrumTypeWrapper MSLevel { get; set; }
        public MassSpecUtils.MassType MassTypeFragment { get; set; }
        public bool UseAIons { get; set; }
        public bool UseBIons { get; set; }
        public bool UseCIons { get; set; }
        public bool UseXIons { get; set; }
        public bool UseYIons { get; set; }
        public bool UseZIons { get; set; }
    }
}
