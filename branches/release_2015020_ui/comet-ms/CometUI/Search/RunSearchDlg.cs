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
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.Search.SearchSettings;

namespace CometUI.Search
{
    public partial class RunSearchDlg : Form
    {
        private new CometUIMainForm Parent { get; set; }
        private string[] _inputFiles = new string[0];

        public RunSearchDlg(CometUIMainForm parent)
        {
            InitializeComponent();

            InitializeFromDefaultSettings();

            UpdateButtons();

            Parent = parent;
        }

        public string[] InputFiles
        {
            get { return _inputFiles; }

            set
            {
                // Set new value
                _inputFiles = value;

                inputFilesList.BeginUpdate();

                // Populate the input files list
                inputFilesList.Items.Clear();
                foreach (String fileName in _inputFiles)
                {
                    inputFilesList.Items.Add(fileName);
                }

                inputFilesList.EndUpdate();

                btnRunSearch.Enabled = inputFilesList.Items.Count > 0 && File.Exists(proteomeDbFileCombo.Text);
                btnRemInputFile.Enabled = inputFilesList.SelectedItems.Count > 0;
            }
        }

        public static string[] AddInputFiles(Form parent, IEnumerable<string> inputFileNames,
                                             IEnumerable<string> fileNames)
        {
            var filesNew = new List<string>(inputFileNames);
            var filesError = new List<string>();
            foreach (var fileName in fileNames)
            {
                if (IsValidInputFile(fileName))
                {
                    if (inputFileNames != null && !filesNew.Contains(fileName))
                    {
                        filesNew.Add(fileName);
                    }
                }
                else
                {
                    filesError.Add(fileName);
                }
            }

            if (filesError.Count > 0)
            {
                string errorMessage;
                if (filesError.Count == 1)
                {
                    errorMessage = String.Format("The file {0} is not a valid search input file.", filesError[0]);
                }
                else
                {
                    errorMessage = String.Format("The files {0}", filesError[0]);
                    for (int i = 1; i < filesError.Count - 1; i++)
                    {
                        errorMessage += String.Format(", {0}", filesError[i]);
                    }

                    errorMessage += String.Format(" and {0} are not valid search input files.",
                                                  filesError[filesError.Count - 1]);
                }

                MessageBox.Show(errorMessage, Resources.CometUI_Title_Error, MessageBoxButtons.OKCancel);
            }

            return filesNew.ToArray();
        }

        private void InitializeFromDefaultSettings()
        {
            if (null != CometUIMainForm.RunSearchSettings.InputFiles)
            {
                InputFiles = CometUIMainForm.RunSearchSettings.InputFiles.Cast<string>().ToArray();
            }

            proteomeDbFileCombo.Text = CometUIMainForm.SearchSettings.ProteomeDatabaseFile;
        }

        private void BtnSettingsClick(object sender, EventArgs e)
        {
            OpenSettingsDialog();
        }

        private void OpenSettingsDialog()
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
                Parent.SearchSettingsChanged = searchSettingsDlg.SettingsChanged;
            }
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnAddInputFileClick(object sender, EventArgs e)
        {
            IEnumerable<string> addFiles = ShowAddInputFile();
            if (addFiles != null)
            {
                string[] addValidFiles = AddInputFiles(Parent, InputFiles, addFiles);
                AddInputFiles(addValidFiles);
            }
        }

        private static IEnumerable<string> ShowAddInputFile()
        {
            const string filter =
                "Search Input Files (*.mgf, *.mzXML, *.mzML, *.ms2, *.cms2, *.raw)|*.mgf;*.mzXML;*.mzML;*.ms2;*.cms2;*.raw|All Files (*.*)|*.*";
            var fdlg = new OpenFileDialog
                           {
                               Title = Resources.InputFilesControl_ShowAddInputFile_Open_Input_File,
                               InitialDirectory = @".",
                               Filter = filter,
                               FilterIndex = 1,
                               Multiselect = true,
                               RestoreDirectory = true
                           };

            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                return fdlg.FileNames;
            }

            return null;
        }

        private void AddInputFiles(IEnumerable<string> inputFiles)
        {
            InputFiles = AddInputFiles(Parent, InputFiles, inputFiles);
        }

        private static bool IsValidInputFile(string fileName)
        {
            var extension = Path.GetExtension(fileName);
            if (extension != null)
            {
                string fileExt = extension.ToLower();
                return File.Exists(fileName) &&
                       (fileExt == ".mgf" || fileExt == ".mzxml" || fileExt == ".mzml" || fileExt == ".ms2" || fileExt == ".cms2" || fileExt == ".raw");
            }
            return false;
        }

        private void BtnRemInputFileClick(object sender, EventArgs e)
        {
            var selectedIndices = inputFilesList.SelectedIndices;
            var inputFileNames = InputFiles.ToList();
            for (int i = selectedIndices.Count - 1; i >= 0; i--)
            {
                inputFileNames.RemoveAt(i);
            }

            InputFiles = inputFileNames.ToArray();
        }

        private void UpdateButtons()
        {
            btnRemInputFile.Enabled = inputFilesList.SelectedItems.Count > 0;
            btnRunSearch.Enabled = inputFilesList.Items.Count > 0 && File.Exists(proteomeDbFileCombo.Text);
        }

        private void BtnRunSearchClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }

        private string DatabaseFile
        {
            get { return proteomeDbFileCombo.Text; }

            set { proteomeDbFileCombo.Text = value; }
        }

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            string databaseFile = InputSettingsControl.ShowOpenDatabaseFile();
            if (null != databaseFile)
            {
                if (!String.Equals(DatabaseFile, databaseFile))
                {
                    DatabaseFile = databaseFile;
                    ProteomeDbFileNameChanged();
                }
            }

            UpdateButtons();
        }

        private void ProteomeDbFileComboTextUpdate(object sender, EventArgs e)
        {
            ProteomeDbFileNameChanged();
        } 

        private void ProteomeDbFileNameChanged()
        {
            if (File.Exists(proteomeDbFileCombo.Text) && !String.Equals(proteomeDbFileCombo.Text, CometUIMainForm.SearchSettings.ProteomeDatabaseFile))
            {
                if (DialogResult.OK == MessageBox.Show(Resources.RunSearchDlg_ProteomeDbFileNameChanged_Would_you_like_to_update_the_proteome_database_file_name_in_the_search_settings_with_the_one_you_have_specified_here_, Resources.RunSearchDlg_ProteomeDbFileNameChanged_Run_Search, MessageBoxButtons.OKCancel, MessageBoxIcon.Question))
                {
                    CometUIMainForm.SearchSettings.ProteomeDatabaseFile = proteomeDbFileCombo.Text;
                }
            }

            if (-1 == proteomeDbFileCombo.FindStringExact(proteomeDbFileCombo.Text))
            {
                proteomeDbFileCombo.Items.Add(proteomeDbFileCombo.Text);
            }
        }

        private void ProteomeDbFileComboSelectedIndexChanged(object sender, EventArgs e)
        {
            ProteomeDbFileNameChanged();
        }

        private void RunSearchDlgLoad(object sender, EventArgs e)
        {
            InitializeFromDefaultSettings();
            UpdateButtons();
        }

        private void RunSearchDlgFormClosing(object sender, FormClosingEventArgs e)
        {
            SaveInputFilesToSettings();
        }

        private void SaveInputFilesToSettings()
        {
            var inputFiles = new StringCollection();
            for (var i = 0; i < inputFilesList.Items.Count; i++)
            {
                inputFiles.Add(InputFiles[i]);
            }

            CometUIMainForm.RunSearchSettings.InputFiles = inputFiles;
            CometUIMainForm.RunSearchSettings.Save();
        }

        private void InputFilesListSelectedIndexChanged(object sender, EventArgs e)
        {
            UpdateButtons();
        }

    }
}
