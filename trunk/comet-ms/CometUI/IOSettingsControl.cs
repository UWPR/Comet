using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows.Forms;
using System.IO;
using CometUI.Properties;

namespace CometUI
{
    public partial class IOSettingsControl : UserControl
    {
        private enum SearchType
        {
            SearchTypeTarget = 0,
            SearchTypeDecoyOne,
            SearchTypeDecoyTwo
            
        }
        private new Form  Parent { get; set; }

        public IOSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            comboBoxReadingFrame.Text = Settings.Default.NucleotideReadingFrame.ToString(CultureInfo.InvariantCulture);
            if (Settings.Default.IsProteinDB)
            {
                radioButtonProtein.Checked = true;
            }
            else
            {
                radioButtonNucleotide.Checked = true;
            }

            switch ((SearchType)Settings.Default.SearchType)
            {
                case SearchType.SearchTypeDecoyOne:
                    radioButtonDecoyOne.Checked = true;
                    break;
                case SearchType.SearchTypeDecoyTwo:
                    radioButtonDecoyTwo.Checked = true;
                    break;
                    case SearchType.SearchTypeTarget:
                    radioButtonTarget.Checked = true;
                    break;
                default:
                    radioButtonTarget.Checked = true;
                    break;
            }

            textBoxDecoyPrefix.Text = Settings.Default.DecoyPrefix;
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

        public string DatabaseFile 
        {
            get { return proteomeDbFileCombo.Text; }

            set { proteomeDbFileCombo.Text = value; }
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
                    for (int i = 1; i < filesError.Count-1; i++)
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

        private void BtnRemInputFileClick(object sender, EventArgs e)
        {
            RemoveFiles();
        }

        private void RemoveFiles()
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
        }

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            string databaseFile = ShowOpenDatabaseFile();
            if (null != databaseFile)
            {
                DatabaseFile = databaseFile;
            }
        }

        private static string ShowOpenDatabaseFile()
        {
            const string filter = "All Files (*.*)|*.*";
            var fdlg = new OpenFileDialog
            {
                Title = Resources.InputFilesControl_ShowOpenDatabaseFile_Open_Proteome_Database_File,
                InitialDirectory = @".",
                Filter = filter,
                FilterIndex = 1,
                Multiselect = false,
                RestoreDirectory = true
            };

            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                return fdlg.FileName;
            }

            return null;
        }

        private void RadioButtonDecoyOneCheckedChanged(object sender, EventArgs e)
        {
            panelDecoyPrefix.Enabled = radioButtonDecoyOne.Checked;
        }

        private void RadioButtonDecoyTwoCheckedChanged(object sender, EventArgs e)
        {
            panelDecoyPrefix.Enabled = radioButtonDecoyTwo.Checked;
        }

        private void RadioButtonNucleotideCheckedChanged(object sender, EventArgs e)
        {
            panelNucleotideReadingFrame.Enabled = radioButtonNucleotide.Checked;
        }

        private void SqtCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            sqtScoreCombo.Enabled = sqtCheckBox.Checked;
        }

        private void OutFileCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            outFileScoreCombo.Enabled = outFileCheckBox.Checked;
        }
    }
}
