using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class MassSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        public MassSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            // Set up defaults for the precursor mass settings
            precursorMassTolTextBox.Text = Settings.Default.PrecursorMassTolerance.ToString(CultureInfo.InvariantCulture);
            precursorMassUnitCombo.SelectedItem =
                Settings.Default.PrecursorMassUnit.ToString(CultureInfo.InvariantCulture);
            precursorTolTypeCombo.SelectedItem = Settings.Default.PrecursorToleranceType;
            precursorMassTypeCombo.SelectedItem = Settings.Default.PrecursorMassType;
            precursorIsotopeErrorCombo.SelectedItem = Settings.Default.PrecursorIsotopeError;

            // Set up defaults for fragment settings
            fragmentBinSizeTextBox.Text = Settings.Default.FragmentBinSize.ToString(CultureInfo.InvariantCulture);
            fragmentOffsetTextBox.Text = Settings.Default.FragmentBinOffset.ToString(CultureInfo.InvariantCulture);
            fragmentMassTypeCombo.SelectedItem = Settings.Default.FragmentMassType;
        
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
    }
}
