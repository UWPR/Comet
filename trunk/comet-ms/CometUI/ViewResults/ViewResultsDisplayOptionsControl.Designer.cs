namespace CometUI.ViewResults
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
            this.displayOptionsMainPanel = new System.Windows.Forms.Panel();
            this.btnUpdateResults = new System.Windows.Forms.Button();
            this.columnHeadersRadioButtonsPanel = new System.Windows.Forms.Panel();
            this.columnHeadersRegularRadioButton = new System.Windows.Forms.RadioButton();
            this.columnHeadersCondensedRadioButton = new System.Windows.Forms.RadioButton();
            this.highlightSpectrumTextBox = new System.Windows.Forms.TextBox();
            this.highlightProteinTextBox = new System.Windows.Forms.TextBox();
            this.highlightPeptideTextBox = new System.Windows.Forms.TextBox();
            this.columnHeadersLabel = new System.Windows.Forms.Label();
            this.highlightSpectrumLabel = new System.Windows.Forms.Label();
            this.highlightPeptideIncludeModCheckBox = new System.Windows.Forms.CheckBox();
            this.highlightProteinLabel = new System.Windows.Forms.Label();
            this.highlightPeptideLabel = new System.Windows.Forms.Label();
            this.rowsPerPageCombo = new System.Windows.Forms.ComboBox();
            this.rowsPerPageLabel = new System.Windows.Forms.Label();
            this.displayOptionsMainPanel.SuspendLayout();
            this.columnHeadersRadioButtonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // displayOptionsMainPanel
            // 
            this.displayOptionsMainPanel.AutoScroll = true;
            this.displayOptionsMainPanel.Controls.Add(this.btnUpdateResults);
            this.displayOptionsMainPanel.Controls.Add(this.columnHeadersRadioButtonsPanel);
            this.displayOptionsMainPanel.Controls.Add(this.highlightSpectrumTextBox);
            this.displayOptionsMainPanel.Controls.Add(this.highlightProteinTextBox);
            this.displayOptionsMainPanel.Controls.Add(this.highlightPeptideTextBox);
            this.displayOptionsMainPanel.Controls.Add(this.columnHeadersLabel);
            this.displayOptionsMainPanel.Controls.Add(this.highlightSpectrumLabel);
            this.displayOptionsMainPanel.Controls.Add(this.highlightPeptideIncludeModCheckBox);
            this.displayOptionsMainPanel.Controls.Add(this.highlightProteinLabel);
            this.displayOptionsMainPanel.Controls.Add(this.highlightPeptideLabel);
            this.displayOptionsMainPanel.Controls.Add(this.rowsPerPageCombo);
            this.displayOptionsMainPanel.Controls.Add(this.rowsPerPageLabel);
            this.displayOptionsMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.displayOptionsMainPanel.Location = new System.Drawing.Point(0, 0);
            this.displayOptionsMainPanel.Name = "displayOptionsMainPanel";
            this.displayOptionsMainPanel.Size = new System.Drawing.Size(653, 220);
            this.displayOptionsMainPanel.TabIndex = 0;
            // 
            // btnUpdateResults
            // 
            this.btnUpdateResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpdateResults.Location = new System.Drawing.Point(561, 180);
            this.btnUpdateResults.Name = "btnUpdateResults";
            this.btnUpdateResults.Size = new System.Drawing.Size(75, 23);
            this.btnUpdateResults.TabIndex = 59;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            this.btnUpdateResults.Click += new System.EventHandler(this.BtnUpdateResultsClick);
            // 
            // columnHeadersRadioButtonsPanel
            // 
            this.columnHeadersRadioButtonsPanel.Controls.Add(this.columnHeadersRegularRadioButton);
            this.columnHeadersRadioButtonsPanel.Controls.Add(this.columnHeadersCondensedRadioButton);
            this.columnHeadersRadioButtonsPanel.Location = new System.Drawing.Point(176, 163);
            this.columnHeadersRadioButtonsPanel.Name = "columnHeadersRadioButtonsPanel";
            this.columnHeadersRadioButtonsPanel.Size = new System.Drawing.Size(244, 21);
            this.columnHeadersRadioButtonsPanel.TabIndex = 58;
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
            this.columnHeadersRegularRadioButton.CheckedChanged += new System.EventHandler(this.ColumnHeadersRegularRadioButtonCheckedChanged);
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
            this.columnHeadersCondensedRadioButton.CheckedChanged += new System.EventHandler(this.ColumnHeadersCondensedRadioButtonCheckedChanged);
            // 
            // highlightSpectrumTextBox
            // 
            this.highlightSpectrumTextBox.Location = new System.Drawing.Point(176, 127);
            this.highlightSpectrumTextBox.Name = "highlightSpectrumTextBox";
            this.highlightSpectrumTextBox.Size = new System.Drawing.Size(209, 20);
            this.highlightSpectrumTextBox.TabIndex = 56;
            // 
            // highlightProteinTextBox
            // 
            this.highlightProteinTextBox.Location = new System.Drawing.Point(176, 89);
            this.highlightProteinTextBox.Name = "highlightProteinTextBox";
            this.highlightProteinTextBox.Size = new System.Drawing.Size(209, 20);
            this.highlightProteinTextBox.TabIndex = 55;
            // 
            // highlightPeptideTextBox
            // 
            this.highlightPeptideTextBox.Location = new System.Drawing.Point(176, 52);
            this.highlightPeptideTextBox.Name = "highlightPeptideTextBox";
            this.highlightPeptideTextBox.Size = new System.Drawing.Size(209, 20);
            this.highlightPeptideTextBox.TabIndex = 54;
            // 
            // columnHeadersLabel
            // 
            this.columnHeadersLabel.AutoSize = true;
            this.columnHeadersLabel.Location = new System.Drawing.Point(18, 170);
            this.columnHeadersLabel.Name = "columnHeadersLabel";
            this.columnHeadersLabel.Size = new System.Drawing.Size(86, 13);
            this.columnHeadersLabel.TabIndex = 51;
            this.columnHeadersLabel.Text = "Column headers:";
            // 
            // highlightSpectrumLabel
            // 
            this.highlightSpectrumLabel.AutoSize = true;
            this.highlightSpectrumLabel.Location = new System.Drawing.Point(18, 130);
            this.highlightSpectrumLabel.Name = "highlightSpectrumLabel";
            this.highlightSpectrumLabel.Size = new System.Drawing.Size(152, 13);
            this.highlightSpectrumLabel.TabIndex = 46;
            this.highlightSpectrumLabel.Text = "Highlight spectrum text (regex):";
            // 
            // highlightPeptideIncludeModCheckBox
            // 
            this.highlightPeptideIncludeModCheckBox.AutoSize = true;
            this.highlightPeptideIncludeModCheckBox.Location = new System.Drawing.Point(404, 54);
            this.highlightPeptideIncludeModCheckBox.Name = "highlightPeptideIncludeModCheckBox";
            this.highlightPeptideIncludeModCheckBox.Size = new System.Drawing.Size(191, 17);
            this.highlightPeptideIncludeModCheckBox.TabIndex = 45;
            this.highlightPeptideIncludeModCheckBox.Text = "Include modification (subscript) text";
            this.highlightPeptideIncludeModCheckBox.UseVisualStyleBackColor = true;
            this.highlightPeptideIncludeModCheckBox.CheckedChanged += new System.EventHandler(this.HighlightPeptideIncludeModCheckBoxCheckedChanged);
            // 
            // highlightProteinLabel
            // 
            this.highlightProteinLabel.AutoSize = true;
            this.highlightProteinLabel.Location = new System.Drawing.Point(18, 92);
            this.highlightProteinLabel.Name = "highlightProteinLabel";
            this.highlightProteinLabel.Size = new System.Drawing.Size(141, 13);
            this.highlightProteinLabel.TabIndex = 43;
            this.highlightProteinLabel.Text = "Highlight protein text (regex):";
            // 
            // highlightPeptideLabel
            // 
            this.highlightPeptideLabel.AutoSize = true;
            this.highlightPeptideLabel.Location = new System.Drawing.Point(18, 55);
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
            this.rowsPerPageCombo.SelectedIndexChanged += new System.EventHandler(this.RowsPerPageComboSelectedIndexChanged);
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
            // ViewResultsDisplayOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.displayOptionsMainPanel);
            this.Name = "ViewResultsDisplayOptionsControl";
            this.Size = new System.Drawing.Size(653, 220);
            this.displayOptionsMainPanel.ResumeLayout(false);
            this.displayOptionsMainPanel.PerformLayout();
            this.columnHeadersRadioButtonsPanel.ResumeLayout(false);
            this.columnHeadersRadioButtonsPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel displayOptionsMainPanel;
        private System.Windows.Forms.CheckBox highlightPeptideIncludeModCheckBox;
        private System.Windows.Forms.Label highlightProteinLabel;
        private System.Windows.Forms.Label highlightPeptideLabel;
        private System.Windows.Forms.ComboBox rowsPerPageCombo;
        private System.Windows.Forms.Label rowsPerPageLabel;
        private System.Windows.Forms.Label highlightSpectrumLabel;
        private System.Windows.Forms.Label columnHeadersLabel;
        private System.Windows.Forms.RadioButton columnHeadersCondensedRadioButton;
        private System.Windows.Forms.RadioButton columnHeadersRegularRadioButton;
        private System.Windows.Forms.TextBox highlightProteinTextBox;
        private System.Windows.Forms.TextBox highlightPeptideTextBox;
        private System.Windows.Forms.TextBox highlightSpectrumTextBox;
        private System.Windows.Forms.Panel columnHeadersRadioButtonsPanel;
        private System.Windows.Forms.Button btnUpdateResults;

    }
}
