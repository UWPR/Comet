using System;
using System.Drawing;
using System.Windows.Forms;
using BrightIdeasSoftware;
using CometUI.Properties;

namespace CometUI.ViewResults
{
    public partial class ViewSearchResultsControl : UserControl
    {
        public bool SettingsChanged { get; set; }
        public String ErrorMessage { get; private set; }

        private CometUI CometUI { get; set; }
        private bool OptionsPanelShown { get; set; }
        private ViewResultsSummaryOptionsControl ViewResultsSummaryOptionsControl { get; set; }
        private ViewResultsDisplayOptionsControl ViewResultsDisplayOptionsControl { get; set; }
        private ViewResultsPickColumnsControl ViewResultsPickColumnsControl { get; set; }
        private SearchResultsManager SearchResultsMgr { get; set; }

        private const String BlastHttpLink = "http://blast.ncbi.nlm.nih.gov/Blast.cgi?CMD=Web&LAYOUT=TwoWindows&AUTO_FORMAT=Semiauto&ALIGNMENTS=50&ALIGNMENT_VIEW=Pairwise&CDD_SEARCH=on&CLIENT=web&COMPOSITION_BASED_STATISTICS=on&DATABASE=nr&DESCRIPTIONS=100&ENTREZ_QUERY=(none)&EXPECT=1000&FILTER=L&FORMAT_OBJECT=Alignment&FORMAT_TYPE=HTML&I_THRESH=0.005&MATRIX_NAME=BLOSUM62&NCBI_GI=on&PAGE=Proteins&PROGRAM=blastp&SERVICE=plain&SET_DEFAULTS.x=41&SET_DEFAULTS.y=5&SHOW_OVERVIEW=on&END_OF_HTTPGET=Yes&SHOW_LINKOUT=yes&QUERY=";

        public ViewSearchResultsControl(CometUI parent)
        {
            InitializeComponent();

            CometUI = parent;

            SearchResultsMgr = new SearchResultsManager();

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

            proteinSequencePanel.Visible = false;

            UpdateViewSearchResults(String.Empty);
        }

        public void UpdateViewSearchResults(String resultsPepXMLFile)
        {
            ShowProteinSequencePanel(false);

            ErrorMessage = String.Empty;
            if (null != resultsPepXMLFile)
            {
                ShowResultsListPanel(String.Empty != resultsPepXMLFile);
                
                if (!ViewResultsSummaryOptionsControl.UpdateSummaryOptions(resultsPepXMLFile))
                {
                    ErrorMessage = SearchResultsMgr.ErrorMessage;
                    MessageBox.Show(ViewResultsSummaryOptionsControl.ErrorMessage,
                                    Resources.ViewResults_View_Results_Title, MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                }

                if (!SearchResultsMgr.UpdateResults(resultsPepXMLFile))
                {
                    ErrorMessage = SearchResultsMgr.ErrorMessage;
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
            resultsListView.SetObjects(SearchResultsMgr.SearchResults);
            resultsListView.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            resultsListView.EndUpdate();
        }

        private void UpdateColumnHeaders()
        {
            resultsListView.Columns.Clear();

            foreach (var item in CometUI.ViewResultsSettings.PickColumnsShowList)
            {
                SearchResultColumn resultCol;
                if (SearchResultsMgr.ResultColumns.TryGetValue(item, out resultCol))
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

        private void ShowProteinSequencePanel(bool show)
        {
            proteinSequencePanel.Visible = show;
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
                    var dbReader = new SearchDBReader(SearchResultsMgr.SearchDatabaseFile);
                    var proteinSequence = dbReader.ReadProtein(result.ProteinInfo.Name);
                    if (null != proteinSequence)
                    {
                        databaseLabel.Text = "Database: " + SearchResultsMgr.SearchDatabaseFile;
                        proteinSequenceTextBox.Text = proteinSequence;
                        ShowProteinSequencePanel(true);
                        int highlightStartIndex = proteinSequenceTextBox.Find(result.Peptide);
                        proteinSequenceTextBox.Select(highlightStartIndex, result.Peptide.Length);
                        proteinSequenceTextBox.SelectionBackColor = Color.Orange;
                        proteinSequenceTextBox.ShowSelectionMargin = true;
                    }
                }
                catch (Exception exception)
                {
                    MessageBox.Show("Could not look up the protein sequence. " + exception.Message,
                                    "View Results Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void HideProteinPanelButtonClick(object sender, EventArgs e)
        {
            ShowProteinSequencePanel(false);
        }
    }
}
