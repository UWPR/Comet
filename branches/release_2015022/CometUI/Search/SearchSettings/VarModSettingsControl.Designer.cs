namespace CometUI.Search.SearchSettings
{
    partial class VarModSettingsControl
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
            this.label6 = new System.Windows.Forms.Label();
            this.maxModsInPeptideTextBox = new System.Windows.Forms.NumericUpDown();
            this.varModsListBox = new System.Windows.Forms.ListBox();
            this.label1 = new System.Windows.Forms.Label();
            this.addVarModBtn = new System.Windows.Forms.Button();
            this.editVarModBtn = new System.Windows.Forms.Button();
            this.removeVarModBtn = new System.Windows.Forms.Button();
            this.requireVarModCheckBox = new System.Windows.Forms.CheckBox();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).BeginInit();
            this.SuspendLayout();
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(23, 229);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(107, 13);
            this.label6.TabIndex = 15;
            this.label6.Text = "Max mods in peptide:";
            // 
            // maxModsInPeptideTextBox
            // 
            this.maxModsInPeptideTextBox.Location = new System.Drawing.Point(26, 245);
            this.maxModsInPeptideTextBox.Maximum = new decimal(new int[] {
            20,
            0,
            0,
            0});
            this.maxModsInPeptideTextBox.Name = "maxModsInPeptideTextBox";
            this.maxModsInPeptideTextBox.Size = new System.Drawing.Size(104, 20);
            this.maxModsInPeptideTextBox.TabIndex = 32;
            // 
            // varModsListBox
            // 
            this.varModsListBox.FormattingEnabled = true;
            this.varModsListBox.Location = new System.Drawing.Point(26, 53);
            this.varModsListBox.Name = "varModsListBox";
            this.varModsListBox.Size = new System.Drawing.Size(382, 134);
            this.varModsListBox.TabIndex = 33;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(23, 37);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(77, 13);
            this.label1.TabIndex = 34;
            this.label1.Text = "Variable Mods:";
            // 
            // addVarModBtn
            // 
            this.addVarModBtn.Location = new System.Drawing.Point(427, 53);
            this.addVarModBtn.Name = "addVarModBtn";
            this.addVarModBtn.Size = new System.Drawing.Size(75, 23);
            this.addVarModBtn.TabIndex = 35;
            this.addVarModBtn.Text = "&Add...";
            this.addVarModBtn.UseVisualStyleBackColor = true;
            this.addVarModBtn.Click += new System.EventHandler(this.AddVarModBtnClick);
            // 
            // editVarModBtn
            // 
            this.editVarModBtn.Location = new System.Drawing.Point(427, 82);
            this.editVarModBtn.Name = "editVarModBtn";
            this.editVarModBtn.Size = new System.Drawing.Size(75, 23);
            this.editVarModBtn.TabIndex = 36;
            this.editVarModBtn.Text = "&Edit...";
            this.editVarModBtn.UseVisualStyleBackColor = true;
            this.editVarModBtn.Click += new System.EventHandler(this.EditVarModBtnClick);
            // 
            // removeVarModBtn
            // 
            this.removeVarModBtn.Location = new System.Drawing.Point(427, 111);
            this.removeVarModBtn.Name = "removeVarModBtn";
            this.removeVarModBtn.Size = new System.Drawing.Size(75, 23);
            this.removeVarModBtn.TabIndex = 37;
            this.removeVarModBtn.Text = "&Remove";
            this.removeVarModBtn.UseVisualStyleBackColor = true;
            this.removeVarModBtn.Click += new System.EventHandler(this.RemoveVarModBtnClick);
            // 
            // requireVarModCheckBox
            // 
            this.requireVarModCheckBox.AutoSize = true;
            this.requireVarModCheckBox.Location = new System.Drawing.Point(26, 288);
            this.requireVarModCheckBox.Name = "requireVarModCheckBox";
            this.requireVarModCheckBox.Size = new System.Drawing.Size(126, 17);
            this.requireVarModCheckBox.TabIndex = 38;
            this.requireVarModCheckBox.Text = "Require variable mod";
            this.requireVarModCheckBox.UseVisualStyleBackColor = true;
            // 
            // VarModSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.maxModsInPeptideTextBox);
            this.Controls.Add(this.requireVarModCheckBox);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.removeVarModBtn);
            this.Controls.Add(this.editVarModBtn);
            this.Controls.Add(this.addVarModBtn);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.varModsListBox);
            this.Name = "VarModSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.NumericUpDown maxModsInPeptideTextBox;
        private System.Windows.Forms.ListBox varModsListBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button addVarModBtn;
        private System.Windows.Forms.Button editVarModBtn;
        private System.Windows.Forms.Button removeVarModBtn;
        private System.Windows.Forms.CheckBox requireVarModCheckBox;

    }
}
