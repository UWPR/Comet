using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class ImportParamsDlg : Form
    {
        public ImportParamsDlg()
        {
            InitializeComponent();
        }
        
        private void Button1Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            var paramsOpenFileDialog = new OpenFileDialog();
            paramsOpenFileDialog.Title = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Open_Params_File;
            paramsOpenFileDialog.InitialDirectory = @".";
            paramsOpenFileDialog.Filter = "Params files (*.params)|*.params";
            paramsOpenFileDialog.Multiselect = false;
            paramsOpenFileDialog.RestoreDirectory = true;

            if (paramsOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                paramsDbFileCombo.Text = paramsOpenFileDialog.SafeFileName;

            }

        }

        private void ImportParamsDlg_Load(object sender, EventArgs e)
        {

        }

     }
}
