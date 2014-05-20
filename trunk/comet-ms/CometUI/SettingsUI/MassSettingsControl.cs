using System;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class MassSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }

        public MassSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            // Set up defaults for the precursor mass settings
            precursorMassTolTextBox.Text = Settings.Default.PrecursorMassTolerance.ToString(CultureInfo.InvariantCulture);
            precursorMassUnitCombo.SelectedItem = Settings.Default.PrecursorMassUnit;
            precursorTolTypeCombo.SelectedItem = Settings.Default.PrecursorToleranceType;
            precursorMassTypeCombo.SelectedItem = Settings.Default.PrecursorMassType;
            precursorIsotopeErrorCombo.SelectedItem = Settings.Default.PrecursorIsotopeError;

            // Set up defaults for fragment settings
            fragmentBinSizeTextBox.Text = Settings.Default.FragmentBinSize.ToString(CultureInfo.InvariantCulture);
            fragmentOffsetTextBox.Text = Settings.Default.FragmentBinOffset.ToString(CultureInfo.InvariantCulture);
            fragmentMassTypeCombo.SelectedItem = Settings.Default.FragmentMassType;
            sparseMatrixCheckBox.Checked = Settings.Default.UseSparseMatrix;
        
            // Set up defaults for ion settings
            aIonCheckBox.Checked = Settings.Default.UseAIons;
            bIonCheckBox.Checked = Settings.Default.UseBIons;
            cIonCheckBox.Checked = Settings.Default.UseCIons;
            xIonCheckBox.Checked = Settings.Default.UseXIons;
            yIonCheckBox.Checked = Settings.Default.UseYIons;
            zIonCheckBox.Checked = Settings.Default.UseZIons;
            useNLCheckBox.Checked = Settings.Default.UseNLIons;
            flankCheckBox.Checked = Settings.Default.TheoreticalFragmentIons;
        }

        public bool VerifyAndUpdateSettings()
        {
            // Verify and save the precursor mass settings
            double precursorMassTol;
            if (!Convert(precursorMassTolTextBox.Text, out precursorMassTol))
            {
                return false;
            }
            if (!Settings.Default.PrecursorMassTolerance.Equals(precursorMassTol))
            {
                Settings.Default.PrecursorMassTolerance = precursorMassTol;
                Parent.SettingsChanged = true;
            }

            if (!Settings.Default.PrecursorMassUnit.Equals(precursorMassUnitCombo.SelectedItem))
            {
                Settings.Default.PrecursorMassUnit = precursorMassUnitCombo.SelectedItem.ToString();
                Parent.SettingsChanged = true;
            }

            if (!Settings.Default.PrecursorToleranceType.Equals(precursorTolTypeCombo.SelectedItem))
            {
                Settings.Default.PrecursorToleranceType = precursorTolTypeCombo.SelectedItem.ToString();
                Parent.SettingsChanged = true;
            }

            if (!Settings.Default.PrecursorMassType.Equals(precursorMassTypeCombo.SelectedItem))
            {
                Settings.Default.PrecursorMassType = precursorMassTypeCombo.SelectedItem.ToString();
                Parent.SettingsChanged = true;
            }

            if (!Settings.Default.PrecursorIsotopeError.Equals(precursorIsotopeErrorCombo.SelectedItem))
            {
                Settings.Default.PrecursorIsotopeError = precursorIsotopeErrorCombo.SelectedItem.ToString();
                Parent.SettingsChanged = true;
            }

            // Set up defaults for fragment settings
            double fragmentBinSize;
            if (!Convert(fragmentBinSizeTextBox.Text, out fragmentBinSize))
            {
                return false;
            }
            if (!Settings.Default.FragmentBinSize.Equals(fragmentBinSize))
            {
                Settings.Default.FragmentBinSize = fragmentBinSize;
                Parent.SettingsChanged = true;
            }

            double fragmentOffset;
            if (!Convert(fragmentOffsetTextBox.Text, out fragmentOffset))
            {
                return false;
            }
            if (!Settings.Default.FragmentBinOffset.Equals(fragmentOffset))
            {
                Settings.Default.FragmentBinOffset = fragmentOffset;
                Parent.SettingsChanged = true;
            }

            if (!Settings.Default.FragmentMassType.Equals(fragmentMassTypeCombo.SelectedItem))
            {
                Settings.Default.FragmentMassType = fragmentMassTypeCombo.SelectedItem.ToString();
                Parent.SettingsChanged = true;
             }

            if (Settings.Default.UseSparseMatrix != sparseMatrixCheckBox.Checked)
            {
                Settings.Default.UseSparseMatrix = sparseMatrixCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            // Set up defaults for ion settings
            if (Settings.Default.UseAIons != aIonCheckBox.Checked)
            {
                Settings.Default.UseAIons = aIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.UseBIons != bIonCheckBox.Checked)
            {
                Settings.Default.UseBIons = bIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.UseCIons != cIonCheckBox.Checked)
            {
                Settings.Default.UseCIons = cIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.UseXIons != xIonCheckBox.Checked)
            {
                Settings.Default.UseXIons = xIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.UseYIons != yIonCheckBox.Checked)
            {
                Settings.Default.UseYIons = yIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.UseZIons != zIonCheckBox.Checked)
            {
                Settings.Default.UseZIons = zIonCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.UseNLIons != useNLCheckBox.Checked)
            {
                Settings.Default.UseNLIons = useNLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (Settings.Default.TheoreticalFragmentIons != flankCheckBox.Checked)
            {
                Settings.Default.TheoreticalFragmentIons = flankCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private bool Convert(string strValue, out double doubleValueOut)
        {
            var doubleValue = 0.0;
            try
            {
                doubleValue = System.Convert.ToDouble(strValue);
            }
            catch (Exception)
            {
                return false;
            }
            finally
            {
                doubleValueOut = doubleValue;
            }

            return true;
        }
    }
}
