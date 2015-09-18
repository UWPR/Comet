namespace CometUI.Search
{
    partial class ImportSearchParamsDlg
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ImportSearchParamsDlg));
            this.btnCancel = new System.Windows.Forms.Button();
            this.paramsFileCombo = new System.Windows.Forms.ComboBox();
            this.btnBrowseParams = new System.Windows.Forms.Button();
            this.paramsDbLabel = new System.Windows.Forms.Label();
            this.btnImport = new System.Windows.Forms.Button();
            this.btnSettings = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.Location = new System.Drawing.Point(381, 90);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 103;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // paramsFileCombo
            // 
            this.paramsFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.paramsFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.paramsFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.paramsFileCombo.FormattingEnabled = true;
            this.paramsFileCombo.Location = new System.Drawing.Point(15, 32);
            this.paramsFileCombo.Name = "paramsFileCombo";
            this.paramsFileCombo.Size = new System.Drawing.Size(411, 23);
            this.paramsFileCombo.TabIndex = 100;
            this.paramsFileCombo.SelectedIndexChanged += new System.EventHandler(this.ParamsFileComboSelectedIndexChanged);
            this.paramsFileCombo.TextChanged += new System.EventHandler(this.ParamsFileComboTextChanged);
            // 
            // btnBrowseParams
            // 
            this.btnBrowseParams.BackColor = System.Drawing.Color.Transparent;
            this.btnBrowseParams.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnBrowseParams.BackgroundImage")));
            this.btnBrowseParams.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.btnBrowseParams.Location = new System.Drawing.Point(432, 31);
            this.btnBrowseParams.Name = "btnBrowseParams";
            this.btnBrowseParams.Size = new System.Drawing.Size(24, 24);
            this.btnBrowseParams.TabIndex = 101;
            this.btnBrowseParams.UseVisualStyleBackColor = false;
            this.btnBrowseParams.Click += new System.EventHandler(this.BtnBrowseParamsClick);
            // 
            // paramsDbLabel
            // 
            this.paramsDbLabel.AutoSize = true;
            this.paramsDbLabel.Location = new System.Drawing.Point(12, 16);
            this.paramsDbLabel.Name = "paramsDbLabel";
            this.paramsDbLabel.Size = new System.Drawing.Size(128, 13);
            this.paramsDbLabel.TabIndex = 43;
            this.paramsDbLabel.Text = "&Parameters File (.params):";
            // 
            // btnImport
            // 
            this.btnImport.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnImport.Enabled = false;
            this.btnImport.Location = new System.Drawing.Point(300, 90);
            this.btnImport.Name = "btnImport";
            this.btnImport.Size = new System.Drawing.Size(75, 23);
            this.btnImport.TabIndex = 102;
            this.btnImport.Text = "&Import";
            this.btnImport.UseVisualStyleBackColor = true;
            this.btnImport.Click += new System.EventHandler(this.BtnImportClick);
            // 
            // btnSettings
            // 
            this.btnSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSettings.Location = new System.Drawing.Point(219, 90);
            this.btnSettings.Name = "btnSettings";
            this.btnSettings.Size = new System.Drawing.Size(75, 23);
            this.btnSettings.TabIndex = 104;
            this.btnSettings.Text = "&Settings...";
            this.btnSettings.UseVisualStyleBackColor = true;
            this.btnSettings.Click += new System.EventHandler(this.BtnSettingsClick);
            // 
            // ImportSearchParamsDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(465, 125);
            this.Controls.Add(this.btnSettings);
            this.Controls.Add(this.btnImport);
            this.Controls.Add(this.btnBrowseParams);
            this.Controls.Add(this.paramsDbLabel);
            this.Controls.Add(this.paramsFileCombo);
            this.Controls.Add(this.btnCancel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ImportSearchParamsDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Import Search Settings";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ComboBox paramsFileCombo;
        private System.Windows.Forms.Button btnBrowseParams;
        private System.Windows.Forms.Label paramsDbLabel;
        private System.Windows.Forms.Button btnImport;
        private System.Windows.Forms.Button btnSettings;
    }
}