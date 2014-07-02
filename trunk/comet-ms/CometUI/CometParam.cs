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
        VarMods,
        DoubleRange,
        IntRange,
        StrCollection
    }

    public class CometParamsMap
    {
        public Dictionary<string, CometParam> CometParams { get; private set; }

        private Settings _cometSettings;
        //public Settings CometSettings
        //{
        //    get
        //    {
        //        if (UpdateSettings)
        //        {
        //            if (!UpdateSettingsFromCometParams())
        //            {
        //                // Need to do something here: Show error, pass error on somewhere?
        //            }
        //            else
        //            {
        //                UpdateSettings = false;
        //            }
        //        }

        //        return _cometSettings;
        //    } 
        //}

        //private bool UpdateSettings { get; set; }

        public CometParamsMap()
        {
            CometParams = new Dictionary<string, CometParam>();
        }

        public CometParamsMap(Settings settings)
        {
            CometParams = new Dictionary<string, CometParam>();
            UpdateCometParamsFromSettings(settings);
        }

        //private bool UpdateSettingsFromCometParams()
        //{
        //    String dbName;
        //    if (!GetCometParamValue("database_name", out dbName))
        //    {
        //        return false;
        //    }
        //    _cometSettings.ProteomeDatabaseFile = dbName;

        //    int searchType;
        //    if (!GetCometParamValue("decoy_search", out searchType))
        //    {
        //        return false;
        //    }

        //    _cometSettings.SearchType = searchType;

        //    String decoyPrefix;
        //    if (!GetCometParamValue("decoy_prefix", out decoyPrefix))
        //    {
        //        return false;
        //    }
        //    _cometSettings.DecoyPrefix = decoyPrefix;

        //    int nucleotideReadingFrame;
        //    if (!GetCometParamValue("nucleotide_reading_frame", out nucleotideReadingFrame))
        //    {
        //        return false;
        //    }
        //    _cometSettings.NucleotideReadingFrame = nucleotideReadingFrame;

        //    int outputPepXMLFile;
        //    if (!GetCometParamValue("output_pepxmlfile", out outputPepXMLFile))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatPepXML = outputPepXMLFile == 1;

        //    int outputPinXMLFile;
        //    if (!GetCometParamValue("output_pinxmlfile", out outputPinXMLFile))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatPinXML = outputPinXMLFile == 1;

        //    int outputTextFile;
        //    if (!GetCometParamValue("output_txtfile", out outputTextFile))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatTextFile = outputTextFile == 1;

        //    int outputSqtFile;
        //    if (!GetCometParamValue("output_sqtfile", out outputSqtFile))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatSqtFile = outputSqtFile == 1;

        //    int outputOutFile;
        //    if (!GetCometParamValue("output_outfiles", out outputOutFile))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatOutFiles = outputOutFile == 1;

        //    int printExpectScore;
        //    if (!GetCometParamValue("print_expect_score", out printExpectScore))
        //    {
        //        return false;
        //    }
        //    _cometSettings.PrintExpectScoreInPlaceOfSP = printExpectScore == 1;

        //    int showFragmentIons;
        //    if (!GetCometParamValue("show_fragment_ions", out showFragmentIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatShowFragmentIons = showFragmentIons == 1;

        //    int skipResearching;
        //    if (!GetCometParamValue("skip_researching", out skipResearching))
        //    {
        //        return false;
        //    }
        //    _cometSettings.OutputFormatSkipReSearching = skipResearching == 1;

        //    int numOutputLines;
        //    if (!GetCometParamValue("num_output_lines", out numOutputLines))
        //    {
        //        return false;
        //    }
        //    _cometSettings.NumOutputLines = numOutputLines;

        //    int searchEnzymeNumber;
        //    if (!GetCometParamValue("search_enzyme_number", out searchEnzymeNumber))
        //    {
        //        return false;
        //    }
        //    _cometSettings.SearchEnzymeNumber = searchEnzymeNumber;

        //    int sampleEnzymeNumber;
        //    if (!GetCometParamValue("sample_enzyme_number", out sampleEnzymeNumber))
        //    {
        //        return false;
        //    }
        //    _cometSettings.SampleEnzymeNumber = sampleEnzymeNumber;

        //    int allowedMissedCleavages;
        //    if (!GetCometParamValue("allowed_missed_cleavage", out allowedMissedCleavages))
        //    {
        //        return false;
        //    }
        //    _cometSettings.AllowedMissedCleavages = allowedMissedCleavages;

        //    int enzymeTermini;
        //    if (!GetCometParamValue("num_enzyme_termini", out enzymeTermini))
        //    {
        //        return false;
        //    }
        //    _cometSettings.EnzymeTermini = enzymeTermini;

        //    double precursorMassTol;
        //    if (!GetCometParamValue("peptide_mass_tolerance", out precursorMassTol))
        //    {
        //        return false;
        //    }
        //    _cometSettings.PrecursorMassTolerance = precursorMassTol;

        //    String precursorMassUnit;
        //    if (!GetCometParamValue("peptide_mass_units", out precursorMassUnit))
        //    {
        //        return false;
        //    }
        //    _cometSettings.PrecursorMassUnit = precursorMassUnit;

        //    String precursorMassType;
        //    if (!GetCometParamValue("mass_type_parent", out precursorMassType))
        //    {
        //        return false;
        //    }
        //    _cometSettings.PrecursorMassType = precursorMassType;

        //    String precursorTolType;
        //    if (!GetCometParamValue("precursor_tolerance_type", out precursorTolType))
        //    {
        //        return false;
        //    }
        //    _cometSettings.PrecursorToleranceType = precursorTolType;

        //    String isotopeError;
        //    if (!GetCometParamValue("isotope_error", out isotopeError))
        //    {
        //        return false;
        //    }
        //    _cometSettings.PrecursorIsotopeError = isotopeError;

        //    double fragmentBinSize;
        //    if (!GetCometParamValue("fragment_bin_tol", out fragmentBinSize))
        //    {
        //        return false;
        //    }
        //    _cometSettings.FragmentBinSize = fragmentBinSize;

        //    double fragmentBinOffset;
        //    if (!GetCometParamValue("fragment_bin_offset", out fragmentBinOffset))
        //    {
        //        return false;
        //    }
        //    _cometSettings.FragmentBinOffset = fragmentBinOffset;

        //    String fragmentMassType;
        //    if (!GetCometParamValue("mass_type_fragment", out fragmentMassType))
        //    {
        //        return false;
        //    }
        //    _cometSettings.FragmentMassType = fragmentMassType;

        //    int useSparseMatrix;
        //    if (!GetCometParamValue("use_sparse_matrix", out useSparseMatrix))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseSparseMatrix = useSparseMatrix == 1;

        //    int useAIons;
        //    if (!GetCometParamValue("use_A_ions", out useAIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseAIons = useAIons == 1;

        //    int useBIons;
        //    if (!GetCometParamValue("use_B_ions", out useBIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseBIons = useBIons == 1;

        //    int useCIons;
        //    if (!GetCometParamValue("use_C_ions", out useCIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseCIons = useCIons == 1;

        //    int useXIons;
        //    if (!GetCometParamValue("use_X_ions", out useXIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseXIons = useXIons == 1;

        //    int useYIons;
        //    if (!GetCometParamValue("use_Y_ions", out useYIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseYIons = useYIons == 1;

        //    int useZIons;
        //    if (!GetCometParamValue("use_Z_ions", out useZIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseZIons = useZIons == 1;

        //    int useFlankIons;
        //    if (!GetCometParamValue("theoretical_fragment_ions", out useFlankIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.TheoreticalFragmentIons = useFlankIons == 1;

        //    int useNLIons;
        //    if (!GetCometParamValue("use_NL_ions", out useNLIons))
        //    {
        //        return false;
        //    }
        //    _cometSettings.UseNLIons = useNLIons == 1;


        //    //foreach (var item in settings.StaticMods)
        //    //{
        //    //    string[] staticMods = item.Split(',');

        //    //    string paramName = GetStaticModParamName(staticMods[1]);
        //    //    double massDiff = Convert.ToDouble(staticMods[2]);
        //    //    CometParams.Add(paramName,
        //    //                    new TypedCometParam<double>(CometParamType.Double,
        //    //                                                massDiff.ToString(CultureInfo.InvariantCulture),
        //    //                                                massDiff));
        //    //}

        //    double cTermPeptideMass;
        //    if (!GetCometParamValue("add_Cterm_peptide", out cTermPeptideMass))
        //    {
        //        return false;
        //    }
        //    _cometSettings.StaticModCTermPeptide = cTermPeptideMass;

        //    double nTermPeptideMass;
        //    if (!GetCometParamValue("add_Nterm_peptide", out nTermPeptideMass))
        //    {
        //        return false;
        //    }
        //    _cometSettings.StaticModNTermPeptide = nTermPeptideMass;

        //    double cTermProteinMass;
        //    if (!GetCometParamValue("add_Cterm_protein", out cTermProteinMass))
        //    {
        //        return false;
        //    }
        //    _cometSettings.StaticModCTermProtein = cTermProteinMass;

        //    double nTermProteinMass;
        //    if (!GetCometParamValue("add_Nterm_protein", out nTermProteinMass))
        //    {
        //        return false;
        //    }
        //    _cometSettings.StaticModNTermProtein = nTermProteinMass;

        //    StringCollection varModsStrCollection = new StringCollection();
        //    for (int modNum = 0; modNum < NumVarMods; modNum++)
        //    {
        //        string paramName = "variable_mod" + modNum;
        //        VarMod varMod;
        //        if (!GetCometParamValue(paramName, out varMod))
        //        {
        //            return false;
        //        }


        //    }

        //        foreach (var item in settings.VariableMods)
        //        {
        //            modNum++;
        //            string paramName = "variable_mod" + modNum;
        //            string[] varModsStr = item.Split(',');
        //            var varMods = new VarMod(varModsStr[0],
        //                                      Convert.ToDouble(varModsStr[1]),
        //                                      Convert.ToInt32(varModsStr[2]),
        //                                      Convert.ToInt32(varModsStr[3]));
        //            var varModsStrValue = varMods.VarModMass + " " + varMods.VarModChar + " " + varMods.BinaryMod + " " + varMods.MaxNumVarModAAPerMod;
        //            CometParams.Add(paramName,
        //                            new TypedCometParam<VarMod>(CometParamType.VarMods,
        //                                                         varModsStrValue,
        //                                                         varMods));

        //        }

        //    var varCTerminus = settings.VariableCTerminus;
        //    CometParams.Add("variable_C_terminus",
        //        new TypedCometParam<double>(CometParamType.Double,
        //                                    varCTerminus.ToString(CultureInfo.InvariantCulture),
        //                                    varCTerminus));

        //    var varCTerminusDist = settings.VariableCTermDistance;
        //    CometParams.Add("variable_C_terminus_distance",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             varCTerminusDist.ToString(CultureInfo.InvariantCulture),
        //                                             varCTerminusDist));

        //    var varNTerminus = settings.VariableNTerminus;
        //    CometParams.Add("variable_N_terminus",
        //        new TypedCometParam<double>(CometParamType.Double,
        //                                    varNTerminus.ToString(CultureInfo.InvariantCulture),
        //                                    varNTerminus));

        //    var varNTerminusDist = settings.VariableNTermDistance;
        //    CometParams.Add("variable_N_terminus_distance",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             varNTerminusDist.ToString(CultureInfo.InvariantCulture),
        //                                             varNTerminusDist));

        //    var maxVarModsInPeptide = settings.MaxVarModsInPeptide;
        //    CometParams.Add("max_variable_mods_in_peptide",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture),
        //                                             maxVarModsInPeptide));

        //    var mzxmlScanRange = new IntRange(settings.mzxmlScanRangeMin, settings.mzxmlScanRangeMax);
        //    string mzxmlScanRangeString = mzxmlScanRange.Start.ToString(CultureInfo.InvariantCulture)
        //                                  + " " + mzxmlScanRange.End.ToString(CultureInfo.InvariantCulture);
        //    CometParams.Add("scan_range",
        //                    new TypedCometParam<IntRange>(CometParamType.IntRange,
        //                                                  mzxmlScanRangeString,
        //                                                  mzxmlScanRange));

        //    var mzxmlPrecursorChargeRange = new IntRange(settings.mzxmlPrecursorChargeRangeMin, settings.mzxmlPrecursorChargeRangeMax);
        //    string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeRange.Start.ToString(CultureInfo.InvariantCulture)
        //                                  + " " + mzxmlPrecursorChargeRange.End.ToString(CultureInfo.InvariantCulture);
        //    CometParams.Add("precursor_charge",
        //                    new TypedCometParam<IntRange>(CometParamType.IntRange,
        //                                                  mzxmlPrecursorChargeRageString,
        //                                                  mzxmlPrecursorChargeRange));

        //    var mzxmlMSLevel = settings.mzxmlMsLevel;
        //    CometParams.Add("ms_level",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             mzxmlMSLevel.ToString(CultureInfo.InvariantCulture),
        //                                             mzxmlMSLevel));

        //    var mzxmlActivationMethod = settings.mzxmlActivationMethod;
        //    CometParams.Add("activation_method",
        //                    new TypedCometParam<string>(CometParamType.String,
        //                                                mzxmlActivationMethod,
        //                                                mzxmlActivationMethod));

        //    var minPeaks = settings.spectralProcessingMinPeaks;
        //    CometParams.Add("minimum_peaks",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             minPeaks.ToString(CultureInfo.InvariantCulture),
        //                                             minPeaks));

        //    var minIntensity = settings.spectralProcessingMinIntensity;
        //    CometParams.Add("minimum_intensity",
        //        new TypedCometParam<double>(CometParamType.Double,
        //                                    minIntensity.ToString(CultureInfo.InvariantCulture),
        //                                    minIntensity));

        //    var removePrecursorTol = settings.spectralProcessingRemovePrecursorTol;
        //    CometParams.Add("remove_precursor_tolerance",
        //        new TypedCometParam<double>(CometParamType.Double,
        //                                    removePrecursorTol.ToString(CultureInfo.InvariantCulture),
        //                                    removePrecursorTol));

        //    var removePrecursorPeak = settings.spectralProcessingRemovePrecursorPeak;
        //    CometParams.Add("remove_precursor_peak",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             removePrecursorPeak.ToString(CultureInfo.InvariantCulture),
        //                                             removePrecursorPeak));

        //    var clearMzRange = new DoubleRange(settings.spectralProcessingClearMzMin,
        //                                       settings.spectralProcessingClearMzMax);
        //    string clearMzRangeString = clearMzRange.Start.ToString(CultureInfo.InvariantCulture)
        //                                  + " " + clearMzRange.End.ToString(CultureInfo.InvariantCulture);
        //    CometParams.Add("clear_mz_range",
        //                    new TypedCometParam<DoubleRange>(CometParamType.DoubleRange,
        //                                                     clearMzRangeString,
        //                                                     clearMzRange));

        //    var spectrumBatchSize = settings.SpectrumBatchSize;
        //    CometParams.Add("spectrum_batch_size",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             spectrumBatchSize.ToString(CultureInfo.InvariantCulture),
        //                                             spectrumBatchSize));

        //    var numThreads = settings.NumThreads;
        //    CometParams.Add("num_threads",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             numThreads.ToString(CultureInfo.InvariantCulture),
        //                                             numThreads));

        //    var numResults = settings.NumResults;
        //    CometParams.Add("num_results",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             numResults.ToString(CultureInfo.InvariantCulture),
        //                                             numResults));

        //    var maxFragmentCharge = settings.MaxFragmentCharge;
        //    CometParams.Add("max_fragment_charge",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             maxFragmentCharge.ToString(CultureInfo.InvariantCulture),
        //                                             maxFragmentCharge));

        //    var maxPrecursorCharge = settings.MaxPrecursorCharge;
        //    CometParams.Add("max_precursor_charge",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             maxPrecursorCharge.ToString(CultureInfo.InvariantCulture),
        //                                             maxPrecursorCharge));

        //    var clipNTermMethionine = settings.ClipNTermMethionine ? 1 : 0;
        //    CometParams.Add("clip_nterm_methionine",
        //                    new TypedCometParam<int>(CometParamType.Int,
        //                                             clipNTermMethionine.ToString(CultureInfo.InvariantCulture),
        //                                             clipNTermMethionine));

        //    string enzymeInfoStr = String.Empty;
        //    foreach (var row in settings.EnzymeInfo)
        //    {
        //        enzymeInfoStr += row + Environment.NewLine;
        //    }

        //    CometParams.Add("[COMET_ENZYME_INFO]",
        //                    new TypedCometParam<StringCollection>(CometParamType.StrCollection,
        //                                                          enzymeInfoStr,
        //                                                          settings.EnzymeInfo));
        //}

        public void UpdateCometParamsFromSettings(Settings settings)
        {
            if (CometParams.Count > 0)
            {
                CometParams.Clear();
            }

            var dbName = settings.ProteomeDatabaseFile;
            CometParams.Add("database_name",
                            new TypedCometParam<string>(CometParamType.String,
                                                        dbName,
                                                        dbName));

            var searchType = settings.SearchType;
            CometParams.Add("decoy_search",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     searchType.ToString(CultureInfo.InvariantCulture),
                                                     searchType));

            var decoyPrefix = settings.DecoyPrefix;
            CometParams.Add("decoy_prefix",
                            new TypedCometParam<string>(CometParamType.String,
                                                        decoyPrefix,
                                                        decoyPrefix));

            var nucleotideReadingFrame = settings.NucleotideReadingFrame;
            CometParams.Add("nucleotide_reading_frame",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     nucleotideReadingFrame.ToString(CultureInfo.InvariantCulture),
                                                     nucleotideReadingFrame));

            var outputPepXMLFile = settings.OutputFormatPepXML ? 1 : 0;
            CometParams.Add("output_pepxmlfile",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     outputPepXMLFile.ToString(CultureInfo.InvariantCulture),
                                                     outputPepXMLFile));

            var outputPinXMLFile = settings.OutputFormatPinXML ? 1 : 0;
            CometParams.Add("output_pinxmlfile",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     outputPinXMLFile.ToString(CultureInfo.InvariantCulture),
                                                     outputPinXMLFile));

            var outputTextFile = settings.OutputFormatTextFile ? 1 : 0;
            CometParams.Add("output_txtfile",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     outputTextFile.ToString(CultureInfo.InvariantCulture),
                                                     outputTextFile));

            var outputSqtFile = settings.OutputFormatSqtFile ? 1 : 0;
            CometParams.Add("output_sqtfile",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     outputSqtFile.ToString(CultureInfo.InvariantCulture),
                                                     outputSqtFile));

            var outputOutFile = settings.OutputFormatOutFiles ? 1 : 0;
            CometParams.Add("output_outfiles",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     outputOutFile.ToString(CultureInfo.InvariantCulture),
                                                     outputOutFile));

            var printExpectScore = settings.PrintExpectScoreInPlaceOfSP ? 1 : 0;
            CometParams.Add("print_expect_score",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     printExpectScore.ToString(CultureInfo.InvariantCulture),
                                                     printExpectScore));

            var showFragmentIons = settings.OutputFormatShowFragmentIons ? 1 : 0;
            CometParams.Add("show_fragment_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     showFragmentIons.ToString(CultureInfo.InvariantCulture),
                                                     showFragmentIons));

            var skipResearching = settings.OutputFormatSkipReSearching ? 1 : 0;
            CometParams.Add("skip_researching",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     skipResearching.ToString(CultureInfo.InvariantCulture),
                                                     skipResearching));

            var numOutputLines = settings.NumOutputLines;
            CometParams.Add("num_output_lines",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     numOutputLines.ToString(CultureInfo.InvariantCulture),
                                                     numOutputLines));

            var searchEnzymeNumber = settings.SearchEnzymeNumber;
            CometParams.Add("search_enzyme_number",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     searchEnzymeNumber.ToString(CultureInfo.InvariantCulture),
                                                     searchEnzymeNumber));

            var sampleEnzymeNumber = settings.SampleEnzymeNumber;
            CometParams.Add("sample_enzyme_number",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     sampleEnzymeNumber.ToString(CultureInfo.InvariantCulture),
                                                     sampleEnzymeNumber));

            var allowedMissedCleavages = settings.AllowedMissedCleavages;
            CometParams.Add("allowed_missed_cleavage",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     allowedMissedCleavages.ToString(CultureInfo.InvariantCulture),
                                                     allowedMissedCleavages));

            var enzymeTermini = settings.EnzymeTermini;
            CometParams.Add("num_enzyme_termini",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     enzymeTermini.ToString(CultureInfo.InvariantCulture),
                                                     enzymeTermini));

            var precursorMassTol = settings.PrecursorMassTolerance;
            CometParams.Add("peptide_mass_tolerance",
                            new TypedCometParam<double>(CometParamType.Double,
                                                        precursorMassTol.ToString(CultureInfo.InvariantCulture),
                                                        precursorMassTol));

            var precursorMassUnit = settings.PrecursorMassUnit;
            CometParams.Add("peptide_mass_units",
                            new TypedCometParam<int>(CometParamType.String,
                                                        precursorMassUnit.ToString(CultureInfo.InvariantCulture),
                                                        precursorMassUnit));

            var precursorMassType = settings.PrecursorMassType;
            CometParams.Add("mass_type_parent",
                            new TypedCometParam<int>(CometParamType.String,
                                                        precursorMassType.ToString(CultureInfo.InvariantCulture),
                                                        precursorMassType));

            var precursorTolType = settings.PrecursorToleranceType;
            CometParams.Add("precursor_tolerance_type",
                            new TypedCometParam<int>(CometParamType.String,
                                                        precursorTolType.ToString(CultureInfo.InvariantCulture),
                                                        precursorTolType));

            var isotopeError = settings.PrecursorIsotopeError;
            CometParams.Add("isotope_error",
                            new TypedCometParam<int>(CometParamType.String,
                                                        isotopeError.ToString(CultureInfo.InvariantCulture),
                                                        isotopeError));

            var fragmentBinSize = settings.FragmentBinSize;
            CometParams.Add("fragment_bin_tol",
                            new TypedCometParam<double>(CometParamType.Double,
                                                        fragmentBinSize.ToString(CultureInfo.InvariantCulture),
                                                        fragmentBinSize));

            var fragmentBinOffset = settings.FragmentBinOffset;
            CometParams.Add("fragment_bin_offset",
                            new TypedCometParam<double>(CometParamType.Double,
                                                        fragmentBinOffset.ToString(CultureInfo.InvariantCulture),
                                                        fragmentBinOffset));

            var fragmentMassType = settings.FragmentMassType;
            CometParams.Add("mass_type_fragment",
                            new TypedCometParam<string>(CometParamType.String,
                                                        fragmentMassType,
                                                        fragmentMassType));

            var useSparseMatrix = settings.UseSparseMatrix ? 1 : 0;
            CometParams.Add("use_sparse_matrix",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useSparseMatrix.ToString(CultureInfo.InvariantCulture),
                                                     useSparseMatrix));

            var useAIons = settings.UseAIons ? 1 : 0;
            CometParams.Add("use_A_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useAIons.ToString(CultureInfo.InvariantCulture),
                                                     useAIons));

            var useBIons = settings.UseBIons ? 1 : 0;
            CometParams.Add("use_B_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useBIons.ToString(CultureInfo.InvariantCulture),
                                                     useBIons));

            var useCIons = settings.UseCIons ? 1 : 0;
            CometParams.Add("use_C_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useCIons.ToString(CultureInfo.InvariantCulture),
                                                     useCIons));

            var useXIons = settings.UseXIons ? 1 : 0;
            CometParams.Add("use_X_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useXIons.ToString(CultureInfo.InvariantCulture),
                                                     useXIons));

            var useYIons = settings.UseYIons ? 1 : 0;
            CometParams.Add("use_Y_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useYIons.ToString(CultureInfo.InvariantCulture),
                                                     useYIons));

            var useZIons = settings.UseZIons ? 1 : 0;
            CometParams.Add("use_Z_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useZIons.ToString(CultureInfo.InvariantCulture),
                                                     useZIons));

            var useFlankIons = settings.TheoreticalFragmentIons ? 1 : 0;
            CometParams.Add("theoretical_fragment_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useFlankIons.ToString(CultureInfo.InvariantCulture),
                                                     useFlankIons));

            var useNLIons = settings.UseNLIons ? 1 : 0;
            CometParams.Add("use_NL_ions",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     useNLIons.ToString(CultureInfo.InvariantCulture),
                                                     useNLIons));

            foreach (var item in settings.StaticMods)
            {
                string[] staticMods = item.Split(',');

                string paramName = GetStaticModParamName(staticMods[1]);
                double massDiff = Convert.ToDouble(staticMods[2]);
                CometParams.Add(paramName,
                                new TypedCometParam<double>(CometParamType.Double,
                                                            massDiff.ToString(CultureInfo.InvariantCulture),
                                                            massDiff));
            }

            var cTermPeptideMass = settings.StaticModCTermPeptide;
            CometParams.Add("add_Cterm_peptide",
                new TypedCometParam<double>(CometParamType.Double,
                                            cTermPeptideMass.ToString(CultureInfo.InvariantCulture),
                                            cTermPeptideMass));

            var nTermPeptideMass = settings.StaticModNTermPeptide;
            CometParams.Add("add_Nterm_peptide",
                new TypedCometParam<double>(CometParamType.Double,
                                            nTermPeptideMass.ToString(CultureInfo.InvariantCulture),
                                            nTermPeptideMass));

            var cTermProteinMass = settings.StaticModCTermProtein;
            CometParams.Add("add_Cterm_protein",
                new TypedCometParam<double>(CometParamType.Double,
                                            cTermProteinMass.ToString(CultureInfo.InvariantCulture),
                                            cTermProteinMass));

            var nTermProteinMass = settings.StaticModNTermProtein;
            CometParams.Add("add_Nterm_protein",
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
                CometParams.Add(paramName,
                                new TypedCometParam<VarMod>(CometParamType.VarMods,
                                                             varModsStrValue,
                                                             varMods));

            }

            var varCTerminus = settings.VariableCTerminus;
            CometParams.Add("variable_C_terminus",
                new TypedCometParam<double>(CometParamType.Double,
                                            varCTerminus.ToString(CultureInfo.InvariantCulture),
                                            varCTerminus));

            var varCTerminusDist = settings.VariableCTermDistance;
            CometParams.Add("variable_C_terminus_distance",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     varCTerminusDist.ToString(CultureInfo.InvariantCulture),
                                                     varCTerminusDist));

            var varNTerminus = settings.VariableNTerminus;
            CometParams.Add("variable_N_terminus",
                new TypedCometParam<double>(CometParamType.Double,
                                            varNTerminus.ToString(CultureInfo.InvariantCulture),
                                            varNTerminus));

            var varNTerminusDist = settings.VariableNTermDistance;
            CometParams.Add("variable_N_terminus_distance",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     varNTerminusDist.ToString(CultureInfo.InvariantCulture),
                                                     varNTerminusDist));

            var maxVarModsInPeptide = settings.MaxVarModsInPeptide;
            CometParams.Add("max_variable_mods_in_peptide",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     maxVarModsInPeptide.ToString(CultureInfo.InvariantCulture),
                                                     maxVarModsInPeptide));

            var mzxmlScanRange = new IntRange(settings.mzxmlScanRangeMin, settings.mzxmlScanRangeMax);
            string mzxmlScanRangeString = mzxmlScanRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlScanRange.End.ToString(CultureInfo.InvariantCulture);
            CometParams.Add("scan_range",
                            new TypedCometParam<IntRange>(CometParamType.IntRange,
                                                          mzxmlScanRangeString,
                                                          mzxmlScanRange));

            var mzxmlPrecursorChargeRange = new IntRange(settings.mzxmlPrecursorChargeRangeMin, settings.mzxmlPrecursorChargeRangeMax);
            string mzxmlPrecursorChargeRageString = mzxmlPrecursorChargeRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + mzxmlPrecursorChargeRange.End.ToString(CultureInfo.InvariantCulture);
            CometParams.Add("precursor_charge",
                            new TypedCometParam<IntRange>(CometParamType.IntRange,
                                                          mzxmlPrecursorChargeRageString,
                                                          mzxmlPrecursorChargeRange));

            var mzxmlMSLevel = settings.mzxmlMsLevel;
            CometParams.Add("ms_level",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     mzxmlMSLevel.ToString(CultureInfo.InvariantCulture),
                                                     mzxmlMSLevel));

            var mzxmlActivationMethod = settings.mzxmlActivationMethod;
            CometParams.Add("activation_method",
                            new TypedCometParam<string>(CometParamType.String,
                                                        mzxmlActivationMethod,
                                                        mzxmlActivationMethod));

            var minPeaks = settings.spectralProcessingMinPeaks;
            CometParams.Add("minimum_peaks",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     minPeaks.ToString(CultureInfo.InvariantCulture),
                                                     minPeaks));

            var minIntensity = settings.spectralProcessingMinIntensity;
            CometParams.Add("minimum_intensity",
                new TypedCometParam<double>(CometParamType.Double,
                                            minIntensity.ToString(CultureInfo.InvariantCulture),
                                            minIntensity));

            var removePrecursorTol = settings.spectralProcessingRemovePrecursorTol;
            CometParams.Add("remove_precursor_tolerance",
                new TypedCometParam<double>(CometParamType.Double,
                                            removePrecursorTol.ToString(CultureInfo.InvariantCulture),
                                            removePrecursorTol));

            var removePrecursorPeak = settings.spectralProcessingRemovePrecursorPeak;
            CometParams.Add("remove_precursor_peak",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     removePrecursorPeak.ToString(CultureInfo.InvariantCulture),
                                                     removePrecursorPeak));

            var clearMzRange = new DoubleRange(settings.spectralProcessingClearMzMin,
                                               settings.spectralProcessingClearMzMax);
            string clearMzRangeString = clearMzRange.Start.ToString(CultureInfo.InvariantCulture)
                                          + " " + clearMzRange.End.ToString(CultureInfo.InvariantCulture);
            CometParams.Add("clear_mz_range",
                            new TypedCometParam<DoubleRange>(CometParamType.DoubleRange,
                                                             clearMzRangeString,
                                                             clearMzRange));

            var spectrumBatchSize = settings.SpectrumBatchSize;
            CometParams.Add("spectrum_batch_size",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     spectrumBatchSize.ToString(CultureInfo.InvariantCulture),
                                                     spectrumBatchSize));

            var numThreads = settings.NumThreads;
            CometParams.Add("num_threads",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     numThreads.ToString(CultureInfo.InvariantCulture),
                                                     numThreads));

            var numResults = settings.NumResults;
            CometParams.Add("num_results",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     numResults.ToString(CultureInfo.InvariantCulture),
                                                     numResults));

            var maxFragmentCharge = settings.MaxFragmentCharge;
            CometParams.Add("max_fragment_charge",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     maxFragmentCharge.ToString(CultureInfo.InvariantCulture),
                                                     maxFragmentCharge));

            var maxPrecursorCharge = settings.MaxPrecursorCharge;
            CometParams.Add("max_precursor_charge",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     maxPrecursorCharge.ToString(CultureInfo.InvariantCulture),
                                                     maxPrecursorCharge));

            var clipNTermMethionine = settings.ClipNTermMethionine ? 1 : 0;
            CometParams.Add("clip_nterm_methionine",
                            new TypedCometParam<int>(CometParamType.Int,
                                                     clipNTermMethionine.ToString(CultureInfo.InvariantCulture),
                                                     clipNTermMethionine));

            string enzymeInfoStr = String.Empty;
            foreach (var row in settings.EnzymeInfo)
            {
                enzymeInfoStr += row + Environment.NewLine;
            }

            CometParams.Add("[COMET_ENZYME_INFO]",
                            new TypedCometParam<StringCollection>(CometParamType.StrCollection,
                                                                  enzymeInfoStr,
                                                                  settings.EnzymeInfo));

            _cometSettings = settings;
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

        public bool GetCometParamValue(String name, out double value)
        {
            value = 0.0;
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
            return true;
        }

        public bool GetCometParamValue(String name, out String value)
        {
            value = String.Empty;
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

        public bool GetCometParamValue(String name, out DoubleRange value)
        {
            value = null;
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
            return true;
        }

        public bool GetCometParamValue(String name, out IntRange value)
        {
            value = null;
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
            return true;
        }

        public bool GetCometParamValue(String name, out StringCollection value)
        {
            value = null;
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
            return true;
        }

        public bool SetCometParam(String paramName, CometParamType paramType, String strValue)
        {
            CometParam cometParam = null;
            switch (paramType)
            {
                case CometParamType.Int:
                    int intValue;
                    if (!int.TryParse(strValue, out intValue))
                    {
                        return false;
                    }
                    cometParam = new TypedCometParam<int>(paramType, strValue, intValue); 
                    break;
                case CometParamType.Double:
                    double doubleValue;
                    if (!double.TryParse(strValue, out doubleValue))
                    {
                        return false;
                    }
                    cometParam = new TypedCometParam<double>(paramType, strValue, doubleValue); 
                    break;
                case CometParamType.String:
                    cometParam = new TypedCometParam<String>(paramType, strValue, strValue);
                    break;
                case CometParamType.IntRange:
                    String[] intStrValues = strValue.Split(' ');
                    if (intStrValues.Length < 2)
                    {
                        return false;
                    }
                    int minIntVal, maxIntVal;
                    if (!int.TryParse(intStrValues[0], out minIntVal) || !int.TryParse(intStrValues[1], out maxIntVal))
                    {
                        return false;
                    }
                    cometParam = new TypedCometParam<IntRange>(paramType, strValue, new IntRange(minIntVal, maxIntVal));
                    break; 
                case CometParamType.DoubleRange:
                    String[] doubleStrValues = strValue.Split(' ');
                    if (doubleStrValues.Length < 2)
                    {
                        return false;
                    }
                    double minDoubleVal, maxDoubleVal;
                    if (!double.TryParse(doubleStrValues[0], out minDoubleVal) || !double.TryParse(doubleStrValues[1], out maxDoubleVal))
                    {
                        return false;
                    }
                    cometParam = new TypedCometParam<DoubleRange>(paramType, strValue, new DoubleRange(minDoubleVal, maxDoubleVal));
                    break;
                case CometParamType.VarMods:
                    break;
                case CometParamType.StrCollection:
                    break;
                default:
                    return false;
            }

            if (null != cometParam)
            {
                // If a parameter by the paramName passed in already exists,
                // just update its value; otherwise, add a new dictionary 
                // entry for it.
                CometParam currentParam;
                if (CometParams.TryGetValue(paramName, out currentParam))
                {
                    CometParams[paramName] = cometParam;
                }
                else
                {
                    CometParams.Add(paramName, cometParam);
                }
            }

            return true;
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
