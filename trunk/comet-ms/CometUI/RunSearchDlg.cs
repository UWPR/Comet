using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows.Forms;
using CometUI.Properties;
using CometUI.SettingsUI;
using CometWrapper;

namespace CometUI
{
    public partial class RunSearchDlg : Form
    {
        private new Form Parent { get; set; }
        
        public RunSearchDlg(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
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
            const string filter =
                "Search Input Files (*.mzXML, *.mzML, *.ms2, *.cms2)|*.mzXML;*.mzML;*.ms2;*.cms2|All Files (*.*)|*.*";
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

        public static string[] AddInputFiles(Form parent, IEnumerable<string> inputFileNames,
                                             IEnumerable<string> fileNames)
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
            DialogResult = DialogResult.OK;
        }

        public bool RunSearch()
        {
            // Begin with the assumption that the search is going to fail,
            // and only set it to succeeded at the end if run completes
            // successfully
            SearchSucceeded = false;

            var searchMgr = new CometSearchManagerWrapper();

            if (!ConfigureInputFiles(searchMgr))
            {
                return false;
            }

            if (!ConfigureInputSettings(searchMgr))
            {
                return false;
            }

            Thread.Sleep(10000);

            SearchStatusMessage = "Search completed successfully.";
            SearchSucceeded = true;
            return true;
        }

        private bool ConfigureInputFiles(CometSearchManagerWrapper searchMgr)
        {
            // Set up the input files
            var inputFiles = new List<InputFileInfoWrapper>();
            foreach (var inputFile in InputFiles)
            {
                var inputFileInfo = new InputFileInfoWrapper();
                var inputType = GetInputFileType(inputFile);
                inputFileInfo.set_InputType(inputType);
                if (inputType == InputType.MZXML)
                {
                    inputFileInfo.set_FirstScan(Settings.Default.mzxmlScanRangeMin);
                    inputFileInfo.set_LastScan(Settings.Default.mzxmlScanRangeMax);
                }
                else
                {
                    inputFileInfo.set_FirstScan(0);
                    inputFileInfo.set_LastScan(0);
                }

                inputFileInfo.set_AnalysisType(AnalysisType.EntireFile);
                inputFileInfo.set_FileName(inputFile);
                inputFileInfo.set_BaseName(Path.GetFileNameWithoutExtension(inputFile));
                inputFiles.Add(inputFileInfo);
            }

            if (!searchMgr.AddInputFiles(inputFiles))
            {
                SearchStatusMessage = "Could not add input files.";
                return false;
            }

            return true;
        }

        private bool ConfigureInputSettings(CometSearchManagerWrapper searchMgr)
        {
            // Set up the proteome database
            var dbFileName = Settings.Default.ProteomeDatabaseFile;
            if (!searchMgr.SetParam("database_name", dbFileName, dbFileName))
            {
                SearchStatusMessage = "Could not set proteome database parameter.";
                return false;
            }

            // Set up the target vs. decoy parameters
            var searchType = Settings.Default.SearchType;
            if (!searchMgr.SetParam("decoy_search", searchType.ToString(CultureInfo.InvariantCulture), searchType))
            {
                SearchStatusMessage = "Could not set search type.";
                return false;
            }

            var decoyPrefix = Settings.Default.DecoyPrefix;
            if (!searchMgr.SetParam("decoy_prefix", decoyPrefix, decoyPrefix))
            {
                SearchStatusMessage = "Could not set the decoy prefix.";
                return false;
            }

            var nucleotideReadingFrame = Settings.Default.NucleotideReadingFrame;
            if (!searchMgr.SetParam("nucleotide_reading_frame", nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture), nucleotideReadingFrame))
            {
                SearchStatusMessage = "Could not set the nucleotide reading frame.";
                return false;
            }

            return true;
        }

        public String SearchStatusMessage { get; private set; }
        public bool SearchSucceeded { get; private set; } 
    }
}
