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
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
{
    public partial class EnzymeInfoDlg : Form
    {
        public string SelectedEnzymeName { get; set; }
        public bool EnzymeInfoChanged { get; private set; }

        private EnzymeSettingsControl EnzymeSettingsDlg { get; set; }
    
        public EnzymeInfoDlg(EnzymeSettingsControl enzymeSettings)
        {
            InitializeComponent();

            EnzymeSettingsDlg = enzymeSettings;

            EnzymeInfoChanged = false;

            foreach (var row in EnzymeSettingsDlg.EnzymeInfo)
            {
                string[] cells = row.Split(',');
                enzymeInfoDataGridView.Rows.Add(cells);
            }
        }

        private void EnzymeInfoOkButtonClick(object sender, EventArgs e)
        {
            if (EnzymeInfoChanged)
            {
                var newEnzymeInfo = new StringCollection();
                foreach (DataGridViewRow row in enzymeInfoDataGridView.Rows)
                {
                    int numColumns = enzymeInfoDataGridView.ColumnCount;
                    string newEnzymeInfoItem = String.Empty;
                    bool isValidRow = true;
                    for (int i = 0; i < numColumns; i++)
                    {
                        if ((string) row.Cells[i].Value == null)
                        {
                            isValidRow = false;
                            break;
                        }
                        newEnzymeInfoItem += row.Cells[i].Value;
                        if (i != numColumns - 1)
                        {
                            newEnzymeInfoItem += ",";
                        }
                    }

                    if (isValidRow)
                    {
                        newEnzymeInfo.Add(newEnzymeInfoItem);
                    }
                }

                EnzymeSettingsDlg.EnzymeInfo = newEnzymeInfo;
            }

            DialogResult = DialogResult.OK;
        }

        private void EnzymeInfoCancelButtonClick(object sender, EventArgs e)
        {
            EnzymeInfoChanged = false;
            DialogResult = DialogResult.Cancel;
        }

        private void EnzymeInfoDataGridViewCellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            EnzymeInfoChanged = true;
        }
    }
}
