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

using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsDisplayOptionsControl : UserControl
    {
        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public ViewResultsDisplayOptionsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            ViewSearchResultsControl = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            if (CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
            {
                columnHeadersCondensedRadioButton.Checked = true;
            }
            else
            {
                columnHeadersRegularRadioButton.Checked = true;
            }
        }

        private void ColumnHeadersRegularRadioButtonCheckedChanged(object sender, System.EventArgs e)
        {
            VerifyAndUpdateColumnHeaderCondensedSetting();
        }

        private void ColumnHeadersCondensedRadioButtonCheckedChanged(object sender, System.EventArgs e)
        {
            VerifyAndUpdateColumnHeaderCondensedSetting();
        }

        private void VerifyAndUpdateColumnHeaderCondensedSetting()
        {
            if (columnHeadersCondensedRadioButton.Checked !=
                CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders)
            {
                CometUI.ViewResultsSettings.DisplayOptionsCondensedColumnHeaders =
                    columnHeadersCondensedRadioButton.Checked;
                ViewSearchResultsControl.SettingsChanged = true;
            } 
        }

        private void BtnUpdateResultsClick(object sender, System.EventArgs e)
        {
            ViewSearchResultsControl.UpdateSearchResultsList();
        }
    }
}
