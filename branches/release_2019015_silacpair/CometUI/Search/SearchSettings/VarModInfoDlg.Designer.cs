using CometUI.SharedUI;

namespace CometUI.Search.SearchSettings
{
    partial class VarModInfoDlg
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
         this.components = new System.ComponentModel.Container();
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(VarModInfoDlg));
         this.varModInfoDlgMainSplitContainer = new System.Windows.Forms.SplitContainer();
         this.binaryModGroupLabel = new System.Windows.Forms.Label();
         this.requireModCheckBox = new System.Windows.Forms.CheckBox();
         this.termDistLabel = new System.Windows.Forms.Label();
         this.whichTermCombo = new System.Windows.Forms.ComboBox();
         this.whichTermLabel = new System.Windows.Forms.Label();
         this.maxModsLabel = new System.Windows.Forms.Label();
         this.massDiffLabel = new System.Windows.Forms.Label();
         this.residueLabel = new System.Windows.Forms.Label();
         this.residueTextBox = new System.Windows.Forms.TextBox();
         this.okBtn = new System.Windows.Forms.Button();
         this.cancelBtn = new System.Windows.Forms.Button();
         this.varModInfoDlgToolTip = new System.Windows.Forms.ToolTip(this.components);
         this.label1 = new System.Windows.Forms.Label();
         this.binaryModGroup = new System.Windows.Forms.NumericUpDown();
         this.maxModsNumericTextBox = new CometUI.SharedUI.NumericTextBox();
         this.massDiffNumericTextBox = new CometUI.SharedUI.NumericTextBox();
         this.termDistNumericTextBox = new CometUI.SharedUI.NumericTextBox();
         ((System.ComponentModel.ISupportInitialize)(this.varModInfoDlgMainSplitContainer)).BeginInit();
         this.varModInfoDlgMainSplitContainer.Panel1.SuspendLayout();
         this.varModInfoDlgMainSplitContainer.Panel2.SuspendLayout();
         this.varModInfoDlgMainSplitContainer.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.binaryModGroup)).BeginInit();
         this.SuspendLayout();
         // 
         // varModInfoDlgMainSplitContainer
         // 
         this.varModInfoDlgMainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
         this.varModInfoDlgMainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
         this.varModInfoDlgMainSplitContainer.IsSplitterFixed = true;
         this.varModInfoDlgMainSplitContainer.Location = new System.Drawing.Point(0, 0);
         this.varModInfoDlgMainSplitContainer.Name = "varModInfoDlgMainSplitContainer";
         this.varModInfoDlgMainSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
         // 
         // varModInfoDlgMainSplitContainer.Panel1
         // 
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.binaryModGroup);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.label1);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.binaryModGroupLabel);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.requireModCheckBox);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.maxModsNumericTextBox);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.massDiffNumericTextBox);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.termDistNumericTextBox);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.termDistLabel);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.whichTermCombo);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.whichTermLabel);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.maxModsLabel);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.massDiffLabel);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.residueLabel);
         this.varModInfoDlgMainSplitContainer.Panel1.Controls.Add(this.residueTextBox);
         // 
         // varModInfoDlgMainSplitContainer.Panel2
         // 
         this.varModInfoDlgMainSplitContainer.Panel2.Controls.Add(this.okBtn);
         this.varModInfoDlgMainSplitContainer.Panel2.Controls.Add(this.cancelBtn);
         this.varModInfoDlgMainSplitContainer.Size = new System.Drawing.Size(472, 249);
         this.varModInfoDlgMainSplitContainer.SplitterDistance = 201;
         this.varModInfoDlgMainSplitContainer.TabIndex = 0;
         // 
         // binaryModGroupLabel
         // 
         this.binaryModGroupLabel.AutoSize = true;
         this.binaryModGroupLabel.Location = new System.Drawing.Point(245, 149);
         this.binaryModGroupLabel.Name = "binaryModGroupLabel";
         this.binaryModGroupLabel.Size = new System.Drawing.Size(128, 13);
         this.binaryModGroupLabel.TabIndex = 24;
         this.binaryModGroupLabel.Text = "Binary modification group:";
         this.varModInfoDlgToolTip.SetToolTip(this.binaryModGroupLabel, "The terminus the distance constraint is applied to (N-terminus or C-terminus). Th" +
        "is is only valid if a distance constraint is specified.");
         // 
         // requireModCheckBox
         // 
         this.requireModCheckBox.AutoSize = true;
         this.requireModCheckBox.Location = new System.Drawing.Point(27, 149);
         this.requireModCheckBox.Name = "requireModCheckBox";
         this.requireModCheckBox.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
         this.requireModCheckBox.Size = new System.Drawing.Size(144, 17);
         this.requireModCheckBox.TabIndex = 22;
         this.requireModCheckBox.Text = ":Require this modification";
         this.requireModCheckBox.UseVisualStyleBackColor = true;
         // 
         // termDistLabel
         // 
         this.termDistLabel.AutoSize = true;
         this.termDistLabel.Location = new System.Drawing.Point(245, 33);
         this.termDistLabel.Name = "termDistLabel";
         this.termDistLabel.Size = new System.Drawing.Size(87, 13);
         this.termDistLabel.TabIndex = 21;
         this.termDistLabel.Text = "Termini distance:";
         this.varModInfoDlgToolTip.SetToolTip(this.termDistLabel, "The distance the modification is applied to from the respective terminus. Leave t" +
        "his blank for no distance constraint.");
         // 
         // whichTermCombo
         // 
         this.whichTermCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.whichTermCombo.FormattingEnabled = true;
         this.whichTermCombo.Location = new System.Drawing.Point(338, 69);
         this.whichTermCombo.Name = "whichTermCombo";
         this.whichTermCombo.Size = new System.Drawing.Size(99, 21);
         this.whichTermCombo.TabIndex = 6;
         this.varModInfoDlgToolTip.SetToolTip(this.whichTermCombo, "The terminus the distance constraint is applied to (N-terminus or C-terminus). Th" +
        "is is only valid if a distance constraint is specified.");
         this.whichTermCombo.SelectedIndexChanged += new System.EventHandler(this.WhichTermComboSelectedIndexChanged);
         // 
         // whichTermLabel
         // 
         this.whichTermLabel.AutoSize = true;
         this.whichTermLabel.Location = new System.Drawing.Point(245, 73);
         this.whichTermLabel.Name = "whichTermLabel";
         this.whichTermLabel.Size = new System.Drawing.Size(74, 13);
         this.whichTermLabel.TabIndex = 19;
         this.whichTermLabel.Text = "Which termini:";
         this.varModInfoDlgToolTip.SetToolTip(this.whichTermLabel, "The terminus the distance constraint is applied to (N-terminus or C-terminus). Th" +
        "is is only valid if a distance constraint is specified.");
         // 
         // maxModsLabel
         // 
         this.maxModsLabel.AutoSize = true;
         this.maxModsLabel.Location = new System.Drawing.Point(24, 113);
         this.maxModsLabel.Name = "maxModsLabel";
         this.maxModsLabel.Size = new System.Drawing.Size(129, 13);
         this.maxModsLabel.TabIndex = 18;
         this.maxModsLabel.Text = "Max # of mods in peptide:";
         this.varModInfoDlgToolTip.SetToolTip(this.maxModsLabel, "An integer specifying the maximum number of modified residues possible in a pepti" +
        "de for this modification.");
         // 
         // massDiffLabel
         // 
         this.massDiffLabel.AutoSize = true;
         this.massDiffLabel.Location = new System.Drawing.Point(24, 74);
         this.massDiffLabel.Name = "massDiffLabel";
         this.massDiffLabel.Size = new System.Drawing.Size(94, 13);
         this.massDiffLabel.TabIndex = 14;
         this.massDiffLabel.Text = "Modification mass:";
         this.varModInfoDlgToolTip.SetToolTip(this.massDiffLabel, "A decimal value specifying the modification mass difference.");
         // 
         // residueLabel
         // 
         this.residueLabel.AutoSize = true;
         this.residueLabel.Location = new System.Drawing.Point(58, 33);
         this.residueLabel.Name = "residueLabel";
         this.residueLabel.Size = new System.Drawing.Size(60, 13);
         this.residueLabel.TabIndex = 13;
         this.residueLabel.Text = "Residue(s):";
         this.varModInfoDlgToolTip.SetToolTip(this.residueLabel, "The residue(s) that the modifications will be applied to.");
         // 
         // residueTextBox
         // 
         this.residueTextBox.Location = new System.Drawing.Point(124, 30);
         this.residueTextBox.Name = "residueTextBox";
         this.residueTextBox.Size = new System.Drawing.Size(85, 20);
         this.residueTextBox.TabIndex = 1;
         this.varModInfoDlgToolTip.SetToolTip(this.residueTextBox, "The residue(s) that the modifications will be applied to.");
         this.residueTextBox.TextChanged += new System.EventHandler(this.ResidueTextBoxTextChanged);
         this.residueTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.ResidueTextBoxKeyPress);
         // 
         // okBtn
         // 
         this.okBtn.Location = new System.Drawing.Point(292, 9);
         this.okBtn.Name = "okBtn";
         this.okBtn.Size = new System.Drawing.Size(75, 23);
         this.okBtn.TabIndex = 0;
         this.okBtn.Text = "&OK";
         this.okBtn.UseVisualStyleBackColor = true;
         this.okBtn.Click += new System.EventHandler(this.OKBtnClick);
         // 
         // cancelBtn
         // 
         this.cancelBtn.Location = new System.Drawing.Point(373, 9);
         this.cancelBtn.Name = "cancelBtn";
         this.cancelBtn.Size = new System.Drawing.Size(75, 23);
         this.cancelBtn.TabIndex = 8;
         this.cancelBtn.Text = "&Cancel";
         this.cancelBtn.UseVisualStyleBackColor = true;
         this.cancelBtn.Click += new System.EventHandler(this.CancelBtnClick);
         // 
         // varModInfoDlgToolTip
         // 
         this.varModInfoDlgToolTip.AutomaticDelay = 400;
         this.varModInfoDlgToolTip.IsBalloon = true;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(265, 162);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(85, 13);
         this.label1.TabIndex = 25;
         this.label1.Text = "(0=variable mod)";
         this.varModInfoDlgToolTip.SetToolTip(this.label1, "The terminus the distance constraint is applied to (N-terminus or C-terminus). Th" +
        "is is only valid if a distance constraint is specified.");
         // 
         // binaryModGroup
         // 
         this.binaryModGroup.Location = new System.Drawing.Point(379, 147);
         this.binaryModGroup.Maximum = new decimal(new int[] {
            9,
            0,
            0,
            0});
         this.binaryModGroup.Name = "binaryModGroup";
         this.binaryModGroup.Size = new System.Drawing.Size(58, 20);
         this.binaryModGroup.TabIndex = 33;
         // 
         // maxModsNumericTextBox
         // 
         this.maxModsNumericTextBox.AllowDecimal = false;
         this.maxModsNumericTextBox.AllowGroupSeparator = false;
         this.maxModsNumericTextBox.AllowNegative = false;
         this.maxModsNumericTextBox.AllowSpace = false;
         this.maxModsNumericTextBox.Location = new System.Drawing.Point(159, 110);
         this.maxModsNumericTextBox.Name = "maxModsNumericTextBox";
         this.maxModsNumericTextBox.Size = new System.Drawing.Size(50, 20);
         this.maxModsNumericTextBox.TabIndex = 3;
         this.varModInfoDlgToolTip.SetToolTip(this.maxModsNumericTextBox, "An integer specifying the maximum number of modified residues possible in a pepti" +
        "de for this modification.");
         this.maxModsNumericTextBox.TextChanged += new System.EventHandler(this.MaxModsNumericTextBoxTextChanged);
         // 
         // massDiffNumericTextBox
         // 
         this.massDiffNumericTextBox.AllowDecimal = true;
         this.massDiffNumericTextBox.AllowGroupSeparator = false;
         this.massDiffNumericTextBox.AllowNegative = false;
         this.massDiffNumericTextBox.AllowSpace = false;
         this.massDiffNumericTextBox.Location = new System.Drawing.Point(124, 70);
         this.massDiffNumericTextBox.Name = "massDiffNumericTextBox";
         this.massDiffNumericTextBox.Size = new System.Drawing.Size(85, 20);
         this.massDiffNumericTextBox.TabIndex = 2;
         this.varModInfoDlgToolTip.SetToolTip(this.massDiffNumericTextBox, "A decimal value specifying the modification mass difference.");
         this.massDiffNumericTextBox.TextChanged += new System.EventHandler(this.MassDiffNumericTextBoxTextChanged);
         // 
         // termDistNumericTextBox
         // 
         this.termDistNumericTextBox.AllowDecimal = false;
         this.termDistNumericTextBox.AllowGroupSeparator = false;
         this.termDistNumericTextBox.AllowNegative = false;
         this.termDistNumericTextBox.AllowSpace = false;
         this.termDistNumericTextBox.Location = new System.Drawing.Point(338, 30);
         this.termDistNumericTextBox.Name = "termDistNumericTextBox";
         this.termDistNumericTextBox.Size = new System.Drawing.Size(99, 20);
         this.termDistNumericTextBox.TabIndex = 5;
         this.varModInfoDlgToolTip.SetToolTip(this.termDistNumericTextBox, "The distance the modification is applied to from the respective terminus. Leave t" +
        "his blank for no distance constraint.");
         this.termDistNumericTextBox.TextChanged += new System.EventHandler(this.TermDistNumericTextBoxTextChanged);
         // 
         // VarModInfoDlg
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(472, 249);
         this.Controls.Add(this.varModInfoDlgMainSplitContainer);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
         this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
         this.Name = "VarModInfoDlg";
         this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
         this.varModInfoDlgMainSplitContainer.Panel1.ResumeLayout(false);
         this.varModInfoDlgMainSplitContainer.Panel1.PerformLayout();
         this.varModInfoDlgMainSplitContainer.Panel2.ResumeLayout(false);
         ((System.ComponentModel.ISupportInitialize)(this.varModInfoDlgMainSplitContainer)).EndInit();
         this.varModInfoDlgMainSplitContainer.ResumeLayout(false);
         ((System.ComponentModel.ISupportInitialize)(this.binaryModGroup)).EndInit();
         this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer varModInfoDlgMainSplitContainer;
        private NumericTextBox termDistNumericTextBox;
        private System.Windows.Forms.Label termDistLabel;
        private System.Windows.Forms.ComboBox whichTermCombo;
        private System.Windows.Forms.Label whichTermLabel;
        private System.Windows.Forms.Label maxModsLabel;
        private System.Windows.Forms.Label massDiffLabel;
        private System.Windows.Forms.Label residueLabel;
        private System.Windows.Forms.TextBox residueTextBox;
        private System.Windows.Forms.Button okBtn;
        private System.Windows.Forms.Button cancelBtn;
        private NumericTextBox massDiffNumericTextBox;
        private NumericTextBox maxModsNumericTextBox;
        private System.Windows.Forms.ToolTip varModInfoDlgToolTip;
        private System.Windows.Forms.CheckBox requireModCheckBox;
        private System.Windows.Forms.Label binaryModGroupLabel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown binaryModGroup;
    }
}