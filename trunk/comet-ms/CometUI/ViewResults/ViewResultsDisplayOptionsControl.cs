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
            rowsPerPageCombo.SelectedItem = CometUI.ViewResultsSettings.DisplayOptionsRowsPerPage.ToString(CultureInfo.InvariantCulture);

            highlightPeptideIncludeModCheckBox.Checked =
                CometUI.ViewResultsSettings.DisplayOptionsInlcudeModsWhenHighlightingPeptides;

            if (CometUI.ViewResultsSettings.DisplayOptionsOnlyTopProteinHit)
            {
                multipleProteinHitsTopHitRadioButton.Checked = true;
            }
            else
            {
                multipleProteinHitsAllHitsRadioButton.Checked = true;
            }

            if (CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
            {
                columnHeadersCondensedRadioButton.Checked = true;
            }
            else
            {
                columnHeadersRegularRadioButton.Checked = true;
            }
        }

        private void BtnUpdateResultsClick(object sender, System.EventArgs e)
        {
            VerifyAndUpdateSettings();
        }

        private void VerifyAndUpdateSettings()
        {
            if (
                !rowsPerPageCombo.SelectedItem.Equals(
                    CometUI.ViewResultsSettings.DisplayOptionsRowsPerPage.ToString(CultureInfo.InvariantCulture)))
            {
                CometUI.ViewResultsSettings.DisplayOptionsRowsPerPage = int.Parse(rowsPerPageCombo.SelectedItem.ToString());
                ViewSearchResultsControl.SettingsChanged = true;
            }

            if (highlightPeptideIncludeModCheckBox.Checked !=
                CometUI.ViewResultsSettings.DisplayOptionsInlcudeModsWhenHighlightingPeptides)
            {
                CometUI.ViewResultsSettings.DisplayOptionsInlcudeModsWhenHighlightingPeptides =
                    highlightPeptideIncludeModCheckBox.Checked;
                ViewSearchResultsControl.SettingsChanged = true;
            }

            if (multipleProteinHitsTopHitRadioButton.Checked != 
                CometUI.ViewResultsSettings.DisplayOptionsOnlyTopProteinHit)
            {
                CometUI.ViewResultsSettings.DisplayOptionsOnlyTopProteinHit =
                    multipleProteinHitsTopHitRadioButton.Checked;
                ViewSearchResultsControl.SettingsChanged = true;
            }

            if (columnHeadersCondensedRadioButton.Checked != 
                CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
            {
                CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders =
                    columnHeadersCondensedRadioButton.Checked;
                ViewSearchResultsControl.SettingsChanged = true;
            }
        }
    }
}
