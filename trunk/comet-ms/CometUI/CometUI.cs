using System;
using System.Collections.Generic;
using System.Windows.Forms;
using CometUI.SettingsUI;

namespace CometUI
{
    public partial class CometUI : Form
    {
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
                // Do something here?  Maybe save the settings?
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
    }
}
