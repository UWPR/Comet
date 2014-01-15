using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace CometUI
{
    public partial class EnzymeInfoDlg : Form
    {
        public string SelectedEnzymeName { get; set; }

        private EnzymeSettingsControl EnzymeSettingsDlg { get; set; }

        public EnzymeInfoDlg(EnzymeSettingsControl enzymeSettings)
        {
            InitializeComponent();

            EnzymeSettingsDlg = enzymeSettings;

            //List<string[]> enzymeInfoList = new List<string[]>();

            //string[] row0 = { "0", "No_enzyme", "0", "-", "-" };
            //enzymeInfoList.Add(row0);

            //string[] row1 = { "1", "Trypsin", "1", "KR", "P" };
            //enzymeInfoList.Add(row1);

            //string[] row2 = { "2", "Trypsin/P", "1", "KR", "-" };
            //enzymeInfoList.Add(row2);

            //string[] row3 = { "3", "Lys_C", "1", "K", "P" };
            //enzymeInfoList.Add(row3);

            //string[] row4 = { "4", "Lys_N", "0", "K", "-" };
            //enzymeInfoList.Add(row4);

            //string[] row5 = { "5", "Arg_C", "1", "R", "P" };
            //enzymeInfoList.Add(row5);

            //string[] row6 = { "6", "Asp_N", "0", "D", "-" };
            //enzymeInfoList.Add(row6);

            //string[] row7 = { "7", "CNBr", "1", "M", "-" };
            //enzymeInfoList.Add(row7);

            //string[] row8 = { "8", "Glu_C", "1", "DE", "P" };
            //enzymeInfoList.Add(row8);

            //string[] row9 = { "9", "PepsinA", "1", "FL", "P" };
            //enzymeInfoList.Add(row9);

            //string[] row10 = { "10", "Chymotrypsin", "1", "FWYL", "P" };
            //enzymeInfoList.Add(row10);

            foreach (string[] row in EnzymeSettingsDlg.EnzymeInfo)
            {
                enzymeInfoDataGridView.Rows.Add(row);
            }

        }

        private void EnzymeInfoOkButtonClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }

        private void EnzymeInfoCancelButtonClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void EnzymeInfoDataGridViewSelectionChanged(object sender, EventArgs e)
        {

        }
    }
}
