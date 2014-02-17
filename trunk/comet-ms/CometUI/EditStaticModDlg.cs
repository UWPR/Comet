using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.VisualStyles;

namespace CometUI
{
    public partial class EditStaticModDlg : Form
    {
        public bool AvgMassChanged { get; private set; }
        public double AvgMass { get; private set; }
        public bool MonoMassChanged { get; private set; }
        public double MonoMass { get; private set; }
        public EditStaticModDlg(StaticMod staticMod)
        {
            InitializeComponent();

            staticModNameTextBox.Text = staticMod.Name;
            residueTextBox.Text = staticMod.Residue;
            monoisotopicTextBox.Text = staticMod.MonoisotopicMass.ToString(CultureInfo.InvariantCulture);
            avgTextBox.Text = staticMod.AvgMass.ToString(CultureInfo.InvariantCulture);

            monoisotopicRadioButton.Checked = true;

            if (staticMod.Name.Contains("User Amino Acid"))
            {
                monoisotopicTextBox.Enabled = true;
                avgTextBox.Enabled = true;
            }

            MonoMassChanged = false;
            AvgMassChanged = false;

            MonoMass = Convert.ToDouble(monoisotopicTextBox.Text);
            AvgMass = Convert.ToDouble(avgTextBox.Text);
        }

        private void CancelButtonClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void MonoisotopicTextBoxTextChanged(object sender, EventArgs e)
        {
            MonoMassChanged = true;
        }

        private void AvgTextBoxTextChanged(object sender, EventArgs e)
        {
            AvgMassChanged = true;
        }

        private void OkButtonClick(object sender, EventArgs e)
        {
            MonoMass = Convert.ToDouble(monoisotopicTextBox.Text);
            AvgMass = Convert.ToDouble(avgTextBox.Text);

            DialogResult = DialogResult.OK;
        }
    }
}
