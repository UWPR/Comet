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
using System.Drawing;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.Search;
using CometUI.Search.SearchSettings;
using CometUI.ViewResults;

namespace CometUI
{
    public partial class CometUI : Form
    {
        public static SearchSettings SearchSettings { get; set; }
        public static RunSearchSettings RunSearchSettings { get; set; }
        public static ViewResultsSettings ViewResultsSettings { get; set; }
        public bool SearchSettingsChanged { get; set; }

        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public CometUI()
        {
            InitializeComponent();

            SearchSettings = SearchSettings.Default;

            RunSearchSettings = RunSearchSettings.Default;

            ViewResultsSettings = ViewResultsSettings.Default;

            ViewSearchResultsControl = new ViewSearchResultsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(10, 5)
            };

            viewSearchResultsPanel.Controls.Add(ViewSearchResultsControl);
        }

        public static string ShowOpenPepXMLFile()
        {
            const string filter = "PepXML file (*.pep.xml)|*.pep.xml";
            var fdlg = new OpenFileDialog
            {
                Title = Resources.CometUI_ShowOpenPepXMLFile_Open_PepXML_File,
                InitialDirectory = @".",
                Filter = filter,
                FilterIndex = 1,
                Multiselect = false,
                RestoreDirectory = true
            };

            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                return fdlg.FileName;
            }

            return null;
        }

        private void SaveSearchSettings()
        {
            if (SearchSettingsChanged)
            {
                SearchSettings.Save();
                SearchSettingsChanged = false;
            }
        }

        private void SearchSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
                SearchSettingsChanged = searchSettingsDlg.SettingsChanged;
            }
        }

        private void RunSearchToolStripMenuItemClick(object sender, EventArgs e)
        {
            var runSearchDlg = new RunSearchDlg(this);
            if (DialogResult.OK == runSearchDlg.ShowDialog())
            {
                var cometSearch = new CometSearch(runSearchDlg.InputFiles);
                var runSearchWorker = new RunSearchBackgroundWorker(cometSearch);
                runSearchWorker.DoWork();
            }
        }

        private void CometUIFormClosing(object sender, FormClosingEventArgs e)
        {
            if (SearchSettingsChanged)
            {
                if (MessageBox.Show(Resources.CometUI_CometUIFormClosing_You_have_modified_the_search_settings__Would_you_like_to_save_the_changes_before_you_exit_, 
                    Resources.CometUI_CometUIFormClosing_Search_Settings_Changed, 
                    MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    SaveSearchSettings();
                }
            }

            if (ViewSearchResultsControl.SettingsChanged)
            {
                if (MessageBox.Show(
                    Resources.CometUI_CometUIFormClosing_You_have_modified_the_view_results_settings__Would_you_like_to_save_the_changes_before_you_exit_,
                    Resources.CometUI_CometUIFormClosing_View_Results_Settings_Changed,
                    MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    ViewSearchResultsControl.SaveViewResultsSettings();
                }
            }
        }

        private void ExitToolStripMenuItemClick(object sender, EventArgs e)
        {
            Close();
        }

        private void SaveSearchSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            if (MessageBox.Show(Resources.CometUI_SaveSearchSettingsToolStripMenuItemClick_Are_you_sure_you_want_to_overwrite_the_current_settings_,
                    Resources.CometUI_SaveSearchSettingsToolStripMenuItemClick_Save_Search_Settings,
                    MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
            {
                SaveSearchSettings();
            }
        }

        private void SearchSettingsImportToolStripMenuItemClick(object sender, EventArgs e)
        {
            var importParamsDlg = new ImportSearchParamsDlg(this);
            importParamsDlg.ShowDialog();
        }

        private void SearchSettingsExportToolStripMenuItemClick(object sender, EventArgs e)
        {
            var exportParamsDlg = new ExportSearchParamsDlg(this);
            if (DialogResult.OK == exportParamsDlg.ShowDialog())
            {
                MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_Settings_exported_to_ + exportParamsDlg.FileFullPath,
                Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings, MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            }
        }

        private void HelpAboutToolStripMenuItemClick(object sender, EventArgs e)
        {
            var aboutDlg = new AboutDlg();
            aboutDlg.ShowDialog();
        }

        private void OpenToolStripMenuItemClick(object sender, EventArgs e)
        {
            ViewSearchResultsControl.UpdateViewSearchResults(ShowOpenPepXMLFile());
        }

        private void ViewResultsSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            if (MessageBox.Show(Resources.CometUI_ViewResultsSettingsToolStripMenuItemClick_Are_you_sure_you_want_to_overwrite_the_current_settings_, 
                Resources.CometUI_ViewResultsSettingsToolStripMenuItemClick_Save_View_Results_Settings,
                MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
            {
                ViewSearchResultsControl.SaveViewResultsSettings();
            }
        }
    }
}
