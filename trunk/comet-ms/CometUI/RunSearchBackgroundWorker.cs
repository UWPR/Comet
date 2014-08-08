using System;
using System.ComponentModel;
using System.Threading;
using System.Windows.Forms;
using CometUI.CustomControls;
using CometUI.Properties;

namespace CometUI
{
    class RunSearchBackgroundWorker
    {
        private readonly BackgroundWorker _runSearchBackgroundWorker = new BackgroundWorker();
        private readonly AutoResetEvent _runSearchResetEvent = new AutoResetEvent(false);
        readonly ProgressDlg _progressDialog;
        RunSearchDlg _runSearchDlg;

        public RunSearchBackgroundWorker()
        {
            _runSearchBackgroundWorker.WorkerSupportsCancellation = true;
            _runSearchBackgroundWorker.WorkerReportsProgress = true;
            _runSearchBackgroundWorker.DoWork += RunSearchBackgroundWorkerDoWork;
            _runSearchBackgroundWorker.ProgressChanged += RunSearchBackgroundWorkerProgressChanged;
            _runSearchBackgroundWorker.RunWorkerCompleted += RunSearchBackgroundWorkerRunWorkerCompleted;

            _runSearchDlg = null;

            _progressDialog = new ProgressDlg(_runSearchBackgroundWorker);
        }

        public void DoWork(RunSearchDlg runSearchDlg)
        {
            _runSearchDlg = runSearchDlg;
            _runSearchResetEvent.Reset();
            if (!_runSearchBackgroundWorker.IsBusy)
            {
                _runSearchBackgroundWorker.RunWorkerAsync(runSearchDlg);
                _progressDialog.UpdateTitleText("Search Progress");
                _progressDialog.UpdateStatusText("Running search...");
                _progressDialog.Show();
            }
        }

        public void CancelAsync()
        {
            if (_runSearchBackgroundWorker.IsBusy)
            {
                _runSearchBackgroundWorker.CancelAsync();
                _runSearchResetEvent.WaitOne(5000);
            }
        }

        public bool IsBusy()
        {
            return _runSearchBackgroundWorker.IsBusy;
        }


        private void RunSearchBackgroundWorkerDoWork(object sender, DoWorkEventArgs e)
        {
            var runSearchDlg = e.Argument as RunSearchDlg;
            if (_runSearchDlg != null)
            {
                _runSearchDlg.RunSearch();

                _runSearchBackgroundWorker.ReportProgress(1);

                if (_runSearchBackgroundWorker.CancellationPending)
                {
                    // What to do here?
                }

                e.Result = runSearchDlg;
            }
        }

        private void RunSearchBackgroundWorkerProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            // This will become useful if we can get search to send us status
            // updates
            const string statusText = "Running search...";
            _progressDialog.UpdateStatusText(statusText);
        }

        private void RunSearchBackgroundWorkerRunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            _progressDialog.Hide();

            String msg = String.Empty;
            MessageBoxIcon msgIcon = MessageBoxIcon.None;
            try
            {
                var runSearchDlg = e.Result as RunSearchDlg;
                if (runSearchDlg != null)
                {
                    if (!e.Cancelled && runSearchDlg.SearchSucceeded)
                    {
                        msgIcon = MessageBoxIcon.Information;
                    }
                    else
                    {
                        msgIcon = MessageBoxIcon.Error;
                    }

                    msg += runSearchDlg.SearchStatusMessage;

                }
            }
            catch (Exception exception)
            {
                msg = "Search failed. " + exception.Message;
                msgIcon = MessageBoxIcon.Error;
            }

            if (!e.Cancelled)
            {
                MessageBox.Show(msg, Resources.RunSearchBackgroundWorker_RunSearchBackgroundWorkerRunWorkerCompleted_Run_Search_Completed, MessageBoxButtons.OK, msgIcon);
            }

            _runSearchDlg = null;
            _runSearchResetEvent.Set();
        }
    }
}
