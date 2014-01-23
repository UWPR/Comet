using System;
using System.Collections.Specialized;
using System.Windows.Forms;

namespace CometUI
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

        private void EnzymeInfoDataGridViewRowsAdded(object sender, DataGridViewRowsAddedEventArgs e)
        {
            EnzymeInfoChanged = true;
        }

        private void EnzymeInfoDataGridViewRowsRemoved(object sender, DataGridViewRowsRemovedEventArgs e)
        {
            EnzymeInfoChanged = true;
        }
    }
}
