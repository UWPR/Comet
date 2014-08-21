namespace CometUI
{
    partial class RunSearchDlg
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(RunSearchDlg));
            this.btnSettings = new System.Windows.Forms.Button();
            this.inputFilesLabel = new System.Windows.Forms.Label();
            this.inputFilesList = new System.Windows.Forms.CheckedListBox();
            this.btnRemInputFile = new System.Windows.Forms.Button();
            this.btnAddInputFile = new System.Windows.Forms.Button();
            this.btnRunSearch = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnBrowseProteomeDbFile = new System.Windows.Forms.Button();
            this.proteomeDbFileCombo = new System.Windows.Forms.ComboBox();
            this.protDbLabel = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // btnSettings
            // 
            this.btnSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSettings.Location = new System.Drawing.Point(211, 255);
            this.btnSettings.Name = "btnSettings";
            this.btnSettings.Size = new System.Drawing.Size(75, 23);
            this.btnSettings.TabIndex = 4;
            this.btnSettings.Text = "&Settings...";
            this.btnSettings.UseVisualStyleBackColor = true;
            this.btnSettings.Click += new System.EventHandler(this.BtnSettingsClick);
            // 
            // inputFilesLabel
            // 
            this.inputFilesLabel.AutoSize = true;
            this.inputFilesLabel.Location = new System.Drawing.Point(12, 77);
            this.inputFilesLabel.Name = "inputFilesLabel";
            this.inputFilesLabel.Size = new System.Drawing.Size(58, 13);
            this.inputFilesLabel.TabIndex = 37;
            this.inputFilesLabel.Text = "&Input Files:";
            // 
            // inputFilesList
            // 
            this.inputFilesList.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.inputFilesList.CheckOnClick = true;
            this.inputFilesList.FormattingEnabled = true;
            this.inputFilesList.HorizontalScrollbar = true;
            this.inputFilesList.Location = new System.Drawing.Point(12, 93);
            this.inputFilesList.Name = "inputFilesList";
            this.inputFilesList.Size = new System.Drawing.Size(355, 124);
            this.inputFilesList.TabIndex = 1;
            this.inputFilesList.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.InputFilesListItemCheck);
            // 
            // btnRemInputFile
            // 
            this.btnRemInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnRemInputFile.Enabled = false;
            this.btnRemInputFile.Location = new System.Drawing.Point(373, 122);
            this.btnRemInputFile.Name = "btnRemInputFile";
            this.btnRemInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnRemInputFile.TabIndex = 3;
            this.btnRemInputFile.Text = "Re&move";
            this.btnRemInputFile.UseVisualStyleBackColor = true;
            this.btnRemInputFile.Click += new System.EventHandler(this.BtnRemInputFileClick);
            // 
            // btnAddInputFile
            // 
            this.btnAddInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnAddInputFile.Location = new System.Drawing.Point(373, 93);
            this.btnAddInputFile.Name = "btnAddInputFile";
            this.btnAddInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnAddInputFile.TabIndex = 2;
            this.btnAddInputFile.Text = "&Add...";
            this.btnAddInputFile.UseVisualStyleBackColor = true;
            this.btnAddInputFile.Click += new System.EventHandler(this.BtnAddInputFileClick);
            // 
            // btnRunSearch
            // 
            this.btnRunSearch.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnRunSearch.Enabled = false;
            this.btnRunSearch.Location = new System.Drawing.Point(292, 255);
            this.btnRunSearch.Name = "btnRunSearch";
            this.btnRunSearch.Size = new System.Drawing.Size(75, 23);
            this.btnRunSearch.TabIndex = 5;
            this.btnRunSearch.Text = "&Run";
            this.btnRunSearch.UseVisualStyleBackColor = true;
            this.btnRunSearch.Click += new System.EventHandler(this.BtnRunSearchClick);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.Location = new System.Drawing.Point(373, 255);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // btnBrowseProteomeDbFile
            // 
            this.btnBrowseProteomeDbFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowseProteomeDbFile.Location = new System.Drawing.Point(373, 35);
            this.btnBrowseProteomeDbFile.Name = "btnBrowseProteomeDbFile";
            this.btnBrowseProteomeDbFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowseProteomeDbFile.TabIndex = 40;
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
            this.proteomeDbFileCombo.Location = new System.Drawing.Point(15, 35);
            this.proteomeDbFileCombo.Name = "proteomeDbFileCombo";
            this.proteomeDbFileCombo.Size = new System.Drawing.Size(352, 23);
            this.proteomeDbFileCombo.TabIndex = 39;
            this.proteomeDbFileCombo.SelectedIndexChanged += new System.EventHandler(this.ProteomeDbFileComboSelectedIndexChanged);
            this.proteomeDbFileCombo.TextUpdate += new System.EventHandler(this.ProteomeDbFileComboTextUpdate);
            // 
            // protDbLabel
            // 
            this.protDbLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.protDbLabel.AutoSize = true;
            this.protDbLabel.Location = new System.Drawing.Point(12, 19);
            this.protDbLabel.Name = "protDbLabel";
            this.protDbLabel.Size = new System.Drawing.Size(139, 13);
            this.protDbLabel.TabIndex = 41;
            this.protDbLabel.Text = "&Proteome Database (.fasta):";
            // 
            // RunSearchDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(460, 290);
            this.Controls.Add(this.btnBrowseProteomeDbFile);
            this.Controls.Add(this.proteomeDbFileCombo);
            this.Controls.Add(this.protDbLabel);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnRunSearch);
            this.Controls.Add(this.inputFilesLabel);
            this.Controls.Add(this.inputFilesList);
            this.Controls.Add(this.btnRemInputFile);
            this.Controls.Add(this.btnAddInputFile);
            this.Controls.Add(this.btnSettings);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "RunSearchDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Run Search";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.RunSearchDlgFormClosing);
            this.Load += new System.EventHandler(this.RunSearchDlgLoad);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnSettings;
        private System.Windows.Forms.Label inputFilesLabel;
        private System.Windows.Forms.CheckedListBox inputFilesList;
        private System.Windows.Forms.Button btnRemInputFile;
        private System.Windows.Forms.Button btnAddInputFile;
        private System.Windows.Forms.Button btnRunSearch;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnBrowseProteomeDbFile;
        private System.Windows.Forms.ComboBox proteomeDbFileCombo;
        private System.Windows.Forms.Label protDbLabel;
    }
}