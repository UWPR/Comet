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
            if (!CometUI.SearchSettings.PrecursorMassTolerance.Equals(precursorMassTol))
            {
                CometUI.SearchSettings.PrecursorMassTolerance = precursorMassTol;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.PrecursorMassUnit != precursorMassUnitCombo.SelectedIndex)
            {
                CometUI.SearchSettings.PrecursorMassUnit = precursorMassUnitCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.PrecursorMassType != precursorMassTypeCombo.SelectedIndex)
            {
                CometUI.SearchSettings.PrecursorMassType = precursorMassTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.PrecursorIsotopeError != precursorIsotopeErrorCombo.SelectedIndex)
            {
                CometUI.SearchSettings.PrecursorIsotopeError = precursorIsotopeErrorCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for fragment settings
            var fragmentBinSize = (double) fragmentBinSizeTextBox.DecimalValue;
            if (!CometUI.SearchSettings.FragmentBinSize.Equals(fragmentBinSize))
            {
                CometUI.SearchSettings.FragmentBinSize = fragmentBinSize;
                Parent.SettingsChanged = true;
            }

            var fragmentOffset = (double) fragmentOffsetTextBox.DecimalValue;
            if (!CometUI.SearchSettings.FragmentBinOffset.Equals(fragmentOffset))
            {
                CometUI.SearchSettings.FragmentBinOffset = fragmentOffset;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.FragmentMassType != fragmentMassTypeCombo.SelectedIndex)
            {
                CometUI.SearchSettings.FragmentMassType = fragmentMassTypeCombo.SelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseSparseMatrix != sparseMatrixCheckBox.Checked)
            {
                CometUI.SearchSettings.UseSparseMatrix = sparseMatrixCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for ion settings
            if (CometUI.SearchSettings.UseAIons != aIonCheckBox.Checked)
            {
                CometUI.SearchSettings.UseAIons = aIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseBIons != bIonCheckBox.Checked)
            {
                CometUI.SearchSettings.UseBIons = bIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseCIons != cIonCheckBox.Checked)
            {
                CometUI.SearchSettings.UseCIons = cIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseXIons != xIonCheckBox.Checked)
            {
                CometUI.SearchSettings.UseXIons = xIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseYIons != yIonCheckBox.Checked)
            {
                CometUI.SearchSettings.UseYIons = yIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseZIons != zIonCheckBox.Checked)
            {
                CometUI.SearchSettings.UseZIons = zIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.UseNLIons != useNLCheckBox.Checked)
            {
                CometUI.SearchSettings.UseNLIons = useNLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (CometUI.SearchSettings.TheoreticalFragmentIons != flankCheckBox.Checked)
            {
                CometUI.SearchSettings.TheoreticalFragmentIons = flankCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            // Set up defaults for the precursor mass settings
            precursorMassTolTextBox.Text = CometUI.SearchSettings.PrecursorMassTolerance.ToString(CultureInfo.InvariantCulture);
            precursorMassUnitCombo.SelectedIndex = CometUI.SearchSettings.PrecursorMassUnit;
            precursorMassTypeCombo.SelectedIndex = CometUI.SearchSettings.PrecursorMassType;
            precursorIsotopeErrorCombo.SelectedIndex = CometUI.SearchSettings.PrecursorIsotopeError;

            // Set up defaults for fragment settings
            fragmentBinSizeTextBox.Text = CometUI.SearchSettings.FragmentBinSize.ToString(CultureInfo.InvariantCulture);
            fragmentOffsetTextBox.Text = CometUI.SearchSettings.FragmentBinOffset.ToString(CultureInfo.InvariantCulture);
            fragmentMassTypeCombo.SelectedIndex = CometUI.SearchSettings.FragmentMassType;
            sparseMatrixCheckBox.Checked = CometUI.SearchSettings.UseSparseMatrix;
        
            // Set up defaults for ion settings
            aIonCheckBox.Checked = CometUI.SearchSettings.UseAIons;
            bIonCheckBox.Checked = CometUI.SearchSettings.UseBIons;
            cIonCheckBox.Checked = CometUI.SearchSettings.UseCIons;
            xIonCheckBox.Checked = CometUI.SearchSettings.UseXIons;
            yIonCheckBox.Checked = CometUI.SearchSettings.UseYIons;
            zIonCheckBox.Checked = CometUI.SearchSettings.UseZIons;
            useNLCheckBox.Checked = CometUI.SearchSettings.UseNLIons;
            flankCheckBox.Checked = CometUI.SearchSettings.TheoreticalFragmentIons;
        }
    }
}
