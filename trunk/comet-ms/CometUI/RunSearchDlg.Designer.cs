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
            this.SuspendLayout();
            // 
            // btnSettings
            // 
            this.btnSettings.Location = new System.Drawing.Point(338, 262);
            this.btnSettings.Name = "btnSettings";
            this.btnSettings.Size = new System.Drawing.Size(75, 23);
            this.btnSettings.TabIndex = 0;
            this.btnSettings.Text = "&Settings...";
            this.btnSettings.UseVisualStyleBackColor = true;
            this.btnSettings.Click += new System.EventHandler(this.BtnSettingsClick);
            // 
            // inputFilesLabel
            // 
            this.inputFilesLabel.AutoSize = true;
            this.inputFilesLabel.Location = new System.Drawing.Point(12, 17);
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
            this.inputFilesList.Location = new System.Drawing.Point(12, 33);
            this.inputFilesList.Name = "inputFilesList";
            this.inputFilesList.Size = new System.Drawing.Size(401, 184);
            this.inputFilesList.TabIndex = 35;
            // 
            // btnRemInputFile
            // 
            this.btnRemInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnRemInputFile.Enabled = false;
            this.btnRemInputFile.Location = new System.Drawing.Point(419, 62);
            this.btnRemInputFile.Name = "btnRemInputFile";
            this.btnRemInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnRemInputFile.TabIndex = 38;
            this.btnRemInputFile.Text = "Re&move";
            this.btnRemInputFile.UseVisualStyleBackColor = true;
            // 
            // btnAddInputFile
            // 
            this.btnAddInputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnAddInputFile.Location = new System.Drawing.Point(419, 33);
            this.btnAddInputFile.Name = "btnAddInputFile";
            this.btnAddInputFile.Size = new System.Drawing.Size(75, 23);
            this.btnAddInputFile.TabIndex = 36;
            this.btnAddInputFile.Text = "&Add...";
            this.btnAddInputFile.UseVisualStyleBackColor = true;
            // 
            // btnRunSearch
            // 
            this.btnRunSearch.Location = new System.Drawing.Point(419, 262);
            this.btnRunSearch.Name = "btnRunSearch";
            this.btnRunSearch.Size = new System.Drawing.Size(75, 23);
            this.btnRunSearch.TabIndex = 39;
            this.btnRunSearch.Text = "&Run";
            this.btnRunSearch.UseVisualStyleBackColor = true;
            // 
            // RunSearchDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(506, 299);
            this.Controls.Add(this.btnRunSearch);
            this.Controls.Add(this.inputFilesLabel);
            this.Controls.Add(this.inputFilesList);
            this.Controls.Add(this.btnRemInputFile);
            this.Controls.Add(this.btnAddInputFile);
            this.Controls.Add(this.btnSettings);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "RunSearchDlg";
            this.Text = "Run Search";
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
    }
}