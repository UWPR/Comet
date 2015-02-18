using System;
using System.IO;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.ViewResults
{
    public partial class FindProteinDBDlg : Form
    {
        public String SearchDBFile { get; set; }

        public FindProteinDBDlg()
        {
            InitializeComponent();

            btnOK.Enabled = false;
        }

        private void BtnBrowseSearchDBFileClick(object sender, EventArgs e)
        {
            var findProteinDBOpenFileDialog = new OpenFileDialog
            {
                Title = Resources.FindProteinDBDlg_BtnBrowseSearchDBFileClick_Open_Protein_Database_File,
                InitialDirectory = @".",
                Filter = Resources.FindProteinDBDlg_BtnBrowseSearchDBFileClick_All_Files__________,
                Multiselect = false,
                RestoreDirectory = true
            };

            if (findProteinDBOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                searchDBFileCombo.Text = findProteinDBOpenFileDialog.FileName;
            }

            string path = searchDBFileCombo.Text;
            if (File.Exists(path))
            {
                SearchDBFile = path;
                btnOK.Enabled = true;
            }

        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnOKClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }
    }
}
