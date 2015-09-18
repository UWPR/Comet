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
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.SharedUI;

namespace CometUI.Search
{
    public partial class RunSearchProgressDlg : ProgressDlg
    {
        private CometSearch CometSearch { get; set; }

        public RunSearchProgressDlg(CometSearch cometSearch, BackgroundWorker backgroundWorker)
            : base(backgroundWorker)
        {
            InitializeComponent();

            progressStatusMessageTimer.Interval = 10;
            progressStatusMessageTimer.Tick += ProgressStatusMessageTimerTick;
            progressStatusMessageTimer.Start();

            FormClosing += RunSearchProgressDlgClosing;

            CometSearch = cometSearch;
        }

        protected override bool VerifyCancel()
        {
            if (DialogResult.Yes == MessageBox.Show(Resources.RunSearchProgressDlg_VerifyCancel_Are_you_sure_you_want_to_cancel_search_, Text, MessageBoxButtons.YesNo, MessageBoxIcon.Information))
            {
                return true;
            }

            return false;
        }

        public override void Cancel()
        {
            CometSearch.CancelSearch();
            base.Cancel();
        }

        private void ProgressStatusMessageTimerTick(object sender, EventArgs e)
        {
            UpdateStatusText();
        }

        private void UpdateStatusText()
        {
            String newStatusText = "Running search...";
            String statusMsg = String.Empty;
            if (CometSearch.GetStatusMessage(ref statusMsg) && !String.IsNullOrEmpty(statusMsg))
            {
                newStatusText = statusMsg;
            }

            UpdateStatusText(newStatusText);
        }

        private void RunSearchProgressDlgClosing(object sender, FormClosingEventArgs e)
        {
            progressStatusMessageTimer.Stop();
        }
    }
}
