using System;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class StaticModSettingsControl : UserControl
    {
        private new Form Parent { get; set; }
        public StringCollection StaticMods { get; set; }

        public StaticModSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            UpdateStatidModsDataGridView();
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
                    try
                    {
                        double massDiff = Convert.ToDouble(strValue);
                    }
                    catch (Exception)
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
