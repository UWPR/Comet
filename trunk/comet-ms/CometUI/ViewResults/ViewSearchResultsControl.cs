using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Xml.XPath;
using BrightIdeasSoftware;
using CometUI.Properties;

namespace CometUI.ViewResults
{
    public partial class ViewSearchResultsControl : UserControl
    {
        public String ResultsPepXMLFile { get; set; }
        public bool SettingsChanged { get; set; }
        public String ErrorMessage { get; private set; }

        private CometUI CometUI { get; set; }
        private bool OptionsPanelShown { get; set; }
        private String SearchDatabaseFile { get; set; }
        private ViewResultsSummaryOptionsControl ViewResultsSummaryOptionsControl { get; set; }
        private ViewResultsDisplayOptionsControl ViewResultsDisplayOptionsControl { get; set; }
        private ViewResultsPickColumnsControl ViewResultsPickColumnsControl { get; set; }
        private List<SearchResult> SearchResults { get; set; }
        private Dictionary<String, SearchResultColumn> ResultColumns { get; set; }

        private const String BlastHttpLink = "http://blast.ncbi.nlm.nih.gov/Blast.cgi?CMD=Web&LAYOUT=TwoWindows&AUTO_FORMAT=Semiauto&ALIGNMENTS=50&ALIGNMENT_VIEW=Pairwise&CDD_SEARCH=on&CLIENT=web&COMPOSITION_BASED_STATISTICS=on&DATABASE=nr&DESCRIPTIONS=100&ENTREZ_QUERY=(none)&EXPECT=1000&FILTER=L&FORMAT_OBJECT=Alignment&FORMAT_TYPE=HTML&I_THRESH=0.005&MATRIX_NAME=BLOSUM62&NCBI_GI=on&PAGE=Proteins&PROGRAM=blastp&SERVICE=plain&SET_DEFAULTS.x=41&SET_DEFAULTS.y=5&SHOW_OVERVIEW=on&END_OF_HTTPGET=Yes&SHOW_LINKOUT=yes&QUERY=";

        public ViewSearchResultsControl(CometUI parent)
        {
            InitializeComponent();

            CometUI = parent;

            InitializeResultsColumns();

            SearchResults = new List<SearchResult>();

            SettingsChanged = false;

            ViewResultsSummaryOptionsControl = new ViewResultsSummaryOptionsControl(this)
                                                   {
                                                       Anchor =
                                                           (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left |
                                                            AnchorStyles.Right),
                                                       Location = new Point(0, 0)
                                                   };
            summaryTabPage.Controls.Add(ViewResultsSummaryOptionsControl);


            ViewResultsDisplayOptionsControl = new ViewResultsDisplayOptionsControl(this)
                                                   {
                                                       Anchor =
                                                           (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left |
                                                            AnchorStyles.Right),
                                                       Location = new Point(0, 0)
                                                   };
            displayOptionsTabPage.Controls.Add(ViewResultsDisplayOptionsControl);

            ViewResultsPickColumnsControl = new ViewResultsPickColumnsControl(this)
                                                {
                                                    Anchor =
                                                        (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left |
                                                         AnchorStyles.Right),
                                                    Location = new Point(0, 0)
                                                };
            pickColumnsTabPage.Controls.Add(ViewResultsPickColumnsControl);

            InitializeFromDefaultSettings();

            UpdateViewSearchResults(String.Empty);
        }

        public void UpdateViewSearchResults(String resultsPepXMLFile)
        {
            ErrorMessage = String.Empty;
            if (null != resultsPepXMLFile)
            {
                ResultsPepXMLFile = resultsPepXMLFile;
                ShowResultsListPanel(String.Empty != ResultsPepXMLFile);
                
                if (!ViewResultsSummaryOptionsControl.UpdateSummaryOptions())
                {
                    MessageBox.Show(ViewResultsSummaryOptionsControl.ErrorMessage,
                                    Resources.ViewResults_View_Results_Title, MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                }

                if (!ReadSearchResults())
                {
                    MessageBox.Show(ErrorMessage, Resources.ViewResults_View_Results_Title, MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                }
            }

            UpdateSearchResultsList();
        }

        public void SaveViewResultsSettings()
        {
            if (SettingsChanged)
            {
                CometUI.ViewResultsSettings.Save();
                SettingsChanged = false;
            }
        }

        public void UpdateSearchResultsList()
        {
            resultsListView.BeginUpdate();
            resultsListView.Clear();
            UpdateColumnHeaders();
            resultsListView.SetObjects(SearchResults);
            resultsListView.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            resultsListView.EndUpdate();
        }

        private void UpdateColumnHeaders()
        {
            resultsListView.Columns.Clear();

            foreach (var item in CometUI.ViewResultsSettings.PickColumnsShowList)
            {
                SearchResultColumn resultCol;
                if (ResultColumns.TryGetValue(item, out resultCol))
                {
                    String columnHeader;
                    if (CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
                    {
                        columnHeader = resultCol.CondensedHeader;
                    }
                    else
                    {
                        columnHeader = resultCol.Header;
                    }

                    var olvColumn = new OLVColumn(columnHeader, resultCol.Aspect)
                    {
                        Hyperlink = resultCol.Hyperlink,
                        MinimumWidth = 30
                    };

                    resultsListView.Columns.Add(olvColumn);
                }
            }
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
            SearchDatabaseFile = pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary/search_database", "local_path");

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

                    var peptideprophetResultNavigator = pepXMLReader.ReadFirstMatchingDescendant(searchHitNavigator, "peptideprophet_result");
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
            var modNodes = pepXMLReader.ReadDescendants(searchHitNavigator, "mod_aminoacid_mass");
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

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            if (CometUI.ViewResultsSettings.ShowOptions)
            {
                ShowViewOptionsPanel();
            }
            else
            {
                HideViewOptionsPanel();
            }
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
                                    {"ions2", new SearchResultColumn("Ions2", "IONS2", "IONS2", true)},
                                    {"mzratio", new SearchResultColumn("MzRatio", "MZRATIO", "MZRATIO")},
                                    {"massdiff", new SearchResultColumn("MassDiff", "MASSDIFF", "MASSDIFF")},
                                    {"ppm", new SearchResultColumn("PPM", "PPM", "PPM")},
                                    {"pi", new SearchResultColumn("PI", "PI", "PPM")}
                                };
        }

        private void ShowViewOptionsPanel()
        {
            showHideOptionsLabel.Text = Resources.ViewSearchResultsControl_ShowViewOptionsPanel_Hide_options;
            showOptionsPanel.Visible = true;
            hideOptionsGroupBox.Visible = false;
            OptionsPanelShown = true;
            resultsListPanel.Location = resultsListPanelNormal.Location;
            resultsListPanel.Size = resultsListPanelNormal.Size;
            showHideOptionsBtn.Text = Resources.ViewSearchResultsControl_ShowViewOptionsPanel__;
        }

        private void HideViewOptionsPanel()
        {
            showHideOptionsLabel.Text = Resources.ViewSearchResultsControl_HideViewOptionsPanel_Show_options;
            showOptionsPanel.Visible = false;
            hideOptionsGroupBox.Visible = true;
            OptionsPanelShown = false;
            resultsListPanel.Location = resultsListPanelFull.Location;
            resultsListPanel.Size = resultsListPanelFull.Size;
            showHideOptionsBtn.Text = Resources.ViewSearchResultsControl_HideViewOptionsPanel__;
        }

        private void ShowHideOptionsBtnClick(object sender, EventArgs e)
        {
            SettingsChanged = true;
            
            if (OptionsPanelShown)
            {
                HideViewOptionsPanel();
                CometUI.ViewResultsSettings.ShowOptions = false;
            }
            else
            {
                ShowViewOptionsPanel();
                CometUI.ViewResultsSettings.ShowOptions = true;
            }
        }

        private void ShowResultsListPanel(bool show)
        {
            if (show)
            {
                resultsListPanel.Show();
            }
            else
            {
                resultsListPanel.Hide();
            }
        }

        private void ResultsListViewCellToolTipShowing(object sender, ToolTipShowingEventArgs e)
        {
            if (e.Column.AspectName.Equals("ProteinDisplayStr"))
            {
                var result = e.Model as SearchResult;
                if ((null != result) && (result.AltProteins.Count > 0))
                {
                    e.AutoPopDelay = 30000;
                    foreach (var altProtein in result.AltProteins)
                    {
                        e.Text += String.Format("{0}\r\n", altProtein.Name);
                    }
                }
            }
        }

        private void ResultsListViewHyperlinkClicked(object sender, HyperlinkClickedEventArgs e)
        {
            switch (e.Column.AspectName)
            {
                case "PeptideDisplayStr":
                    OnPeptideLinkClick(e);
                    break;

                case "ProteinDisplayStr":
                    OnProteinLinkClick(e);
                    break;
            }
        }

        private static void OnPeptideLinkClick(HyperlinkClickedEventArgs e)
        {
            var result = e.Model as SearchResult;
            if (null != result)
            {
                e.Url = BlastHttpLink + result.Peptide;
            }
        }

        private void OnProteinLinkClick(HyperlinkClickedEventArgs e)
        {
            // Todo: Here's how this should work:
            // Try to open the SearchDatabaseFile read in from the pep.xml file.
            // If it works, great, display it on a closable panel that opens up next to/underneath the results list
            // Highlight the peptide sequence that was found
            // If it doesn't work, allow the user to browse to a path where the database file is located, then try again
            // Make sure to allow the user to cancel out of the UI that lets them browse to cancel the operation

            var result = e.Model as SearchResult;
            if (null != result)
            {
                try
                {
                    var dbReader = new SearchDBReader(SearchDatabaseFile);
                    var proteinSequence = dbReader.ReadProtein(result.ProteinInfo.Name);
                    if (null != proteinSequence)
                    {
                        MessageBox.Show(proteinSequence, "View Results", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                catch (Exception exception)
                {
                    MessageBox.Show("Could not look up the protein sequence. " + exception.Message,
                                    "View Results Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }
    }
}
