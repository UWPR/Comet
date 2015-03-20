using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using BrightIdeasSoftware;
using CometUI.Properties;
using CometWrapper;
using ZedGraph;

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
        private SpectrumGraphUserOptions SpectrumGraphUserOptions { get; set; }

        private const String BlastHttpLink = "http://blast.ncbi.nlm.nih.gov/Blast.cgi?CMD=Web&LAYOUT=TwoWindows&AUTO_FORMAT=Semiauto&ALIGNMENTS=50&ALIGNMENT_VIEW=Pairwise&CDD_SEARCH=on&CLIENT=web&COMPOSITION_BASED_STATISTICS=on&DATABASE=nr&DESCRIPTIONS=100&ENTREZ_QUERY=(none)&EXPECT=1000&FILTER=L&FORMAT_OBJECT=Alignment&FORMAT_TYPE=HTML&I_THRESH=0.005&MATRIX_NAME=BLOSUM62&NCBI_GI=on&PAGE=Proteins&PROGRAM=blastp&SERVICE=plain&SET_DEFAULTS.x=41&SET_DEFAULTS.y=5&SHOW_OVERVIEW=on&END_OF_HTTPGET=Yes&SHOW_LINKOUT=yes&QUERY=";
        private const int DetailsPanelExtraHeight = 150;

        private const double DefaultMassTol = 0.5;

        public ViewSearchResultsControl(CometUI parent)
        {
            InitializeComponent();

            CometUI = parent;

            SearchResultsMgr = new SearchResultsManager();

            SpectrumGraphUserOptions = new SpectrumGraphUserOptions();

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

            ShowDetailsPanel(false);
   
            UpdateViewSearchResults(String.Empty);
        }

        public void UpdateViewSearchResults(String resultsPepXMLFile)
        {
            ShowDetailsPanel(false);

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

        private void ShowDetailsPanel(bool show)
        {
            detailsPanel.Visible = show;
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

                case "Ions":
                    OnIonsLinkClick(e);
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
            // Make sure the View Spectrum UI gets hidden first
            ShowViewSpectraUI(false);

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
                    ShowDetailsPanel(true);
                    ShowProteinSequenceUI(true);

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
                ShowProteinSequenceUI(false);
                ShowDetailsPanel(false);

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

        private void OnIonsLinkClick(HyperlinkClickedEventArgs e)
        {
            // Make sure the protein sequence UI gets hidden first
            ShowProteinSequenceUI(false); 

            var result = e.Model as SearchResult;
            if (null != result)
            {
                InitializeSpectrumGraphUserOptions();
                ViewSpectra(result);
            }
        }

        private void ViewSpectra(SearchResult result)
        {
            var msFileReaderWrapper = new MSFileReaderWrapper();
            var peaks = new List<Peak_T_Wrapper>();
            if (msFileReaderWrapper.ReadPeaks(SearchResultsMgr.SpectraFile, result.StartScan, SearchResultsMgr.SearchParams.MSLevel, peaks))
            {
                InitializeSpectrumGraph();

                ShowViewSpectraUI(true);
                ShowDetailsPanel(true);

                // Todo: Work in progress, need to:
                // 1) calculate and add ions to the graph
                // 2) draw the table of ions
                // 3) provide the user options for the graph

                DrawSpectrumGraph(result, peaks);
            }
            else
            {
                ShowViewSpectraUI(false);
                ShowDetailsPanel(false);

                if (DialogResult.Yes == MessageBox.Show(Resources.ViewSearchResultsControl_ViewSpectra_Could_not_read_the_spectra_file__Would_you_like_to_try_specifying_an_alternate_path_to_the_file_, 
                    Resources.ViewSearchResultsControl_ShowProteinSequence_View_Results_Error, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Error))
                {
                    var findSpectraFileDlg = new FindFileDlg
                    {
                        OpenFileDlgTitle = "Open Spectra File",
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

        private void InitializeSpectrumGraphUserOptions()
        {
            SpectrumGraphUserOptions.MassTol = DefaultMassTol;
            SpectrumGraphUserOptions.MassType = SearchResultsMgr.SearchParams.MassTypeFragment;
            SpectrumGraphUserOptions.NeutralLoss = MassSpecUtils.NeutralLoss.None;
            SpectrumGraphUserOptions.PeakLabel = PeakLabel.Ion;

            if (SearchResultsMgr.SearchParams.UseAIons)
            {
                SpectrumGraphUserOptions.UseIonsMap.Add(IonType.A, new Dictionary<IonCharge, int> { { IonCharge.Singly, 1 } });
            }

            if (SearchResultsMgr.SearchParams.UseBIons)
            {
                SpectrumGraphUserOptions.UseIonsMap.Add(IonType.B, new Dictionary<IonCharge, int> { { IonCharge.Singly, 1 } });
            }

            if (SearchResultsMgr.SearchParams.UseCIons)
            {
                SpectrumGraphUserOptions.UseIonsMap.Add(IonType.C, new Dictionary<IonCharge, int> { { IonCharge.Singly, 1 } });
            }

            if (SearchResultsMgr.SearchParams.UseXIons)
            {
                SpectrumGraphUserOptions.UseIonsMap.Add(IonType.X, new Dictionary<IonCharge, int> { { IonCharge.Singly, 1 } });
            }

            if (SearchResultsMgr.SearchParams.UseYIons)
            {
                SpectrumGraphUserOptions.UseIonsMap.Add(IonType.Y, new Dictionary<IonCharge, int> { { IonCharge.Singly, 1 } });
            }

            if (SearchResultsMgr.SearchParams.UseZIons)
            {
                SpectrumGraphUserOptions.UseIonsMap.Add(IonType.Z, new Dictionary<IonCharge, int> { { IonCharge.Singly, 1 } });
            }
        }

        private void InitializeSpectrumGraph()
        {
            InitializeIonCharge(aIonSinglyChargedCheckBox, IonCharge.Singly, IonType.A);
            InitializeIonCharge(aIonDoublyChargedCheckBox, IonCharge.Doubly, IonType.A);
            InitializeIonCharge(aIonTriplyChargedCheckBox, IonCharge.Triply, IonType.A);

            InitializeIonCharge(bIonSinglyChargedCheckBox, IonCharge.Singly, IonType.B);
            InitializeIonCharge(bIonDoublyChargedCheckBox, IonCharge.Doubly, IonType.B);
            InitializeIonCharge(bIonTriplyChargedCheckBox, IonCharge.Triply, IonType.B);

            InitializeIonCharge(cIonSinglyChargedCheckBox, IonCharge.Singly, IonType.C);
            InitializeIonCharge(cIonDoublyChargedCheckBox, IonCharge.Doubly, IonType.C);
            InitializeIonCharge(cIonTriplyChargedCheckBox, IonCharge.Triply, IonType.C);

            InitializeIonCharge(xIonSinglyChargedCheckBox, IonCharge.Singly, IonType.X);
            InitializeIonCharge(xIonDoublyChargedCheckBox, IonCharge.Doubly, IonType.X);
            InitializeIonCharge(xIonTriplyChargedCheckBox, IonCharge.Triply, IonType.X);

            InitializeIonCharge(yIonSinglyChargedCheckBox, IonCharge.Singly, IonType.Y);
            InitializeIonCharge(yIonDoublyChargedCheckBox, IonCharge.Doubly, IonType.Y);
            InitializeIonCharge(yIonTriplyChargedCheckBox, IonCharge.Triply, IonType.Y);

            InitializeIonCharge(zIonSinglyChargedCheckBox, IonCharge.Singly, IonType.Z);
            InitializeIonCharge(zIonDoublyChargedCheckBox, IonCharge.Doubly, IonType.Z);
            InitializeIonCharge(zIonTriplyChargedCheckBox, IonCharge.Triply, IonType.Z);


            massTypeAvgRadioButton.Checked = SpectrumGraphUserOptions.MassType == MassSpecUtils.MassType.Average;
            massTypeMonoRadioButton.Checked = SpectrumGraphUserOptions.MassType == MassSpecUtils.MassType.Monoisotopic;

            massTolTextBox.Text = Convert.ToString(SpectrumGraphUserOptions.MassTol);

            peakLabelIonRadioButton.Checked = SpectrumGraphUserOptions.PeakLabel == PeakLabel.Ion;
            peakLabelMzRadioButton.Checked = SpectrumGraphUserOptions.PeakLabel == PeakLabel.Mz;
            peakLabelNoneRadioButton.Checked = SpectrumGraphUserOptions.PeakLabel == PeakLabel.None;
        }

        private void InitializeIonCharge(CheckBox ionChargeCheckBox, IonCharge ionCharge, IonType ionType)
        {
            Dictionary<IonCharge, int> ionCharges;
            if (SpectrumGraphUserOptions.UseIonsMap.TryGetValue(ionType, out ionCharges))
            {
                int charge;
                if (ionCharges.TryGetValue(ionCharge, out charge))
                {
                    ionChargeCheckBox.Checked = true;
                }
            }
        }

        private void DrawSpectrumGraph(SearchResult result, IEnumerable<Peak_T_Wrapper> peaks)
        {
            // Todo: This is work in progress, there's lots left to do here!
            spectrumGraphItem.GraphPane.GraphObjList.Clear();

            GraphPane graphPane = spectrumGraphItem.GraphPane;

            // Set the titles and axis labels
            graphPane.Title.Text = result.Peptide + Environment.NewLine + SearchResultsMgr.ResultsPepXMLFile;
            graphPane.XAxis.Title.Text = "m/z";
            //graphPane.XAxis.Scale.Max = Convert.ToDouble(XMaxTextBox.Text);
            //graphPane.XAxis.Scale.Min = Convert.ToDouble(XMinTextBox.Text);
            graphPane.YAxis.Title.Text = "Intensity";
            //graphPane.YAxis.Scale.Min = Convert.ToDouble(YMinTextBox.Text);
            //graphPane.YAxis.Scale.Max = Convert.ToDouble(YMaxTextBox.Text);

            var peaksList = new PointPairList();
            foreach (var peak in peaks)
            {
                peaksList.Add(peak.get_mz(), peak.get_intensity());
            }

            // Draw the QValue plot
            graphPane.AddCurve(null, peaksList, Color.Black, SymbolType.None);

            // Calculate the Axis Scale Ranges
            spectrumGraphItem.AxisChange();

            // Redraw the whole graph control for smooth transition
            spectrumGraphItem.Invalidate();
        }

        private void HideDetailsPanelButtonClick(object sender, EventArgs e)
        {
            ShowProteinSequenceUI(false);
            ShowViewSpectraUI(false);
            ShowDetailsPanel(false);
        }

        private void ShowProteinSequenceUI(bool show)
        {
            databaseLabel.Visible = show;
            proteinSequenceTextBox.Visible = show;
        }

        private void ShowViewSpectraUI(bool show)
        {
            if (show && !viewSpectraSplitContainer.Visible)
            {
                GrowDetailsPanel();
            }

            if (!show && viewSpectraSplitContainer.Visible)
            {
                ShrinkDetailsPanel();
            }
            
            viewSpectraSplitContainer.Visible = show;
        }

        private void ShrinkDetailsPanel()
        {
            detailsPanel.Size = new Size(detailsPanel.Size.Width, detailsPanel.Size.Height - DetailsPanelExtraHeight);
        }

        private void GrowDetailsPanel()
        {
            detailsPanel.Size = new Size(detailsPanel.Size.Width, detailsPanel.Size.Height + DetailsPanelExtraHeight);
        }
    }

    public enum IonCharge
    {
        Singly = 1,
        Doubly,
        Triply
    }

    public enum PeakLabel
    {
        None = 0,
        Ion,
        Mz
    }

    public class SpectrumGraphUserOptions
    {
        public MassSpecUtils.MassType MassType { get; set; }
        public double MassTol { get; set; }
        public MassSpecUtils.NeutralLoss NeutralLoss { get; set; }
        public Dictionary<IonType, Dictionary<IonCharge, int>> UseIonsMap { get; set; }
        public PeakLabel PeakLabel { get; set; }

        //public bool UseAIons { get; set; }
        //public bool UseBIons { get; set; }
        //public bool UseCIons { get; set; }
        //public bool UseXIons { get; set; }
        //public bool UseYIons { get; set; }
        //public bool UseZIons { get; set; }

        public SpectrumGraphUserOptions()
        {
            UseIonsMap = new Dictionary<IonType, Dictionary<IonCharge, int>>();
            MassType = MassSpecUtils.MassType.Monoisotopic;
            MassTol = 0.5;
            NeutralLoss = MassSpecUtils.NeutralLoss.None;
            PeakLabel = PeakLabel.Ion;

            //UseAIons = false;
            //UseBIons = true;
            //UseCIons = false;
            //UseXIons = false;
            //UseYIons = true;
            //UseZIons = false;
        }
    }
}
