using System;
using System.Drawing;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.SettingsUI;

namespace CometUI
{
    public partial class CometUI : Form
    {
        public static SearchSettings SearchSettings { get; set; }
        public static RunSearchSettings RunSearchSettings { get; set; }

        private bool OptionsPanelShown { get; set; }

        private SearchSettingsDlg _searchSettingsDlg;
        private SearchSettingsDlg SearchSettingsDlg
        {
            get { return _searchSettingsDlg ?? (_searchSettingsDlg = new SearchSettingsDlg()); }
        }

        public CometUI()
        {
            InitializeComponent();

            SearchSettings = SearchSettings.Default;

            RunSearchSettings = RunSearchSettings.Default;

            HideViewOptionsPanel();
        }

        private void ShowViewOptionsPanel()
        {
            showHideOptionsLabel.Text = "Hide options";
            showOptionsPanel.Visible = true;
            hideOptionsGroupBox.Visible = false;
            OptionsPanelShown = true;
            resultsListPanel.Location = resultsListPanelNormal.Location;
            resultsListPanel.Size = resultsListPanelNormal.Size;
        }

        private void HideViewOptionsPanel()
        {
            showHideOptionsLabel.Text = "Show options";
            showOptionsPanel.Visible = false;
            hideOptionsGroupBox.Visible = true;
            OptionsPanelShown = false;
            resultsListPanel.Location = resultsListPanelFull.Location;
            resultsListPanel.Size = resultsListPanelFull.Size;
        }

        private void SearchSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            if (DialogResult.OK == SearchSettingsDlg.ShowDialog())
            {
                // Todo: do we need to do something here?
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
            if (SearchSettingsDlg.SettingsChanged)
            {
                if (MessageBox.Show(Resources.CometUI_CometUIFormClosing_You_have_modified_the_search_settings__Would_you_like_to_save_the_changes_before_you_exit_, 
                    Resources.CometUI_CometUIFormClosing_Search_Settings_Changed, 
                    MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    SearchSettingsDlg.SaveSearchSettings();
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
                SearchSettingsDlg.SaveSearchSettings();
            }
        }

        private void SearchSettingsImportToolStripMenuItemClick(object sender, EventArgs e)
        {
            var importParamsDlg = new ImportParamsDlg();
            importParamsDlg.ShowDialog();
        }

        private void SearchSettingsExportToolStripMenuItemClick(object sender, EventArgs e)
        {
            var exportParamsDlg = new ExportParamsDlg();
            if (DialogResult.OK == exportParamsDlg.ShowDialog())
            {
                MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_Settings_exported_to_ + exportParamsDlg.FilePath,
                Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings, MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            }
        }

        private void HelpAboutToolStripMenuItemClick(object sender, EventArgs e)
        {
            var aboutDlg = new AboutDlg();
            aboutDlg.ShowDialog();
        }

        private void ShowHideOptionsBtnClick(object sender, EventArgs e)
        {
            if (OptionsPanelShown)
            {
                HideViewOptionsPanel();
            }
            else
            {
                ShowViewOptionsPanel();
            }
        }
    }
}
