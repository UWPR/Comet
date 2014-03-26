using System;
using System.Windows.Forms;
using CometUI.SettingsUI;

namespace CometUI
{
    public partial class CometUI : Form
    {
        public CometUI()
        {
            InitializeComponent();
        }

        private void SearchSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
                // Do something here?  Maybe save the settings?
            }
        }
    }
}
