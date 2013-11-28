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
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // searchEnzymeLabel
            // 
            this.searchEnzymeLabel.AutoSize = true;
            this.searchEnzymeLabel.Location = new System.Drawing.Point(10, 38);
            this.searchEnzymeLabel.Name = "searchEnzymeLabel";
            this.searchEnzymeLabel.Size = new System.Drawing.Size(84, 13);
            this.searchEnzymeLabel.TabIndex = 0;
            this.searchEnzymeLabel.Text = "Search Enzyme:";
            // 
            // searchEnzymeCombo
            // 
            this.searchEnzymeCombo.FormattingEnabled = true;
            this.searchEnzymeCombo.Location = new System.Drawing.Point(100, 35);
            this.searchEnzymeCombo.Name = "searchEnzymeCombo";
            this.searchEnzymeCombo.Size = new System.Drawing.Size(121, 21);
            this.searchEnzymeCombo.TabIndex = 1;
            // 
            // sampleEnzymeCombo
            // 
            this.sampleEnzymeCombo.FormattingEnabled = true;
            this.sampleEnzymeCombo.Location = new System.Drawing.Point(100, 75);
            this.sampleEnzymeCombo.Name = "sampleEnzymeCombo";
            this.sampleEnzymeCombo.Size = new System.Drawing.Size(121, 21);
            this.sampleEnzymeCombo.TabIndex = 3;
            // 
            // sampleEnzymeLabel
            // 
            this.sampleEnzymeLabel.AutoSize = true;
            this.sampleEnzymeLabel.Location = new System.Drawing.Point(10, 78);
            this.sampleEnzymeLabel.Name = "sampleEnzymeLabel";
            this.sampleEnzymeLabel.Size = new System.Drawing.Size(85, 13);
            this.sampleEnzymeLabel.TabIndex = 2;
            this.sampleEnzymeLabel.Text = "Sample Enzyme:";
            // 
            // enzymeTerminiCombo
            // 
            this.enzymeTerminiCombo.FormattingEnabled = true;
            this.enzymeTerminiCombo.Location = new System.Drawing.Point(100, 115);
            this.enzymeTerminiCombo.Name = "enzymeTerminiCombo";
            this.enzymeTerminiCombo.Size = new System.Drawing.Size(121, 21);
            this.enzymeTerminiCombo.TabIndex = 5;
            // 
            // enzypeTerminiLabel
            // 
            this.enzypeTerminiLabel.AutoSize = true;
            this.enzypeTerminiLabel.Location = new System.Drawing.Point(10, 118);
            this.enzypeTerminiLabel.Name = "enzypeTerminiLabel";
            this.enzypeTerminiLabel.Size = new System.Drawing.Size(84, 13);
            this.enzypeTerminiLabel.TabIndex = 4;
            this.enzypeTerminiLabel.Text = "Enzyme Termini:";
            // 
            // comboBox1
            // 
            this.comboBox1.FormattingEnabled = true;
            this.comboBox1.Location = new System.Drawing.Point(100, 155);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(121, 21);
            this.comboBox1.TabIndex = 7;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 152);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(80, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "Allowed Missed";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 165);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(60, 13);
            this.label2.TabIndex = 8;
            this.label2.Text = "Cleavages:";
            // 
            // EnzymeSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.label2);
            this.Controls.Add(this.comboBox1);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.enzymeTerminiCombo);
            this.Controls.Add(this.enzypeTerminiLabel);
            this.Controls.Add(this.sampleEnzymeCombo);
            this.Controls.Add(this.sampleEnzymeLabel);
            this.Controls.Add(this.searchEnzymeCombo);
            this.Controls.Add(this.searchEnzymeLabel);
            this.Name = "EnzymeSettingsControl";
            this.Size = new System.Drawing.Size(527, 330);
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
        private System.Windows.Forms.ComboBox comboBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;

    }
}
