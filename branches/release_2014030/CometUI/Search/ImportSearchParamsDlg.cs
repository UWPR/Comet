using System;
using System.IO;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.Search
{
    public partial class ImportSearchParamsDlg : Form
    {
        public ImportSearchParamsDlg()
        {
            InitializeComponent();
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnBrowseParamsClick(object sender, EventArgs e)
        {
            var paramsOpenFileDialog = new OpenFileDialog
            {
                Title = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Open_Params_File,
                InitialDirectory = @".",
                Filter = Resources.ImportParamsDlg_BtnBrowseProteomeDbFileClick_Comet_Params_Files____params____params,
                Multiselect = false,
                RestoreDirectory = true
            };

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
            var cometParamsReader = new CometParamsReader(@paramsFileCombo.Text);
            var paramsMap = new CometParamsMap();
            bool succeeded = cometParamsReader.ReadParamsFile(paramsMap);
            cometParamsReader.Close();
            if (succeeded)
            {
                succeeded = paramsMap.GetSettingsFromCometParams(CometUI.SearchSettings);
            }

            if (succeeded)
            {
                MessageBox.Show(Resources.ImportParamsDlg_BtnImportClick_Import_completed_successfully_, Resources.ImportParamsDlg_BtnImportClick_Import_Search_Settings, MessageBoxButtons.OK, MessageBoxIcon.Information);
                DialogResult = DialogResult.OK;
            }
            else
            {
                MessageBox.Show(Resources.ImportParamsDlg_BtnImportClick_Import_failed_ + cometParamsReader.ErrorMessage, Resources.ImportParamsDlg_BtnImportClick_Import_Search_Settings, MessageBoxButtons.OK, MessageBoxIcon.Error);
                DialogResult = DialogResult.Cancel;
            }
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