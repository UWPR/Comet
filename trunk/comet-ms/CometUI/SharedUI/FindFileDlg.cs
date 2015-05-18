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
using System.IO;
using System.Windows.Forms;

namespace CometUI.SharedUI
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