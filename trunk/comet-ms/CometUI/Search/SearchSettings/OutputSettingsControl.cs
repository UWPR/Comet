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
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
{
    /// <summary>
    /// This class represents the tab page control for allowing the user to
    /// change the output settings search parameters, such as which output 
    /// files to generate. 
    /// </summary>
    public partial class OutputSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }

        /// <summary>
        /// Constructor for the output settings tab page.
        /// </summary>
        /// <param name="parent"> The tab control hosting this tab page. </param>
        public OutputSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
        }

        /// <summary>
        /// Public method to initialize the output settings tab page from the
        /// user's settings.
        /// </summary>
        public void Initialize()
        {
            InitializeFromDefaultSettings();
        }

        /// <summary>
        /// Saves the settings the user modified to the user's settings.
        /// </summary>
        /// <returns> True if settings were updated successfully; False for error. </returns>
        public bool VerifyAndUpdateSettings()
        {
            if (pepXMLCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatPepXML)
            {
                CometUIMainForm.SearchSettings.OutputFormatPepXML = pepXMLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (percolatorCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatPercolator)
            {
                CometUIMainForm.SearchSettings.OutputFormatPercolator = percolatorCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outFileCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatOutFiles)
            {
                CometUIMainForm.SearchSettings.OutputFormatOutFiles = outFileCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (textCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatTextFile)
            {
                CometUIMainForm.SearchSettings.OutputFormatTextFile = textCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtToStdoutCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatSqtToStandardOutput)
            {
                CometUIMainForm.SearchSettings.OutputFormatSqtToStandardOutput = sqtToStdoutCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatSqtFile)
            {
                CometUIMainForm.SearchSettings.OutputFormatSqtFile = sqtCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtExpectScoreCheckBox.Checked != CometUIMainForm.SearchSettings.PrintExpectScoreInPlaceOfSP)
            {
                CometUIMainForm.SearchSettings.PrintExpectScoreInPlaceOfSP = sqtExpectScoreCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outExpectScoreCheckBox.Checked != CometUIMainForm.SearchSettings.PrintExpectScoreInPlaceOfSP)
            {
                CometUIMainForm.SearchSettings.PrintExpectScoreInPlaceOfSP = outExpectScoreCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outShowFragmentIonsCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatShowFragmentIons)
            {
                CometUIMainForm.SearchSettings.OutputFormatShowFragmentIons = outShowFragmentIonsCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            var numOutputLines = (int) numOutputLinesSpinner.Value;
            if (numOutputLines != CometUIMainForm.SearchSettings.NumOutputLines)
            {
                CometUIMainForm.SearchSettings.NumOutputLines = numOutputLines;
                Parent.SettingsChanged = true;
            }

            if (outSkipReSearchingCheckBox.Checked != CometUIMainForm.SearchSettings.OutputFormatSkipReSearching)
            {
                CometUIMainForm.SearchSettings.OutputFormatSkipReSearching = outSkipReSearchingCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        /// <summary>
        /// Initializes the input settings tab page from the settings saved
        /// in the user's settings.
        /// </summary>
        private void InitializeFromDefaultSettings()
        {
            pepXMLCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatPepXML;
            percolatorCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatPercolator;
            outFileCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatOutFiles;
            textCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatTextFile;
            sqtToStdoutCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatSqtToStandardOutput;
            sqtCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatSqtFile;

            sqtExpectScoreCheckBox.Checked = CometUIMainForm.SearchSettings.PrintExpectScoreInPlaceOfSP;
            outExpectScoreCheckBox.Checked = CometUIMainForm.SearchSettings.PrintExpectScoreInPlaceOfSP;

            outShowFragmentIonsCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatShowFragmentIons;

            numOutputLinesSpinner.Text = CometUIMainForm.SearchSettings.NumOutputLines.ToString(CultureInfo.InvariantCulture);

            outSkipReSearchingCheckBox.Checked = CometUIMainForm.SearchSettings.OutputFormatSkipReSearching;
        }

        private void SqtCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            sqtExpectScoreCheckBox.Enabled = sqtCheckBox.Checked;
        }

        private void OutFileCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            outExpectScoreCheckBox.Enabled = outFileCheckBox.Checked;
            outShowFragmentIonsCheckBox.Enabled = outFileCheckBox.Checked;
            outSkipReSearchingCheckBox.Enabled = outFileCheckBox.Checked;
        }
    }
}
