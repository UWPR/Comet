using System;
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.Search.SearchSettings
{
    public partial class OutputSettingsControl : UserControl
    {
        private new SearchSettingsDlg Parent { get; set; }

        public OutputSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        public bool VerifyAndUpdateSettings()
        {
            if (pepXMLCheckBox.Checked != CometUI.SearchSettings.OutputFormatPepXML)
            {
                CometUI.SearchSettings.OutputFormatPepXML = pepXMLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (percolatorCheckBox.Checked != CometUI.SearchSettings.OutputFormatPercolator)
            {
                CometUI.SearchSettings.OutputFormatPercolator = percolatorCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outFileCheckBox.Checked != CometUI.SearchSettings.OutputFormatOutFiles)
            {
                CometUI.SearchSettings.OutputFormatOutFiles = outFileCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (textCheckBox.Checked != CometUI.SearchSettings.OutputFormatTextFile)
            {
                CometUI.SearchSettings.OutputFormatTextFile = textCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtToStdoutCheckBox.Checked != CometUI.SearchSettings.OutputFormatSqtToStandardOutput)
            {
                CometUI.SearchSettings.OutputFormatSqtToStandardOutput = sqtToStdoutCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtCheckBox.Checked != CometUI.SearchSettings.OutputFormatSqtFile)
            {
                CometUI.SearchSettings.OutputFormatSqtFile = sqtCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtExpectScoreCheckBox.Checked != CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP)
            {
                CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP = sqtExpectScoreCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outExpectScoreCheckBox.Checked != CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP)
            {
                CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP = outExpectScoreCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outShowFragmentIonsCheckBox.Checked != CometUI.SearchSettings.OutputFormatShowFragmentIons)
            {
                CometUI.SearchSettings.OutputFormatShowFragmentIons = outShowFragmentIonsCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            var numOutputLines = (int) numOutputLinesSpinner.Value;
            if (numOutputLines != CometUI.SearchSettings.NumOutputLines)
            {
                CometUI.SearchSettings.NumOutputLines = numOutputLines;
                Parent.SettingsChanged = true;
            }

            if (outSkipReSearchingCheckBox.Checked != CometUI.SearchSettings.OutputFormatSkipReSearching)
            {
                CometUI.SearchSettings.OutputFormatSkipReSearching = outSkipReSearchingCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            pepXMLCheckBox.Checked = CometUI.SearchSettings.OutputFormatPepXML;
            percolatorCheckBox.Checked = CometUI.SearchSettings.OutputFormatPercolator;
            outFileCheckBox.Checked = CometUI.SearchSettings.OutputFormatOutFiles;
            textCheckBox.Checked = CometUI.SearchSettings.OutputFormatTextFile;
            sqtToStdoutCheckBox.Checked = CometUI.SearchSettings.OutputFormatSqtToStandardOutput;
            sqtCheckBox.Checked = CometUI.SearchSettings.OutputFormatSqtFile;

            sqtExpectScoreCheckBox.Checked = CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP;
            outExpectScoreCheckBox.Checked = CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP;

            outShowFragmentIonsCheckBox.Checked = CometUI.SearchSettings.OutputFormatShowFragmentIons;

            numOutputLinesSpinner.Text = CometUI.SearchSettings.NumOutputLines.ToString(CultureInfo.InvariantCulture);

            outSkipReSearchingCheckBox.Checked = CometUI.SearchSettings.OutputFormatSkipReSearching;
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
