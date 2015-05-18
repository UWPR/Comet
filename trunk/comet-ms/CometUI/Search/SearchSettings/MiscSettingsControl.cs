/*
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
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
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
        }

        public void Initialize()
        {
            InitializeFromDefaultSettings();
        }

        public bool VerifyAndUpdateSettings()
        {
            // mzXML settings

            var scanRangeMin = mzxmlScanRangeMinTextBox.IntValue;
            if (scanRangeMin != CometUI.SearchSettings.mzxmlScanRangeMin)
            {
                CometUI.SearchSettings.mzxmlScanRangeMin = scanRangeMin;
                Parent.SettingsChanged = true;
            }

            var scanRangeMax = mzxmlScanRangeMaxTextBox.IntValue;
            if (scanRangeMax != CometUI.SearchSettings.mzxmlScanRangeMax)
            {
                CometUI.SearchSettings.mzxmlScanRangeMax = scanRangeMax;
                Parent.SettingsChanged = true;
            }

            var precursorChargeMin = mzxmlPrecursorChargeMinTextBox.IntValue;
            if (precursorChargeMin != CometUI.SearchSettings.mzxmlPrecursorChargeRangeMin)
            {
                CometUI.SearchSettings.mzxmlPrecursorChargeRangeMin = precursorChargeMin;
                Parent.SettingsChanged = true;
            }

            var precursorChargeMax = mzxmlPrecursorChargeMaxTextBox.IntValue;
            if (precursorChargeMax != CometUI.SearchSettings.mzxmlPrecursorChargeRangeMax)
            {
                CometUI.SearchSettings.mzxmlPrecursorChargeRangeMax = precursorChargeMax;
                Parent.SettingsChanged = true;
            }

            var mzLevel = Int32.Parse(mzxmlMsLevelCombo.SelectedItem.ToString());
            if (mzLevel != CometUI.SearchSettings.mzxmlMsLevel)
            {
                CometUI.SearchSettings.mzxmlMsLevel = mzLevel;
                Parent.SettingsChanged = true;
            }

            var activationLevel = mzxmlActivationLevelCombo.SelectedItem.ToString();
            if (!activationLevel.Equals(CometUI.SearchSettings.mzxmlActivationMethod))
            {
                CometUI.SearchSettings.mzxmlActivationMethod = activationLevel;
                Parent.SettingsChanged = true;
            }

            // Spectral processing settings

            var minPeaks = spectralProcessingMinPeaksTextBox.IntValue;
            if (minPeaks != CometUI.SearchSettings.spectralProcessingMinPeaks)
            {
                CometUI.SearchSettings.spectralProcessingMinPeaks = minPeaks;
                Parent.SettingsChanged = true;
            }

            var minIntensity = (double)spectralProcessingMinIntensityTextBox.DecimalValue;
            if (!minIntensity.Equals(CometUI.SearchSettings.spectralProcessingMinIntensity))
            {
                CometUI.SearchSettings.spectralProcessingMinIntensity = minIntensity;
                Parent.SettingsChanged = true;
            }

            var precursorRemovalTol = (double) spectralProcessingPrecursorRemovalTolTextBox.DecimalValue;
            if (!precursorRemovalTol.Equals(CometUI.SearchSettings.spectralProcessingRemovePrecursorTol))
            {
                CometUI.SearchSettings.spectralProcessingRemovePrecursorTol = precursorRemovalTol;
                Parent.SettingsChanged = true;
            }

            // Check each key in the dictionary to see which matches the value
            // currently selected in the combo box.
            foreach (var key in _removePrecursorPeak.Keys)
            {
                if (_removePrecursorPeak[key].Equals(spectralProcessingRemovePrecursorPeakCombo.SelectedItem.ToString()))
                {
                    if (CometUI.SearchSettings.spectralProcessingRemovePrecursorPeak != key)
                    {
                        CometUI.SearchSettings.spectralProcessingRemovePrecursorPeak = key;
                        Parent.SettingsChanged = true;
                    }

                    break;
                }
            }

            var clearMzMin = (double) spectralProcessingClearMZRangeMinTextBox.DecimalValue;
            if (!clearMzMin.Equals(CometUI.SearchSettings.spectralProcessingClearMzMin))
            {
                CometUI.SearchSettings.spectralProcessingClearMzMin = clearMzMin;
                Parent.SettingsChanged = true;
            }

            var clearMzMax = (double)spectralProcessingClearMZRangeMaxTextBox.DecimalValue;
            if (!clearMzMax.Equals(CometUI.SearchSettings.spectralProcessingClearMzMax))
            {
                CometUI.SearchSettings.spectralProcessingClearMzMax = clearMzMax;
                Parent.SettingsChanged = true;
            }

            // Other settings
            var spectrumBatchSize = spectrumBatchSizeTextBox.IntValue;
            if (!spectrumBatchSize.Equals(CometUI.SearchSettings.SpectrumBatchSize))
            {
                CometUI.SearchSettings.SpectrumBatchSize = spectrumBatchSize;
                Parent.SettingsChanged = true;
            }

            var numThreads = Int32.Parse(numThreadsCombo.SelectedItem.ToString());
            if (numThreads != CometUI.SearchSettings.NumThreads)
            {
                CometUI.SearchSettings.NumThreads = numThreads;
                Parent.SettingsChanged = true;
            }

            var numResults = numResultsTextBox.IntValue;
            if (!numResults.Equals(CometUI.SearchSettings.NumResults))
            {
                CometUI.SearchSettings.NumResults = numResults;
                Parent.SettingsChanged = true;
            }

            var maxFragmentCharge = Int32.Parse(maxFragmentChargeCombo.SelectedItem.ToString());
            if (maxFragmentCharge != CometUI.SearchSettings.MaxFragmentCharge)
            {
                CometUI.SearchSettings.MaxFragmentCharge = maxFragmentCharge;
                Parent.SettingsChanged = true;
            }

            var maxPrecursorCharge = Int32.Parse(maxPrecursorChargeCombo.SelectedItem.ToString());
            if (maxPrecursorCharge != CometUI.SearchSettings.MaxPrecursorCharge)
            {
                CometUI.SearchSettings.MaxPrecursorCharge = maxPrecursorCharge;
                Parent.SettingsChanged = true;
            }

            if (clipNTermMethionineCheckBox.Checked != CometUI.SearchSettings.ClipNTermMethionine)
            {
                CometUI.SearchSettings.ClipNTermMethionine = clipNTermMethionineCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            numThreadsCombo.SelectedItem = CometUI.SearchSettings.NumThreads.ToString(CultureInfo.InvariantCulture);

            spectrumBatchSizeTextBox.Text = CometUI.SearchSettings.SpectrumBatchSize.ToString(CultureInfo.InvariantCulture);

            numResultsTextBox.Text = CometUI.SearchSettings.NumResults.ToString(CultureInfo.InvariantCulture);

            maxFragmentChargeCombo.SelectedItem = CometUI.SearchSettings.MaxFragmentCharge.ToString(CultureInfo.InvariantCulture);

            maxPrecursorChargeCombo.SelectedItem = CometUI.SearchSettings.MaxPrecursorCharge.ToString(CultureInfo.InvariantCulture);

            clipNTermMethionineCheckBox.Checked = CometUI.SearchSettings.ClipNTermMethionine;

            mzxmlScanRangeMinTextBox.Text = CometUI.SearchSettings.mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlScanRangeMaxTextBox.Text = CometUI.SearchSettings.mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMinTextBox.Text = CometUI.SearchSettings.mzxmlPrecursorChargeRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMaxTextBox.Text = CometUI.SearchSettings.mzxmlPrecursorChargeRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlMsLevelCombo.SelectedItem = CometUI.SearchSettings.mzxmlMsLevel.ToString(CultureInfo.InvariantCulture);
            mzxmlActivationLevelCombo.SelectedItem = CometUI.SearchSettings.mzxmlActivationMethod;

            spectralProcessingMinPeaksTextBox.Text = CometUI.SearchSettings.spectralProcessingMinPeaks.ToString(CultureInfo.InvariantCulture);
            spectralProcessingMinIntensityTextBox.Text = CometUI.SearchSettings.spectralProcessingMinIntensity.ToString(CultureInfo.InvariantCulture);
            spectralProcessingPrecursorRemovalTolTextBox.Text =
                CometUI.SearchSettings.spectralProcessingRemovePrecursorTol.ToString(CultureInfo.InvariantCulture);
            spectralProcessingClearMZRangeMinTextBox.Text = CometUI.SearchSettings.spectralProcessingClearMzMin.ToString(CultureInfo.InvariantCulture);
            spectralProcessingClearMZRangeMaxTextBox.Text = CometUI.SearchSettings.spectralProcessingClearMzMax.ToString(CultureInfo.InvariantCulture);
            
            spectralProcessingRemovePrecursorPeakCombo.SelectedItem =
                _removePrecursorPeak[CometUI.SearchSettings.spectralProcessingRemovePrecursorPeak];
        }
    }
}
