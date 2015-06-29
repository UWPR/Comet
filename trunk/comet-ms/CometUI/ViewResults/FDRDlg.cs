using System;
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class FDRDlg : Form
    {
        public double FDRCutoff { get; private set; }

        public FDRDlg()
        {
            InitializeComponent();
            FDRCutoff = 0.0;
            fdrCutoffTextBox.Text = FDRCutoff.ToString(CultureInfo.InvariantCulture);
        }

        private void CancelFDRCutoffBtnClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void ApplyFDRCutoffBtnClick(object sender, EventArgs e)
        {
            if (!fdrCutoffTextBox.Text.Equals(String.Empty))
            {
                FDRCutoff = Convert.ToDouble(fdrCutoffTextBox.Text);
            }

            DialogResult = DialogResult.OK;
        }
    }
}
