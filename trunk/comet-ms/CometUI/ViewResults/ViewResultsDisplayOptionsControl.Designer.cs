namespace CometUI
{
    partial class ViewResultsDisplayOptionsControl
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
            this.displayOptionsPanel = new System.Windows.Forms.Panel();
            this.highlightPeptideIncludeModCheckBox = new System.Windows.Forms.CheckBox();
            this.highlightProteinLabel = new System.Windows.Forms.Label();
            this.highlightPeptideLabel = new System.Windows.Forms.Label();
            this.rowsPerPageCombo = new System.Windows.Forms.ComboBox();
            this.rowsPerPageLabel = new System.Windows.Forms.Label();
            this.highlightSpectrumLabel = new System.Windows.Forms.Label();
            this.multipleProteinHitsLabel = new System.Windows.Forms.Label();
            this.multipleProteinHitsTopHitRadioButton = new System.Windows.Forms.RadioButton();
            this.multipleProteinHitsAllHitsRadioButton = new System.Windows.Forms.RadioButton();
            this.columnHeadersLabel = new System.Windows.Forms.Label();
            this.columnHeadersRegularRadioButton = new System.Windows.Forms.RadioButton();
            this.columnHeadersCondensedRadioButton = new System.Windows.Forms.RadioButton();
            this.highlightPeptideTextBox = new System.Windows.Forms.TextBox();
            this.highlightProteinTextBox = new System.Windows.Forms.TextBox();
            this.highlightSpectrumTextBox = new System.Windows.Forms.TextBox();
            this.multipleProteinHitsRadioButtonsPanel = new System.Windows.Forms.Panel();
            this.columnHeadersRadioButtonsPanel = new System.Windows.Forms.Panel();
            this.displayOptionsPanel.SuspendLayout();
            this.multipleProteinHitsRadioButtonsPanel.SuspendLayout();
            this.columnHeadersRadioButtonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // displayOptionsPanel
            // 
            this.displayOptionsPanel.AutoScroll = true;
            this.displayOptionsPanel.Controls.Add(this.columnHeadersRadioButtonsPanel);
            this.displayOptionsPanel.Controls.Add(this.multipleProteinHitsRadioButtonsPanel);
            this.displayOptionsPanel.Controls.Add(this.highlightSpectrumTextBox);
            this.displayOptionsPanel.Controls.Add(this.highlightProteinTextBox);
            this.displayOptionsPanel.Controls.Add(this.highlightPeptideTextBox);
            this.displayOptionsPanel.Controls.Add(this.columnHeadersLabel);
            this.displayOptionsPanel.Controls.Add(this.multipleProteinHitsLabel);
            this.displayOptionsPanel.Controls.Add(this.highlightSpectrumLabel);
            this.displayOptionsPanel.Controls.Add(this.highlightPeptideIncludeModCheckBox);
            this.displayOptionsPanel.Controls.Add(this.highlightProteinLabel);
            this.displayOptionsPanel.Controls.Add(this.highlightPeptideLabel);
            this.displayOptionsPanel.Controls.Add(this.rowsPerPageCombo);
            this.displayOptionsPanel.Controls.Add(this.rowsPerPageLabel);
            this.displayOptionsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.displayOptionsPanel.Location = new System.Drawing.Point(0, 0);
            this.displayOptionsPanel.Name = "displayOptionsPanel";
            this.displayOptionsPanel.Size = new System.Drawing.Size(653, 250);
            this.displayOptionsPanel.TabIndex = 0;
            // 
            // highlightPeptideIncludeModCheckBox
            // 
            this.highlightPeptideIncludeModCheckBox.AutoSize = true;
            this.highlightPeptideIncludeModCheckBox.Location = new System.Drawing.Point(406, 58);
            this.highlightPeptideIncludeModCheckBox.Name = "highlightPeptideIncludeModCheckBox";
            this.highlightPeptideIncludeModCheckBox.Size = new System.Drawing.Size(191, 17);
            this.highlightPeptideIncludeModCheckBox.TabIndex = 45;
            this.highlightPeptideIncludeModCheckBox.Text = "Include modification (subscript) text";
            this.highlightPeptideIncludeModCheckBox.UseVisualStyleBackColor = true;
            // 
            // highlightProteinLabel
            // 
            this.highlightProteinLabel.AutoSize = true;
            this.highlightProteinLabel.Location = new System.Drawing.Point(18, 96);
            this.highlightProteinLabel.Name = "highlightProteinLabel";
            this.highlightProteinLabel.Size = new System.Drawing.Size(141, 13);
            this.highlightProteinLabel.TabIndex = 43;
            this.highlightProteinLabel.Text = "Highlight protein text (regex):";
            // 
            // highlightPeptideLabel
            // 
            this.highlightPeptideLabel.AutoSize = true;
            this.highlightPeptideLabel.Location = new System.Drawing.Point(18, 59);
            this.highlightPeptideLabel.Name = "highlightPeptideLabel";
            this.highlightPeptideLabel.Size = new System.Drawing.Size(144, 13);
            this.highlightPeptideLabel.TabIndex = 41;
            this.highlightPeptideLabel.Text = "Highlight peptide text (regex):";
            // 
            // rowsPerPageCombo
            // 
            this.rowsPerPageCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.rowsPerPageCombo.FormattingEnabled = true;
            this.rowsPerPageCombo.Items.AddRange(new object[] {
            "10",
            "25",
            "50",
            "100",
            "250",
            "500",
            "1000",
            "all"});
            this.rowsPerPageCombo.Location = new System.Drawing.Point(176, 16);
            this.rowsPerPageCombo.Name = "rowsPerPageCombo";
            this.rowsPerPageCombo.Size = new System.Drawing.Size(87, 21);
            this.rowsPerPageCombo.TabIndex = 40;
            // 
            // rowsPerPageLabel
            // 
            this.rowsPerPageLabel.AutoSize = true;
            this.rowsPerPageLabel.Location = new System.Drawing.Point(18, 16);
            this.rowsPerPageLabel.Name = "rowsPerPageLabel";
            this.rowsPerPageLabel.Size = new System.Drawing.Size(82, 13);
            this.rowsPerPageLabel.TabIndex = 39;
            this.rowsPerPageLabel.Text = "Rows per page:";
            // 
            // highlightSpectrumLabel
            // 
            this.highlightSpectrumLabel.AutoSize = true;
            this.highlightSpectrumLabel.Location = new System.Drawing.Point(18, 134);
            this.highlightSpectrumLabel.Name = "highlightSpectrumLabel";
            this.highlightSpectrumLabel.Size = new System.Drawing.Size(152, 13);
            this.highlightSpectrumLabel.TabIndex = 46;
            this.highlightSpectrumLabel.Text = "Highlight spectrum text (regex):";
            // 
            // multipleProteinHitsLabel
            // 
            this.multipleProteinHitsLabel.AutoSize = true;
            this.multipleProteinHitsLabel.Location = new System.Drawing.Point(18, 172);
            this.multipleProteinHitsLabel.Name = "multipleProteinHitsLabel";
            this.multipleProteinHitsLabel.Size = new System.Drawing.Size(100, 13);
            this.multipleProteinHitsLabel.TabIndex = 48;
            this.multipleProteinHitsLabel.Text = "Multiple protein hits:";
            // 
            // multipleProteinHitsTopHitRadioButton
            // 
            this.multipleProteinHitsTopHitRadioButton.AutoSize = true;
            this.multipleProteinHitsTopHitRadioButton.Location = new System.Drawing.Point(3, 3);
            this.multipleProteinHitsTopHitRadioButton.Name = "multipleProteinHitsTopHitRadioButton";
            this.multipleProteinHitsTopHitRadioButton.Size = new System.Drawing.Size(80, 17);
            this.multipleProteinHitsTopHitRadioButton.TabIndex = 49;
            this.multipleProteinHitsTopHitRadioButton.TabStop = true;
            this.multipleProteinHitsTopHitRadioButton.Text = "Top hit only";
            this.multipleProteinHitsTopHitRadioButton.UseVisualStyleBackColor = true;
            // 
            // multipleProteinHitsAllHitsRadioButton
            // 
            this.multipleProteinHitsAllHitsRadioButton.AutoSize = true;
            this.multipleProteinHitsAllHitsRadioButton.Location = new System.Drawing.Point(127, 3);
            this.multipleProteinHitsAllHitsRadioButton.Name = "multipleProteinHitsAllHitsRadioButton";
            this.multipleProteinHitsAllHitsRadioButton.Size = new System.Drawing.Size(85, 17);
            this.multipleProteinHitsAllHitsRadioButton.TabIndex = 50;
            this.multipleProteinHitsAllHitsRadioButton.TabStop = true;
            this.multipleProteinHitsAllHitsRadioButton.Text = "List of all hits";
            this.multipleProteinHitsAllHitsRadioButton.UseVisualStyleBackColor = true;
            // 
            // columnHeadersLabel
            // 
            this.columnHeadersLabel.AutoSize = true;
            this.columnHeadersLabel.Location = new System.Drawing.Point(18, 206);
            this.columnHeadersLabel.Name = "columnHeadersLabel";
            this.columnHeadersLabel.Size = new System.Drawing.Size(86, 13);
            this.columnHeadersLabel.TabIndex = 51;
            this.columnHeadersLabel.Text = "Column headers:";
            // 
            // columnHeadersRegularRadioButton
            // 
            this.columnHeadersRegularRadioButton.AutoSize = true;
            this.columnHeadersRegularRadioButton.Location = new System.Drawing.Point(3, 3);
            this.columnHeadersRegularRadioButton.Name = "columnHeadersRegularRadioButton";
            this.columnHeadersRegularRadioButton.Size = new System.Drawing.Size(62, 17);
            this.columnHeadersRegularRadioButton.TabIndex = 52;
            this.columnHeadersRegularRadioButton.TabStop = true;
            this.columnHeadersRegularRadioButton.Text = "Regular";
            this.columnHeadersRegularRadioButton.UseVisualStyleBackColor = true;
            // 
            // columnHeadersCondensedRadioButton
            // 
            this.columnHeadersCondensedRadioButton.AutoSize = true;
            this.columnHeadersCondensedRadioButton.Location = new System.Drawing.Point(127, 4);
            this.columnHeadersCondensedRadioButton.Name = "columnHeadersCondensedRadioButton";
            this.columnHeadersCondensedRadioButton.Size = new System.Drawing.Size(79, 17);
            this.columnHeadersCondensedRadioButton.TabIndex = 53;
            this.columnHeadersCondensedRadioButton.TabStop = true;
            this.columnHeadersCondensedRadioButton.Text = "Condensed";
            this.columnHeadersCondensedRadioButton.UseVisualStyleBackColor = true;
            // 
            // highlightPeptideTextBox
            // 
            this.highlightPeptideTextBox.Location = new System.Drawing.Point(176, 56);
            this.highlightPeptideTextBox.Name = "highlightPeptideTextBox";
            this.highlightPeptideTextBox.Size = new System.Drawing.Size(209, 20);
            this.highlightPeptideTextBox.TabIndex = 54;
            // 
            // highlightProteinTextBox
            // 
            this.highlightProteinTextBox.Location = new System.Drawing.Point(176, 93);
            this.highlightProteinTextBox.Name = "highlightProteinTextBox";
            this.highlightProteinTextBox.Size = new System.Drawing.Size(209, 20);
            this.highlightProteinTextBox.TabIndex = 55;
            // 
            // highlightSpectrumTextBox
            // 
            this.highlightSpectrumTextBox.Location = new System.Drawing.Point(176, 131);
            this.highlightSpectrumTextBox.Name = "highlightSpectrumTextBox";
            this.highlightSpectrumTextBox.Size = new System.Drawing.Size(209, 20);
            this.highlightSpectrumTextBox.TabIndex = 56;
            // 
            // multipleProteinHitsRadioButtonsPanel
            // 
            this.multipleProteinHitsRadioButtonsPanel.Controls.Add(this.multipleProteinHitsTopHitRadioButton);
            this.multipleProteinHitsRadioButtonsPanel.Controls.Add(this.multipleProteinHitsAllHitsRadioButton);
            this.multipleProteinHitsRadioButtonsPanel.Location = new System.Drawing.Point(176, 168);
            this.multipleProteinHitsRadioButtonsPanel.Name = "multipleProteinHitsRadioButtonsPanel";
            this.multipleProteinHitsRadioButtonsPanel.Size = new System.Drawing.Size(244, 23);
            this.multipleProteinHitsRadioButtonsPanel.TabIndex = 57;
            // 
            // columnHeadersRadioButtonsPanel
            // 
            this.columnHeadersRadioButtonsPanel.Controls.Add(this.columnHeadersRegularRadioButton);
            this.columnHeadersRadioButtonsPanel.Controls.Add(this.columnHeadersCondensedRadioButton);
            this.columnHeadersRadioButtonsPanel.Location = new System.Drawing.Point(176, 202);
            this.columnHeadersRadioButtonsPanel.Name = "columnHeadersRadioButtonsPanel";
            this.columnHeadersRadioButtonsPanel.Size = new System.Drawing.Size(244, 28);
            this.columnHeadersRadioButtonsPanel.TabIndex = 58;
            // 
            // ViewResultsDisplayOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.displayOptionsPanel);
            this.Name = "ViewResultsDisplayOptionsControl";
            this.Size = new System.Drawing.Size(653, 250);
            this.displayOptionsPanel.ResumeLayout(false);
            this.displayOptionsPanel.PerformLayout();
            this.multipleProteinHitsRadioButtonsPanel.ResumeLayout(false);
            this.multipleProteinHitsRadioButtonsPanel.PerformLayout();
            this.columnHeadersRadioButtonsPanel.ResumeLayout(false);
            this.columnHeadersRadioButtonsPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel displayOptionsPanel;
        private System.Windows.Forms.CheckBox highlightPeptideIncludeModCheckBox;
        private System.Windows.Forms.Label highlightProteinLabel;
        private System.Windows.Forms.Label highlightPeptideLabel;
        private System.Windows.Forms.ComboBox rowsPerPageCombo;
        private System.Windows.Forms.Label rowsPerPageLabel;
        private System.Windows.Forms.Label highlightSpectrumLabel;
        private System.Windows.Forms.Label multipleProteinHitsLabel;
        private System.Windows.Forms.RadioButton multipleProteinHitsAllHitsRadioButton;
        private System.Windows.Forms.RadioButton multipleProteinHitsTopHitRadioButton;
        private System.Windows.Forms.Label columnHeadersLabel;
        private System.Windows.Forms.RadioButton columnHeadersCondensedRadioButton;
        private System.Windows.Forms.RadioButton columnHeadersRegularRadioButton;
        private System.Windows.Forms.TextBox highlightProteinTextBox;
        private System.Windows.Forms.TextBox highlightPeptideTextBox;
        private System.Windows.Forms.TextBox highlightSpectrumTextBox;
        private System.Windows.Forms.Panel multipleProteinHitsRadioButtonsPanel;
        private System.Windows.Forms.Panel columnHeadersRadioButtonsPanel;

    }
}
