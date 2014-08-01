using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class MiscSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }

        private readonly Dictionary<int, string> _removePrecursorPeak = new Dictionary<int, string>();

        public MiscSettingsControl(SearchSettingsDlg parent)
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

        public bool VerifyAndUpdateSettings()
        {
            // mzXML settings

            var scanRangeMin = mzxmlScanRangeMinTextBox.IntValue;
            if (scanRangeMin != Settings.Default.mzxmlScanRangeMin)
            {
                Settings.Default.mzxmlScanRangeMin = scanRangeMin;
                Parent.SettingsChanged = true;
            }

            var scanRangeMax = mzxmlScanRangeMaxTextBox.IntValue;
            if (scanRangeMax != Settings.Default.mzxmlScanRangeMax)
            {
                Settings.Default.mzxmlScanRangeMax = scanRangeMax;
                Parent.SettingsChanged = true;
            }

            var precursorChargeMin = mzxmlPrecursorChargeMinTextBox.IntValue;
            if (precursorChargeMin != Settings.Default.mzxmlPrecursorChargeRangeMin)
            {
                Settings.Default.mzxmlPrecursorChargeRangeMin = precursorChargeMin;
                Parent.SettingsChanged = true;
            }

            var precursorChargeMax = mzxmlPrecursorChargeMaxTextBox.IntValue;
            if (precursorChargeMax != Settings.Default.mzxmlPrecursorChargeRangeMax)
            {
                Settings.Default.mzxmlPrecursorChargeRangeMax = precursorChargeMax;
                Parent.SettingsChanged = true;
            }

            var mzLevel = Int32.Parse(mzxmlMsLevelCombo.SelectedItem.ToString());
            if (mzLevel != Settings.Default.mzxmlMsLevel)
            {
                Settings.Default.mzxmlMsLevel = mzLevel;
                Parent.SettingsChanged = true;
            }

            var activationLevel = mzxmlActivationLevelCombo.SelectedItem.ToString();
            if (!activationLevel.Equals(Settings.Default.mzxmlActivationMethod))
            {
                Settings.Default.mzxmlActivationMethod = activationLevel;
                Parent.SettingsChanged = true;
            }

            // Spectral processing settings

            var minPeaks = spectralProcessingMinPeaksTextBox.IntValue;
            if (minPeaks != Settings.Default.spectralProcessingMinPeaks)
            {
                Settings.Default.spectralProcessingMinPeaks = minPeaks;
                Parent.SettingsChanged = true;
            }

            var minIntensity = (double)spectralProcessingMinIntensityTextBox.DecimalValue;
            if (!minIntensity.Equals(Settings.Default.spectralProcessingMinIntensity))
            {
                Settings.Default.spectralProcessingMinIntensity = minIntensity;
                Parent.SettingsChanged = true;
            }

            var precursorRemovalTol = (double) spectralProcessingPrecursorRemovalTolTextBox.DecimalValue;
            if (!precursorRemovalTol.Equals(Settings.Default.spectralProcessingRemovePrecursorTol))
            {
                Settings.Default.spectralProcessingRemovePrecursorTol = precursorRemovalTol;
                Parent.SettingsChanged = true;
            }

            // Check each key in the dictionary to see which matches the value
            // currently selected in the combo box.
            foreach (var key in _removePrecursorPeak.Keys)
            {
                if (_removePrecursorPeak[key].Equals(spectralProcessingRemovePrecursorPeakCombo.SelectedItem.ToString()))
                {
                    if (Settings.Default.spectralProcessingRemovePrecursorPeak != key)
                    {
                        Settings.Default.spectralProcessingRemovePrecursorPeak = key;
                        Parent.SettingsChanged = true;
                    }

                    break;
                }
            }

            var clearMzMin = (double) spectralProcessingClearMZRangeMinTextBox.DecimalValue;
            if (!clearMzMin.Equals(Settings.Default.spectralProcessingClearMzMin))
            {
                Settings.Default.spectralProcessingClearMzMin = clearMzMin;
                Parent.SettingsChanged = true;
            }

            var clearMzMax = (double)spectralProcessingClearMZRangeMaxTextBox.DecimalValue;
            if (!clearMzMax.Equals(Settings.Default.spectralProcessingClearMzMax))
            {
                Settings.Default.spectralProcessingClearMzMax = clearMzMax;
                Parent.SettingsChanged = true;
            }

            // Other settings
            var spectrumBatchSize = spectrumBatchSizeTextBox.IntValue;
            if (!spectrumBatchSize.Equals(Settings.Default.SpectrumBatchSize))
            {
                Settings.Default.SpectrumBatchSize = spectrumBatchSize;
                Parent.SettingsChanged = true;
            }

            var numThreads = Int32.Parse(numThreadsCombo.SelectedItem.ToString());
            if (numThreads != Settings.Default.NumThreads)
            {
                Settings.Default.NumThreads = numThreads;
                Parent.SettingsChanged = true;
            }

            var numResults = numResultsTextBox.IntValue;
            if (!numResults.Equals(Settings.Default.NumResults))
            {
                Settings.Default.NumResults = numResults;
                Parent.SettingsChanged = true;
            }

            var maxFragmentCharge = Int32.Parse(maxFragmentChargeCombo.SelectedItem.ToString());
            if (maxFragmentCharge != Settings.Default.MaxFragmentCharge)
            {
                Settings.Default.MaxFragmentCharge = maxFragmentCharge;
                Parent.SettingsChanged = true;
            }

            var maxPrecursorCharge = Int32.Parse(maxPrecursorChargeCombo.SelectedItem.ToString());
            if (maxPrecursorCharge != Settings.Default.MaxPrecursorCharge)
            {
                Settings.Default.MaxPrecursorCharge = maxPrecursorCharge;
                Parent.SettingsChanged = true;
            }

            if (clipNTermMethionineCheckBox.Checked != Settings.Default.ClipNTermMethionine)
            {
                Settings.Default.ClipNTermMethionine = clipNTermMethionineCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
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
