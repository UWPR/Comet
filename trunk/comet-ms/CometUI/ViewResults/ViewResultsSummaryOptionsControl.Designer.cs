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
            this.btnBrowsePepXMLFile = new System.Windows.Forms.Button();
            this.pepXMLFileCombo = new System.Windows.Forms.ComboBox();
            this.pepXMLFileLabel = new System.Windows.Forms.Label();
            this.btnUpdateResults = new System.Windows.Forms.Button();
            this.searchSummaryLabel = new System.Windows.Forms.Label();
            this.viewResultsSummaryMainPanel = new System.Windows.Forms.Panel();
            this.viewResultsSummaryMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnBrowsePepXMLFile
            // 
            this.btnBrowsePepXMLFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowsePepXMLFile.Location = new System.Drawing.Point(561, 32);
            this.btnBrowsePepXMLFile.Name = "btnBrowsePepXMLFile";
            this.btnBrowsePepXMLFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowsePepXMLFile.TabIndex = 42;
            this.btnBrowsePepXMLFile.Text = "&Browse";
            this.btnBrowsePepXMLFile.UseVisualStyleBackColor = true;
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
            this.pepXMLFileCombo.Location = new System.Drawing.Point(93, 32);
            this.pepXMLFileCombo.Name = "pepXMLFileCombo";
            this.pepXMLFileCombo.Size = new System.Drawing.Size(462, 23);
            this.pepXMLFileCombo.TabIndex = 41;
            // 
            // pepXMLFileLabel
            // 
            this.pepXMLFileLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.pepXMLFileLabel.AutoSize = true;
            this.pepXMLFileLabel.Location = new System.Drawing.Point(18, 37);
            this.pepXMLFileLabel.Name = "pepXMLFileLabel";
            this.pepXMLFileLabel.Size = new System.Drawing.Size(69, 13);
            this.pepXMLFileLabel.TabIndex = 43;
            this.pepXMLFileLabel.Text = "&pepXML File:";
            // 
            // btnUpdateResults
            // 
            this.btnUpdateResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpdateResults.Location = new System.Drawing.Point(569, 200);
            this.btnUpdateResults.Name = "btnUpdateResults";
            this.btnUpdateResults.Size = new System.Drawing.Size(75, 23);
            this.btnUpdateResults.TabIndex = 44;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            this.btnUpdateResults.Click += new System.EventHandler(this.BtnUpdateResultsClick);
            // 
            // searchSummaryLabel
            // 
            this.searchSummaryLabel.AutoSize = true;
            this.searchSummaryLabel.Location = new System.Drawing.Point(18, 66);
            this.searchSummaryLabel.Name = "searchSummaryLabel";
            this.searchSummaryLabel.Size = new System.Drawing.Size(37, 13);
            this.searchSummaryLabel.TabIndex = 45;
            this.searchSummaryLabel.Text = "          ";
            // 
            // viewResultsSummaryMainPanel
            // 
            this.viewResultsSummaryMainPanel.Controls.Add(this.btnUpdateResults);
            this.viewResultsSummaryMainPanel.Controls.Add(this.btnBrowsePepXMLFile);
            this.viewResultsSummaryMainPanel.Controls.Add(this.pepXMLFileLabel);
            this.viewResultsSummaryMainPanel.Controls.Add(this.pepXMLFileCombo);
            this.viewResultsSummaryMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewResultsSummaryMainPanel.Location = new System.Drawing.Point(0, 0);
            this.viewResultsSummaryMainPanel.Name = "viewResultsSummaryMainPanel";
            this.viewResultsSummaryMainPanel.Size = new System.Drawing.Size(653, 230);
            this.viewResultsSummaryMainPanel.TabIndex = 46;
            // 
            // ViewResultsSummaryOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.viewResultsSummaryMainPanel);
            this.Controls.Add(this.searchSummaryLabel);
            this.Name = "ViewResultsSummaryOptionsControl";
            this.Size = new System.Drawing.Size(653, 230);
            this.viewResultsSummaryMainPanel.ResumeLayout(false);
            this.viewResultsSummaryMainPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnBrowsePepXMLFile;
        private System.Windows.Forms.ComboBox pepXMLFileCombo;
        private System.Windows.Forms.Label pepXMLFileLabel;
        private System.Windows.Forms.Button btnUpdateResults;
        private System.Windows.Forms.Label searchSummaryLabel;
        private System.Windows.Forms.Panel viewResultsSummaryMainPanel;
    }
}
