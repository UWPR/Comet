using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Globalization;
using CometUI.Properties;

namespace CometUI
{
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
        public Dictionary<string, CometParam> CometParams { get; private set; }

        public CometParamsMap()
        {
            CometParams = new Dictionary<string, CometParam>();
            UpdateCometParamsFromSettings(Settings.Default);
        }

        public CometParamsMap(Settings settings)
        {
            CometParams = new Dictionary<string, CometParam>();
            UpdateCometParamsFromSettings(settings);
        }

        public bool GetSettingsFromCometParams(Settings cometSettings)
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

            int outputPinXMLFile;
            if (!GetCometParamValue("output_pinxmlfile", out outputPinXMLFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatPinXML = outputPinXMLFile == 1;

            int outputTextFile;
            if (!GetCometParamValue("output_txtfile", out outputTextFile, out paramValueStr))
            {
                return false;
            }
            cometSettings.OutputFormatTextFile = outputTextFile == 1;

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

        public void UpdateCometParamsFromSettings(Settings settings)
        {
            var dbName = settings.ProteomeDatabaseFile;
            UpdateCometParam("database_name", 
                             new TypedCometParam<string>(CometParamType.String,
                                                         dbName,
                                                         dbName));

            var searchType = settings.SearchType;
            UpdateCometParam("decoy_search",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      searchType.ToString(CultureInfo.InvariantCulture),
                                                      searchType));

            var decoyPrefix = settings.DecoyPrefix;
            UpdateCometParam("decoy_prefix",
                             new TypedCometParam<string>(CometParamType.String,
                                                         decoyPrefix,
                                                         decoyPrefix));

            var nucleotideReadingFrame = settings.NucleotideReadingFrame;
            UpdateCometParam("nucleotide_reading_frame",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture),
                                                      nucleotideReadingFrame));

            var outputPepXMLFile = settings.OutputFormatPepXML ? 1 : 0;
            UpdateCometParam("output_pepxmlfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputPepXMLFile.ToString(CultureInfo.InvariantCulture),
                                                      outputPepXMLFile));

            var outputPinXMLFile = settings.OutputFormatPinXML ? 1 : 0;
            UpdateCometParam("output_pinxmlfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputPinXMLFile.ToString(CultureInfo.InvariantCulture),
                                                      outputPinXMLFile));

            var outputTextFile = settings.OutputFormatTextFile ? 1 : 0;
            UpdateCometParam("output_txtfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputTextFile.ToString(CultureInfo.InvariantCulture),
                                                      outputTextFile));

            var outputSqtFile = settings.OutputFormatSqtFile ? 1 : 0;
            UpdateCometParam("output_sqtfile",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputSqtFile.ToString(CultureInfo.InvariantCulture),
                                                      outputSqtFile));

            var outputOutFile = settings.OutputFormatOutFiles ? 1 : 0;
            UpdateCometParam("output_outfiles",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      outputOutFile.ToString(CultureInfo.InvariantCulture),
                                                      outputOutFile));

            var printExpectScore = settings.PrintExpectScoreInPlaceOfSP ? 1 : 0;
            UpdateCometParam("print_expect_score",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      printExpectScore.ToString(CultureInfo.InvariantCulture),
                                                      printExpectScore));

            var showFragmentIons = settings.OutputFormatShowFragmentIons ? 1 : 0;
            UpdateCometParam("show_fragment_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      showFragmentIons.ToString(CultureInfo.InvariantCulture),
                                                      showFragmentIons));

            var skipResearching = settings.OutputFormatSkipReSearching ? 1 : 0;
            UpdateCometParam("skip_researching",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      skipResearching.ToString(CultureInfo.InvariantCulture),
                                                      skipResearching));

            var numOutputLines = settings.NumOutputLines;
            UpdateCometParam("num_output_lines",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      numOutputLines.ToString(CultureInfo.InvariantCulture),
                                                      numOutputLines));

            var searchEnzymeNumber = settings.SearchEnzymeNumber;
            UpdateCometParam("search_enzyme_number",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      searchEnzymeNumber.ToString(CultureInfo.InvariantCulture),
                                                      searchEnzymeNumber));

            var sampleEnzymeNumber = settings.SampleEnzymeNumber;
            UpdateCometParam("sample_enzyme_number",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      sampleEnzymeNumber.ToString(CultureInfo.InvariantCulture),
                                                      sampleEnzymeNumber));

            var allowedMissedCleavages = settings.AllowedMissedCleavages;
            UpdateCometParam("allowed_missed_cleavage",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      allowedMissedCleavages.ToString(CultureInfo.InvariantCulture),
                                                      allowedMissedCleavages));

            var enzymeTermini = settings.EnzymeTermini;
            UpdateCometParam("num_enzyme_termini",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      enzymeTermini.ToString(CultureInfo.InvariantCulture),
                                                      enzymeTermini));

            var digestMassRange = new DoubleRange(settings.digestMassRangeMin,
                                   settings.digestMassRangeMax);
            string digestMassRangeString = digestMassRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + digestMassRange.End.ToString(CultureInfo.InvariantCulture);
            UpdateCometParam("digest_mass_range",
                             new TypedCometParam<DoubleRange>(CometParamType.DoubleRange,
                                                              digestMassRangeString,
                                                              digestMassRange));

            var precursorMassTol = settings.PrecursorMassTolerance;
            UpdateCometParam("peptide_mass_tolerance",
                             new TypedCometParam<double>(CometParamType.Double,
                                                         precursorMassTol.ToString(CultureInfo.InvariantCulture),
                                                         precursorMassTol));

            var precursorMassUnit = settings.PrecursorMassUnit;
            UpdateCometParam("peptide_mass_units",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         precursorMassUnit.ToString(CultureInfo.InvariantCulture),
                                                         precursorMassUnit));

            var precursorMassType = settings.PrecursorMassType;
            UpdateCometParam("mass_type_parent",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         precursorMassType.ToString(CultureInfo.InvariantCulture),
                                                         precursorMassType));

            var precursorTolType = settings.PrecursorToleranceType;
            UpdateCometParam("precursor_tolerance_type",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         precursorTolType.ToString(CultureInfo.InvariantCulture),
                                                         precursorTolType));

            var isotopeError = settings.PrecursorIsotopeError;
            UpdateCometParam("isotope_error",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         isotopeError.ToString(CultureInfo.InvariantCulture),
                                                         isotopeError));

            var fragmentBinSize = settings.FragmentBinSize;
            UpdateCometParam("fragment_bin_tol",
                             new TypedCometParam<double>(CometParamType.Double,
                                                         fragmentBinSize.ToString(CultureInfo.InvariantCulture),
                                                         fragmentBinSize));

            var fragmentBinOffset = settings.FragmentBinOffset;
            UpdateCometParam("fragment_bin_offset",
                             new TypedCometParam<double>(CometParamType.Double,
                                                         fragmentBinOffset.ToString(CultureInfo.InvariantCulture),
                                                         fragmentBinOffset));

            var fragmentMassType = settings.FragmentMassType;
            UpdateCometParam("mass_type_fragment",
                             new TypedCometParam<int>(CometParamType.Int,
                                                         fragmentMassType.ToString(CultureInfo.InvariantCulture),
                                                         fragmentMassType));

            var useSparseMatrix = settings.UseSparseMatrix ? 1 : 0;
            UpdateCometParam("use_sparse_matrix",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useSparseMatrix.ToString(CultureInfo.InvariantCulture),
                                                      useSparseMatrix));

            var useAIons = settings.UseAIons ? 1 : 0;
            UpdateCometParam("use_A_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useAIons.ToString(CultureInfo.InvariantCulture),
                                                      useAIons));

            var useBIons = settings.UseBIons ? 1 : 0;
            UpdateCometParam("use_B_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useBIons.ToString(CultureInfo.InvariantCulture),
                                                      useBIons));

            var useCIons = settings.UseCIons ? 1 : 0;
            UpdateCometParam("use_C_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useCIons.ToString(CultureInfo.InvariantCulture),
                                                      useCIons));

            var useXIons = settings.UseXIons ? 1 : 0;
            UpdateCometParam("use_X_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useXIons.ToString(CultureInfo.InvariantCulture),
                                                      useXIons));

            var useYIons = settings.UseYIons ? 1 : 0;
            UpdateCometParam("use_Y_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useYIons.ToString(CultureInfo.InvariantCulture),
                                                      useYIons));

            var useZIons = settings.UseZIons ? 1 : 0;
            UpdateCometParam("use_Z_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useZIons.ToString(CultureInfo.InvariantCulture),
                                                      useZIons));

            var useFlankIons = settings.TheoreticalFragmentIons ? 1 : 0;
            UpdateCometParam("theoretical_fragment_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useFlankIons.ToString(CultureInfo.InvariantCulture),
                                                      useFlankIons));

            var useNLIons = settings.UseNLIons ? 1 : 0;
            UpdateCometParam("use_NL_ions",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      useNLIons.ToString(CultureInfo.InvariantCulture),
                                                      useNLIons));

            foreach (var item in settings.StaticMods)
            {
                string[] staticMods = item.Split(',');

                string paramName = GetStaticModParamName(staticMods[1]);
                double massDiff = Convert.ToDouble(staticMods[2]);
                UpdateCometParam(paramName,
                                 new TypedCometParam<double>(CometParamType.Double,
                                                             massDiff.ToString(CultureInfo.InvariantCulture),
                                                             massDiff));
            }

            var cTermPeptideMass = settings.StaticModCTermPeptide;
            UpdateCometParam("add_Cterm_peptide",
                new TypedCometParam<double>(CometParamType.Double,
                                            cTermPeptideMass.ToString(CultureInfo.InvariantCulture),
                                            cTermPeptideMass));

            var nTermPeptideMass = settings.StaticModNTermPeptide;
            UpdateCometParam("add_Nterm_peptide",
                new TypedCometParam<double>(CometParamType.Double,
                                            nTermPeptideMass.ToString(CultureInfo.InvariantCulture),
                                            nTermPeptideMass));

            var cTermProteinMass = settings.StaticModCTermProtein;
            UpdateCometParam("add_Cterm_protein",
                new TypedCometParam<double>(CometParamType.Double,
                                            cTermProteinMass.ToString(CultureInfo.InvariantCulture),
                                            cTermProteinMass));

            var nTermProteinMass = settings.StaticModNTermProtein;
            UpdateCometParam("add_Nterm_protein",
                new TypedCometParam<double>(CometParamType.Double,
                                            nTermProteinMass.ToString(CultureInfo.InvariantCulture),
                                            nTermProteinMass));

            int modNum = 0;
            foreach (var item in settings.VariableMods)
            {
                modNum++;
                string paramName = "variable_mod" + modNum;
                string[] varModsStr = item.Split(',');
                var varMods = new VarMod(varModsStr[0],
                                          Convert.ToDouble(varModsStr[1]),
                                          Convert.ToInt32(varModsStr[2]),
                                          Convert.ToInt32(varModsStr[3]));
                var varModsStrValue = varMods.VarModMass + " " + varMods.VarModChar + " " + varMods.BinaryMod + " " + varMods.MaxNumVarModAAPerMod;
                UpdateCometParam(paramName,
                                 new TypedCometParam<VarMod>(CometParamType.VarMod,
                                                             varModsStrValue,
                                                             varMods));

            }

            var varCTerminus = settings.VariableCTerminus;
            UpdateCometParam("variable_C_terminus",
                             new TypedCometParam<double>(CometParamType.Double,
                                            varCTerminus.ToString(CultureInfo.InvariantCulture),
                                            varCTerminus));

            var varCTerminusDist = settings.VariableCTermDistance;
            UpdateCometParam("variable_C_terminus_distance",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      varCTerminusDist.ToString(CultureInfo.InvariantCulture),
                                                      varCTerminusDist));

            var varNTerminus = settings.VariableNTerminus;
            UpdateCometParam("variable_N_terminus",
                             new TypedCometParam<double>(CometParamType.Double,
                                            varNTerminus.ToString(CultureInfo.InvariantCulture),
                                            varNTerminus));

            var varNTerminusDist = settings.VariableNTermDistance;
            UpdateCometParam("variable_N_terminus_distance",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     varNTerminusDist.ToString(CultureInfo.InvariantCulture),
                                                     varNTerminusDist));

            var maxVarModsInPeptide = settings.MaxVarModsInPeptide;
            UpdateCometParam("max_variable_mods_in_peptide",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture),
                                                      maxVarModsInPeptide));

            var mzxmlScanRange = new IntRange(settings.mzxmlScanRangeMin, settings.mzxmlScanRangeMax);
            string mzxmlScanRangeString = mzxmlScanRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlScanRange.End.ToString(CultureInfo.InvariantCulture);
            UpdateCometParam("scan_range",
                             new TypedCometParam<IntRange>(CometParamType.IntRange,
                                                           mzxmlScanRangeString,
                                                           mzxmlScanRange));

            var mzxmlPrecursorChargeRange = new IntRange(settings.mzxmlPrecursorChargeRangeMin, settings.mzxmlPrecursorChargeRangeMax);
            string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlPrecursorChargeRange.End.ToString(CultureInfo.InvariantCulture);
            UpdateCometParam("precursor_charge",
                             new TypedCometParam<IntRange>(CometParamType.IntRange,
                                                           mzxmlPrecursorChargeRageString,
                                                           mzxmlPrecursorChargeRange));

            var mzxmlMSLevel = settings.mzxmlMsLevel;
            UpdateCometParam("ms_level",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      mzxmlMSLevel.ToString(CultureInfo.InvariantCulture),
                                                      mzxmlMSLevel));

            var mzxmlActivationMethod = settings.mzxmlActivationMethod;
            UpdateCometParam("activation_method",
                             new TypedCometParam<string>(CometParamType.String,
                                                         mzxmlActivationMethod,
                                                         mzxmlActivationMethod));

            var minPeaks = settings.spectralProcessingMinPeaks;
            UpdateCometParam("minimum_peaks",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      minPeaks.ToString(CultureInfo.InvariantCulture),
                                                      minPeaks));

            var minIntensity = settings.spectralProcessingMinIntensity;
            UpdateCometParam("minimum_intensity",
                new TypedCometParam<double>(CometParamType.Double,
                                            minIntensity.ToString(CultureInfo.InvariantCulture),
                                            minIntensity));

            var removePrecursorTol = settings.spectralProcessingRemovePrecursorTol;
            UpdateCometParam("remove_precursor_tolerance",
                new TypedCometParam<double>(CometParamType.Double,
                                            removePrecursorTol.ToString(CultureInfo.InvariantCulture),
                                            removePrecursorTol));

            var removePrecursorPeak = settings.spectralProcessingRemovePrecursorPeak;
            UpdateCometParam("remove_precursor_peak",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      removePrecursorPeak.ToString(CultureInfo.InvariantCulture),
                                                      removePrecursorPeak));

            var clearMzRange = new DoubleRange(settings.spectralProcessingClearMzMin,
                                               settings.spectralProcessingClearMzMax);
            string clearMzRangeString = clearMzRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + clearMzRange.End.ToString(CultureInfo.InvariantCulture);
            UpdateCometParam("clear_mz_range",
                             new TypedCometParam<DoubleRange>(CometParamType.DoubleRange,
                                                              clearMzRangeString,
                                                              clearMzRange));

            var spectrumBatchSize = settings.SpectrumBatchSize;
            UpdateCometParam("spectrum_batch_size",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      spectrumBatchSize.ToString(CultureInfo.InvariantCulture),
                                                      spectrumBatchSize));

            var numThreads = settings.NumThreads;
            UpdateCometParam("num_threads",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      numThreads.ToString(CultureInfo.InvariantCulture),
                                                      numThreads));

            var numResults = settings.NumResults;
            UpdateCometParam("num_results",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      numResults.ToString(CultureInfo.InvariantCulture),
                                                      numResults));

            var maxFragmentCharge = settings.MaxFragmentCharge;
            UpdateCometParam("max_fragment_charge",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      maxFragmentCharge.ToString(CultureInfo.InvariantCulture),
                                                      maxFragmentCharge));

            var maxPrecursorCharge = settings.MaxPrecursorCharge;
            UpdateCometParam("max_precursor_charge",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      maxPrecursorCharge.ToString(CultureInfo.InvariantCulture),
                                                      maxPrecursorCharge));

            var clipNTermMethionine = settings.ClipNTermMethionine ? 1 : 0;
            UpdateCometParam("clip_nterm_methionine",
                             new TypedCometParam<int>(CometParamType.Int,
                                                      clipNTermMethionine.ToString(CultureInfo.InvariantCulture),
                                                      clipNTermMethionine));

            string enzymeInfoStr = String.Empty;
            foreach (var row in settings.EnzymeInfo)
            {
                enzymeInfoStr += row + Environment.NewLine;
            }

            UpdateCometParam("[COMET_ENZYME_INFO]",
                             new TypedCometParam<StringCollection>(CometParamType.StrCollection,
                                                                   enzymeInfoStr,
                                                                   settings.EnzymeInfo));
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

        public static String GetStaticModParamName(String aa)
        {
            String paramName = String.Empty;
            switch (aa)
            {
                case "G":
                    paramName = "add_G_glycine";
                    break;
                case "A":
                    paramName = "add_A_alanine";
                    break;
                case "S":
                    paramName = "add_S_serine";
                    break;
                case "P":
                    paramName = "add_P_proline";
                    break;
                case "V":
                    paramName = "add_V_valine";
                    break;
                case "T":
                    paramName = "add_T_threonine";
                    break;
                case "C":
                    paramName = "add_C_cysteine";
                    break;
                case "L":
                    paramName = "add_L_leucine";
                    break;
                case "I":
                    paramName = "add_I_isoleucine";
                    break;
                case "N":
                    paramName = "add_N_asparagine";
                    break;
                case "D":
                    paramName = "add_D_aspartic_acid";
                    break;
                case "Q":
                    paramName = "add_Q_glutamine";
                    break;
                case "K":
                    paramName = "add_K_lysine";
                    break;
                case "E":
                    paramName = "add_E_glutamic_acid";
                    break;
                case "M":
                    paramName = "add_M_methionine";
                    break;
                case "O":
                    paramName = "add_O_ornithine";
                    break;
                case "H":
                    paramName = "add_H_histidine";
                    break;
                case "F":
                    paramName = "add_F_phenylalanine";
                    break;
                case "R":
                    paramName = "add_R_arginine";
                    break;
                case "Y":
                    paramName = "add_Y_tyrosine";
                    break;
                case "W":
                    paramName = "add_W_tryptophan";
                    break;
                case "B":
                    paramName = "add_B_user_amino_acid";
                    break;
                case "J":
                    paramName = "add_J_user_amino_acid";
                    break;
                case "U":
                    paramName = "add_U_user_amino_acid";
                    break;
                case "X":
                    paramName = "add_X_user_amino_acid";
                    break;
                case "Z":
                    paramName = "add_Z_user_amino_acid";
                    break;
            }

            return paramName;
        }

        private void UpdateCometParam(String paramName, CometParam newCometParam)
        {
            CometParam currentCometParam;
            if (CometParams.TryGetValue(paramName, out currentCometParam))
            {
                CometParams[paramName] = newCometParam;
            }
            else
            {
                CometParams.Add(paramName, newCometParam);
            }
        }

        private bool AddVarModsToStrCollection(ref StringCollection varMods)
        {
            for (int modNum = 0; modNum < MaxNumVarMods; modNum++)
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
            if (!AddStaticModToStrCollection("add_G_glycine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_A_alanine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_S_serine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_P_proline", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_V_valine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_T_threonine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_C_cysteine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_L_leucine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_I_isoleucine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_N_asparagine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_D_aspartic_acid", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_Q_glutamine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_K_lysine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_E_glutamic_acid", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_M_methionine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_O_ornithine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_H_histidine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_F_phenylalanine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_R_arginine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_Y_tyrosine", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_W_tryptophan", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_B_user_amino_acid", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_J_user_amino_acid", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_U_user_amino_acid", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_X_user_amino_acid", ref staticMods))
            {
                return false;
            }

            if (!AddStaticModToStrCollection("add_Z_user_amino_acid", ref staticMods))
            {
                return false;
            }

            return false;
        }

        private bool AddStaticModToStrCollection(String modName, ref StringCollection strCollection)
        {
            String paramValueStr;
            double massDiff;
            if (!GetCometParamValue(modName, out massDiff, out paramValueStr))
            {
                return false;
            }
            strCollection.Add(modName + "," + massDiff);
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
            if (varModStrValues.Length < 4)
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
            if (!int.TryParse(varModStrValues[2], out maxMods))
            {
                return null;
            }

            var newStrValue = strValue.Replace(' ', ',');

            return new TypedCometParam<VarMod>(CometParamType.VarMod, newStrValue, new VarMod(varModChar, mass, binaryMod, maxMods));
        }

        private CometParam ParseCometStringCollectionParam(String strValue)
        {
            var strCollectionParam = new StringCollection();
            String newStrValue = String.Empty;
            String modifiedStrValue = strValue.Replace(Environment.NewLine, "\n");
            String[] rows = modifiedStrValue.Split('\n');
            foreach (var row in rows)
            {
                var newRow = row.Replace('.', ' ');
                char[] duplicateChars = { ' ' };
                newRow = RemoveDuplicateChars(newRow, duplicateChars);
                newRow = newRow.Replace(' ', ',');
                strCollectionParam.Add(newRow);
                newStrValue += newRow + Environment.NewLine;
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

        public VarMod(String varModChar, double varModMass, int binaryMod, int maxNumVarModPerMod)
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
