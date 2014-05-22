using System;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.SettingsUI
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
            if (pepXMLCheckBox.Checked != Settings.Default.OutputFormatPepXML)
            {
                Settings.Default.OutputFormatPepXML = pepXMLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (pinXMLCheckBox.Checked != Settings.Default.OutputFormatPinXML)
            {
                Settings.Default.OutputFormatPinXML = pinXMLCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outFileCheckBox.Checked != Settings.Default.OutputFormatOutFiles)
            {
                Settings.Default.OutputFormatOutFiles = outFileCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (textCheckBox.Checked != Settings.Default.OutputFormatTextFile)
            {
                Settings.Default.OutputFormatTextFile = textCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtCheckBox.Checked != Settings.Default.OutputFormatSqtFile)
            {
                Settings.Default.OutputFormatSqtFile = sqtCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (sqtExpectScoreCheckBox.Checked != Settings.Default.PrintExpectScoreInPlaceOfSP)
            {
                Settings.Default.PrintExpectScoreInPlaceOfSP = sqtExpectScoreCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outExpectScoreCheckBox.Checked != Settings.Default.PrintExpectScoreInPlaceOfSP)
            {
                Settings.Default.PrintExpectScoreInPlaceOfSP = outExpectScoreCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            if (outShowFragmentIonsCheckBox.Checked != Settings.Default.OutputFormatShowFragmentIons)
            {
                Settings.Default.OutputFormatShowFragmentIons = outShowFragmentIonsCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            // No way for Convert to throw an exception here because we
            // already limit the input of the spinner control to int values.
            var numOutputLines = Convert.ToInt32(numOutputLinesSpinner.Text);
            if (numOutputLines != Settings.Default.NumOutputLines)
            {
                Settings.Default.NumOutputLines = numOutputLines;
                Parent.SettingsChanged = true;
            }

            if (outSkipReSearchingCheckBox.Checked != Settings.Default.OutputFormatSkipReSearching)
            {
                Settings.Default.OutputFormatSkipReSearching = outSkipReSearchingCheckBox.Checked;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void InitializeFromDefaultSettings()
        {
            pepXMLCheckBox.Checked = Settings.Default.OutputFormatPepXML;
            pinXMLCheckBox.Checked = Settings.Default.OutputFormatPinXML;
            outFileCheckBox.Checked = Settings.Default.OutputFormatOutFiles;
            textCheckBox.Checked = Settings.Default.OutputFormatTextFile;
            sqtCheckBox.Checked = Settings.Default.OutputFormatSqtFile;

            sqtExpectScoreCheckBox.Checked = Settings.Default.PrintExpectScoreInPlaceOfSP;
            outExpectScoreCheckBox.Checked = Settings.Default.PrintExpectScoreInPlaceOfSP;

            outShowFragmentIonsCheckBox.Checked = Settings.Default.OutputFormatShowFragmentIons;

            numOutputLinesSpinner.Text = Settings.Default.NumOutputLines.ToString(CultureInfo.InvariantCulture);

            outSkipReSearchingCheckBox.Checked = Settings.Default.OutputFormatSkipReSearching;
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
