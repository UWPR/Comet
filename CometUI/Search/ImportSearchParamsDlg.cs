﻿/*
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
using CometUI.Properties;
using CometUI.Search.SearchSettings;

namespace CometUI.Search
{
    public partial class ImportSearchParamsDlg : Form
    {
        private new CometUIMainForm Parent { get; set; }

        public ImportSearchParamsDlg(CometUIMainForm parent)
        {
            InitializeComponent();

            Parent = parent;
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnBrowseParamsClick(object sender, EventArgs e)
        {
            var paramsOpenFileDialog = new OpenFileDialog
            {
                Title = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Open_Params_File,
                InitialDirectory = @".",
                Filter = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Comet_Params_Files____params____params,
                Multiselect = false,
                RestoreDirectory = true
            };

            if (paramsOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                paramsFileCombo.Text = paramsOpenFileDialog.FileName;
            }
            
            btnImport.Enabled = false;
            string path = paramsFileCombo.Text;
            if (File.Exists(path))
            {
                btnImport.Enabled = true;
            }
        }

        private void BtnImportClick(object sender, EventArgs e)
        {
            var cometParamsReader = new CometParamsReader(@paramsFileCombo.Text);
            var paramsMap = new CometParamsMap();
            bool succeeded = cometParamsReader.ReadParamsFile(paramsMap);
            cometParamsReader.Close();
            if (succeeded)
            {
                succeeded = paramsMap.GetSettingsFromCometParams(CometUIMainForm.SearchSettings);
            }

            if (succeeded)
            {
                // Todo: Add functionality to check if something actually changed
                Parent.SearchSettingsChanged = true;
                
                MessageBox.Show(Resources.ImportParamsDlg_BtnImportClick_Import_completed_successfully_, Resources.ImportParamsDlg_BtnImportClick_Import_Search_Settings, MessageBoxButtons.OK, MessageBoxIcon.Information);
                DialogResult = DialogResult.OK;
            }
            else
            {
                MessageBox.Show(Resources.ImportParamsDlg_BtnImportClick_Import_failed_ + cometParamsReader.ErrorMessage, Resources.ImportParamsDlg_BtnImportClick_Import_Search_Settings, MessageBoxButtons.OK, MessageBoxIcon.Error);
                DialogResult = DialogResult.Cancel;
            }
        }

        private void ParamsTextChanged()
        {
            string path = paramsFileCombo.Text;
            btnImport.Enabled = File.Exists(path);
        }

        private void ParamsFileComboSelectedIndexChanged(object sender, EventArgs e)
        {
            ParamsTextChanged();
        }

        private void ParamsFileComboTextChanged(object sender, EventArgs e)
        {
            ParamsTextChanged();
        }

        private void BtnSettingsClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
                Parent.SearchSettingsChanged = searchSettingsDlg.SettingsChanged;
            }
        }
    }
}