using System;
using System.IO;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SharedUI
{
    public partial class ExportFileDlg : Form
    {
        public String FileExtension { get; set; }
        public String FileFullPath { get; set; }

        public String DlgTitle
        {
            get { return Text; }
            set { Text = value; }
        }

        public String FilePathTitle
        {
            get { return filePathLabel.Text; }
            set { filePathLabel.Text = value; }
        }

        public String FilePathText
        {
            get { return filePathTextBox.Text; }
            set { filePathTextBox.Text = value; }
        }

        public String FileNameTitle
        {
            get { return fileNameLabel.Text; }
            set { fileNameLabel.Text = value; }
        }

        public String FileNameText
        {
            get { return fileNameTextBox.Text; }
            set { fileNameTextBox.Text = value; }
        }

        public ExportFileDlg()
        {
            InitializeComponent();

            FilePathText = Directory.GetCurrentDirectory();
        }

        private void BtnBrowseFilePathClick(object sender, EventArgs e)
        {
            var pathFolderBrowserDialog = new FolderBrowserDialog();
            if (pathFolderBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                filePathTextBox.Text = pathFolderBrowserDialog.SelectedPath;
            }
        }

        private void BtnSaveClick(object sender, EventArgs e)
        {
            if (!ExportFile())
            {
                DialogResult = DialogResult.Cancel;
            }
            DialogResult = DialogResult.OK;
        }

        protected virtual bool ExportFile()
        {
            // If there are any invalid characters in the file name, cancel and display warning
            var fileName = fileNameTextBox.Text + FileExtension;
            if (!IsValidFileName(fileName))
            {
                MessageBox.Show(Resources.ExportFileDlg_ExportFile_Invalid_characters_found_in_the_file_name_provided_,
                                Resources.ExportFileDlg_ExportFile_Export_Failed, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                return false;
            }

            var pathString = filePathTextBox.Text;

            // Check to see if the file already exists, and if yes, ask the
            // user if they want to overwrite it.
            FileFullPath = Path.Combine(pathString, fileName);
            if (File.Exists(FileFullPath))
            {
                if (DialogResult.Yes !=
                MessageBox.Show('"' + fileName + '"' +
                                Resources.ExportFileDlg_ExportFile__alreay_exists__Would_you_like_to_overwrite_it_,
                                Resources.ExportFileDlg_ExportFile_Export_File,
                                MessageBoxButtons.YesNo,
                                MessageBoxIcon.Warning))
                {
                    return false;
                }

                File.Delete(FileFullPath);
            }

            return true;
        }

        private bool IsValidFileName(String fileName)
        {
            char[] invalidFileNameChars = Path.GetInvalidFileNameChars();
            return fileName.IndexOfAny(invalidFileNameChars) == -1;
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void FileNameTextBoxTextChanged(object sender, EventArgs e)
        {
            ExportTextChange();
        }

        private void FilePathTextBoxTextChanged(object sender, EventArgs e)
        {
            ExportTextChange();
        }

        private void ExportTextChange()
        {
            string fileName = fileNameTextBox.Text;
            string filePath = filePathTextBox.Text;

            btnSave.Enabled = (fileName != string.Empty) && Directory.Exists(filePath);
        }
    }
}
