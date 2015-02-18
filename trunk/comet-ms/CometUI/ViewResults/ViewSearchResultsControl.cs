using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using BrightIdeasSoftware;
using CometUI.Properties;
using CometWrapper;

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
            resultsPanel.Location = resultsPanelNormal.Location;
            resultsPanel.Size = resultsPanelNormal.Size;
            showHideOptionsBtn.Text = Resources.ViewSearchResultsControl_ShowViewOptionsPanel__;
        }

        private void HideViewOptionsPanel()
        {
            showHideOptionsLabel.Text = Resources.ViewSearchResultsControl_HideViewOptionsPanel_Show_options;
            showOptionsPanel.Visible = false;
            hideOptionsGroupBox.Visible = true;
            OptionsPanelShown = false;
            resultsPanel.Location = resultsPanelFull.Location;
            resultsPanel.Size = resultsPanelFull.Size;
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
                resultsPanel.Show();
            }
            else
            {
                resultsPanel.Hide();
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

                case "Ions2":
                    OnIons2LinkClick(e);
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
            var result = e.Model as SearchResult;
            if (null != result)
            {
                ShowProteinSequence(result);
            }
        }

        private void ShowProteinSequence(SearchResult result)
        {
            try
            {
                var dbReader = new ProteinDBReader(SearchResultsMgr.SearchDatabaseFile);
                var proteinSequence = dbReader.ReadProtein(result.ProteinInfo.Name);
                if (null != proteinSequence)
                {
                    ShowProteinSequencePanel(true);

                    // Show the path of the protein database file
                    databaseLabel.Text = Resources.ViewSearchResultsControl_ShowProteinSequence_Database__
                        + SearchResultsMgr.SearchDatabaseFile;

                    // Show the protein
                    proteinSequenceTextBox.Text = proteinSequence;

                    // Highlight the matching peptide within the protein
                    int highlightStartIndex = proteinSequenceTextBox.Find(result.Peptide);
                    proteinSequenceTextBox.Select(highlightStartIndex, result.Peptide.Length);
                    proteinSequenceTextBox.SelectionBackColor = Color.Orange;
                    proteinSequenceTextBox.ShowSelectionMargin = true;
                }
            }
            catch (Exception exception)
            {
                if (DialogResult.Yes == MessageBox.Show(Resources.ViewSearchResultsControl_ShowProteinSequence_Could_not_find_the_protein_database_file__
                    + exception.Message
                    + Environment.NewLine + Environment.NewLine
                    + Resources.ViewSearchResultsControl_ShowProteinSequence_Would_you_like_to_specify_an_alternate_path_to_the_file_,
                                Resources.ViewSearchResultsControl_ShowProteinSequence_View_Results_Error, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Error))
                {
                    var findProteinDBDlg = new FindFileDlg
                                               {
                                                   OpenFileDlgTitle =
                                                       Resources.
                                                       ViewSearchResultsControl_Open_Protein_Database_File_Dlg_Title,
                                                       DlgTitle = "Find Protein DB File",
                                                       FileComboLabel = "Protein DB File:"
                                               };
                    if (DialogResult.OK == findProteinDBDlg.ShowDialog())
                    {
                        SearchResultsMgr.SearchDatabaseFile = findProteinDBDlg.FileName;

                        // As long as the user keeps specifying a valid new
                        // path to a protein database, recursively call into
                        // this method to try to display the protein sequence.
                        ShowProteinSequence(result);
                    }
                }
            }
        }

        private void HideProteinPanelButtonClick(object sender, EventArgs e)
        {
            ShowProteinSequencePanel(false);
        }

        private void OnIons2LinkClick(HyperlinkClickedEventArgs e)
        {
            var result = e.Model as SearchResult;
            if (null != result)
            {
                ViewSpectra(result);
            }
        }

        private void ViewSpectra(SearchResult result)
        {
            // Todo: This method is just for the purposes of testing the MSFileReaderWrapper for now.
            // Most of this functionality will move into a spectrum viewer

            var msFileReaderWrapper = new MSFileReaderWrapper();
            var peaks = new List<Peak_T_Wrapper>();
            if (!msFileReaderWrapper.ReadPeaks(SearchResultsMgr.SpectraFile, result.StartScan, SearchResultsMgr.MSLevel, peaks))
            {
                if (DialogResult.Yes == MessageBox.Show("Could not read the spectra file. Would you like to try specifying an alternate path to the file?",
            Resources.ViewSearchResultsControl_ShowProteinSequence_View_Results_Error, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Error))
                {
                    var findSpectraFileDlg = new FindFileDlg
                    {
                        OpenFileDlgTitle ="Open Spectra File",
                        DlgTitle = "Find Spectra File",
                        FileComboLabel = "Spectra File:"
                    };
                    if (DialogResult.OK == findSpectraFileDlg.ShowDialog())
                    {
                        SearchResultsMgr.SpectraFile = findSpectraFileDlg.FileName;

                        ViewSpectra(result);
                    }
                }
            }
        }
    }
}
