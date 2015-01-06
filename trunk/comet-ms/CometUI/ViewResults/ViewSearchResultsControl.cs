using System;
using System.Collections.Generic;
using System.Drawing;
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

            //_condensedColumnHeadersMap.Add("index", "#");
            //_condensedColumnHeadersMap.Add("assumed_charge", "Z");
            //_condensedColumnHeadersMap.Add("precursor_neutral_mass", "EXP_MASS");
            //_condensedColumnHeadersMap.Add("probability", "PROB");
            //_condensedColumnHeadersMap.Add("start_scan", "SSCAN");
            //_condensedColumnHeadersMap.Add("calc_neutral_pep_mass", "CALC_MASS");

            //_columnHeadersMap.Add("MZratio", "MZRATIO");
            //_columnHeadersMap.Add("protein_descr", "PROTEIN_DESCR");
            //_columnHeadersMap.Add("pI", "PI");
            //_columnHeadersMap.Add("retention_time_sec", "RETENTION_TIME_SEC");
            //_columnHeadersMap.Add("precursor_intensity", "PRECURSOR_INTENSITY");
            //_columnHeadersMap.Add("ppm", "PPM");
            //_columnHeadersMap.Add("xcorr", "XCORR");
            //_columnHeadersMap.Add("deltacn", "DELTACN");
            //_columnHeadersMap.Add("deltacnstar", "DELTACNSTAR");
            //_columnHeadersMap.Add("spectrum", "SPECTRUM");
            //_columnHeadersMap.Add("spscore", "SPSCORE");
            //_columnHeadersMap.Add("ions2", "IONS2");
            //_columnHeadersMap.Add("peptide", "PEPTIDE");
            //_columnHeadersMap.Add("protein", "PROTEIN");
            //_columnHeadersMap.Add("xpress", "XPRESS");

            foreach (var searchResult in SearchResults)
            {
                var row = new List<string>();
                foreach (ColumnHeader column in resultsListView.Columns)
                {
                    var columnHeader = column.Text;
                    switch (columnHeader)
                    {
                        case "#":
                        case "INDEX":
                            var index = (TypedSearchResultField<int>)searchResult.Fields["index"];
                            row.Add(Convert.ToString(index.Value));
                            break;

                        case "ASSUMED_CHARGE":
                        case "Z":
                            var charge = (TypedSearchResultField<int>)searchResult.Fields["assumed_charge"];
                            row.Add(Convert.ToString(charge.Value));
                            break;

                        case "SPECTRUM":
                            var spectrum = (TypedSearchResultField<String>)searchResult.Fields["spectrum"];
                            row.Add(spectrum.Value);
                            break;

                        case "PROB":
                        case "PROBABILITY":
                            ISearchResultField probField;
                            if (searchResult.Fields.TryGetValue("probability", out probField))
                            {
                                var probability = (TypedSearchResultField<double>) probField;
                                row.Add(Convert.ToString(probability.Value));
                            }
                            else
                            {
                                row.Add(String.Empty);
                            }
                            break;

                        default:
                            row.Add(String.Empty);
                            break;
                    }
                }

                var item = new ListViewItem(row[0]);
                for (int i = 1; i < row.Count; i++)
                {
                    item.SubItems.Add(row[i]);
                }

                resultsListView.Items.Add(item);
            }

            resultsListView.EndUpdate();
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
                    if (null != searchHitNavigator)
                    {
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
                }

                SearchResults.Add(result);
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
            _condensedColumnHeadersMap.Add("index", "#");
            _condensedColumnHeadersMap.Add("assumed_charge", "Z");
            _condensedColumnHeadersMap.Add("precursor_neutral_mass", "EXP_MASS");
            _condensedColumnHeadersMap.Add("start_scan", "SSCAN");
            _condensedColumnHeadersMap.Add("calc_neutral_pep_mass", "CALC_MASS");

            //_columnHeadersMap.Add("spectrum", "SPECTRUM");
            //_columnHeadersMap.Add("retention_time_sec", "RETENTION_TIME_SEC");

            // Need to add peptide_prev_aa="R" peptide_next_aa="T" before and after, like the viewer now "R.EIAQDFK.T"
            //_columnHeadersMap.Add("peptide", "PEPTIDE");

            //_columnHeadersMap.Add("xcorr", "XCORR");
            //_columnHeadersMap.Add("deltacn", "DELTACN");
            //_columnHeadersMap.Add("deltacnstar", "DELTACNSTAR");
            //_columnHeadersMap.Add("spscore", "SPSCORE");
            // ADD THIS
            //_columnHeadersMap.Add("expect", "EXPECT");


            _condensedColumnHeadersMap.Add("probability", "PROB");

            // calc_neutral_pep_mass / assumed_charge (add protons and stuff, the usual calcs)
            //_columnHeadersMap.Add("MZratio", "MZRATIO");

            // calculated (isoelectric point - there is code floating around)
            //_columnHeadersMap.Add("pI", "PI");

            // This one is optional - if it were there, it would be in the spectrum query line
            //_columnHeadersMap.Add("precursor_intensity", "PRECURSOR_INTENSITY");

            // calculated - diff between theoretical and experimental masses - convert to ppm after taking diff
            //_columnHeadersMap.Add("ppm", "PPM");


            // num_matched_ions/tot_num_ions (ideally, it should have something like Vagisha's "lorikeet" tool to show graphical drawing) https://code.google.com/p/lorikeet/
            //_columnHeadersMap.Add("ions2", "IONS2");


            // There may be multiple protein hits for this field, so I need to create a new data structure to store this, or maybe use a string collection or something?
            //_columnHeadersMap.Add("protein", "PROTEIN");
            //_columnHeadersMap.Add("protein_descr", "PROTEIN_DESCR");

            //_columnHeadersMap.Add("xpress", "XPRESS");
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
    }

    public interface ISearchResultField
    {
        Type FieldType { get; }
    }

    public class TypedSearchResultField<T> : ISearchResultField
    {
        public new T Value { get; set; }

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
