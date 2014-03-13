using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class MiscSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        public MiscSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            numThreadsCombo.SelectedItem = Settings.Default.NumThreads.ToString(CultureInfo.InvariantCulture);

            spectrumBatchSizeTextBox.Text = Settings.Default.SpectrumBatchSize.ToString(CultureInfo.InvariantCulture);

            numResultsTextBox.Text = Settings.Default.NumResults.ToString(CultureInfo.InvariantCulture);

            maxFragmentChargeCombo.SelectedItem = Settings.Default.MaxFragmentCharge.ToString(CultureInfo.InvariantCulture);

            maxPrecursorChargeCombo.SelectedItem = Settings.Default.MaxPrecursorCharge.ToString(CultureInfo.InvariantCulture);

            clipNTermMethionineCheckBox.Checked = Settings.Default.ClipNTermMethionine;

            mzxmlScanRangeMinTextBox.Text = Settings.Default.mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlScanRangeMaxTextBox.Text = Settings.Default.mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMinTextBox.Text = Settings.Default.mzxmlPrecursorChargeRangeMin.ToString(CultureInfo.InvariantCulture);
            mzxmlPrecursorChargeMaxTextBox.Text = Settings.Default.mzxmlPrecursorChargeRangeMax.ToString(CultureInfo.InvariantCulture);
            mzxmlMsLevelCombo.SelectedItem = Settings.Default.mzxmlMsLevel.ToString(CultureInfo.InvariantCulture);
            mzxmlActivationLevelCombo.SelectedItem = Settings.Default.mzxmlActivationMethod;
        }
    }
}
