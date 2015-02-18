using System;
using System.IO;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class FindFileDlg : Form
    {
        public String FileName { get; set; }
        public String OpenFileDlgTitle { get; set; }
        public String OpenFileDlgFilter { get; set; }

        public String DlgTitle
        {
            get { return Text; }
            set { Text = value; }
        }

        public String FileComboLabel
        {
            get { return findFileLabel.Text; }
            set { findFileLabel.Text = value; }
        }

        public FindFileDlg()
        {
            InitializeComponent();

            DlgTitle = "Find File";
            FileComboLabel = "File Name:";

            OpenFileDlgTitle = "Open File";
            OpenFileDlgFilter = "All Files (*.*)|*.*";
            btnOK.Enabled = false;
        }

        private void BtnBrowseSearchDBFileClick(object sender, EventArgs e)
        {
            var findFileOpenFileDialog = new OpenFileDialog
            {
                Title = OpenFileDlgTitle,
                InitialDirectory = @".",
                Filter = OpenFileDlgFilter,
                Multiselect = false,
                RestoreDirectory = true
            };

            if (findFileOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                findFileCombo.Text = findFileOpenFileDialog.FileName;
            }

            string path = findFileCombo.Text;
            if (File.Exists(path))
            {
                FileName = path;
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