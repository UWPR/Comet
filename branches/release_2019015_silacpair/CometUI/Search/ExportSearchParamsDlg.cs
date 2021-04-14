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
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.Search.SearchSettings;
using CometUI.SharedUI;

namespace CometUI.Search
{
    class ExportSearchParamsDlg : ExportFileDlg
    {
        private new CometUIMainForm Parent { get; set; }

        private Button _btnSettings;

        public ExportSearchParamsDlg(CometUIMainForm parent)
        {
            InitializeComponent();

            Parent = parent;
            
            DlgTitle = "Export Search Settings";
            FileNameText = Resources.ExportParamsDlg_ExportParamsDlg_comet;
            FileExtension = ".params";
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExportSearchParamsDlg));
            this._btnSettings = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // _btnSettings
            // 
            this._btnSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._btnSettings.Location = new System.Drawing.Point(205, 137);
            this._btnSettings.Name = "_btnSettings";
            this._btnSettings.Size = new System.Drawing.Size(75, 23);
            this._btnSettings.TabIndex = 108;
            this._btnSettings.Text = global::CometUI.Properties.Resources.ExportSearchParamsDlg_InitializeComponent__Settings___;
            this._btnSettings.UseVisualStyleBackColor = true;
            // 
            // ExportSearchParamsDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.ClientSize = new System.Drawing.Size(454, 172);
            this.Controls.Add(this._btnSettings);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ExportSearchParamsDlg";
            this.Controls.SetChildIndex(this._btnSettings, 0);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void BtnSettingsClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
                Parent.SearchSettingsChanged = searchSettingsDlg.SettingsChanged;
            }
        }

        protected override bool ExportFile()
        {
            if (!base.ExportFile())
            {
                return false;
            }

            var paramsMap = new CometParamsMap(CometUIMainForm.SearchSettings);

            if (!CheckProteomeDatabaseFile(paramsMap))
            {
                // Show message indicating absence of "database_name" in params file
                if (DialogResult.OK != MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_You_have_not_specified_a_proteome_database_name_in_the_Input_Settings_The_params_file_you_are_about_to_create_will_leave_the_database_name_field_blank,
                    Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                    MessageBoxButtons.OKCancel, MessageBoxIcon.Warning))
                {
                    return false;
                }
            }

            var cometParamsWriter = new CometParamsWriter(FileFullPath);
            if (!cometParamsWriter.WriteParamsFile(paramsMap))
            {
                MessageBox.Show(Resources.ExportParamsDlg_ExportCometParams_Failed_to_generate_the_params_file_ + cometParamsWriter.ErrorMessage,
                                Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                                MessageBoxButtons.OK, MessageBoxIcon.Error);
                cometParamsWriter.Close();
                return false;
            }

            cometParamsWriter.Close();
            return true;
        }

        private bool CheckProteomeDatabaseFile(CometParamsMap paramsMap)
        {
            var dbNameParam = paramsMap.CometParams["database_name"];
            return !String.IsNullOrEmpty(dbNameParam.Value);
        }
    }
}
