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
        private new SearchSettingsDlg Parent { get; set; }

        private const string AminoAcids = "GASPVTCLINDQKEMOHFRYW";


        public VarModSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            UpdateVarModsDataGridView();
        }

        public bool VerifyAndUpdateSettings()
        {
            VarMods = VarModsDataGridViewToStringCollection();
            for (int i = 0; i < VarMods.Count; i++ )
            {
                if (!VarMods[i].Equals(CometUI.SearchSettings.VariableMods[i]))
                {
                    Parent.SettingsChanged = true;
                    CometUI.SearchSettings.VariableMods = VarMods;
                    break;
                }
            }

            int maxModsInPeptide = (int)maxModsInPeptideTextBox.Value;
            if (!maxModsInPeptide.Equals(CometUI.SearchSettings.MaxVarModsInPeptide))
            {
                CometUI.SearchSettings.MaxVarModsInPeptide = maxModsInPeptide;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            VarMods = new StringCollection();
            foreach (var item in CometUI.SearchSettings.VariableMods)
            {
                VarMods.Add(item);
            }

            maxModsInPeptideTextBox.Text = CometUI.SearchSettings.MaxVarModsInPeptide.ToString(CultureInfo.InvariantCulture);            
        }

        private StringCollection VarModsDataGridViewToStringCollection()
        {
            var strCollection = new StringCollection();
            for (int rowIndex = 0; rowIndex < varModsDataGridView.Rows.Count; rowIndex++)
            {
                var dataGridViewRow = varModsDataGridView.Rows[rowIndex];
                string row = String.Empty;
                for (int colIndex = 0; colIndex < dataGridViewRow.Cells.Count; colIndex++)
                {
                    var textBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewTextBoxCell;
                    if (null != textBoxCell)
                    {
                        row += textBoxCell.Value;
                        if (colIndex != dataGridViewRow.Cells.Count - 1)
                        {
                            row += ",";
                        }
                        continue;
                    }

                    var checkBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewCheckBoxCell;
                    if (null != checkBoxCell)
                    {
                        row += (bool)checkBoxCell.Value ? "1" : "0";
                        if (colIndex != dataGridViewRow.Cells.Count - 1)
                        {
                            row += ",";
                        }
                        continue;
                    }

                    var dropDownCell = dataGridViewRow.Cells[colIndex] as DataGridViewComboBoxCell;
                    if (null != dropDownCell)
                    {
                        row += dropDownCell.Value;
                        if (colIndex != dataGridViewRow.Cells.Count - 1)
                        {
                            row += ",";
                        }
                    }
                }
                strCollection.Add(row);
            }

            return strCollection;
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
                    if (cellColTitle.Equals("Bin Mod"))
                    {
                        var checkBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewCheckBoxCell;
                        if (null != checkBoxCell)
                        {
                            checkBoxCell.Value = varModsCells[colIndex].Equals("1");
                        }
                    }
                    else if (cellColTitle.Equals("Which Term"))
                    {
                        var dropDownCell = dataGridViewRow.Cells[colIndex] as DataGridViewComboBoxCell;
                        if (null != dropDownCell)
                        {
                            dropDownCell.Value = varModsCells[colIndex];
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
                ValidateResidueCellEdit(cell);
            }
            else if (cell.OwningColumn.HeaderText.Equals("Mass Diff"))
            {
                ValidateMassDiffCellEdit(cell);
            }
            else if (cell.OwningColumn.HeaderText.Equals("Max Mods"))
            {
                ValidateMaxModsCellEdit(cell);
            }
            else if (cell.OwningColumn.HeaderText.Equals("Term Dist"))
            {
                ValidateTermDistCellEdit(cell);
            }
        }

        private void ValidateResidueCellEdit(DataGridViewCell cell)
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

        private void ValidateMassDiffCellEdit(DataGridViewCell cell)
        {
            var textBoxCell = cell as DataGridViewTextBoxCell;
            if (textBoxCell != null)
            {
                string strValue = textBoxCell.Value.ToString();
                double massDiff;
                if (!SearchSettingsDlg.ConvertStrToDouble(strValue, out massDiff))
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

        private void ValidateMaxModsCellEdit(DataGridViewCell cell)
        {
            // Do we want to make this a combo box drop-down?
            var textBoxCell = cell as DataGridViewTextBoxCell;
            if (textBoxCell != null)
            {
                string strValue = textBoxCell.Value.ToString();
                int maxMods;
                if (!SearchSettingsDlg.ConvertStrToInt32(strValue, out maxMods))
                {
                    MessageBox.Show(this, Resources.VarModSettingsControl_VarModsDataGridViewCellEndEdit_Please_enter_a_valid_number_between_0_and_64_, Resources.VarModSettingsControl_VarModsDataGridViewCellEndEdit_Invalid_Max_Mods, MessageBoxButtons.OKCancel);
                    cell.Value = "3";
                }

                if (maxMods > 64)
                {
                    throw new ArgumentException();
                }
            }
        }

        private void ValidateTermDistCellEdit(DataGridViewCell cell)
        {
            var textBoxCell = cell as DataGridViewTextBoxCell;
            if (textBoxCell != null)
            {
                string strValue = textBoxCell.Value.ToString();
                int maxMods;
                if (!SearchSettingsDlg.ConvertStrToInt32(strValue, out maxMods))
                {
                    MessageBox.Show(this,
                                    Resources.VarModSettingsControl_ValidateTermDistCellEdit_Please_enter_a_decimal_number_,
                                    Resources.VarModSettingsControl_ValidateTermDistCellEdit_Invalid_Terminus_Distance,
                                    MessageBoxButtons.OKCancel);
                    cell.Value = "-1";
                }
            }
        }
    }
}
