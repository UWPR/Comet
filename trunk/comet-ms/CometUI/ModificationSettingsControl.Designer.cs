namespace CometUI
{
    partial class ModificationSettingsControl
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
            this.varModsDataGridView = new System.Windows.Forms.DataGridView();
            this.variableNTerminusTextBox = new System.Windows.Forms.TextBox();
            this.variableCTerminusTextBox = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.variableNTerminusDistTextBox = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.variableCTerminusDistTextBox = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.maxModsInPeptideTextBox = new System.Windows.Forms.NumericUpDown();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.button1 = new System.Windows.Forms.Button();
            this.checkedListBox1 = new System.Windows.Forms.CheckedListBox();
            this.varModsResidueCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsMassCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsBinaryModCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsMaxModsPerPeptide = new System.Windows.Forms.DataGridViewTextBoxColumn();
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).BeginInit();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // varModsDataGridView
            // 
            this.varModsDataGridView.AllowUserToAddRows = false;
            this.varModsDataGridView.AllowUserToDeleteRows = false;
            this.varModsDataGridView.AllowUserToResizeRows = false;
            this.varModsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.varModsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.varModsResidueCol,
            this.varModsMassCol,
            this.varModsBinaryModCol,
            this.varModsMaxModsPerPeptide});
            this.varModsDataGridView.Location = new System.Drawing.Point(17, 31);
            this.varModsDataGridView.Name = "varModsDataGridView";
            this.varModsDataGridView.Size = new System.Drawing.Size(440, 108);
            this.varModsDataGridView.TabIndex = 0;
            // 
            // variableNTerminusTextBox
            // 
            this.variableNTerminusTextBox.Location = new System.Drawing.Point(80, 159);
            this.variableNTerminusTextBox.Name = "variableNTerminusTextBox";
            this.variableNTerminusTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableNTerminusTextBox.TabIndex = 7;
            // 
            // variableCTerminusTextBox
            // 
            this.variableCTerminusTextBox.Location = new System.Drawing.Point(80, 185);
            this.variableCTerminusTextBox.Name = "variableCTerminusTextBox";
            this.variableCTerminusTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableCTerminusTextBox.TabIndex = 8;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(14, 162);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(60, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "N-terminus:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(14, 188);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(59, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "C-terminus:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(162, 162);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(52, 13);
            this.label4.TabIndex = 12;
            this.label4.Text = "Distance:";
            // 
            // variableNTerminusDistTextBox
            // 
            this.variableNTerminusDistTextBox.Location = new System.Drawing.Point(220, 159);
            this.variableNTerminusDistTextBox.Name = "variableNTerminusDistTextBox";
            this.variableNTerminusDistTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableNTerminusDistTextBox.TabIndex = 11;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(162, 188);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(52, 13);
            this.label5.TabIndex = 14;
            this.label5.Text = "Distance:";
            // 
            // variableCTerminusDistTextBox
            // 
            this.variableCTerminusDistTextBox.Location = new System.Drawing.Point(220, 185);
            this.variableCTerminusDistTextBox.Name = "variableCTerminusDistTextBox";
            this.variableCTerminusDistTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableCTerminusDistTextBox.TabIndex = 13;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(353, 162);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(107, 13);
            this.label6.TabIndex = 15;
            this.label6.Text = "Max mods in peptide:";
            // 
            // maxModsInPeptideTextBox
            // 
            this.maxModsInPeptideTextBox.Location = new System.Drawing.Point(356, 178);
            this.maxModsInPeptideTextBox.Maximum = new decimal(new int[] {
            20,
            0,
            0,
            0});
            this.maxModsInPeptideTextBox.Name = "maxModsInPeptideTextBox";
            this.maxModsInPeptideTextBox.Size = new System.Drawing.Size(104, 20);
            this.maxModsInPeptideTextBox.TabIndex = 32;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.varModsDataGridView);
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Controls.Add(this.maxModsInPeptideTextBox);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.variableNTerminusTextBox);
            this.groupBox1.Controls.Add(this.label5);
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.variableCTerminusDistTextBox);
            this.groupBox1.Controls.Add(this.variableNTerminusDistTextBox);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.variableCTerminusTextBox);
            this.groupBox1.Location = new System.Drawing.Point(26, 17);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(478, 231);
            this.groupBox1.TabIndex = 33;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Variable Mods";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.button1);
            this.groupBox2.Controls.Add(this.checkedListBox1);
            this.groupBox2.Location = new System.Drawing.Point(26, 275);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(478, 136);
            this.groupBox2.TabIndex = 34;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Static Mods";
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(185, 28);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 1;
            this.button1.Text = "Edit";
            this.button1.UseVisualStyleBackColor = true;
            // 
            // checkedListBox1
            // 
            this.checkedListBox1.FormattingEnabled = true;
            this.checkedListBox1.Items.AddRange(new object[] {
            "Glycine (G)",
            "Alanine (A)",
            "Serine (S)",
            "Proline (P)",
            "Valine (V)",
            "Threonine (T)",
            "Cysteine (C)",
            "Leucine (L)",
            "Isoleucine (I)",
            "Asparagine (N)",
            "Aspartic Acid (D)",
            "Glutamine (Q)",
            "Lysine (K)",
            "Glutamic Acid (E)",
            "Methionine (M)",
            "Ornithine (O)",
            "Histidine (H)",
            "Phenylalanine (F)",
            "Arginine (R)",
            "Tyrosine (Y)",
            "Tryptophan (W)",
            "User Amino Acid (B)",
            "User Amino Acid (J)",
            "User Amino Acid (U)",
            "User Amino Acid (X)",
            "User Amino Acid (Z)"});
            this.checkedListBox1.Location = new System.Drawing.Point(17, 28);
            this.checkedListBox1.Name = "checkedListBox1";
            this.checkedListBox1.Size = new System.Drawing.Size(162, 79);
            this.checkedListBox1.TabIndex = 0;
            // 
            // varModsResidueCol
            // 
            this.varModsResidueCol.HeaderText = "Residue";
            this.varModsResidueCol.Name = "varModsResidueCol";
            this.varModsResidueCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsResidueCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsResidueCol.Width = 95;
            // 
            // varModsMassCol
            // 
            this.varModsMassCol.HeaderText = "Mass";
            this.varModsMassCol.Name = "varModsMassCol";
            this.varModsMassCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsMassCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsMassCol.Width = 95;
            // 
            // varModsBinaryModCol
            // 
            this.varModsBinaryModCol.HeaderText = "Binary Mod";
            this.varModsBinaryModCol.Name = "varModsBinaryModCol";
            this.varModsBinaryModCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsBinaryModCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsBinaryModCol.Width = 95;
            // 
            // varModsMaxModsPerPeptide
            // 
            this.varModsMaxModsPerPeptide.HeaderText = "Max Mods";
            this.varModsMaxModsPerPeptide.Name = "varModsMaxModsPerPeptide";
            this.varModsMaxModsPerPeptide.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsMaxModsPerPeptide.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsMaxModsPerPeptide.Width = 95;
            // 
            // ModificationSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Name = "ModificationSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataGridView varModsDataGridView;
        private System.Windows.Forms.TextBox variableNTerminusTextBox;
        private System.Windows.Forms.TextBox variableCTerminusTextBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox variableNTerminusDistTextBox;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox variableCTerminusDistTextBox;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.NumericUpDown maxModsInPeptideTextBox;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckedListBox checkedListBox1;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsResidueCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMassCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsBinaryModCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMaxModsPerPeptide;

    }
}
