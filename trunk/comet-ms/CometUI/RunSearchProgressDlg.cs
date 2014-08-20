using System;
using System.ComponentModel;
using CometUI.CustomControls;

namespace CometUI
{
    public partial class RunSearchProgressDlg : ProgressDlg
    {
        private CometSearch CometSearch { get; set; }

        public RunSearchProgressDlg(CometSearch cometSearch, BackgroundWorker backgroundWorker)
            : base(backgroundWorker)
        {
            InitializeComponent();

            CometSearch = cometSearch;
        }

        protected override void UpdateStatusText()
        {
            String newStatusText = "Running search...";
            String statusMsg = String.Empty;
            if (CometSearch.GetStatusMessage(ref statusMsg) && !String.IsNullOrEmpty(statusMsg))
            {
                newStatusText = statusMsg;
            }

            StatusMessage = newStatusText;
            base.UpdateStatusText();
        }

        public override void Cancel()
        {
            CometSearch.CancelSearch();
            base.Cancel();
        }
    }
}
