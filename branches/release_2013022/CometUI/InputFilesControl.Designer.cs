namespace CometUI
{
    partial class InputFilesControl
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
            this.panelNucleotideReadingFrame = new System.Windows.Forms.Panel();
            this.comboBoxReadingFrame = new System.Windows.Forms.ComboBox();
            this.labelReadingFrame = new System.Windows.Forms.Label();
            this.inputFilesLabel = new System.Windows.Forms.Label();
            this.labelDecoyPrefix = new System.Windows.Forms.Label();
            this.textBoxDecoyPrefix = new System.Windows.Forms.TextBox();
            this.radioButtonProtein = new System.Windows.Forms.RadioButton();
            this.radioButtonNucleotide = new System.Windows.Forms.RadioButton();
            this.radioButtonTarget = new System.Windows.Forms.RadioButton();
            this.panelProteinNucleotide = new System.Windows.Forms.Panel();
            this.panelDecoyPrefix = new System.Windows.Forms.Panel();
            this.panelTargetDecoy = new System.Windows.Forms.Panel();
            this.radioButtonDecoyOne = new System.Windows.Forms.RadioButton();
            this.radioButtonDecoyTwo = new System.Windows.Forms.RadioButton();
            this.protDbLabel = new System.Windows.Forms.Label();
            this.btnAddInputFile = new System.Windows.Forms.Button();
            this.btnRemInputFile = new System.Windows.Forms.Button();
            this.btnBrowseProteomeDbFile = new System.Windows.Forms.Button();
            this.proteomeDbFileCombo = new System.Windows.Forms.ComboBox();
            this.inputFilesList = new System.Windows.Forms.CheckedListBox();
            this.panelNucleotideReadingFrame.SuspendLayout();
            this.panelProteinNucleotide.SuspendLayout();
            this.panelDecoyPrefix.SuspendLayout();
            this.panelTargetDecoy.SuspendLayout();
            this.SuspendLayout();
            // 
            // panelNucleotideReadingFrame
            // 
            this.panelNucleotideReadingFrame.Controls.Add(this.comboBoxReadingFrame);
            this.panelNucleotideReadingFrame.Controls.Add(this.labelReadingFrame);
            this.panelNucleotideReadingFrame.Enabled = false;
            this.panelNucleotideReadingFrame.Location = new System.Drawing.Point(77, 23);
            this.panelNucleotideReadingFrame.Name = "panelNucleotideReadingFrame";
            this.panelNucleotideReadingFrame.Size = new System.Drawing.Size(195, 28);
            this.panelNucleotideReadingFrame.TabIndex = 14;
            // 
            // comboBoxReadingFrame
            // 
            this.comboBoxReadingFrame.BackColor = System.Drawing.SystemColors.Window;
            this.comboBoxReadingFrame.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxReadingFrame.FormattingEnabled = true;
            this.comboBoxReadingFrame.Items.AddRange(new object[] {
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9"});
            this.comboBoxReadingFrame.Location = new System.Drawing.Point(88, 3);
            this.comboBoxReadingFrame.Name = "comboBoxReadingFrame";
            this.comboBoxReadingFrame.Size = new System.Drawing.Size(62, 21);
            this.comboBoxReadingFrame.TabIndex = 15;
            // 
            // labelReadingFrame
            // 
            this.labelReadingFrame.AutoSize = true;
            this.labelReadingFrame.Location = new System.Drawing.Point(4, 6);
            this.labelReadingFrame.Name = "labelReadingFrame";
            this.labelReadingFrame.Size = new System.Drawing.Size(82, 13);
            this.labelReadingFrame.TabIndex = 0;
            this.labelReadingFrame.Text = "Reading Frame:";
            // 
            // inputFilesLabel
            // 
            this.inputFilesLabel.AutoSize = true;
            this.inputFilesLabel.Location = new System.Drawing.Point(9, 9);
            this.inputFilesLabel.Name = "inputFilesLabel";
            this.inputFilesLabel.Size = new System.Drawing.Size(58, 13);
            this.inputFilesLabel.TabIndex = 14;
            this.inputFilesLabel.Text = "&Input Files:";
            // 
            // labelDecoyPrefix
            // 
            this.labelDecoyPrefix.AutoSize = true;
            this.labelDecoyPrefix.Location = new System.Drawing.Point(4, 6);
            this.labelDecoyPrefix.Name = "labelDecoyPrefix";
            this.labelDecoyPrefix.Size = new System.Drawing.Size(70, 13);
            this.labelDecoyPrefix.TabIndex = 0;
            this.labelDecoyPrefix.Text = "Decoy Prefix:";
            // 
            // textBoxDecoyPrefix
            // 
            this.textBoxDecoyPrefix.Location = new System.Drawing.Point(76, 3);
            this.textBoxDecoyPrefix.Name = "textBoxDecoyPrefix";
            this.textBoxDecoyPrefix.Size = new System.Drawing.Size(74, 20);
            this.textBoxDecoyPrefix.TabIndex = 10;
            // 
            // radioButtonProtein
            // 
            this.radioButtonProtein.AutoSize = true;
            this.radioButtonProtein.Checked = true;
            this.radioButtonProtein.Location = new System.Drawing.Point(3, 4);
            this.radioButtonProtein.Name = "radioButtonProtein";
            this.radioButtonProtein.Size = new System.Drawing.Size(58, 17);
            this.radioButtonProtein.TabIndex = 12;
            this.radioButtonProtein.TabStop = true;
            this.radioButtonProtein.Text = "Protein";
            this.radioButtonProtein.UseVisualStyleBackColor = true;
            // 
            // radioButtonNucleotide
            // 
            this.radioButtonNucleotide.AutoSize = true;
            this.radioButtonNucleotide.Location = new System.Drawing.Point(65, 4);
            this.radioButtonNucleotide.Name = "radioButtonNucleotide";
            this.radioButtonNucleotide.Size = new System.Drawing.Size(76, 17);
            this.radioButtonNucleotide.TabIndex = 13;
            this.radioButtonNucleotide.TabStop = true;
            this.radioButtonNucleotide.Text = "Nucleotide";
            this.radioButtonNucleotide.UseVisualStyleBackColor = true;
            this.radioButtonNucleotide.CheckedChanged += new System.EventHandler(this.RadioButtonNucleotideCheckedChanged);
            // 
            // radioButtonTarget
            // 
            this.radioButtonTarget.AutoSize = true;
            this.radioButtonTarget.Checked = true;
            this.radioButtonTarget.Location = new System.Drawing.Point(4, 3);
            this.radioButtonTarget.Name = "radioButtonTarget";
            this.radioButtonTarget.Size = new System.Drawing.Size(56, 17);
            this.radioButtonTarget.TabIndex = 6;
            this.radioButtonTarget.TabStop = true;
            this.radioButtonTarget.Text = "Target";
            this.radioButtonTarget.UseVisualStyleBackColor = true;
            // 
            // panelProteinNucleotide
            // 
            this.panelProteinNucleotide.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelProteinNucleotide.Controls.Add(this.panelNucleotideReadingFrame);
            this.panelProteinNucleotide.Controls.Add(this.radioButtonProtein);
            this.panelProteinNucleotide.Controls.Add(this.radioButtonNucleotide);
            this.panelProteinNucleotide.Location = new System.Drawing.Point(9, 266);
            this.panelProteinNucleotide.Name = "panelProteinNucleotide";
            this.panelProteinNucleotide.Size = new System.Drawing.Size(427, 53);
            this.panelProteinNucleotide.TabIndex = 20;
            // 
            // panelDecoyPrefix
            // 
            this.panelDecoyPrefix.Controls.Add(this.textBoxDecoyPrefix);
            this.panelDecoyPrefix.Controls.Add(this.labelDecoyPrefix);
            this.panelDecoyPrefix.Enabled = false;
            this.panelDecoyPrefix.Location = new System.Drawing.Point(77, 22);
            this.panelDecoyPrefix.Name = "panelDecoyPrefix";
            this.panelDecoyPrefix.Size = new System.Drawing.Size(195, 28);
            this.panelDecoyPrefix.TabIndex = 9;
            // 
            // panelTargetDecoy
            // 
            this.panelTargetDecoy.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelTargetDecoy.Controls.Add(this.radioButtonTarget);
            this.panelTargetDecoy.Controls.Add(this.radioButtonDecoyOne);
            this.panelTargetDecoy.Controls.Add(this.radioButtonDecoyTwo);
            this.panelTargetDecoy.Controls.Add(this.panelDecoyPrefix);
            this.panelTargetDecoy.Location = new System.Drawing.Point(9, 214);
            this.panelTargetDecoy.Name = "panelTargetDecoy";
            this.panelTargetDecoy.Size = new System.Drawing.Size(427, 52);
            this.panelTargetDecoy.TabIndex = 18;
            // 
            // radioButtonDecoyOne
            // 
            this.radioButtonDecoyOne.AutoSize = true;
            this.radioButtonDecoyOne.Location = new System.Drawing.Point(66, 3);
            this.radioButtonDecoyOne.Name = "radioButtonDecoyOne";
            this.radioButtonDecoyOne.Size = new System.Drawing.Size(65, 17);
            this.radioButtonDecoyOne.TabIndex = 7;
            this.radioButtonDecoyOne.Text = "Decoy 1";
            this.radioButtonDecoyOne.UseVisualStyleBackColor = true;
            this.radioButtonDecoyOne.CheckedChanged += new System.EventHandler(this.RadioButtonDecoyOneCheckedChanged);
            // 
            // radioButtonDecoyTwo
            // 
            this.radioButtonDecoyTwo.AutoSize = true;
            this.radioButtonDecoyTwo.Location = new System.Drawing.Point(137, 3);
            this.radioButtonDecoyTwo.Name = "radioButtonDecoyTwo";
            this.radioButtonDecoyTwo.Size = new System.Drawing.Size(65, 17);
            this.radioButtonDecoyTwo.TabIndex = 12;
            this.radioButtonDecoyTwo.TabStop = true;
            this.radioButtonDecoyTwo.Text = "Decoy 2";
            this.radioButtonDecoyTwo.UseVisualStyleBackColor = true;
            this.radioButtonDecoyTwo.CheckedChanged += new System.EventHandler(this.RadioButtonDecoyTwoCheckedChanged);
            // 
            // protDbLabel
            // 
            this.protDbLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.protDbLabel.AutoSize = true;
            this.protDbLabel.Location = new System.Drawing.Point(9, 172);
            this.protDbLabel.Name = "protDbLabel";
            this.protDbLabel.Size = new System.Drawing.Size(139, 13);
            this.protDbLabel.TabIndex = 19;
            this.protDbLabel.Text = "&Proteome Database (.fasta):";
            // 
            // btnAddInputFile
            // 
            this.btnAddInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnAddInputFile.Location = new System.Drawing.Point(442, 25);
            this.btnAddInputFile.Name = "btnAddInputFile";
            this.btnAddInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnAddInputFile.TabIndex = 13;
            this.btnAddInputFile.Text = "&Add...";
            this.btnAddInputFile.UseVisualStyleBackColor = true;
            this.btnAddInputFile.Click += new System.EventHandler(this.BtnAddInputFileClick);
            // 
            // btnRemInputFile
            // 
            this.btnRemInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnRemInputFile.Enabled = false;
            this.btnRemInputFile.Location = new System.Drawing.Point(442, 54);
            this.btnRemInputFile.Name = "btnRemInputFile";
            this.btnRemInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnRemInputFile.TabIndex = 15;
            this.btnRemInputFile.Text = "&Remove";
            this.btnRemInputFile.UseVisualStyleBackColor = true;
            this.btnRemInputFile.Click += new System.EventHandler(this.BtnRemInputFileClick);
            // 
            // btnBrowseProteomeDbFile
            // 
            this.btnBrowseProteomeDbFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowseProteomeDbFile.Location = new System.Drawing.Point(442, 187);
            this.btnBrowseProteomeDbFile.Name = "btnBrowseProteomeDbFile";
            this.btnBrowseProteomeDbFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowseProteomeDbFile.TabIndex = 17;
            this.btnBrowseProteomeDbFile.Text = "&Browse";
            this.btnBrowseProteomeDbFile.UseVisualStyleBackColor = true;
            this.btnBrowseProteomeDbFile.Click += new System.EventHandler(this.BtnBrowseProteomeDbFileClick);
            // 
            // proteomeDbFileCombo
            // 
            this.proteomeDbFileCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.proteomeDbFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.proteomeDbFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.proteomeDbFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.proteomeDbFileCombo.FormattingEnabled = true;
            this.proteomeDbFileCombo.Location = new System.Drawing.Point(9, 188);
            this.proteomeDbFileCombo.Name = "proteomeDbFileCombo";
            this.proteomeDbFileCombo.Size = new System.Drawing.Size(427, 23);
            this.proteomeDbFileCombo.TabIndex = 16;
            // 
            // inputFilesList
            // 
            this.inputFilesList.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.inputFilesList.CheckOnClick = true;
            this.inputFilesList.FormattingEnabled = true;
            this.inputFilesList.HorizontalScrollbar = true;
            this.inputFilesList.Location = new System.Drawing.Point(9, 25);
            this.inputFilesList.Name = "inputFilesList";
            this.inputFilesList.Size = new System.Drawing.Size(427, 124);
            this.inputFilesList.TabIndex = 12;
            this.inputFilesList.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.InputFilesListItemCheck);
            // 
            // InputFilesControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.inputFilesList);
            this.Controls.Add(this.inputFilesLabel);
            this.Controls.Add(this.panelProteinNucleotide);
            this.Controls.Add(this.panelTargetDecoy);
            this.Controls.Add(this.protDbLabel);
            this.Controls.Add(this.btnAddInputFile);
            this.Controls.Add(this.btnRemInputFile);
            this.Controls.Add(this.btnBrowseProteomeDbFile);
            this.Controls.Add(this.proteomeDbFileCombo);
            this.Name = "InputFilesControl";
            this.Size = new System.Drawing.Size(527, 330);
            this.panelNucleotideReadingFrame.ResumeLayout(false);
            this.panelNucleotideReadingFrame.PerformLayout();
            this.panelProteinNucleotide.ResumeLayout(false);
            this.panelProteinNucleotide.PerformLayout();
            this.panelDecoyPrefix.ResumeLayout(false);
            this.panelDecoyPrefix.PerformLayout();
            this.panelTargetDecoy.ResumeLayout(false);
            this.panelTargetDecoy.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Panel panelNucleotideReadingFrame;
        private System.Windows.Forms.ComboBox comboBoxReadingFrame;
        private System.Windows.Forms.Label labelReadingFrame;
        private System.Windows.Forms.Label inputFilesLabel;
        private System.Windows.Forms.Label labelDecoyPrefix;
        private System.Windows.Forms.TextBox textBoxDecoyPrefix;
        private System.Windows.Forms.RadioButton radioButtonProtein;
        private System.Windows.Forms.RadioButton radioButtonNucleotide;
        private System.Windows.Forms.RadioButton radioButtonTarget;
        private System.Windows.Forms.Panel panelProteinNucleotide;
        private System.Windows.Forms.Panel panelDecoyPrefix;
        private System.Windows.Forms.Panel panelTargetDecoy;
        private System.Windows.Forms.RadioButton radioButtonDecoyOne;
        private System.Windows.Forms.RadioButton radioButtonDecoyTwo;
        private System.Windows.Forms.Label protDbLabel;
        private System.Windows.Forms.Button btnAddInputFile;
        private System.Windows.Forms.Button btnRemInputFile;
        private System.Windows.Forms.Button btnBrowseProteomeDbFile;
        private System.Windows.Forms.ComboBox proteomeDbFileCombo;
        private System.Windows.Forms.CheckedListBox inputFilesList;
    }
}
