using System;
using System.Collections.Generic;
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

        private static string ConvertStringArrayToString(IEnumerable<string> array)
        {
            var builder = new StringBuilder();
            foreach (string v in array)
            {
                builder.Append(v);
                builder.AppendLine();
            }
            return builder.ToString();
        }

        private void BtnImportClick(object sender, EventArgs e)
        {
            var cometParamsReader = new CometParamsReader(@paramsFileCombo.Text);
            var paramsMap = new CometParamsMap();
            cometParamsReader.ReadParamsFile(paramsMap);

            
            //var listValues = new List<string>();
            //bool proceedWithImport = true;
            //bool staticModCheck = true;
            //var paramsMap = new CometParamsMap();
            //StreamReader sr = File.OpenText(@paramsFileCombo.Text);
            //string line;
            //while ((line = sr.ReadLine()) != null)
            //{
            //    if (!line.Contains("add") && !line.Contains("term") && !line.Contains("additional"))
            //    {
            //        staticModCheck = false;
            //    }
            //    else
            //    {
            //        staticModCheck = true;
            //    }
            //    if (line.Contains('#') && staticModCheck)
            //    {
            //        string newLine = line.Substring(0, line.IndexOf("#", StringComparison.Ordinal) + 1);
            //        newLine = newLine.Replace('#', ' ');
            //        if (!string.IsNullOrWhiteSpace(newLine))
            //        {
            //            string[] temp = newLine.Split('=');
            //            string name = temp[0].Trim();
            //            string value = temp[1].Trim();
            //            if (!paramsMap.SetCometParam(name, value))
            //            {
            //                proceedWithImport = false;
            //            }
            //        }
            //    }
            //    else
            //    {
            //        if (!string.IsNullOrWhiteSpace(line) && staticModCheck)
            //        {
            //            if ((!line.Contains('=')) &&
            //                (!line.Contains("[COMET_ENZYME_INFO]")))
            //            {
            //                int x = 0;
            //                for (int i = 0; i < 1; i++)
            //                {
            //                    const string name = "[COMET_ENZYME_INFO]";
            //                    string[] temp = line.Split(new[] {' '}, StringSplitOptions.RemoveEmptyEntries);
            //                    for (int j = 0; j < 5; j++)
            //                    {
            //                        temp[j] = temp[j].Trim();
            //                    }
            //                    string values = string.Join(",", temp);
            //                    values = values.Replace(".", "");
            //                    listValues.Add(values);
            //                    x++;
            //                    if (x == 9)
            //                    {
            //                        string[] valueArray = listValues.ToArray();
            //                        string value = ConvertStringArrayToString(valueArray);
            //                        if (!paramsMap.SetCometParam(name, value))
            //                        {
            //                            proceedWithImport = false;
            //                        }
            //                    }
            //                }
            //            }
            //            else if ((line.Contains('=')) &&
            //                     (!line.Contains("[COMET_ENZYME_INFO]")))
            //            {
            //                string[] temp = line.Split('=');
            //                string name = temp[0].Trim();
            //                string value = temp[1].Trim();
            //                if (!paramsMap.SetCometParam(name, value))
            //                {
            //                    proceedWithImport = false;
            //                }
            //            }
            //            else if ((line.Contains("# comet_version")))
            //            {
            //                //check if it matches current version else fail message
            //            }
            //        }
            //    }
            //}
            //if (proceedWithImport)
            //{
            //    if (paramsMap.GetSettingsFromCometParams(CometUI.SearchSettings))
            //    {
            //        MessageBox.Show("Import has succeeded", "Import Search Settings", MessageBoxButtons.OK, MessageBoxIcon.Information);
            //        DialogResult = DialogResult.OK;
            //    }
            //    else
            //    {
            //        MessageBox.Show("Import has failed.", "Import Search Settings", MessageBoxButtons.OK, MessageBoxIcon.Error);
            //    }
            //}
            //else
            //{
            //    MessageBox.Show("Import has failed.", "Import Search Settings", MessageBoxButtons.OK, MessageBoxIcon.Error);
            //}
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