using System;
using System.IO;
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
                paramsFileCombo.Text = paramsOpenFileDialog.FileName;
            }
            btnImport.Enabled = false;
            string path = paramsFileCombo.Text;
            if (File.Exists(path))
            {
                btnImport.Enabled = true;
            }
        }

        private void BtnImportClick(object sender, EventArgs e)
        {
            
        }

        private void ParamsTextChanged()
        {
            string path = paramsFileCombo.Text;
            btnImport.Enabled = File.Exists(path);
        }

        private void ParamsFileComboSelectedIndexChanged(object sender, EventArgs e)
        {
            ParamsTextChanged();
        }

        private void ParamsFileComboTextChanged(object sender, EventArgs e)
        {
            ParamsTextChanged();
        }
     }
}
