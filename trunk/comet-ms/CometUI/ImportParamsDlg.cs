using System;
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
        
        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnBrowseParamsClick(object sender, EventArgs e)
        {
            var paramsOpenFileDialog = new OpenFileDialog();
            paramsOpenFileDialog.Title = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Open_Params_File;
            paramsOpenFileDialog.InitialDirectory = @".";
            paramsOpenFileDialog.Filter = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Comet_Params_Files____params____params;
            paramsOpenFileDialog.Multiselect = false;
            paramsOpenFileDialog.RestoreDirectory = true;

            if (paramsOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                paramsDbFileCombo.Text = paramsOpenFileDialog.FileName;
            }
        }
     }
}
