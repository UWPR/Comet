using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using System.Xml.XPath;
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
        private ViewResultsSummaryOptionsControl ViewResultsSummaryOptionsControl { get; set; }
        private ViewResultsDisplayOptionsControl ViewResultsDisplayOptionsControl { get; set; }
        private ViewResultsPickColumnsControl ViewResultsPickColumnsControl { get; set; }

        private List<SearchResult> SearchResults { get; set; }

        private readonly Dictionary<String, String> _condensedColumnHeadersMap = new Dictionary<String, String>();

        public ViewSearchResultsControl(CometUI parent)
        {
            InitializeComponent();

            CometUI = parent;

            InitializeColumnHeadersMap();

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

            if (resultsListView.Columns.Count > 0)
            {
                foreach (var searchResult in SearchResults)
                {
                    var resultColumns = GetResultColumns(searchResult);
                    var item = new ListViewItem(resultColumns[0]);
                    for (int i = 1; i < resultColumns.Count; i++)
                    {
                        item.SubItems.Add(resultColumns[i]);
                    }

                    TagAlternateProteins(searchResult, item);

                    resultsListView.Items.Add(item);
                }
            }

            resultsListView.EndUpdate();
        }

        private List<String> GetResultColumns(SearchResult searchResult)
        {
            //_columnHeadersMap.Add("pI", "PI");
            //_columnHeadersMap.Add("xpress", "XPRESS");

            var resultColumns = new List<String>();
            ProteinInfo proteinInfo = null;
            double calcNeutralMass = -1;
            double expMass = -1;
            String peptide = null;
            foreach (ColumnHeader column in resultsListView.Columns)
            {
                var columnHeader = column.Text;
                switch (columnHeader)
                {
                    case "#":
                    case "INDEX":
                        var index = ((TypedSearchResultField<int>)searchResult.Fields["index"]).Value;
                        resultColumns.Add(Convert.ToString(index));
                        break;

                    case "ASSUMED_CHARGE":
                    case "Z":
                        var charge = ((TypedSearchResultField<int>)searchResult.Fields["assumed_charge"]).Value;
                        resultColumns.Add(Convert.ToString(charge));
                        break;

                    case "PRECURSOR_NEUTRAL_MASS":
                    case "EXP_MASS":
                        if (expMass < 0)
                        {
                            expMass = ((TypedSearchResultField<double>)searchResult.Fields["precursor_neutral_mass"]).Value;
                        }
                        resultColumns.Add(Convert.ToString(expMass));
                        break;

                    case "PROB":
                    case "PROBABILITY":
                        ISearchResultField probField;
                        String probabilityDisplayString = String.Empty;
                        if (searchResult.Fields.TryGetValue("probability", out probField))
                        {
                            var probability = ((TypedSearchResultField<double>)probField).Value;
                            probabilityDisplayString = Convert.ToString(probability);
                        }

                        resultColumns.Add(probabilityDisplayString);
                        break;

                    case "SSCAN":
                    case "START_SCAN":
                        var sscan = ((TypedSearchResultField<int>)searchResult.Fields["start_scan"]).Value;
                        resultColumns.Add(Convert.ToString(sscan));
                        break;

                    case "CALC_MASS":
                    case "CALC_NEUTRAL_PEP_MASS":
                        var calcMass =
                            ((TypedSearchResultField<double>)searchResult.Fields["calc_neutral_pep_mass"]).Value;
                        resultColumns.Add(Convert.ToString(calcMass));
                        break;

                    case "SPECTRUM":
                        var spectrum = ((TypedSearchResultField<String>)searchResult.Fields["spectrum"]).Value;
                        resultColumns.Add(spectrum);
                        break;

                    case "RETENTION_TIME_SEC":
                        var retentionTime =
                            ((TypedSearchResultField<double>)searchResult.Fields["retention_time_sec"]).Value;
                        resultColumns.Add(Convert.ToString(retentionTime));
                        break;

                    case "PRECURSOR_INTENSITY":
                        ISearchResultField precursorIntensityField;
                        String precursorIntensityDisplayString = String.Empty;
                        if (searchResult.Fields.TryGetValue("precursor_intensity", out precursorIntensityField))
                        {
                            var intensity = ((TypedSearchResultField<double>)precursorIntensityField).Value;
                            precursorIntensityDisplayString = Convert.ToString(intensity);
                        }
                        resultColumns.Add(precursorIntensityDisplayString);
                        break;

                    case "XCORR":
                        var xcorr = ((TypedSearchResultField<double>)searchResult.Fields["xcorr"]).Value;
                        resultColumns.Add(Convert.ToString(xcorr));
                        break;

                    case "DELTACN":
                        var deltacn = ((TypedSearchResultField<double>)searchResult.Fields["deltacn"]).Value;
                        resultColumns.Add(Convert.ToString(deltacn));
                        break;

                    case "DELTACNSTAR":
                        var deltacnstar = ((TypedSearchResultField<double>)searchResult.Fields["deltacnstar"]).Value;
                        resultColumns.Add(Convert.ToString(deltacnstar));
                        break;

                    case "SPSCORE":
                        var spscore = ((TypedSearchResultField<double>)searchResult.Fields["spscore"]).Value;
                        resultColumns.Add(Convert.ToString(spscore));
                        break;

                    case "EXPECT":
                        ISearchResultField expectField;
                        if (searchResult.Fields.TryGetValue("expect", out expectField))
                        {
                            var expect = ((TypedSearchResultField<double>)expectField).Value;
                            resultColumns.Add(Convert.ToString(expect));
                        }
                        else
                        {
                            resultColumns.Add(String.Empty);
                        }
                        break;

                    case "PEPTIDE":
                        if (null == peptide)
                        {
                            peptide = ((TypedSearchResultField<String>) searchResult.Fields["peptide"]).Value;
                        }
                        if (null == proteinInfo)
                        {
                            proteinInfo =
                                ((TypedSearchResultField<ProteinInfo>)searchResult.Fields["protein"]).Value;
                        }
                        resultColumns.Add(proteinInfo.PeptidePrevAA + "." + peptide + "." + proteinInfo.PeptideNextAA);
                        break;

                    case "PROTEIN_DESCR":
                        if (null == proteinInfo)
                        {
                            proteinInfo =
                                ((TypedSearchResultField<ProteinInfo>)searchResult.Fields["protein"]).Value;
                        }
                        resultColumns.Add(proteinInfo.ProteinDescr);
                        break;

                    case "PROTEIN":
                        if (null == proteinInfo)
                        {
                            proteinInfo =
                                ((TypedSearchResultField<ProteinInfo>)searchResult.Fields["protein"]).Value;
                        }
                        String proteinDisplayText = proteinInfo.Name;

                        ISearchResultField altProteinsField;
                        if (searchResult.Fields.TryGetValue("alternative_protein", out altProteinsField))
                        {
                            var altProteins = ((TypedSearchResultField<List<ProteinInfo>>)altProteinsField).Value;
                            if (altProteins.Count > 0)
                            {
                                proteinDisplayText += " +" + altProteins.Count.ToString(CultureInfo.InvariantCulture);
                            }
                        }
                        resultColumns.Add(proteinDisplayText);
                        break;

                    case "MZRATIO":
                        if (calcNeutralMass < 0)
                        {
                            calcNeutralMass =
                                ((TypedSearchResultField<double>)searchResult.Fields["calc_neutral_pep_mass"]).
                                    Value;
                        }
                        var assumedCharge = ((TypedSearchResultField<int>)searchResult.Fields["assumed_charge"]).Value;
                        double mzRatio = MassSpecUtils.CalculateMzRatio(calcNeutralMass, assumedCharge);
                        mzRatio = Math.Round(mzRatio, 4);

                        resultColumns.Add(Convert.ToString(mzRatio));
                        break;

                    case "MASSDIFF":
                        if (calcNeutralMass < 0)
                        {
                            calcNeutralMass =
                                ((TypedSearchResultField<double>)searchResult.Fields["calc_neutral_pep_mass"]).
                                    Value;
                        }

                        if (expMass < 0)
                        {
                            expMass = ((TypedSearchResultField<double>)searchResult.Fields["precursor_neutral_mass"]).Value;
                        }

                        var massDiff = MassSpecUtils.CalculateMassDiff(calcNeutralMass, expMass);
                        massDiff = Math.Round(massDiff, 4);

                        resultColumns.Add(Convert.ToString(massDiff));
                        break;

                    case "PPM":
                        // (cal mass - exact mass)/exact mass) x 10^6 
                        if (calcNeutralMass < 0)
                        {
                            calcNeutralMass =
                                ((TypedSearchResultField<double>)searchResult.Fields["calc_neutral_pep_mass"]).
                                    Value;
                        }

                        if (expMass < 0)
                        {
                            expMass = ((TypedSearchResultField<double>)searchResult.Fields["precursor_neutral_mass"]).Value;
                        }

                        var ppm = MassSpecUtils.CalculateMassErrorPPM(calcNeutralMass, expMass);
                        ppm = Math.Round(ppm, 4);

                        resultColumns.Add(ppm.ToString(CultureInfo.InvariantCulture));
                        break;

                    case "IONS2":
                        // Eventually, clicking this value should show something like Vagisha's "lorikeet" tool to show graphical drawing: https://code.google.com/p/lorikeet/
                        int numMatchedIons = ((TypedSearchResultField<int>)searchResult.Fields["num_matched_ions"]).Value;
                        var matchedIonsStr = Convert.ToString(numMatchedIons);

                        int totNumIons = ((TypedSearchResultField<int>)searchResult.Fields["tot_num_ions"]).Value;
                        var totalIonsStr = Convert.ToString(totNumIons);

                        resultColumns.Add(matchedIonsStr + "/" + totalIonsStr);
                        break;

                    case "PI":
                        if (null == peptide)
                        {
                            peptide = ((TypedSearchResultField<String>)searchResult.Fields["peptide"]).Value;
                        }
                        var pI = MassSpecUtils.CalculatePI(peptide);
                        pI = Math.Round(pI, 2);

                        resultColumns.Add(Convert.ToString(pI));
                        break;

                    default:
                        resultColumns.Add(String.Empty);
                        break;
                }
            }

            return resultColumns;
        }

        private void TagAlternateProteins(SearchResult searchResult, ListViewItem item)
        {
            int proteinColumn = FindColumnIndex(resultsListView, "PROTEIN");
            if (proteinColumn >= 0)
            {
                ISearchResultField altProteinsField;
                if (searchResult.Fields.TryGetValue("alternative_protein", out altProteinsField))
                {
                    var altProteins = ((TypedSearchResultField<List<ProteinInfo>>)altProteinsField).Value;
                    foreach (var altProtein in altProteins)
                    {
                        item.SubItems[proteinColumn].Tag += altProtein.Name + Environment.NewLine;
                    }
                }
            }
        }

        private int FindColumnIndex(ListView list, String columnHeaderText)
        {
            foreach (ColumnHeader header in list.Columns)
            {
                if (header.Text.Equals(columnHeaderText))
                {
                    return header.Index;
                }
            }

            return -1;
        }

        private void UpdateColumnHeaders()
        {
            foreach (var item in CometUI.ViewResultsSettings.PickColumnsShowList)
            {
                String columnHeader;
                String value;
                if (CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders &&
                    _condensedColumnHeadersMap.TryGetValue(item, out value))
                {
                    columnHeader = value.ToUpper();
                }
                else
                {
                    columnHeader = item.ToUpper();
                }

                resultsListView.Columns.Add(columnHeader);
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

        private bool ReadSpectrumQueryAttributes(PepXMLReader pepXMLReader, XPathNavigator spectrumQueryNavigator,
                                                 SearchResult result)
        {
            if (!ResultFieldFromAttribute<String>(pepXMLReader, spectrumQueryNavigator, "spectrum", result))
            {
                ErrorMessage = "Could not read the spectrum attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<int>(pepXMLReader, spectrumQueryNavigator, "start_scan", result))
            {
                ErrorMessage = "Could not read the start_scan attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<int>(pepXMLReader, spectrumQueryNavigator, "index", result))
            {
                ErrorMessage = "Could not read the index attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<int>(pepXMLReader, spectrumQueryNavigator, "assumed_charge", result))
            {
                ErrorMessage = "Could not read the assumed_charge attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<double>(pepXMLReader, spectrumQueryNavigator, "precursor_neutral_mass", result))
            {
                ErrorMessage = "Could not read the precursor_neutral_mass attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<double>(pepXMLReader, spectrumQueryNavigator, "retention_time_sec", result))
            {
                ErrorMessage = "Could not read the retention_time_sec attribute.";
                return false;
            }

            // The "precursor_intensity" field may or may not be there, so just ignore the return value.
            ResultFieldFromAttribute<double>(pepXMLReader, spectrumQueryNavigator, "precursor_intensity",
                                             result);

            return true;
        }

        private bool ReadSearchHitAttributes(PepXMLReader pepXMLReader, XPathNavigator searchHitNavigator, SearchResult result)
        {
            if (!ResultFieldFromAttribute<double>(pepXMLReader, searchHitNavigator, "calc_neutral_pep_mass",
                                                              result))
            {
                ErrorMessage = "Could not read the calc_neutral_pep_mass attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<int>(pepXMLReader, searchHitNavigator, "num_matched_ions",
                                                  result))
            {
                ErrorMessage = "Could not read the num_matched_ions attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<int>(pepXMLReader, searchHitNavigator, "tot_num_ions",
                                      result))
            {
                ErrorMessage = "Could not read the tot_num_ions attribute.";
                return false;
            }

            if (!ResultFieldFromAttribute<String>(pepXMLReader, searchHitNavigator, "peptide",
                                                  result))
            {
                ErrorMessage = "Could not read the peptide attribute.";
                return false;
            }

            String proteinName = pepXMLReader.ReadAttribute(searchHitNavigator, "protein");
            if (proteinName.Equals(String.Empty))
            {
                ErrorMessage = "Could not read the protein attribute.";
                return false;
            }

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

            String proteinDescr = pepXMLReader.ReadAttribute(searchHitNavigator, "protein_descr");

            var proteinInfo = new ProteinInfo(proteinName, proteinDescr, peptidePrevAAA, peptideNextAAA);
            var proteinResultField = new TypedSearchResultField<ProteinInfo>(proteinInfo);
            result.Fields.Add("protein", proteinResultField);

            return true;
        }

        private bool ReadAlternativeProteins(PepXMLReader pepXMLReader, XPathNavigator searchHitNavigator, SearchResult result)
        {
            var altProteinsList = new List<ProteinInfo>();
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
                altProteinsList.Add(altProteinInfo);
            }

            var altProteinResultField = new TypedSearchResultField<List<ProteinInfo>>(altProteinsList);
            result.Fields.Add("alternative_protein", altProteinResultField);

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

                double value;
                if (!pepXMLReader.ReadAttribute(searchScoreNavigator, "value", out value))
                {
                    ErrorMessage = "Could not read a search score value attribute.";
                    return false;
                }

                result.Fields.Add(name, new TypedSearchResultField<double>(value));
            }

            return true;
        }

        private bool ReadPeptideProphetResults(PepXMLReader pepXMLReader, XPathNavigator peptideprophetResultNavigator, SearchResult result)
        {
            if (!ResultFieldFromAttribute<double>(pepXMLReader, peptideprophetResultNavigator, "probability", result))
            {
                ErrorMessage = "Could not read the probability attribute.";
                return false;
            }

            return true;
        }

        private bool ResultFieldFromAttribute<T>(PepXMLReader pepXMLReader, XPathNavigator nodeNavigator, String attributeName, SearchResult result)
        {
            T attribute;
            if (!pepXMLReader.ReadAttribute(nodeNavigator, attributeName, out attribute))
            {
                return false;
            }

            var resultField = new TypedSearchResultField<T>(attribute);
            result.Fields.Add(attributeName, resultField);
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

        private void InitializeColumnHeadersMap()
        {
            // These are the column headers that have alternate condensed names
            _condensedColumnHeadersMap.Add("index", "#");
            _condensedColumnHeadersMap.Add("assumed_charge", "Z");
            _condensedColumnHeadersMap.Add("precursor_neutral_mass", "EXP_MASS");
            _condensedColumnHeadersMap.Add("start_scan", "SSCAN");
            _condensedColumnHeadersMap.Add("calc_neutral_pep_mass", "CALC_MASS");
            _condensedColumnHeadersMap.Add("probability", "PROB");
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

        private void ResultsListViewItemMouseHover(object sender, ListViewItemMouseHoverEventArgs e)
        {
            viewResultsToolTip.Hide(sender as IWin32Window);

            Point mousePos = resultsListView.PointToClient(MousePosition);
            ListViewHitTestInfo ht = resultsListView.HitTest(mousePos);
            if ((ht.SubItem != null))
            {
                var text = ht.SubItem.Tag as String;
                if (!String.IsNullOrEmpty(text))
                {
                    viewResultsToolTip.Show(text, sender as IWin32Window, mousePos, 5000);
                }
            }
        }

        private void ResultsListViewMouseMove(object sender, MouseEventArgs e)
        {
            Point mousePos = resultsListView.PointToClient(MousePosition);
            ListViewHitTestInfo ht = resultsListView.HitTest(mousePos);
            if ((ht.SubItem == null))
            {
                viewResultsToolTip.Hide(sender as IWin32Window);
            }
        }

        private void ResultsListViewMouseLeave(object sender, EventArgs e)
        {
            Point mousePos = resultsListView.PointToClient(MousePosition);
            ListViewHitTestInfo ht = resultsListView.HitTest(mousePos);
            if ((ht.SubItem == null))
            {
                viewResultsToolTip.Hide(sender as IWin32Window);
            }
        }
    }

    public interface ISearchResultField
    {
        Type FieldType { get; }
    }

    public class TypedSearchResultField<T> : ISearchResultField
    {
        public T Value { get; set; }

        public TypedSearchResultField(T value)
        {
            Value = value;
        }

        public Type FieldType
        {
            get { return typeof (T); }
        }
    }

    public class SearchResult
    {
        public Dictionary<String, ISearchResultField> Fields { get; set; }

        public SearchResult()
        {
            Fields = new Dictionary<string, ISearchResultField>();
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
}
