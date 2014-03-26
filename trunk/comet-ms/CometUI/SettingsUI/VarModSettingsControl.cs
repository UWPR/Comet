using System;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class VarModSettingsControl : UserControl
    {
        public StringCollection VarMods { get; set; }
        private new Form Parent { get; set; }

        private const string AminoAcids = "GASPVTCLINDQKEMOHFRYW";


        public VarModSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            UpdateVarModsDataGridView();
        }

        private void InitializeFromDefaultSettings()
        {
            VarMods = new StringCollection();
            foreach (var item in Settings.Default.VariableMods)
            {
                VarMods.Add(item);
            }

            variableNTerminusTextBox.Text = Settings.Default.VariableNTerminus.ToString(CultureInfo.InvariantCulture);
            variableCTerminusTextBox.Text = Settings.Default.VariableCTerminus.ToString(CultureInfo.InvariantCulture);
            variableNTerminusDistTextBox.Text = Settings.Default.VariableNTermDistance.ToString(CultureInfo.InvariantCulture);
            variableCTerminusDistTextBox.Text = Settings.Default.VariableCTermDistance.ToString(CultureInfo.InvariantCulture);

            maxModsInPeptideTextBox.Text = Settings.Default.MaxVarModsInPeptide.ToString(CultureInfo.InvariantCulture);            
        }

        private void UpdateVarModsDataGridView()
        {
            varModsDataGridView.Rows.Add(VarMods.Count);
            for (int rowIndex = 0; rowIndex < VarMods.Count; rowIndex++)
            {
                var varModsRow = VarMods[rowIndex];
                string[] varModsCells = varModsRow.Split(',');
                var dataGridViewRow = varModsDataGridView.Rows[rowIndex];
                for (int colIndex = 0; colIndex < dataGridViewRow.Cells.Count; colIndex++)
                {
                    string cellColTitle = dataGridViewRow.Cells[colIndex].OwningColumn.HeaderText;
                    if (cellColTitle.Equals("Binary Mod"))
                    {
                        var checkBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewCheckBoxCell;
                        if (null != checkBoxCell)
                        {
                            checkBoxCell.Value = varModsCells[colIndex].Equals("1");
                        }
                    }
                    else
                    {
                        var textBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewTextBoxCell;
                        if (null != textBoxCell)
                        {
                            textBoxCell.Value = varModsCells[colIndex];
                        }
                    }
                }
            }
        }

        private void VarModsDataGridViewCellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            var cell = varModsDataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
            if (cell.OwningColumn.HeaderText.Equals("Residue"))
            {
                var textBoxCell = cell as DataGridViewTextBoxCell;
                if (textBoxCell != null)
                {
                    if (!textBoxCell.Value.ToString().ToUpper().Equals("X"))
                    {
                        char[] residue = textBoxCell.Value.ToString().ToUpper().ToCharArray();
                        foreach (var aa in residue)
                        {
                            if (!AminoAcids.Contains(aa.ToString(CultureInfo.InvariantCulture)))
                            {
                                MessageBox.Show(this,
                                                Resources.
                                                    VarModSettingsControl_VarModsDataGridViewCellEndEdit_Please_enter_a_valid_residue_,
                                                Resources.
                                                    VarModSettingsControl_VarModsDataGridViewCellEndEdit_Invalid_Residue,
                                                MessageBoxButtons.OKCancel);
                                cell.Value = "X";
                            }
                        }
                    }
                }
            }
            else if (cell.OwningColumn.HeaderText.Equals("Mass Diff"))
            {
                var textBoxCell = cell as DataGridViewTextBoxCell;
                if (textBoxCell != null)
                {
                    string strValue = textBoxCell.Value.ToString();
                    try
                    {
                        double massDiff = Convert.ToDouble(strValue);
                    }
                    catch (Exception)
                    {

                        MessageBox.Show(this,
                                        Resources.
                                            VarModSettingsControl_VarModsDataGridViewCellEndEdit_Please_enter_a_valid_number_for_the_mass_difference_,
                                        Resources.VarModSettingsControl_VarModsDataGridViewCellEndEdit_Invalid_Mass_Diff,
                                        MessageBoxButtons.OKCancel);
                        cell.Value = "0.0";
                    }

                }
            }
            else if (cell.OwningColumn.HeaderText.Equals("Max Mods"))
            {
                // Do we want to make this a combo box drop-down?
                var textBoxCell = cell as DataGridViewTextBoxCell;
                if (textBoxCell != null)
                {
                    string strValue = textBoxCell.Value.ToString();
                    try
                    {
                        var maxMods = (int) Convert.ToUInt16(strValue);
                        if (maxMods > 64)
                        {
                            throw new ArgumentException();
                        }
                    }
                    catch (Exception)
                    {

                        MessageBox.Show(this,Resources.VarModSettingsControl_VarModsDataGridViewCellEndEdit_Please_enter_a_valid_number_between_0_and_64_, Resources.VarModSettingsControl_VarModsDataGridViewCellEndEdit_Invalid_Max_Mods, MessageBoxButtons.OKCancel);
                        cell.Value = "3";
                    }
                }
            }
        }
    }
}
