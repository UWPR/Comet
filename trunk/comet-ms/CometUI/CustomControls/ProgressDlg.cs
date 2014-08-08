using System;
using System.ComponentModel;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.CustomControls
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
                Resources.ProgressDlg_ProgressDlgClosing_Cancellation_request_has_been_sent__but_the_background_proces_may_continue_running_until_the_request_has_been_processed_,
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
            if (DialogResult.OK == MessageBox.Show(Resources.ProgressDlg_VerifyCancel_Cancellation_request_will_be_sent__but_the_background_proces_may_continue_running_until_the_request_has_been_processed__Click_Cancel_to_continue_receiving_progress_status__Otherwise__please_click_OK_, Text, MessageBoxButtons.OKCancel, MessageBoxIcon.Information))
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
