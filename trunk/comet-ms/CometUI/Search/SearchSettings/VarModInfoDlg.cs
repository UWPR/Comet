using System;
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
{
    public partial class VarModInfoDlg : Form
    {
        public VarModInfoDlg(VarModSettingsControl varModSettingsControl, String varModName = null)
        {
            InitializeComponent();

            VarModSettingsControl = varModSettingsControl;

            InitializeWhichTermCombo();

            VarModName = varModName;
            InitializeVarModInfo();
            
            UpdateOKButton();
        }

        public String Title { set { Text = value; } }
        public int NamedVarModsListIndex { get; private set; }

        private String VarModName { get; set; }
        private VarModSettingsControl VarModSettingsControl { get; set; }

        private void InitializeWhichTermCombo()
        {
            whichTermCombo.Items.Add("N-term peptide");
            whichTermCombo.Items.Add("C-term peptide");

            whichTermCombo.Enabled = false;
        }

        private void InitializeVarModInfo()
        {
            if (String.IsNullOrEmpty(VarModName))
            {
                // If no VarModName was passed in, the user is trying to add
                // a new variable mod.
                InitializeAsAddVarMod();
            }
            else
            {
                // If a VarModName was passed in, the user is trying to edit
                // and existing variable mod.
                InitializeAsEditVarMod();
            }
        }

        private void InitializeAsAddVarMod()
        {
            for (int i = 0; i < VarModSettingsControl.NamedVarModsList.Count; i++)
            {
                // Grab the index of the first unused var mod item in the
                // list so we can save the new var mod in it
                var varMod = VarModSettingsControl.NamedVarModsList[i];
                if (varMod.VarModInfo.VarModChar.Equals("X"))
                {
                    NamedVarModsListIndex = i;
                }
            }
        }

        private void InitializeAsEditVarMod()
        {
            for (int i = 0; i < VarModSettingsControl.NamedVarModsList.Count; i++)
            {
                var varMod = VarModSettingsControl.NamedVarModsList[i];
                if (VarModName.Equals(varMod.Name))
                {
                    // Save the index of the mod in the list so we can
                    // edit it later
                    NamedVarModsListIndex = i;

                    residueTextBox.Text = varMod.VarModInfo.VarModChar;
                    massDiffNumericTextBox.Text = varMod.VarModInfo.VarModMass.ToString(CultureInfo.InvariantCulture);
                    maxModsNumericTextBox.Text =
                        varMod.VarModInfo.MaxNumVarModAAPerMod.ToString(CultureInfo.InvariantCulture);
                    isBinaryModCheckBox.Checked = varMod.VarModInfo.BinaryMod == 1;
                    requireModCheckBox.Checked = varMod.VarModInfo.RequireThisMod == 1;
                    int varModTermDist = varMod.VarModInfo.VarModTermDistance;
                    if (varModTermDist > -1)
                    {
                        termDistNumericTextBox.Text =
                            varMod.VarModInfo.VarModTermDistance.ToString(CultureInfo.InvariantCulture);
                        whichTermCombo.SelectedIndex = varMod.VarModInfo.WhichTerm;
                    }

                    break;
                }
            }
        }

        private void UpdateOKButton()
        {
            okBtn.Enabled = IsValidResidue() && IsValidMassDiff() && IsValidMaxMods() && IsValidWhichTerm();
        }

        private bool IsValidResidue()
        {
            return !String.IsNullOrEmpty(residueTextBox.Text);
        }

        private bool IsValidMassDiff()
        {
            return !String.IsNullOrEmpty(massDiffNumericTextBox.Text);
        }

        private bool IsValidMaxMods()
        {
            return !String.IsNullOrEmpty(maxModsNumericTextBox.Text)
                && (maxModsNumericTextBox.IntValue >= 0)
                && (maxModsNumericTextBox.IntValue <= 64);
        }

        private bool IsValidWhichTerm()
        {
            if (!whichTermCombo.Enabled)
            {
                return true;
            }

            return whichTermCombo.SelectedIndex > -1;
        }

        private void CancelBtnClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void OKBtnClick(object sender, EventArgs e)
        {
            String varModChar = residueTextBox.Text;
            var varModMassDiff = (double) massDiffNumericTextBox.DecimalValue;
            var maxVarMods = maxModsNumericTextBox.IntValue;
            var isBinaryMod = isBinaryModCheckBox.Checked;
            int termDist = -1;
            if (!String.IsNullOrEmpty(termDistNumericTextBox.Text))
            {
                termDist = termDistNumericTextBox.IntValue;
            }

            int whichTerm = 0;
            if (whichTermCombo.Enabled)
            {
                whichTerm = whichTermCombo.SelectedIndex;
            }

            var requireThisMod = requireModCheckBox.Checked;

            var newVarMod = new VarMod(varModMassDiff, varModChar, isBinaryMod ? 1 : 0, maxVarMods, termDist, whichTerm, requireThisMod ? 1: 0);
            VarModSettingsControl.NamedVarModsList[NamedVarModsListIndex].VarModInfo = newVarMod;
            VarModSettingsControl.NamedVarModsList[NamedVarModsListIndex].Name =
                VarModSettingsControl.GetVarModName(newVarMod);
            DialogResult = DialogResult.OK;
        }

        private void ResidueTextBoxKeyPress(object sender, KeyPressEventArgs e)
        {
            if (VarModSettingsControl.IsValidAA(e.KeyChar))
            {
                // If the key pressed is a valid amino acid, we're good
            }
            else if (e.KeyChar == '\b')
            {
                // Allow a backspace
            }
            else
            {
                e.Handled = true;
            }
        }

        private void TermDistNumericTextBoxTextChanged(object sender, EventArgs e)
        {
            whichTermCombo.Enabled = !String.IsNullOrEmpty(termDistNumericTextBox.Text);
            if (whichTermCombo.Enabled)
            {
                varModInfoDlgToolTip.Show("Please specify which terminus the distance constraint should be applied to (N-term or C-term).", this, whichTermCombo.Location, 5000);
            }
            UpdateOKButton();
        }

        private void WhichTermComboSelectedIndexChanged(object sender, EventArgs e)
        {
            UpdateOKButton();
        }

        private void ResidueTextBoxTextChanged(object sender, EventArgs e)
        {
            UpdateOKButton();
        }

        private void MassDiffNumericTextBoxTextChanged(object sender, EventArgs e)
        {
            UpdateOKButton();
        }

        private void MaxModsNumericTextBoxTextChanged(object sender, EventArgs e)
        {
            UpdateOKButton();
        }
    }
}
