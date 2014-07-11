using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using CometUI.Properties;
using CometWrapper;

namespace CometUI
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

            var paramsMap = new CometParamsMap(Settings.Default);
            Dictionary<string, CometParam> map = paramsMap.CometParams;

            var dbNameParam = map["database_name"];
            if (String.IsNullOrEmpty(dbNameParam.Value))
            {
                // Show message indicating absence of "database_name" in params file
                if (DialogResult.OK != MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_You_have_not_specified_a_proteome_database_name_in_the_Input_Settings_The_params_file_you_are_about_to_create_will_leave_the_database_name_field_blank,
                    Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                    MessageBoxButtons.OKCancel, MessageBoxIcon.Warning))
                {
                    return false;
                }
            }

            using (var sw = new StreamWriter(FilePath))
            {
                var searchManager = new CometSearchManagerWrapper();
                String cometVersion = String.Empty;
                if (!searchManager.GetParamValue("# comet_version ", ref cometVersion))
                {
                    MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_Unable_to_get_the_Comet_version__Settings_cannot_be_exported_without_a_valid_Comet_version,
                        Resources.ExportParamsDlg_BtnExportClick_Error, MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                    return false;
                }

                // write the version to the params file here
                sw.WriteLine("# comet_version " + cometVersion);

                foreach (var pair in map)
                {
                    if (pair.Key == "[COMET_ENZYME_INFO]")
                    {
                        WriteEnzymeInfoParam(sw, pair.Key, pair.Value.Value);
                    }
                    else
                    {
                        sw.WriteLine(pair.Key + " = " + pair.Value.Value);
                    }
                }

                sw.Flush();
            }
            
            return true;
        }

        private void WriteEnzymeInfoParam(StreamWriter sw, String paramName, String paramStrValue)
        {
            sw.WriteLine(paramName);
            String enzymeInfoStr = paramStrValue.Replace(Environment.NewLine, "\n");
            String[] enzymeInfoLines = enzymeInfoStr.Split('\n');
            foreach (var line in enzymeInfoLines)
            {
                if (!String.IsNullOrEmpty(line))
                {
                    String[] enzymeInfoRows = line.Split(',');
                    string enzymeInfoFormattedRow = enzymeInfoRows[0] + ".";
                    for (int i = 1; i < enzymeInfoRows.Length; i++)
                    {
                        enzymeInfoFormattedRow += " " + enzymeInfoRows[i];
                    }

                    sw.WriteLine(enzymeInfoFormattedRow);
                }
            }
        }

        private bool IsValidFileName(String fileName)
        {
            char[] invalidFileNameChars = Path.GetInvalidFileNameChars();
            return fileName.IndexOfAny(invalidFileNameChars) == -1;
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
    }
}