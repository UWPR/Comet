using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.SettingsUI;
using CometWrapper;

namespace CometUI
{
    public partial class RunSearchDlg : Form
    {
        private new Form Parent { get; set; }
        private readonly CometSearchManagerWrapper _searchMgr;

        public RunSearchDlg(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();

            _searchMgr = new CometSearchManagerWrapper();
        }

        private void InitializeFromDefaultSettings()
        {
            var filesList = new List<string>();
            var inputFilesChecked = new List<bool>();
            foreach (var item in Settings.Default.InputFiles)
            {
                string[] row = item.Split(',');
                filesList.Add(row[0]);
                inputFilesChecked.Add(row[1].Equals("1"));
            }

            InputFiles = filesList.ToArray();
            for (int i = 0; i < inputFilesList.Items.Count; i++)
            {
                inputFilesList.SetItemChecked(i, inputFilesChecked[i]);
            }
        }

        private string[] _inputFiles = new string[0];
        public string[] InputFiles
        {
            get { return _inputFiles; }

            set
            {
                // Store checked state for existing files
                var checkStates = new Dictionary<String, bool>();
                for (int i = 0; i < _inputFiles.Length; i++)
                {
                    checkStates.Add(_inputFiles[i], inputFilesList.GetItemChecked(i));
                }

                // Set new value
                _inputFiles = value;

                inputFilesList.BeginUpdate();

                // Populate the input files list
                inputFilesList.Items.Clear();
                foreach (String fileName in _inputFiles)
                {
                    bool checkFile;
                    if (!checkStates.TryGetValue(fileName, out checkFile))
                    {
                        checkFile = true; // New files start out checked
                    }
                    inputFilesList.Items.Add(fileName, checkFile);
                }

                inputFilesList.EndUpdate();

                btnRemInputFile.Enabled = inputFilesList.CheckedItems.Count > 0;
            }
        }

        private void BtnSettingsClick(object sender, EventArgs e)
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
                // Do something here?  Maybe save the settings?
            }
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnAddInputFileClick(object sender, EventArgs e)
        {
            IEnumerable<string> addFiles = ShowAddInputFile();
            if (addFiles != null)
            {
                string[] addValidFiles = AddInputFiles(Parent, InputFiles, addFiles);
                AddInputFiles(addValidFiles);
            }
        }

        private static IEnumerable<string> ShowAddInputFile()
        {
            const string filter = "Search Input Files (*.mzXML, *.mzML, *.ms2, *.cms2)|*.mzXML;*.mzML;*.ms2;*.cms2|All Files (*.*)|*.*";
            var fdlg = new OpenFileDialog
            {
                Title = Resources.InputFilesControl_ShowAddInputFile_Open_Input_File,
                InitialDirectory = @".",
                Filter = filter,
                FilterIndex = 1,
                Multiselect = true,
                RestoreDirectory = true
            };

            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                return fdlg.FileNames;
            }

            return null;
        }

        private void AddInputFiles(IEnumerable<string> inputFiles)
        {
            InputFiles = AddInputFiles(Parent, InputFiles, inputFiles);
        }

        public static string[] AddInputFiles(Form parent, IEnumerable<string> inputFileNames, IEnumerable<string> fileNames)
        {
            var filesNew = new List<string>(inputFileNames);
            var filesError = new List<string>();
            foreach (var fileName in fileNames)
            {
                if (IsValidInputFile(fileName))
                {
                    if (inputFileNames != null && !filesNew.Contains(fileName))
                    {
                        filesNew.Add(fileName);
                    }
                }
                else
                {
                    filesError.Add(fileName);
                }
            }

            if (filesError.Count > 0)
            {
                string errorMessage;
                if (filesError.Count == 1)
                {
                    errorMessage = String.Format("The file {0} is not a valid search input file.", filesError[0]);
                }
                else
                {
                    errorMessage = String.Format("The files {0}", filesError[0]);
                    for (int i = 1; i < filesError.Count - 1; i++)
                    {
                        errorMessage += String.Format(", {0}", filesError[i]);
                    }

                    errorMessage += String.Format(" and {0} are not valid search input files.",
                                                  filesError[filesError.Count - 1]);
                }

                MessageBox.Show(errorMessage, Resources.CometUI_Title_Error, MessageBoxButtons.OKCancel);
            }

            return filesNew.ToArray();
        }

        private static bool IsValidInputFile(string fileName)
        {
            var extension = Path.GetExtension(fileName);
            if (extension != null)
            {
                string fileExt = extension.ToLower();
                return File.Exists(fileName) &&
                       (fileExt == ".mzxml" || fileExt == ".mzml" || fileExt == ".ms2" || fileExt == ".cms2");
            }
            return false;
        }

        private static InputType GetInputFileType(string inputFileName)
        {
            var inputType = InputType.Unknown;
            var extension = Path.GetExtension(inputFileName);
            if (extension != null)
            {
                string fileExt = extension.ToLower();
                switch (fileExt)
                {
                    case ".mzxml":
                        inputType = InputType.MZXML;
                        break;
                    case ".mzml":
                        inputType = InputType.MZML;
                        break;
                    case ".ms2":
                        inputType = InputType.MS2;
                        break;
                    case ".cms2":
                        inputType = InputType.CMS2;
                        break;
                }
            }

            return inputType;
        }

        private void BtnRemInputFileClick(object sender, EventArgs e)
        {
            var checkedIndices = inputFilesList.CheckedIndices;
            var inputFileNames = InputFiles.ToList();
            for (int i = checkedIndices.Count - 1; i >= 0; i--)
            {
                inputFileNames.RemoveAt(i);
            }

            InputFiles = inputFileNames.ToArray();
        }

        private void InputFilesListItemCheck(object sender, ItemCheckEventArgs e)
        {
            int checkedItemsCount = inputFilesList.CheckedItems.Count;
            if (e.NewValue == CheckState.Checked)
            {
                checkedItemsCount += 1;
            }
            else if (e.NewValue == CheckState.Unchecked)
            {
                checkedItemsCount -= 1;
            }

            btnRemInputFile.Enabled = checkedItemsCount > 0;
            btnRunSearch.Enabled = checkedItemsCount > 0;
        }

        private void BtnRunSearchClick(object sender, EventArgs e)
        {
            RunSearch();
        }

        private void RunSearch()
        {
            // Set up the input files
            var inputFiles = new List<InputFileInfoWrapper>();
            foreach (var inputFile in InputFiles)
            {
                var inputFileInfo = new InputFileInfoWrapper();
                inputFileInfo.set_InputType(GetInputFileType(inputFile));
                inputFileInfo.set_FirstScan(0);
                inputFileInfo.set_LastScan(0);
                inputFileInfo.set_AnalysisType(AnalysisType.EntireFile);
                inputFileInfo.set_FileName(inputFile);
                inputFileInfo.set_BaseName(Path.GetFileNameWithoutExtension(inputFile));
                inputFiles.Add(inputFileInfo);
            }

            _searchMgr.AddInputFiles(inputFiles);
        }

    }
}
