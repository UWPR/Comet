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
            this.BtnTest = new System.Windows.Forms.Button();
            this.mainMenuStrip = new System.Windows.Forms.MenuStrip();
            this.toolStripMenuItemHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.settingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.searchSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
            this.inputFilesListBox = new System.Windows.Forms.ListBox();
            this.inputFilesLabel = new System.Windows.Forms.Label();
            this.btnRemFile = new System.Windows.Forms.Button();
            this.btnAddFile = new System.Windows.Forms.Button();
            this.proteomeDbFileCombo = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.btnBrowseProteomeDbFile = new System.Windows.Forms.Button();
            this.mainMenuStrip.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).BeginInit();
            this.mainSplitContainer.Panel1.SuspendLayout();
            this.mainSplitContainer.Panel2.SuspendLayout();
            this.mainSplitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // BtnTest
            // 
            this.BtnTest.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.BtnTest.Location = new System.Drawing.Point(462, 12);
            this.BtnTest.Name = "BtnTest";
            this.BtnTest.Size = new System.Drawing.Size(75, 23);
            this.BtnTest.TabIndex = 0;
            this.BtnTest.Text = "Test";
            this.BtnTest.UseVisualStyleBackColor = true;
            this.BtnTest.Click += new System.EventHandler(this.BtnTestClick);
            // 
            // mainMenuStrip
            // 
            this.mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.settingsToolStripMenuItem,
            this.toolStripMenuItemHelp});
            this.mainMenuStrip.Location = new System.Drawing.Point(0, 0);
            this.mainMenuStrip.Name = "mainMenuStrip";
            this.mainMenuStrip.Size = new System.Drawing.Size(559, 24);
            this.mainMenuStrip.TabIndex = 1;
            this.mainMenuStrip.Text = "Main Menu";
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
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.aboutToolStripMenuItem.Text = "&About";
            // 
            // settingsToolStripMenuItem
            // 
            this.settingsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.searchSettingsToolStripMenuItem});
            this.settingsToolStripMenuItem.Name = "settingsToolStripMenuItem";
            this.settingsToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
            this.settingsToolStripMenuItem.Text = "&Settings";
            // 
            // searchSettingsToolStripMenuItem
            // 
            this.searchSettingsToolStripMenuItem.Name = "searchSettingsToolStripMenuItem";
            this.searchSettingsToolStripMenuItem.Size = new System.Drawing.Size(171, 22);
            this.searchSettingsToolStripMenuItem.Text = "&Search Parameters";
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
            this.mainSplitContainer.Panel1.Controls.Add(this.btnBrowseProteomeDbFile);
            this.mainSplitContainer.Panel1.Controls.Add(this.label1);
            this.mainSplitContainer.Panel1.Controls.Add(this.proteomeDbFileCombo);
            this.mainSplitContainer.Panel1.Controls.Add(this.btnRemFile);
            this.mainSplitContainer.Panel1.Controls.Add(this.btnAddFile);
            this.mainSplitContainer.Panel1.Controls.Add(this.inputFilesLabel);
            this.mainSplitContainer.Panel1.Controls.Add(this.inputFilesListBox);
            // 
            // mainSplitContainer.Panel2
            // 
            this.mainSplitContainer.Panel2.Controls.Add(this.BtnTest);
            this.mainSplitContainer.Size = new System.Drawing.Size(559, 413);
            this.mainSplitContainer.SplitterDistance = 362;
            this.mainSplitContainer.TabIndex = 2;
            // 
            // inputFilesListBox
            // 
            this.inputFilesListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.inputFilesListBox.FormattingEnabled = true;
            this.inputFilesListBox.Location = new System.Drawing.Point(12, 31);
            this.inputFilesListBox.Name = "inputFilesListBox";
            this.inputFilesListBox.Size = new System.Drawing.Size(444, 134);
            this.inputFilesListBox.TabIndex = 0;
            // 
            // inputFilesLabel
            // 
            this.inputFilesLabel.AutoSize = true;
            this.inputFilesLabel.Location = new System.Drawing.Point(12, 12);
            this.inputFilesLabel.Name = "inputFilesLabel";
            this.inputFilesLabel.Size = new System.Drawing.Size(58, 13);
            this.inputFilesLabel.TabIndex = 1;
            this.inputFilesLabel.Text = "&Input Files:";
            // 
            // btnRemFile
            // 
            this.btnRemFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnRemFile.Enabled = false;
            this.btnRemFile.Location = new System.Drawing.Point(462, 60);
            this.btnRemFile.Name = "btnRemFile";
            this.btnRemFile.Size = new System.Drawing.Size(75, 23);
            this.btnRemFile.TabIndex = 5;
            this.btnRemFile.Text = "&Remove";
            this.btnRemFile.UseVisualStyleBackColor = true;
            // 
            // btnAddFile
            // 
            this.btnAddFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnAddFile.Location = new System.Drawing.Point(462, 31);
            this.btnAddFile.Name = "btnAddFile";
            this.btnAddFile.Size = new System.Drawing.Size(75, 23);
            this.btnAddFile.TabIndex = 4;
            this.btnAddFile.Text = "&Add...";
            this.btnAddFile.UseVisualStyleBackColor = true;
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
            this.proteomeDbFileCombo.Location = new System.Drawing.Point(12, 194);
            this.proteomeDbFileCombo.Name = "proteomeDbFileCombo";
            this.proteomeDbFileCombo.Size = new System.Drawing.Size(444, 23);
            this.proteomeDbFileCombo.TabIndex = 6;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 178);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(139, 13);
            this.label1.TabIndex = 8;
            this.label1.Text = "&Proteome Database (.fasta):";
            // 
            // btnBrowseProteomeDbFile
            // 
            this.btnBrowseProteomeDbFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowseProteomeDbFile.Enabled = false;
            this.btnBrowseProteomeDbFile.Location = new System.Drawing.Point(462, 193);
            this.btnBrowseProteomeDbFile.Name = "btnBrowseProteomeDbFile";
            this.btnBrowseProteomeDbFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowseProteomeDbFile.TabIndex = 9;
            this.btnBrowseProteomeDbFile.Text = "&Browse";
            this.btnBrowseProteomeDbFile.UseVisualStyleBackColor = true;
            // 
            // CometMainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(559, 437);
            this.Controls.Add(this.mainSplitContainer);
            this.Controls.Add(this.mainMenuStrip);
            this.MainMenuStrip = this.mainMenuStrip;
            this.Name = "CometMainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Comet";
            this.mainMenuStrip.ResumeLayout(false);
            this.mainMenuStrip.PerformLayout();
            this.mainSplitContainer.Panel1.ResumeLayout(false);
            this.mainSplitContainer.Panel1.PerformLayout();
            this.mainSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).EndInit();
            this.mainSplitContainer.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button BtnTest;
        private System.Windows.Forms.MenuStrip mainMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemHelp;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem settingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem;
        private System.Windows.Forms.SplitContainer mainSplitContainer;
        private System.Windows.Forms.ListBox inputFilesListBox;
        private System.Windows.Forms.Label inputFilesLabel;
        private System.Windows.Forms.Button btnRemFile;
        private System.Windows.Forms.Button btnAddFile;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox proteomeDbFileCombo;
        private System.Windows.Forms.Button btnBrowseProteomeDbFile;
    }
}

