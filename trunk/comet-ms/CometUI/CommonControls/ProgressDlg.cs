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

        public void UpdateStatusText(String statusText)
        {
            StatusText.Text = statusText;
        }

        private void ProgressDlgClosing(object sender, FormClosingEventArgs e)
        {
            _backgroundWorker.CancelAsync();
        }

        private void CancelButtonClick(object sender, EventArgs e)
        {
            Close();
        }

        private void StatusTextMouseHover(object sender, EventArgs e)
        {
            ProgressDlgToolTip.Show(StatusText.Text, StatusText);
        }
    }
}
