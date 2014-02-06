namespace CometUI
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
            this.missedCleavagesCombo = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // searchEnzymeLabel
            // 
            this.searchEnzymeLabel.AutoSize = true;
            this.searchEnzymeLabel.Location = new System.Drawing.Point(22, 25);
            this.searchEnzymeLabel.Name = "searchEnzymeLabel";
            this.searchEnzymeLabel.Size = new System.Drawing.Size(84, 13);
            this.searchEnzymeLabel.TabIndex = 0;
            this.searchEnzymeLabel.Text = "Search Enzyme:";
            // 
            // searchEnzymeCombo
            // 
            this.searchEnzymeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.searchEnzymeCombo.FormattingEnabled = true;
            this.searchEnzymeCombo.Location = new System.Drawing.Point(25, 41);
            this.searchEnzymeCombo.Name = "searchEnzymeCombo";
            this.searchEnzymeCombo.Size = new System.Drawing.Size(133, 21);
            this.searchEnzymeCombo.TabIndex = 1;
            this.searchEnzymeCombo.SelectedIndexChanged += new System.EventHandler(this.SearchEnzymeComboSelectedIndexChanged);
            // 
            // sampleEnzymeCombo
            // 
            this.sampleEnzymeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.sampleEnzymeCombo.FormattingEnabled = true;
            this.sampleEnzymeCombo.Location = new System.Drawing.Point(25, 106);
            this.sampleEnzymeCombo.Name = "sampleEnzymeCombo";
            this.sampleEnzymeCombo.Size = new System.Drawing.Size(133, 21);
            this.sampleEnzymeCombo.TabIndex = 2;
            this.sampleEnzymeCombo.SelectedIndexChanged += new System.EventHandler(this.SampleEnzymeComboSelectedIndexChanged);
            // 
            // sampleEnzymeLabel
            // 
            this.sampleEnzymeLabel.AutoSize = true;
            this.sampleEnzymeLabel.Location = new System.Drawing.Point(22, 90);
            this.sampleEnzymeLabel.Name = "sampleEnzymeLabel";
            this.sampleEnzymeLabel.Size = new System.Drawing.Size(85, 13);
            this.sampleEnzymeLabel.TabIndex = 2;
            this.sampleEnzymeLabel.Text = "Sample Enzyme:";
            // 
            // enzymeTerminiCombo
            // 
            this.enzymeTerminiCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.enzymeTerminiCombo.FormattingEnabled = true;
            this.enzymeTerminiCombo.Location = new System.Drawing.Point(25, 236);
            this.enzymeTerminiCombo.Name = "enzymeTerminiCombo";
            this.enzymeTerminiCombo.Size = new System.Drawing.Size(133, 21);
            this.enzymeTerminiCombo.TabIndex = 4;
            // 
            // enzypeTerminiLabel
            // 
            this.enzypeTerminiLabel.AutoSize = true;
            this.enzypeTerminiLabel.Location = new System.Drawing.Point(23, 220);
            this.enzypeTerminiLabel.Name = "enzypeTerminiLabel";
            this.enzypeTerminiLabel.Size = new System.Drawing.Size(84, 13);
            this.enzypeTerminiLabel.TabIndex = 4;
            this.enzypeTerminiLabel.Text = "Enzyme Termini:";
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
            "5"});
            this.missedCleavagesCombo.Location = new System.Drawing.Point(25, 171);
            this.missedCleavagesCombo.Name = "missedCleavagesCombo";
            this.missedCleavagesCombo.Size = new System.Drawing.Size(133, 21);
            this.missedCleavagesCombo.TabIndex = 3;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(22, 155);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(136, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "Allowed Missed Cleavages:";
            // 
            // EnzymeSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.missedCleavagesCombo);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.enzymeTerminiCombo);
            this.Controls.Add(this.enzypeTerminiLabel);
            this.Controls.Add(this.sampleEnzymeCombo);
            this.Controls.Add(this.sampleEnzymeLabel);
            this.Controls.Add(this.searchEnzymeCombo);
            this.Controls.Add(this.searchEnzymeLabel);
            this.Name = "EnzymeSettingsControl";
            this.Size = new System.Drawing.Size(527, 425);
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
        private System.Windows.Forms.ComboBox missedCleavagesCombo;
        private System.Windows.Forms.Label label1;

    }
}
