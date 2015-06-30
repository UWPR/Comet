using System;
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class FDRDlg : Form
    {
        // Normalized value with a min of 0 and max of 1
        public double FDRCutoff { get; private set; }

        public FDRDlg()
        {
            InitializeComponent();
            FDRCutoff = 1;
            double fdrCutoffPercent = FDRCutoff*100;
            fdrCutoffTextBox.Text = fdrCutoffPercent.ToString(CultureInfo.InvariantCulture);
        }

        private void CancelFDRCutoffBtnClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void ApplyFDRCutoffBtnClick(object sender, EventArgs e)
        {
            if (!fdrCutoffTextBox.Text.Equals(String.Empty))
            {
                // The cutoff is entered as a percentage, so normalize it
                FDRCutoff = Convert.ToDouble(fdrCutoffTextBox.Text)/100;
            }

            DialogResult = DialogResult.OK;
        }
    }
}
