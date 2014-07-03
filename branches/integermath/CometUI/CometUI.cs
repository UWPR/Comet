using System;
using System.Collections.Generic;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.SettingsUI;

namespace CometUI
{
    public partial class CometUI : Form
    {
        public Dictionary<String, CometParam> CometParams = new Dictionary<string, CometParam>();

        private readonly List<RunSearchBackgroundWorker> _runSearchWorkers = 
            new List<RunSearchBackgroundWorker>();

        public CometUI()
        {
            InitializeComponent();
        }

        private void SearchSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
            }
        }

        private void RunSearchToolStripMenuItemClick(object sender, EventArgs e)
        {
            var runSearchDlg = new RunSearchDlg(this);
            if (DialogResult.OK == runSearchDlg.ShowDialog())
            {
                var runSearchWorker = new RunSearchBackgroundWorker();
                runSearchWorker.DoWork(runSearchDlg);
                _runSearchWorkers.Add(runSearchWorker);
            }
        }

        private void CometUIFormClosing(object sender, FormClosingEventArgs e)
        {
            WorkerThreadsCleanupTimer.Stop();
            WorkerThreadsCleanupTimer.Enabled = false;

            foreach (var worker in _runSearchWorkers)
            {
                worker.CancelAsync();
            }

            _runSearchWorkers.Clear();
        }

        private void WorkerThreadsCleanupTimerTick(object sender, EventArgs e)
        {
            foreach (var worker in _runSearchWorkers)
            {
                if (!worker.IsBusy())
                {
                    _runSearchWorkers.Remove(worker);
                }
            }
        }

        private void CometUILoad(object sender, EventArgs e)
        {
            WorkerThreadsCleanupTimer.Enabled = true;
            WorkerThreadsCleanupTimer.Start();
        }

        private void ExitToolStripMenuItemClick(object sender, EventArgs e)
        {
            Close();
        }

        private void SaveSearchSettingsToolStripMenuItemClick(object sender, EventArgs e)
        {
            if (MessageBox.Show(Resources.CometUI_SaveSearchSettingsToolStripMenuItemClick_Are_you_sure_you_want_to_overwrite_the_current_settings_, Resources.CometUI_SaveSearchSettingsToolStripMenuItemClick_Save_Search_Settings, MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
            {
                Settings.Default.Save();
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
    }
}
