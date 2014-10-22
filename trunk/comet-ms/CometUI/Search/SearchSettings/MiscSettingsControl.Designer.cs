using CometUI.CustomControls;

namespace CometUI.Search.SearchSettings
{
    partial class MiscSettingsControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.numThreadsCombo = new System.Windows.Forms.ComboBox();
            this.searchEnzymeLabel = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.maxFragmentChargeCombo = new System.Windows.Forms.ComboBox();
            this.maxPrecursorChargeCombo = new System.Windows.Forms.ComboBox();
            this.clipNTermMethionineCheckBox = new System.Windows.Forms.CheckBox();
            this.label6 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.mzxmlMsLevelCombo = new System.Windows.Forms.ComboBox();
            this.label11 = new System.Windows.Forms.Label();
            this.mzxmlActivationLevelCombo = new System.Windows.Forms.ComboBox();
            this.mzXMLGroupBox = new System.Windows.Forms.GroupBox();
            this.mzxmlPrecursorChargeMaxTextBox = new NumericTextBox();
            this.mzxmlPrecursorChargeMinTextBox = new NumericTextBox();
            this.mzxmlScanRangeMaxTextBox = new NumericTextBox();
            this.mzxmlScanRangeMinTextBox = new NumericTextBox();
            this.spectralProcessingGroupBox = new System.Windows.Forms.GroupBox();
            this.spectralProcessingMinPeaksTextBox = new NumericTextBox();
            this.label16 = new System.Windows.Forms.Label();
            this.spectralProcessingClearMZRangeMaxTextBox = new NumericTextBox();
            this.label17 = new System.Windows.Forms.Label();
            this.spectralProcessingClearMZRangeMinTextBox = new NumericTextBox();
            this.spectralProcessingRemovePrecursorPeakCombo = new System.Windows.Forms.ComboBox();
            this.label15 = new System.Windows.Forms.Label();
            this.spectralProcessingPrecursorRemovalTolTextBox = new NumericTextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.spectralProcessingMinIntensityTextBox = new NumericTextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.spectrumBatchSizeTextBox = new NumericTextBox();
            this.numResultsTextBox = new NumericTextBox();
            this.miscSettingsToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.mzXMLGroupBox.SuspendLayout();
            this.spectralProcessingGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // numThreadsCombo
            // 
            this.numThreadsCombo.DropDownHeight = 80;
            this.numThreadsCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.numThreadsCombo.DropDownWidth = 40;
            this.numThreadsCombo.FormattingEnabled = true;
            this.numThreadsCombo.IntegralHeight = false;
            this.numThreadsCombo.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16",
            "17",
            "18",
            "19",
            "20",
            "21",
            "22",
            "23",
            "24",
            "25",
            "26",
            "27",
            "28",
            "29",
            "30",
            "31",
            "32"});
            this.numThreadsCombo.Location = new System.Drawing.Point(32, 339);
            this.numThreadsCombo.Name = "numThreadsCombo";
            this.numThreadsCombo.Size = new System.Drawing.Size(65, 21);
            this.numThreadsCombo.TabIndex = 14;
            // 
            // searchEnzymeLabel
            // 
            this.searchEnzymeLabel.AutoSize = true;
            this.searchEnzymeLabel.Location = new System.Drawing.Point(29, 323);
            this.searchEnzymeLabel.Name = "searchEnzymeLabel";
            this.searchEnzymeLabel.Size = new System.Drawing.Size(74, 13);
            this.searchEnzymeLabel.TabIndex = 3;
            this.searchEnzymeLabel.Text = "Num Threads:";
            this.miscSettingsToolTip.SetToolTip(this.searchEnzymeLabel, "The number of processing threads that will be spawned for a search. When a value " +
        "of \"0\" is specified, the number of threads is set to the same value as the numbe" +
        "r of CPU cores available. ");
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(29, 270);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(109, 13);
            this.label1.TabIndex = 8;
            this.label1.Text = "Spectrum Batch Size:";
            this.miscSettingsToolTip.SetToolTip(this.label1, "When this parameter is set to a non-zero value, say 5000, this causes Comet to lo" +
        "ad and search about 5000 spectra at a time, looping through sets of 5000 spectra" +
        " until all data have been analyzed. ");
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(123, 323);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(70, 13);
            this.label2.TabIndex = 10;
            this.label2.Text = "Num Results:";
            this.miscSettingsToolTip.SetToolTip(this.label2, "Controls the number of peptide search results that are stored internally.");
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(277, 273);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(114, 13);
            this.label3.TabIndex = 11;
            this.label3.Text = "Max Fragment Charge:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(277, 309);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(115, 13);
            this.label4.TabIndex = 12;
            this.label4.Text = "Max Precursor Charge:";
            // 
            // maxFragmentChargeCombo
            // 
            this.maxFragmentChargeCombo.DropDownHeight = 80;
            this.maxFragmentChargeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.maxFragmentChargeCombo.DropDownWidth = 40;
            this.maxFragmentChargeCombo.FormattingEnabled = true;
            this.maxFragmentChargeCombo.IntegralHeight = false;
            this.maxFragmentChargeCombo.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5"});
            this.maxFragmentChargeCombo.Location = new System.Drawing.Point(397, 270);
            this.maxFragmentChargeCombo.Name = "maxFragmentChargeCombo";
            this.maxFragmentChargeCombo.Size = new System.Drawing.Size(43, 21);
            this.maxFragmentChargeCombo.TabIndex = 16;
            // 
            // maxPrecursorChargeCombo
            // 
            this.maxPrecursorChargeCombo.DropDownHeight = 80;
            this.maxPrecursorChargeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.maxPrecursorChargeCombo.DropDownWidth = 40;
            this.maxPrecursorChargeCombo.FormattingEnabled = true;
            this.maxPrecursorChargeCombo.IntegralHeight = false;
            this.maxPrecursorChargeCombo.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9"});
            this.maxPrecursorChargeCombo.Location = new System.Drawing.Point(397, 306);
            this.maxPrecursorChargeCombo.Name = "maxPrecursorChargeCombo";
            this.maxPrecursorChargeCombo.Size = new System.Drawing.Size(43, 21);
            this.maxPrecursorChargeCombo.TabIndex = 17;
            // 
            // clipNTermMethionineCheckBox
            // 
            this.clipNTermMethionineCheckBox.AutoSize = true;
            this.clipNTermMethionineCheckBox.Location = new System.Drawing.Point(280, 344);
            this.clipNTermMethionineCheckBox.Name = "clipNTermMethionineCheckBox";
            this.clipNTermMethionineCheckBox.Size = new System.Drawing.Size(131, 17);
            this.clipNTermMethionineCheckBox.TabIndex = 18;
            this.clipNTermMethionineCheckBox.Text = "Clip N-term methionine";
            this.clipNTermMethionineCheckBox.UseVisualStyleBackColor = true;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(15, 30);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(70, 13);
            this.label6.TabIndex = 20;
            this.label6.Text = "Scan Range:";
            this.miscSettingsToolTip.SetToolTip(this.label6, "When non-zero, only spectra within (and inclusive of) the specified scan range wi" +
        "ll be searched. ");
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(101, 49);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(16, 13);
            this.label7.TabIndex = 21;
            this.label7.Text = "to";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(15, 85);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(92, 13);
            this.label8.TabIndex = 22;
            this.label8.Text = "Precursor Charge:";
            this.miscSettingsToolTip.SetToolTip(this.label8, "Precursor charge range to search.");
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(101, 103);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(16, 13);
            this.label9.TabIndex = 25;
            this.label9.Text = "to";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(15, 139);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(55, 13);
            this.label10.TabIndex = 27;
            this.label10.Text = "MS Level:";
            // 
            // mzxmlMsLevelCombo
            // 
            this.mzxmlMsLevelCombo.DropDownHeight = 80;
            this.mzxmlMsLevelCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.mzxmlMsLevelCombo.DropDownWidth = 40;
            this.mzxmlMsLevelCombo.FormattingEnabled = true;
            this.mzxmlMsLevelCombo.IntegralHeight = false;
            this.mzxmlMsLevelCombo.Items.AddRange(new object[] {
            "2",
            "3"});
            this.mzxmlMsLevelCombo.Location = new System.Drawing.Point(18, 155);
            this.mzxmlMsLevelCombo.Name = "mzxmlMsLevelCombo";
            this.mzxmlMsLevelCombo.Size = new System.Drawing.Size(52, 21);
            this.mzxmlMsLevelCombo.TabIndex = 5;
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(117, 139);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(86, 13);
            this.label11.TabIndex = 29;
            this.label11.Text = "Activation Level:";
            // 
            // mzxmlActivationLevelCombo
            // 
            this.mzxmlActivationLevelCombo.DropDownHeight = 80;
            this.mzxmlActivationLevelCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.mzxmlActivationLevelCombo.DropDownWidth = 40;
            this.mzxmlActivationLevelCombo.FormattingEnabled = true;
            this.mzxmlActivationLevelCombo.IntegralHeight = false;
            this.mzxmlActivationLevelCombo.Items.AddRange(new object[] {
            "ALL",
            "CID",
            "ECD",
            "ETD",
            "PQD",
            "HCD",
            "IRMPD"});
            this.mzxmlActivationLevelCombo.Location = new System.Drawing.Point(120, 155);
            this.mzxmlActivationLevelCombo.Name = "mzxmlActivationLevelCombo";
            this.mzxmlActivationLevelCombo.Size = new System.Drawing.Size(83, 21);
            this.mzxmlActivationLevelCombo.TabIndex = 6;
            // 
            // mzXMLGroupBox
            // 
            this.mzXMLGroupBox.Controls.Add(this.mzxmlPrecursorChargeMaxTextBox);
            this.mzXMLGroupBox.Controls.Add(this.mzxmlPrecursorChargeMinTextBox);
            this.mzXMLGroupBox.Controls.Add(this.mzxmlScanRangeMaxTextBox);
            this.mzXMLGroupBox.Controls.Add(this.mzxmlScanRangeMinTextBox);
            this.mzXMLGroupBox.Controls.Add(this.label6);
            this.mzXMLGroupBox.Controls.Add(this.mzxmlActivationLevelCombo);
            this.mzXMLGroupBox.Controls.Add(this.label11);
            this.mzXMLGroupBox.Controls.Add(this.label7);
            this.mzXMLGroupBox.Controls.Add(this.mzxmlMsLevelCombo);
            this.mzXMLGroupBox.Controls.Add(this.label10);
            this.mzXMLGroupBox.Controls.Add(this.label8);
            this.mzXMLGroupBox.Controls.Add(this.label9);
            this.mzXMLGroupBox.Location = new System.Drawing.Point(28, 24);
            this.mzXMLGroupBox.Name = "mzXMLGroupBox";
            this.mzXMLGroupBox.Size = new System.Drawing.Size(218, 216);
            this.mzXMLGroupBox.TabIndex = 0;
            this.mzXMLGroupBox.TabStop = false;
            this.mzXMLGroupBox.Text = "mzXML";
            // 
            // mzxmlPrecursorChargeMaxTextBox
            // 
            this.mzxmlPrecursorChargeMaxTextBox.AllowDecimal = false;
            this.mzxmlPrecursorChargeMaxTextBox.AllowSpace = false;
            this.mzxmlPrecursorChargeMaxTextBox.Location = new System.Drawing.Point(123, 100);
            this.mzxmlPrecursorChargeMaxTextBox.Name = "mzxmlPrecursorChargeMaxTextBox";
            this.mzxmlPrecursorChargeMaxTextBox.Size = new System.Drawing.Size(80, 20);
            this.mzxmlPrecursorChargeMaxTextBox.TabIndex = 4;
            // 
            // mzxmlPrecursorChargeMinTextBox
            // 
            this.mzxmlPrecursorChargeMinTextBox.AllowDecimal = false;
            this.mzxmlPrecursorChargeMinTextBox.AllowSpace = false;
            this.mzxmlPrecursorChargeMinTextBox.Location = new System.Drawing.Point(18, 100);
            this.mzxmlPrecursorChargeMinTextBox.Name = "mzxmlPrecursorChargeMinTextBox";
            this.mzxmlPrecursorChargeMinTextBox.Size = new System.Drawing.Size(80, 20);
            this.mzxmlPrecursorChargeMinTextBox.TabIndex = 3;
            // 
            // mzxmlScanRangeMaxTextBox
            // 
            this.mzxmlScanRangeMaxTextBox.AllowDecimal = false;
            this.mzxmlScanRangeMaxTextBox.AllowSpace = false;
            this.mzxmlScanRangeMaxTextBox.Location = new System.Drawing.Point(123, 45);
            this.mzxmlScanRangeMaxTextBox.Name = "mzxmlScanRangeMaxTextBox";
            this.mzxmlScanRangeMaxTextBox.Size = new System.Drawing.Size(80, 20);
            this.mzxmlScanRangeMaxTextBox.TabIndex = 2;
            // 
            // mzxmlScanRangeMinTextBox
            // 
            this.mzxmlScanRangeMinTextBox.AllowDecimal = false;
            this.mzxmlScanRangeMinTextBox.AllowSpace = false;
            this.mzxmlScanRangeMinTextBox.Location = new System.Drawing.Point(18, 46);
            this.mzxmlScanRangeMinTextBox.Name = "mzxmlScanRangeMinTextBox";
            this.mzxmlScanRangeMinTextBox.Size = new System.Drawing.Size(80, 20);
            this.mzxmlScanRangeMinTextBox.TabIndex = 1;
            // 
            // spectralProcessingGroupBox
            // 
            this.spectralProcessingGroupBox.Controls.Add(this.spectralProcessingMinPeaksTextBox);
            this.spectralProcessingGroupBox.Controls.Add(this.label16);
            this.spectralProcessingGroupBox.Controls.Add(this.spectralProcessingClearMZRangeMaxTextBox);
            this.spectralProcessingGroupBox.Controls.Add(this.label17);
            this.spectralProcessingGroupBox.Controls.Add(this.spectralProcessingClearMZRangeMinTextBox);
            this.spectralProcessingGroupBox.Controls.Add(this.spectralProcessingRemovePrecursorPeakCombo);
            this.spectralProcessingGroupBox.Controls.Add(this.label15);
            this.spectralProcessingGroupBox.Controls.Add(this.spectralProcessingPrecursorRemovalTolTextBox);
            this.spectralProcessingGroupBox.Controls.Add(this.label14);
            this.spectralProcessingGroupBox.Controls.Add(this.label13);
            this.spectralProcessingGroupBox.Controls.Add(this.label12);
            this.spectralProcessingGroupBox.Controls.Add(this.spectralProcessingMinIntensityTextBox);
            this.spectralProcessingGroupBox.Controls.Add(this.label5);
            this.spectralProcessingGroupBox.Location = new System.Drawing.Point(267, 24);
            this.spectralProcessingGroupBox.Name = "spectralProcessingGroupBox";
            this.spectralProcessingGroupBox.Size = new System.Drawing.Size(234, 216);
            this.spectralProcessingGroupBox.TabIndex = 7;
            this.spectralProcessingGroupBox.TabStop = false;
            this.spectralProcessingGroupBox.Text = "Spectral Processing";
            // 
            // spectralProcessingMinPeaksTextBox
            // 
            this.spectralProcessingMinPeaksTextBox.AllowDecimal = false;
            this.spectralProcessingMinPeaksTextBox.AllowSpace = false;
            this.spectralProcessingMinPeaksTextBox.Location = new System.Drawing.Point(20, 45);
            this.spectralProcessingMinPeaksTextBox.Name = "spectralProcessingMinPeaksTextBox";
            this.spectralProcessingMinPeaksTextBox.Size = new System.Drawing.Size(80, 20);
            this.spectralProcessingMinPeaksTextBox.TabIndex = 7;
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(17, 156);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(90, 13);
            this.label16.TabIndex = 30;
            this.label16.Text = "Clear m/z Range:";
            this.miscSettingsToolTip.SetToolTip(this.label16, "Intended for iTRAQ/TMT type data where one might want to remove the reporter ion " +
        "signals in the MS/MS spectra prior to searching.");
            // 
            // spectralProcessingClearMZRangeMaxTextBox
            // 
            this.spectralProcessingClearMZRangeMaxTextBox.AllowDecimal = true;
            this.spectralProcessingClearMZRangeMaxTextBox.AllowSpace = false;
            this.spectralProcessingClearMZRangeMaxTextBox.Location = new System.Drawing.Point(135, 172);
            this.spectralProcessingClearMZRangeMaxTextBox.Name = "spectralProcessingClearMZRangeMaxTextBox";
            this.spectralProcessingClearMZRangeMaxTextBox.Size = new System.Drawing.Size(80, 20);
            this.spectralProcessingClearMZRangeMaxTextBox.TabIndex = 12;
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(109, 175);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(16, 13);
            this.label17.TabIndex = 33;
            this.label17.Text = "to";
            // 
            // spectralProcessingClearMZRangeMinTextBox
            // 
            this.spectralProcessingClearMZRangeMinTextBox.AllowDecimal = true;
            this.spectralProcessingClearMZRangeMinTextBox.AllowSpace = false;
            this.spectralProcessingClearMZRangeMinTextBox.Location = new System.Drawing.Point(20, 172);
            this.spectralProcessingClearMZRangeMinTextBox.Name = "spectralProcessingClearMZRangeMinTextBox";
            this.spectralProcessingClearMZRangeMinTextBox.Size = new System.Drawing.Size(80, 20);
            this.spectralProcessingClearMZRangeMinTextBox.TabIndex = 11;
            // 
            // spectralProcessingRemovePrecursorPeakCombo
            // 
            this.spectralProcessingRemovePrecursorPeakCombo.DropDownHeight = 80;
            this.spectralProcessingRemovePrecursorPeakCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.spectralProcessingRemovePrecursorPeakCombo.DropDownWidth = 40;
            this.spectralProcessingRemovePrecursorPeakCombo.FormattingEnabled = true;
            this.spectralProcessingRemovePrecursorPeakCombo.IntegralHeight = false;
            this.spectralProcessingRemovePrecursorPeakCombo.Location = new System.Drawing.Point(147, 119);
            this.spectralProcessingRemovePrecursorPeakCombo.Name = "spectralProcessingRemovePrecursorPeakCombo";
            this.spectralProcessingRemovePrecursorPeakCombo.Size = new System.Drawing.Size(68, 21);
            this.spectralProcessingRemovePrecursorPeakCombo.TabIndex = 10;
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(17, 122);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(126, 13);
            this.label15.TabIndex = 28;
            this.label15.Text = "Remove Precursor Peak:";
            this.miscSettingsToolTip.SetToolTip(this.label15, "Specify whether to exclude/remove any precursor signals from the input MS/MS spec" +
        "trum.");
            // 
            // spectralProcessingPrecursorRemovalTolTextBox
            // 
            this.spectralProcessingPrecursorRemovalTolTextBox.AllowDecimal = true;
            this.spectralProcessingPrecursorRemovalTolTextBox.AllowSpace = false;
            this.spectralProcessingPrecursorRemovalTolTextBox.Location = new System.Drawing.Point(149, 82);
            this.spectralProcessingPrecursorRemovalTolTextBox.Name = "spectralProcessingPrecursorRemovalTolTextBox";
            this.spectralProcessingPrecursorRemovalTolTextBox.Size = new System.Drawing.Size(66, 20);
            this.spectralProcessingPrecursorRemovalTolTextBox.TabIndex = 9;
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(17, 85);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(118, 13);
            this.label14.TabIndex = 26;
            this.label14.Text = "Precursor Removal Tol:";
            this.miscSettingsToolTip.SetToolTip(this.label14, "The mass tolerance around each precursor m/z that would be removed when the \"Remo" +
        "ve Precursor Peak\" option is invoked.");
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.BackColor = System.Drawing.Color.Transparent;
            this.label13.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label13.Location = new System.Drawing.Point(134, 84);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(13, 13);
            this.label13.TabIndex = 25;
            this.label13.Text = "+";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(132, 30);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(69, 13);
            this.label12.TabIndex = 24;
            this.label12.Text = "Min Intensity:";
            this.miscSettingsToolTip.SetToolTip(this.label12, "Minimum intensity value for input peaks.");
            // 
            // spectralProcessingMinIntensityTextBox
            // 
            this.spectralProcessingMinIntensityTextBox.AllowDecimal = true;
            this.spectralProcessingMinIntensityTextBox.AllowSpace = false;
            this.spectralProcessingMinIntensityTextBox.Location = new System.Drawing.Point(135, 46);
            this.spectralProcessingMinIntensityTextBox.Name = "spectralProcessingMinIntensityTextBox";
            this.spectralProcessingMinIntensityTextBox.Size = new System.Drawing.Size(80, 20);
            this.spectralProcessingMinIntensityTextBox.TabIndex = 8;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(17, 30);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(60, 13);
            this.label5.TabIndex = 22;
            this.label5.Text = "Min Peaks:";
            this.miscSettingsToolTip.SetToolTip(this.label5, "The minimum number of m/z-intensity pairs that must be present in a spectrum befo" +
        "re it is searched");
            // 
            // spectrumBatchSizeTextBox
            // 
            this.spectrumBatchSizeTextBox.AllowDecimal = false;
            this.spectrumBatchSizeTextBox.AllowSpace = false;
            this.spectrumBatchSizeTextBox.Location = new System.Drawing.Point(32, 287);
            this.spectrumBatchSizeTextBox.Name = "spectrumBatchSizeTextBox";
            this.spectrumBatchSizeTextBox.Size = new System.Drawing.Size(103, 20);
            this.spectrumBatchSizeTextBox.TabIndex = 13;
            // 
            // numResultsTextBox
            // 
            this.numResultsTextBox.AllowDecimal = false;
            this.numResultsTextBox.AllowSpace = false;
            this.numResultsTextBox.Location = new System.Drawing.Point(126, 339);
            this.numResultsTextBox.Name = "numResultsTextBox";
            this.numResultsTextBox.Size = new System.Drawing.Size(67, 20);
            this.numResultsTextBox.TabIndex = 15;
            // 
            // MiscSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.numResultsTextBox);
            this.Controls.Add(this.spectrumBatchSizeTextBox);
            this.Controls.Add(this.spectralProcessingGroupBox);
            this.Controls.Add(this.maxPrecursorChargeCombo);
            this.Controls.Add(this.maxFragmentChargeCombo);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.mzXMLGroupBox);
            this.Controls.Add(this.clipNTermMethionineCheckBox);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.searchEnzymeLabel);
            this.Controls.Add(this.numThreadsCombo);
            this.Name = "MiscSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            this.mzXMLGroupBox.ResumeLayout(false);
            this.mzXMLGroupBox.PerformLayout();
            this.spectralProcessingGroupBox.ResumeLayout(false);
            this.spectralProcessingGroupBox.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ComboBox numThreadsCombo;
        private System.Windows.Forms.Label searchEnzymeLabel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ComboBox maxFragmentChargeCombo;
        private System.Windows.Forms.ComboBox maxPrecursorChargeCombo;
        private System.Windows.Forms.CheckBox clipNTermMethionineCheckBox;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.ComboBox mzxmlMsLevelCombo;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.ComboBox mzxmlActivationLevelCombo;
        private System.Windows.Forms.GroupBox mzXMLGroupBox;
        private System.Windows.Forms.GroupBox spectralProcessingGroupBox;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.ComboBox spectralProcessingRemovePrecursorPeakCombo;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.Label label17;
        private NumericTextBox spectralProcessingMinIntensityTextBox;
        private NumericTextBox spectralProcessingPrecursorRemovalTolTextBox;
        private NumericTextBox spectralProcessingClearMZRangeMaxTextBox;
        private NumericTextBox spectralProcessingClearMZRangeMinTextBox;
        private NumericTextBox mzxmlScanRangeMinTextBox;
        private NumericTextBox mzxmlScanRangeMaxTextBox;
        private NumericTextBox mzxmlPrecursorChargeMinTextBox;
        private NumericTextBox mzxmlPrecursorChargeMaxTextBox;
        private NumericTextBox spectrumBatchSizeTextBox;
        private NumericTextBox numResultsTextBox;
        private NumericTextBox spectralProcessingMinPeaksTextBox;
        private System.Windows.Forms.ToolTip miscSettingsToolTip;
    }
}
