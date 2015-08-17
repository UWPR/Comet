/*
   Copyright 2015 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Globalization;
using System.IO;
using System.Windows.Forms;
using CometUI.Properties;
using CometWrapper;

namespace CometUI.Search
{
    public class CometSearch
    {
        private CometSearchManagerWrapper SearchMgr { get; set; }
        private Properties.SearchSettings SearchSettings { get { return CometUIMainForm.SearchSettings; } }
        private CometUIMainForm Parent { get; set; }
        private string[] InputFiles { get; set; }
        private String DatabaseFileName { get; set; }

        public String SearchStatusMessage { get; private set; }
        public bool SearchSucceeded { get; private set; }

        public CometSearch(String[] inputFiles, String dbFile, CometUIMainForm parent)
        {
            InputFiles = inputFiles;
            DatabaseFileName = dbFile;
            SearchMgr = new CometSearchManagerWrapper();
            Parent = parent;
        }

        public bool RunSearch()
        {
            // Begin with the assumption that the search is going to fail,
            // and only set it to succeeded at the end if run completes
            // successfully
            SearchSucceeded = false;

            if (!ConfigureInputFiles())
            {
                return false;
            }

            if (!ConfigureInputSettings())
            {
                return false;
            }

            if (!ConfigureOutputSettings())
            {
                return false;
            }

            if (!ConfigureEnzymeSettings())
            {
                return false;
            }

            if (!ConfigureMassSettings())
            {
                return false;
            }

            if (!ConfigureStaticModSettings())
            {
                return false;
            }

            if (!ConfigureVariableModSettings())
            {
                return false;
            }

            if (!ConfigureMiscSettings())
            {
                return false;
            }

            if (!SearchMgr.DoSearch())
            {
                string errorMessage = null;
                if (SearchMgr.GetStatusMessage(ref errorMessage))
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

        public bool GetStatusMessage(ref String statusMsg)
        {
            if (!SearchMgr.GetStatusMessage(ref statusMsg))
            {
                return false;
            }

            return true;
        }

        public bool IsCancel()
        {
            bool bCancel = false;
            if (!SearchMgr.IsCancelSearch(ref bCancel))
            {
                return false;
            }

            return bCancel;
        }

        public bool CancelSearch()
        {
            return SearchMgr.CancelSearch();
        }

        public void ResetCancel()
        {
            SearchMgr.ResetSearchStatus();
        }

        public void ViewResults()
        {
            if (SearchSettings.OutputFormatPepXML)
            {
                var fileName = InputFiles[0];
                int fileExtPos = fileName.LastIndexOf(".", StringComparison.Ordinal);
                if (fileExtPos >= 0)
                {
                    fileName = fileName.Substring(0, fileExtPos);
                }
                String outputPepXML = fileName + ".pep.xml";
                Parent.UpdateViewSearchResults(outputPepXML, SearchSettings.DecoyPrefix);
            }
            else
            {
                MessageBox.Show(Resources.CometSearch_ViewResults_No_pep_xml_file_found_to_display_in_the_viewer_, Resources.CometSearch_ViewResults_View_Results, MessageBoxButtons.OK,
                MessageBoxIcon.Warning);
            }
        }

        private bool ConfigureInputFiles()
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
                    inputFileInfo.set_FirstScan(SearchSettings.mzxmlScanRangeMin);
                    inputFileInfo.set_LastScan(SearchSettings.mzxmlScanRangeMax);
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

            if (!SearchMgr.AddInputFiles(inputFiles))
            {
                SearchStatusMessage = "Could not add input files.";
                return false;
            }

            return true;
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

        private bool ConfigureInputSettings()
        {
            // Set up the proteome database
            var dbFileName = DatabaseFileName;
            if (!SearchMgr.SetParam("database_name", dbFileName, dbFileName))
            {
                SearchStatusMessage = "Could not set the database_name parameter.";
                return false;
            }

            // Set up the target vs. decoy parameters
            var searchType = SearchSettings.SearchType;
            if (!SearchMgr.SetParam("decoy_search", searchType.ToString(CultureInfo.InvariantCulture), searchType))
            {
                SearchStatusMessage = "Could not set the decoy_search parameter.";
                return false;
            }

            var decoyPrefix = SearchSettings.DecoyPrefix;
            if (!SearchMgr.SetParam("decoy_prefix", decoyPrefix, decoyPrefix))
            {
                SearchStatusMessage = "Could not set the decoy_prefix parameter.";
                return false;
            }

            var nucleotideReadingFrame = SearchSettings.NucleotideReadingFrame;
            if (!SearchMgr.SetParam("nucleotide_reading_frame", nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture), nucleotideReadingFrame))
            {
                SearchStatusMessage = "Could not set the nucleotide_reading_frame parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureOutputSettings()
        {
            var outputPepXMLFile = SearchSettings.OutputFormatPepXML ? 1 : 0;
            if (!SearchMgr.SetParam("output_pepxmlfile", outputPepXMLFile.ToString(CultureInfo.InvariantCulture), outputPepXMLFile))
            {
                SearchStatusMessage = "Could not set the output_pepxmlfile parameter.";
                return false;
            }

            var outputPercolatorFile = SearchSettings.OutputFormatPercolator ? 1 : 0;
            if (!SearchMgr.SetParam("output_percolatorfile", outputPercolatorFile.ToString(CultureInfo.InvariantCulture), outputPercolatorFile))
            {
                SearchStatusMessage = "Could not set the output_percolatorfile parameter.";
                return false;
            }

            var outputTextFile = SearchSettings.OutputFormatTextFile ? 1 : 0;
            if (!SearchMgr.SetParam("output_txtfile", outputTextFile.ToString(CultureInfo.InvariantCulture), outputTextFile))
            {
                SearchStatusMessage = "Could not set the output_txtfile parameter.";
                return false;
            }

            var outputSqtToStdout = SearchSettings.OutputFormatSqtToStandardOutput ? 1 : 0;
            if (!SearchMgr.SetParam("output_sqtstream", outputSqtToStdout.ToString(CultureInfo.InvariantCulture), outputSqtToStdout))
            {
                SearchStatusMessage = "Could not set the output_sqtstream parameter.";
                return false;
            }

            var outputSqtFile = SearchSettings.OutputFormatSqtFile ? 1 : 0;
            if (!SearchMgr.SetParam("output_sqtfile", outputSqtFile.ToString(CultureInfo.InvariantCulture), outputSqtFile))
            {
                SearchStatusMessage = "Could not set the output_sqtfile parameter.";
                return false;
            }

            var outputOutFile = SearchSettings.OutputFormatOutFiles ? 1 : 0;
            if (!SearchMgr.SetParam("output_outfiles", outputOutFile.ToString(CultureInfo.InvariantCulture), outputOutFile))
            {
                SearchStatusMessage = "Could not set the output_outfiles parameter.";
                return false;
            }

            var printExpectScore = SearchSettings.PrintExpectScoreInPlaceOfSP ? 1 : 0;
            if (!SearchMgr.SetParam("print_expect_score", printExpectScore.ToString(CultureInfo.InvariantCulture), printExpectScore))
            {
                SearchStatusMessage = "Could not set the print_expect_score parameter.";
                return false;
            }

            var showFragmentIons = SearchSettings.OutputFormatShowFragmentIons ? 1 : 0;
            if (!SearchMgr.SetParam("show_fragment_ions", showFragmentIons.ToString(CultureInfo.InvariantCulture), showFragmentIons))
            {
                SearchStatusMessage = "Could not set the show_fragment_ions parameter.";
                return false;
            }

            var skipResearching = SearchSettings.OutputFormatSkipReSearching ? 1 : 0;
            if (!SearchMgr.SetParam("skip_researching", skipResearching.ToString(CultureInfo.InvariantCulture), skipResearching))
            {
                SearchStatusMessage = "Could not set the skip_researching parameter.";
                return false;
            }

            var numOutputLines = SearchSettings.NumOutputLines;
            if (!SearchMgr.SetParam("num_output_lines", numOutputLines.ToString(CultureInfo.InvariantCulture), numOutputLines))
            {
                SearchStatusMessage = "Could not set the num_output_lines parameter.";
                return false;
            }

            var outputSuffix = SearchSettings.OutputSuffix;
            if (!SearchMgr.SetParam("output_suffix", outputSuffix, outputSuffix))
            {
                SearchStatusMessage = "Could not set the output_suffix parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureEnzymeSettings()
        {
            var searchEnzymeNumber = SearchSettings.SearchEnzymeNumber;
            if (!SearchMgr.SetParam("search_enzyme_number", searchEnzymeNumber.ToString(CultureInfo.InvariantCulture), searchEnzymeNumber))
            {
                SearchStatusMessage = "Could not set the search_enzyme_number parameter.";
                return false;
            }

            var sampleEnzymeNumber = SearchSettings.SampleEnzymeNumber;
            if (!SearchMgr.SetParam("sample_enzyme_number", sampleEnzymeNumber.ToString(CultureInfo.InvariantCulture), sampleEnzymeNumber))
            {
                SearchStatusMessage = "Could not set the sample_enzyme_number parameter.";
                return false;
            }

            if (!SetEnzymeInfo(searchEnzymeNumber, sampleEnzymeNumber))
            {
                return false;
            }

            var allowedMissedCleavages = SearchSettings.AllowedMissedCleavages;
            if (!SearchMgr.SetParam("allowed_missed_cleavage", allowedMissedCleavages.ToString(CultureInfo.InvariantCulture), allowedMissedCleavages))
            {
                SearchStatusMessage = "Could not set the allowed_missed_cleavage parameter.";
                return false;
            }

            var enzymeTermini = SearchSettings.EnzymeTermini;
            if (!SearchMgr.SetParam("num_enzyme_termini", enzymeTermini.ToString(CultureInfo.InvariantCulture), enzymeTermini))
            {
                SearchStatusMessage = "Could not set the num_enzyme_termini parameter.";
                return false;
            }

            var digestMassMin = SearchSettings.digestMassRangeMin;
            var digestMassMax = SearchSettings.digestMassRangeMax;
            var digestMassRange = new DoubleRangeWrapper(digestMassMin, digestMassMax);
            string digestMassRangeString = digestMassMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + digestMassMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("digest_mass_range", digestMassRangeString, digestMassRange))
            {
                SearchStatusMessage = "Could not set the digest_mass_range parameter.";
                return false;
            }


            return true;
        }

        private bool SetEnzymeInfo(int searchEnzymeNumber, int sampleEnzymeNumber)
        {
            const int enzymeInfoNumColumns = 5;
            const int enzymeNumberColumn = 0;
            const int enzymeNameColumn = 1;
            const int enzymeOffsetColumn = 2;
            const int enzymeBreakAAColumn = 3;
            const int enzymeNoBreakAAColumn = 4;

            StringCollection enzymeInfoStrCollection = SearchSettings.EnzymeInfo;
            var enzymeInfoLines = new String[enzymeInfoStrCollection.Count];
            enzymeInfoStrCollection.CopyTo(enzymeInfoLines, 0);
            var formattedEnzymeInfoStr = String.Empty;
            var enzymeInfo = new EnzymeInfoWrapper();

            foreach (var line in enzymeInfoLines)
            {
                if (!String.IsNullOrEmpty(line))
                {
                    String[] enzymeInfoColumns = line.Split(',');
                    if (enzymeInfoColumns.Length != enzymeInfoNumColumns)
                    {
                        SearchStatusMessage = "Invalid enzyme info in settings.";
                        return false;
                    }

                    int enzymeNumber = Convert.ToInt32(enzymeInfoColumns[enzymeNumberColumn]);
                    if (enzymeNumber == searchEnzymeNumber)
                    {
                        enzymeInfo.set_SearchEnzymeName(enzymeInfoColumns[enzymeNameColumn]);
                        enzymeInfo.set_SearchEnzymeBreakAA(enzymeInfoColumns[enzymeBreakAAColumn]);
                        enzymeInfo.set_SearchEnzymeNoBreakAA(enzymeInfoColumns[enzymeNoBreakAAColumn]);

                        int enzymeOffset = Convert.ToInt32(enzymeInfoColumns[enzymeOffsetColumn]);
                        enzymeInfo.set_SearchEnzymeOffSet(enzymeOffset);
                    }

                    if (enzymeNumber == sampleEnzymeNumber)
                    {
                        enzymeInfo.set_SampleEnzymeName(enzymeInfoColumns[enzymeNameColumn]);
                        enzymeInfo.set_SampleEnzymeBreakAA(enzymeInfoColumns[enzymeBreakAAColumn]);
                        enzymeInfo.set_SampleEnzymeNoBreakAA(enzymeInfoColumns[enzymeNoBreakAAColumn]);

                        int enzymeOffset = Convert.ToInt32(enzymeInfoColumns[enzymeOffsetColumn]);
                        enzymeInfo.set_SampleEnzymeOffSet(enzymeOffset);
                    }

                    String enzymeInfoFormattedRow = enzymeInfoColumns[enzymeNumberColumn] + ".";
                    for (int i = enzymeNameColumn; i < enzymeInfoColumns.Length; i++)
                    {
                        enzymeInfoFormattedRow += " " + enzymeInfoColumns[i];
                    }

                    formattedEnzymeInfoStr += enzymeInfoFormattedRow + Environment.NewLine;
                }
            }

            if (!SearchMgr.SetParam("[COMET_ENZYME_INFO]", formattedEnzymeInfoStr, enzymeInfo))
            {
                SearchStatusMessage = "Could not set the [COMET_ENZYME_INFO] parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureMassSettings()
        {
            // Configure precursor mass settings

            var precursorMassTol = SearchSettings.PrecursorMassTolerance;
            if (!SearchMgr.SetParam("peptide_mass_tolerance", precursorMassTol.ToString(CultureInfo.InvariantCulture), precursorMassTol))
            {
                SearchStatusMessage = "Could not set the peptide_mass_tolerance parameter.";
                return false;
            }

            var precursorMassUnit = SearchSettings.PrecursorMassUnit;
            if (!SearchMgr.SetParam("peptide_mass_units", precursorMassUnit.ToString(CultureInfo.InvariantCulture), precursorMassUnit))
            {
                SearchStatusMessage = "Could not set the peptide_mass_units parameter.";
                return false;
            }

            var precursorMassType = SearchSettings.PrecursorMassType;
            if (!SearchMgr.SetParam("mass_type_parent", precursorMassType.ToString(CultureInfo.InvariantCulture), precursorMassType))
            {
                SearchStatusMessage = "Could not set the mass_type_parent parameter.";
                return false;
            }

            var isotopeError = SearchSettings.PrecursorIsotopeError;
            if (!SearchMgr.SetParam("isotope_error", isotopeError.ToString(CultureInfo.InvariantCulture), isotopeError))
            {
                SearchStatusMessage = "Could not set the isotope_error parameter.";
                return false;
            }

            var toleranceType = SearchSettings.PrecursorToleranceType;
            if (!SearchMgr.SetParam("precursor_tolerance_type", toleranceType.ToString(CultureInfo.InvariantCulture), toleranceType))
            {
                SearchStatusMessage = "Could not set the precursor_tolerance_type parameter.";
                return false;
            }

            var massOffsetsStr = SearchSettings.PrecursorMassOffsets;
            if (!SearchMgr.SetParam("mass_offsets", massOffsetsStr, CometParamsMap.GetMassOffsetsListFromString(massOffsetsStr)))
            {
                SearchStatusMessage = "Could not set the mass_offsets parameter.";
                return false;
            }


            // Configure fragment mass settings

            var fragmentBinSize = SearchSettings.FragmentBinSize;
            if (!SearchMgr.SetParam("fragment_bin_tol", fragmentBinSize.ToString(CultureInfo.InvariantCulture), fragmentBinSize))
            {
                SearchStatusMessage = "Could not set the fragment_bin_tol parameter.";
                return false;
            }

            var fragmentBinOffset = SearchSettings.FragmentBinOffset;
            if (!SearchMgr.SetParam("fragment_bin_offset", fragmentBinOffset.ToString(CultureInfo.InvariantCulture), fragmentBinOffset))
            {
                SearchStatusMessage = "Could not set the fragment_bin_offset parameter.";
                return false;
            }

            var fragmentMassType = SearchSettings.FragmentMassType;
            if (!SearchMgr.SetParam("mass_type_fragment", fragmentMassType.ToString(CultureInfo.InvariantCulture), fragmentMassType))
            {
                SearchStatusMessage = "Could not set the mass_type_fragment parameter.";
                return false;
            }

            // Configure fragment ions

            var useAIons = SearchSettings.UseAIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_A_ions", useAIons.ToString(CultureInfo.InvariantCulture), useAIons))
            {
                SearchStatusMessage = "Could not set the use_A_ions parameter.";
                return false;
            }

            var useBIons = SearchSettings.UseBIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_B_ions", useBIons.ToString(CultureInfo.InvariantCulture), useBIons))
            {
                SearchStatusMessage = "Could not set the use_B_ions parameter.";
                return false;
            }

            var useCIons = SearchSettings.UseCIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_C_ions", useCIons.ToString(CultureInfo.InvariantCulture), useCIons))
            {
                SearchStatusMessage = "Could not set the use_C_ions parameter.";
                return false;
            }

            var useXIons = SearchSettings.UseXIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_X_ions", useXIons.ToString(CultureInfo.InvariantCulture), useXIons))
            {
                SearchStatusMessage = "Could not set the use_X_ions parameter.";
                return false;
            }

            var useYIons = SearchSettings.UseYIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_Y_ions", useYIons.ToString(CultureInfo.InvariantCulture), useYIons))
            {
                SearchStatusMessage = "Could not set the use_Y_ions parameter.";
                return false;
            }

            var useZIons = SearchSettings.UseZIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_Z_ions", useZIons.ToString(CultureInfo.InvariantCulture), useZIons))
            {
                SearchStatusMessage = "Could not set the use_Z_ions parameter.";
                return false;
            }

            var useFlankIons = SearchSettings.TheoreticalFragmentIons ? 1 : 0;
            if (!SearchMgr.SetParam("theoretical_fragment_ions", useFlankIons.ToString(CultureInfo.InvariantCulture), useFlankIons))
            {
                SearchStatusMessage = "Could not set the theoretical_fragment_ions parameter.";
                return false;
            }

            var useNLIons = SearchSettings.UseNLIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_NL_ions", useNLIons.ToString(CultureInfo.InvariantCulture), useNLIons))
            {
                SearchStatusMessage = "Could not set the use_NL_ions parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureStaticModSettings()
        {
            foreach (var item in SearchSettings.StaticMods)
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
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid mass difference value for " + paramName + ".";
                    return false;
                }

                if (!SearchMgr.SetParam(paramName, massDiff.ToString(CultureInfo.InvariantCulture), massDiff))
                {
                    SearchStatusMessage = "Could not set the " + paramName + " parameter.";
                    return false;
                }
            }

            var cTermPeptideMass = SearchSettings.StaticModCTermPeptide;
            if (!SearchMgr.SetParam("add_Cterm_peptide", cTermPeptideMass.ToString(CultureInfo.InvariantCulture), cTermPeptideMass))
            {
                SearchStatusMessage = "Could not set the add_Cterm_peptide parameter.";
                return false;
            }

            var nTermPeptideMass = SearchSettings.StaticModNTermPeptide;
            if (!SearchMgr.SetParam("add_Nterm_peptide", nTermPeptideMass.ToString(CultureInfo.InvariantCulture), nTermPeptideMass))
            {
                SearchStatusMessage = "Could not set the add_Nterm_peptide parameter.";
                return false;
            }

            var cTermProteinMass = SearchSettings.StaticModCTermProtein;
            if (!SearchMgr.SetParam("add_Cterm_protein", cTermProteinMass.ToString(CultureInfo.InvariantCulture), cTermProteinMass))
            {
                SearchStatusMessage = "Could not set the add_Cterm_protein parameter.";
                return false;
            }

            var nTermProteinMass = SearchSettings.StaticModNTermProtein;
            if (!SearchMgr.SetParam("add_Nterm_protein", nTermProteinMass.ToString(CultureInfo.InvariantCulture), nTermProteinMass))
            {
                SearchStatusMessage = "Could not set the add_Nterm_protein parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureVariableModSettings()
        {
            int modNum = 0;
            foreach (var item in SearchSettings.VariableMods)
            {
                modNum++;
                string paramName = "variable_mod0" + modNum;
                string[] varMods = item.Split(',');
                var varModsWrapper = new VarModsWrapper();
                varModsWrapper.set_VarModChar(varMods[CometParamsMap.VarModsColResidue]);
                try
                {
                    varModsWrapper.set_VarModMass(Convert.ToDouble(varMods[CometParamsMap.VarModsColMassDiff]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Mass Diff value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_BinaryMod(Convert.ToInt32(varMods[CometParamsMap.VarModsColBinaryMod]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Bin Mod value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_MaxNumVarModAAPerMod(Convert.ToInt32(varMods[CometParamsMap.VarModsColMaxMods]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Max Mods value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_VarModTermDistance(Convert.ToInt32(varMods[CometParamsMap.VarModsColTermDist]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Term Dist value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_WhichTerm(Convert.ToInt32(varMods[CometParamsMap.VarModsColWhichTerm]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Which Term value for " + paramName + ".";
                    return false;
                }

                try
                {
                    varModsWrapper.set_RequireThisMod(Convert.ToInt32(varMods[CometParamsMap.VarModsColRequireThisMod]));
                }
                catch (Exception e)
                {
                    SearchStatusMessage = e.Message + "\nInvalid Require This Mod value for " + paramName + ".";
                    return false;
                }

                if (!SearchMgr.SetParam(paramName, item, varModsWrapper))
                {
                    SearchStatusMessage = "Could not set the " + paramName + " parameter.";
                    return false;
                }
            }

            var maxVarModsInPeptide = SearchSettings.MaxVarModsInPeptide;
            if (!SearchMgr.SetParam("max_variable_mods_in_peptide", maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture), maxVarModsInPeptide))
            {
                SearchStatusMessage = "Could not set the max_variable_mods_in_peptide parameter.";
                return false;
            }

            int requireVarMods = SearchSettings.RequireVariableMod ? 1 : 0;
            if (!SearchMgr.SetParam("require_variable_mod", requireVarMods.ToString(CultureInfo.InvariantCulture), requireVarMods))
            {
                SearchStatusMessage = "Could not set the require_variable_mod parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureMiscSettings()
        {
            // Set the mzXML-specific miscellaneous settings
            var mzxmlScanRangeMin = SearchSettings.mzxmlScanRangeMin;
            var mzxmlScanRangeMax = SearchSettings.mzxmlScanRangeMax;
            var mzxmlScanRange = new IntRangeWrapper(mzxmlScanRangeMin, mzxmlScanRangeMax);
            string mzxmlScanRangeString = mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("scan_range", mzxmlScanRangeString, mzxmlScanRange))
            {
                SearchStatusMessage = "Could not set the scan_range parameter.";
                return false;
            }

            var mzxmlPrecursorChargeMin = SearchSettings.mzxmlPrecursorChargeRangeMin;
            var mzxmlPrecursorChargeMax = SearchSettings.mzxmlPrecursorChargeRangeMax;
            var mzxmlPrecursorChargeRange = new IntRangeWrapper(mzxmlPrecursorChargeMin, mzxmlPrecursorChargeMax);
            string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeMin.ToString(CultureInfo.InvariantCulture)
                                                    + " " + mzxmlPrecursorChargeMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("precursor_charge", mzxmlPrecursorChargeRageString, mzxmlPrecursorChargeRange))
            {
                SearchStatusMessage = "Could not set the precursor_charge parameter.";
                return false;
            }

            var mzxmlOverrideCharge = SearchSettings.mzxmlOverrideCharge;
            if (!SearchMgr.SetParam("override_charge", mzxmlOverrideCharge.ToString(CultureInfo.InvariantCulture), mzxmlOverrideCharge))
            {
                SearchStatusMessage = "Could not set the override_charge parameter.";
                return false;
            }

            var mzxmlMSLevel = SearchSettings.mzxmlMsLevel;
            if (!SearchMgr.SetParam("ms_level", mzxmlMSLevel.ToString(CultureInfo.InvariantCulture), mzxmlMSLevel))
            {
                SearchStatusMessage = "Could not set the ms_level parameter.";
                return false;
            }

            var mzxmlActivationMethod = SearchSettings.mzxmlActivationMethod;
            if (!SearchMgr.SetParam("activation_method", mzxmlActivationMethod, mzxmlActivationMethod))
            {
                SearchStatusMessage = "Could not set the activation_method parameter.";
                return false;
            }

            // Set the spectral processing-specific miscellaneous settings
            var minPeaks = SearchSettings.spectralProcessingMinPeaks;
            if (!SearchMgr.SetParam("minimum_peaks", minPeaks.ToString(CultureInfo.InvariantCulture), minPeaks))
            {
                SearchStatusMessage = "Could not set the minimum_peaks parameter.";
                return false;
            }

            var minIntensity = SearchSettings.spectralProcessingMinIntensity;
            if (!SearchMgr.SetParam("minimum_intensity", minIntensity.ToString(CultureInfo.InvariantCulture), minIntensity))
            {
                SearchStatusMessage = "Could not set the minimum_intensity parameter.";
                return false;
            }

            var removePrecursorTol = SearchSettings.spectralProcessingRemovePrecursorTol;
            if (!SearchMgr.SetParam("remove_precursor_tolerance", removePrecursorTol.ToString(CultureInfo.InvariantCulture), removePrecursorTol))
            {
                SearchStatusMessage = "Could not set the remove_precursor_tolerance parameter.";
                return false;
            }

            var removePrecursorPeak = SearchSettings.spectralProcessingRemovePrecursorPeak;
            if (!SearchMgr.SetParam("remove_precursor_peak", removePrecursorPeak.ToString(CultureInfo.InvariantCulture), removePrecursorPeak))
            {
                SearchStatusMessage = "Could not set the remove_precursor_peak parameter.";
                return false;
            }

            var clearMzMin = SearchSettings.spectralProcessingClearMzMin;
            var clearMzMax = SearchSettings.spectralProcessingClearMzMax;
            var clearMzRange = new DoubleRangeWrapper(clearMzMin, clearMzMax);
            string clearMzRangeString = clearMzMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + clearMzMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("clear_mz_range", clearMzRangeString, clearMzRange))
            {
                SearchStatusMessage = "Could not set the clear_mz_range parameter.";
                return false;
            }

            // Configure the rest of the miscellaneous parameters
            var spectrumBatchSize = SearchSettings.SpectrumBatchSize;
            if (!SearchMgr.SetParam("spectrum_batch_size", spectrumBatchSize.ToString(CultureInfo.InvariantCulture), spectrumBatchSize))
            {
                SearchStatusMessage = "Could not set the spectrum_batch_size parameter.";
                return false;
            }

            var numThreads = SearchSettings.NumThreads;
            if (!SearchMgr.SetParam("num_threads", numThreads.ToString(CultureInfo.InvariantCulture), numThreads))
            {
                SearchStatusMessage = "Could not set the num_threads parameter.";
                return false;
            }

            var numResults = SearchSettings.NumResults;
            if (!SearchMgr.SetParam("num_results", numResults.ToString(CultureInfo.InvariantCulture), numResults))
            {
                SearchStatusMessage = "Could not set the num_results parameter.";
                return false;
            }

            var maxFragmentCharge = SearchSettings.MaxFragmentCharge;
            if (!SearchMgr.SetParam("max_fragment_charge", maxFragmentCharge.ToString(CultureInfo.InvariantCulture), maxFragmentCharge))
            {
                SearchStatusMessage = "Could not set the max_fragment_charge parameter.";
                return false;
            }

            var maxPrecursorCharge = SearchSettings.MaxPrecursorCharge;
            if (!SearchMgr.SetParam("max_precursor_charge", maxPrecursorCharge.ToString(CultureInfo.InvariantCulture), maxPrecursorCharge))
            {
                SearchStatusMessage = "Could not set the max_precursor_charge parameter.";
                return false;
            }

            var clipNTermMethionine = SearchSettings.ClipNTermMethionine ? 1 : 0;
            if (!SearchMgr.SetParam("clip_nterm_methionine", clipNTermMethionine.ToString(CultureInfo.InvariantCulture), clipNTermMethionine))
            {
                SearchStatusMessage = "Could not set the clip_nterm_methionine parameter.";
                return false;
            }

            return true;
        }
    }
}
