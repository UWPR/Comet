using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Globalization;
using CometUI.Properties;

namespace CometUI
{
    // Todo: Comment the public classes and methods here

    public class CometParam
    {
        public CometParamType Type { get; set; }
        public String Value { get; set; }

        public CometParam(CometParamType paramType, ref String strValue)
        {
            Type = paramType;
            Value = strValue;
        }
    }

    public class TypedCometParam <T> : CometParam
    {
        public new T Value { get; set; }

        public TypedCometParam(CometParamType paramType, String strValue, T value) : base(paramType, ref strValue)
        {
            // The field has the same type as the parameter.
            Value = value;
        }
    }

    public enum CometParamType
    {
        Unknown = 0,
        Int,
        Double,
        String,
        VarMod,
        DoubleRange,
        IntRange,
        StrCollection
    }

    public class CometParamsMap
    {
        private const int MaxNumVarMods = 6;
        private const int NumVarModFieldsInSettings = 4;
        private const int NumStaticModFieldsInSettings = 3;

        public Dictionary<string, CometParam> CometParams { get; private set; }

        public CometParamsMap()
        {
            CometParams = new Dictionary<string, CometParam>();
            UpdateCometParamsFromSettings(CometUI.SearchSettings);
        }

        public CometParamsMap(SearchSettings settings)
        {
            CometParams = new Dictionary<string, CometParam>();
            UpdateCometParamsFromSettings(settings);
        }

        public bool GetSettingsFromCometParams(SearchSettings cometSettings)
        {
            String paramValueStr;

            String dbName;
            if (!GetCometParamValue("database_name", out dbName, out paramValueStr))
            {
                return false;
            }
            cometSettings.ProteomeDatabaseFile = dbName;

            int searchType;
            if (!GetCometParamValue("decoy_search", out searchType, out paramValueStr))
            {
                return false;
            }

            cometSettings.SearchType = searchType;

            String decoyPrefix;
            if (!GetCometParamValue("decoy_prefix", out decoyPrefix, out paramValueStr))
            {
                return false;
            }
            cometSettings.DecoyPrefix = decoyPrefix;

            int nucleotideReadingFrame;
            if (!GetCometParamValue("nucleotide_reading_frame",
                out nucleotideReadingFrame,
                out paramValueStr))
            {
                return false;
            }
            cometSettings.NucleotideReadingFrame = nucleotideReadingFrame;

            int outputPepXMLFile;
            if (!GetCometParamValue("output_pepxmlfile", out outputPepXMLFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatPepXML = outputPepXMLFile == 1;

            int outputPercolatorFile;
            if (!GetCometParamValue("output_percolatorfile", out outputPercolatorFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatPercolator = outputPercolatorFile == 1;

            int outputTextFile;
            if (!GetCometParamValue("output_txtfile", out outputTextFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatTextFile = outputTextFile == 1;

            int outputSqtToStdout;
            if (!GetCometParamValue("output_sqtstream", out outputSqtToStdout, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatSqtToStandardOutput = outputSqtToStdout == 1;

            int outputSqtFile;
            if (!GetCometParamValue("output_sqtfile", out outputSqtFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatSqtFile = outputSqtFile == 1;

            int outputOutFile;
            if (!GetCometParamValue("output_outfiles", out outputOutFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatOutFiles = outputOutFile == 1;

            int printExpectScore;
            if (!GetCometParamValue("print_expect_score", out printExpectScore, out paramValueStr))
            {
                return false;
            }
            cometSettings.PrintExpectScoreInPlaceOfSP = printExpectScore == 1;

            int showFragmentIons;
            if (!GetCometParamValue("show_fragment_ions", out showFragmentIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatShowFragmentIons = showFragmentIons == 1;

            int skipResearching;
            if (!GetCometParamValue("skip_researching", out skipResearching, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatSkipReSearching = skipResearching == 1;

            int numOutputLines;
            if (!GetCometParamValue("num_output_lines", out numOutputLines, out paramValueStr))
            {
                return false;
            }
            cometSettings.NumOutputLines = numOutputLines;

            int searchEnzymeNumber;
            if (!GetCometParamValue("search_enzyme_number", out searchEnzymeNumber, out paramValueStr))
            {
                return false;
            }
            cometSettings.SearchEnzymeNumber = searchEnzymeNumber;

            int sampleEnzymeNumber;
            if (!GetCometParamValue("sample_enzyme_number", out sampleEnzymeNumber, out paramValueStr))
            {
                return false;
            }
            cometSettings.SampleEnzymeNumber = sampleEnzymeNumber;

            int allowedMissedCleavages;
            if (!GetCometParamValue("allowed_missed_cleavage", out allowedMissedCleavages, out paramValueStr))
            {
                return false;
            }
            cometSettings.AllowedMissedCleavages = allowedMissedCleavages;

            int enzymeTermini;
            if (!GetCometParamValue("num_enzyme_termini", out enzymeTermini, out paramValueStr))
            {
                return false;
            }
            cometSettings.EnzymeTermini = enzymeTermini;

            DoubleRange digestMassRange;
            if (!GetCometParamValue("digest_mass_range", out digestMassRange, out paramValueStr))
            {
                return false;
            }
            cometSettings.digestMassRangeMin = digestMassRange.Start;
            cometSettings.digestMassRangeMax = digestMassRange.End;

            double precursorMassTol;
            if (!GetCometParamValue("peptide_mass_tolerance", out precursorMassTol, out paramValueStr))
            {
                return false;
            }
            cometSettings.PrecursorMassTolerance = precursorMassTol;

            int precursorMassUnit;
            if (!GetCometParamValue("peptide_mass_units", out precursorMassUnit, out paramValueStr))
            {
                return false;
            }
            cometSettings.PrecursorMassUnit = precursorMassUnit;

            int precursorMassType;
            if (!GetCometParamValue("mass_type_parent", out precursorMassType, out paramValueStr))
            {
                return false;
            }
            cometSettings.PrecursorMassType = precursorMassType;

            int precursorTolType;
            if (!GetCometParamValue("precursor_tolerance_type", out precursorTolType, out paramValueStr))
            {
                return false;
            }
            cometSettings.PrecursorToleranceType = precursorTolType;

            int isotopeError;
            if (!GetCometParamValue("isotope_error", out isotopeError, out paramValueStr))
            {
                return false;
            }
            cometSettings.PrecursorIsotopeError = isotopeError;

            double fragmentBinSize;
            if (!GetCometParamValue("fragment_bin_tol", out fragmentBinSize, out paramValueStr))
            {
                return false;
            }
            cometSettings.FragmentBinSize = fragmentBinSize;

            double fragmentBinOffset;
            if (!GetCometParamValue("fragment_bin_offset", out fragmentBinOffset, out paramValueStr))
            {
                return false;
            }
            cometSettings.FragmentBinOffset = fragmentBinOffset;

            int fragmentMassType;
            if (!GetCometParamValue("mass_type_fragment", out fragmentMassType, out paramValueStr))
            {
                return false;
            }
            cometSettings.FragmentMassType = fragmentMassType;

            int useSparseMatrix;
            if (!GetCometParamValue("use_sparse_matrix", out useSparseMatrix, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseSparseMatrix = useSparseMatrix == 1;

            int useAIons;
            if (!GetCometParamValue("use_A_ions", out useAIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseAIons = useAIons == 1;

            int useBIons;
            if (!GetCometParamValue("use_B_ions", out useBIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseBIons = useBIons == 1;

            int useCIons;
            if (!GetCometParamValue("use_C_ions", out useCIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseCIons = useCIons == 1;

            int useXIons;
            if (!GetCometParamValue("use_X_ions", out useXIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseXIons = useXIons == 1;

            int useYIons;
            if (!GetCometParamValue("use_Y_ions", out useYIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseYIons = useYIons == 1;

            int useZIons;
            if (!GetCometParamValue("use_Z_ions", out useZIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseZIons = useZIons == 1;

            int useFlankIons;
            if (!GetCometParamValue("theoretical_fragment_ions", out useFlankIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.TheoreticalFragmentIons = useFlankIons == 1;

            int useNLIons;
            if (!GetCometParamValue("use_NL_ions", out useNLIons, out paramValueStr))
            {
                return false;
            }
            cometSettings.UseNLIons = useNLIons == 1;

            var staticMods = new StringCollection();
            if (!AddStaticModsToStrCollection(ref staticMods))
            {
                return false;
            }
            cometSettings.StaticMods = staticMods;

            double cTermPeptideMass;
            if (!GetCometParamValue("add_Cterm_peptide", out cTermPeptideMass, out paramValueStr))
            {
                return false;
            }
            cometSettings.StaticModCTermPeptide = cTermPeptideMass;

            double nTermPeptideMass;
            if (!GetCometParamValue("add_Nterm_peptide", out nTermPeptideMass, out paramValueStr))
            {
                return false;
            }
            cometSettings.StaticModNTermPeptide = nTermPeptideMass;

            double cTermProteinMass;
            if (!GetCometParamValue("add_Cterm_protein", out cTermProteinMass, out paramValueStr))
            {
                return false;
            }
            cometSettings.StaticModCTermProtein = cTermProteinMass;

            double nTermProteinMass;
            if (!GetCometParamValue("add_Nterm_protein", out nTermProteinMass, out paramValueStr))
            {
                return false;
            }
            cometSettings.StaticModNTermProtein = nTermProteinMass;

            var varModsStrCollection = new StringCollection();
            if (!AddVarModsToStrCollection(ref varModsStrCollection))
            {
                return false;
            }
            cometSettings.VariableMods = varModsStrCollection;

            double varCTerminus;
            if (!GetCometParamValue("variable_C_terminus", out varCTerminus, out paramValueStr))
            {
                return false;
            }
            cometSettings.VariableCTerminus = varCTerminus;

            int varCTerminusDist;
            if (!GetCometParamValue("variable_C_terminus_distance", out varCTerminusDist, out paramValueStr))
            {
                return false;
            }
            cometSettings.VariableCTermDistance = varCTerminusDist;

            double varNTerminus;
            if (!GetCometParamValue("variable_N_terminus", out varNTerminus, out paramValueStr))
            {
                return false;
            }
            cometSettings.VariableNTerminus = varNTerminus;

            int varNTerminusDist;
            if (!GetCometParamValue("variable_N_terminus_distance", out varNTerminusDist, out paramValueStr))
            {
                return false;
            }
            cometSettings.VariableNTermDistance = varNTerminusDist;

            int maxVarModsInPeptide;
            if (!GetCometParamValue("max_variable_mods_in_peptide", out maxVarModsInPeptide, out paramValueStr))
            {
                return false;
            }
            cometSettings.MaxVarModsInPeptide = maxVarModsInPeptide;

            IntRange mzxmlScanRange;
            if (!GetCometParamValue("scan_range", out mzxmlScanRange, out paramValueStr))
            {
                return false;
            }
            cometSettings.mzxmlScanRangeMin = mzxmlScanRange.Start;
            cometSettings.mzxmlScanRangeMax = mzxmlScanRange.End;

            IntRange mzxmlPrecursorChargeRange;
            if (!GetCometParamValue("precursor_charge", out mzxmlPrecursorChargeRange, out paramValueStr))
            {
                return false;
            }
            cometSettings.mzxmlPrecursorChargeRangeMin = mzxmlPrecursorChargeRange.Start;
            cometSettings.mzxmlPrecursorChargeRangeMax = mzxmlPrecursorChargeRange.End;

            int mzxmlMSLevel;
            if (!GetCometParamValue("ms_level", out mzxmlMSLevel, out paramValueStr))
            {
                return false;
            }
            cometSettings.mzxmlMsLevel = mzxmlMSLevel;

            String mzxmlActivationMethod;
            if (!GetCometParamValue("activation_method", out mzxmlActivationMethod, out paramValueStr))
            {
                return false;
            }
            cometSettings.mzxmlActivationMethod = mzxmlActivationMethod;

            int minPeaks;
            if (!GetCometParamValue("minimum_peaks", out minPeaks, out paramValueStr))
            {
                return false;
            }
            cometSettings.spectralProcessingMinPeaks = minPeaks;

            double minIntensity;
            if (!GetCometParamValue("minimum_intensity", out minIntensity, out paramValueStr))
            {
                return false;
            }
            cometSettings.spectralProcessingMinIntensity = minIntensity;

            double removePrecursorTol;
            if (!GetCometParamValue("remove_precursor_tolerance", out removePrecursorTol, out paramValueStr))
            {
                return false;
            }
            cometSettings.spectralProcessingRemovePrecursorTol = removePrecursorTol;

            int removePrecursorPeak;
            if (!GetCometParamValue("remove_precursor_peak", out removePrecursorPeak, out paramValueStr))
            {
                return false;
            }
            cometSettings.spectralProcessingRemovePrecursorPeak = removePrecursorPeak;

            DoubleRange clearMzRange;
            if (!GetCometParamValue("clear_mz_range", out clearMzRange, out paramValueStr))
            {
                return false;
            }
            cometSettings.spectralProcessingClearMzMin = clearMzRange.Start;
            cometSettings.spectralProcessingClearMzMax = clearMzRange.End;

            int spectrumBatchSize;
            if (!GetCometParamValue("spectrum_batch_size", out spectrumBatchSize, out paramValueStr))
            {
                return false;
            }
            cometSettings.SpectrumBatchSize = spectrumBatchSize;

            int numThreads;
            if (!GetCometParamValue("num_threads", out numThreads, out paramValueStr))
            {
                return false;
            }
            cometSettings.NumThreads = numThreads;

            int numResults;
            if (!GetCometParamValue("num_results", out numResults, out paramValueStr))
            {
                return false;
            }
            cometSettings.NumResults = numResults;

            int maxFragmentCharge;
            if (!GetCometParamValue("max_fragment_charge", out maxFragmentCharge, out paramValueStr))
            {
                return false;
            }
            cometSettings.MaxFragmentCharge = maxFragmentCharge;

            int maxPrecursorCharge;
            if (!GetCometParamValue("max_precursor_charge", out maxPrecursorCharge, out paramValueStr))
            {
                return false;
            }
            cometSettings.MaxPrecursorCharge = maxPrecursorCharge;

            int clipNTermMethionine;
            if (!GetCometParamValue("clip_nterm_methionine", out clipNTermMethionine, out paramValueStr))
            {
                return false;
            }
            cometSettings.ClipNTermMethionine = clipNTermMethionine == 1;

            StringCollection enzymeInfo;
            if (!GetCometParamValue("[COMET_ENZYME_INFO]", out enzymeInfo, out paramValueStr))
            {
                return false;
            }
            cometSettings.EnzymeInfo = enzymeInfo;

            return true;
        }

        public bool UpdateCometParamsFromSettings(SearchSettings settings)
        {
            var dbName = settings.ProteomeDatabaseFile;
            if (!UpdateCometParam("database_name", 
                             new TypedCometParam<string>(CometParamType.String,
                                                         dbName,
                                                         dbName)))
            {
                return false;
            }

            var searchType = settings.SearchType;
            if (!UpdateCometParam("decoy_search",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      searchType.ToString(CultureInfo.InvariantCulture),
                                                      searchType)))
            {
                return false;
            }

            var decoyPrefix = settings.DecoyPrefix;
            if (!UpdateCometParam("decoy_prefix",
                             new TypedCometParam<string>(CometParamType.String,
                                                         decoyPrefix,
                                                         decoyPrefix)))
            {
                return false;
            }

            var nucleotideReadingFrame = settings.NucleotideReadingFrame;
            if (!UpdateCometParam("nucleotide_reading_frame",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture),
                                                      nucleotideReadingFrame)))
            {
                return false;
            }

            var outputPepXMLFile = settings.OutputFormatPepXML ? 1 : 0;
            if (!UpdateCometParam("output_pepxmlfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputPepXMLFile.ToString(CultureInfo.InvariantCulture),
                                                      outputPepXMLFile)))
            {
                return false;
            }

            var outputPercolatorFile = settings.OutputFormatPercolator ? 1 : 0;
            if (!UpdateCometParam("output_percolatorfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputPercolatorFile.ToString(CultureInfo.InvariantCulture),
                                                      outputPercolatorFile)))
            {
                return false;
            }

            var outputTextFile = settings.OutputFormatTextFile ? 1 : 0;
            if (!UpdateCometParam("output_txtfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputTextFile.ToString(CultureInfo.InvariantCulture),
                                                      outputTextFile)))
            {
                return false;
            }

            var outputSqtToStdout = settings.OutputFormatSqtToStandardOutput ? 1 : 0;
            if (!UpdateCometParam("output_sqtstream",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputSqtToStdout.ToString(CultureInfo.InvariantCulture),
                                                      outputSqtToStdout)))
            {
                return false;
            }
            
            var outputSqtFile = settings.OutputFormatSqtFile ? 1 : 0;
            if (!UpdateCometParam("output_sqtfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputSqtFile.ToString(CultureInfo.InvariantCulture),
                                                      outputSqtFile)))
            {
                return false;
            }

            var outputOutFile = settings.OutputFormatOutFiles ? 1 : 0;
            if (!UpdateCometParam("output_outfiles",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputOutFile.ToString(CultureInfo.InvariantCulture),
                                                      outputOutFile)))
            {
                return false;
            }

            var printExpectScore = settings.PrintExpectScoreInPlaceOfSP ? 1 : 0;
            if (!UpdateCometParam("print_expect_score",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      printExpectScore.ToString(CultureInfo.InvariantCulture),
                                                      printExpectScore)))
            {
                return false;
            }

            var showFragmentIons = settings.OutputFormatShowFragmentIons ? 1 : 0;
            if (!UpdateCometParam("show_fragment_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      showFragmentIons.ToString(CultureInfo.InvariantCulture),
                                                      showFragmentIons)))
            {
                return false;
            }

            var skipResearching = settings.OutputFormatSkipReSearching ? 1 : 0;
            if (!UpdateCometParam("skip_researching",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      skipResearching.ToString(CultureInfo.InvariantCulture),
                                                      skipResearching)))
            {
                return false;
            }

            var numOutputLines = settings.NumOutputLines;
            if (!UpdateCometParam("num_output_lines",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      numOutputLines.ToString(CultureInfo.InvariantCulture),
                                                      numOutputLines)))
            {
                return false;
            }

            var searchEnzymeNumber = settings.SearchEnzymeNumber;
            if (!UpdateCometParam("search_enzyme_number",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      searchEnzymeNumber.ToString(CultureInfo.InvariantCulture),
                                                      searchEnzymeNumber)))
            {
                return false;
            }

            var sampleEnzymeNumber = settings.SampleEnzymeNumber;
            if (!UpdateCometParam("sample_enzyme_number",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      sampleEnzymeNumber.ToString(CultureInfo.InvariantCulture),
                                                      sampleEnzymeNumber)))
            {
                return false;
            }

            var allowedMissedCleavages = settings.AllowedMissedCleavages;
            if (!UpdateCometParam("allowed_missed_cleavage",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      allowedMissedCleavages.ToString(CultureInfo.InvariantCulture),
                                                      allowedMissedCleavages)))
            {
                return false;
            }

            var enzymeTermini = settings.EnzymeTermini;
            if (!UpdateCometParam("num_enzyme_termini",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      enzymeTermini.ToString(CultureInfo.InvariantCulture),
                                                      enzymeTermini)))
            {
                return false;
            }

            var digestMassRange = new DoubleRange(settings.digestMassRangeMin,
                                   settings.digestMassRangeMax);
            string digestMassRangeString = digestMassRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + digestMassRange.End.ToString(CultureInfo.InvariantCulture);
            if (!UpdateCometParam("digest_mass_range",
                             new TypedCometParam<DoubleRange>(CometParamType.DoubleRange,
                                                              digestMassRangeString,
                                                              digestMassRange)))
            {
                return false;
            }

            var precursorMassTol = settings.PrecursorMassTolerance;
            if (!UpdateCometParam("peptide_mass_tolerance",
                             new TypedCometParam<double>(CometParamType.Double,
                                                         precursorMassTol.ToString(CultureInfo.InvariantCulture),
                                                         precursorMassTol)))
            {
                return false;
            }

            var precursorMassUnit = settings.PrecursorMassUnit;
            if (!UpdateCometParam("peptide_mass_units",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         precursorMassUnit.ToString(CultureInfo.InvariantCulture),
                                                         precursorMassUnit)))
            {
                return false;
            }

            var precursorMassType = settings.PrecursorMassType;
            if (!UpdateCometParam("mass_type_parent",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         precursorMassType.ToString(CultureInfo.InvariantCulture),
                                                         precursorMassType)))
            {
                return false;
            }

            var precursorTolType = settings.PrecursorToleranceType;
            if (!UpdateCometParam("precursor_tolerance_type",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         precursorTolType.ToString(CultureInfo.InvariantCulture),
                                                         precursorTolType)))
            {
                return false;
            }

            var isotopeError = settings.PrecursorIsotopeError;
            if (!UpdateCometParam("isotope_error",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         isotopeError.ToString(CultureInfo.InvariantCulture),
                                                         isotopeError)))
            {
                return false;
            }

            var fragmentBinSize = settings.FragmentBinSize;
            if (!UpdateCometParam("fragment_bin_tol",
                             new TypedCometParam<double>(CometParamType.Double,
                                                         fragmentBinSize.ToString(CultureInfo.InvariantCulture),
                                                         fragmentBinSize)))
            {
                return false;
            }

            var fragmentBinOffset = settings.FragmentBinOffset;
            if (!UpdateCometParam("fragment_bin_offset",
                             new TypedCometParam<double>(CometParamType.Double,
                                                         fragmentBinOffset.ToString(CultureInfo.InvariantCulture),
                                                         fragmentBinOffset)))
            {
                return false;
            }

            var fragmentMassType = settings.FragmentMassType;
            if (!UpdateCometParam("mass_type_fragment",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         fragmentMassType.ToString(CultureInfo.InvariantCulture),
                                                         fragmentMassType)))
            {
                return false;
            }

            var useSparseMatrix = settings.UseSparseMatrix ? 1 : 0;
            if (!UpdateCometParam("use_sparse_matrix",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useSparseMatrix.ToString(CultureInfo.InvariantCulture),
                                                      useSparseMatrix)))
            {
                return false;
            }

            var useAIons = settings.UseAIons ? 1 : 0;
            if (!UpdateCometParam("use_A_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useAIons.ToString(CultureInfo.InvariantCulture),
                                                      useAIons)))
            {
                return false;
            }

            var useBIons = settings.UseBIons ? 1 : 0;
            if (!UpdateCometParam("use_B_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useBIons.ToString(CultureInfo.InvariantCulture),
                                                      useBIons)))
            {
                return false;
            }

            var useCIons = settings.UseCIons ? 1 : 0;
            if (!UpdateCometParam("use_C_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useCIons.ToString(CultureInfo.InvariantCulture),
                                                      useCIons)))
            {
                return false;
            }

            var useXIons = settings.UseXIons ? 1 : 0;
            if (!UpdateCometParam("use_X_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useXIons.ToString(CultureInfo.InvariantCulture),
                                                      useXIons)))
            {
                return false;
            }

            var useYIons = settings.UseYIons ? 1 : 0;
            if (!UpdateCometParam("use_Y_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useYIons.ToString(CultureInfo.InvariantCulture),
                                                      useYIons)))
            {
                return false;
            }

            var useZIons = settings.UseZIons ? 1 : 0;
            if (!UpdateCometParam("use_Z_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useZIons.ToString(CultureInfo.InvariantCulture),
                                                      useZIons)))
            {
                return false;
            }

            var useFlankIons = settings.TheoreticalFragmentIons ? 1 : 0;
            if (!UpdateCometParam("theoretical_fragment_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useFlankIons.ToString(CultureInfo.InvariantCulture),
                                                      useFlankIons)))
            {
                return false;
            }

            var useNLIons = settings.UseNLIons ? 1 : 0;
            if (!UpdateCometParam("use_NL_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useNLIons.ToString(CultureInfo.InvariantCulture),
                                                      useNLIons)))
            {
                return false;
            }

            foreach (var item in settings.StaticMods)
            {
                String[] staticMods = item.Split(',');

                if (staticMods.Length < NumStaticModFieldsInSettings)
                {
                    return false;
                }

                String paramName;
                String aaName;
                if (!GetStaticModParamInfo(staticMods[1], out paramName, out aaName))
                {
                    return false;
                }

                double massDiff = Convert.ToDouble(staticMods[2]);
                if (!UpdateCometParam(paramName,
                                 new TypedCometParam<double>(CometParamType.Double,
                                                             massDiff.ToString(CultureInfo.InvariantCulture),
                                                             massDiff)))
                {
                    return false;
                }
            }

            var cTermPeptideMass = settings.StaticModCTermPeptide;
            if (!UpdateCometParam("add_Cterm_peptide",
                new TypedCometParam<double>(CometParamType.Double,
                                            cTermPeptideMass.ToString(CultureInfo.InvariantCulture),
                                            cTermPeptideMass)))
            {
                return false;
            }

            var nTermPeptideMass = settings.StaticModNTermPeptide;
            if (!UpdateCometParam("add_Nterm_peptide",
                new TypedCometParam<double>(CometParamType.Double,
                                            nTermPeptideMass.ToString(CultureInfo.InvariantCulture),
                                            nTermPeptideMass)))
            {
                return false;
            }

            var cTermProteinMass = settings.StaticModCTermProtein;
            if (!UpdateCometParam("add_Cterm_protein",
                new TypedCometParam<double>(CometParamType.Double,
                                            cTermProteinMass.ToString(CultureInfo.InvariantCulture),
                                            cTermProteinMass)))
            {
                return false;
            }

            var nTermProteinMass = settings.StaticModNTermProtein;
            if (!UpdateCometParam("add_Nterm_protein",
                new TypedCometParam<double>(CometParamType.Double,
                                            nTermProteinMass.ToString(CultureInfo.InvariantCulture),
                                            nTermProteinMass)))
            {
                return false;
            }

            int modNum = 0;
            foreach (var item in settings.VariableMods)
            {
                modNum++;
                string paramName = "variable_mod" + modNum;
                string[] varModsStr = item.Split(',');
                if (varModsStr.Length < NumVarModFieldsInSettings)
                {
                    return false;
                }
                var varMods = new VarMod(Convert.ToDouble(varModsStr[1]),   // mass diff
                                          varModsStr[0],                    // residue
                                          Convert.ToInt32(varModsStr[2]),   // binary mod
                                          Convert.ToInt32(varModsStr[3]));  // max mods
                var varModsStrValue = varMods.VarModMass + " " 
                    + varMods.VarModChar + " " 
                    + varMods.BinaryMod + " " 
                    + varMods.MaxNumVarModAAPerMod;
                if (!UpdateCometParam(paramName,
                                 new TypedCometParam<VarMod>(CometParamType.VarMod,
                                                             varModsStrValue,
                                                             varMods)))
                {
                    return false;
                }

            }

            var varCTerminus = settings.VariableCTerminus;
            if (!UpdateCometParam("variable_C_terminus",
                             new TypedCometParam<double>(CometParamType.Double,
                                            varCTerminus.ToString(CultureInfo.InvariantCulture),
                                            varCTerminus)))
            {
                return false;
            }

            var varCTerminusDist = settings.VariableCTermDistance;
            if (!UpdateCometParam("variable_C_terminus_distance",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      varCTerminusDist.ToString(CultureInfo.InvariantCulture),
                                                      varCTerminusDist)))
            {
                return false;
            }

            var varNTerminus = settings.VariableNTerminus;
            if (!UpdateCometParam("variable_N_terminus",
                             new TypedCometParam<double>(CometParamType.Double,
                                            varNTerminus.ToString(CultureInfo.InvariantCulture),
                                            varNTerminus)))
            {
                return false;
            }

            var varNTerminusDist = settings.VariableNTermDistance;
            if (!UpdateCometParam("variable_N_terminus_distance",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     varNTerminusDist.ToString(CultureInfo.InvariantCulture),
                                                     varNTerminusDist)))
            {
                return false;
            }

            var maxVarModsInPeptide = settings.MaxVarModsInPeptide;
            if (!UpdateCometParam("max_variable_mods_in_peptide",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture),
                                                      maxVarModsInPeptide)))
            {
                return false;
            }

            var mzxmlScanRange = new IntRange(settings.mzxmlScanRangeMin, settings.mzxmlScanRangeMax);
            string mzxmlScanRangeString = mzxmlScanRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlScanRange.End.ToString(CultureInfo.InvariantCulture);
            if (!UpdateCometParam("scan_range",
                             new TypedCometParam<IntRange>(CometParamType.IntRange,
                                                           mzxmlScanRangeString,
                                                           mzxmlScanRange)))
            {
                return false;
            }

            var mzxmlPrecursorChargeRange = new IntRange(settings.mzxmlPrecursorChargeRangeMin, settings.mzxmlPrecursorChargeRangeMax);
            string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlPrecursorChargeRange.End.ToString(CultureInfo.InvariantCulture);
            if (!UpdateCometParam("precursor_charge",
                             new TypedCometParam<IntRange>(CometParamType.IntRange,
                                                           mzxmlPrecursorChargeRageString,
                                                           mzxmlPrecursorChargeRange)))
            {
                return false;
            }

            var mzxmlMSLevel = settings.mzxmlMsLevel;
            if (!UpdateCometParam("ms_level",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      mzxmlMSLevel.ToString(CultureInfo.InvariantCulture),
                                                      mzxmlMSLevel)))
            {
                return false;
            }

            var mzxmlActivationMethod = settings.mzxmlActivationMethod;
            if (!UpdateCometParam("activation_method",
                             new TypedCometParam<string>(CometParamType.String,
                                                         mzxmlActivationMethod,
                                                         mzxmlActivationMethod)))
            {
                return false;
            }

            var minPeaks = settings.spectralProcessingMinPeaks;
            if (!UpdateCometParam("minimum_peaks",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      minPeaks.ToString(CultureInfo.InvariantCulture),
                                                      minPeaks)))
            {
                return false;
            }

            var minIntensity = settings.spectralProcessingMinIntensity;
            if (!UpdateCometParam("minimum_intensity",
                new TypedCometParam<double>(CometParamType.Double,
                                            minIntensity.ToString(CultureInfo.InvariantCulture),
                                            minIntensity)))
            {
                return false;
            }

            var removePrecursorTol = settings.spectralProcessingRemovePrecursorTol;
            if (!UpdateCometParam("remove_precursor_tolerance",
                new TypedCometParam<double>(CometParamType.Double,
                                            removePrecursorTol.ToString(CultureInfo.InvariantCulture),
                                            removePrecursorTol)))
            {
                return false;
            }

            var removePrecursorPeak = settings.spectralProcessingRemovePrecursorPeak;
            if (!UpdateCometParam("remove_precursor_peak",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      removePrecursorPeak.ToString(CultureInfo.InvariantCulture),
                                                      removePrecursorPeak)))
            {
                return false;
            }

            var clearMzRange = new DoubleRange(settings.spectralProcessingClearMzMin,
                                               settings.spectralProcessingClearMzMax);
            string clearMzRangeString = clearMzRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + clearMzRange.End.ToString(CultureInfo.InvariantCulture);
            if (!UpdateCometParam("clear_mz_range",
                             new TypedCometParam<DoubleRange>(CometParamType.DoubleRange,
                                                              clearMzRangeString,
                                                              clearMzRange)))
            {
                return false;
            }

            var spectrumBatchSize = settings.SpectrumBatchSize;
            if (!UpdateCometParam("spectrum_batch_size",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      spectrumBatchSize.ToString(CultureInfo.InvariantCulture),
                                                      spectrumBatchSize)))
            {
                return false;
            }

            var numThreads = settings.NumThreads;
            if (!UpdateCometParam("num_threads",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      numThreads.ToString(CultureInfo.InvariantCulture),
                                                      numThreads)))
            {
                return false;
            }

            var numResults = settings.NumResults;
            if (!UpdateCometParam("num_results",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      numResults.ToString(CultureInfo.InvariantCulture),
                                                      numResults)))
            {
                return false;
            }

            var maxFragmentCharge = settings.MaxFragmentCharge;
            if (!UpdateCometParam("max_fragment_charge",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      maxFragmentCharge.ToString(CultureInfo.InvariantCulture),
                                                      maxFragmentCharge)))
            {
                return false;
            }

            var maxPrecursorCharge = settings.MaxPrecursorCharge;
            if (!UpdateCometParam("max_precursor_charge",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      maxPrecursorCharge.ToString(CultureInfo.InvariantCulture),
                                                      maxPrecursorCharge)))
            {
                return false;
            }

            var clipNTermMethionine = settings.ClipNTermMethionine ? 1 : 0;
            if (!UpdateCometParam("clip_nterm_methionine",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      clipNTermMethionine.ToString(CultureInfo.InvariantCulture),
                                                      clipNTermMethionine)))
            {
                return false;
            }

            string enzymeInfoStr = String.Empty;
            foreach (var row in settings.EnzymeInfo)
            {
                enzymeInfoStr += row + Environment.NewLine;
            }

            if (!UpdateCometParam("[COMET_ENZYME_INFO]",
                             new TypedCometParam<StringCollection>(CometParamType.StrCollection,
                                                                   enzymeInfoStr,
                                                                   settings.EnzymeInfo)))
            {
                return false;
            }

            return true;
        }

        public bool GetCometParamValue(String name, out int value, out String strValue)
        {
            value = 0;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<int>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool GetCometParamValue(String name, out double value, out String strValue)
        {
            value = 0.0;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<double>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool GetCometParamValue(String name, out String value, out String strValue)
        {
            value = String.Empty;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<String>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool GetCometParamValue(String name, out VarMod value, out String strValue)
        {
            value = null;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<VarMod>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool GetCometParamValue(String name, out DoubleRange value, out String strValue)
        {
            value = null;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<DoubleRange>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool GetCometParamValue(String name, out IntRange value, out String strValue)
        {
            value = null;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<IntRange>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool GetCometParamValue(String name, out StringCollection value, out String strValue)
        {
            value = null;
            strValue = String.Empty;

            CometParam param;
            if (!CometParams.TryGetValue(name, out param))
            {
                return false;
            }

            var typedParam = param as TypedCometParam<StringCollection>;
            if (null == typedParam)
            {
                return false;
            }

            value = typedParam.Value;
            strValue = param.Value;
            return true;
        }

        public bool SetCometParam(String paramName, String strValue)
        {
            CometParam cometParam;
            if (!CometParams.TryGetValue(paramName, out cometParam))
            {
                return false;
            }

            CometParamType paramType = cometParam.Type;
            switch (paramType)
            {
                case CometParamType.Int:
                    cometParam = ParseCometIntParam(strValue);
                    break;
                case CometParamType.Double:
                    cometParam = ParseCometDoubleParam(strValue);
                    break;
                case CometParamType.String:
                    cometParam = new TypedCometParam<String>(paramType, strValue, strValue);
                    break;
                case CometParamType.IntRange:
                    cometParam = ParseCometIntRangeParam(strValue);
                    break;
                case CometParamType.DoubleRange:
                    cometParam = ParseCometDoubleRangeParam(strValue);
                    break;
                case CometParamType.VarMod:
                    cometParam = ParseCometVarModParam(strValue);
                    break;
                case CometParamType.StrCollection:
                    cometParam = ParseCometStringCollectionParam(strValue);
                    break;
                default:
                    return false;
            }

            if (null == cometParam)
            {
                return false;
            }

            CometParams[paramName] = cometParam;

            return true;
        }

        public static bool GetStaticModParamInfo(String aa, out String paramName, out String aaName)
        {
            paramName = String.Empty;
            aaName = String.Empty;
            switch (aa)
            {
                case "G":
                    paramName = "add_G_glycine";
                    aaName = "Glycine";
                    break;
                case "A":
                    paramName = "add_A_alanine";
                    aaName = "Alanine";
                    break;
                case "S":
                    paramName = "add_S_serine";
                    aaName = "Serine";
                    break;
                case "P":
                    paramName = "add_P_proline";
                    aaName = "Proline";
                    break;
                case "V":
                    paramName = "add_V_valine";
                    aaName = "Valine";
                    break;
                case "T":
                    paramName = "add_T_threonine";
                    aaName = "Threonine";
                    break;
                case "C":
                    paramName = "add_C_cysteine";
                    aaName = "Cysteine";
                    break;
                case "L":
                    paramName = "add_L_leucine";
                    aaName = "Leucine";
                    break;
                case "I":
                    paramName = "add_I_isoleucine";
                    aaName = "Isoleucine";
                    break;
                case "N":
                    paramName = "add_N_asparagine";
                    aaName = "Asparagine";
                    break;
                case "D":
                    paramName = "add_D_aspartic_acid";
                    aaName = "Aspartic Acid";
                    break;
                case "Q":
                    paramName = "add_Q_glutamine";
                    aaName = "Glutamine";
                    break;
                case "K":
                    paramName = "add_K_lysine";
                    aaName = "Lysine";
                    break;
                case "E":
                    paramName = "add_E_glutamic_acid";
                    aaName = "Glutamic Acid";
                    break;
                case "M":
                    paramName = "add_M_methionine";
                    aaName = "Methionine";
                    break;
                case "O":
                    paramName = "add_O_ornithine";
                    aaName = "Ornithine";
                    break;
                case "H":
                    paramName = "add_H_histidine";
                    aaName = "Histidine";
                    break;
                case "F":
                    paramName = "add_F_phenylalanine";
                    aaName = "Phenylalanine";
                    break;
                case "R":
                    paramName = "add_R_arginine";
                    aaName = "Arginine";
                    break;
                case "Y":
                    paramName = "add_Y_tyrosine";
                    aaName = "Tyrosine";
                    break;
                case "W":
                    paramName = "add_W_tryptophan";
                    aaName = "Tryptophan";
                    break;
                case "B":
                    paramName = "add_B_user_amino_acid";
                    aaName = "User Amino Acid";
                    break;
                case "J":
                    paramName = "add_J_user_amino_acid";
                    aaName = "User Amino Acid";
                    break;
                case "U":
                    paramName = "add_U_user_amino_acid";
                    aaName = "User Amino Acid";
                    break;
                case "X":
                    paramName = "add_X_user_amino_acid";
                    aaName = "User Amino Acid";
                    break;
                case "Z":
                    paramName = "add_Z_user_amino_acid";
                    aaName = "User Amino Acid";
                    break;
                default:
                    return false;
            }

            return true;
        }

        private bool UpdateCometParam(String paramName, CometParam newCometParam)
        {
            if (String.IsNullOrEmpty(paramName))
            {
                return false;
            }

            CometParam currentCometParam;
            if (CometParams.TryGetValue(paramName, out currentCometParam))
            {
                CometParams[paramName] = newCometParam;
            }
            else
            {
                CometParams.Add(paramName, newCometParam);
            }

            return true;
        }

        private bool AddVarModsToStrCollection(ref StringCollection varMods)
        {
            for (int modNum = 1; modNum <= MaxNumVarMods; modNum++)
            {
                string paramName = "variable_mod" + modNum;
                VarMod varMod;
                String paramValueStr;
                if (!GetCometParamValue(paramName, out varMod, out paramValueStr))
                {
                    return false;
                }

                varMods.Add(paramValueStr);
            }

            return true;
        }

        private bool AddStaticModsToStrCollection(ref StringCollection staticMods)
        {
            if (!AddStaticModToStrCollection("G", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("A", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("S", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("P", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("V", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("T", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("C", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("L", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("I", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("N", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("D", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("Q", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("K", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("E", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("M", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("O", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("H", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("F", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("R", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("Y", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("W", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("B", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("J", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("U", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("X", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("Z", ref staticMods))
            {
                return false;
            }

            return true;
        }

        private bool AddStaticModToStrCollection(String aa, ref StringCollection strCollection)
        {
            String paramName;
            String aaName;
            if (!GetStaticModParamInfo(aa, out paramName, out aaName))
            {
                return false;
            }

            String paramValueStr;
            double massDiff;
            if (!GetCometParamValue(paramName, out massDiff, out paramValueStr))
            {
                return false;
            }

            strCollection.Add(aaName + "," + aa + "," + massDiff);
            return true;
        }

        private CometParam ParseCometIntParam(String strValue)
        {
            int intValue;
            if (!int.TryParse(strValue, out intValue))
            {
                return null;
            }
            return new TypedCometParam<int>(CometParamType.Int, strValue, intValue);
        }

        private CometParam ParseCometDoubleParam(String strValue)
        {
            double doubleValue;
            if (!double.TryParse(strValue, out doubleValue))
            {
                return null;
            }
            return new TypedCometParam<double>(CometParamType.Double, strValue, doubleValue);
        }

        private CometParam ParseCometIntRangeParam(String strValue)
        {
            String[] intStrValues = strValue.Split(' ');
            if (intStrValues.Length < 2)
            {
                return null;
            }
            int minIntVal, maxIntVal;
            if (!int.TryParse(intStrValues[0], out minIntVal) || !int.TryParse(intStrValues[1], out maxIntVal))
            {
                return null;
            }
            return new TypedCometParam<IntRange>(CometParamType.IntRange, strValue, new IntRange(minIntVal, maxIntVal));
        }

        private CometParam ParseCometDoubleRangeParam(String strValue)
        {
            String[] doubleStrValues = strValue.Split(' ');
            if (doubleStrValues.Length < 2)
            {
                return null;
            }

            double minDoubleVal, maxDoubleVal;
            if (!double.TryParse(doubleStrValues[0], out minDoubleVal) || !double.TryParse(doubleStrValues[1], out maxDoubleVal))
            {
                return null;
            }
            
            return new TypedCometParam<DoubleRange>(CometParamType.DoubleRange, strValue, new DoubleRange(minDoubleVal, maxDoubleVal));
        }

        private CometParam ParseCometVarModParam(String strValue)
        {
            String[] varModStrValues = strValue.Split(' ');
            if (varModStrValues.Length < NumVarModFieldsInSettings)
            {
                return null;
            }

            double mass;
            if (!double.TryParse(varModStrValues[0], out mass))
            {
                return null;
            }

            var varModChar = varModStrValues[1];

            int binaryMod;
            if (!int.TryParse(varModStrValues[2], out binaryMod))
            {
                return null;
            }

            int maxMods;
            if (!int.TryParse(varModStrValues[3], out maxMods))
            {
                return null;
            }

            var newStrValue = strValue.Replace(' ', ',');

            return new TypedCometParam<VarMod>(CometParamType.VarMod, newStrValue, new VarMod(mass, varModChar, binaryMod, maxMods));
        }

        private CometParam ParseCometStringCollectionParam(String strValue)
        {
            var strCollectionParam = new StringCollection();
            String newStrValue = String.Empty;
            String modifiedStrValue = strValue.Replace(Environment.NewLine, "\n");
            String[] rows = modifiedStrValue.Split('\n');
            foreach (var row in rows)
            {
                if (String.Empty != row)
                {
                    var newRow = row.Replace('.', ' ');
                    char[] duplicateChars = {' '};
                    newRow = RemoveDuplicateChars(newRow, duplicateChars);
                    newRow = newRow.Replace(' ', ',');
                    strCollectionParam.Add(newRow);
                    newStrValue += newRow + Environment.NewLine;
                }
            }
            return new TypedCometParam<StringCollection>(CometParamType.StrCollection, newStrValue, strCollectionParam);
        }

        private string RemoveDuplicateChars(string src, char[] dupes)
        {
            return string.Join(" ", src.Split(dupes, StringSplitOptions.RemoveEmptyEntries));
        }

    }

    public class VarMod
    {
        public int BinaryMod { get; set; }
        public int MaxNumVarModAAPerMod { get; set; }
        public double VarModMass { get; set; }
        public String VarModChar { get; set; }

        public VarMod()
        {
            BinaryMod = 0;
            MaxNumVarModAAPerMod = 0;
            VarModMass = 0.0;
            VarModChar = String.Empty;
        }

        public VarMod(double varModMass, String varModChar, int binaryMod, int maxNumVarModPerMod)
        {
            VarModChar = varModChar;
            VarModMass = varModMass;
            BinaryMod = binaryMod;
            MaxNumVarModAAPerMod = maxNumVarModPerMod;
        }
    }

    public class IntRange
    {
        public int Start { get; set; }
        public int End { get; set; }

        public IntRange()
        {
            Start = 0;
            End = 0;
        }

        public IntRange(int start, int end)
        {
            Start = start;
            End = end;
        }
    }

    public class DoubleRange
    {
        public double Start { get; set; }
        public double End { get; set; }

        public DoubleRange()
        {
            Start = 0;
            End = 0;
        }

        public DoubleRange(double start, double end)
        {
            Start = start;
            End = end;
        }
    }
};
