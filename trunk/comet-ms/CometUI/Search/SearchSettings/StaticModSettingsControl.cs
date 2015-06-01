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

using System;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.Search.SearchSettings
{
    public partial class StaticModSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }
        public StringCollection StaticMods { get; set; }

        public StaticModSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
        }

        public void Initialize()
        {
            InitializeFromDefaultSettings();
            UpdateStatidModsDataGridView();
        }

        public bool VerifyAndUpdateSettings()
        {
            StaticMods = StaticModsDataGridViewToStringCollection();
            for (int i = 0; i < StaticMods.Count; i++ )
            {
                if (!StaticMods[i].Equals(CometUIMainForm.SearchSettings.StaticMods[i]))
                {
                    CometUIMainForm.SearchSettings.StaticMods = StaticMods;
                    Parent.SettingsChanged = true;
                    break;
                }
            }

            var staticNTermPeptide = (double)staticNTermPeptideTextBox.DecimalValue;
            if (!staticNTermPeptide.Equals(CometUIMainForm.SearchSettings.StaticModNTermPeptide))
            {
                CometUIMainForm.SearchSettings.StaticModNTermPeptide = staticNTermPeptide;
                Parent.SettingsChanged = true;
            }

            var staticCTermPeptide = (double) staticCTermPeptideTextBox.DecimalValue;
            if (!staticCTermPeptide.Equals(CometUIMainForm.SearchSettings.StaticModCTermPeptide))
            {
                CometUIMainForm.SearchSettings.StaticModCTermPeptide = staticCTermPeptide;
                Parent.SettingsChanged = true;
            }

            var staticNTermProtein = (double)staticNTermProteinTextBox.DecimalValue;
            if (!staticNTermProtein.Equals(CometUIMainForm.SearchSettings.StaticModNTermProtein))
            {
                CometUIMainForm.SearchSettings.StaticModNTermProtein = staticNTermProtein;
                Parent.SettingsChanged = true;
            }

            var staticCTermProtein = (double) staticCTermProteinTextBox.DecimalValue;
            if (!staticCTermProtein.Equals(CometUIMainForm.SearchSettings.StaticModCTermProtein))
            {
                CometUIMainForm.SearchSettings.StaticModCTermProtein = staticCTermProtein;
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
            foreach (var item in CometUIMainForm.SearchSettings.StaticMods)
            {
                StaticMods.Add(item);
            }

            staticNTermPeptideTextBox.Text = CometUIMainForm.SearchSettings.StaticModNTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticCTermPeptideTextBox.Text = CometUIMainForm.SearchSettings.StaticModCTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticNTermProteinTextBox.Text = CometUIMainForm.SearchSettings.StaticModNTermProtein.ToString(CultureInfo.InvariantCulture);
            staticCTermProteinTextBox.Text = CometUIMainForm.SearchSettings.StaticModCTermProtein.ToString(CultureInfo.InvariantCulture);
        }

        private void UpdateStatidModsDataGridView()
        {
            staticModsDataGridView.Rows.Clear();
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
                        Convert.ToDouble(strValue);
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
