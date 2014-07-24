using System;
using System.Collections.Generic;
using System.Globalization;
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
        private new CometUI Parent { get; set; }
        
        public RunSearchDlg(CometUI parent)
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

            proteomeDbFileCombo.Text = Settings.Default.ProteomeDatabaseFile;
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

                btnRunSearch.Enabled = inputFilesList.CheckedItems.Count > 0 && File.Exists(proteomeDbFileCombo.Text);
                btnRemInputFile.Enabled = inputFilesList.CheckedItems.Count > 0;
            }
        }

        private void BtnSettingsClick(object sender, EventArgs e)
        {
            OpenSettingsDialog();
        }

        private void OpenSettingsDialog()
        {
            var searchSettingsDlg = new SearchSettingsDlg();
            if (DialogResult.OK == searchSettingsDlg.ShowDialog())
            {
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
            btnRunSearch.Enabled = checkedItemsCount > 0 && File.Exists(proteomeDbFileCombo.Text);
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

            if (!ConfigureOutputSettings(searchMgr))
            {
                return false;
            }

            if (!ConfigureEnzymeSettings(searchMgr))
            {
                return false;
            }

            if (!ConfigureMassSettings(searchMgr))
            {
                return false;
            }

            if (!ConfigureStaticModSettings(searchMgr))
            {
                return false;
            }

            if (!ConfigureVariableModSettings(searchMgr))
            {
                return false;
            }

            if (!ConfigureMiscSettings(searchMgr))
            {
                return false;
            }

            if (!searchMgr.DoSearch())
            {
                string errorMessage = null;
                if (searchMgr.GetErrorMessage(ref errorMessage))
                {
                    SearchStatusMessage = errorMessage;
                }
                else
                {
                    SearchStatusMessage = "Search failed, but unable to get the error message.";
                }
                return false;
            }

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
                SearchStatusMessage = "Could not set the database_name parameter.";
                return false;
            }

            // Set up the target vs. decoy parameters
            var searchType = Settings.Default.SearchType;
            if (!searchMgr.SetParam("decoy_search", searchType.ToString(CultureInfo.InvariantCulture), searchType))
            {
                SearchStatusMessage = "Could not set the decoy_search parameter.";
                return false;
            }

            var decoyPrefix = Settings.Default.DecoyPrefix;
            if (!searchMgr.SetParam("decoy_prefix", decoyPrefix, decoyPrefix))
            {
                SearchStatusMessage = "Could not set the decoy_prefix parameter.";
                return false;
            }

            var nucleotideReadingFrame = Settings.Default.NucleotideReadingFrame;
            if (!searchMgr.SetParam("nucleotide_reading_frame", nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture), nucleotideReadingFrame))
            {
                SearchStatusMessage = "Could not set the nucleotide_reading_frame parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureOutputSettings(CometSearchManagerWrapper searchMgr)
        {
            var outputPepXMLFile = Settings.Default.OutputFormatPepXML ? 1 : 0;
            if (!searchMgr.SetParam("output_pepxmlfile", outputPepXMLFile.ToString(CultureInfo.InvariantCulture), outputPepXMLFile))
            {
                SearchStatusMessage = "Could not set the output_pepxmlfile parameter.";
                return false;
            }

            var outputPinXMLFile = Settings.Default.OutputFormatPinXML ? 1 : 0;
            if (!searchMgr.SetParam("output_pinxmlfile", outputPinXMLFile.ToString(CultureInfo.InvariantCulture), outputPinXMLFile))
            {
                SearchStatusMessage = "Could not set the output_pinxmlfile parameter.";
                return false;
            }

            var outputTextFile = Settings.Default.OutputFormatTextFile ? 1 : 0;
            if (!searchMgr.SetParam("output_txtfile", outputTextFile.ToString(CultureInfo.InvariantCulture), outputTextFile))
            {
                SearchStatusMessage = "Could not set the output_txtfile parameter.";
                return false;
            }

            var outputSqtToStdout = Settings.Default.OutputFormatSqtToStandardOutput ? 1 : 0;
            if (!searchMgr.SetParam("output_sqtstream", outputSqtToStdout.ToString(CultureInfo.InvariantCulture), outputSqtToStdout))
            {
                SearchStatusMessage = "Could not set the output_sqtstream parameter.";
                return false;
            }

            var outputSqtFile = Settings.Default.OutputFormatSqtFile ? 1 : 0;
            if (!searchMgr.SetParam("output_sqtfile", outputSqtFile.ToString(CultureInfo.InvariantCulture), outputSqtFile))
            {
                SearchStatusMessage = "Could not set the output_sqtfile parameter.";
                return false;
            }

            var outputOutFile = Settings.Default.OutputFormatOutFiles ? 1 : 0;
            if (!searchMgr.SetParam("output_outfiles", outputOutFile.ToString(CultureInfo.InvariantCulture), outputOutFile))
            {
                SearchStatusMessage = "Could not set the output_outfiles parameter.";
                return false;
            }

            var printExpectScore = Settings.Default.PrintExpectScoreInPlaceOfSP ? 1 : 0;
            if (!searchMgr.SetParam("print_expect_score", printExpectScore.ToString(CultureInfo.InvariantCulture), printExpectScore))
            {
                SearchStatusMessage = "Could not set the print_expect_score parameter.";
                return false;
            }

            var showFragmentIons = Settings.Default.OutputFormatShowFragmentIons ? 1 : 0;
            if (!searchMgr.SetParam("show_fragment_ions", showFragmentIons.ToString(CultureInfo.InvariantCulture), showFragmentIons))
            {
                SearchStatusMessage = "Could not set the show_fragment_ions parameter.";
                return false;
            }

            var skipResearching = Settings.Default.OutputFormatSkipReSearching ? 1 : 0;
            if (!searchMgr.SetParam("skip_researching", skipResearching.ToString(CultureInfo.InvariantCulture), skipResearching))
            {
                SearchStatusMessage = "Could not set the skip_researching parameter.";
                return false;
            }

            var numOutputLines = Settings.Default.NumOutputLines;
            if (!searchMgr.SetParam("num_output_lines", numOutputLines.ToString(CultureInfo.InvariantCulture), numOutputLines))
            {
                SearchStatusMessage = "Could not set the num_output_lines parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureEnzymeSettings(CometSearchManagerWrapper searchMgr)
        {
            var searchEnzymeNumber = Settings.Default.SearchEnzymeNumber;
            if (!searchMgr.SetParam("search_enzyme_number", searchEnzymeNumber.ToString(CultureInfo.InvariantCulture), searchEnzymeNumber))
            {
                SearchStatusMessage = "Could not set the search_enzyme_number parameter.";
                return false;
            }

            var sampleEnzymeNumber = Settings.Default.SampleEnzymeNumber;
            if (!searchMgr.SetParam("sample_enzyme_number", sampleEnzymeNumber.ToString(CultureInfo.InvariantCulture), sampleEnzymeNumber))
            {
                SearchStatusMessage = "Could not set the sample_enzyme_number parameter.";
                return false;
            }

            var allowedMissedCleavages = Settings.Default.AllowedMissedCleavages;
            if (!searchMgr.SetParam("allowed_missed_cleavage", allowedMissedCleavages.ToString(CultureInfo.InvariantCulture), allowedMissedCleavages))
            {
                SearchStatusMessage = "Could not set the allowed_missed_cleavage parameter.";
                return false;
            }

            var enzymeTermini = Settings.Default.EnzymeTermini;
            if (!searchMgr.SetParam("num_enzyme_termini", enzymeTermini.ToString(CultureInfo.InvariantCulture), enzymeTermini))
            {
                SearchStatusMessage = "Could not set the num_enzyme_termini parameter.";
                return false;
            }

            var digestMassMin = Settings.Default.digestMassRangeMin;
            var digestMassMax = Settings.Default.digestMassRangeMax;
            var digestMassRange = new DoubleRangeWrapper(digestMassMin, digestMassMax);
            string digestMassRangeString = digestMassMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + digestMassMax.ToString(CultureInfo.InvariantCulture);
            if (!searchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange))
            {
                SearchStatusMessage = "Could not set the digest_mass_range parameter.";
                return false;
            }


            return true;
        }

        private bool ConfigureMassSettings(CometSearchManagerWrapper searchMgr)
        {
            // Configure precursor mass settings
            
            var precursorMassTol = Settings.Default.PrecursorMassTolerance;
            if (!searchMgr.SetParam("peptide_mass_tolerance", precursorMassTol.ToString(CultureInfo.InvariantCulture), precursorMassTol))
            {
                SearchStatusMessage = "Could not set the peptide_mass_tolerance parameter.";
                return false;
            }

            var precursorMassUnit = Settings.Default.PrecursorMassUnit;
            if (!searchMgr.SetParam("peptide_mass_units", precursorMassUnit.ToString(CultureInfo.InvariantCulture), precursorMassUnit))
            {
                SearchStatusMessage = "Could not set the peptide_mass_units parameter.";
                return false;
            }

            var precursorMassType = Settings.Default.PrecursorMassType;
            if (!searchMgr.SetParam("mass_type_parent", precursorMassType.ToString(CultureInfo.InvariantCulture), precursorMassType))
            {
                SearchStatusMessage = "Could not set the mass_type_parent parameter.";
                return false;
            }

            var precursorTolType = Settings.Default.PrecursorToleranceType;
            if (!searchMgr.SetParam("precursor_tolerance_type", precursorTolType.ToString(CultureInfo.InvariantCulture), precursorTolType))
            {
                SearchStatusMessage = "Could not set the precursor_tolerance_type parameter.";
                return false;
            }

            var isotopeError = Settings.Default.PrecursorIsotopeError;
            if (!searchMgr.SetParam("isotope_error", isotopeError.ToString(CultureInfo.InvariantCulture), isotopeError))
            {
                SearchStatusMessage = "Could not set the isotope_error parameter.";
                return false;
            }

            
            // Configure fragment mass settings
            
            var fragmentBinSize = Settings.Default.FragmentBinSize;
            if (!searchMgr.SetParam("fragment_bin_tol", fragmentBinSize.ToString(CultureInfo.InvariantCulture), fragmentBinSize))
            {
                SearchStatusMessage = "Could not set the fragment_bin_tol parameter.";
                return false;
            }

            var fragmentBinOffset = Settings.Default.FragmentBinOffset;
            if (!searchMgr.SetParam("fragment_bin_offset", fragmentBinOffset.ToString(CultureInfo.InvariantCulture), fragmentBinOffset))
            {
                SearchStatusMessage = "Could not set the fragment_bin_offset parameter.";
                return false;
            }

            var fragmentMassType = Settings.Default.FragmentMassType;
            if (!searchMgr.SetParam("mass_type_fragment", fragmentMassType.ToString(CultureInfo.InvariantCulture), fragmentMassType))
            {
                SearchStatusMessage = "Could not set the mass_type_fragment parameter.";
                return false;
            }

            var useSparseMatrix = Settings.Default.UseSparseMatrix ? 1 : 0;
            if (!searchMgr.SetParam("use_sparse_matrix", useSparseMatrix.ToString(CultureInfo.InvariantCulture), useSparseMatrix))
            {
                SearchStatusMessage = "Could not set the use_sparse_matrix parameter.";
                return false;
            }

            // Configure fragment ions

            var useAIons = Settings.Default.UseAIons ? 1 : 0;
            if (!searchMgr.SetParam("use_A_ions", useAIons.ToString(CultureInfo.InvariantCulture), useAIons))
            {
                SearchStatusMessage = "Could not set the use_A_ions parameter.";
                return false;
            }

            var useBIons = Settings.Default.UseBIons ? 1 : 0;
            if (!searchMgr.SetParam("use_B_ions", useBIons.ToString(CultureInfo.InvariantCulture), useBIons))
            {
                SearchStatusMessage = "Could not set the use_B_ions parameter.";
                return false;
            }

            var useCIons = Settings.Default.UseCIons ? 1 : 0;
            if (!searchMgr.SetParam("use_C_ions", useCIons.ToString(CultureInfo.InvariantCulture), useCIons))
            {
                SearchStatusMessage = "Could not set the use_C_ions parameter.";
                return false;
            }

            var useXIons = Settings.Default.UseXIons ? 1 : 0;
            if (!searchMgr.SetParam("use_X_ions", useXIons.ToString(CultureInfo.InvariantCulture), useXIons))
            {
                SearchStatusMessage = "Could not set the use_X_ions parameter.";
                return false;
            }

            var useYIons = Settings.Default.UseYIons ? 1 : 0;
            if (!searchMgr.SetParam("use_Y_ions", useXIons.ToString(CultureInfo.InvariantCulture), useYIons))
            {
                SearchStatusMessage = "Could not set the use_Y_ions parameter.";
                return false;
            }

            var useZIons = Settings.Default.UseZIons ? 1 : 0;
            if (!searchMgr.SetParam("use_Z_ions", useZIons.ToString(CultureInfo.InvariantCulture), useZIons))
            {
                SearchStatusMessage = "Could not set the use_Z_ions parameter.";
                return false;
            }

            var useFlankIons = Settings.Default.TheoreticalFragmentIons ? 1 : 0;
            if (!searchMgr.SetParam("theoretical_fragment_ions", useFlankIons.ToString(CultureInfo.InvariantCulture), useFlankIons))
            {
                SearchStatusMessage = "Could not set the theoretical_fragment_ions parameter.";
                return false;
            }

            var useNLIons = Settings.Default.UseNLIons ? 1 : 0;
            if (!searchMgr.SetParam("use_NL_ions", useNLIons.ToString(CultureInfo.InvariantCulture), useNLIons))
            {
                SearchStatusMessage = "Could not set the use_NL_ions parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureStaticModSettings(CometSearchManagerWrapper searchMgr)
        {
            foreach (var item in Settings.Default.StaticMods)
            {
                string[] staticMods = item.Split(',');
                string paramName;
                String aaName;
                if (!CometParamsMap.GetStaticModParamInfo(staticMods[1], out paramName, out aaName))
                {
                    return false;
                }

                double massDiff;
                try
                {
                    massDiff = Convert.ToDouble(staticMods[2]);
                }
                catch(Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid mass difference value for " + paramName + ".";
                    return false;
                }

                if (!searchMgr.SetParam(paramName, massDiff.ToString(CultureInfo.InvariantCulture), massDiff))
                {
                    SearchStatusMessage = "Could not set the " + paramName + " parameter.";
                    return false;
                }
            }

            var cTermPeptideMass = Settings.Default.StaticModCTermPeptide;
            if (!searchMgr.SetParam("add_Cterm_peptide", cTermPeptideMass.ToString(CultureInfo.InvariantCulture), cTermPeptideMass))
            {
                SearchStatusMessage = "Could not set the add_Cterm_peptide parameter.";
                return false;
            }

            var nTermPeptideMass = Settings.Default.StaticModNTermPeptide;
            if (!searchMgr.SetParam("add_Nterm_peptide", nTermPeptideMass.ToString(CultureInfo.InvariantCulture), nTermPeptideMass))
            {
                SearchStatusMessage = "Could not set the add_Nterm_peptide parameter.";
                return false;
            }

            var cTermProteinMass = Settings.Default.StaticModCTermProtein;
            if (!searchMgr.SetParam("add_Cterm_protein", cTermProteinMass.ToString(CultureInfo.InvariantCulture), cTermProteinMass))
            {
                SearchStatusMessage = "Could not set the add_Cterm_protein parameter.";
                return false;
            }

            var nTermProteinMass = Settings.Default.StaticModNTermProtein;
            if (!searchMgr.SetParam("add_Nterm_protein", nTermProteinMass.ToString(CultureInfo.InvariantCulture), nTermProteinMass))
            {
                SearchStatusMessage = "Could not set the add_Nterm_protein parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureVariableModSettings(CometSearchManagerWrapper searchMgr)
        {
            int modNum = 0;
            foreach (var item in Settings.Default.VariableMods)
            {
                modNum++;
                string paramName = "variable_mod" + modNum;
                string[] varMods = item.Split(',');
                var varModsWrapper = new VarModsWrapper();
                varModsWrapper.set_VarModChar(varMods[0]);

                try
                {
                    varModsWrapper.set_VarModMass(Convert.ToDouble(varMods[1]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Mass Diff value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_BinaryMod(Convert.ToInt32(varMods[2]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Binary Mod value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_MaxNumVarModAAPerMod(Convert.ToInt32(varMods[3]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Max Mods value for " + paramName + ".";
                    return false;
                }

                if (!searchMgr.SetParam(paramName, item, varModsWrapper))
                {
                    SearchStatusMessage = "Could not set the " + paramName + " parameter.";
                    return false;
                }
            }

            var varCTerminus = Settings.Default.VariableCTerminus;
            if (!searchMgr.SetParam("variable_C_terminus", varCTerminus.ToString(CultureInfo.InvariantCulture), varCTerminus))
            {
                SearchStatusMessage = "Could not set the variable_C_terminus parameter.";
                return false;
            }

            var varCTerminusDist = Settings.Default.VariableCTermDistance;
            if (!searchMgr.SetParam("variable_C_terminus_distance", varCTerminusDist.ToString(CultureInfo.InvariantCulture), varCTerminusDist))
            {
                SearchStatusMessage = "Could not set the variable_C_terminus_distance parameter.";
                return false;
            }

            var varNTerminus = Settings.Default.VariableNTerminus;
            if (!searchMgr.SetParam("variable_N_terminus", varNTerminus.ToString(CultureInfo.InvariantCulture), varNTerminus))
            {
                SearchStatusMessage = "Could not set the variable_N_terminus parameter.";
                return false;
            }

            var varNTerminusDist = Settings.Default.VariableNTermDistance;
            if (!searchMgr.SetParam("variable_N_terminus_distance", varNTerminusDist.ToString(CultureInfo.InvariantCulture), varNTerminusDist))
            {
                SearchStatusMessage = "Could not set the variable_N_terminus_distance parameter.";
                return false;
            }

            var maxVarModsInPeptide = Settings.Default.MaxVarModsInPeptide;
            if (!searchMgr.SetParam("max_variable_mods_in_peptide", maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture), maxVarModsInPeptide))
            {
                SearchStatusMessage = "Could not set the max_variable_mods_in_peptide parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureMiscSettings(CometSearchManagerWrapper searchMgr)
        {
            // Set the mzXML-specific miscellaneous settings
            var mzxmlScanRangeMin = Settings.Default.mzxmlScanRangeMin;
            var mzxmlScanRangeMax = Settings.Default.mzxmlScanRangeMax;
            var mzxmlScanRange = new IntRangeWrapper(mzxmlScanRangeMin, mzxmlScanRangeMax);
            string mzxmlScanRangeString = mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            if (!searchMgr.SetParam("scan_range", mzxmlScanRangeString, mzxmlScanRange))
            {
                SearchStatusMessage = "Could not set the scan_range parameter.";
                return false;
            }

            var mzxmlPrecursorChargeMin = Settings.Default.mzxmlPrecursorChargeRangeMin;
            var mzxmlPrecursorChargeMax = Settings.Default.mzxmlPrecursorChargeRangeMax;
            var mzxmlPrecursorChargeRange = new IntRangeWrapper(mzxmlPrecursorChargeMin, mzxmlPrecursorChargeMax);
            string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeMin.ToString(CultureInfo.InvariantCulture)
                                                    + " " + mzxmlPrecursorChargeMax.ToString(CultureInfo.InvariantCulture);
            if (!searchMgr.SetParam("precursor_charge", mzxmlPrecursorChargeRageString, mzxmlPrecursorChargeRange))
            {
                SearchStatusMessage = "Could not set the precursor_charge parameter.";
                return false;
            }

            var mzxmlMSLevel = Settings.Default.mzxmlMsLevel;
            if (!searchMgr.SetParam("ms_level", mzxmlMSLevel.ToString(CultureInfo.InvariantCulture), mzxmlMSLevel))
            {
                SearchStatusMessage = "Could not set the ms_level parameter.";
                return false;
            }

            var mzxmlActivationMethod = Settings.Default.mzxmlActivationMethod;
            if (!searchMgr.SetParam("activation_method", mzxmlActivationMethod, mzxmlActivationMethod))
            {
                SearchStatusMessage = "Could not set the activation_method parameter.";
                return false;
            }

            // Set the spectral processing-specific miscellaneous settings
            var minPeaks = Settings.Default.spectralProcessingMinPeaks;
            if (!searchMgr.SetParam("minimum_peaks", minPeaks.ToString(CultureInfo.InvariantCulture), minPeaks))
            {
                SearchStatusMessage = "Could not set the minimum_peaks parameter.";
                return false;
            }

            var minIntensity = Settings.Default.spectralProcessingMinIntensity;
            if (!searchMgr.SetParam("minimum_intensity", minIntensity.ToString(CultureInfo.InvariantCulture), minIntensity))
            {
                SearchStatusMessage = "Could not set the minimum_intensity parameter.";
                return false;
            }

            var removePrecursorTol = Settings.Default.spectralProcessingRemovePrecursorTol;
            if (!searchMgr.SetParam("remove_precursor_tolerance", removePrecursorTol.ToString(CultureInfo.InvariantCulture), removePrecursorTol))
            {
                SearchStatusMessage = "Could not set the remove_precursor_tolerance parameter.";
                return false;
            }

            var removePrecursorPeak = Settings.Default.spectralProcessingRemovePrecursorPeak;
            if (!searchMgr.SetParam("remove_precursor_peak", removePrecursorPeak.ToString(CultureInfo.InvariantCulture), removePrecursorPeak))
            {
                SearchStatusMessage = "Could not set the remove_precursor_peak parameter.";
                return false;
            }

            var clearMzMin = Settings.Default.spectralProcessingClearMzMin;
            var clearMzMax = Settings.Default.spectralProcessingClearMzMax;
            var clearMzRange = new DoubleRangeWrapper(clearMzMin, clearMzMax);
            string clearMzRangeString = clearMzMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + clearMzMax.ToString(CultureInfo.InvariantCulture);
            if (!searchMgr.SetParam("clear_mz_range", clearMzRangeString, clearMzRange))
            {
                SearchStatusMessage = "Could not set the clear_mz_range parameter.";
                return false;
            }

            // Configure the rest of the miscellaneous parameters
            var spectrumBatchSize = Settings.Default.SpectrumBatchSize;
            if (!searchMgr.SetParam("spectrum_batch_size", spectrumBatchSize.ToString(CultureInfo.InvariantCulture), spectrumBatchSize))
            {
                SearchStatusMessage = "Could not set the spectrum_batch_size parameter.";
                return false;
            }

            var numThreads = Settings.Default.NumThreads;
            if (!searchMgr.SetParam("num_threads", numThreads.ToString(CultureInfo.InvariantCulture), numThreads))
            {
                SearchStatusMessage = "Could not set the num_threads parameter.";
                return false;
            }

            var numResults = Settings.Default.NumResults;
            if (!searchMgr.SetParam("num_results", numResults.ToString(CultureInfo.InvariantCulture), numResults))
            {
                SearchStatusMessage = "Could not set the num_results parameter.";
                return false;
            }

            var maxFragmentCharge = Settings.Default.MaxFragmentCharge;
            if (!searchMgr.SetParam("max_fragment_charge", maxFragmentCharge.ToString(CultureInfo.InvariantCulture), maxFragmentCharge))
            {
                SearchStatusMessage = "Could not set the max_fragment_charge parameter.";
                return false;
            }

            var maxPrecursorCharge = Settings.Default.MaxPrecursorCharge;
            if (!searchMgr.SetParam("max_precursor_charge", maxPrecursorCharge.ToString(CultureInfo.InvariantCulture), maxPrecursorCharge))
            {
                SearchStatusMessage = "Could not set the max_precursor_charge parameter.";
                return false;
            }

            var clipNTermMethionine = Settings.Default.ClipNTermMethionine ? 1 : 0;
            if (!searchMgr.SetParam("clip_nterm_methionine", clipNTermMethionine.ToString(CultureInfo.InvariantCulture), clipNTermMethionine))
            {
                SearchStatusMessage = "Could not set the clip_nterm_methionine parameter.";
                return false;
            }

            return true;
        }

        public String SearchStatusMessage { get; private set; }
        public bool SearchSucceeded { get; private set; }

        private string DatabaseFile
        {
            get { return proteomeDbFileCombo.Text; }

            set { proteomeDbFileCombo.Text = value; }
        }

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            string databaseFile = InputSettingsControl.ShowOpenDatabaseFile();
            if (null != databaseFile)
            {
                if (!String.Equals(DatabaseFile, databaseFile))
                {
                    DatabaseFile = databaseFile;
                    ProteomeDbFileNameChanged();
                }
            }
        }

        private void ProteomeDbFileComboTextUpdate(object sender, EventArgs e)
        {
            ProteomeDbFileNameChanged();
        } 

        private void ProteomeDbFileNameChanged()
        {
            if (File.Exists(proteomeDbFileCombo.Text) && !String.Equals(proteomeDbFileCombo.Text, Settings.Default.ProteomeDatabaseFile))
            {
                if (DialogResult.OK == MessageBox.Show(Resources.RunSearchDlg_ProteomeDbFileNameChanged_Would_you_like_to_update_the_proteome_database_file_name_in_the_search_settings_with_the_one_you_have_specified_here_, Resources.RunSearchDlg_ProteomeDbFileNameChanged_Run_Search, MessageBoxButtons.OKCancel, MessageBoxIcon.Question))
                {
                    Settings.Default.ProteomeDatabaseFile = proteomeDbFileCombo.Text;
                }
            }

            if (-1 == proteomeDbFileCombo.FindStringExact(proteomeDbFileCombo.Text))
            {
                proteomeDbFileCombo.Items.Add(proteomeDbFileCombo.Text);
            }
        }

        private void ProteomeDbFileComboSelectedIndexChanged(object sender, EventArgs e)
        {
            ProteomeDbFileNameChanged();
        }
    }
}
