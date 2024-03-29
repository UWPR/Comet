﻿/*
   Copyright 2015 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using BrightIdeasSoftware;
using CometUI.Properties;
using CometUI.SharedUI;
using CometWrapper;
using ZedGraph;

namespace CometUI.ViewResults
{
    public partial class ViewSearchResultsControl : UserControl
    {
        public String ResultsFile { get; set; }
        public String DecoyPrefix { get; set; }
        public bool SettingsChanged { get; set; }
        public String ErrorMessage { get; private set; }
        public ViewResultsSummaryOptionsControl ViewResultsSummaryOptionsControl { get; set; }
        public bool HasSearchResults { get { return SearchResults.Count > 0; } }
        
        private int NumResultsShown { get { return SearchResults.Count; } }
        private SearchResultsManager SearchResultsMgr { get; set; }
        private int TotalNumResults { get { return SearchResultsMgr.SearchResults.Count; } }
        private List<SearchResult> SearchResults { get; set; }
        private CometUIMainForm CometUIMainForm { get; set; }
        private bool OptionsPanelShown { get; set; }
        private ViewResultsDisplayOptionsControl ViewResultsDisplayOptionsControl { get; set; }
        private ViewResultsPickColumnsControl ViewResultsPickColumnsControl { get; set; }
        private ViewResultsOtherActionsControl ViewResultsOtherActionsControl { get; set; }
        private SearchResult ViewSpectraSearchResult { get; set; }
        private SpectrumGraphUserOptions SpectrumGraphUserOptions { get; set; }
        private List<Peak_T_Wrapper> Peaks { get; set; }
        private PeptideFragmentCalculator IonCalculator { get; set; }
        private List<TextObj> SpectrumGraphPeakLabels { get; set; }
        private List<TextObj> PrecursorGraphPeakLabels { get; set; }
        private Dictionary<IonType, FragmentIonGraphInfo> FragmentIonPeaks { get; set; }
        private PointPairList SpectrumGraphPeaksList { get; set; }
        private PointPairList PrecursorGraphPeaksList { get; set; }

        private const String BlastHttpLink =
            "http://blast.ncbi.nlm.nih.gov/Blast.cgi?CMD=Web&LAYOUT=TwoWindows&AUTO_FORMAT=Semiauto&ALIGNMENTS=50&ALIGNMENT_VIEW=Pairwise&CDD_SEARCH=on&CLIENT=web&COMPOSITION_BASED_STATISTICS=on&DATABASE=nr&DESCRIPTIONS=100&ENTREZ_QUERY=(none)&EXPECT=1000&FILTER=L&FORMAT_OBJECT=Alignment&FORMAT_TYPE=HTML&I_THRESH=0.005&MATRIX_NAME=BLOSUM62&NCBI_GI=on&PAGE=Proteins&PROGRAM=blastp&SERVICE=plain&SET_DEFAULTS.x=41&SET_DEFAULTS.y=5&SHOW_OVERVIEW=on&END_OF_HTTPGET=Yes&SHOW_LINKOUT=yes&QUERY=";

        private const int MaxIonCharge = 3;


        public ViewSearchResultsControl(CometUIMainForm parent)
        {
            InitializeComponent();

            CometUIMainForm = parent;

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

            ViewResultsOtherActionsControl = new ViewResultsOtherActionsControl(this)
            {
                Anchor =
                    (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left |
                     AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            otherActionsTabPage.Controls.Add(ViewResultsOtherActionsControl);

            InitializeFromDefaultSettings();

            HideDetailsPanel();

            ResultsFile = String.Empty;
            DecoyPrefix = SearchSettings.Default.DecoyPrefix;

            UpdateViewSearchResults(String.Empty, String.Empty);
        }

        public void UpdateViewSearchResults(String resultsPepXMLFile, String decoyPrefix)
        {
            HideDetailsPanel();
            HideResultsListPanel();

            DecoyPrefix = decoyPrefix;

            ErrorMessage = String.Empty;

            ResultsFile = resultsPepXMLFile;
            if (null != ResultsFile)
            {
                if (String.Empty == ResultsFile)
                {
                    ClearResults();
                }
                else
                {
                    // We have a valid results file, so launch the results 
                    // loader to load the file into the UI. Note that we need
                    // to disable the options panel to make sure the user
                    // doesn't try to load another set of results while we're
                    // already trying to load one set.
                    DisableViewOptionsPanel();
                    var viewResultsWorker = new ViewResultsBackgroundWorker(this);
                    viewResultsWorker.DoWork();
                }
            }
        }

        public bool ReadResults()
        {
            bool res = SearchResultsMgr.UpdateResults(ResultsFile, DecoyPrefix);
            if (!res)
            {
                ErrorMessage = SearchResultsMgr.ErrorMessage;
            }

            return res;
        }

        public void UpdateResultsUI()
        {
            SearchResults = SearchResultsMgr.SearchResults;

            // We *MUST* call this AFTER calling "SearchResultMgr.UpdateResults".
            // Otherwise, the SearchResultsMgr.ResultsFileReader we use later will reflect 
            // the OLD results file, NOT the updated one.
            if (!ViewResultsSummaryOptionsControl.UpdateSummaryOptions(ResultsFile, SearchResultsMgr.ResultsFileReader))
            {
                ErrorMessage = SearchResultsMgr.ErrorMessage;
                MessageBox.Show(ViewResultsSummaryOptionsControl.ErrorMessage,
                                Resources.ViewResults_View_Results_Title, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
            }

            if (ViewResultsSummaryOptionsControl.eValueCheckBox.Checked)
            {
                double dEvalueCutoff = Convert.ToDouble(ViewResultsSummaryOptionsControl.textBoxEValueCutoff.Text);
                SearchResults = SearchResultsMgr.ApplyEValueCutoff(dEvalueCutoff);
            }

            UpdateSearchResultsList();

            if (HasSearchResults)
            {
                ShowResultsListPanel();
            }

            EnableViewOptionsPanel();
        }

        public void ClearResults()
        {
            ResultsFile = String.Empty;
            SearchResultsMgr.ClearResults();
            SearchResults = SearchResultsMgr.SearchResults;
            ViewResultsSummaryOptionsControl.ClearSummaryOptions();
            EnableViewOptionsPanel();
        }

        public void SaveViewResultsSettings()
        {
            if (SettingsChanged)
            {
                CometUIMainForm.ViewResultsSettings.Save();
                SettingsChanged = false;
            }
        }

        public void ExportResultsList()
        {
            if (!HasSearchResults)
            {
                MessageBox.Show(Resources.ViewSearchResultsControl_ExportResultsList_No_results_to_export_,
                    Resources.ViewSearchResultsControl_ResultsListContextMenuItemExportClick_Export_Search_Results,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            var exportFileDlg = new ExportFileDlg
            {
                FileExtension = ".txt",
                DlgTitle = Resources.ViewSearchResultsControl_ResultsListContextMenuItemExportClick_Export_Search_Results,
                FileNameText = SearchResultsMgr.ResultsFileBaseName,
                FilePathText = SearchResultsMgr.ResultsFilePath
            };
            if (exportFileDlg.ShowDialog() == DialogResult.OK)
            {
                var resultsExporter = new ExportSearchResults();
                resultsExporter.Export(resultsFastObjectListView, exportFileDlg.FileFullPath);
                MessageBox.Show(Resources.ViewSearchResultsControl_ResultsListContextMenuItemExportClick_Search_results_exported_to_ + exportFileDlg.FileFullPath,
                    Resources.ViewSearchResultsControl_ResultsListContextMenuItemExportClick_Export_Search_Results,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
        }

        public void FilterResultsListByQValue(double cutoffValue, bool showDecoyHits)
        {
            SearchResults = SearchResultsMgr.ApplyFDRCutoff(cutoffValue, showDecoyHits);
            UpdateSearchResultsList();
        }

        public void UpdateSearchResultsList()
        {
            resultsFastObjectListView.BeginUpdate();
            resultsFastObjectListView.Clear();
            UpdateColumnHeaders();
            resultsFastObjectListView.SetObjects(SearchResults);
            
            resultsFastObjectListView.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
          
            SortByExpectColumn(); //JKE
            
            resultsFastObjectListView.EndUpdate();
            UpdateResultsListSummaryText();
        }

        public void SortByExpectColumn()
        {
           foreach (OLVColumn item in resultsFastObjectListView.Columns)
           {
              //MessageBox.Show("item.Text");
              if (item.Text.ToUpper().Equals("SSCAN"))
              {
                 item.Width = 60;
              }
              else if (item.Text.ToUpper().Equals("EXPECT"))
              {
                 resultsFastObjectListView.Sort(item, SortOrder.Ascending);
                 item.Width = 70;
              }
              else if (item.Text.ToUpper().Equals("IONS"))
              {
                 item.Width = 60;
              }
           }
        }

        private void UpdateResultsListSummaryText()
        {
            var resultsListSummary = String.Empty;
            if (TotalNumResults > 0)
            {
                resultsListSummary = "Displaying " + NumResultsShown + " of " +
                                                                          TotalNumResults + " total spectra.";
            }

            ViewResultsSummaryOptionsControl.ResultsListSummaryText = resultsListSummary;
        }

        private void UpdateColumnHeaders()
        {
            resultsFastObjectListView.Columns.Clear();

            foreach (var item in CometUIMainForm.ViewResultsSettings.PickColumnsShowList)
            {
                SearchResultColumn resultCol;
                if (SearchResultsMgr.ResultColumns.TryGetValue(item.ToLower(), out resultCol))
                {
                    String columnHeader;
                    if (CometUIMainForm.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
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

                    resultsFastObjectListView.Columns.Add(olvColumn);
                }
            }
        }

        private void InitializeFromDefaultSettings()
        {
            if (CometUIMainForm.ViewResultsSettings.ShowOptions)
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

        private void DisableViewOptionsPanel()
        {
            showOptionsPanel.Enabled = false;
        }

        private void EnableViewOptionsPanel()
        {
            showOptionsPanel.Enabled = true;
        }

        private void ShowHideOptionsBtnClick(object sender, EventArgs e)
        {
            SettingsChanged = true;

            if (OptionsPanelShown)
            {
                HideViewOptionsPanel();
                CometUIMainForm.ViewResultsSettings.ShowOptions = false;
            }
            else
            {
                ShowViewOptionsPanel();
                CometUIMainForm.ViewResultsSettings.ShowOptions = true;
            }
        }

        private void ShowResultsListPanel()
        {
            resultsPanel.Show();
        }

        private void HideResultsListPanel()
        {
            resultsPanel.Hide();
        }

        private void ShowDetailsPanel()
        {
            if (!detailsPanel.Visible)
            {
                resultsSubPanelSplitContainer.SplitterDistance = resultsSubPanelSplitContainer.Height / 4;
                resultsSubPanelSplitContainer.Panel2.Visible = true;
                resultsSubPanelSplitContainer.IsSplitterFixed = false;
                resultsSubPanelSplitContainer.FixedPanel = FixedPanel.None;
                detailsPanel.Visible = true;
            }
        }

        private void HideDetailsPanel()
        {
            resultsSubPanelSplitContainer.SplitterDistance = resultsSubPanelSplitContainer.Height;
            resultsSubPanelSplitContainer.Panel2.Visible = false;
            resultsSubPanelSplitContainer.IsSplitterFixed = true;
            resultsSubPanelSplitContainer.FixedPanel = FixedPanel.Panel2;
            detailsPanel.Visible = false;
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
            HideViewSpectraUI();

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
                    ShowDetailsPanel();
                    ShowProteinSequenceUI();

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
                HideProteinSequenceUI();
                HideDetailsPanel();

                if (DialogResult.Yes ==
                    MessageBox.Show(
                        Resources.
                            ViewSearchResultsControl_ShowProteinSequence_Could_not_find_the_protein_database_file__
                        + exception.Message
                        + Environment.NewLine + Environment.NewLine
                        +
                        Resources.
                            ViewSearchResultsControl_ShowProteinSequence_Would_you_like_to_specify_an_alternate_path_to_the_file_,
                        Resources.ViewSearchResultsControl_ShowProteinSequence_View_Results_Error,
                        MessageBoxButtons.YesNoCancel, MessageBoxIcon.Error))
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
            HideProteinSequenceUI();

            var result = e.Model as SearchResult;
            if (null != result)
            {
                ViewSpectraSearchResult = result;
                ViewSpectra();
            }
        }

        private void HideDetailsPanelButtonClick(object sender, EventArgs e)
        {
            HideProteinSequenceUI();
            HideViewSpectraUI();
            HideDetailsPanel();
        }

        private void ShowProteinSequenceUI()
        {
            databaseLabel.Visible = true;
            proteinSequenceTextBox.Visible = true;
        }

        private void HideProteinSequenceUI()
        {
            databaseLabel.Visible = false;
            proteinSequenceTextBox.Visible = false;
        }

        private void ShowViewSpectraUI()
        {
            viewSpectraSplitContainer.Visible = true;
        }

        private void HideViewSpectraUI()
        {
            viewSpectraSplitContainer.Visible = false;
        }

        private void ViewSpectra()
        {
            if (null == IonCalculator)
            {
                IonCalculator = new PeptideFragmentCalculator();
            }

            if (null == Peaks)
            {
                Peaks = new List<Peak_T_Wrapper>();
            }
            else
            {
                Peaks.Clear();
            }

            var msFileReader = new MSFileReaderWrapper();
            if (msFileReader.ReadPeaks(SearchResultsMgr.SpectraFile, ViewSpectraSearchResult.StartScan,
                                              SearchResultsMgr.SearchParams.MSLevel, Peaks))
            {
                InitializeSpectrumGraphOptions();
                ShowViewSpectraUI();
                ShowDetailsPanel();
                UpdateViewSpectra();
            }
            else
            {
                HideViewSpectraUI();
                HideDetailsPanel();

                if (DialogResult.Yes ==
                    MessageBox.Show(
                        Resources.
                            ViewSearchResultsControl_ViewSpectra_Could_not_read_the_spectra_file__Would_you_like_to_try_specifying_an_alternate_path_to_the_file_,
                        Resources.ViewSearchResultsControl_ShowProteinSequence_View_Results_Error,
                        MessageBoxButtons.YesNoCancel, MessageBoxIcon.Error))
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

                        ViewSpectra();
                    }
                }
            }
        }

        private void InitializeSpectrumGraphOptions()
        {
           if (SearchResultsMgr.SearchParams.UseAIons)
           {
              aIonSinglyChargedCheckBox.Checked = true;
              aIonDoublyChargedCheckBox.Checked = false;
              aIonTriplyChargedCheckBox.Checked = false;
              if (ViewSpectraSearchResult.AssumedCharge > 2)
              {
                 aIonDoublyChargedCheckBox.Checked = true;
              }
           }
           if (SearchResultsMgr.SearchParams.UseBIons)
           {
              bIonSinglyChargedCheckBox.Checked = true;
              bIonDoublyChargedCheckBox.Checked = false;
              bIonTriplyChargedCheckBox.Checked = false;
              if (ViewSpectraSearchResult.AssumedCharge > 2)
              {
                 bIonDoublyChargedCheckBox.Checked = true;
              }
           }
           if (SearchResultsMgr.SearchParams.UseCIons)
           {
              cIonSinglyChargedCheckBox.Checked = true;
              cIonDoublyChargedCheckBox.Checked = false;
              cIonTriplyChargedCheckBox.Checked = false;
              if (ViewSpectraSearchResult.AssumedCharge > 2)
              {
                 cIonDoublyChargedCheckBox.Checked = true;
              }
           }
           if (SearchResultsMgr.SearchParams.UseXIons)
           {
              xIonSinglyChargedCheckBox.Checked = true;
              xIonDoublyChargedCheckBox.Checked = false;
              xIonTriplyChargedCheckBox.Checked = false;
              if (ViewSpectraSearchResult.AssumedCharge > 2)
              {
                 xIonDoublyChargedCheckBox.Checked = true;
              }
           }
           if (SearchResultsMgr.SearchParams.UseYIons)
           {
              yIonSinglyChargedCheckBox.Checked = true;
              yIonDoublyChargedCheckBox.Checked = false;
              yIonTriplyChargedCheckBox.Checked = false;
              if (ViewSpectraSearchResult.AssumedCharge > 2)
              {
                 yIonDoublyChargedCheckBox.Checked = true;
              }
           }
           if (SearchResultsMgr.SearchParams.UseZIons)
           {
              zIonSinglyChargedCheckBox.Checked = true;
              zIonDoublyChargedCheckBox.Checked = false;
              zIonTriplyChargedCheckBox.Checked = false;
              if (ViewSpectraSearchResult.AssumedCharge > 2)
              {
                 zIonDoublyChargedCheckBox.Checked = true;
              }
           }

            if (MassSpecUtils.MassType.Average == SearchResultsMgr.SearchParams.MassTypeFragment)
            {
                massTypeAvgRadioButton.Checked = true;
            }

            if (MassSpecUtils.MassType.Monoisotopic == SearchResultsMgr.SearchParams.MassTypeFragment)
            {
                massTypeMonoRadioButton.Checked = true;
            }

            if (SearchResultsMgr.SearchParams.FragmentBinSize <= 0.1)
               massTolTextBox.Text = "0.05";
            else
               massTolTextBox.Text = SpectrumGraphUserOptions.MassTol.ToString();

            peakLabelIonRadioButton.Checked = true;
        }

        private void UpdateViewSpectra()
        {
            IonCalculator.CalculateIons(ViewSpectraSearchResult, SpectrumGraphUserOptions);
            UpdateIonTable();

            DrawSpectrumGraph();

            // Don't show the precursor graph for spectra files that do NOT
            // contain MS1 scans.
            if (NoMs1ScansInSpectraFile())
            {
                HidePrecursorGraph();
            }
            else
            {
                DrawPrecursorGraph();
                ShowPrecursorGraph();
            }
        }

        private void UpdateIonTable()
        {
            spectrumGraphIonsTable.BeginUpdate();
            spectrumGraphIonsTable.Clear();

            for (var i = (int)IonType.A; i <= (int)IonType.C; i++)
            {
                for (int j = 0; j <= MaxIonCharge; j++)
                {
                    AddIonTableColumn((IonType)i, j);
                }
            }

            spectrumGraphIonsTable.Columns.Add(new OLVColumn("#", "BIonCounter") { TextAlign = HorizontalAlignment.Center });
            spectrumGraphIonsTable.Columns.Add(new OLVColumn(".", "AA") { TextAlign = HorizontalAlignment.Center });  //JKE
            spectrumGraphIonsTable.Columns.Add(new OLVColumn("#", "YIonCounter") { TextAlign = HorizontalAlignment.Center });

            for (var i = (int)IonType.X; i <= (int)IonType.Z; i++)
            {
                for (int j = 0; j <= MaxIonCharge; j++)
                {
                    AddIonTableColumn((IonType)i, j);
                }
            }

            spectrumGraphIonsTable.SetObjects(IonCalculator.FragmentIonRows);
            spectrumGraphIonsTable.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            spectrumGraphIonsTable.EndUpdate();
        }

        private void AddIonTableColumn(IonType ionType, int charge)
        {
            List<int> ionCharges;
            if (SpectrumGraphUserOptions.UseIonsMap.TryGetValue(ionType, out ionCharges))
            {
                if (ionCharges.Contains(charge))
                {
                    var ionTypeStr = IonCalculator.IonTypeTable[ionType];
                    var header = String.Format("{0}{1}+", ionTypeStr, charge);
                    var aspect = ionTypeStr.ToUpper() + IonCalculator.IonChargeTable[charge] + "ChargedIonMass";
                    spectrumGraphIonsTable.Columns.Add(new OLVColumn(header, aspect) { TextAlign = HorizontalAlignment.Right, HeaderTextAlign = HorizontalAlignment.Center });
                    spectrumGraphIonsTable.OwnerDraw = true;
                    spectrumGraphIonsTable.HeaderUsesThemes = false;
                    //spectrumGraphIonsTable.HeaderFormatStyle.SetBackColor(Color.Aqua); 
                }
            }
        }

        private void DrawSpectrumGraph()
        {
            InitializeSpectrumGraph();

            CreateSpectrumGraphPeakLists();

            // Plot the fragment ion peaks (Note: Do this BEFORE plotting 
            // the regular peaks, otherwise the regular peaks show up over
            // the colored peaks when they are overlayed on top of each other)
            foreach (var fragmentIon in FragmentIonPeaks)
            {
                var fragmentIonGraphInfo = fragmentIon.Value;
                if (fragmentIonGraphInfo.FragmentIonPeaks.Count > 0)
                {
                    spectrumGraphItem.GraphPane.AddStick(null, fragmentIonGraphInfo.FragmentIonPeaks, fragmentIonGraphInfo.PeakColor);
                }
            }

            // Plot the regular peaks
            spectrumGraphItem.GraphPane.AddStick(null, SpectrumGraphPeaksList, Color.LightGray);

            // Draw the peak labels for the spectrum graph
            foreach (var peakLabel in SpectrumGraphPeakLabels)
            {
                spectrumGraphItem.GraphPane.GraphObjList.Add(peakLabel);
            }

            // Calculate the Axis Scale Ranges and redraw the whole graph 
            // control for smooth transition
            spectrumGraphItem.AxisChange();
            spectrumGraphItem.Invalidate();
            spectrumGraphItem.Refresh();
        }

        private void InitializeSpectrumGraph()
        {
            if (null == SpectrumGraphPeakLabels)
            {
                SpectrumGraphPeakLabels = new List<TextObj>();
            }
            else
            {
                SpectrumGraphPeakLabels.Clear();
            }

            if (null == SpectrumGraphPeaksList)
            {
                SpectrumGraphPeaksList = new PointPairList();
            }
            else
            {
                SpectrumGraphPeaksList.Clear();
            }

            FragmentIonPeaks = new Dictionary<IonType, FragmentIonGraphInfo>
                                {
                                        {IonType.A, new FragmentIonGraphInfo(new PointPairList(), Color.LawnGreen)},
                                        {IonType.B, new FragmentIonGraphInfo(new PointPairList(), Color.Blue)},
                                        {IonType.C, new FragmentIonGraphInfo(new PointPairList(), Color.DeepSkyBlue)},
                                        {IonType.X, new FragmentIonGraphInfo(new PointPairList(), Color.Fuchsia)},
                                        {IonType.Y, new FragmentIonGraphInfo(new PointPairList(), Color.Red)},
                                        {IonType.Z, new FragmentIonGraphInfo(new PointPairList(), Color.Orange)}
                                };

            GraphPane spectrumGraphPane = spectrumGraphItem.GraphPane;

            spectrumGraphPane.CurveList.Clear();
            spectrumGraphPane.GraphObjList.Clear();

            // Set the spectrum graph title
            var expMz = MassSpecUtils.CalculateMz(ViewSpectraSearchResult.ExperimentalMass,
                                                  ViewSpectraSearchResult.AssumedCharge);
            var titleStrSecondLine = String.Format("{0}, Scan: {1}, Exp. m/z: {2}, Charge: {3}",
                SearchResultsMgr.ResultsFile,
                ViewSpectraSearchResult.StartScan,
                Math.Round(expMz, 4),
                ViewSpectraSearchResult.AssumedCharge);
            spectrumGraphPane.Title.Text = ViewSpectraSearchResult.Peptide + Environment.NewLine + titleStrSecondLine;
            var border = new Border { IsVisible = false };
            var fill = new Fill { IsVisible = false };
            spectrumGraphPane.Title.FontSpec = new FontSpec { Size = 12, Border = border , Fill =  fill};

            //  Set the axis labels for the spectrum graph
            spectrumGraphPane.XAxis.Title.Text = "m/z";
            spectrumGraphPane.YAxis.Title.Text = "Intensity";

            // Set the spectrum graph axis scale
            spectrumGraphPane.XAxis.Scale.Min = 0.0;
            spectrumGraphPane.YAxis.Scale.Min = 0.0;
        }

        private void CreateSpectrumGraphPeakLists()
        {
            var topIntensities = MassSpecUtils.GetTopIntensities(Peaks, 50);
            for (int i = 0; i < Peaks.Count; i++)
            {
                var peak = new Peak(Peaks[i]);
                bool isFragmentIon = false;

                // Ignore the peaks with zero intensity
                if (peak.Intensity.Equals(0.0))
                {
                    continue;
                }

                // Ensure the peak is actually a peak, not merely noise
                if (MassSpecUtils.PickPeak(Peaks, i, peak, topIntensities))
                {
                    // Check to see if any of the fragment ions we're supposed to 
                    // show match this peak
                    foreach (var fragmentIon in IonCalculator.FragmentIons)
                    {
                        if (fragmentIon.Show &&
                            MassSpecUtils.IsPeakPresent(peak.Mz, fragmentIon.Mass, (double)massTolTextBox.DecimalValue))
                        {
                            FragmentIonPeaks[fragmentIon.Type].FragmentIonPeaks.Add(peak.Mz, peak.Intensity);
                            AddPeakLabel(fragmentIon.Label, FragmentIonPeaks[fragmentIon.Type].PeakColor, peak.Mz, peak.Intensity);
                            isFragmentIon = true;
                            break;
                        }
                    }
                }

                // If this peak doesn't match any of the fragment ions we're 
                // supposed to show, then it's just a regular peak to plot.
                if (!isFragmentIon)
                {
                    SpectrumGraphPeaksList.Add(peak.Mz, peak.Intensity);
                }
            }
        }

        private bool NoMs1ScansInSpectraFile()
        {
            var spectraFileExtension = Path.GetExtension(SearchResultsMgr.SpectraFile);
            spectraFileExtension = null == spectraFileExtension ? String.Empty : spectraFileExtension.ToLower();
            return spectraFileExtension.Equals(".mgf") || spectraFileExtension.Equals(".ms2") ||
                   spectraFileExtension.Equals(".cms2") || spectraFileExtension.Equals(".bms2");
        }

        private void ShowPrecursorGraph()
        {
            //precursorGraphSplitContainer.SplitterDistance = (int)(precursorGraphSplitContainer.Height / 2);
            precursorGraphSplitContainer.Panel2.Visible = true;
            precursorGraphSplitContainer.IsSplitterFixed = false;
            precursorGraphSplitContainer.FixedPanel = FixedPanel.None;
        }

        private void HidePrecursorGraph()
        {
            precursorGraphSplitContainer.SplitterDistance = precursorGraphSplitContainer.Height;
            precursorGraphSplitContainer.Panel2.Visible = false;
            precursorGraphSplitContainer.IsSplitterFixed = true;
            precursorGraphSplitContainer.FixedPanel = FixedPanel.Panel2;
        }

        private void DrawPrecursorGraph()
        {
            var theoreticalPrecursorMz = MassSpecUtils.CalculateMz(ViewSpectraSearchResult.CalculatedMass,
                                                                 ViewSpectraSearchResult.AssumedCharge);

            var acquiredPrecursorMz = MassSpecUtils.CalculateMz(ViewSpectraSearchResult.ExperimentalMass,
                                         ViewSpectraSearchResult.AssumedCharge);


            InitializePrecursorGraph(theoreticalPrecursorMz, acquiredPrecursorMz);

            var precursorPeaks = new List<Peak_T_Wrapper>();
            var msFileReader = new MSFileReaderWrapper();
            int ms1ScanNum = 0;
            if (!msFileReader.ReadPrecursorPeaks(SearchResultsMgr.SpectraFile, 
                                                ViewSpectraSearchResult.StartScan, 
                                                SearchResultsMgr.SearchParams.MSLevel,
                                                precursorPeaks,
                                                ref ms1ScanNum))
            {
                // If there are no MS1 scans, just exit, nothing to do.
                return;
            }

            SetPrecursorGraphTitle(theoreticalPrecursorMz, ms1ScanNum);

            PointPair acquiredPrecursorPeak = null;
            PointPair theoreticalPrecursorPeak = null;

            for (int i = 0; i < precursorPeaks.Count; i++)
            {
                var peak = new Peak(precursorPeaks[i]);
                
                // Ignore the peaks with zero intensity
                if (peak.Intensity.Equals(0.0))
                {
                    continue;
                }

                // Add this peak to the list to draw later
                PrecursorGraphPeaksList.Add(peak.Mz, peak.Intensity);

                // See if this peak is a good fit for the theoretical precursor peak
                UpdatePrecursorPeak(peak, theoreticalPrecursorMz, ref theoreticalPrecursorPeak);

                // See if this peak is a good fit for the acquired precursor peak
                UpdatePrecursorPeak(peak, acquiredPrecursorMz, ref acquiredPrecursorPeak);
            }

            var border = new Border { IsVisible = false };
            var fill = new Fill { IsVisible = false };

            // Draw the theoretical precursor peak and get the label for it
            // Check to see if the theoretical and acquired peaks are the same,
            // and if they are, draw the theoretical peak a little shorter
            // so we can see the two peaks overlayed on top of each other.
            double peakHeightFraction = 1;
            if (null != acquiredPrecursorPeak && null != theoreticalPrecursorPeak)
            {
                if (theoreticalPrecursorPeak.Equals(acquiredPrecursorPeak))
                {
                    peakHeightFraction = 0.9;
                }
            }
            var theoreticalFontSpec = new FontSpec { Border = border, Fill = fill, FontColor = Color.DeepSkyBlue, IsDropShadow = false };
            var theoreticalPeakLabel = DrawPrecursorPeak(theoreticalPrecursorMz, theoreticalPrecursorPeak, "T: ",
                                                         theoreticalFontSpec, "T = theoretical m/z", peakHeightFraction);
            PrecursorGraphPeakLabels.Add(theoreticalPeakLabel);

            // Draw the acquired precursor peak and get the label for it
            var acquiredFontSpec = new FontSpec { Border = border, Fill = fill, FontColor = Color.Red, IsDropShadow = false };
            var acquiredPeakLabel = DrawPrecursorPeak(acquiredPrecursorMz, acquiredPrecursorPeak, "E: ",
                                             acquiredFontSpec, "E = experimental m/z");
            PrecursorGraphPeakLabels.Add(acquiredPeakLabel);

            // Draw the peak labels for the precursor graph
            foreach (var precursorPeakLabel in PrecursorGraphPeakLabels)
            {
                precursorGraphItem.GraphPane.GraphObjList.Add(precursorPeakLabel);
            }
            
            // Plot the regular peaks on the precursor graph
            precursorGraphItem.GraphPane.AddStick(null, PrecursorGraphPeaksList, Color.LightGray);

            // Calculate the Axis Scale Ranges and redraw the whole graph 
            // control for smooth transition
            AdjustPrecursorGraphYAxis();
            precursorGraphItem.AxisChange();
            precursorGraphItem.Invalidate();
            precursorGraphItem.Refresh();
        }

        private void InitializePrecursorGraph(double theoreticalPrecursorMz, double acquiredPrecursorMz)
        {
            if (null == PrecursorGraphPeakLabels)
            {
                PrecursorGraphPeakLabels = new List<TextObj>();
            }
            else
            {
                PrecursorGraphPeakLabels.Clear();
            }

            if (null == PrecursorGraphPeaksList)
            {
                PrecursorGraphPeaksList = new PointPairList();
            }
            else
            {
                PrecursorGraphPeaksList.Clear();
            }

            GraphPane precursorGraphPane = precursorGraphItem.GraphPane;

            precursorGraphPane.CurveList.Clear();
            precursorGraphPane.GraphObjList.Clear();

            precursorGraphPane.XAxis.Title.Text = "m/z";
            precursorGraphPane.YAxis.Title.Text = "Intensity";
            precursorGraphPane.YAxis.Scale.Min = 0.0;
            precursorGraphPane.XAxis.Scale.Min = Math.Min(theoreticalPrecursorMz, acquiredPrecursorMz) - ((double)massTolTextBox.DecimalValue * 20);
            precursorGraphPane.XAxis.Scale.Max = Math.Max(theoreticalPrecursorMz, acquiredPrecursorMz) + ((double)massTolTextBox.DecimalValue * 20);
            precursorGraphPane.Legend.Position = LegendPos.InsideTopRight;
        }

        private void SetPrecursorGraphTitle(double theoreticalPrecursorMz, int ms1ScanNum)
        {
            GraphPane precursorGraphPane = precursorGraphItem.GraphPane;
            precursorGraphPane.Title.Text = String.Format("Zoomed in Precursor Plot: m/z {0}, scan {1}", Math.Round(theoreticalPrecursorMz, 4), ms1ScanNum);
            var border = new Border { IsVisible = false };
            var fill = new Fill { IsVisible = false };
            precursorGraphPane.Title.FontSpec = new FontSpec { Size = 14, Border = border, Fill = fill };
        }

        private void UpdatePrecursorPeak(Peak peak, double precursorMz, ref PointPair precursorPeak)
        {
            if (MassSpecUtils.IsPeakPresent(peak.Mz, precursorMz, (double)massTolTextBox.DecimalValue))
            {
                if (null == precursorPeak)
                {
                    precursorPeak = new PointPair(peak.Mz, peak.Intensity);
                }
                else
                {
                    double thisPeakDistance = Math.Abs(peak.Mz - precursorMz);
                    double theoreticalPrecursorPeakDistance =
                        Math.Abs(precursorPeak.X - precursorMz);
                    if (thisPeakDistance < theoreticalPrecursorPeakDistance)
                    {
                        precursorPeak.X = peak.Mz;
                        precursorPeak.Y = peak.Intensity;
                    }
                }
            }
        }

        private TextObj DrawPrecursorPeak(double precursorMz, PointPair precursorPeak, String peakLabelText, FontSpec fontSpec, String peakLegend, double peakHeightFraction = 1)
        {
            TextObj peakLabel;
            if (null != precursorPeak)
            {
                peakLabel =
                    new TextObj(PadPeakLabel(peakLabelText + Convert.ToString(Math.Round(precursorPeak.X, 2))),
                                precursorPeak.X, precursorPeak.Y * peakHeightFraction) { FontSpec = fontSpec };

                // Draw the precursor peak
                precursorPeak.Y = precursorPeak.Y*peakHeightFraction;
                precursorGraphItem.GraphPane.AddStick(peakLegend,
                                                      new PointPairList { precursorPeak },
                                                      fontSpec.FontColor);
            }
            else
            {
                peakLabel = new TextObj(PadPeakLabel(peakLabelText + Convert.ToString(Math.Round(precursorMz, 2))), precursorMz, 1) { FontSpec = fontSpec };
            }

            return peakLabel;
        }

        private void AdjustPrecursorGraphYAxis()
        {
            // Adjust the Y-axis to ensure we zoom in on the theoretical and 
            // acquired precursor peaks
            var highestIntensity = 0.0;
            foreach (var peak in PrecursorGraphPeaksList)
            {
                if ((peak.X < precursorGraphItem.GraphPane.XAxis.Scale.Min) ||
                    (peak.X > precursorGraphItem.GraphPane.XAxis.Scale.Max))
                {
                    continue;
                }

                if (peak.Y > highestIntensity)
                {
                    highestIntensity = peak.Y;
                }
            }
            precursorGraphItem.GraphPane.YAxis.Scale.Max = highestIntensity * 1.25;
        }

        private void AddPeakLabel(String label, Color labelColor, double mz, double intensity)
        {
            var border = new Border {IsVisible = false};
            var fill = new Fill {IsVisible = false};
            var fontSpec = new FontSpec { Border = border, Fill = fill, Angle = 90, FontColor = labelColor, IsDropShadow = false};
            switch (SpectrumGraphUserOptions.PeakLabel)
            {
                case PeakLabel.Ion:
                    // Add a few spaces in front of the label string so that it
                    // doesn't cover any part of the stick plot.
                    SpectrumGraphPeakLabels.Add(new TextObj(PadPeakLabel(label), mz, intensity) { FontSpec = fontSpec, ZOrder = ZOrder.A_InFront });
                    break;

                case PeakLabel.Mz:
                    // Add a few spaces in front of the m/z string so that it
                    // doesn't cover any part of the stick plot.
                    SpectrumGraphPeakLabels.Add(new TextObj(PadPeakLabel(Convert.ToString(Math.Round(mz, 2))), mz, intensity) { FontSpec = fontSpec, ZOrder = ZOrder.A_InFront });
                    break;

                case PeakLabel.None:
                    // Do nothing, no label
                    break;
            }
        }

        private String PadPeakLabel(String label)
        {
            var numSpacesToAdd = label.Length * 3;
            return label.PadLeft(numSpacesToAdd);
        }

        private void SpectrumGraphItemZoomEvent(ZedGraphControl sender, ZoomState oldState, ZoomState newState)
        {
            OnGraphItemZoom(spectrumGraphItem.GraphPane, SpectrumGraphPeakLabels);
        }

        private void PrecursorGraphItemZoomEvent(ZedGraphControl sender, ZoomState oldState, ZoomState newState)
        {
            OnGraphItemZoom(precursorGraphItem.GraphPane, PrecursorGraphPeakLabels);
        }

        private void OnGraphItemZoom(GraphPane pane, IEnumerable<TextObj> peakLabels)
        {
            ResetAxesToPositive(pane);

            // Only show the peak labels if they are on the graph pane
            if (null != peakLabels)
            {
                foreach (var peakLabel in peakLabels)
                {
                    peakLabel.IsVisible = IsPointOnGraphPane(pane, new PointPair(peakLabel.Location.X, peakLabel.Location.Y));
                }
            }
        }

        private void ResetAxesToPositive(GraphPane pane)
        {
            // Don't allow X-axis scale to go negative
            if (pane.XAxis.Scale.Min < 0)
            {
                pane.XAxis.Scale.Min = 0;
            }

            // Don't allo the Y-axis scale to go negative
            if (pane.YAxis.Scale.Min < 0)
            {
                pane.YAxis.Scale.Min = 0;
            }
        }

        private static bool IsPointOnGraphPane(GraphPane pane, PointPair point)
        {
            Scale xScale = pane.XAxis.Scale;
            Scale yScale = pane.YAxis.Scale;
            return point.X > xScale.Min && point.X < xScale.Max && point.Y > yScale.Min && point.Y < yScale.Max;
        }

        private void NeutralLossNh3CheckBoxCheckedChanged(object sender, EventArgs e)
        {
            OnNeutralLossChanged(neutralLossNH3CheckBox, MassSpecUtils.NeutralLoss.NH3);
        }

        private void NeutralLossH2OcheckBoxCheckedChanged(object sender, EventArgs e)
        {
            OnNeutralLossChanged(neutralLossH2OCheckBox, MassSpecUtils.NeutralLoss.H2O);
        }

        private void OnNeutralLossChanged(CheckBox neutralLossCheckBox, MassSpecUtils.NeutralLoss neutralLoss)
        {
            if (neutralLossCheckBox.Checked)
            {
                SpectrumGraphUserOptions.NeutralLoss.Add(neutralLoss);
            }
            else
            {
                SpectrumGraphUserOptions.NeutralLoss.Remove(neutralLoss);
            }
        }

        private void PeakLabelIonRadioButtonCheckedChanged(object sender, EventArgs e)
        {
            OnPeakLabelChanged(peakLabelIonRadioButton, PeakLabel.Ion);
        }

        private void PeakLabelMzRadioButtonCheckedChanged(object sender, EventArgs e)
        {
            OnPeakLabelChanged(peakLabelMzRadioButton, PeakLabel.Mz);
        }

        private void PeakLabelNoneRadioButtonCheckedChanged(object sender, EventArgs e)
        {
            OnPeakLabelChanged(peakLabelNoneRadioButton, PeakLabel.None);
        }

        private void OnPeakLabelChanged(RadioButton pealLabelRadioButton, PeakLabel peakLabel)
        {
            if (pealLabelRadioButton.Checked)
            {
                SpectrumGraphUserOptions.PeakLabel = peakLabel;
            }
        }

        private void MassTypeAvgRadioButtonCheckedChanged(object sender, EventArgs e)
        {
            if (massTypeAvgRadioButton.Checked)
            {
                SpectrumGraphUserOptions.MassType = MassSpecUtils.MassType.Average;
            }
        }

        private void MassTypeMonoRadioButtonCheckedChanged(object sender, EventArgs e)
        {
            if (massTypeMonoRadioButton.Checked)
            {
                SpectrumGraphUserOptions.MassType = MassSpecUtils.MassType.Monoisotopic;
            }
        }

        private void UpdateBtnClick(object sender, EventArgs e)
        {
            SpectrumGraphUserOptions.MassTol = Convert.ToDouble(massTolTextBox.Text);
            UpdateViewSpectra();
        }

        private void AIonSinglyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(aIonSinglyChargedCheckBox.Checked, IonType.A, 1);
        }

        private void AIonDoublyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(aIonDoublyChargedCheckBox.Checked, IonType.A, 2);
        }

        private void AIonTriplyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(aIonTriplyChargedCheckBox.Checked, IonType.A, 3);
        }

        private void BIonSinglyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(bIonSinglyChargedCheckBox.Checked, IonType.B, 1);
        }

        private void BIonDoublyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(bIonDoublyChargedCheckBox.Checked, IonType.B, 2);
        }

        private void BIonTriplyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(bIonTriplyChargedCheckBox.Checked, IonType.B, 3);
        }

        private void CIonSinglyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(cIonSinglyChargedCheckBox.Checked, IonType.C, 1);
        }

        private void CIonDoublyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(cIonDoublyChargedCheckBox.Checked, IonType.C, 2);
        }

        private void CIonTriplyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(cIonTriplyChargedCheckBox.Checked, IonType.C, 3);
        }

        private void XIonSinglyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(xIonSinglyChargedCheckBox.Checked, IonType.X, 1);
        }

        private void XIonDoublyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(xIonDoublyChargedCheckBox.Checked, IonType.X, 2);
        }

        private void XIonTriplyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(xIonTriplyChargedCheckBox.Checked, IonType.X, 3);
        }

        private void YIonSinglyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(yIonSinglyChargedCheckBox.Checked, IonType.Y, 1);
        }

        private void YIonDoublyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(yIonDoublyChargedCheckBox.Checked, IonType.Y, 2);
        }

        private void YIonTriplyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(yIonTriplyChargedCheckBox.Checked, IonType.Y, 3);
        }

        private void ZIonSinglyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(zIonSinglyChargedCheckBox.Checked, IonType.Z, 1);
        }

        private void ZIonDoublyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(zIonDoublyChargedCheckBox.Checked, IonType.Z, 2);
        }

        private void ZIonTriplyChargedCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            UpdateUseIonsMap(zIonTriplyChargedCheckBox.Checked, IonType.Z, 3);
        }

        private void UpdateUseIonsMap(bool useIonCharge, IonType ionType, int ionCharge)
        {
            if (useIonCharge)
            {
                List<int> charges;
                if (!SpectrumGraphUserOptions.UseIonsMap.TryGetValue(ionType, out charges))
                {
                    // The IonType doesn't exist in the map at all, simply add the (IonType, charge) pair to it
                    SpectrumGraphUserOptions.UseIonsMap.Add(ionType, new List<int> { ionCharge });
                }
                else
                {
                    // The IonType exists in the map, but check to see if the charge exists for the IonType
                    // If the charge also exists, do nothing, since we already have this pair in the map.
                    if (!charges.Contains(ionCharge))
                    {
                        // If the charge does NOT exist, we need to add it and associate it with the IonType
                        var newUseIonsMap = new Dictionary<IonType, List<int>>();
                        foreach (var item in SpectrumGraphUserOptions.UseIonsMap)
                        {
                            if (item.Key == ionType)
                            {
                                item.Value.Add(ionCharge);
                            }

                            newUseIonsMap.Add(item.Key, item.Value);
                        }

                        // Now overwrite the current UseIonsMap with the new one
                        SpectrumGraphUserOptions.UseIonsMap = newUseIonsMap;
                    }
                }
            }
            else
            {
                // Here, the user has deselected this particular (IonType, charge) pair, so remove it if necessary
                // If the (IonType, charge) pair isn't even found in the map, just do nothing
                List<int> charges;
                if (SpectrumGraphUserOptions.UseIonsMap.TryGetValue(ionType, out charges) && charges.Contains(ionCharge))
                {
                    // The (IonType, charge) pair was found, so we need to remove it
                    var newUseIonsMap = new Dictionary<IonType, List<int>>();
                    foreach (var item in SpectrumGraphUserOptions.UseIonsMap)
                    {
                        if (item.Key == ionType)
                        {
                            item.Value.Remove(ionCharge);
                            if (item.Value.Count > 0)
                            {
                                newUseIonsMap.Add(item.Key, item.Value);
                            }
                        }
                        else
                        {
                            newUseIonsMap.Add(item.Key, item.Value);
                        }
                    }

                    // Now overwrite the current UseIonsMap with the new one
                    SpectrumGraphUserOptions.UseIonsMap = newUseIonsMap;
                }
            }
        }

        private void ResultsListContextMenuItemExportClick(object sender, EventArgs e)
        {
            ExportResultsList();
        }
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
        public List<MassSpecUtils.NeutralLoss> NeutralLoss { get; set; }
        public Dictionary<IonType, List<int>> UseIonsMap { get; set; }
        public PeakLabel PeakLabel { get; set; }

        public SpectrumGraphUserOptions()
        {
            UseIonsMap = new Dictionary<IonType, List<int>>();
            MassType = MassSpecUtils.MassType.Monoisotopic;
            MassTol = 0.5;
            NeutralLoss = new List<MassSpecUtils.NeutralLoss>();
            PeakLabel = PeakLabel.Ion;
        }
    }

    class FragmentIonGraphInfo
    {
        public PointPairList FragmentIonPeaks { get; set; }
        public Color PeakColor { get; set; }

        public FragmentIonGraphInfo(PointPairList fragmentIonPeaks, Color peakColor)
        {
            FragmentIonPeaks = new PointPairList(fragmentIonPeaks);
            PeakColor = peakColor;
        }
    }
}
