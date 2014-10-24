using System;
using System.ComponentModel;
using System.Windows.Forms;
using CometUI.CustomControls;
using CometUI.Properties;

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
