using System;
using System.ComponentModel;
using System.Threading;
using System.Windows.Forms;

namespace CometUI
{
    class RunSearchBackgroundWorker
    {
        private readonly BackgroundWorker _runSearchBackgroundWorker = new BackgroundWorker();
        private readonly AutoResetEvent _runSearchResetEvent = new AutoResetEvent(false);
        readonly RunSearchProgressDlg _progressDialog;
        private CometSearch CometSearch { get; set; }

        public RunSearchBackgroundWorker(CometSearch cometSearch)
        {
            _runSearchBackgroundWorker.WorkerSupportsCancellation = true;
            _runSearchBackgroundWorker.WorkerReportsProgress = true;
            _runSearchBackgroundWorker.DoWork += RunSearchBackgroundWorkerDoWork;
            _runSearchBackgroundWorker.ProgressChanged += RunSearchBackgroundWorkerProgressChanged;
            _runSearchBackgroundWorker.RunWorkerCompleted += RunSearchBackgroundWorkerRunWorkerCompleted;

            CometSearch = cometSearch;
            _progressDialog = new RunSearchProgressDlg(CometSearch, _runSearchBackgroundWorker);
        }

        public void DoWork()
        {
            _runSearchResetEvent.Reset();
            if (!_runSearchBackgroundWorker.IsBusy)
            {
                _runSearchBackgroundWorker.RunWorkerAsync(CometSearch);
                _progressDialog.UpdateTitleText("Search Progress");
                _progressDialog.UpdateStatusText("Running search...");
                _progressDialog.Show();
            }
        }

        public void CancelAsync()
        {
            CometSearch.CancelSearch();
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
            var cometSearch = e.Argument as CometSearch;
            if (cometSearch != null)
            {
                if (!cometSearch.RunSearch())
                {
                    if (cometSearch.IsCancel())
                    {
                        cometSearch.ResetCancel();
                    }
                }

                _runSearchBackgroundWorker.ReportProgress(1);

                if (_runSearchBackgroundWorker.CancellationPending)
                {
                    e.Cancel = true;
                }

                e.Result = cometSearch;
            }
        }

        private void RunSearchBackgroundWorkerProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            String statusText = "Running search...";
            String statusMsg = String.Empty;
            if (CometSearch.GetStatusMessage(statusMsg))
            {
                statusText = statusMsg;
            }
            _progressDialog.UpdateStatusText(statusText);
        }

        private void RunSearchBackgroundWorkerRunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            _progressDialog.Hide();

            String msg = String.Empty;
            MessageBoxIcon msgIcon = MessageBoxIcon.None;
            try
            {
                var cometSearch = e.Result as CometSearch;
                if (cometSearch != null)
                {
                    if (!e.Cancelled && CometSearch.SearchSucceeded)
                    {
                        msgIcon = MessageBoxIcon.Information;
                    }
                    else if (e.Cancelled)
                    {
                        msgIcon = MessageBoxIcon.Warning;
                        cometSearch.ResetCancel();
                    }
                    else
                    {
                        msgIcon = MessageBoxIcon.Error;
                    }

                    msg += CometSearch.SearchStatusMessage;
                }
            }
            catch (Exception exception)
            {
                msg = "Search failed. " + exception.Message;
                msgIcon = MessageBoxIcon.Error;
            }

            MessageBox.Show(msg, "Run Search", MessageBoxButtons.OK, msgIcon);

            _runSearchResetEvent.Set();
        }
    }
}
