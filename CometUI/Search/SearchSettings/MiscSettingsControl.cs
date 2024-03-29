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
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
{
    public partial class MiscSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }

        public MiscSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializePrecursorPeakCombo();

            InitializeOverrideChargeCombo();
        }

        public void Initialize()
        {
            InitializeFromDefaultSettings();
        }

        public bool VerifyAndUpdateSettings()
        {
            // mzXML settings

            var scanRangeMin = mzxmlScanRangeMinTextBox.IntValue;
            if (scanRangeMin != CometUIMainForm.SearchSettings.mzxmlScanRangeMin)
            {
                CometUIMainForm.SearchSettings.mzxmlScanRangeMin = scanRangeMin;
                Parent.SettingsChanged = true;
            }

            var scanRangeMax = mzxmlScanRangeMaxTextBox.IntValue;
            if (scanRangeMax != CometUIMainForm.SearchSettings.mzxmlScanRangeMax)
            {
                CometUIMainForm.SearchSettings.mzxmlScanRangeMax = scanRangeMax;
                Parent.SettingsChanged = true;
            }

            var precursorChargeMin = mzxmlPrecursorChargeMinTextBox.IntValue;
            if (precursorChargeMin != CometUIMainForm.SearchSettings.mzxmlPrecursorChargeRangeMin)
            {
                CometUIMainForm.SearchSettings.mzxmlPrecursorChargeRangeMin = precursorChargeMin;
                Parent.SettingsChanged = true;
            }

            var precursorChargeMax = mzxmlPrecursorChargeMaxTextBox.IntValue;
            if (precursorChargeMax != CometUIMainForm.SearchSettings.mzxmlPrecursorChargeRangeMax)
            {
                CometUIMainForm.SearchSettings.mzxmlPrecursorChargeRangeMax = precursorChargeMax;
                Parent.SettingsChanged = true;
            }

            var overrideCharge = mzxmlOverrideChargeCombo.SelectedIndex;
            if (overrideCharge != CometUIMainForm.SearchSettings.mzxmlOverrideCharge)
            {
                CometUIMainForm.SearchSettings.mzxmlOverrideCharge = overrideCharge;
                Parent.SettingsChanged = true;
            }

            var mzLevel = Int32.Parse(mzxmlMsLevelCombo.SelectedItem.ToString());
            if (mzLevel != CometUIMainForm.SearchSettings.mzxmlMsLevel)
            {
                CometUIMainForm.SearchSettings.mzxmlMsLevel = mzLevel;
                Parent.SettingsChanged = true;
            }

            var activationLevel = mzxmlActivationLevelCombo.SelectedItem.ToString();
            if (!activationLevel.Equals(CometUIMainForm.SearchSettings.mzxmlActivationMethod))
            {
                CometUIMainForm.SearchSettings.mzxmlActivationMethod = activationLevel;
                Parent.SettingsChanged = true;
            }

            // Spectral processing settings

            var minPeaks = spectralProcessingMinPeaksTextBox.IntValue;
            if (minPeaks != CometUIMainForm.SearchSettings.spectralProcessingMinPeaks)
            {
                CometUIMainForm.SearchSettings.spectralProcessingMinPeaks = minPeaks;
                Parent.SettingsChanged = true;
            }

            var minIntensity = (double)spectralProcessingMinIntensityTextBox.DecimalValue;
            if (!minIntensity.Equals(CometUIMainForm.SearchSettings.spectralProcessingMinIntensity))
            {
                CometUIMainForm.SearchSettings.spectralProcessingMinIntensity = minIntensity;
                Parent.SettingsChanged = true;
            }

            var precursorRemovalTol = (double) spectralProcessingPrecursorRemovalTolTextBox.DecimalValue;
            if (!precursorRemovalTol.Equals(CometUIMainForm.SearchSettings.spectralProcessingRemovePrecursorTol))
            {
                CometUIMainForm.SearchSettings.spectralProcessingRemovePrecursorTol = precursorRemovalTol;
                Parent.SettingsChanged = true;
            }

            // Check each key in the dictionary to see which matches the value
            // currently selected in the combo box.
            var removePrecursorPeak = spectralProcessingRemovePrecursorPeakCombo.SelectedIndex;
            if (CometUIMainForm.SearchSettings.spectralProcessingRemovePrecursorPeak != removePrecursorPeak)
            {
                CometUIMainForm.SearchSettings.spectralProcessingRemovePrecursorPeak = removePrecursorPeak;
                Parent.SettingsChanged = true;
            }

            var clearMzMin = (double) spectralProcessingClearMZRangeMinTextBox.DecimalValue;
            if (!clearMzMin.Equals(CometUIMainForm.SearchSettings.spectralProcessingClearMzMin))
            {
                CometUIMainForm.SearchSettings.spectralProcessingClearMzMin = clearMzMin;
                Parent.SettingsChanged = true;
            }

            var clearMzMax = (double)spectralProcessingClearMZRangeMaxTextBox.DecimalValue;
            if (!clearMzMax.Equals(CometUIMainForm.SearchSettings.spectralProcessingClearMzMax))
            {
                CometUIMainForm.SearchSettings.spectralProcessingClearMzMax = clearMzMax;
                Parent.SettingsChanged = true;
            }

            // Other settings
            var spectrumBatchSize = spectrumBatchSizeTextBox.IntValue;
            if (!spectrumBatchSize.Equals(CometUIMainForm.SearchSettings.SpectrumBatchSize))
            {
                CometUIMainForm.SearchSettings.SpectrumBatchSize = spectrumBatchSize;
                Parent.SettingsChanged = true;
            }

            var numThreads = Int32.Parse(numThreadsCombo.SelectedItem.ToString());
            if (numThreads != CometUIMainForm.SearchSettings.NumThreads)
            {
                CometUIMainForm.SearchSettings.NumThreads = numThreads;
                Parent.SettingsChanged = true;
            }

            var numResults = numResultsTextBox.IntValue;
            if (!numResults.Equals(CometUIMainForm.SearchSettings.NumResults))
            {
                CometUIMainForm.SearchSettings.NumResults = numResults;
                Parent.SettingsChanged = true;
            }

            var maxFragmentCharge = Int32.Parse(maxFragmentChargeCombo.SelectedItem.ToString());
            if (maxFragmentCharge != CometUIMainForm.SearchSettings.MaxFragmentCharge)
            {
                CometUIMainForm.SearchSettings.MaxFragmentCharge = maxFragmentCharge;
                Parent.SettingsChanged = true;
            }

            var maxPrecursorCharge = Int32.Parse(maxPrecursorChargeCombo.SelectedItem.ToString());
            if (maxPrecursorCharge != CometUIMainForm.SearchSettings.MaxPrecursorCharge)
            {
                CometUIMainForm.SearchSettings.MaxPrecursorCharge = maxPrecursorCharge;
                Parent.SettingsChanged = true;
            }

            if (clipNTermMethionineCheckBox.Checked != CometUIMainForm.SearchSettings.ClipNTermMethionine)
            {
                CometUIMainForm.SearchSettings.ClipNTermMethionine = clipNTermMethionineCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializePrecursorPeakCombo()
        {
            // MUST be added in the following order since the value of this 
            // parameter corresponds to the index of the string in the combo
            spectralProcessingRemovePrecursorPeakCombo.Items.Add("No");
            spectralProcessingRemovePrecursorPeakCombo.Items.Add("Yes");
            spectralProcessingRemovePrecursorPeakCombo.Items.Add("Yes/ETD");
        }

        private void InitializeOverrideChargeCombo()
        {
            // MUST be added in the following order since the value of this
            // parameter corresponds to the index of the string in the combo
            mzxmlOverrideChargeCombo.Items.Add("Keep known charges");
            mzxmlOverrideChargeCombo.Items.Add("Always use charge range");           
            mzxmlOverrideChargeCombo.Items.Add("Use charge range as charge filter");
            mzxmlOverrideChargeCombo.Items.Add("Keep charges & use 1+ rule");
        }

        private void InitializeFromDefaultSettings()
        {
            numThreadsCombo.SelectedItem = CometUIMainForm.SearchSettings.NumThreads.ToString(CultureInfo.InvariantCulture);

            spectrumBatchSizeTextBox.Text = CometUIMainForm.SearchSettings.SpectrumBatchSize.ToString(CultureInfo.InvariantCulture);

            numResultsTextBox.Text = CometUIMainForm.SearchSettings.NumResults.ToString(CultureInfo.InvariantCulture);

            maxFragmentChargeCombo.SelectedItem = CometUIMainForm.SearchSettings.MaxFragmentCharge.ToString(CultureInfo.InvariantCulture);

            maxPrecursorChargeCombo.SelectedItem = CometUIMainForm.SearchSettings.MaxPrecursorCharge.ToString(CultureInfo.InvariantCulture);

            clipNTermMethionineCheckBox.Checked = CometUIMainForm.SearchSettings.ClipNTermMethionine;

            mzxmlScanRangeMinTextBox.Text = CometUIMainForm.SearchSettings.mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlScanRangeMaxTextBox.Text = CometUIMainForm.SearchSettings.mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMinTextBox.Text = CometUIMainForm.SearchSettings.mzxmlPrecursorChargeRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMaxTextBox.Text = CometUIMainForm.SearchSettings.mzxmlPrecursorChargeRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlMsLevelCombo.SelectedItem = CometUIMainForm.SearchSettings.mzxmlMsLevel.ToString(CultureInfo.InvariantCulture);
            mzxmlActivationLevelCombo.SelectedItem = CometUIMainForm.SearchSettings.mzxmlActivationMethod;

            spectralProcessingMinPeaksTextBox.Text = CometUIMainForm.SearchSettings.spectralProcessingMinPeaks.ToString(CultureInfo.InvariantCulture);
            spectralProcessingMinIntensityTextBox.Text = CometUIMainForm.SearchSettings.spectralProcessingMinIntensity.ToString(CultureInfo.InvariantCulture);
            spectralProcessingPrecursorRemovalTolTextBox.Text =
                CometUIMainForm.SearchSettings.spectralProcessingRemovePrecursorTol.ToString(CultureInfo.InvariantCulture);
            spectralProcessingClearMZRangeMinTextBox.Text = CometUIMainForm.SearchSettings.spectralProcessingClearMzMin.ToString(CultureInfo.InvariantCulture);
            spectralProcessingClearMZRangeMaxTextBox.Text = CometUIMainForm.SearchSettings.spectralProcessingClearMzMax.ToString(CultureInfo.InvariantCulture);
            
            spectralProcessingRemovePrecursorPeakCombo.SelectedIndex = CometUIMainForm.SearchSettings.spectralProcessingRemovePrecursorPeak;

            mzxmlOverrideChargeCombo.SelectedIndex = CometUIMainForm.SearchSettings.mzxmlOverrideCharge;
        }

        private void MzxmlOverrideChargeComboDropDown(object sender, EventArgs e)
        {
            var senderComboBox = (ComboBox)sender;
            int width = senderComboBox.DropDownWidth;
            Graphics g = senderComboBox.CreateGraphics();
            Font font = senderComboBox.Font;
            int vertScrollBarWidth =
                (senderComboBox.Items.Count > senderComboBox.MaxDropDownItems)
                ? SystemInformation.VerticalScrollBarWidth : 0;

            foreach (string s in ((ComboBox)sender).Items)
            {
                int newWidth = (int)g.MeasureString(s, font).Width
                               + vertScrollBarWidth;
                if (width < newWidth)
                {
                    width = newWidth;
                }
            }
            senderComboBox.DropDownWidth = width;
        }
    }
}
