using System;
using System.Windows.Forms;

namespace CometUI
{
    public partial class ExportParamsDlg : Form
    {
        public ExportParamsDlg()
        {
            InitializeComponent();
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnExportPathClick(object sender, EventArgs e)
        {
            var paramsFolderBrowserDialog = new FolderBrowserDialog();

            if (paramsFolderBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                textBoxLocation.Text = paramsFolderBrowserDialog.SelectedPath;
            }
        }
    }
}
