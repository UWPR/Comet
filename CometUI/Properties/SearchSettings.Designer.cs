﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace CometUI.Properties {
    
    
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "10.0.0.0")]
    public sealed partial class SearchSettings : global::System.Configuration.ApplicationSettingsBase {
        
        private static SearchSettings defaultInstance = ((SearchSettings)(global::System.Configuration.ApplicationSettingsBase.Synchronized(new SearchSettings())));
        
        public static SearchSettings Default {
            get {
                return defaultInstance;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string ProteomeDatabaseFile {
            get {
                return ((string)(this["ProteomeDatabaseFile"]));
            }
            set {
                this["ProteomeDatabaseFile"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int NucleotideReadingFrame {
            get {
                return ((int)(this["NucleotideReadingFrame"]));
            }
            set {
                this["NucleotideReadingFrame"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int SearchType {
            get {
                return ((int)(this["SearchType"]));
            }
            set {
                this["SearchType"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("DECOY_")]
        public string DecoyPrefix {
            get {
                return ((string)(this["DecoyPrefix"]));
            }
            set {
                this["DecoyPrefix"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("2")]
        public int EnzymeTermini {
            get {
                return ((int)(this["EnzymeTermini"]));
            }
            set {
                this["EnzymeTermini"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("2")]
        public int AllowedMissedCleavages {
            get {
                return ((int)(this["AllowedMissedCleavages"]));
            }
            set {
                this["AllowedMissedCleavages"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute(@"<?xml version=""1.0"" encoding=""utf-16""?>
<ArrayOfString xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"">
  <string>0,No_enzyme,0,-,-</string>
  <string>1,Trypsin,1,KR,P</string>
  <string>2,Trypsin/P,1,KR,-</string>
  <string>3,Lys_C,1,K,P</string>
  <string>4,Lys_N,0,K,-</string>
  <string>5,Arg_C,1,R,P</string>
  <string>6,Asp_N,0,D,-</string>
  <string>7,CNBr,1,M,-</string>
  <string>8,Glu_C,1,DE,P</string>
  <string>9,PepsinA,1,FL,P</string>
  <string>10,Chymotrypsin,1,FWYL,P</string>
</ArrayOfString>")]
        public global::System.Collections.Specialized.StringCollection EnzymeInfo {
            get {
                return ((global::System.Collections.Specialized.StringCollection)(this["EnzymeInfo"]));
            }
            set {
                this["EnzymeInfo"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1")]
        public int SearchEnzymeNumber {
            get {
                return ((int)(this["SearchEnzymeNumber"]));
            }
            set {
                this["SearchEnzymeNumber"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1")]
        public int SampleEnzymeNumber {
            get {
                return ((int)(this["SampleEnzymeNumber"]));
            }
            set {
                this["SampleEnzymeNumber"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("20")]
        public double PrecursorMassTolerance {
            get {
                return ((double)(this["PrecursorMassTolerance"]));
            }
            set {
                this["PrecursorMassTolerance"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("2")]
        public int PrecursorMassUnit {
            get {
                return ((int)(this["PrecursorMassUnit"]));
            }
            set {
                this["PrecursorMassUnit"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1")]
        public int PrecursorMassType {
            get {
                return ((int)(this["PrecursorMassType"]));
            }
            set {
                this["PrecursorMassType"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1")]
        public int PrecursorIsotopeError {
            get {
                return ((int)(this["PrecursorIsotopeError"]));
            }
            set {
                this["PrecursorIsotopeError"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1.0005")]
        public double FragmentBinSize {
            get {
                return ((double)(this["FragmentBinSize"]));
            }
            set {
                this["FragmentBinSize"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0.4")]
        public double FragmentBinOffset {
            get {
                return ((double)(this["FragmentBinOffset"]));
            }
            set {
                this["FragmentBinOffset"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1")]
        public int FragmentMassType {
            get {
                return ((int)(this["FragmentMassType"]));
            }
            set {
                this["FragmentMassType"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool UseAIons {
            get {
                return ((bool)(this["UseAIons"]));
            }
            set {
                this["UseAIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool UseBIons {
            get {
                return ((bool)(this["UseBIons"]));
            }
            set {
                this["UseBIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool UseCIons {
            get {
                return ((bool)(this["UseCIons"]));
            }
            set {
                this["UseCIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool UseXIons {
            get {
                return ((bool)(this["UseXIons"]));
            }
            set {
                this["UseXIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool UseYIons {
            get {
                return ((bool)(this["UseYIons"]));
            }
            set {
                this["UseYIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool UseZIons {
            get {
                return ((bool)(this["UseZIons"]));
            }
            set {
                this["UseZIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool UseNLIons {
            get {
                return ((bool)(this["UseNLIons"]));
            }
            set {
                this["UseNLIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool TheoreticalFragmentIons {
            get {
                return ((bool)(this["TheoreticalFragmentIons"]));
            }
            set {
                this["TheoreticalFragmentIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool OutputFormatPepXML {
            get {
                return ((bool)(this["OutputFormatPepXML"]));
            }
            set {
                this["OutputFormatPepXML"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool OutputFormatPercolator {
            get {
                return ((bool)(this["OutputFormatPercolator"]));
            }
            set {
                this["OutputFormatPercolator"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool OutputFormatOutFiles {
            get {
                return ((bool)(this["OutputFormatOutFiles"]));
            }
            set {
                this["OutputFormatOutFiles"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool OutputFormatSqtFile {
            get {
                return ((bool)(this["OutputFormatSqtFile"]));
            }
            set {
                this["OutputFormatSqtFile"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool PrintExpectScoreInPlaceOfSP {
            get {
                return ((bool)(this["PrintExpectScoreInPlaceOfSP"]));
            }
            set {
                this["PrintExpectScoreInPlaceOfSP"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("5")]
        public int NumOutputLines {
            get {
                return ((int)(this["NumOutputLines"]));
            }
            set {
                this["NumOutputLines"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool OutputFormatTextFile {
            get {
                return ((bool)(this["OutputFormatTextFile"]));
            }
            set {
                this["OutputFormatTextFile"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool OutputFormatShowFragmentIons {
            get {
                return ((bool)(this["OutputFormatShowFragmentIons"]));
            }
            set {
                this["OutputFormatShowFragmentIons"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute(@"<?xml version=""1.0"" encoding=""utf-16""?>
<ArrayOfString xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"">
  <string>15.9949,M,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
  <string>0.0,X,0,3,-1,0,0</string>
</ArrayOfString>")]
        public global::System.Collections.Specialized.StringCollection VariableMods {
            get {
                return ((global::System.Collections.Specialized.StringCollection)(this["VariableMods"]));
            }
            set {
                this["VariableMods"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("5")]
        public int MaxVarModsInPeptide {
            get {
                return ((int)(this["MaxVarModsInPeptide"]));
            }
            set {
                this["MaxVarModsInPeptide"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute(@"<?xml version=""1.0"" encoding=""utf-16""?>
<ArrayOfString xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"">
  <string>Glycine,G,0.0000</string>
  <string>Alanine,A,0.0000</string>
  <string>Serine,S,0.0000</string>
  <string>Proline,P,0.0000</string>
  <string>Valine,V,0.0000</string>
  <string>Threonine,T,0.0000</string>
  <string>Cysteine,C,57.021464</string>
  <string>Leucine,L,0.0000</string>
  <string>Isoleucine,I,0.0000</string>
  <string>Asparagine,N,0.0000</string>
  <string>Aspartic Acid,D,0.0000</string>
  <string>Glutamine,Q,0.0000</string>
  <string>Lysine,K,0.0000</string>
  <string>Glutamic Acid,E,0.0000</string>
  <string>Methionine,M,0.0000</string>
  <string>Ornithine,O,0.0000</string>
  <string>Histidine,H,0.0000</string>
  <string>Phenylalanine,F,0.0000</string>
  <string>Selenocysteine,U,0.0000</string>
  <string>Arginine,R,0.0000</string>
  <string>Tyrosine,Y,0.0000</string>
  <string>Tryptophan,W,0.0000</string>
  <string>User Amino Acid,B,0.0000</string>
  <string>User Amino Acid,J,0.0000</string>
  <string>User Amino Acid,X,0.0000</string>
  <string>User Amino Acid,Z,0.0000</string>
</ArrayOfString>")]
        public global::System.Collections.Specialized.StringCollection StaticMods {
            get {
                return ((global::System.Collections.Specialized.StringCollection)(this["StaticMods"]));
            }
            set {
                this["StaticMods"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double StaticModNTermPeptide {
            get {
                return ((double)(this["StaticModNTermPeptide"]));
            }
            set {
                this["StaticModNTermPeptide"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double StaticModCTermPeptide {
            get {
                return ((double)(this["StaticModCTermPeptide"]));
            }
            set {
                this["StaticModCTermPeptide"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double StaticModNTermProtein {
            get {
                return ((double)(this["StaticModNTermProtein"]));
            }
            set {
                this["StaticModNTermProtein"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double StaticModCTermProtein {
            get {
                return ((double)(this["StaticModCTermProtein"]));
            }
            set {
                this["StaticModCTermProtein"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool OutputFormatSkipReSearching {
            get {
                return ((bool)(this["OutputFormatSkipReSearching"]));
            }
            set {
                this["OutputFormatSkipReSearching"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int NumThreads {
            get {
                return ((int)(this["NumThreads"]));
            }
            set {
                this["NumThreads"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int SpectrumBatchSize {
            get {
                return ((int)(this["SpectrumBatchSize"]));
            }
            set {
                this["SpectrumBatchSize"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("50")]
        public int NumResults {
            get {
                return ((int)(this["NumResults"]));
            }
            set {
                this["NumResults"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("3")]
        public int MaxFragmentCharge {
            get {
                return ((int)(this["MaxFragmentCharge"]));
            }
            set {
                this["MaxFragmentCharge"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("6")]
        public int MaxPrecursorCharge {
            get {
                return ((int)(this["MaxPrecursorCharge"]));
            }
            set {
                this["MaxPrecursorCharge"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool ClipNTermMethionine {
            get {
                return ((bool)(this["ClipNTermMethionine"]));
            }
            set {
                this["ClipNTermMethionine"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int mzxmlScanRangeMin {
            get {
                return ((int)(this["mzxmlScanRangeMin"]));
            }
            set {
                this["mzxmlScanRangeMin"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int mzxmlScanRangeMax {
            get {
                return ((int)(this["mzxmlScanRangeMax"]));
            }
            set {
                this["mzxmlScanRangeMax"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int mzxmlPrecursorChargeRangeMin {
            get {
                return ((int)(this["mzxmlPrecursorChargeRangeMin"]));
            }
            set {
                this["mzxmlPrecursorChargeRangeMin"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int mzxmlPrecursorChargeRangeMax {
            get {
                return ((int)(this["mzxmlPrecursorChargeRangeMax"]));
            }
            set {
                this["mzxmlPrecursorChargeRangeMax"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("2")]
        public int mzxmlMsLevel {
            get {
                return ((int)(this["mzxmlMsLevel"]));
            }
            set {
                this["mzxmlMsLevel"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("ALL")]
        public string mzxmlActivationMethod {
            get {
                return ((string)(this["mzxmlActivationMethod"]));
            }
            set {
                this["mzxmlActivationMethod"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("10")]
        public int spectralProcessingMinPeaks {
            get {
                return ((int)(this["spectralProcessingMinPeaks"]));
            }
            set {
                this["spectralProcessingMinPeaks"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double spectralProcessingMinIntensity {
            get {
                return ((double)(this["spectralProcessingMinIntensity"]));
            }
            set {
                this["spectralProcessingMinIntensity"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int spectralProcessingRemovePrecursorPeak {
            get {
                return ((int)(this["spectralProcessingRemovePrecursorPeak"]));
            }
            set {
                this["spectralProcessingRemovePrecursorPeak"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1.5")]
        public double spectralProcessingRemovePrecursorTol {
            get {
                return ((double)(this["spectralProcessingRemovePrecursorTol"]));
            }
            set {
                this["spectralProcessingRemovePrecursorTol"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double spectralProcessingClearMzMin {
            get {
                return ((double)(this["spectralProcessingClearMzMin"]));
            }
            set {
                this["spectralProcessingClearMzMin"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public double spectralProcessingClearMzMax {
            get {
                return ((double)(this["spectralProcessingClearMzMax"]));
            }
            set {
                this["spectralProcessingClearMzMax"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("600")]
        public double digestMassRangeMin {
            get {
                return ((double)(this["digestMassRangeMin"]));
            }
            set {
                this["digestMassRangeMin"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("5000")]
        public double digestMassRangeMax {
            get {
                return ((double)(this["digestMassRangeMax"]));
            }
            set {
                this["digestMassRangeMax"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool OutputFormatSqtToStandardOutput {
            get {
                return ((bool)(this["OutputFormatSqtToStandardOutput"]));
            }
            set {
                this["OutputFormatSqtToStandardOutput"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool RequireVariableMod {
            get {
                return ((bool)(this["RequireVariableMod"]));
            }
            set {
                this["RequireVariableMod"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int mzxmlOverrideCharge {
            get {
                return ((int)(this["mzxmlOverrideCharge"]));
            }
            set {
                this["mzxmlOverrideCharge"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string OutputSuffix {
            get {
                return ((string)(this["OutputSuffix"]));
            }
            set {
                this["OutputSuffix"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("0")]
        public int PrecursorToleranceType {
            get {
                return ((int)(this["PrecursorToleranceType"]));
            }
            set {
                this["PrecursorToleranceType"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string PrecursorMassOffsets {
            get {
                return ((string)(this["PrecursorMassOffsets"]));
            }
            set {
                this["PrecursorMassOffsets"] = value;
            }
        }
    }
}
