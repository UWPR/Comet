using System;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class StaticModSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }
        public StringCollection StaticMods { get; set; }

        public StaticModSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            UpdateStatidModsDataGridView();
        }

        public bool VerifyAndUpdateSettings()
        {
            StaticMods = StaticModsDataGridViewToStringCollection();
            if (!StaticMods.Equals(Settings.Default.StaticMods))
            {
                Settings.Default.StaticMods = StaticMods;
                Parent.SettingsChanged = true;
            }

            double staticNTermPeptide;
            if (!SearchSettingsDlg.ConvertStrToDouble(staticNTermPeptideTextBox.Text, out staticNTermPeptide))
            {
                return false;
            }
            if (!staticNTermPeptide.Equals(Settings.Default.StaticModNTermPeptide))
            {
                Settings.Default.StaticModNTermPeptide = staticNTermPeptide;
                Parent.SettingsChanged = true;
            }

            double staticCTermPeptide;
            if (!SearchSettingsDlg.ConvertStrToDouble(staticCTermPeptideTextBox.Text, out staticCTermPeptide))
            {
                return false;
            }
            if (!staticCTermPeptide.Equals(Settings.Default.StaticModCTermPeptide))
            {
                Settings.Default.StaticModCTermPeptide = staticCTermPeptide;
                Parent.SettingsChanged = true;
            }

            double staticNTermProtein;
            if (!SearchSettingsDlg.ConvertStrToDouble(staticNTermProteinTextBox.Text, out staticNTermProtein))
            {
                return false;
            }
            if (!staticNTermProtein.Equals(Settings.Default.StaticModNTermProtein))
            {
                Settings.Default.StaticModNTermProtein = staticNTermProtein;
                Parent.SettingsChanged = true;
            }

            double staticCTermProtein;
            if (!SearchSettingsDlg.ConvertStrToDouble(staticCTermProteinTextBox.Text, out staticCTermProtein))
            {
                return false;
            }
            if (!staticCTermProtein.Equals(Settings.Default.StaticModCTermProtein))
            {
                Settings.Default.StaticModCTermProtein = staticCTermProtein;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private StringCollection StaticModsDataGridViewToStringCollection()
        {
            var strCollection = new StringCollection();
            for (int rowIndex = 0; rowIndex < staticModsDataGridView.Rows.Count; rowIndex++)
            {
                var dataGridViewRow = staticModsDataGridView.Rows[rowIndex];
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
                    }
                }
                strCollection.Add(row);
            }

            return strCollection;
        }

        private void InitializeFromDefaultSettings()
        {
            StaticMods = new StringCollection();
            foreach (var item in Settings.Default.StaticMods)
            {
                StaticMods.Add(item);
            }

            staticNTermPeptideTextBox.Text = Settings.Default.StaticModNTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticCTermPeptideTextBox.Text = Settings.Default.StaticModCTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticNTermProteinTextBox.Text = Settings.Default.StaticModNTermProtein.ToString(CultureInfo.InvariantCulture);
            staticCTermProteinTextBox.Text = Settings.Default.StaticModCTermProtein.ToString(CultureInfo.InvariantCulture);
        }

        private void UpdateStatidModsDataGridView()
        {
            staticModsDataGridView.Rows.Add(StaticMods.Count);
            for (int rowIndex = 0; rowIndex < StaticMods.Count; rowIndex++)
            {
                var staticModsRow = StaticMods[rowIndex];
                string[] staticModsCells = staticModsRow.Split(',');
                var dataGridViewRow = staticModsDataGridView.Rows[rowIndex];
                for (int colIndex = 0; colIndex < dataGridViewRow.Cells.Count; colIndex++)
                {
                    var textBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewTextBoxCell;
                    if (null != textBoxCell)
                    {
                        textBoxCell.Value = staticModsCells[colIndex];
                    }
                }
            }
        }

        private void StaticModsDataGridViewCellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            var cell = staticModsDataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
            if (cell.OwningColumn.HeaderText.Equals("Mass Diff"))
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
                                          StaticModSettingsControl_StaticModsDataGridViewCellEndEdit_Please_enter_a_valid_number_for_the_mass_difference_,
                                      Resources.
                                          StaticModSettingsControl_StaticModsDataGridViewCellEndEdit_Invalid_Mass_Diff,
                                      MessageBoxButtons.OKCancel);
                        cell.Value = "0.0000";
                    }
                }
            }
        }
    }
}
