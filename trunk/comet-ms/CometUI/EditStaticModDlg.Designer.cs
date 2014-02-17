namespace CometUI
{
    partial class EditStaticModDlg
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
            this.editStaticModSplitContainer = new System.Windows.Forms.SplitContainer();
            this.residueTextBox = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.avgTextBox = new System.Windows.Forms.TextBox();
            this.monoisotopicTextBox = new System.Windows.Forms.TextBox();
            this.avgRadioButton = new System.Windows.Forms.RadioButton();
            this.monoisotopicRadioButton = new System.Windows.Forms.RadioButton();
            this.staticModNameTextBox = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.okButton = new System.Windows.Forms.Button();
            this.cancelButton = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.editStaticModSplitContainer)).BeginInit();
            this.editStaticModSplitContainer.Panel1.SuspendLayout();
            this.editStaticModSplitContainer.Panel2.SuspendLayout();
            this.editStaticModSplitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // editStaticModSplitContainer
            // 
            this.editStaticModSplitContainer.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.editStaticModSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.editStaticModSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.editStaticModSplitContainer.IsSplitterFixed = true;
            this.editStaticModSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.editStaticModSplitContainer.Name = "editStaticModSplitContainer";
            this.editStaticModSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // editStaticModSplitContainer.Panel1
            // 
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.residueTextBox);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.label2);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.avgTextBox);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.monoisotopicTextBox);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.avgRadioButton);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.monoisotopicRadioButton);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.staticModNameTextBox);
            this.editStaticModSplitContainer.Panel1.Controls.Add(this.label1);
            // 
            // editStaticModSplitContainer.Panel2
            // 
            this.editStaticModSplitContainer.Panel2.Controls.Add(this.okButton);
            this.editStaticModSplitContainer.Panel2.Controls.Add(this.cancelButton);
            this.editStaticModSplitContainer.Size = new System.Drawing.Size(408, 216);
            this.editStaticModSplitContainer.SplitterDistance = 169;
            this.editStaticModSplitContainer.TabIndex = 0;
            // 
            // residueTextBox
            // 
            this.residueTextBox.Enabled = false;
            this.residueTextBox.Location = new System.Drawing.Point(323, 21);
            this.residueTextBox.Name = "residueTextBox";
            this.residueTextBox.Size = new System.Drawing.Size(64, 20);
            this.residueTextBox.TabIndex = 7;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(268, 24);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(49, 13);
            this.label2.TabIndex = 6;
            this.label2.Text = "Residue:";
            // 
            // avgTextBox
            // 
            this.avgTextBox.Enabled = false;
            this.avgTextBox.Location = new System.Drawing.Point(160, 99);
            this.avgTextBox.Name = "avgTextBox";
            this.avgTextBox.Size = new System.Drawing.Size(86, 20);
            this.avgTextBox.TabIndex = 5;
            // 
            // monoisotopicTextBox
            // 
            this.monoisotopicTextBox.Enabled = false;
            this.monoisotopicTextBox.Location = new System.Drawing.Point(160, 67);
            this.monoisotopicTextBox.Name = "monoisotopicTextBox";
            this.monoisotopicTextBox.Size = new System.Drawing.Size(86, 20);
            this.monoisotopicTextBox.TabIndex = 4;
            // 
            // avgRadioButton
            // 
            this.avgRadioButton.AutoSize = true;
            this.avgRadioButton.Location = new System.Drawing.Point(25, 100);
            this.avgRadioButton.Name = "avgRadioButton";
            this.avgRadioButton.Size = new System.Drawing.Size(116, 17);
            this.avgRadioButton.TabIndex = 3;
            this.avgRadioButton.TabStop = true;
            this.avgRadioButton.Text = "Use average mass:";
            this.avgRadioButton.UseVisualStyleBackColor = true;
            // 
            // monoisotopicRadioButton
            // 
            this.monoisotopicRadioButton.AutoSize = true;
            this.monoisotopicRadioButton.Location = new System.Drawing.Point(25, 68);
            this.monoisotopicRadioButton.Name = "monoisotopicRadioButton";
            this.monoisotopicRadioButton.Size = new System.Drawing.Size(139, 17);
            this.monoisotopicRadioButton.TabIndex = 2;
            this.monoisotopicRadioButton.TabStop = true;
            this.monoisotopicRadioButton.Text = "Use monoisotopic mass:";
            this.monoisotopicRadioButton.UseVisualStyleBackColor = true;
            // 
            // staticModNameTextBox
            // 
            this.staticModNameTextBox.Enabled = false;
            this.staticModNameTextBox.Location = new System.Drawing.Point(66, 21);
            this.staticModNameTextBox.Name = "staticModNameTextBox";
            this.staticModNameTextBox.Size = new System.Drawing.Size(156, 20);
            this.staticModNameTextBox.TabIndex = 1;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(22, 24);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(38, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Name:";
            // 
            // okButton
            // 
            this.okButton.Location = new System.Drawing.Point(231, 8);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(75, 23);
            this.okButton.TabIndex = 1;
            this.okButton.Text = "OK";
            this.okButton.UseVisualStyleBackColor = true;
            // 
            // cancelButton
            // 
            this.cancelButton.Location = new System.Drawing.Point(312, 8);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 0;
            this.cancelButton.Text = "Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.CancelButtonClick);
            // 
            // EditStaticModDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(408, 216);
            this.Controls.Add(this.editStaticModSplitContainer);
            this.Name = "EditStaticModDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Edit Static Mod";
            this.editStaticModSplitContainer.Panel1.ResumeLayout(false);
            this.editStaticModSplitContainer.Panel1.PerformLayout();
            this.editStaticModSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.editStaticModSplitContainer)).EndInit();
            this.editStaticModSplitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer editStaticModSplitContainer;
        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.Button cancelButton;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox staticModNameTextBox;
        private System.Windows.Forms.RadioButton avgRadioButton;
        private System.Windows.Forms.RadioButton monoisotopicRadioButton;
        private System.Windows.Forms.TextBox avgTextBox;
        private System.Windows.Forms.TextBox monoisotopicTextBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox residueTextBox;
    }
}