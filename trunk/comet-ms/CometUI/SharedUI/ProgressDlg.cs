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

namespace CometUI.SharedUI
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

        public String TitleText
        {
            get { return Text; }
            set { Text = value; }
        }

        public void UpdateStatusText(String statusText)
        {
            StatusText.Text = statusText;
        }

        public void AllowCancel(bool allow)
        {
            CancelButton.Enabled = allow;
        }

        public virtual void Cancel()
        {
            _backgroundWorker.CancelAsync();
        }

        private void CancelButtonClick(object sender, EventArgs e)
        {
            if (VerifyCancel())
            {
                Cancel();
                Close();
            }
        }

        protected virtual bool VerifyCancel()
        {
            if (DialogResult.OK == MessageBox.Show(Resources.ProgressDlg_VerifyCancel_Are_you_sure_you_want_to_cancel_the_operation_, Text, MessageBoxButtons.OKCancel, MessageBoxIcon.Information))
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
