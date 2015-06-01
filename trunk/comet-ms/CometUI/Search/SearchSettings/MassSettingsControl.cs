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
     
        public MassSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
        }

        public void Initialize()
        {
            InitializeFromDefaultSettings();
        }

        public bool VerifyAndUpdateSettings()
        {
            // Verify and save the precursor mass settings
            var precursorMassTol = (double) precursorMassTolTextBox.DecimalValue;
            if (!CometUIMainForm.SearchSettings.PrecursorMassTolerance.Equals(precursorMassTol))
            {
                CometUIMainForm.SearchSettings.PrecursorMassTolerance = precursorMassTol;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.PrecursorMassUnit != precursorMassUnitCombo.SelectedIndex)
            {
                CometUIMainForm.SearchSettings.PrecursorMassUnit = precursorMassUnitCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.PrecursorMassType != precursorMassTypeCombo.SelectedIndex)
            {
                CometUIMainForm.SearchSettings.PrecursorMassType = precursorMassTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.PrecursorIsotopeError != precursorIsotopeErrorCombo.SelectedIndex)
            {
                CometUIMainForm.SearchSettings.PrecursorIsotopeError = precursorIsotopeErrorCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for fragment settings
            var fragmentBinSize = (double) fragmentBinSizeTextBox.DecimalValue;
            if (!CometUIMainForm.SearchSettings.FragmentBinSize.Equals(fragmentBinSize))
            {
                CometUIMainForm.SearchSettings.FragmentBinSize = fragmentBinSize;
                Parent.SettingsChanged = true;
            }

            var fragmentOffset = (double) fragmentOffsetTextBox.DecimalValue;
            if (!CometUIMainForm.SearchSettings.FragmentBinOffset.Equals(fragmentOffset))
            {
                CometUIMainForm.SearchSettings.FragmentBinOffset = fragmentOffset;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.FragmentMassType != fragmentMassTypeCombo.SelectedIndex)
            {
                CometUIMainForm.SearchSettings.FragmentMassType = fragmentMassTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseSparseMatrix != sparseMatrixCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseSparseMatrix = sparseMatrixCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for ion settings
            if (CometUIMainForm.SearchSettings.UseAIons != aIonCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseAIons = aIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseBIons != bIonCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseBIons = bIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseCIons != cIonCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseCIons = cIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseXIons != xIonCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseXIons = xIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseYIons != yIonCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseYIons = yIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseZIons != zIonCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseZIons = zIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.UseNLIons != useNLCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.UseNLIons = useNLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.TheoreticalFragmentIons != flankCheckBox.Checked)
            {
                CometUIMainForm.SearchSettings.TheoreticalFragmentIons = flankCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            // Set up defaults for the precursor mass settings
            precursorMassTolTextBox.Text = CometUIMainForm.SearchSettings.PrecursorMassTolerance.ToString(CultureInfo.InvariantCulture);
            precursorMassUnitCombo.SelectedIndex = CometUIMainForm.SearchSettings.PrecursorMassUnit;
            precursorMassTypeCombo.SelectedIndex = CometUIMainForm.SearchSettings.PrecursorMassType;
            precursorIsotopeErrorCombo.SelectedIndex = CometUIMainForm.SearchSettings.PrecursorIsotopeError;

            // Set up defaults for fragment settings
            fragmentBinSizeTextBox.Text = CometUIMainForm.SearchSettings.FragmentBinSize.ToString(CultureInfo.InvariantCulture);
            fragmentOffsetTextBox.Text = CometUIMainForm.SearchSettings.FragmentBinOffset.ToString(CultureInfo.InvariantCulture);
            fragmentMassTypeCombo.SelectedIndex = CometUIMainForm.SearchSettings.FragmentMassType;
            sparseMatrixCheckBox.Checked = CometUIMainForm.SearchSettings.UseSparseMatrix;
        
            // Set up defaults for ion settings
            aIonCheckBox.Checked = CometUIMainForm.SearchSettings.UseAIons;
            bIonCheckBox.Checked = CometUIMainForm.SearchSettings.UseBIons;
            cIonCheckBox.Checked = CometUIMainForm.SearchSettings.UseCIons;
            xIonCheckBox.Checked = CometUIMainForm.SearchSettings.UseXIons;
            yIonCheckBox.Checked = CometUIMainForm.SearchSettings.UseYIons;
            zIonCheckBox.Checked = CometUIMainForm.SearchSettings.UseZIons;
            useNLCheckBox.Checked = CometUIMainForm.SearchSettings.UseNLIons;
            flankCheckBox.Checked = CometUIMainForm.SearchSettings.TheoreticalFragmentIons;
        }
    }
}
