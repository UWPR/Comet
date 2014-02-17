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
        public EditStaticModDlg(Modification staticMod)
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
        }

        private void CancelButtonClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }
    }
}
