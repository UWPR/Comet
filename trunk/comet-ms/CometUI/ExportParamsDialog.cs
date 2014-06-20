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
    public partial class ExportParamsDlg : Form
    {
        public ExportParamsDlg()
        {
            InitializeComponent();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void btnFindLocation_Click(object sender, EventArgs e)
        {
            var paramsFolderBrowserDialog = new FolderBrowserDialog();

            if (paramsFolderBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                textBoxLocation.Text = paramsFolderBrowserDialog.SelectedPath;

            }
        }

        private void btnExport_Click(object sender, EventArgs e)
        {

        }
    }
}
