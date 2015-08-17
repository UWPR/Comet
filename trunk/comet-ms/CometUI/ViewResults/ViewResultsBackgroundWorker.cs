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
using System.ComponentModel;
using System.Threading;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.ViewResults
{
    class ViewResultsBackgroundWorker
    {
        private readonly BackgroundWorker _viewResultsBackgroundWorker = new BackgroundWorker();
        private readonly AutoResetEvent _viewResultsResetEvent = new AutoResetEvent(false);
        readonly ViewResultsProgressDlg _progressDialog;
        private ViewSearchResultsControl ViewResultsControl { get; set; }

        public ViewResultsBackgroundWorker(ViewSearchResultsControl viewResultsControl)
        {
            _viewResultsBackgroundWorker.WorkerSupportsCancellation = true;
            _viewResultsBackgroundWorker.WorkerReportsProgress = true;
            _viewResultsBackgroundWorker.DoWork += ViewResultsBackgroundWorkerDoWork;
            _viewResultsBackgroundWorker.ProgressChanged += ViewResultsBackgroundWorkerProgressChanged;
            _viewResultsBackgroundWorker.RunWorkerCompleted += ViewResultsBackgroundWorkerRunWorkerCompleted;

            _progressDialog = new ViewResultsProgressDlg(_viewResultsBackgroundWorker);

            ViewResultsControl = viewResultsControl;
        }

        public void DoWork()
        {
            _viewResultsResetEvent.Reset();
            if (!_viewResultsBackgroundWorker.IsBusy)
            {
                _viewResultsBackgroundWorker.RunWorkerAsync(ViewResultsControl);
                _progressDialog.TitleText = Resources.ViewResultsBackgroundWorker_DoWork_View_Results;
                _progressDialog.UpdateStatusText("Loading results...");
                _progressDialog.Show();
            }
        }

        public void CancelAsync()
        {
            if (_viewResultsBackgroundWorker.IsBusy)
            {
                _viewResultsBackgroundWorker.CancelAsync();
                _viewResultsResetEvent.WaitOne(5000);
            }
        }

        public bool IsBusy()
        {
            return _viewResultsBackgroundWorker.IsBusy;
        }

        private void ViewResultsBackgroundWorkerDoWork(object sender, DoWorkEventArgs e)
        {
            var viewResultsControl = e.Argument as ViewSearchResultsControl;
            if (viewResultsControl != null)
            {
                if (viewResultsControl.BeginUpdatingResults())
                {
                    e.Result = viewResultsControl;
                }

                _viewResultsBackgroundWorker.ReportProgress(1);

                if (_viewResultsBackgroundWorker.CancellationPending)
                {
                    e.Cancel = true;
                }
            }
        }

        private void ViewResultsBackgroundWorkerProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            _progressDialog.UpdateStatusText("Loading results...");
        }

        private void ViewResultsBackgroundWorkerRunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            _progressDialog.Hide();

            String msg;
            MessageBoxIcon msgIcon;
            try
            {
                var viewResultsControl = e.Result as ViewSearchResultsControl;
                if (viewResultsControl != null)
                {
                    viewResultsControl.FinishUpdatingResults();
                    msg = Resources.ViewResultsBackgroundWorker_ViewResultsBackgroundWorkerRunWorkerCompleted_Done_loading_results_;
                    msgIcon = MessageBoxIcon.Information;
                }
                else
                {
                    ViewResultsControl.ClearResults();
                    msg = Resources.ViewResultsBackgroundWorker_ViewResultsBackgroundWorkerRunWorkerCompleted_Failed_to_load_results_;
                    msgIcon = MessageBoxIcon.Error;
                }
            }
            catch (Exception exception)
            {
                ViewResultsControl.ClearResults();
                msg = Resources.ViewResultsBackgroundWorker_ViewResultsBackgroundWorkerRunWorkerCompleted_Failed_to_load_results__ + exception.Message;
                msgIcon = MessageBoxIcon.Error;
            }

            MessageBox.Show(msg, Resources.ViewResultsBackgroundWorker_DoWork_View_Results, MessageBoxButtons.OK, msgIcon);

            _viewResultsResetEvent.Set();
        }
    }
}
