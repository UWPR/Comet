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
            numOutputLinesSpinner.Text = Settings.Default.NumOutputLines.ToString(CultureInfo.InvariantCulture);
        }
    }
}
