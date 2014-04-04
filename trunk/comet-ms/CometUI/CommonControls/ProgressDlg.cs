using System;
using System.ComponentModel;
using System.Windows.Forms;

namespace CometUI.CommonControls
{
    public partial class ProgressDlg : Form
    {
        readonly BackgroundWorker _backgroundWorker;
        public ProgressDlg(BackgroundWorker backgroundWorker)
        {
            InitializeComponent();
            _backgroundWorker = backgroundWorker;
            StatusText.Text = String.Empty;
            ProgressBar.Value = 1;
            ProgressBar.Visible = true;
        }

        public void AllowCancel(bool allow)
        {
            CancelButton.Enabled = allow;
        }

        public void UpdateStatusText(String statusText)
        {
            StatusText.Text = statusText;
        }

        public void UpdateTitleText(String titleText)
        {
            Text = titleText;
        }

        private void ProgressDlgClosing(object sender, FormClosingEventArgs e)
        {
            _backgroundWorker.CancelAsync();
            MessageBox.Show(
                "Cancellation request has been sent, but the background proces may continue running until the request has been processed.",
                Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void CancelButtonClick(object sender, EventArgs e)
        {
            if (VerifyCancel())
            {
                Close();
            }
        }

        private bool VerifyCancel()
        {
            if (DialogResult.OK == MessageBox.Show("Cancellation request will be sent, but the background proces may continue running until the request has been processed. Click Cancel to continue receiving progress status. Otherwise, please click OK.", Text, MessageBoxButtons.OKCancel, MessageBoxIcon.Information))
            {
                return true;
            }

            return false;
        }

        private void StatusTextMouseHover(object sender, EventArgs e)
        {
            ProgressDlgToolTip.Show(StatusText.Text, StatusText);
        }
    }
}
