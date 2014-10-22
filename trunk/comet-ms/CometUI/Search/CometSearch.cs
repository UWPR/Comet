using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using CometWrapper;

namespace CometUI.RunSearch
{
    public class CometSearch
    {
        private CometSearchManagerWrapper SearchMgr { get; set; }
        private string[] InputFiles { get; set; }

        public String SearchStatusMessage { get; private set; }
        public bool SearchSucceeded { get; private set; }

        public CometSearch(string[] inputFiles)
        {
            InputFiles = inputFiles;
            SearchMgr = new CometSearchManagerWrapper();
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
                    inputFileInfo.set_FirstScan(CometUI.SearchSettings.mzxmlScanRangeMin);
                    inputFileInfo.set_LastScan(CometUI.SearchSettings.mzxmlScanRangeMax);
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
            var dbFileName = CometUI.SearchSettings.ProteomeDatabaseFile;
            if (!SearchMgr.SetParam("database_name", dbFileName, dbFileName))
            {
                SearchStatusMessage = "Could not set the database_name parameter.";
                return false;
            }

            // Set up the target vs. decoy parameters
            var searchType = CometUI.SearchSettings.SearchType;
            if (!SearchMgr.SetParam("decoy_search", searchType.ToString(CultureInfo.InvariantCulture), searchType))
            {
                SearchStatusMessage = "Could not set the decoy_search parameter.";
                return false;
            }

            var decoyPrefix = CometUI.SearchSettings.DecoyPrefix;
            if (!SearchMgr.SetParam("decoy_prefix", decoyPrefix, decoyPrefix))
            {
                SearchStatusMessage = "Could not set the decoy_prefix parameter.";
                return false;
            }

            var nucleotideReadingFrame = CometUI.SearchSettings.NucleotideReadingFrame;
            if (!SearchMgr.SetParam("nucleotide_reading_frame", nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture), nucleotideReadingFrame))
            {
                SearchStatusMessage = "Could not set the nucleotide_reading_frame parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureOutputSettings()
        {
            var outputPepXMLFile = CometUI.SearchSettings.OutputFormatPepXML ? 1 : 0;
            if (!SearchMgr.SetParam("output_pepxmlfile", outputPepXMLFile.ToString(CultureInfo.InvariantCulture), outputPepXMLFile))
            {
                SearchStatusMessage = "Could not set the output_pepxmlfile parameter.";
                return false;
            }

            var outputPercolatorFile = CometUI.SearchSettings.OutputFormatPercolator ? 1 : 0;
            if (!SearchMgr.SetParam("output_percolatorfile", outputPercolatorFile.ToString(CultureInfo.InvariantCulture), outputPercolatorFile))
            {
                SearchStatusMessage = "Could not set the output_percolatorfile parameter.";
                return false;
            }

            var outputTextFile = CometUI.SearchSettings.OutputFormatTextFile ? 1 : 0;
            if (!SearchMgr.SetParam("output_txtfile", outputTextFile.ToString(CultureInfo.InvariantCulture), outputTextFile))
            {
                SearchStatusMessage = "Could not set the output_txtfile parameter.";
                return false;
            }

            var outputSqtToStdout = CometUI.SearchSettings.OutputFormatSqtToStandardOutput ? 1 : 0;
            if (!SearchMgr.SetParam("output_sqtstream", outputSqtToStdout.ToString(CultureInfo.InvariantCulture), outputSqtToStdout))
            {
                SearchStatusMessage = "Could not set the output_sqtstream parameter.";
                return false;
            }

            var outputSqtFile = CometUI.SearchSettings.OutputFormatSqtFile ? 1 : 0;
            if (!SearchMgr.SetParam("output_sqtfile", outputSqtFile.ToString(CultureInfo.InvariantCulture), outputSqtFile))
            {
                SearchStatusMessage = "Could not set the output_sqtfile parameter.";
                return false;
            }

            var outputOutFile = CometUI.SearchSettings.OutputFormatOutFiles ? 1 : 0;
            if (!SearchMgr.SetParam("output_outfiles", outputOutFile.ToString(CultureInfo.InvariantCulture), outputOutFile))
            {
                SearchStatusMessage = "Could not set the output_outfiles parameter.";
                return false;
            }

            var printExpectScore = CometUI.SearchSettings.PrintExpectScoreInPlaceOfSP ? 1 : 0;
            if (!SearchMgr.SetParam("print_expect_score", printExpectScore.ToString(CultureInfo.InvariantCulture), printExpectScore))
            {
                SearchStatusMessage = "Could not set the print_expect_score parameter.";
                return false;
            }

            var showFragmentIons = CometUI.SearchSettings.OutputFormatShowFragmentIons ? 1 : 0;
            if (!SearchMgr.SetParam("show_fragment_ions", showFragmentIons.ToString(CultureInfo.InvariantCulture), showFragmentIons))
            {
                SearchStatusMessage = "Could not set the show_fragment_ions parameter.";
                return false;
            }

            var skipResearching = CometUI.SearchSettings.OutputFormatSkipReSearching ? 1 : 0;
            if (!SearchMgr.SetParam("skip_researching", skipResearching.ToString(CultureInfo.InvariantCulture), skipResearching))
            {
                SearchStatusMessage = "Could not set the skip_researching parameter.";
                return false;
            }

            var numOutputLines = CometUI.SearchSettings.NumOutputLines;
            if (!SearchMgr.SetParam("num_output_lines", numOutputLines.ToString(CultureInfo.InvariantCulture), numOutputLines))
            {
                SearchStatusMessage = "Could not set the num_output_lines parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureEnzymeSettings()
        {
            var searchEnzymeNumber = CometUI.SearchSettings.SearchEnzymeNumber;
            if (!SearchMgr.SetParam("search_enzyme_number", searchEnzymeNumber.ToString(CultureInfo.InvariantCulture), searchEnzymeNumber))
            {
                SearchStatusMessage = "Could not set the search_enzyme_number parameter.";
                return false;
            }

            var sampleEnzymeNumber = CometUI.SearchSettings.SampleEnzymeNumber;
            if (!SearchMgr.SetParam("sample_enzyme_number", sampleEnzymeNumber.ToString(CultureInfo.InvariantCulture), sampleEnzymeNumber))
            {
                SearchStatusMessage = "Could not set the sample_enzyme_number parameter.";
                return false;
            }

            var allowedMissedCleavages = CometUI.SearchSettings.AllowedMissedCleavages;
            if (!SearchMgr.SetParam("allowed_missed_cleavage", allowedMissedCleavages.ToString(CultureInfo.InvariantCulture), allowedMissedCleavages))
            {
                SearchStatusMessage = "Could not set the allowed_missed_cleavage parameter.";
                return false;
            }

            var enzymeTermini = CometUI.SearchSettings.EnzymeTermini;
            if (!SearchMgr.SetParam("num_enzyme_termini", enzymeTermini.ToString(CultureInfo.InvariantCulture), enzymeTermini))
            {
                SearchStatusMessage = "Could not set the num_enzyme_termini parameter.";
                return false;
            }

            var digestMassMin = CometUI.SearchSettings.digestMassRangeMin;
            var digestMassMax = CometUI.SearchSettings.digestMassRangeMax;
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

        private bool ConfigureMassSettings()
        {
            // Configure precursor mass settings

            var precursorMassTol = CometUI.SearchSettings.PrecursorMassTolerance;
            if (!SearchMgr.SetParam("peptide_mass_tolerance", precursorMassTol.ToString(CultureInfo.InvariantCulture), precursorMassTol))
            {
                SearchStatusMessage = "Could not set the peptide_mass_tolerance parameter.";
                return false;
            }

            var precursorMassUnit = CometUI.SearchSettings.PrecursorMassUnit;
            if (!SearchMgr.SetParam("peptide_mass_units", precursorMassUnit.ToString(CultureInfo.InvariantCulture), precursorMassUnit))
            {
                SearchStatusMessage = "Could not set the peptide_mass_units parameter.";
                return false;
            }

            var precursorMassType = CometUI.SearchSettings.PrecursorMassType;
            if (!SearchMgr.SetParam("mass_type_parent", precursorMassType.ToString(CultureInfo.InvariantCulture), precursorMassType))
            {
                SearchStatusMessage = "Could not set the mass_type_parent parameter.";
                return false;
            }

            var isotopeError = CometUI.SearchSettings.PrecursorIsotopeError;
            if (!SearchMgr.SetParam("isotope_error", isotopeError.ToString(CultureInfo.InvariantCulture), isotopeError))
            {
                SearchStatusMessage = "Could not set the isotope_error parameter.";
                return false;
            }


            // Configure fragment mass settings

            var fragmentBinSize = CometUI.SearchSettings.FragmentBinSize;
            if (!SearchMgr.SetParam("fragment_bin_tol", fragmentBinSize.ToString(CultureInfo.InvariantCulture), fragmentBinSize))
            {
                SearchStatusMessage = "Could not set the fragment_bin_tol parameter.";
                return false;
            }

            var fragmentBinOffset = CometUI.SearchSettings.FragmentBinOffset;
            if (!SearchMgr.SetParam("fragment_bin_offset", fragmentBinOffset.ToString(CultureInfo.InvariantCulture), fragmentBinOffset))
            {
                SearchStatusMessage = "Could not set the fragment_bin_offset parameter.";
                return false;
            }

            var fragmentMassType = CometUI.SearchSettings.FragmentMassType;
            if (!SearchMgr.SetParam("mass_type_fragment", fragmentMassType.ToString(CultureInfo.InvariantCulture), fragmentMassType))
            {
                SearchStatusMessage = "Could not set the mass_type_fragment parameter.";
                return false;
            }

            var useSparseMatrix = CometUI.SearchSettings.UseSparseMatrix ? 1 : 0;
            if (!SearchMgr.SetParam("use_sparse_matrix", useSparseMatrix.ToString(CultureInfo.InvariantCulture), useSparseMatrix))
            {
                SearchStatusMessage = "Could not set the use_sparse_matrix parameter.";
                return false;
            }

            // Configure fragment ions

            var useAIons = CometUI.SearchSettings.UseAIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_A_ions", useAIons.ToString(CultureInfo.InvariantCulture), useAIons))
            {
                SearchStatusMessage = "Could not set the use_A_ions parameter.";
                return false;
            }

            var useBIons = CometUI.SearchSettings.UseBIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_B_ions", useBIons.ToString(CultureInfo.InvariantCulture), useBIons))
            {
                SearchStatusMessage = "Could not set the use_B_ions parameter.";
                return false;
            }

            var useCIons = CometUI.SearchSettings.UseCIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_C_ions", useCIons.ToString(CultureInfo.InvariantCulture), useCIons))
            {
                SearchStatusMessage = "Could not set the use_C_ions parameter.";
                return false;
            }

            var useXIons = CometUI.SearchSettings.UseXIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_X_ions", useXIons.ToString(CultureInfo.InvariantCulture), useXIons))
            {
                SearchStatusMessage = "Could not set the use_X_ions parameter.";
                return false;
            }

            var useYIons = CometUI.SearchSettings.UseYIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_Y_ions", useXIons.ToString(CultureInfo.InvariantCulture), useYIons))
            {
                SearchStatusMessage = "Could not set the use_Y_ions parameter.";
                return false;
            }

            var useZIons = CometUI.SearchSettings.UseZIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_Z_ions", useZIons.ToString(CultureInfo.InvariantCulture), useZIons))
            {
                SearchStatusMessage = "Could not set the use_Z_ions parameter.";
                return false;
            }

            var useFlankIons = CometUI.SearchSettings.TheoreticalFragmentIons ? 1 : 0;
            if (!SearchMgr.SetParam("theoretical_fragment_ions", useFlankIons.ToString(CultureInfo.InvariantCulture), useFlankIons))
            {
                SearchStatusMessage = "Could not set the theoretical_fragment_ions parameter.";
                return false;
            }

            var useNLIons = CometUI.SearchSettings.UseNLIons ? 1 : 0;
            if (!SearchMgr.SetParam("use_NL_ions", useNLIons.ToString(CultureInfo.InvariantCulture), useNLIons))
            {
                SearchStatusMessage = "Could not set the use_NL_ions parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureStaticModSettings()
        {
            foreach (var item in CometUI.SearchSettings.StaticMods)
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

            var cTermPeptideMass = CometUI.SearchSettings.StaticModCTermPeptide;
            if (!SearchMgr.SetParam("add_Cterm_peptide", cTermPeptideMass.ToString(CultureInfo.InvariantCulture), cTermPeptideMass))
            {
                SearchStatusMessage = "Could not set the add_Cterm_peptide parameter.";
                return false;
            }

            var nTermPeptideMass = CometUI.SearchSettings.StaticModNTermPeptide;
            if (!SearchMgr.SetParam("add_Nterm_peptide", nTermPeptideMass.ToString(CultureInfo.InvariantCulture), nTermPeptideMass))
            {
                SearchStatusMessage = "Could not set the add_Nterm_peptide parameter.";
                return false;
            }

            var cTermProteinMass = CometUI.SearchSettings.StaticModCTermProtein;
            if (!SearchMgr.SetParam("add_Cterm_protein", cTermProteinMass.ToString(CultureInfo.InvariantCulture), cTermProteinMass))
            {
                SearchStatusMessage = "Could not set the add_Cterm_protein parameter.";
                return false;
            }

            var nTermProteinMass = CometUI.SearchSettings.StaticModNTermProtein;
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
            foreach (var item in CometUI.SearchSettings.VariableMods)
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

                if (!SearchMgr.SetParam(paramName, item, varModsWrapper))
                {
                    SearchStatusMessage = "Could not set the " + paramName + " parameter.";
                    return false;
                }
            }

            var maxVarModsInPeptide = CometUI.SearchSettings.MaxVarModsInPeptide;
            if (!SearchMgr.SetParam("max_variable_mods_in_peptide", maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture), maxVarModsInPeptide))
            {
                SearchStatusMessage = "Could not set the max_variable_mods_in_peptide parameter.";
                return false;
            }

            return true;
        }

        private bool ConfigureMiscSettings()
        {
            // Set the mzXML-specific miscellaneous settings
            var mzxmlScanRangeMin = CometUI.SearchSettings.mzxmlScanRangeMin;
            var mzxmlScanRangeMax = CometUI.SearchSettings.mzxmlScanRangeMax;
            var mzxmlScanRange = new IntRangeWrapper(mzxmlScanRangeMin, mzxmlScanRangeMax);
            string mzxmlScanRangeString = mzxmlScanRangeMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlScanRangeMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("scan_range", mzxmlScanRangeString, mzxmlScanRange))
            {
                SearchStatusMessage = "Could not set the scan_range parameter.";
                return false;
            }

            var mzxmlPrecursorChargeMin = CometUI.SearchSettings.mzxmlPrecursorChargeRangeMin;
            var mzxmlPrecursorChargeMax = CometUI.SearchSettings.mzxmlPrecursorChargeRangeMax;
            var mzxmlPrecursorChargeRange = new IntRangeWrapper(mzxmlPrecursorChargeMin, mzxmlPrecursorChargeMax);
            string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeMin.ToString(CultureInfo.InvariantCulture)
                                                    + " " + mzxmlPrecursorChargeMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("precursor_charge", mzxmlPrecursorChargeRageString, mzxmlPrecursorChargeRange))
            {
                SearchStatusMessage = "Could not set the precursor_charge parameter.";
                return false;
            }

            var mzxmlMSLevel = CometUI.SearchSettings.mzxmlMsLevel;
            if (!SearchMgr.SetParam("ms_level", mzxmlMSLevel.ToString(CultureInfo.InvariantCulture), mzxmlMSLevel))
            {
                SearchStatusMessage = "Could not set the ms_level parameter.";
                return false;
            }

            var mzxmlActivationMethod = CometUI.SearchSettings.mzxmlActivationMethod;
            if (!SearchMgr.SetParam("activation_method", mzxmlActivationMethod, mzxmlActivationMethod))
            {
                SearchStatusMessage = "Could not set the activation_method parameter.";
                return false;
            }

            // Set the spectral processing-specific miscellaneous settings
            var minPeaks = CometUI.SearchSettings.spectralProcessingMinPeaks;
            if (!SearchMgr.SetParam("minimum_peaks", minPeaks.ToString(CultureInfo.InvariantCulture), minPeaks))
            {
                SearchStatusMessage = "Could not set the minimum_peaks parameter.";
                return false;
            }

            var minIntensity = CometUI.SearchSettings.spectralProcessingMinIntensity;
            if (!SearchMgr.SetParam("minimum_intensity", minIntensity.ToString(CultureInfo.InvariantCulture), minIntensity))
            {
                SearchStatusMessage = "Could not set the minimum_intensity parameter.";
                return false;
            }

            var removePrecursorTol = CometUI.SearchSettings.spectralProcessingRemovePrecursorTol;
            if (!SearchMgr.SetParam("remove_precursor_tolerance", removePrecursorTol.ToString(CultureInfo.InvariantCulture), removePrecursorTol))
            {
                SearchStatusMessage = "Could not set the remove_precursor_tolerance parameter.";
                return false;
            }

            var removePrecursorPeak = CometUI.SearchSettings.spectralProcessingRemovePrecursorPeak;
            if (!SearchMgr.SetParam("remove_precursor_peak", removePrecursorPeak.ToString(CultureInfo.InvariantCulture), removePrecursorPeak))
            {
                SearchStatusMessage = "Could not set the remove_precursor_peak parameter.";
                return false;
            }

            var clearMzMin = CometUI.SearchSettings.spectralProcessingClearMzMin;
            var clearMzMax = CometUI.SearchSettings.spectralProcessingClearMzMax;
            var clearMzRange = new DoubleRangeWrapper(clearMzMin, clearMzMax);
            string clearMzRangeString = clearMzMin.ToString(CultureInfo.InvariantCulture)
                                          + " " + clearMzMax.ToString(CultureInfo.InvariantCulture);
            if (!SearchMgr.SetParam("clear_mz_range", clearMzRangeString, clearMzRange))
            {
                SearchStatusMessage = "Could not set the clear_mz_range parameter.";
                return false;
            }

            // Configure the rest of the miscellaneous parameters
            var spectrumBatchSize = CometUI.SearchSettings.SpectrumBatchSize;
            if (!SearchMgr.SetParam("spectrum_batch_size", spectrumBatchSize.ToString(CultureInfo.InvariantCulture), spectrumBatchSize))
            {
                SearchStatusMessage = "Could not set the spectrum_batch_size parameter.";
                return false;
            }

            var numThreads = CometUI.SearchSettings.NumThreads;
            if (!SearchMgr.SetParam("num_threads", numThreads.ToString(CultureInfo.InvariantCulture), numThreads))
            {
                SearchStatusMessage = "Could not set the num_threads parameter.";
                return false;
            }

            var numResults = CometUI.SearchSettings.NumResults;
            if (!SearchMgr.SetParam("num_results", numResults.ToString(CultureInfo.InvariantCulture), numResults))
            {
                SearchStatusMessage = "Could not set the num_results parameter.";
                return false;
            }

            var maxFragmentCharge = CometUI.SearchSettings.MaxFragmentCharge;
            if (!SearchMgr.SetParam("max_fragment_charge", maxFragmentCharge.ToString(CultureInfo.InvariantCulture), maxFragmentCharge))
            {
                SearchStatusMessage = "Could not set the max_fragment_charge parameter.";
                return false;
            }

            var maxPrecursorCharge = CometUI.SearchSettings.MaxPrecursorCharge;
            if (!SearchMgr.SetParam("max_precursor_charge", maxPrecursorCharge.ToString(CultureInfo.InvariantCulture), maxPrecursorCharge))
            {
                SearchStatusMessage = "Could not set the max_precursor_charge parameter.";
                return false;
            }

            var clipNTermMethionine = CometUI.SearchSettings.ClipNTermMethionine ? 1 : 0;
            if (!SearchMgr.SetParam("clip_nterm_methionine", clipNTermMethionine.ToString(CultureInfo.InvariantCulture), clipNTermMethionine))
            {
                SearchStatusMessage = "Could not set the clip_nterm_methionine parameter.";
                return false;
            }

            return true;
        }
    }
}
