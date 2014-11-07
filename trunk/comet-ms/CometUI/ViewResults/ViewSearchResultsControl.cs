using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.ViewResults
{
    public partial class ViewSearchResultsControl : UserControl
    {
        public String ResultsPepXMLFile { get; set; }

        public bool SettingsChanged { get; set; }
        
        private CometUI CometUI { get; set; }
        private bool OptionsPanelShown { get; set; }
        private ViewResultsSummaryOptionsControl ViewResultsSummaryOptionsControl { get; set; }
        private ViewResultsDisplayOptionsControl ViewResultsDisplayOptionsControl { get; set; }
        private ViewResultsPickColumnsControl ViewResultsPickColumnsControl { get; set; }

        private readonly Dictionary<String, String> _condensedColumnHeadersMap = new Dictionary<String, String>();

        public ViewSearchResultsControl(CometUI parent)
        {
            InitializeComponent();

            CometUI = parent;

            InitializeColumnHeadersMap();

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
            if (null != resultsPepXMLFile)
            {
                ResultsPepXMLFile = resultsPepXMLFile;
                ShowResultsListPanel(String.Empty != ResultsPepXMLFile);
                ViewResultsSummaryOptionsControl.UpdateSummaryOptions();
                UpdateColumnHeaders();
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

        private void UpdateSearchResultsList()
        {

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
            _condensedColumnHeadersMap.Add("probability", "PROB");
            _condensedColumnHeadersMap.Add("start_scan", "SSCAN");
            _condensedColumnHeadersMap.Add("calc_neutral_pep_mass", "CALC_MASS");

            //_columnHeadersMap.Add("MZratio", "MZRATIO");
            //_columnHeadersMap.Add("protein_descr", "PROTEIN_DESCR");
            //_columnHeadersMap.Add("pI", "PI");
            //_columnHeadersMap.Add("retention_time_sec", "RETENTION_TIME_SEC");
            //_columnHeadersMap.Add("compensation_voltage", "COMPENSATION_VOLTAGE");
            //_columnHeadersMap.Add("precursor_intensity", "PRECURSOR_INTENSITY");
            //_columnHeadersMap.Add("collision_energy", "COLLISION_ENERGY");
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
    //_columnHeadersMap.Add("compensation_voltage", "COMPENSATION_VOLTAGE");
    //_columnHeadersMap.Add("precursor_intensity", "PRECURSOR_INTENSITY");
    //_columnHeadersMap.Add("collision_energy", "COLLISION_ENERGY");
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

    public class SearchResult
    {
        private Dictionary<String, SearchResultField> Fields { get; set; }  
        
        SearchResult()
        {
            Fields = new Dictionary<string, SearchResultField>();
        }
    }

    public class SearchResultField
    {
        public SearchResultFieldType Type { get; set; }

        public SearchResultField(SearchResultFieldType type)
        {
            Type = type;
        }
    }

    public class TypedSearchResultField<T> : SearchResultField
    {
        public new T Value { get; set; }

        public TypedSearchResultField(SearchResultFieldType type, T value)
            : base(type)
        {
            Value = value;
        }
    }

    public enum SearchResultFieldType
    {
        Unknown = 0,
        Int,
        Double,
        String
    }
}
