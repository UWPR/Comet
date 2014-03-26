using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

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
