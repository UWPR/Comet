namespace CometUI
{
    partial class CometMainForm
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btnSearch = new System.Windows.Forms.Button();
            this.menuStripMain = new System.Windows.Forms.MenuStrip();
            this.toolStripMenuItemHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
            this.cometMainTab = new System.Windows.Forms.TabControl();
            this.inputFilesTabPage = new System.Windows.Forms.TabPage();
            this.panelProteinNucleotide = new System.Windows.Forms.Panel();
            this.panelNucleotideReadingFrame = new System.Windows.Forms.Panel();
            this.comboBoxReadingFrame = new System.Windows.Forms.ComboBox();
            this.labelReadingFrame = new System.Windows.Forms.Label();
            this.radioButtonProtein = new System.Windows.Forms.RadioButton();
            this.radioButtonNucleotide = new System.Windows.Forms.RadioButton();
            this.panelTargetDecoy = new System.Windows.Forms.Panel();
            this.radioButtonTarget = new System.Windows.Forms.RadioButton();
            this.radioButtonDecoyOne = new System.Windows.Forms.RadioButton();
            this.radioButtonDecoyTwo = new System.Windows.Forms.RadioButton();
            this.panelDecoyPrefix = new System.Windows.Forms.Panel();
            this.textBoxDecoyPrefix = new System.Windows.Forms.TextBox();
            this.labelDecoyPrefix = new System.Windows.Forms.Label();
            this.protDbLabel = new System.Windows.Forms.Label();
            this.btnRemInputFile = new System.Windows.Forms.Button();
            this.btnBrowseProteomeDbFile = new System.Windows.Forms.Button();
            this.btnAddInputFile = new System.Windows.Forms.Button();
            this.proteomeDbFileCombo = new System.Windows.Forms.ComboBox();
            this.inputFilesLabel = new System.Windows.Forms.Label();
            this.inputFilesListBox = new System.Windows.Forms.ListBox();
            this.enzymeTabPage = new System.Windows.Forms.TabPage();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.btnLoadParams = new System.Windows.Forms.Button();
            this.btnSaveParams = new System.Windows.Forms.Button();
            this.statusStripMain = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.menuStripMain.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).BeginInit();
            this.mainSplitContainer.Panel1.SuspendLayout();
            this.mainSplitContainer.Panel2.SuspendLayout();
            this.mainSplitContainer.SuspendLayout();
            this.cometMainTab.SuspendLayout();
            this.inputFilesTabPage.SuspendLayout();
            this.panelProteinNucleotide.SuspendLayout();
            this.panelNucleotideReadingFrame.SuspendLayout();
            this.panelTargetDecoy.SuspendLayout();
            this.panelDecoyPrefix.SuspendLayout();
            this.statusStripMain.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnSearch
            // 
            this.btnSearch.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSearch.Location = new System.Drawing.Point(469, 0);
            this.btnSearch.Name = "btnSearch";
            this.btnSearch.Size = new System.Drawing.Size(78, 23);
            this.btnSearch.TabIndex = 18;
            this.btnSearch.Text = "Search";
            this.btnSearch.UseVisualStyleBackColor = true;
            this.btnSearch.Click += new System.EventHandler(this.BtnTestClick);
            // 
            // menuStripMain
            // 
            this.menuStripMain.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItemHelp});
            this.menuStripMain.Location = new System.Drawing.Point(0, 0);
            this.menuStripMain.Name = "menuStripMain";
            this.menuStripMain.Size = new System.Drawing.Size(559, 24);
            this.menuStripMain.TabIndex = 1;
            this.menuStripMain.Text = "Main Menu";
            // 
            // toolStripMenuItemHelp
            // 
            this.toolStripMenuItemHelp.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem});
            this.toolStripMenuItemHelp.Name = "toolStripMenuItemHelp";
            this.toolStripMenuItemHelp.Size = new System.Drawing.Size(44, 20);
            this.toolStripMenuItemHelp.Text = "&Help";
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(107, 22);
            this.aboutToolStripMenuItem.Text = "&About";
            // 
            // mainSplitContainer
            // 
            this.mainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.mainSplitContainer.IsSplitterFixed = true;
            this.mainSplitContainer.Location = new System.Drawing.Point(0, 24);
            this.mainSplitContainer.Name = "mainSplitContainer";
            this.mainSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // mainSplitContainer.Panel1
            // 
            this.mainSplitContainer.Panel1.Controls.Add(this.cometMainTab);
            // 
            // mainSplitContainer.Panel2
            // 
            this.mainSplitContainer.Panel2.Controls.Add(this.btnLoadParams);
            this.mainSplitContainer.Panel2.Controls.Add(this.btnSaveParams);
            this.mainSplitContainer.Panel2.Controls.Add(this.statusStripMain);
            this.mainSplitContainer.Panel2.Controls.Add(this.btnSearch);
            this.mainSplitContainer.Size = new System.Drawing.Size(559, 413);
            this.mainSplitContainer.SplitterDistance = 358;
            this.mainSplitContainer.TabIndex = 2;
            // 
            // cometMainTab
            // 
            this.cometMainTab.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.cometMainTab.Controls.Add(this.inputFilesTabPage);
            this.cometMainTab.Controls.Add(this.enzymeTabPage);
            this.cometMainTab.Controls.Add(this.tabPage1);
            this.cometMainTab.Controls.Add(this.tabPage2);
            this.cometMainTab.Controls.Add(this.tabPage3);
            this.cometMainTab.Controls.Add(this.tabPage4);
            this.cometMainTab.Location = new System.Drawing.Point(12, 0);
            this.cometMainTab.Name = "cometMainTab";
            this.cometMainTab.SelectedIndex = 0;
            this.cometMainTab.Size = new System.Drawing.Size(535, 356);
            this.cometMainTab.TabIndex = 10;
            // 
            // inputFilesTabPage
            // 
            this.inputFilesTabPage.Controls.Add(this.panelProteinNucleotide);
            this.inputFilesTabPage.Controls.Add(this.panelTargetDecoy);
            this.inputFilesTabPage.Controls.Add(this.protDbLabel);
            this.inputFilesTabPage.Controls.Add(this.btnRemInputFile);
            this.inputFilesTabPage.Controls.Add(this.btnBrowseProteomeDbFile);
            this.inputFilesTabPage.Controls.Add(this.btnAddInputFile);
            this.inputFilesTabPage.Controls.Add(this.proteomeDbFileCombo);
            this.inputFilesTabPage.Controls.Add(this.inputFilesLabel);
            this.inputFilesTabPage.Controls.Add(this.inputFilesListBox);
            this.inputFilesTabPage.Location = new System.Drawing.Point(4, 22);
            this.inputFilesTabPage.Name = "inputFilesTabPage";
            this.inputFilesTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.inputFilesTabPage.Size = new System.Drawing.Size(527, 330);
            this.inputFilesTabPage.TabIndex = 0;
            this.inputFilesTabPage.Text = "Input Files";
            this.inputFilesTabPage.UseVisualStyleBackColor = true;
            // 
            // panelProteinNucleotide
            // 
            this.panelProteinNucleotide.Controls.Add(this.panelNucleotideReadingFrame);
            this.panelProteinNucleotide.Controls.Add(this.radioButtonProtein);
            this.panelProteinNucleotide.Controls.Add(this.radioButtonNucleotide);
            this.panelProteinNucleotide.Location = new System.Drawing.Point(7, 269);
            this.panelProteinNucleotide.Name = "panelProteinNucleotide";
            this.panelProteinNucleotide.Size = new System.Drawing.Size(427, 55);
            this.panelProteinNucleotide.TabIndex = 11;
            // 
            // panelNucleotideReadingFrame
            // 
            this.panelNucleotideReadingFrame.Controls.Add(this.comboBoxReadingFrame);
            this.panelNucleotideReadingFrame.Controls.Add(this.labelReadingFrame);
            this.panelNucleotideReadingFrame.Location = new System.Drawing.Point(77, 22);
            this.panelNucleotideReadingFrame.Name = "panelNucleotideReadingFrame";
            this.panelNucleotideReadingFrame.Size = new System.Drawing.Size(195, 28);
            this.panelNucleotideReadingFrame.TabIndex = 14;
            // 
            // comboBoxReadingFrame
            // 
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
            // radioButtonProtein
            // 
            this.radioButtonProtein.AutoSize = true;
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
            // 
            // panelTargetDecoy
            // 
            this.panelTargetDecoy.Controls.Add(this.radioButtonTarget);
            this.panelTargetDecoy.Controls.Add(this.radioButtonDecoyOne);
            this.panelTargetDecoy.Controls.Add(this.radioButtonDecoyTwo);
            this.panelTargetDecoy.Controls.Add(this.panelDecoyPrefix);
            this.panelTargetDecoy.Location = new System.Drawing.Point(7, 217);
            this.panelTargetDecoy.Name = "panelTargetDecoy";
            this.panelTargetDecoy.Size = new System.Drawing.Size(427, 51);
            this.panelTargetDecoy.TabIndex = 5;
            // 
            // radioButtonTarget
            // 
            this.radioButtonTarget.AutoSize = true;
            this.radioButtonTarget.Location = new System.Drawing.Point(4, 3);
            this.radioButtonTarget.Name = "radioButtonTarget";
            this.radioButtonTarget.Size = new System.Drawing.Size(56, 17);
            this.radioButtonTarget.TabIndex = 6;
            this.radioButtonTarget.TabStop = true;
            this.radioButtonTarget.Text = "Target";
            this.radioButtonTarget.UseVisualStyleBackColor = true;
            // 
            // radioButtonDecoyOne
            // 
            this.radioButtonDecoyOne.AutoSize = true;
            this.radioButtonDecoyOne.Location = new System.Drawing.Point(66, 3);
            this.radioButtonDecoyOne.Name = "radioButtonDecoyOne";
            this.radioButtonDecoyOne.Size = new System.Drawing.Size(65, 17);
            this.radioButtonDecoyOne.TabIndex = 7;
            this.radioButtonDecoyOne.TabStop = true;
            this.radioButtonDecoyOne.Text = "Decoy 1";
            this.radioButtonDecoyOne.UseVisualStyleBackColor = true;
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
            // 
            // panelDecoyPrefix
            // 
            this.panelDecoyPrefix.Controls.Add(this.textBoxDecoyPrefix);
            this.panelDecoyPrefix.Controls.Add(this.labelDecoyPrefix);
            this.panelDecoyPrefix.Location = new System.Drawing.Point(77, 21);
            this.panelDecoyPrefix.Name = "panelDecoyPrefix";
            this.panelDecoyPrefix.Size = new System.Drawing.Size(195, 28);
            this.panelDecoyPrefix.TabIndex = 9;
            // 
            // textBoxDecoyPrefix
            // 
            this.textBoxDecoyPrefix.Location = new System.Drawing.Point(76, 3);
            this.textBoxDecoyPrefix.Name = "textBoxDecoyPrefix";
            this.textBoxDecoyPrefix.Size = new System.Drawing.Size(74, 20);
            this.textBoxDecoyPrefix.TabIndex = 10;
            this.textBoxDecoyPrefix.Text = "DECOY_";
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
            // protDbLabel
            // 
            this.protDbLabel.AutoSize = true;
            this.protDbLabel.Location = new System.Drawing.Point(7, 174);
            this.protDbLabel.Name = "protDbLabel";
            this.protDbLabel.Size = new System.Drawing.Size(139, 13);
            this.protDbLabel.TabIndex = 8;
            this.protDbLabel.Text = "&Proteome Database (.fasta):";
            // 
            // btnRemInputFile
            // 
            this.btnRemInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnRemInputFile.Enabled = false;
            this.btnRemInputFile.Location = new System.Drawing.Point(440, 56);
            this.btnRemInputFile.Name = "btnRemInputFile";
            this.btnRemInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnRemInputFile.TabIndex = 2;
            this.btnRemInputFile.Text = "&Remove";
            this.btnRemInputFile.UseVisualStyleBackColor = true;
            // 
            // btnBrowseProteomeDbFile
            // 
            this.btnBrowseProteomeDbFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowseProteomeDbFile.Enabled = false;
            this.btnBrowseProteomeDbFile.Location = new System.Drawing.Point(440, 189);
            this.btnBrowseProteomeDbFile.Name = "btnBrowseProteomeDbFile";
            this.btnBrowseProteomeDbFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowseProteomeDbFile.TabIndex = 4;
            this.btnBrowseProteomeDbFile.Text = "&Browse";
            this.btnBrowseProteomeDbFile.UseVisualStyleBackColor = true;
            // 
            // btnAddInputFile
            // 
            this.btnAddInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnAddInputFile.Location = new System.Drawing.Point(440, 27);
            this.btnAddInputFile.Name = "btnAddInputFile";
            this.btnAddInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnAddInputFile.TabIndex = 1;
            this.btnAddInputFile.Text = "&Add...";
            this.btnAddInputFile.UseVisualStyleBackColor = true;
            // 
            // proteomeDbFileCombo
            // 
            this.proteomeDbFileCombo.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.proteomeDbFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.proteomeDbFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.proteomeDbFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.proteomeDbFileCombo.FormattingEnabled = true;
            this.proteomeDbFileCombo.Location = new System.Drawing.Point(7, 190);
            this.proteomeDbFileCombo.Name = "proteomeDbFileCombo";
            this.proteomeDbFileCombo.Size = new System.Drawing.Size(427, 23);
            this.proteomeDbFileCombo.TabIndex = 3;
            // 
            // inputFilesLabel
            // 
            this.inputFilesLabel.AutoSize = true;
            this.inputFilesLabel.Location = new System.Drawing.Point(7, 11);
            this.inputFilesLabel.Name = "inputFilesLabel";
            this.inputFilesLabel.Size = new System.Drawing.Size(58, 13);
            this.inputFilesLabel.TabIndex = 1;
            this.inputFilesLabel.Text = "&Input Files:";
            // 
            // inputFilesListBox
            // 
            this.inputFilesListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.inputFilesListBox.FormattingEnabled = true;
            this.inputFilesListBox.Location = new System.Drawing.Point(7, 27);
            this.inputFilesListBox.Name = "inputFilesListBox";
            this.inputFilesListBox.Size = new System.Drawing.Size(427, 121);
            this.inputFilesListBox.TabIndex = 0;
            // 
            // enzymeTabPage
            // 
            this.enzymeTabPage.Location = new System.Drawing.Point(4, 22);
            this.enzymeTabPage.Name = "enzymeTabPage";
            this.enzymeTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.enzymeTabPage.Size = new System.Drawing.Size(527, 330);
            this.enzymeTabPage.TabIndex = 1;
            this.enzymeTabPage.Text = "Enzyme";
            this.enzymeTabPage.UseVisualStyleBackColor = true;
            // 
            // tabPage1
            // 
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(527, 330);
            this.tabPage1.TabIndex = 2;
            this.tabPage1.Text = "Masses";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(527, 330);
            this.tabPage2.TabIndex = 3;
            this.tabPage2.Text = "Mods";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // tabPage3
            // 
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(527, 330);
            this.tabPage3.TabIndex = 4;
            this.tabPage3.Text = "Output";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // tabPage4
            // 
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage4.Size = new System.Drawing.Size(527, 330);
            this.tabPage4.TabIndex = 5;
            this.tabPage4.Text = "Misc";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // btnLoadParams
            // 
            this.btnLoadParams.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnLoadParams.Location = new System.Drawing.Point(301, 0);
            this.btnLoadParams.Name = "btnLoadParams";
            this.btnLoadParams.Size = new System.Drawing.Size(78, 23);
            this.btnLoadParams.TabIndex = 16;
            this.btnLoadParams.Text = "Load Params";
            this.btnLoadParams.UseVisualStyleBackColor = true;
            // 
            // btnSaveParams
            // 
            this.btnSaveParams.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSaveParams.Location = new System.Drawing.Point(385, 0);
            this.btnSaveParams.Name = "btnSaveParams";
            this.btnSaveParams.Size = new System.Drawing.Size(78, 23);
            this.btnSaveParams.TabIndex = 17;
            this.btnSaveParams.Text = "Save Params";
            this.btnSaveParams.UseVisualStyleBackColor = true;
            // 
            // statusStripMain
            // 
            this.statusStripMain.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
            this.statusStripMain.Location = new System.Drawing.Point(0, 29);
            this.statusStripMain.Name = "statusStripMain";
            this.statusStripMain.Size = new System.Drawing.Size(559, 22);
            this.statusStripMain.TabIndex = 19;
            this.statusStripMain.Text = "statusStrip";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(218, 17);
            this.toolStripStatusLabel1.Text = "This is where search status will show up.";
            // 
            // CometMainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(559, 437);
            this.Controls.Add(this.mainSplitContainer);
            this.Controls.Add(this.menuStripMain);
            this.MainMenuStrip = this.menuStripMain;
            this.Name = "CometMainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Comet";
            this.menuStripMain.ResumeLayout(false);
            this.menuStripMain.PerformLayout();
            this.mainSplitContainer.Panel1.ResumeLayout(false);
            this.mainSplitContainer.Panel2.ResumeLayout(false);
            this.mainSplitContainer.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).EndInit();
            this.mainSplitContainer.ResumeLayout(false);
            this.cometMainTab.ResumeLayout(false);
            this.inputFilesTabPage.ResumeLayout(false);
            this.inputFilesTabPage.PerformLayout();
            this.panelProteinNucleotide.ResumeLayout(false);
            this.panelProteinNucleotide.PerformLayout();
            this.panelNucleotideReadingFrame.ResumeLayout(false);
            this.panelNucleotideReadingFrame.PerformLayout();
            this.panelTargetDecoy.ResumeLayout(false);
            this.panelTargetDecoy.PerformLayout();
            this.panelDecoyPrefix.ResumeLayout(false);
            this.panelDecoyPrefix.PerformLayout();
            this.statusStripMain.ResumeLayout(false);
            this.statusStripMain.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnSearch;
        private System.Windows.Forms.MenuStrip menuStripMain;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemHelp;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.SplitContainer mainSplitContainer;
        private System.Windows.Forms.ListBox inputFilesListBox;
        private System.Windows.Forms.Label inputFilesLabel;
        private System.Windows.Forms.Button btnRemInputFile;
        private System.Windows.Forms.Button btnAddInputFile;
        private System.Windows.Forms.Label protDbLabel;
        private System.Windows.Forms.ComboBox proteomeDbFileCombo;
        private System.Windows.Forms.Button btnBrowseProteomeDbFile;
        private System.Windows.Forms.TabControl cometMainTab;
        private System.Windows.Forms.TabPage inputFilesTabPage;
        private System.Windows.Forms.TabPage enzymeTabPage;
        private System.Windows.Forms.StatusStrip statusStripMain;
        private System.Windows.Forms.RadioButton radioButtonDecoyTwo;
        private System.Windows.Forms.RadioButton radioButtonDecoyOne;
        private System.Windows.Forms.RadioButton radioButtonTarget;
        private System.Windows.Forms.Panel panelDecoyPrefix;
        private System.Windows.Forms.RadioButton radioButtonProtein;
        private System.Windows.Forms.RadioButton radioButtonNucleotide;
        private System.Windows.Forms.Label labelDecoyPrefix;
        private System.Windows.Forms.TextBox textBoxDecoyPrefix;
        private System.Windows.Forms.Panel panelNucleotideReadingFrame;
        private System.Windows.Forms.Label labelReadingFrame;
        private System.Windows.Forms.ComboBox comboBoxReadingFrame;
        private System.Windows.Forms.Panel panelProteinNucleotide;
        private System.Windows.Forms.Panel panelTargetDecoy;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.TabPage tabPage4;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.Button btnSaveParams;
        private System.Windows.Forms.Button btnLoadParams;
    }
}

