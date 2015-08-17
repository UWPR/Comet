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
            this.textBoxEValueCutoff = new CometUI.SharedUI.NumericTextBox();
            this.eValueCheckBox = new System.Windows.Forms.CheckBox();
            this.resultsListSummaryLabel = new System.Windows.Forms.Label();
            this.textBoxCustomDecoyPrefix = new System.Windows.Forms.TextBox();
            this.customDecoyPrefixCheckBox = new System.Windows.Forms.CheckBox();
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
            this.btnBrowsePepXMLFile.Location = new System.Drawing.Point(654, 53);
            this.btnBrowsePepXMLFile.Name = "btnBrowsePepXMLFile";
            this.btnBrowsePepXMLFile.Size = new System.Drawing.Size(24, 24);
            this.btnBrowsePepXMLFile.TabIndex = 2;
            this.btnBrowsePepXMLFile.UseVisualStyleBackColor = false;
            this.btnBrowsePepXMLFile.Click += new System.EventHandler(this.BtnBrowsePepXMLFileClick);
            // 
            // pepXMLFileCombo
            // 
            this.pepXMLFileCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pepXMLFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.pepXMLFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.pepXMLFileCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.Simple;
            this.pepXMLFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.pepXMLFileCombo.FormattingEnabled = true;
            this.pepXMLFileCombo.Location = new System.Drawing.Point(21, 54);
            this.pepXMLFileCombo.Name = "pepXMLFileCombo";
            this.pepXMLFileCombo.Size = new System.Drawing.Size(627, 23);
            this.pepXMLFileCombo.TabIndex = 1;
            // 
            // pepXMLFileLabel
            // 
            this.pepXMLFileLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.pepXMLFileLabel.AutoSize = true;
            this.pepXMLFileLabel.Location = new System.Drawing.Point(18, 36);
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
            this.btnUpdateResults.TabIndex = 5;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            this.btnUpdateResults.Click += new System.EventHandler(this.BtnUpdateResultsClick);
            // 
            // viewResultsSummaryMainPanel
            // 
            this.viewResultsSummaryMainPanel.Controls.Add(this.textBoxEValueCutoff);
            this.viewResultsSummaryMainPanel.Controls.Add(this.eValueCheckBox);
            this.viewResultsSummaryMainPanel.Controls.Add(this.resultsListSummaryLabel);
            this.viewResultsSummaryMainPanel.Controls.Add(this.textBoxCustomDecoyPrefix);
            this.viewResultsSummaryMainPanel.Controls.Add(this.customDecoyPrefixCheckBox);
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
            // textBoxEValueCutoff
            // 
            this.textBoxEValueCutoff.AllowDecimal = true;
            this.textBoxEValueCutoff.AllowGroupSeparator = false;
            this.textBoxEValueCutoff.AllowNegative = false;
            this.textBoxEValueCutoff.AllowSpace = false;
            this.textBoxEValueCutoff.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxEValueCutoff.Location = new System.Drawing.Point(849, 89);
            this.textBoxEValueCutoff.Name = "textBoxEValueCutoff";
            this.textBoxEValueCutoff.Size = new System.Drawing.Size(134, 20);
            this.textBoxEValueCutoff.TabIndex = 49;
            this.textBoxEValueCutoff.Text = "10.0";
            // 
            // eValueCheckBox
            // 
            this.eValueCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.eValueCheckBox.AutoSize = true;
            this.eValueCheckBox.Checked = true;
            this.eValueCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.eValueCheckBox.Location = new System.Drawing.Point(728, 91);
            this.eValueCheckBox.Name = "eValueCheckBox";
            this.eValueCheckBox.Size = new System.Drawing.Size(96, 17);
            this.eValueCheckBox.TabIndex = 47;
            this.eValueCheckBox.Text = "E-Value cutoff:";
            this.eValueCheckBox.UseVisualStyleBackColor = true;
            this.eValueCheckBox.CheckedChanged += new System.EventHandler(this.EValueCheckBoxCheckedChanged);
            // 
            // resultsListSummaryLabel
            // 
            this.resultsListSummaryLabel.AutoSize = true;
            this.resultsListSummaryLabel.Location = new System.Drawing.Point(18, 113);
            this.resultsListSummaryLabel.Name = "resultsListSummaryLabel";
            this.resultsListSummaryLabel.Size = new System.Drawing.Size(35, 13);
            this.resultsListSummaryLabel.TabIndex = 46;
            this.resultsListSummaryLabel.Text = "label1";
            // 
            // textBoxCustomDecoyPrefix
            // 
            this.textBoxCustomDecoyPrefix.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxCustomDecoyPrefix.Enabled = false;
            this.textBoxCustomDecoyPrefix.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.textBoxCustomDecoyPrefix.Location = new System.Drawing.Point(849, 56);
            this.textBoxCustomDecoyPrefix.Name = "textBoxCustomDecoyPrefix";
            this.textBoxCustomDecoyPrefix.Size = new System.Drawing.Size(134, 21);
            this.textBoxCustomDecoyPrefix.TabIndex = 4;
            // 
            // customDecoyPrefixCheckBox
            // 
            this.customDecoyPrefixCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.customDecoyPrefixCheckBox.AutoSize = true;
            this.customDecoyPrefixCheckBox.Location = new System.Drawing.Point(728, 58);
            this.customDecoyPrefixCheckBox.Name = "customDecoyPrefixCheckBox";
            this.customDecoyPrefixCheckBox.Size = new System.Drawing.Size(124, 17);
            this.customDecoyPrefixCheckBox.TabIndex = 3;
            this.customDecoyPrefixCheckBox.Text = "Custom decoy prefix:";
            this.customDecoyPrefixCheckBox.UseVisualStyleBackColor = true;
            this.customDecoyPrefixCheckBox.CheckedChanged += new System.EventHandler(this.CustomDecoyPrefixCheckBoxCheckedChanged);
            // 
            // searchResultsSummaryLabel
            // 
            this.searchResultsSummaryLabel.AutoSize = true;
            this.searchResultsSummaryLabel.Location = new System.Drawing.Point(18, 89);
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
        private System.Windows.Forms.CheckBox customDecoyPrefixCheckBox;
        private System.Windows.Forms.TextBox textBoxCustomDecoyPrefix;
        private System.Windows.Forms.Label resultsListSummaryLabel;
        public System.Windows.Forms.CheckBox eValueCheckBox;
        public SharedUI.NumericTextBox textBoxEValueCutoff;
    }
}
