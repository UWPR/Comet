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
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            summaryTabPage.Controls.Add(ViewResultsSummaryOptionsControl);


            ViewResultsDisplayOptionsControl = new ViewResultsDisplayOptionsControl(this)
                                                   {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            displayOptionsTabPage.Controls.Add(ViewResultsDisplayOptionsControl);

            ViewResultsPickColumnsControl = new ViewResultsPickColumnsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
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
                UpdateColumnHeaders();

                if (!ViewResultsSummaryOptionsControl.UpdateSummaryOptions())
                {
                    MessageBox.Show(ViewResultsSummaryOptionsControl.ErrorMessage, Resources.ViewResults_View_Results_Title, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                }
                
                if (!UpdateSearchResultsList())
                {
                    MessageBox.Show(ErrorMessage, Resources.ViewResults_View_Results_Title, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                }
            }
        }

        public void SaveViewResultsSettings()
        {
            if (SettingsChanged)
            {
                CometUI.ViewResultsSettings.Save();
                SettingsChanged = false;
            }
        }

        public void UpdateColumnHeaders()
        {
            resultsListView.BeginUpdate();

            resultsListView.Columns.Clear();

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

            resultsListView.EndUpdate();
        }

        private bool UpdateSearchResultsList()
        {
            String resultsFile = ResultsPepXMLFile;
            if (String.Empty == resultsFile)
            {
                SearchResults.Clear();
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
                ErrorMessage = Resources.ViewSearchResultsControl_UpdateSearchResultsList_Could_not_read_the_results_pep_xml_file__ + e.Message;
                return false;
            }

            if (!ReadResults(pepXMLReader))
            {
                ErrorMessage = "Could not update the search results list. " + ErrorMessage;
                return false;
            }

            return true;
        }

        private bool ReadResults(PepXMLReader pepXMLReader)
        {
            var spectrumQueryNodes = pepXMLReader.ReadNodes("/msms_pipeline_analysis/msms_run_summary/spectrum_query");
            while (spectrumQueryNodes.MoveNext())
            {
                var spectrumQueryNavigator = spectrumQueryNodes.Current;
                var result = new SearchResult();

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

                var searchResultNodes = pepXMLReader.ReadChildren(spectrumQueryNavigator, "search_result");
                while (searchResultNodes.MoveNext())
                {
                    var searchResultNavigator = searchResultNodes.Current;
                    var searchHitNavigator = pepXMLReader.ReadFirstMatchingChild(searchResultNavigator, "search_hit");
                    if (null != searchHitNavigator)
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

                        int numProteins;
                        if (!pepXMLReader.ReadAttribute(searchHitNavigator, "num_tot_proteins", out numProteins))
                        {
                            ErrorMessage = "Could not read the num_tot_proteins attribute.";
                            return false;
                        }

                        if (numProteins > 1)
                        {
                            //var altProteinsList = new List<ProteinInfo>();
                            //var alternativeProteinNodes = pepXMLReader.ReadChildren(spectrumQueryNavigator, "alternative_protein");
                            //while (alternativeProteinNodes.MoveNext())
                            //{
                            //    var altProteinNavigator = alternativeProteinNodes.Current;
                            //    String altProteinName = pepXMLReader.ReadAttribute(altProteinNavigator, "protein");
                            //    if (altProteinName.Equals(String.Empty))
                            //    {
                            //        ErrorMessage = "Could not read the protein attribute.";
                            //        return false;
                            //    }

                            //    String altPeptidePrevAAA = pepXMLReader.ReadAttribute(altProteinNavigator, "peptide_prev_aa");
                            //    if (altPeptidePrevAAA.Equals(String.Empty))
                            //    {
                            //        ErrorMessage = "Could not read the peptide_prev_aa attribute.";
                            //        return false;
                            //    }

                            //    String altPeptideNextAAA = pepXMLReader.ReadAttribute(altProteinNavigator, "peptide_next_aa");
                            //    if (peptideNextAAA.Equals(String.Empty))
                            //    {
                            //        ErrorMessage = "Could not read the peptide_next_aa attribute.";
                            //        return false;
                            //    }

                            //    String altProteinDescr = pepXMLReader.ReadAttribute(altProteinNavigator, "protein_descr");

                            //    var altProteinInfo = new ProteinInfo(altProteinName, altProteinDescr, altPeptidePrevAAA, altPeptideNextAAA);
                            //    altProteinsList.Add(altProteinInfo);
                            //}

                            //var altProteinResultField = new TypedSearchResultField<List<ProteinInfo>>(altProteinsList);
                            //result.Fields.Add("alternative_protein", altProteinResultField);
                        }
                    }
                }

                SearchResults.Add(result);
            }

            return true;
        }

        //private bool ReadResults(PepXMLReader pepXMLReader)
        //{
        //    IEnumerable<XElement> spectrumQueryElements = pepXMLReader.ReadElements("spectrum_query").ToList();
        //    foreach (var element in spectrumQueryElements)
        //    {
        //        var result = new SearchResult();

        //        // All these fields are required
        //        if (!ResultFieldFromAttribute<String>("spectrum", element, pepXMLReader, result) ||
        //            !ResultFieldFromAttribute<int>("start_scan", element, pepXMLReader, result) ||
        //            !ResultFieldFromAttribute<int>("index", element, pepXMLReader, result) ||
        //            !ResultFieldFromAttribute<int>("assumed_charge", element, pepXMLReader, result) ||
        //            !ResultFieldFromAttribute<double>("precursor_neutral_mass", element, pepXMLReader, result) ||
        //            !ResultFieldFromAttribute<double>("retention_time_sec", element, pepXMLReader, result))
        //        {
        //            return false;
        //        }

        //        // The "precursor_intensity" field may or may not be there, 
        //        // so just ignore the return value.
        //        ResultFieldFromAttribute<double>("precursor_intensity", element, pepXMLReader, result);

        //        IEnumerable<XElement> searchResultElements = element.Descendants();
        //        var searchResultElement = searchResultElements.First();
        //        if (null == searchResultElement)
        //        {
        //            return false;
        //        }

        //        if (searchResultElement.HasElements)
        //        {
        //            IEnumerable<XElement> searchHitElements = searchResultElement.Descendants();
        //            var searchHitElement = searchHitElements.First();
        //            if (null != searchHitElement)
        //            {
        //                if (!ResultFieldFromAttribute<double>("calc_neutral_pep_mass", searchHitElement, pepXMLReader, result))
        //                {
        //                    return false;
        //                }

        //                if (!ResultFieldFromAttribute<String>("peptide", searchHitElement, pepXMLReader, result))
        //                {
        //                    return false;
        //                }


        //                // Todo: need to create a LIST of proteins for cases where we have multiple proteins
        //                // Then, set up a scenario where when the user hovers with the mouse over the protein
        //                // name, pop up a box with a list of all the proteins.
        //                //XAttribute proteinAttribute = pepXMLReader.ReadFirstAttribute(searchHitElement, "protein");
        //                //if (null == proteinAttribute)
        //                //{
        //                //    return false;
        //                //}
        //                //var proteinName = (String)proteinAttribute;

        //                //XAttribute peptidePrevAAAttribute = pepXMLReader.ReadFirstAttribute(searchHitElement, "peptide_prev_aa");
        //                //if (null == peptidePrevAAAttribute)
        //                //{
        //                //    return false;
        //                //}
        //                //var prevAA = (String)peptidePrevAAAttribute;

        //                //XAttribute peptideNextAAAttribute = pepXMLReader.ReadFirstAttribute(searchHitElement, "peptide_next_aa");
        //                //if (null == peptideNextAAAttribute)
        //                //{
        //                //    return false;
        //                //}
        //                //var nextAA = (String)peptideNextAAAttribute;

        //                //var proteinDescr = String.Empty;
        //                //XAttribute proteinDescrAttribute = pepXMLReader.ReadFirstAttribute(searchHitElement, "protein_descr");
        //                //if (null != proteinDescrAttribute)
        //                //{
        //                //    proteinDescr = (String)proteinDescrAttribute;
        //                //}

        //                //var proteinResultField =
        //                //    new TypedSearchResultField<ProteinInfo>(new ProteinInfo(proteinName, proteinDescr, prevAA,
        //                //                                                            nextAA));
        //                //result.Fields.Add("protein", proteinResultField);

        //                IEnumerable<XElement> searchScoreElements = searchHitElement.Descendants();
        //                foreach (var searchScoreElement in searchScoreElements)
        //                {
        //                    XAttribute nameAttribute = pepXMLReader.ReadFirstAttribute(searchScoreElement, "name");
        //                    if (null == nameAttribute)
        //                    {
        //                        return false;
        //                    }
        //                    var name = (String)nameAttribute;

        //                    // We want all the scores except sprank
        //                    if (!name.Equals("sprank"))
        //                    {
        //                        XAttribute valueAttribute = pepXMLReader.ReadFirstAttribute(searchScoreElement, "value");
        //                        if (null == valueAttribute)
        //                        {
        //                            return false;
        //                        }
        //                        var value = (double)valueAttribute;

        //                        result.Fields.Add(name, new TypedSearchResultField<double>(value));
        //                    }
        //                }

        //            }
        //        }

        //        SearchResults.Add(result);
        //    }

        //    return true;
        //}

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
