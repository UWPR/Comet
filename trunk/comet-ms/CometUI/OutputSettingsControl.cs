using System;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class OutputSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        public OutputSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
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
        }

        private void SqtCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            sqtExpectScoreCheckBox.Enabled = sqtCheckBox.Checked;
        }

        private void OutFileCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            outExpectScoreCheckBox.Enabled = outFileCheckBox.Checked;
            outShowFragmentIonsCheckBox.Enabled = outFileCheckBox.Checked;
        }
    }
}
