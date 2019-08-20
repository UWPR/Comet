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

using System.Globalization;
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
{
    public partial class MassSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }
        private Properties.SearchSettings SearchSettings { get { return CometUIMainForm.SearchSettings; } }

        public MassSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeMassUnitCombo();

            InitializeIsotopeErrorCombo();

            InitializePrecursorToleranceType();
        }

        public void Initialize()
        {
            InitializeFromDefaultSettings();
        }

        public bool VerifyAndUpdateSettings()
        {
            // Verify and save the precursor mass settings
            var precursorMassTol = (double) precursorMassTolTextBox.DecimalValue;
            if (!SearchSettings.PrecursorMassTolerance.Equals(precursorMassTol))
            {
                SearchSettings.PrecursorMassTolerance = precursorMassTol;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.PrecursorMassUnit != precursorMassUnitCombo.SelectedIndex)
            {
                SearchSettings.PrecursorMassUnit = precursorMassUnitCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.PrecursorMassType != precursorMassTypeCombo.SelectedIndex)
            {
                SearchSettings.PrecursorMassType = precursorMassTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.PrecursorIsotopeError != precursorIsotopeErrorCombo.SelectedIndex)
            {
                SearchSettings.PrecursorIsotopeError = precursorIsotopeErrorCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.PrecursorToleranceType != precursorToleranceTypeCombo.SelectedIndex)
            {
                SearchSettings.PrecursorToleranceType = precursorToleranceTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (!SearchSettings.PrecursorMassOffsets.Equals(precursorMassOffsetsTextBox.Text))
            {
                SearchSettings.PrecursorMassOffsets = precursorMassOffsetsTextBox.Text;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for fragment settings
            var fragmentBinSize = (double) fragmentBinSizeTextBox.DecimalValue;
            if (!SearchSettings.FragmentBinSize.Equals(fragmentBinSize))
            {
                SearchSettings.FragmentBinSize = fragmentBinSize;
                Parent.SettingsChanged = true;
            }

            var fragmentOffset = (double) fragmentOffsetTextBox.DecimalValue;
            if (!SearchSettings.FragmentBinOffset.Equals(fragmentOffset))
            {
                SearchSettings.FragmentBinOffset = fragmentOffset;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.FragmentMassType != fragmentMassTypeCombo.SelectedIndex)
            {
                SearchSettings.FragmentMassType = fragmentMassTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for ion settings
            if (SearchSettings.UseAIons != aIonCheckBox.Checked)
            {
                SearchSettings.UseAIons = aIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.UseBIons != bIonCheckBox.Checked)
            {
                SearchSettings.UseBIons = bIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.UseCIons != cIonCheckBox.Checked)
            {
                SearchSettings.UseCIons = cIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.UseXIons != xIonCheckBox.Checked)
            {
                SearchSettings.UseXIons = xIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.UseYIons != yIonCheckBox.Checked)
            {
                SearchSettings.UseYIons = yIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.UseZIons != zIonCheckBox.Checked)
            {
                SearchSettings.UseZIons = zIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (SearchSettings.UseNLIons != useNLCheckBox.Checked)
            {
                SearchSettings.UseNLIons = useNLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            var theoreticalFragmentIons = !flankCheckBox.Checked;
            if (SearchSettings.TheoreticalFragmentIons != theoreticalFragmentIons)
            {
                SearchSettings.TheoreticalFragmentIons = theoreticalFragmentIons;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            // Set up defaults for the precursor mass settings
            precursorMassTolTextBox.Text =
                SearchSettings.PrecursorMassTolerance.ToString(CultureInfo.InvariantCulture);
            precursorMassUnitCombo.SelectedIndex = SearchSettings.PrecursorMassUnit;
            precursorMassTypeCombo.SelectedIndex = SearchSettings.PrecursorMassType;
            precursorIsotopeErrorCombo.SelectedIndex = SearchSettings.PrecursorIsotopeError;
            precursorToleranceTypeCombo.SelectedIndex = SearchSettings.PrecursorToleranceType;
            precursorMassOffsetsTextBox.Text = SearchSettings.PrecursorMassOffsets;

            // Set up defaults for fragment settings
            fragmentBinSizeTextBox.Text =
                SearchSettings.FragmentBinSize.ToString(CultureInfo.InvariantCulture);
            fragmentOffsetTextBox.Text =
                SearchSettings.FragmentBinOffset.ToString(CultureInfo.InvariantCulture);
            fragmentMassTypeCombo.SelectedIndex = SearchSettings.FragmentMassType;

            // Set up defaults for ion settings
            aIonCheckBox.Checked = SearchSettings.UseAIons;
            bIonCheckBox.Checked = SearchSettings.UseBIons;
            cIonCheckBox.Checked = SearchSettings.UseCIons;
            xIonCheckBox.Checked = SearchSettings.UseXIons;
            yIonCheckBox.Checked = SearchSettings.UseYIons;
            zIonCheckBox.Checked = SearchSettings.UseZIons;
            useNLCheckBox.Checked = SearchSettings.UseNLIons;
            flankCheckBox.Checked = !SearchSettings.TheoreticalFragmentIons;
        }

        private void InitializeMassUnitCombo()
        {
            // MUST be added in the following order since the value or this
            // parameter corresponds to the index of the string in the combo
            // Add the mass unit types: 0 = amu; 1 = mmu; 2 = ppm
            precursorMassUnitCombo.Items.Add("amu");
            precursorMassUnitCombo.Items.Add("mmu");
            precursorMassUnitCombo.Items.Add("ppm");
        }

        private void InitializeIsotopeErrorCombo()
        {
            // MUST be added in the following order since the value or this
            // parameter corresponds to the index of the string in the combo
            precursorIsotopeErrorCombo.Items.Add("no C13");
            precursorIsotopeErrorCombo.Items.Add("C13 offsets");
        }

        private void InitializePrecursorToleranceType()
        {
            // MUST be added in the following order since the value or this
            // parameter corresponds to the index of the string in the combo
            precursorToleranceTypeCombo.Items.Add("Apply to mass");
            precursorToleranceTypeCombo.Items.Add("Apply to m/z");
        }
    }
}
