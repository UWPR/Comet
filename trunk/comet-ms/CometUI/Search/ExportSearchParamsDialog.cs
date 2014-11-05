using System;
using System.IO;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.Search.SearchSettings;

namespace CometUI.Search
{
    public partial class ExportParamsDlg : Form
    {
        public ExportParamsDlg()
        {
            InitializeComponent();
            AcceptButton = btnExport;
            textBoxName.Text = Resources.ExportParamsDlg_ExportParamsDlg_comet;
            textBoxPath.Text = Directory.GetCurrentDirectory();
        }

        public string FilePath { get; set; }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnExportPathClick(object sender, EventArgs e)
        {
            var paramsFolderBrowserDialog = new FolderBrowserDialog();
            if (paramsFolderBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                textBoxPath.Text = paramsFolderBrowserDialog.SelectedPath;
            }
        }

        private void BtnExportClick(object sender, EventArgs e)
        {
            var fileName = textBoxName.Text + ".params";
            var pathString = textBoxPath.Text;
            if (ExportCometParams(fileName, pathString))
            {
                DialogResult = DialogResult.OK;
            }
        }

        private bool ExportCometParams(String fileName, String pathString)
        {
            // If there are any invalid characters in the file name, cancel and display warning
            if (!IsValidFileName(fileName))
            {
                MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_File_name_has_invalid_characters_,
                                Resources.ExportParamsDlg_BtnExportClick_Export_Failed, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                return false;
            }

            // Check to see if the file already exists, and if yes, ask the
            // user if they want to overwrite it.
            FilePath = Path.Combine(pathString, fileName);
            if (File.Exists(FilePath))
            {
                if (DialogResult.Yes !=
                MessageBox.Show('"' + fileName + '"' +
                                Resources.
                                    ExportParamsDlg_BtnExportClick__already_exists_in_this_folder__Would_you_like_to_replace_it_,
                                Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                                MessageBoxButtons.YesNo,
                                MessageBoxIcon.Warning))
                {
                    return false;
                }
            }

            var paramsMap = new CometParamsMap(CometUI.SearchSettings);
            
            if (!CheckProteomeDatabaseFile(paramsMap))
            {
                // Show message indicating absence of "database_name" in params file
                if (DialogResult.OK != MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_You_have_not_specified_a_proteome_database_name_in_the_Input_Settings_The_params_file_you_are_about_to_create_will_leave_the_database_name_field_blank,
                    Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                    MessageBoxButtons.OKCancel, MessageBoxIcon.Warning))
                {
                    return false;
                }
            }

            var cometParamsWriter = new CometParamsWriter(FilePath);
            if (!cometParamsWriter.WriteParamsFile(paramsMap))
            {
                MessageBox.Show(Resources.ExportParamsDlg_ExportCometParams_Failed_to_generate_the_params_file_ + cometParamsWriter.ErrorMessage,
                                Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                                MessageBoxButtons.OK, MessageBoxIcon.Error);
                cometParamsWriter.Close();
                return false;
            }

            cometParamsWriter.Close();
            return true;
        }

        private bool IsValidFileName(String fileName)
        {
            char[] invalidFileNameChars = Path.GetInvalidFileNameChars();
            return fileName.IndexOfAny(invalidFileNameChars) == -1;
        }

        private bool CheckProteomeDatabaseFile(CometParamsMap paramsMap)
        {
            var dbNameParam = paramsMap.CometParams["database_name"];
            return !String.IsNullOrEmpty(dbNameParam.Value);
        }

        private void ExportTextChange()
        {
            string fileName = textBoxName.Text;
            string filePath = textBoxPath.Text;

            btnExport.Enabled = (fileName != string.Empty) && Directory.Exists(filePath);
        }

        private void TextBoxNameTextChanged(object sender, EventArgs e)
        {
            ExportTextChange();
        }

        private void TextBoxPathTextChanged(object sender, EventArgs e)
        {
            ExportTextChange();
        }

        private void BtnSettingsClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
            }
        }
    }
}