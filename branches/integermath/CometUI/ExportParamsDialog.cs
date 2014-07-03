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
            string fileName = textBoxName.Text + ".params";
            string pathString = textBoxPath.Text;
            char[] invalidPathChars = Path.GetInvalidPathChars();

            // If there are any invalid characters in the file name, cancel and display warning
            if (fileName.IndexOfAny(invalidPathChars) != -1)
            {
                MessageBox.Show(Resources.ExportParamsDlg_BtnExportClick_File_name_has_invalid_characters_,
                                Resources.ExportParamsDlg_BtnExportClick_Export_Failed, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
            }
            else
            {
                FilePath = Path.Combine(pathString, fileName);
                bool proceedWithExport;
                if (File.Exists(FilePath))
                {
                    proceedWithExport = DialogResult.Yes ==
                    MessageBox.Show('"' + fileName + '"' +
                                    Resources.
                                        ExportParamsDlg_BtnExportClick__already_exists_in_this_folder__Would_you_like_to_replace_it_,
                                    Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                                    MessageBoxButtons.YesNo,
                                    MessageBoxIcon.Warning);
                }
                else
                {
                    // If the file doesn't already exist, by default,
                    proceedWithExport = true;
                }

                var paramsMap = new CometParamsMap(Settings.Default);
                Dictionary<string, CometParam> map = paramsMap.CometParams;

                if (proceedWithExport)
                {
                    bool proceedWithExport2 = false;
                    foreach (var pair in map)
                    {
                        if (pair.Key == "database_name")
                        {
                            string value = pair.Value.Value;
                            if (string.IsNullOrEmpty(value))
                            {
                                //Change message dealing with absence of "database_name" in params file
                                proceedWithExport2 = DialogResult.OK == MessageBox.Show(
                                    Resources.
                                        ExportParamsDlg_BtnExportClick_You_have_not_specified_a_proteome_database_name_in_the_Input_Settings_The_params_file_you_are_about_to_create_will_leave_the_database_name_field_blank,
                                    Resources.ExportParamsDlg_BtnExportClick_Export_Search_Settings,
                                    MessageBoxButtons.OKCancel, MessageBoxIcon.Warning);
                            }
                            else
                            {
                                proceedWithExport2 = true;
                            }
                        }
                    }
                    if (proceedWithExport2)
                    {
                        using (var sw = new StreamWriter(FilePath))
                        {
                            var searchManager = new CometSearchManagerWrapper();
                            String cometVersion = String.Empty;
                            if (!searchManager.GetParamValue("# comet_version ", ref cometVersion))
                            {
                                MessageBox.Show(
                                    Resources.
                                        ExportParamsDlg_BtnExportClick_Unable_to_get_the_Comet_version__Settings_cannot_be_exported_without_a_valid_Comet_version,
                                    Resources.ExportParamsDlg_BtnExportClick_Error, MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                                DialogResult = DialogResult.Abort;
                            }
                            else
                            {
                                // write the version to the params file here
                                sw.WriteLine("# comet_version " + cometVersion);

                                foreach (var pair in map)
                                {
                                    if (pair.Key == "[COMET_ENZYME_INFO]")
                                    {
                                        sw.WriteLine(pair.Key + Environment.NewLine + pair.Value.Value);
                                    }
                                    else
                                    {
                                        sw.WriteLine(pair.Key + " = " + pair.Value.Value);
                                    }
                                }

                                sw.Flush();

                                DialogResult = DialogResult.OK;
                            }
                        }
                    }
                }
            }
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