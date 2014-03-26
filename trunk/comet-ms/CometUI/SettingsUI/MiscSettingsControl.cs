using System.Collections.Generic;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class MiscSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        private readonly Dictionary<int, string> _removePrecursorPeak = new Dictionary<int, string>();

        public MiscSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            _removePrecursorPeak.Add(0, "No");
            _removePrecursorPeak.Add(1, "Yes");
            _removePrecursorPeak.Add(2, "Yes/ETD");

            Dictionary<int, string>.KeyCollection remPrecursorPeakKeys = _removePrecursorPeak.Keys;
            foreach (var key in remPrecursorPeakKeys)
            {
                spectralProcessingRemovePrecursorPeakCombo.Items.Add(_removePrecursorPeak[key]);
            }

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            numThreadsCombo.SelectedItem = Settings.Default.NumThreads.ToString(CultureInfo.InvariantCulture);

            spectrumBatchSizeTextBox.Text = Settings.Default.SpectrumBatchSize.ToString(CultureInfo.InvariantCulture);

            numResultsTextBox.Text = Settings.Default.NumResults.ToString(CultureInfo.InvariantCulture);

            maxFragmentChargeCombo.SelectedItem = Settings.Default.MaxFragmentCharge.ToString(CultureInfo.InvariantCulture);

            maxPrecursorChargeCombo.SelectedItem = Settings.Default.MaxPrecursorCharge.ToString(CultureInfo.InvariantCulture);

            clipNTermMethionineCheckBox.Checked = Settings.Default.ClipNTermMethionine;

            mzxmlScanRangeMinTextBox.Text = Settings.Default.mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlScanRangeMaxTextBox.Text = Settings.Default.mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMinTextBox.Text = Settings.Default.mzxmlPrecursorChargeRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMaxTextBox.Text = Settings.Default.mzxmlPrecursorChargeRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlMsLevelCombo.SelectedItem = Settings.Default.mzxmlMsLevel.ToString(CultureInfo.InvariantCulture);
            mzxmlActivationLevelCombo.SelectedItem = Settings.Default.mzxmlActivationMethod;

            spectralProcessingMinPeaksTextBox.Text = Settings.Default.spectralProcessingMinPeaks.ToString(CultureInfo.InvariantCulture);
            spectralProcessingMinIntensityTextBox.Text = Settings.Default.spectralProcessingMinIntensity.ToString(CultureInfo.InvariantCulture);
            spectralProcessingPrecursorRemovalTolTextBox.Text =
                Settings.Default.spectralProcessingRemovePrecursorTol.ToString(CultureInfo.InvariantCulture);
            spectralProcessingClearMZRangeMinTextBox.Text = Settings.Default.spectralProcessingClearMzMin.ToString(CultureInfo.InvariantCulture);
            spectralProcessingClearMZRangeMaxTextBox.Text = Settings.Default.spectralProcessingClearMzMax.ToString(CultureInfo.InvariantCulture);
            
            spectralProcessingRemovePrecursorPeakCombo.SelectedItem =
                _removePrecursorPeak[Settings.Default.spectralProcessingRemovePrecursorPeak];
        }
    }
}
