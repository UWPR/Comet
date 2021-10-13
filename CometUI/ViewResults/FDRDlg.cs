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
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class FDRDlg : Form
    {
        // Normalized value with a min of 0 and max of 1
        public double FDRCutoff { get; private set; }

        public bool ShowDecoyHits { get; private set; }

        public FDRDlg()
        {
            InitializeComponent();

            FDRCutoff = 1;

            fdrCutoffTextBox.Text = String.Empty;

            ShowDecoyHits = false;
            showDecoyHitsCheckBox.Checked = false;
        }

        private void CancelFDRCutoffBtnClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void ApplyFDRCutoffBtnClick(object sender, EventArgs e)
        {
            if (fdrCutoffTextBox.Text.Equals(String.Empty))
            {
                DialogResult = DialogResult.Ignore;
            }
            else
            {
                // The cutoff is entered as a percentage, so normalize it
                FDRCutoff = Convert.ToDouble(fdrCutoffTextBox.Text) / 100;

                ShowDecoyHits = showDecoyHitsCheckBox.Checked;

                DialogResult = DialogResult.OK;
            }
        }
    }
}
