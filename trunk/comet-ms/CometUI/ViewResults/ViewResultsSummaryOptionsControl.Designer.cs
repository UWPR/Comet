namespace CometUI.ViewResults
{
    partial class ViewResultsSummaryOptionsControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ViewResultsSummaryOptionsControl));
            this.btnBrowsePepXMLFile = new System.Windows.Forms.Button();
            this.pepXMLFileCombo = new System.Windows.Forms.ComboBox();
            this.pepXMLFileLabel = new System.Windows.Forms.Label();
            this.btnUpdateResults = new System.Windows.Forms.Button();
            this.viewResultsSummaryMainPanel = new System.Windows.Forms.Panel();
            this.searchResultsSummaryLabel = new System.Windows.Forms.Label();
            this.viewResultsSummaryMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnBrowsePepXMLFile
            // 
            this.btnBrowsePepXMLFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowsePepXMLFile.BackColor = System.Drawing.Color.Transparent;
            this.btnBrowsePepXMLFile.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnBrowsePepXMLFile.BackgroundImage")));
            this.btnBrowsePepXMLFile.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.btnBrowsePepXMLFile.Location = new System.Drawing.Point(959, 39);
            this.btnBrowsePepXMLFile.Name = "btnBrowsePepXMLFile";
            this.btnBrowsePepXMLFile.Size = new System.Drawing.Size(24, 24);
            this.btnBrowsePepXMLFile.TabIndex = 42;
            this.btnBrowsePepXMLFile.UseVisualStyleBackColor = false;
            this.btnBrowsePepXMLFile.Click += new System.EventHandler(this.BtnBrowsePepXMLFileClick);
            // 
            // pepXMLFileCombo
            // 
            this.pepXMLFileCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pepXMLFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.pepXMLFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.pepXMLFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.pepXMLFileCombo.FormattingEnabled = true;
            this.pepXMLFileCombo.Location = new System.Drawing.Point(93, 40);
            this.pepXMLFileCombo.Name = "pepXMLFileCombo";
            this.pepXMLFileCombo.Size = new System.Drawing.Size(860, 23);
            this.pepXMLFileCombo.TabIndex = 41;
            // 
            // pepXMLFileLabel
            // 
            this.pepXMLFileLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.pepXMLFileLabel.AutoSize = true;
            this.pepXMLFileLabel.Location = new System.Drawing.Point(18, 45);
            this.pepXMLFileLabel.Name = "pepXMLFileLabel";
            this.pepXMLFileLabel.Size = new System.Drawing.Size(69, 13);
            this.pepXMLFileLabel.TabIndex = 43;
            this.pepXMLFileLabel.Text = "&pepXML File:";
            // 
            // btnUpdateResults
            // 
            this.btnUpdateResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpdateResults.Location = new System.Drawing.Point(908, 163);
            this.btnUpdateResults.Name = "btnUpdateResults";
            this.btnUpdateResults.Size = new System.Drawing.Size(75, 23);
            this.btnUpdateResults.TabIndex = 44;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            this.btnUpdateResults.Click += new System.EventHandler(this.BtnUpdateResultsClick);
            // 
            // viewResultsSummaryMainPanel
            // 
            this.viewResultsSummaryMainPanel.Controls.Add(this.searchResultsSummaryLabel);
            this.viewResultsSummaryMainPanel.Controls.Add(this.btnUpdateResults);
            this.viewResultsSummaryMainPanel.Controls.Add(this.btnBrowsePepXMLFile);
            this.viewResultsSummaryMainPanel.Controls.Add(this.pepXMLFileLabel);
            this.viewResultsSummaryMainPanel.Controls.Add(this.pepXMLFileCombo);
            this.viewResultsSummaryMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewResultsSummaryMainPanel.Location = new System.Drawing.Point(0, 0);
            this.viewResultsSummaryMainPanel.Name = "viewResultsSummaryMainPanel";
            this.viewResultsSummaryMainPanel.Size = new System.Drawing.Size(1000, 210);
            this.viewResultsSummaryMainPanel.TabIndex = 46;
            // 
            // searchResultsSummaryLabel
            // 
            this.searchResultsSummaryLabel.AutoSize = true;
            this.searchResultsSummaryLabel.Location = new System.Drawing.Point(18, 122);
            this.searchResultsSummaryLabel.Name = "searchResultsSummaryLabel";
            this.searchResultsSummaryLabel.Size = new System.Drawing.Size(35, 13);
            this.searchResultsSummaryLabel.TabIndex = 45;
            this.searchResultsSummaryLabel.Text = "label1";
            // 
            // ViewResultsSummaryOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.viewResultsSummaryMainPanel);
            this.Name = "ViewResultsSummaryOptionsControl";
            this.Size = new System.Drawing.Size(1000, 210);
            this.viewResultsSummaryMainPanel.ResumeLayout(false);
            this.viewResultsSummaryMainPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnBrowsePepXMLFile;
        private System.Windows.Forms.ComboBox pepXMLFileCombo;
        private System.Windows.Forms.Label pepXMLFileLabel;
        private System.Windows.Forms.Button btnUpdateResults;
        private System.Windows.Forms.Panel viewResultsSummaryMainPanel;
        private System.Windows.Forms.Label searchResultsSummaryLabel;
    }
}
