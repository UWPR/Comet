using System.Globalization;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsDisplayOptionsControl : UserControl
    {
        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public ViewResultsDisplayOptionsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            ViewSearchResultsControl = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            highlightPeptideIncludeModCheckBox.Checked =
                CometUI.ViewResultsSettings.DisplayOptionsInlcudeModsWhenHighlightingPeptides;

            if (CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
            {
                columnHeadersCondensedRadioButton.Checked = true;
            }
            else
            {
                columnHeadersRegularRadioButton.Checked = true;
            }
        }

        private void HighlightPeptideIncludeModCheckBoxCheckedChanged(object sender, System.EventArgs e)
        {
            VerifyAndUpdateHighlightPeptideIncludeModSetting();
        }

        private void VerifyAndUpdateHighlightPeptideIncludeModSetting()
        {
            if (highlightPeptideIncludeModCheckBox.Checked !=
                CometUI.ViewResultsSettings.DisplayOptionsInlcudeModsWhenHighlightingPeptides)
            {
                CometUI.ViewResultsSettings.DisplayOptionsInlcudeModsWhenHighlightingPeptides =
                    highlightPeptideIncludeModCheckBox.Checked;
                ViewSearchResultsControl.SettingsChanged = true;
            }
        }

        private void ColumnHeadersRegularRadioButtonCheckedChanged(object sender, System.EventArgs e)
        {
            VerifyAndUpdateColumnHeaderCondensedSetting();
        }

        private void ColumnHeadersCondensedRadioButtonCheckedChanged(object sender, System.EventArgs e)
        {
            VerifyAndUpdateColumnHeaderCondensedSetting();
        }

        private void VerifyAndUpdateColumnHeaderCondensedSetting()
        {
            if (columnHeadersCondensedRadioButton.Checked !=
                CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
            {
                CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders =
                    columnHeadersCondensedRadioButton.Checked;
                ViewSearchResultsControl.SettingsChanged = true;
            } 
        }

        private void BtnUpdateResultsClick(object sender, System.EventArgs e)
        {
            ViewSearchResultsControl.UpdateSearchResultsList();
        }
    }
}
