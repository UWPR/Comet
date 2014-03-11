namespace CometUI
{
    partial class MassSettingsControl
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
            this.fragmentGroupBox = new System.Windows.Forms.GroupBox();
            this.fragmentMassTypeCombo = new System.Windows.Forms.ComboBox();
            this.label9 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.fragmentOffsetTextBox = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.fragmentBinSizeTextBox = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.precursorGroupBox = new System.Windows.Forms.GroupBox();
            this.precursorMassTolTextBox = new System.Windows.Forms.NumericUpDown();
            this.precursorIsotopeErrorCombo = new System.Windows.Forms.ComboBox();
            this.precursorMassTypeCombo = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.precursorTolTypeCombo = new System.Windows.Forms.ComboBox();
            this.precursorMassUnitCombo = new System.Windows.Forms.ComboBox();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.useNLCheckBox = new System.Windows.Forms.CheckBox();
            this.flankCheckBox = new System.Windows.Forms.CheckBox();
            this.zIonCheckBox = new System.Windows.Forms.CheckBox();
            this.yIonCheckBox = new System.Windows.Forms.CheckBox();
            this.bIonCheckBox = new System.Windows.Forms.CheckBox();
            this.xIonCheckBox = new System.Windows.Forms.CheckBox();
            this.cIonCheckBox = new System.Windows.Forms.CheckBox();
            this.aIonCheckBox = new System.Windows.Forms.CheckBox();
            this.fragmentGroupBox.SuspendLayout();
            this.precursorGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.precursorMassTolTextBox)).BeginInit();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // fragmentGroupBox
            // 
            this.fragmentGroupBox.Controls.Add(this.fragmentMassTypeCombo);
            this.fragmentGroupBox.Controls.Add(this.label9);
            this.fragmentGroupBox.Controls.Add(this.label8);
            this.fragmentGroupBox.Controls.Add(this.fragmentOffsetTextBox);
            this.fragmentGroupBox.Controls.Add(this.label7);
            this.fragmentGroupBox.Controls.Add(this.fragmentBinSizeTextBox);
            this.fragmentGroupBox.Location = new System.Drawing.Point(272, 17);
            this.fragmentGroupBox.Name = "fragmentGroupBox";
            this.fragmentGroupBox.Size = new System.Drawing.Size(225, 144);
            this.fragmentGroupBox.TabIndex = 1;
            this.fragmentGroupBox.TabStop = false;
            this.fragmentGroupBox.Text = "Fragment";
            // 
            // fragmentMassTypeCombo
            // 
            this.fragmentMassTypeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.fragmentMassTypeCombo.FormattingEnabled = true;
            this.fragmentMassTypeCombo.Items.AddRange(new object[] {
            "avg",
            "mono"});
            this.fragmentMassTypeCombo.Location = new System.Drawing.Point(18, 104);
            this.fragmentMassTypeCombo.Name = "fragmentMassTypeCombo";
            this.fragmentMassTypeCombo.Size = new System.Drawing.Size(67, 21);
            this.fragmentMassTypeCombo.TabIndex = 8;
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(15, 88);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(62, 13);
            this.label9.TabIndex = 32;
            this.label9.Text = "Mass Type:";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(124, 30);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(38, 13);
            this.label8.TabIndex = 27;
            this.label8.Text = "Offset:";
            // 
            // fragmentOffsetTextBox
            // 
            this.fragmentOffsetTextBox.Location = new System.Drawing.Point(127, 45);
            this.fragmentOffsetTextBox.Name = "fragmentOffsetTextBox";
            this.fragmentOffsetTextBox.Size = new System.Drawing.Size(67, 20);
            this.fragmentOffsetTextBox.TabIndex = 7;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(15, 30);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(48, 13);
            this.label7.TabIndex = 25;
            this.label7.Text = "Bin Size:";
            // 
            // fragmentBinSizeTextBox
            // 
            this.fragmentBinSizeTextBox.Location = new System.Drawing.Point(18, 45);
            this.fragmentBinSizeTextBox.Name = "fragmentBinSizeTextBox";
            this.fragmentBinSizeTextBox.Size = new System.Drawing.Size(67, 20);
            this.fragmentBinSizeTextBox.TabIndex = 6;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(13, 46);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(13, 13);
            this.label1.TabIndex = 22;
            this.label1.Text = "+";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 30);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(53, 13);
            this.label2.TabIndex = 23;
            this.label2.Text = "Mass Tol:";
            // 
            // precursorGroupBox
            // 
            this.precursorGroupBox.Controls.Add(this.precursorMassTolTextBox);
            this.precursorGroupBox.Controls.Add(this.precursorIsotopeErrorCombo);
            this.precursorGroupBox.Controls.Add(this.precursorMassTypeCombo);
            this.precursorGroupBox.Controls.Add(this.label3);
            this.precursorGroupBox.Controls.Add(this.precursorTolTypeCombo);
            this.precursorGroupBox.Controls.Add(this.precursorMassUnitCombo);
            this.precursorGroupBox.Controls.Add(this.label6);
            this.precursorGroupBox.Controls.Add(this.label5);
            this.precursorGroupBox.Controls.Add(this.label4);
            this.precursorGroupBox.Controls.Add(this.label2);
            this.precursorGroupBox.Controls.Add(this.label1);
            this.precursorGroupBox.Location = new System.Drawing.Point(26, 17);
            this.precursorGroupBox.Name = "precursorGroupBox";
            this.precursorGroupBox.Size = new System.Drawing.Size(225, 254);
            this.precursorGroupBox.TabIndex = 0;
            this.precursorGroupBox.TabStop = false;
            this.precursorGroupBox.Text = "Precursor";
            // 
            // precursorMassTolTextBox
            // 
            this.precursorMassTolTextBox.DecimalPlaces = 2;
            this.precursorMassTolTextBox.Location = new System.Drawing.Point(27, 45);
            this.precursorMassTolTextBox.Name = "precursorMassTolTextBox";
            this.precursorMassTolTextBox.Size = new System.Drawing.Size(56, 20);
            this.precursorMassTolTextBox.TabIndex = 31;
            // 
            // precursorIsotopeErrorCombo
            // 
            this.precursorIsotopeErrorCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.precursorIsotopeErrorCombo.FormattingEnabled = true;
            this.precursorIsotopeErrorCombo.Items.AddRange(new object[] {
            "no C13",
            "C13 offsets"});
            this.precursorIsotopeErrorCombo.Location = new System.Drawing.Point(16, 162);
            this.precursorIsotopeErrorCombo.Name = "precursorIsotopeErrorCombo";
            this.precursorIsotopeErrorCombo.Size = new System.Drawing.Size(67, 21);
            this.precursorIsotopeErrorCombo.TabIndex = 5;
            // 
            // precursorMassTypeCombo
            // 
            this.precursorMassTypeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.precursorMassTypeCombo.FormattingEnabled = true;
            this.precursorMassTypeCombo.Items.AddRange(new object[] {
            "avg",
            "mono"});
            this.precursorMassTypeCombo.Location = new System.Drawing.Point(16, 104);
            this.precursorMassTypeCombo.Name = "precursorMassTypeCombo";
            this.precursorMassTypeCombo.Size = new System.Drawing.Size(67, 21);
            this.precursorMassTypeCombo.TabIndex = 3;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(120, 30);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(57, 13);
            this.label3.TabIndex = 30;
            this.label3.Text = "Mass Unit:";
            // 
            // precursorTolTypeCombo
            // 
            this.precursorTolTypeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.precursorTolTypeCombo.FormattingEnabled = true;
            this.precursorTolTypeCombo.Items.AddRange(new object[] {
            "MH+",
            "M/Z"});
            this.precursorTolTypeCombo.Location = new System.Drawing.Point(123, 104);
            this.precursorTolTypeCombo.Name = "precursorTolTypeCombo";
            this.precursorTolTypeCombo.Size = new System.Drawing.Size(67, 21);
            this.precursorTolTypeCombo.TabIndex = 4;
            // 
            // precursorMassUnitCombo
            // 
            this.precursorMassUnitCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.precursorMassUnitCombo.FormattingEnabled = true;
            this.precursorMassUnitCombo.Items.AddRange(new object[] {
            "amu",
            "ppm",
            "mmu"});
            this.precursorMassUnitCombo.Location = new System.Drawing.Point(123, 45);
            this.precursorMassUnitCombo.Name = "precursorMassUnitCombo";
            this.precursorMassUnitCombo.Size = new System.Drawing.Size(67, 21);
            this.precursorMassUnitCombo.TabIndex = 2;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(13, 146);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(70, 13);
            this.label6.TabIndex = 27;
            this.label6.Text = "Isotope Error:";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(13, 88);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(62, 13);
            this.label5.TabIndex = 26;
            this.label5.Text = "Mass Type:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(120, 88);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(52, 13);
            this.label4.TabIndex = 25;
            this.label4.Text = "Tol Type:";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.useNLCheckBox);
            this.groupBox1.Controls.Add(this.flankCheckBox);
            this.groupBox1.Controls.Add(this.zIonCheckBox);
            this.groupBox1.Controls.Add(this.yIonCheckBox);
            this.groupBox1.Controls.Add(this.bIonCheckBox);
            this.groupBox1.Controls.Add(this.xIonCheckBox);
            this.groupBox1.Controls.Add(this.cIonCheckBox);
            this.groupBox1.Controls.Add(this.aIonCheckBox);
            this.groupBox1.Location = new System.Drawing.Point(272, 173);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(225, 98);
            this.groupBox1.TabIndex = 2;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Ions";
            // 
            // useNLCheckBox
            // 
            this.useNLCheckBox.AutoSize = true;
            this.useNLCheckBox.Location = new System.Drawing.Point(120, 40);
            this.useNLCheckBox.Name = "useNLCheckBox";
            this.useNLCheckBox.Size = new System.Drawing.Size(62, 17);
            this.useNLCheckBox.TabIndex = 16;
            this.useNLCheckBox.Text = "Use NL";
            this.useNLCheckBox.UseVisualStyleBackColor = true;
            // 
            // flankCheckBox
            // 
            this.flankCheckBox.AutoSize = true;
            this.flankCheckBox.Location = new System.Drawing.Point(120, 20);
            this.flankCheckBox.Name = "flankCheckBox";
            this.flankCheckBox.Size = new System.Drawing.Size(52, 17);
            this.flankCheckBox.TabIndex = 15;
            this.flankCheckBox.Text = "Flank";
            this.flankCheckBox.UseVisualStyleBackColor = true;
            // 
            // zIonCheckBox
            // 
            this.zIonCheckBox.AutoSize = true;
            this.zIonCheckBox.Location = new System.Drawing.Point(70, 60);
            this.zIonCheckBox.Name = "zIonCheckBox";
            this.zIonCheckBox.Size = new System.Drawing.Size(31, 17);
            this.zIonCheckBox.TabIndex = 14;
            this.zIonCheckBox.Text = "z";
            this.zIonCheckBox.UseVisualStyleBackColor = true;
            // 
            // yIonCheckBox
            // 
            this.yIonCheckBox.AutoSize = true;
            this.yIonCheckBox.Location = new System.Drawing.Point(70, 40);
            this.yIonCheckBox.Name = "yIonCheckBox";
            this.yIonCheckBox.Size = new System.Drawing.Size(31, 17);
            this.yIonCheckBox.TabIndex = 13;
            this.yIonCheckBox.Text = "y";
            this.yIonCheckBox.UseVisualStyleBackColor = true;
            // 
            // bIonCheckBox
            // 
            this.bIonCheckBox.AutoSize = true;
            this.bIonCheckBox.Location = new System.Drawing.Point(20, 40);
            this.bIonCheckBox.Name = "bIonCheckBox";
            this.bIonCheckBox.Size = new System.Drawing.Size(32, 17);
            this.bIonCheckBox.TabIndex = 10;
            this.bIonCheckBox.Text = "b";
            this.bIonCheckBox.UseVisualStyleBackColor = true;
            // 
            // xIonCheckBox
            // 
            this.xIonCheckBox.AutoSize = true;
            this.xIonCheckBox.Location = new System.Drawing.Point(70, 20);
            this.xIonCheckBox.Name = "xIonCheckBox";
            this.xIonCheckBox.Size = new System.Drawing.Size(31, 17);
            this.xIonCheckBox.TabIndex = 12;
            this.xIonCheckBox.Text = "x";
            this.xIonCheckBox.UseVisualStyleBackColor = true;
            // 
            // cIonCheckBox
            // 
            this.cIonCheckBox.AutoSize = true;
            this.cIonCheckBox.Location = new System.Drawing.Point(20, 60);
            this.cIonCheckBox.Name = "cIonCheckBox";
            this.cIonCheckBox.Size = new System.Drawing.Size(32, 17);
            this.cIonCheckBox.TabIndex = 11;
            this.cIonCheckBox.Text = "c";
            this.cIonCheckBox.UseVisualStyleBackColor = true;
            // 
            // aIonCheckBox
            // 
            this.aIonCheckBox.AutoSize = true;
            this.aIonCheckBox.Location = new System.Drawing.Point(20, 20);
            this.aIonCheckBox.Name = "aIonCheckBox";
            this.aIonCheckBox.Size = new System.Drawing.Size(32, 17);
            this.aIonCheckBox.TabIndex = 9;
            this.aIonCheckBox.Text = "a";
            this.aIonCheckBox.UseVisualStyleBackColor = true;
            // 
            // MassSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.fragmentGroupBox);
            this.Controls.Add(this.precursorGroupBox);
            this.Name = "MassSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            this.fragmentGroupBox.ResumeLayout(false);
            this.fragmentGroupBox.PerformLayout();
            this.precursorGroupBox.ResumeLayout(false);
            this.precursorGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.precursorMassTolTextBox)).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox fragmentGroupBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox precursorGroupBox;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.ComboBox precursorMassUnitCombo;
        private System.Windows.Forms.ComboBox precursorTolTypeCombo;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ComboBox precursorMassTypeCombo;
        private System.Windows.Forms.ComboBox precursorIsotopeErrorCombo;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox fragmentBinSizeTextBox;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox fragmentOffsetTextBox;
        private System.Windows.Forms.ComboBox fragmentMassTypeCombo;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.CheckBox xIonCheckBox;
        private System.Windows.Forms.CheckBox cIonCheckBox;
        private System.Windows.Forms.CheckBox aIonCheckBox;
        private System.Windows.Forms.CheckBox zIonCheckBox;
        private System.Windows.Forms.CheckBox yIonCheckBox;
        private System.Windows.Forms.CheckBox bIonCheckBox;
        private System.Windows.Forms.CheckBox useNLCheckBox;
        private System.Windows.Forms.CheckBox flankCheckBox;
        private System.Windows.Forms.NumericUpDown precursorMassTolTextBox;

    }
}
