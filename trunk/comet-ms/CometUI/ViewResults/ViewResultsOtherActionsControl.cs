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
using CometUI.Properties;

namespace CometUI.ViewResults
{
    public partial class ViewResultsOtherActionsControl : UserControl
    {
        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public ViewResultsOtherActionsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            ViewSearchResultsControl = parent;
        }

        private void ExportResultsBtnClick(object sender, EventArgs e)
        {
            ViewSearchResultsControl.ExportResultsList();
        }

        private void FDRCutoffBtnClick(object sender, EventArgs e)
        {
            if (!ViewSearchResultsControl.HasSearchResults)
            {
                MessageBox.Show(Resources.ViewResultsOtherActionsControl_FDRCutoffBtnClick_No_results_to_apply_the_FDR_cutoff_to_,
                    Resources.ViewResultsOtherActionsControl_FDRCutoffBtnClick_Apply_FDR_Cutoff,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            var fdrDlg = new FDRDlg();
            if (DialogResult.OK == fdrDlg.ShowDialog())
            {
                // Todo: Apply FDR cutoff to view results list
            }
        }
    }
}
