using CometUI.CustomControls;

namespace CometUI.Search.SearchSettings
{
    partial class EnzymeSettingsControl
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
            this.searchEnzymeLabel = new System.Windows.Forms.Label();
            this.searchEnzymeCombo = new System.Windows.Forms.ComboBox();
            this.sampleEnzymeCombo = new System.Windows.Forms.ComboBox();
            this.sampleEnzymeLabel = new System.Windows.Forms.Label();
            this.enzymeTerminiCombo = new System.Windows.Forms.ComboBox();
            this.enzypeTerminiLabel = new System.Windows.Forms.Label();
            this.allowedMissedCleavagesLabel = new System.Windows.Forms.Label();
            this.missedCleavagesCombo = new System.Windows.Forms.ComboBox();
            this.digestMassRangeLabel = new System.Windows.Forms.Label();
            this.digestMassRangeMaxTextBox = new NumericTextBox();
            this.label17 = new System.Windows.Forms.Label();
            this.digestMassRangeMinTextBox = new NumericTextBox();
            this.SuspendLayout();
            // 
            // searchEnzymeLabel
            // 
            this.searchEnzymeLabel.AutoSize = true;
            this.searchEnzymeLabel.Location = new System.Drawing.Point(22, 53);
            this.searchEnzymeLabel.Name = "searchEnzymeLabel";
            this.searchEnzymeLabel.Size = new System.Drawing.Size(84, 13);
            this.searchEnzymeLabel.TabIndex = 0;
            this.searchEnzymeLabel.Text = "Search Enzyme:";
            // 
            // searchEnzymeCombo
            // 
            this.searchEnzymeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.searchEnzymeCombo.FormattingEnabled = true;
            this.searchEnzymeCombo.Location = new System.Drawing.Point(112, 50);
            this.searchEnzymeCombo.Name = "searchEnzymeCombo";
            this.searchEnzymeCombo.Size = new System.Drawing.Size(206, 21);
            this.searchEnzymeCombo.TabIndex = 1;
            this.searchEnzymeCombo.SelectedIndexChanged += new System.EventHandler(this.SearchEnzymeComboSelectedIndexChanged);
            // 
            // sampleEnzymeCombo
            // 
            this.sampleEnzymeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.sampleEnzymeCombo.FormattingEnabled = true;
            this.sampleEnzymeCombo.Location = new System.Drawing.Point(112, 110);
            this.sampleEnzymeCombo.Name = "sampleEnzymeCombo";
            this.sampleEnzymeCombo.Size = new System.Drawing.Size(206, 21);
            this.sampleEnzymeCombo.TabIndex = 2;
            this.sampleEnzymeCombo.SelectedIndexChanged += new System.EventHandler(this.SampleEnzymeComboSelectedIndexChanged);
            // 
            // sampleEnzymeLabel
            // 
            this.sampleEnzymeLabel.AutoSize = true;
            this.sampleEnzymeLabel.Location = new System.Drawing.Point(21, 113);
            this.sampleEnzymeLabel.Name = "sampleEnzymeLabel";
            this.sampleEnzymeLabel.Size = new System.Drawing.Size(85, 13);
            this.sampleEnzymeLabel.TabIndex = 2;
            this.sampleEnzymeLabel.Text = "Sample Enzyme:";
            // 
            // enzymeTerminiCombo
            // 
            this.enzymeTerminiCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.enzymeTerminiCombo.FormattingEnabled = true;
            this.enzymeTerminiCombo.Location = new System.Drawing.Point(111, 170);
            this.enzymeTerminiCombo.Name = "enzymeTerminiCombo";
            this.enzymeTerminiCombo.Size = new System.Drawing.Size(207, 21);
            this.enzymeTerminiCombo.TabIndex = 4;
            // 
            // enzypeTerminiLabel
            // 
            this.enzypeTerminiLabel.AutoSize = true;
            this.enzypeTerminiLabel.Location = new System.Drawing.Point(21, 173);
            this.enzypeTerminiLabel.Name = "enzypeTerminiLabel";
            this.enzypeTerminiLabel.Size = new System.Drawing.Size(84, 13);
            this.enzypeTerminiLabel.TabIndex = 4;
            this.enzypeTerminiLabel.Text = "Enzyme Termini:";
            // 
            // allowedMissedCleavagesLabel
            // 
            this.allowedMissedCleavagesLabel.AutoSize = true;
            this.allowedMissedCleavagesLabel.Location = new System.Drawing.Point(22, 233);
            this.allowedMissedCleavagesLabel.Name = "allowedMissedCleavagesLabel";
            this.allowedMissedCleavagesLabel.Size = new System.Drawing.Size(136, 13);
            this.allowedMissedCleavagesLabel.TabIndex = 6;
            this.allowedMissedCleavagesLabel.Text = "Allowed Missed Cleavages:";
            // 
            // missedCleavagesCombo
            // 
            this.missedCleavagesCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.missedCleavagesCombo.FormattingEnabled = true;
            this.missedCleavagesCombo.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10"});
            this.missedCleavagesCombo.Location = new System.Drawing.Point(165, 230);
            this.missedCleavagesCombo.Name = "missedCleavagesCombo";
            this.missedCleavagesCombo.Size = new System.Drawing.Size(153, 21);
            this.missedCleavagesCombo.TabIndex = 3;
            // 
            // digestMassRangeLabel
            // 
            this.digestMassRangeLabel.AutoSize = true;
            this.digestMassRangeLabel.Location = new System.Drawing.Point(22, 296);
            this.digestMassRangeLabel.Name = "digestMassRangeLabel";
            this.digestMassRangeLabel.Size = new System.Drawing.Size(103, 13);
            this.digestMassRangeLabel.TabIndex = 36;
            this.digestMassRangeLabel.Text = "Digest Mass Range:";
            // 
            // digestMassRangeMaxTextBox
            // 
            this.digestMassRangeMaxTextBox.AllowDecimal = true;
            this.digestMassRangeMaxTextBox.AllowSpace = false;
            this.digestMassRangeMaxTextBox.Location = new System.Drawing.Point(238, 293);
            this.digestMassRangeMaxTextBox.Name = "digestMassRangeMaxTextBox";
            this.digestMassRangeMaxTextBox.Size = new System.Drawing.Size(80, 20);
            this.digestMassRangeMaxTextBox.TabIndex = 35;
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(216, 296);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(16, 13);
            this.label17.TabIndex = 37;
            this.label17.Text = "to";
            // 
            // digestMassRangeMinTextBox
            // 
            this.digestMassRangeMinTextBox.AllowDecimal = true;
            this.digestMassRangeMinTextBox.AllowSpace = false;
            this.digestMassRangeMinTextBox.Location = new System.Drawing.Point(130, 293);
            this.digestMassRangeMinTextBox.Name = "digestMassRangeMinTextBox";
            this.digestMassRangeMinTextBox.Size = new System.Drawing.Size(80, 20);
            this.digestMassRangeMinTextBox.TabIndex = 34;
            // 
            // EnzymeSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.digestMassRangeLabel);
            this.Controls.Add(this.digestMassRangeMaxTextBox);
            this.Controls.Add(this.label17);
            this.Controls.Add(this.digestMassRangeMinTextBox);
            this.Controls.Add(this.missedCleavagesCombo);
            this.Controls.Add(this.allowedMissedCleavagesLabel);
            this.Controls.Add(this.enzymeTerminiCombo);
            this.Controls.Add(this.enzypeTerminiLabel);
            this.Controls.Add(this.sampleEnzymeCombo);
            this.Controls.Add(this.sampleEnzymeLabel);
            this.Controls.Add(this.searchEnzymeCombo);
            this.Controls.Add(this.searchEnzymeLabel);
            this.Name = "EnzymeSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label searchEnzymeLabel;
        private System.Windows.Forms.ComboBox searchEnzymeCombo;
        private System.Windows.Forms.ComboBox sampleEnzymeCombo;
        private System.Windows.Forms.Label sampleEnzymeLabel;
        private System.Windows.Forms.ComboBox enzymeTerminiCombo;
        private System.Windows.Forms.Label enzypeTerminiLabel;
        private System.Windows.Forms.Label allowedMissedCleavagesLabel;
        private System.Windows.Forms.ComboBox missedCleavagesCombo;
        private System.Windows.Forms.Label digestMassRangeLabel;
        private CustomControls.NumericTextBox digestMassRangeMaxTextBox;
        private System.Windows.Forms.Label label17;
        private CustomControls.NumericTextBox digestMassRangeMinTextBox;

    }
}
