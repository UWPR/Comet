using System;
using System.IO;
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
                textBoxPath.Text = paramsFolderBrowserDialog.SelectedPath;
            }
        }

        private void BtnExportClick(object sender, EventArgs e)
        {
            
        }

        private void ExportTextChange()
        {
            string fileName = textBoxName.Text;
            string filePath = textBoxPath.Text;

            btnExport.Enabled = (fileName != string.Empty) && Directory.Exists(filePath);
        }

        private void TextBoxNameTextChanged(object sender, EventArgs e)
        {
            ExportTextChange();
        }

        private void TextBoxPathTextChanged(object sender, EventArgs e)
        {
            ExportTextChange();
        }
    }
}
